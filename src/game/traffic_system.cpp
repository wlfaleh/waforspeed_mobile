// WaForSpeed — module « trafic » (WFS-08) : voir traffic_system.h.
#include "traffic_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../core/log.h"
#include "../render/model_loader.h"
#include "../render/effets_systeme.h"
#include "../render/renderer.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

static vec3 hexLin(unsigned h) {
    auto lin = [](float c) { return std::pow(c, 2.2f); };
    return {lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
}

class TrafficSystem : public System {
public:
    const char* name() const override { return "trafic"; }

    void init(Renderer& r, const GameView& g) override {
        // modèles natifs, partagés par toutes les voitures du trafic
        for (int i = 0; i < TRAFIC_MODELES; i++) loadCarModel(r, std::string(CARS[i].file), models_[i]);   // WFS-13
        Material m; m.flags = MAT_SHADOWBLOB | MAT_BLEND; m.color = {0, 0, 0, 0.75f};
        shadowMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        std::vector<Vertex> v = {{-0.5f, 0.02f, -0.5f, 0, 1, 0, 0, 0}, {0.5f, 0.02f, -0.5f, 0, 1, 0, 1, 0},
                                 {0.5f, 0.02f, 0.5f, 0, 1, 0, 1, 1}, {-0.5f, 0.02f, 0.5f, 0, 1, 0, 0, 1}};
        shadow_ = r.makeMesh(v, {0, 1, 2, 0, 2, 3});
        // trafic de la carte (resetRace du jeu web) ; « Rejouer » (WFS-12) repart de la même graine.
        // Graine tirée au hasard à chaque course : avec le menu (WFS-11).
        trafficN_ = g.map ? g.map->trafficN : 42; track_ = g.track;
        traffic_.reset(*g.track, seed_, trafficN_, g.player ? g.player->position : 0);
        stats_ = {};
        active_ = this;
        // banc d'essai (rendu hors écran) : garder du trafic dans le champ pendant la course (WFS-33, RG-02)
        benchDense_ = r.vk().offscreen; denseT_ = 0;
        initVivant(r);   // WFS-67
    }

    void shutdown(Renderer& r) override {
        for (auto& m : models_) r.destroy(m);
        r.destroy(shadow_);
        shutdownVivant(r);   // WFS-67
        if (active_ == this) active_ = nullptr;
    }

    // après chaque pas de conduite : le trafic avance (pas de 1/60 s), puis chocs et quasi-contacts
    void collide(double dt, Player& pl, const GameView& g) override {
        if (benchReq_) { applyBench(pl, *g.track); benchReq_ = false; }
        if (pelotonReq_) { applyPeloton(pl, *g.track); pelotonReq_ = false; }
        if (benchDense_) keepTrafficInView(dt, pl, *g.track);
        // règles de course (WFS-12) : voiture immobile pendant le compte à rebours, chocs seulement en course
        Course* course = activeCourseMut();
        if (course) course->hold(pl);
        const bool playing = !course || course->playing();
        TrafficView pv;
        pv.position = pl.position; pv.playerX = pl.playerX; pv.speed = pl.speed;
        pv.myLen = webCarLength(pl.car->id, pl.car->d.L);
        traffic_.advance(dt, pv);
        if (stops_.size() != traffic_.cars.size()) stops_.resize(traffic_.cars.size());   // WFS-67 : feux stop
        for (size_t i = 0; i < traffic_.cars.size(); i++) stops_[i].maj(traffic_.cars[i].speed, dt);
        if (!playing) { traffic_.watch(pl); return; }
        HitEvents e = traffic_.collide(pl, dt);
        if (course) course->applyHits(e);   // quasi-contacts, combos, accidents
        stats_.nearMisses += e.nearMiss; stats_.accidents += e.accidents; stats_.scrapes += e.scrapes; stats_.bumps += e.bumps;
        if (e.accidents) holdLane_ = false;
    }

    // autopilot() du jeu web : file la plus dégagée toutes les 0,4 s ; on freine derrière plus lent
    void autopilot(double dt, DriveInput& in, const GameView& g) override {
        const Player& pl = *g.player; Track& t = *g.track;
        if (holdLane_) {   // scène de choc du banc d'essai : reste dans la file, pied au plancher
            in.steer = (float)clampf((float)((holdX_ - pl.playerX) * 5 - pl.heading * 4), -1.f, 1.f);
            in.gas = true; in.brake = false; in.nitro = false;
            return;
        }
        autoTimer_ -= dt;
        if (autoTimer_ <= 0) {
            autoTimer_ = 0.4;
            double bestLane = autoTarget_, bestGap = -1;
            for (double lx : LANE_X) {
                double gap = 9000;
                for (const auto& c : traffic_.cars) {
                    if (std::abs(c.offset - lx) > 0.3) continue;
                    const double dz = t.relZ(c.z - (pl.position + PLAYER_Z));
                    if (dz > -800 && dz < gap) gap = dz;
                }
                const double gg = gap + (lx == autoTarget_ ? 600 : 0);
                if (gg > bestGap) { bestGap = gg; bestLane = lx; }
            }
            autoTarget_ = bestLane;
        }
        in.steer = (float)clampf((float)((autoTarget_ - pl.playerX) * 5 - pl.heading * 4), -1.f, 1.f);
        double ahead = 1e9, aheadSpeed = 0;
        for (const auto& c : traffic_.cars) {
            if (std::abs(c.offset - pl.playerX) > 0.3) continue;
            const double dz = t.relZ(c.z - pl.position);
            if (dz > 0 && dz < ahead) { ahead = dz; aheadSpeed = c.speed; }
        }
        const double safe = (8 + pl.speed * M * 0.9) / M;
        in.gas = !(ahead < safe * 1.4 && aheadSpeed < pl.speed);
        in.brake = ahead < safe && aheadSpeed < pl.speed * 0.9;
        in.nitro = false;
    }

    // voitures visibles (30 m derrière à 300 m devant), les plus proches d'abord, 14 au plus (LOD : WFS-20)
    void render(Renderer& r, const GameView& g) override {
        if (g.sceneDediee) return;   // scène dédiée (garage…) : pas de trafic (WFS-44)
        const Player& pl = *g.player; Track& t = *g.track;
        struct Vis { double dz; const TrafficCar* c; };
        Vis vis[64]; int n = 0;
        for (const auto& c : traffic_.cars) {
            const double dz = t.relZ(c.z - pl.position);
            if (dz < -30 / M || dz > 300 / M || n >= 64) continue;
            vis[n++] = {std::abs(dz), &c};
        }
        std::sort(vis, vis + n, [](const Vis& a, const Vis& b) { return a.dz < b.dz; });
        n = std::min(n, 14);
        for (int k = 0; k < n; k++) {
            const TrafficCar& c = *vis[k].c;
            const int mi = nativeModelFor(c.type);
            if (!models_[mi].ok) continue;
            const double s = pl.dist + t.relZ(c.z - pl.position);
            TrackPoint p = t.point(s, c.offset * RW);
            const double yaw = c.laneYaw + (c.crashed ? c.crashYaw : 0);
            const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
            const float L = (float)WEB_CARS[c.type].L, W = (float)WEB_CARS[c.type].W, k0 = L / CARS[mi].d.L;
            r.draw(shadow_, shadowMat_, m * scale({W * 1.25f, 1.f, L * 1.12f}));
            const size_t ci = (size_t)(&c - traffic_.cars.data());
            if (vis[k].dz * M > LOD_CAR_M) { dessineLoin(r, c, m, L, W); continue; }   // WFS-67 : silhouette légère au-delà de 50 m
            feux(c, ci < stops_.size() && stops_[ci].freine(), m, L, W, g.time);       // WFS-67 : clignotants, détresse, feux stop
            const vec3 paint = hexLin(TRAFFIC_PAINTS[c.color]);
            const RouesPose rp = roues::poseTrafic(c.laneYaw, c.crashed, c.z, (models_[mi].nRoues ? models_[mi].roues[0].rayon : roues::RW_DEFAUT) * k0);   // WFS-60
            r.draw(models_[mi], m * scale({k0, k0, k0}), &paint, &rp);
        }
        dessineHalos(r, g);   // WFS-67
    }

    static TrafficSystem* active_;
    Traffic traffic_;
    TrafficStats stats_;
    uint32_t seed_ = 0x5eed1u; int trafficN_ = 42; Track* track_ = nullptr;
    bool benchReq_ = false; double benchM_ = 0, benchLat_ = 0, benchKmh_ = 0; bool benchHold_ = false; int benchCar_ = -1;
    bool pelotonReq_ = false, benchDense_ = false; double pelotonM_ = 0, denseT_ = 0;

private:
    // ---------- trafic vivant (WFS-67) ----------
    // silhouette lointaine (makeCarFar / makeCarLow du jeu web) : caisse peinte + habitacle sombre, 2 appels de dessin
    // au lieu d'un par matériau du vrai modèle ; halos des feux : carrés face à la caméra, texture radiale (MAT.blinkHalo)
    void initVivant(Renderer& r) {
        std::vector<Vertex> v; std::vector<uint32_t> idx;   // cube unité : x, z ∈ [−0,5 ; 0,5], y ∈ [0 ; 1]
        const vec3 nrm[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (const vec3& f : nrm) {
            const vec3 a = std::abs(f.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, b = cross(f, a);
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i < 4; i++) {
                const float sa = (i == 1 || i == 2) ? 0.5f : -0.5f, sb = i >= 2 ? 0.5f : -0.5f;
                const vec3 q = f * 0.5f + a * sa + b * sb + vec3{0, 0.5f, 0};
                v.push_back({q.x, q.y, q.z, f.x, f.y, f.z, sa + 0.5f, sb + 0.5f});
            }
            idx.insert(idx.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        cube_ = r.makeMesh(v, idx);
        for (int i = 0; i < TRAFFIC_PAINT_COUNT; i++) {   // MAT.carFar : rugosité 0,5, métal 0,25
            Material m; const vec3 c = hexLin(TRAFFIC_PAINTS[i]); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.5f; m.metal = 0.25f;
            loinPeint_[i] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        }
        { Material m; const vec3 c = hexLin(0x15181e); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.15f; m.metal = 0.1f; loinVitre_ = r.makeMaterial(m, nullptr, nullptr, nullptr); }
        constexpr int T = 64;
        std::vector<uint8_t> px(T * T * 4);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            const float dx = (x + 0.5f) / T * 2 - 1, dy = (y + 0.5f) / T * 2 - 1, a = std::clamp(1 - std::sqrt(dx * dx + dy * dy), 0.f, 1.f);
            uint8_t* p = &px[(size_t)(y * T + x) * 4]; p[0] = p[1] = p[2] = 255; p[3] = (uint8_t)std::lround(a * a * 255);
        }
        haloTex_ = r.vk().uploadTexture(px.data(), T, T, false);
        // orange des clignotants (#ffa31a) et rouge des feux stop (haloBrake #ff2038), lumière émise vive (bien visible de jour)
        const vec3 cols[2] = {hexLin(0xffa31a) * 7.f, hexLin(0xff2038) * 7.f};
        for (int c = 0; c < 2; c++) {
            Material m; m.color = {0, 0, 0, 1}; m.rough = 1; m.emissive = cols[c];
            haloMat_[c] = r.makeMaterial(m, &haloTex_, nullptr, nullptr);
            haloMat_[c].flags |= MAT_BLEND | MAT_DOUBLE;
            for (int b = 0; b < 2; b++) { haloMesh_[b][c] = r.makeMesh(std::vector<Vertex>(64 * 4), std::vector<uint32_t>(64 * 6), true); haloMesh_[b][c].indexCount = 0; }
        }
        stops_.clear(); halos_[0].clear(); halos_[1].clear();
    }
    void shutdownVivant(Renderer& r) {
        r.destroy(cube_);
        for (auto& b : haloMesh_) for (auto& m : b) r.destroy(m);
        r.vk().destroy(haloTex_);
    }
    // silhouette : caisse (hauteur 0,55 H) et habitacle (0,82 W × 0,4 H × 0,5 L), à la couleur de la voiture
    void dessineLoin(Renderer& r, const TrafficCar& c, const mat4& m, float L, float W) {
        const float H = WEB_FEUX[c.type].H;
        r.draw(cube_, loinPeint_[c.color], m * translate({0, 0.12f, 0}) * scale({W, H * 0.5f, L}));
        r.draw(cube_, loinVitre_, m * translate({0, 0.12f + H * 0.5f, L * 0.04f}) * scale({W * 0.82f, H * 0.4f, L * 0.5f}));
    }
    // clignotants (setIndicators) à chaque coin du côté du signal ; feux de détresse si accidentée ; feux stop
    // (haloBrake) au freinage et après un accident (setLights(nuit, frein = accidentée) du jeu web)
    void feux(const TrafficCar& c, bool freine, const mat4& m, float L, float W, double t) {
        const WebCarFeux& f = WEB_FEUX[c.type];
        const bool on = clignotantAllume(t, c.rng);
        for (int side : {-1, 1}) {
            const bool allume = on && (c.crashed || c.signal == side);
            if (allume)
                for (float z : {L / 2 + 0.03f + 0.08f, -L / 2 - 0.03f - 0.08f})
                    halos_[0].push_back({m.transformPoint({side * (W / 2 - 0.05f), z > 0 ? f.tail - 0.22f : f.nose - 0.14f, z}), 1.5f});
            if (freine || c.crashed) halos_[1].push_back({m.transformPoint({side * (W / 2 - 0.28f), f.tail - 0.12f, L / 2 + 0.1f}), 1.1f});
        }
    }
    void dessineHalos(Renderer& r, const GameView& g) {
        const vec3 right{g.view.at(0, 0), g.view.at(0, 1), g.view.at(0, 2)}, up{g.view.at(1, 0), g.view.at(1, 1), g.view.at(1, 2)};
        const vec3 nrm{g.view.at(2, 0), g.view.at(2, 1), g.view.at(2, 2)};
        const int b = haloBuf_; haloBuf_ ^= 1;   // double tampon : l'image précédente peut encore lire l'autre
        for (int c = 0; c < 2; c++) {
            std::vector<Vertex> v; std::vector<uint32_t> ix;
            for (size_t i = 0; i < halos_[c].size() && i < 64; i++) {
                const vec3 p = halos_[c][i].p; const float h = halos_[c][i].taille * 0.5f;
                const uint32_t n = (uint32_t)v.size();
                const vec3 a = p - right * h - up * h, bb = p + right * h - up * h, cc = p + right * h + up * h, d = p - right * h + up * h;
                v.push_back({a.x, a.y, a.z, nrm.x, nrm.y, nrm.z, 0, 1}); v.push_back({bb.x, bb.y, bb.z, nrm.x, nrm.y, nrm.z, 1, 1});
                v.push_back({cc.x, cc.y, cc.z, nrm.x, nrm.y, nrm.z, 1, 0}); v.push_back({d.x, d.y, d.z, nrm.x, nrm.y, nrm.z, 0, 0});
                ix.insert(ix.end(), {n, n + 1, n + 2, n, n + 2, n + 3});
            }
            Mesh& mesh = haloMesh_[b][c];
            if (v.empty()) mesh.indexCount = 0;
            else { r.updateMesh(mesh, v, ix); r.draw(mesh, haloMat_[c], mat4::identity()); }
            halos_[c].clear();
        }
    }
    struct Halo { vec3 p; float taille; };
    Mesh cube_; Material loinPeint_[TRAFFIC_PAINT_COUNT], loinVitre_;
    Image haloTex_; Material haloMat_[2]; Mesh haloMesh_[2][2]; int haloBuf_ = 0;
    std::vector<Halo> halos_[2];
    std::vector<FeuxStop> stops_;
public:
    const std::vector<FeuxStop>& feuxStop() const { return stops_; }
private:
    // modèle natif affiché pour une voiture du jeu web : le même s'il existe, sinon celui de longueur la plus proche
    static int nativeModelFor(int webType) {
        const WebCarDims& w = WEB_CARS[webType];
        int best = 0; double bd = 1e9;
        for (int i = 0; i < TRAFIC_MODELES; i++) {   // WFS-13 : 4 modèles du trafic (inchangé)
            if (std::strcmp(CARS[i].id, w.id) == 0) return i;
            const double d = std::abs(CARS[i].d.L - w.L);
            if (d < bd) { bd = d; best = i; }
        }
        return best;
    }
    void applyBench(Player& pl, Track& t) {
        int best = 0, bestN = -1;
        for (size_t i = 0; i < traffic_.cars.size(); i++) {
            if (traffic_.cars[i].crashed) continue;
            int n = 0;
            for (const auto& o : traffic_.cars) { const double dz = t.relZ(o.z - traffic_.cars[i].z); if (dz > 0 && dz < 120 / M) n++; }
            if (n > bestN) { bestN = n; best = (int)i; }
        }
        const TrafficCar& c = traffic_.cars[(size_t)best];
        const double np = t.wrap(c.z - benchM_ / M);
        pl.dist += t.relZ(np - pl.position); pl.position = np;
        // décalage toujours vers le milieu de la route (jamais sur le trottoir)
        const double lat = std::abs(c.offset + benchLat_) > 0.7 ? -benchLat_ : benchLat_;
        pl.playerX = c.offset + lat; pl.heading = 0; pl.steer = 0;
        pl.speed = c.speed + benchKmh_ / 260.0 * MAX_SPEED;
        for (auto& o : traffic_.cars) o.prevDz = t.relZ(o.z - pl.position);
        holdLane_ = benchHold_; holdX_ = pl.playerX; benchCar_ = best;
    }
    // banc (WFS-33, RG-01) : peloton de 3 voitures, une par file, à pelotonM_ devant le joueur (+0 / +7 / +14 m),
    // un peu plus lentes que lui ; seul le placement change, pas les règles du trafic
    void applyPeloton(Player& pl, Track& t) {
        std::vector<int> idx;
        for (size_t i = 0; i < traffic_.cars.size(); i++) if (!traffic_.cars[i].crashed) idx.push_back((int)i);
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {   // les plus proches devant le joueur
            const double da = t.relZ(traffic_.cars[(size_t)a].z - pl.position), db = t.relZ(traffic_.cars[(size_t)b].z - pl.position);
            return (da < 0 ? da + 1e9 : da) < (db < 0 ? db + 1e9 : db);
        });
        const int order[LANES] = {1, 0, 2};   // file du milieu devant, puis gauche et droite légèrement décalées
        for (int k = 0; k < LANES && k < (int)idx.size(); k++) {
            TrafficCar& c = traffic_.cars[(size_t)idx[(size_t)k]];
            const int lane = order[k];
            c.z = t.wrap(pl.position + PLAYER_Z + (pelotonM_ + 7.0 * k) / M);
            c.lane = c.target = lane; c.offset = c.prevOff = LANE_X[lane]; c.laneYaw = 0;
            c.signal = 0; c.pending = false;
            c.speed = c.want = pl.speed * 0.92;
            c.prevDz = t.relZ(c.z - pl.position);
        }
        holdLane_ = true; holdX_ = LANE_X[1];   // le joueur reste derrière, dans la file du milieu
        WFS_LOG("trafic : peloton de %d voitures à %.0f m devant (une par file)", std::min(LANES, (int)idx.size()), pelotonM_);
    }
    // banc (WFS-33, RG-02) : toutes les secondes, s'il n'y a aucune voiture entre 40 et 160 m devant, la plus proche
    // derrière le joueur (hors champ) est replacée à 90 m devant, dans la file la plus éloignée du joueur
    void keepTrafficInView(double dt, const Player& pl, Track& t) {
        denseT_ -= dt;
        if (denseT_ > 0) return;
        denseT_ = 1.0;
        int behind = -1; double bestBehind = -1e18;
        for (size_t i = 0; i < traffic_.cars.size(); i++) {
            const TrafficCar& c = traffic_.cars[i];
            const double dz = t.relZ(c.z - pl.position) * M;
            if (!c.crashed && dz > 40 && dz < 160) return;
            if (!c.crashed && dz < -40 && dz > bestBehind) { bestBehind = dz; behind = (int)i; }
        }
        if (behind < 0) return;
        int lane = 0; double far = -1;
        for (int l = 0; l < LANES; l++) if (std::abs(LANE_X[l] - pl.playerX) > far) { far = std::abs(LANE_X[l] - pl.playerX); lane = l; }
        TrafficCar& c = traffic_.cars[(size_t)behind];
        c.z = t.wrap(pl.position + PLAYER_Z + 90 / M);
        c.lane = c.target = lane; c.offset = c.prevOff = LANE_X[lane]; c.laneYaw = 0; c.signal = 0; c.pending = false;
        c.prevDz = t.relZ(c.z - pl.position);
    }
    Model models_[CAR_COUNT];
    Material shadowMat_; Mesh shadow_;
    double autoTimer_ = 0, autoTarget_ = 0;
    bool holdLane_ = false; double holdX_ = 0;
};
TrafficSystem* TrafficSystem::active_ = nullptr;
WFS_SYSTEM(TrafficSystem, 50)   // avant la ville (100) : le décor peut tenir compte du trafic

const Traffic* activeTraffic() { return TrafficSystem::active_ ? &TrafficSystem::active_->traffic_ : nullptr; }
const TrafficStats* activeTrafficStats() { return TrafficSystem::active_ ? &TrafficSystem::active_->stats_ : nullptr; }
void trafficBenchPlace(double metres, double lat, double extraKmh, bool holdLane) {
    if (auto* s = TrafficSystem::active_) { s->benchReq_ = true; s->benchM_ = metres; s->benchLat_ = lat; s->benchKmh_ = extraKmh; s->benchHold_ = holdLane; }
}
Traffic* activeTrafficMut() { return TrafficSystem::active_ ? &TrafficSystem::active_->traffic_ : nullptr; }
void trafficRestart(double playerPos) {
    if (auto* s = TrafficSystem::active_; s && s->track_) { s->traffic_.reset(*s->track_, s->seed_, s->trafficN_, playerPos); s->stats_ = {}; }
}
void trafficRestartSeed(uint32_t seed, double playerPos) {
    if (auto* s = TrafficSystem::active_) { s->seed_ = seed; trafficRestart(playerPos); }
}
int trafficBenchCar() { return TrafficSystem::active_ ? TrafficSystem::active_->benchCar_ : -1; }
static void trafficBenchPeloton(double metres) { if (auto* s = TrafficSystem::active_) { s->pelotonReq_ = true; s->pelotonM_ = metres; } }
static void trafficBenchDense(bool on) { if (auto* s = TrafficSystem::active_) s->benchDense_ = on; }

// scènes du banc d'essai : trafic de près (2 caméras) et choc arrière
void benchTrafficScenes(Game& g, Renderer& r, const std::string& out) {
    const TrafficStats* st = activeTrafficStats();
    if (!st) { WFS_ERR("trafic : module inactif"); return; }
    WFS_LOG("trafic : %zu voitures, pendant la course %d quasi-contacts, %d accidents", activeTraffic()->cars.size(), st->nearMisses, st->accidents);
    g.setAutopilot(true);
    trafficBenchDense(false);   // scènes réglées à la main : pas de replacement automatique
    for (int cam = 0; cam < 2; cam++) {
        g.camMode = cam;
        // cam0 : peloton de 3 voitures, une par file (WFS-33) ; cam1 : voiture voisine de près (comme avant)
        if (cam == 0) trafficBenchPeloton(16); else trafficBenchPlace(22, 0.66, 0, false);
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); g.render(100 + cam * 10 + i / 60.0); }
        char name[160]; std::snprintf(name, sizeof name, "%s/trafic-cam%d.png", out.c_str(), cam);
        r.capturePng(name);
        WFS_LOG("capture %s : %.0f km/h", name, g.player().kmh());
        if (cam == 0) {   // voitures du peloton : distance et file
            const Traffic* tr = activeTraffic(); int n = 0;
            for (const auto& c : tr->cars) {
                const double dz = TrafficSystem::active_->track_->relZ(c.z - g.player().position) * M;
                if (dz > 0 && dz < 60) { WFS_LOG("  peloton : voiture à %.1f m, file %+.2f", dz, c.offset); n++; }
            }
            WFS_LOG("trafic-cam0 : %d voiture(s) à moins de 60 m devant", n);
        }
    }
    // choc : 80 km/h plus vite que la voiture de devant, dans sa file, sans freiner
    g.camMode = 1;
    const int before = st->accidents;
    trafficBenchPlace(14, 0, 80, true);
    int frames = 0;
    for (; frames < 150; frames++) {
        g.update(1.0 / 60); g.render(200 + frames / 60.0);
        if (st->accidents > before && frames > 0) { for (int k = 0; k < 24; k++) { g.update(1.0 / 60); g.render(203 + k / 60.0); } break; }
    }
    r.capturePng(out + "/trafic-choc.png");
    const int hit = trafficBenchCar();
    WFS_LOG("capture trafic-choc : %d accident(s) après %.2f s, voiture %d %s, joueur %.0f km/h, tête-à-queue %.2f rad",
            st->accidents - before, frames / 60.0, hit, hit >= 0 && activeTraffic()->cars[(size_t)hit].crashed ? "en glissade" : "intacte",
            g.player().kmh(), g.player().spinYaw);
    if (hit >= 0) {   // RG-03 : écart latéral borné comme crashTraffic du jeu web (offset ∈ [-1,12 ; 1,12])
        const TrafficCar& c = activeTraffic()->cars[(size_t)hit];
        WFS_LOG("trafic-choc : écart latéral de la voiture percutée %+.3f (borne ±1,12, jeu web identique), %.1f m du centre",
                c.offset, c.offset * RW);
    }
}

// ---------- WFS-67 : scènes du banc d'essai du trafic vivant ----------
void benchTraficVivant(Game& g, Renderer& r, const std::string& out) {
    effetsBanc(true);   // fumée des voitures accidentées (WFS-65) dans trafic-accident
    if (!g.init(r, "golf", "ville")) { WFS_ERR("trafic vivant : chargement impossible"); effetsBanc(false); return; }
    TrafficSystem* s = TrafficSystem::active_;
    if (!s) { g.shutdown(); effetsBanc(false); return; }
    g.setAutopilot(true); g.camMode = 1;   // caméra haute : on voit les voitures de devant et leurs feux
    double t = 800;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto devant = [&](const TrafficCar& c) { return s->track_->relZ(c.z - g.player().position) * M; };
    // une voiture entre 8 et 45 m devant qui vérifie `cond`, cherchée pendant `secondes` au plus
    auto cherche = [&](double secondes, auto cond) {
        for (int i = 0; i < (int)(secondes * 60); i++) {
            image(i % 8 == 0);
            for (size_t k = 0; k < s->traffic_.cars.size(); k++) {
                const TrafficCar& c = s->traffic_.cars[k]; const double dz = devant(c);
                if (dz > 8 && dz < 45 && std::abs(c.offset - g.player().playerX) < 0.9 && cond(c, k)) return (int)k;
            }
        }
        return -1;
    };
    for (int i = 0; i < 120; i++) image(i % 8 == 0);
    // clignotant : changement de file annoncé, au moment où le feu est allumé
    int k = cherche(40, [&](const TrafficCar& c, size_t) { return !c.crashed && c.signal != 0 && clignotantAllume(t, c.rng); });
    g.render(t); g.render(t);
    r.capturePng(out + "/trafic-clignotant.png");
    if (k >= 0) { const TrafficCar& c = s->traffic_.cars[(size_t)k];
        WFS_LOG("capture trafic-clignotant : voiture %d à %.1f m, clignotant %s, file %d → %d", k, devant(c), c.signal < 0 ? "gauche" : "droite", c.lane, c.pendLane); }
    else WFS_LOG("capture trafic-clignotant : aucune voiture en train de changer de file trouvée en 40 s");
    // freinage : feux stop allumés (décélération > 2 m/s²)
    k = cherche(40, [&](const TrafficCar& c, size_t i) { return !c.crashed && i < s->stops_.size() && s->stops_[i].freine(); });
    g.render(t); g.render(t);
    r.capturePng(out + "/trafic-freinage.png");
    if (k >= 0) WFS_LOG("capture trafic-freinage : voiture %d à %.1f m, %.0f km/h, feux stop allumés", k, devant(s->traffic_.cars[(size_t)k]),
                        s->traffic_.cars[(size_t)k].speed / MAX_SPEED * 260);
    else WFS_LOG("capture trafic-freinage : aucune voiture qui freine trouvée en 40 s");
    // variété : modèles et couleurs des voitures visibles, et combien sont en silhouette légère (au-delà de 50 m)
    {
        int vues = 0, loin = 0; std::vector<int> paires;
        for (const auto& c : s->traffic_.cars) {
            const double dz = devant(c);
            if (dz < -30 || dz > 300) continue;
            vues++; loin += dz > LOD_CAR_M || dz < -LOD_CAR_M;
            const int p = c.type * 100 + c.color;
            if (std::find(paires.begin(), paires.end(), p) == paires.end()) paires.push_back(p);
        }
        WFS_LOG("trafic vivant : %d voitures dans le champ, %zu couples modèle / couleur différents, %d en silhouette légère (> %.0f m)",
                vues, paires.size(), loin, LOD_CAR_M);
    }
    // accident : voiture percutée arrêtée, feux de détresse et fumée ; les autres la contournent
    const int avant = s->stats_.accidents;
    trafficBenchPlace(14, 0, 80, true);
    int n = 0;
    for (; n < 180 && s->stats_.accidents == avant; n++) image(true);
    for (int i = 0; i < 100; i++) image(i % 6 == 0);
    // image au moment où les feux de détresse sont allumés
    const int hit = trafficBenchCar();
    for (int i = 0; i < 60 && hit >= 0 && !clignotantAllume(t, s->traffic_.cars[(size_t)hit].rng); i++) image(false);
    g.render(t); g.render(t);
    r.capturePng(out + "/trafic-accident.png");
    if (hit >= 0) {
        const TrafficCar& c = s->traffic_.cars[(size_t)hit];
        int contournent = 0;
        for (const auto& o : s->traffic_.cars) if (&o != &c && !o.crashed && std::abs(s->track_->relZ(o.z - c.z)) * M < 30 && std::abs(o.offset - c.offset) > 0.4) contournent++;
        WFS_LOG("capture trafic-accident : %d accident(s) après %.2f s, voiture %d %s (%.0f km/h, %.1f s), %d voiture(s) à côté d'elle à moins de 30 m",
                s->stats_.accidents - avant, n / 60.0, hit, c.crashed ? "accidentée" : "repartie", c.speed / MAX_SPEED * 260, c.crashT, contournent);
    }
    g.shutdown();
    effetsBanc(false);
}

} // namespace wfs
