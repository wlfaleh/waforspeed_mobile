// WaForSpeed — module « trafic » (WFS-08) : voir traffic_system.h.
#include "traffic_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../core/log.h"
#include "../render/model_loader.h"
#include "../render/effets_systeme.h"
#include "../render/effets_particules.h"   // WFS-109 : effets::versHdr (halos du jeu web)
#include "../render/feux_modele.h"         // WFS-109 : transition du feu stop
#include "../render/renderer.h"
#include "course5.h"   // WFS-110
#include "course_system.h"
#include "game.h"
#include "poursuite.h"   // WFS-111
#include "../world/conducteurs.h"   // WFS-73 (accroche)
#include "deux_roues.h"   // WFS-232 : pilotes casqués des deux-roues
#include "trace_cartes.h"   // WFS-232 : tracé des captures en courbe

namespace wfs {

static vec3 hexLin(unsigned h) {
    auto lin = [](float c) { return std::pow(c, 2.2f); };
    return {lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
}

class TrafficSystem : public System {
public:
    const char* name() const override { return "trafic"; }

    void init(Renderer& r, const GameView& g) override {
        { const char* mo = std::getenv("WFS_MOMENT"); nuit_ = mo && !std::strcmp(mo, "nuit"); }   // WFS-81 : feux stop portés par le moteur la nuit
        // modèles natifs, partagés par toutes les voitures du trafic
        for (int i = 0; i < TRAFIC_MODELES; i++) loadCarModel(r, std::string(CARS[i].file), models_[i]);   // WFS-13
        // WFS-232 : deux-roues (cartes qui en ont : Ville, Corniche, Campagne) ; un phare et un feu arrière au milieu (pas de paire)
        avecDeuxRoues_ = g.map && partDeuxRoues(g.map->id) > 0;
        for (int k = 0; k < DEUX_ROUES_COUNT && avecDeuxRoues_; k++) {
            Model& b = deuxRoues_[k];
            loadCarModel(r, std::string("cars/deux_roues/") + DEUX_ROUES[k].id + ".wfm", b);
            if (!b.ok) continue;
            b.feux.avantX = b.feux.arriereX = 0;
            if (b.feux.avant == 0) b.feux.avant = b.bmin.z + 0.12f;
            if (b.feux.avantY == 0) b.feux.avantY = deux_roues::GABARITS[k].poigneeY - 0.15f;
            if (b.feux.arriere == 0) b.feux.arriere = b.bmax.z - 0.05f;
            if (b.feux.arriereY == 0) b.feux.arriereY = deux_roues::GABARITS[k].selleY - 0.05f;
        }
        if (avecDeuxRoues_) deux_roues::preparer(r);
        Material m; m.flags = MAT_SHADOWBLOB | MAT_BLEND; m.color = {0, 0, 0, 0.75f};
        shadowMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        std::vector<Vertex> v = {{-0.5f, 0.02f, -0.5f, 0, 1, 0, 0, 0}, {0.5f, 0.02f, -0.5f, 0, 1, 0, 1, 0},
                                 {0.5f, 0.02f, 0.5f, 0, 1, 0, 1, 1}, {-0.5f, 0.02f, 0.5f, 0, 1, 0, 0, 1}};
        shadow_ = r.makeMesh(v, {0, 1, 2, 0, 2, 3});
        // trafic de la carte (resetRace du jeu web) ; « Rejouer » (WFS-12) repart de la même graine.
        // Graine tirée au hasard à chaque course : avec le menu (WFS-11).
        trafficN_ = (int)std::lround((g.map ? g.map->trafficN : 42) * config().trafic.densite); track_ = g.track;   // WFS-128 : × trafic.densite (1)
        traffic_.reset(*g.track, seed_, trafficN_, g.player ? g.player->position : 0);
        stats_ = {};
        active_ = this;
        // banc d'essai (rendu hors écran) : garder du trafic dans le champ pendant la course (WFS-33, RG-02)
        benchDense_ = r.vk().offscreen; denseT_ = 0;
        initVivant(r);   // WFS-67
    }

    void shutdown(Renderer& r) override {
        for (auto& m : models_) r.destroy(m);
        for (auto& m : deuxRoues_) if (m.ok) r.destroy(m);   // WFS-232
        deux_roues::detruire(r);
        r.destroy(shadow_); r.destroy(shadowMat_);   // WFS-187 (accroche) : jeux de descripteurs rendus
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
        if (sansTrafic(g.ecran == Ecran::Course)) { traffic_.cars.clear(); stops_.clear(); vide_ = true; return; }   // WFS-110 : course à 5 et duel
        if (vide_) { vide_ = false; traffic_.reset(*g.track, seed_, trafficN_, pl.position); }   // WFS-110 : trafic rendu après
        if (const int n = poursuiteTrafic(trafficN_, g.ecran == Ecran::Course); n > (int)traffic_.cars.size()) traffic_.addCars(n - (int)traffic_.cars.size(), pl.position);   // WFS-111 : trafic dense
        const bool playing = !course || course->playing();
        TrafficView pv;
        pv.position = pl.position; pv.playerX = pl.playerX; pv.speed = pl.speed;
        pv.myLen = webCarLength(pl.car->id, pl.car->d.L);
        for (TrafficCar& c : traffic_.cars) { c.zPrec = c.z; c.offPrec = c.offset; c.yawPrec = c.laneYaw; }   // WFS-221 (accroche)
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
            if (c.cache) continue;   // WFS-136 (accroche) : voiture qui traverse, hors de son passage
            const double dz = t.relZ(c.z - pl.position);
            if (dz < -30 / M || dz > 300 / M || n >= 64) continue;
            vis[n++] = {std::abs(dz), &c};
        }
        std::sort(vis, vis + n, [](const Vis& a, const Vis& b) { return a.dz < b.dz; });
        const bool dense = r.qualiteBasse && poursuiteTrafic(1, g.ecran == Ecran::Course) > 0;   // WFS-111 : poursuite en Basse
        n = std::min(n, dense ? 3 : r.qualiteBasse ? 6 : 14);   // WFS-134 (RG-11) : Basse : moins de voitures, jamais de cubes
        if (r.qualiteBasse) while (n > 0 && vis[n - 1].dz * M > 120) n--;   // Basse : trafic dessiné jusqu'à 120 m
        for (int k = 0; k < n; k++) {
            const TrafficCar& c = *vis[k].c;
            if (c.deuxRoues) { dessinerDeuxRoues(r, g, c); continue; }   // WFS-232
            const int mi = nativeModelFor(c.type);
            if (!models_[mi].ok) continue;
            // WFS-221 (accroche) : rendu interpolé entre les deux derniers pas, comme le joueur (sauf apparition ou saut)
            const double dzc = t.relZ(c.z - c.zPrec), al = std::abs(dzc) * M > 20 ? 1.0 : g.alpha;
            const double s = pl.dist + t.relZ(c.zPrec + dzc * al - pl.position);
            TrackPoint p = t.pointLisse(s, (c.offPrec + (c.offset - c.offPrec) * al) * RW);
            const double yaw = c.yawPrec + (c.laneYaw - c.yawPrec) * al + (c.crashed ? c.crashYaw : 0);
            const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
            const float L = (float)WEB_CARS[c.type].L, W = (float)WEB_CARS[c.type].W, k0 = L / CARS[mi].d.L;
            r.draw(shadow_, shadowMat_, m * scale({W * 1.25f, 1.f, L * 1.12f}));
            const size_t ci = (size_t)(&c - traffic_.cars.data());
            // WFS-67 : silhouette légère au-delà de 50 m ; WFS-82 : dès 25 m en qualité Basse (moins d'appels de dessin)
            // WFS-134 (RG-11) : jamais de cubes, à aucune distance : toujours le vrai modèle (version allégée au-delà de 60 m,
            // avec fondu, choisie par Renderer::draw) ; en Basse, le coût se règle par le nombre et la distance (ci-dessus)
            feux(c, ci < stops_.size() && stops_[ci].freine(), m, L, W, g.time);       // WFS-67 : clignotants, détresse, feux stop
            const vec3 paint = hexLin(TRAFFIC_PAINTS[c.color]);
            const RouesPose rp = roues::poseTrafic(c.laneYaw, c.crashed, c.z, (models_[mi].nRoues ? models_[mi].roues[0].rayon : roues::RW_DEFAUT) * k0);   // WFS-60
            r.feuxProchaineVoiture(feuxDe(ci, (ci < stops_.size() && stops_[ci].freine()) || c.crashed, g.time));   // WFS-81 ; WFS-109 : feu du modèle, jour et nuit
            r.draw(models_[mi], m * scale({k0, k0, k0}), &paint, &rp);
            conducteurs::ajouter(models_[mi], m * scale({k0, k0, k0}), CARS[mi].id, (uint32_t)ci + 2);   // WFS-73 (accroche)
        }
        deux_roues::dessinerFile(r, g.camPos);   // WFS-232 : pilotes des deux-roues les plus proches
        dessineHalos(r, g);   // WFS-67
    }

    static TrafficSystem* active_;
    Traffic traffic_;
    TrafficStats stats_;
    uint32_t seed_ = 0x5eed1u; int trafficN_ = 42; Track* track_ = nullptr;
    bool benchReq_ = false; double benchM_ = 0, benchLat_ = 0, benchKmh_ = 0; bool benchHold_ = false; int benchCar_ = -1;
    bool pelotonReq_ = false, benchDense_ = false, vide_ = false;   // vide_ : WFS-110
    double pelotonM_ = 0, denseT_ = 0;

private:
    // WFS-232 (RG-04, RG-06) : deux-roues : modèle à sa taille réelle, incliné dans les virages autour de la ligne de contact des
    // roues (rendu interpolé comme les voitures), ombre, feux du modèle (phare et feu arrière la nuit, WFS-226), pilote casqué
    Model deuxRoues_[DEUX_ROUES_COUNT];
    bool avecDeuxRoues_ = false;
    void dessinerDeuxRoues(Renderer& r, const GameView& g, const TrafficCar& c) {
        const Model& b = deuxRoues_[c.deuxRoues - 1];
        if (!b.ok) return;
        const Player& pl = *g.player; Track& t = *g.track;
        const double dzc = t.relZ(c.z - c.zPrec), al = std::abs(dzc) * M > 20 ? 1.0 : g.alpha;
        const double s = pl.dist + t.relZ(c.zPrec + dzc * al - pl.position);
        const TrackPoint p = t.pointLisse(s, (c.offPrec + (c.offset - c.offPrec) * al) * RW);
        const double yaw = c.yawPrec + (c.laneYaw - c.yawPrec) * al + (c.crashed ? c.crashYaw : 0);
        const float incl = (float)(c.inclPrec + (c.inclinaison - c.inclPrec) * al);
        const mat4 sol = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
        const mat4 m = sol * rotateZ(-incl);   // inclinaison + vers la droite (rotateZ positif penche vers la gauche)
        const DeuxRouesDims& d = DEUX_ROUES[c.deuxRoues - 1];
        r.draw(shadow_, shadowMat_, sol * scale({(float)d.W * 1.25f, 1.f, (float)d.L * 1.12f}));
        const size_t ci = (size_t)(&c - traffic_.cars.data());
        r.feuxProchaineVoiture(feuxDe(ci, (ci < stops_.size() && stops_[ci].freine()) || c.crashed, g.time));
        r.draw(b, m, nullptr, nullptr);
        deux_roues::noter(m, c.deuxRoues - 1, (uint32_t)ci + 2);   // RG-02 : jamais de deux-roues sans pilote
    }
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
        // WFS-109 (RG-10, ANO-48) : orange des clignotants (MAT.blinkHalo #ff9a10, opacité 0,95, sans sur-intensité) et rouge
        // des feux stop (haloBrake #ff2038) : éclat du jeu web converti par versHdr (plus de lumière × 7 amplifiée par le bloom)
        auto web = [](uint32_t h) { return effets::versHdr(vec3{((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}, 1.f); };
        const vec3 cols[2] = {web(0xff9a10), web(0xff2038)};
        for (int c = 0; c < 2; c++) {
            Material m; m.color = {0, 0, 0, 0.95f}; m.rough = 1; m.emissive = cols[c];
            haloMat_[c] = r.makeMaterial(m, &haloTex_, nullptr, nullptr);
            haloMat_[c].flags |= MAT_BLEND | MAT_DOUBLE;
            for (int b = 0; b < 2; b++) { haloMesh_[b][c] = r.makeMesh(std::vector<Vertex>(64 * 4), std::vector<uint32_t>(64 * 6), true); haloMesh_[b][c].indexCount = 0; }
        }
        stops_.clear(); halos_[0].clear(); halos_[1].clear(); niveaux_.clear();
    }
    void shutdownVivant(Renderer& r) {
        r.destroy(cube_);
        for (auto& b : haloMesh_) for (auto& m : b) r.destroy(m);
        r.vk().destroy(haloTex_);
        for (Material& m : loinPeint_) r.destroy(m);
        for (Material* m : {&loinVitre_, &haloMat_[0], &haloMat_[1]}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
    }
    // silhouette : caisse (hauteur 0,55 H) et habitacle (0,82 W × 0,4 H × 0,5 L), à la couleur de la voiture
    // WFS-134 (RG-11) : plus appelée (aucune voiture en cubes) ; gardée pour mémoire de l'ancienne silhouette
    [[maybe_unused]] void dessineLoin(Renderer& r, const TrafficCar& c, const mat4& m, float L, float W) {
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
        }
        (void)freine;   // WFS-109 (RG-01) : plus de carré de halo plaqué à côté du feu ; le feu du modèle s'allume (feuxDe)
    }
    // WFS-109 : feux du modèle de la voiture ci (Renderer::feuxVoiture), stop avec la transition de 80 ms (RG-02)
    FeuxVoiture feuxDe(size_t ci, bool stop, double t) {
        if (niveaux_.size() != traffic_.cars.size()) niveaux_.assign(traffic_.cars.size(), {0.f, -1.0});
        auto& [n, t0] = niveaux_[ci];
        n = feux_modele::transition(n, stop, t0 < 0 ? -1.f : (float)(t - t0)); t0 = t;
        return {nuit_, stop, false, n};
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
    std::vector<std::pair<float, double>> niveaux_;   // WFS-109 : niveau du stop de chaque voiture et heure de la dernière image
    bool nuit_ = false;   // WFS-81
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
// WFS-232 (RG-07) : deux-roues du trafic, Ville et Corniche, jour et nuit : 3 deux-roues (scooter, routière, sportive) placés
// devant la Golf arrêtée, dans les 3 files, en ligne droite (trafic-motos-<carte>-<moment>) puis dans une courbe de 18 à 60 m
// de rayon (trafic-motos-virage-<carte>-<moment>), puis de près, à 4,5 m de chaque côté du joueur et à 10 m devant lui
// (trafic-motos-pres-<carte>-<moment>) ; journal : modèle, vitesse, rayon, inclinaison et équilibre, pilotes dessinés
static void benchDeuxRoues(Game& g, Renderer& r, const std::string& out) {
    for (const char* carte : {"ville", "plage"}) {
        static Track piste; piste.build(mapById(carte), trace::versionCarte(carte));
        int64_t courbe = -1;   // début de la 1re courbe serrée (rayon de 18 à 60 m tenu sur 20 m au moins) : inclinaison bien visible
        for (int64_t a = 100; a + 20 < (int64_t)piste.segmentCount() && courbe < 0; a++) {
            bool ok = true;
            for (int64_t k = 0; k < (int64_t)(20 / SEG_M) && ok; k++) { const double kap = std::abs(piste.segAt(a + k).curve * CURVE_K / SEG_M); ok = kap > 1 / 60.0 && kap < 1 / 18.0; }
            if (ok) courbe = a;
        }
        for (const char* moment : {"jour", "nuit"}) for (int vue = 0; vue < 3; vue++) {   // 0 : ligne droite, 1 : courbe, 2 : de près
            if (vue == 1 && courbe < 0) continue;
            setenv("WFS_MOMENT", moment, 1);
            setenv("WFS_DEPART", std::to_string(vue != 1 ? 0 : std::max<int64_t>(0, courbe - (int64_t)(6 / SEG_M))).c_str(), 1);
            if (!g.init(r, "golf", carte)) { WFS_ERR("deux-roues : chargement %s impossible", carte); continue; }
            TrafficSystem* s = TrafficSystem::active_;
            if (!s) { g.shutdown(); continue; }
            g.setAutopilot(false); g.camMode = 1;
            double t = 900;
            for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(t += 0.5); }
            const Player& pl = g.player();
            // une voiture de chaque modèle devient (ou reste) un deux-roues, posée devant le joueur, une par file
            std::vector<size_t> pris;
            for (int k = 0; k < DEUX_ROUES_COUNT; k++) {
                size_t best = 0; double loin = -1;
                for (size_t i = 0; i < s->traffic_.cars.size(); i++) {
                    const TrafficCar& c = s->traffic_.cars[i];
                    if (c.cache || c.crashed || std::find(pris.begin(), pris.end(), i) != pris.end()) continue;
                    const double d = std::abs(s->track_->relZ(c.z - pl.position)) + (c.deuxRoues == k + 1 ? 1e7 : 0);
                    if (d > loin) { loin = d; best = i; }
                }
                pris.push_back(best);
                TrafficCar& c = s->traffic_.cars[best];
                const int lane = vue == 2 ? (k == 0 ? 2 : k == 1 ? 0 : 1) : (k == 0 ? 2 : k == 1 ? 1 : 0);   // de près : la file du joueur (1) loin
                c.deuxRoues = k + 1; c.len = DEUX_ROUES[k].L / M; c.w = DEUX_ROUES[k].W / (2 * RW);
                c.z = c.zPrec = s->track_->wrap(pl.position + (vue == 0 ? 7.0 + 6.0 * k : vue == 1 ? 9.0 + 5.0 * k : k < 2 ? 4.5 : 10.0) / M);
                c.lane = c.target = lane; c.offset = c.prevOff = (LANE_X[lane] + DROITE_VOIE) * s->traffic_.facteurVoie(c.z);
                c.offPrec = c.offset; c.laneYaw = c.yawPrec = 0; c.pending = false; c.think = 1e9; c.signal = 0;
                // en courbe : vitesse d'une inclinaison de 20° au rayon local (v² = g·R·tan 20°), de 20 à 50 km/h
                const double kap = s->track_->findSegment(c.z).curve * CURVE_K / SEG_M;
                const double v = vue != 1 || !kap ? 30 / 3.6 : std::clamp(std::sqrt(9.81 / std::abs(kap) * std::tan(20 * 3.14159265 / 180)), 20 / 3.6, 50 / 3.6);
                c.speed = c.want = v / M;
                c.inclinaison = c.inclPrec = inclinaisonEquilibre(v, kap);
                c.prevDz = s->track_->relZ(c.z - pl.position);
            }
            g.update(1.0 / 60); g.render(t += 1.0 / 60); g.render(t += 1.0 / 60);
            const std::string nom = std::string("trafic-motos-") + (vue == 1 ? "virage-" : vue == 2 ? "pres-" : "") + carte + "-" + moment;
            r.capturePng(out + "/" + nom + ".png");
            for (size_t i : pris) {
                const TrafficCar& c = s->traffic_.cars[i];
                const double kap = s->track_->findSegment(c.z).curve * CURVE_K / SEG_M;
                WFS_LOG("capture %s : %s à %.1f m devant, %.0f km/h, rayon %s, inclinaison %.1f° (équilibre %.1f°)", nom.c_str(), DEUX_ROUES[c.deuxRoues - 1].id,
                        s->track_->relZ(c.z - pl.position) * M, c.speed * M * 3.6, kap ? (std::to_string((int)std::lround(1 / std::abs(kap))) + " m").c_str() : "ligne droite",
                        c.inclinaison * 180 / 3.14159265, inclinaisonEquilibre(c.speed * M, kap) * 180 / 3.14159265);
            }
            WFS_LOG("capture %s : %d pilote(s) casqué(s) dessiné(s), %u appels de dessin", nom.c_str(), deux_roues::dessinesDerniereImage(), r.appelsDessin());
            g.shutdown();
        }
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

void benchTraficVivant(Game& g, Renderer& r, const std::string& out) {
    effetsBanc(true);   // fumée des voitures accidentées (WFS-65) dans trafic-accident
    if (!g.init(r, "golf", "ville")) { WFS_ERR("trafic vivant : chargement impossible"); effetsBanc(false); return; }
    TrafficSystem* s = TrafficSystem::active_;
    if (!s) { g.shutdown(); effetsBanc(false); return; }
    g.setAutopilot(true); g.camMode = 1;   // caméra haute : on voit les voitures de devant et leurs feux
    double t = 800;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto devant = [&](const TrafficCar& c) { return s->track_->relZ(c.z - g.player().position) * M; };
    // WFS-101 (RG-03, ANO-26) : la voiture qui clignote puis freine est placée à 15 m devant la Golf, dans la file voisine
    // (8 à 25 m exigés : feux nettement visibles), à sa vitesse ; les autres voitures de cette file proches sont
    // avancées à 75 m et plus (aucune ne la gêne). Seul le placement change : clignotant et feux stop restent ceux du trafic.
    auto placeVoisine = [&]() {
        const Player& pl = g.player();
        int fj = 0;
        for (int l = 1; l < LANES; l++) if (std::abs(LANE_X[l] - pl.playerX) < std::abs(LANE_X[fj] - pl.playerX)) fj = l;
        const int fv = fj == 1 ? 2 : 1;
        int k = -1; double meilleur = -1e18;   // la plus proche derrière le joueur, hors champ (sinon la plus lointaine devant)
        for (size_t i = 0; i < s->traffic_.cars.size(); i++) {
            const TrafficCar& c = s->traffic_.cars[i];
            if (c.crashed) continue;
            const double dz = devant(c), note = dz < -20 ? 1e9 + dz : dz;
            if (note > meilleur) { meilleur = note; k = (int)i; }
        }
        if (k < 0) return -1;
        int j = 0;
        for (size_t i = 0; i < s->traffic_.cars.size(); i++) {
            TrafficCar& o = s->traffic_.cars[i];
            const double dz = devant(o);
            if ((int)i == k || o.crashed || dz < -8 || dz > 60) continue;
            if (o.lane != fv && o.target != fv && std::abs(o.offset - LANE_X[fv]) > 0.45) continue;
            o.z = s->track_->wrap(pl.position + PLAYER_Z + (75.0 + 12.0 * j++) / M);
            o.prevDz = s->track_->relZ(o.z - pl.position);
        }
        TrafficCar& c = s->traffic_.cars[(size_t)k];
        c.z = s->track_->wrap(pl.position + PLAYER_Z + 15.0 / M);
        c.lane = c.target = fv; c.offset = c.prevOff = LANE_X[fv]; c.laneYaw = 0;
        c.speed = c.want = pl.speed; c.signal = 0; c.pending = false; c.think = 6;
        c.prevDz = s->track_->relZ(c.z - pl.position);
        return k;
    };
    for (int i = 0; i < 120; i++) image(i % 8 == 0);
    // clignotant : changement de file annoncé vers la file de la Golf, capturé au moment où le feu est allumé
    int k = placeVoisine();
    if (k >= 0) {
        TrafficCar& c = s->traffic_.cars[(size_t)k];
        int fj = 0;
        for (int l = 1; l < LANES; l++) if (std::abs(LANE_X[l] - g.player().playerX) < std::abs(LANE_X[fj] - g.player().playerX)) fj = l;
        c.pending = true; c.pendLane = fj; c.pendT = 1.2; c.signal = fj < c.lane ? -1 : 1;
        for (int i = 0; i < 30 && !clignotantAllume(t, c.rng); i++) image(i % 8 == 0);   // allumé 0,38 s sur 0,72 s
    }
    g.render(t); g.render(t);
    r.capturePng(out + "/trafic-clignotant.png");
    if (k >= 0) { TrafficCar& c = s->traffic_.cars[(size_t)k];
        WFS_LOG("capture trafic-clignotant : voiture %d à %.1f m, clignotant %s, file %d → %d", k, devant(c), c.signal < 0 ? "gauche" : "droite", c.lane, c.pendLane);
        c.pending = false; c.signal = 0; c.think = 6; }   // changement de file annulé : pas d'accident avant les scènes suivantes
    else WFS_LOG("capture trafic-clignotant : aucune voiture à placer");
    // freinage : même placement, puis la voiture vise 55 % de sa vitesse (feux stop : décélération > 2 m/s²)
    for (int i = 0; i < 30; i++) image(i % 8 == 0);
    k = placeVoisine();
    for (int i = 0; i < 6; i++) image(false);   // feux stop au repos (vitesse stable) avant le freinage
    bool freine = false;
    if (k >= 0) {
        TrafficCar& c = s->traffic_.cars[(size_t)k];
        c.want = c.speed * 0.55;
        for (int i = 0; i < 30 && !(freine = (size_t)k < s->feuxStop().size() && s->feuxStop()[(size_t)k].freine()); i++) image(false);
        for (int i = 0; i < 5; i++) image(false);   // ≈ 0,08 s de freinage : feux stop bien allumés, distance presque inchangée
    }
    g.render(t); g.render(t);
    r.capturePng(out + "/trafic-freinage.png");
    if (k >= 0) { TrafficCar& c = s->traffic_.cars[(size_t)k];
        WFS_LOG("capture trafic-freinage : voiture %d à %.1f m, %.0f km/h, feux stop %s", k, devant(c), c.speed / MAX_SPEED * 260,
                (size_t)k < s->feuxStop().size() && s->feuxStop()[(size_t)k].freine() ? "allumés" : (freine ? "éteints à la capture" : "jamais allumés"));
        c.want = g.player().speed; }
    else WFS_LOG("capture trafic-freinage : aucune voiture à placer");
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
    for (int i = 0; i < 30; i++) image(i % 6 == 0);   // 0,5 s : la voiture percutée est encore dans le champ
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
    benchDeuxRoues(g, r, out);   // WFS-232 : captures trafic-motos-*
}

} // namespace wfs
