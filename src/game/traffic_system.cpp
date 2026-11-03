// WaForSpeed — module « trafic » (WFS-08) : voir traffic_system.h.
#include "traffic_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../core/log.h"
#include "../render/model_loader.h"
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
        for (int i = 0; i < CAR_COUNT; i++) loadCarModel(r, std::string(CARS[i].file), models_[i]);
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
    }

    void shutdown(Renderer& r) override {
        for (auto& m : models_) r.destroy(m);
        r.destroy(shadow_);
        if (active_ == this) active_ = nullptr;
    }

    // après chaque pas de conduite : le trafic avance (pas de 1/60 s), puis chocs et quasi-contacts
    void collide(double dt, Player& pl, const GameView& g) override {
        if (benchReq_) { applyBench(pl, *g.track); benchReq_ = false; }
        // règles de course (WFS-12) : voiture immobile pendant le compte à rebours, chocs seulement en course
        Course* course = activeCourseMut();
        if (course) course->hold(pl);
        const bool playing = !course || course->playing();
        TrafficView pv;
        pv.position = pl.position; pv.playerX = pl.playerX; pv.speed = pl.speed;
        pv.myLen = webCarLength(pl.car->id, pl.car->d.L);
        traffic_.advance(dt, pv);
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
            const vec3 paint = hexLin(TRAFFIC_PAINTS[c.color]);
            r.draw(models_[mi], m * scale({k0, k0, k0}), &paint);
        }
    }

    static TrafficSystem* active_;
    Traffic traffic_;
    TrafficStats stats_;
    uint32_t seed_ = 0x5eed1u; int trafficN_ = 42; Track* track_ = nullptr;
    bool benchReq_ = false; double benchM_ = 0, benchLat_ = 0, benchKmh_ = 0; bool benchHold_ = false; int benchCar_ = -1;

private:
    // modèle natif affiché pour une voiture du jeu web : le même s'il existe, sinon celui de longueur la plus proche
    static int nativeModelFor(int webType) {
        const WebCarDims& w = WEB_CARS[webType];
        int best = 0; double bd = 1e9;
        for (int i = 0; i < CAR_COUNT; i++) {
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
int trafficBenchCar() { return TrafficSystem::active_ ? TrafficSystem::active_->benchCar_ : -1; }

// scènes du banc d'essai : trafic de près (2 caméras) et choc arrière
void benchTrafficScenes(Game& g, Renderer& r, const std::string& out) {
    const TrafficStats* st = activeTrafficStats();
    if (!st) { WFS_ERR("trafic : module inactif"); return; }
    WFS_LOG("trafic : %zu voitures, pendant la course %d quasi-contacts, %d accidents", activeTraffic()->cars.size(), st->nearMisses, st->accidents);
    g.setAutopilot(true);
    for (int cam = 0; cam < 2; cam++) {
        g.camMode = cam;
        trafficBenchPlace(22, 0.66, 0, false);
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); g.render(100 + cam * 10 + i / 60.0); }
        char name[160]; std::snprintf(name, sizeof name, "%s/trafic-cam%d.png", out.c_str(), cam);
        r.capturePng(name);
        WFS_LOG("capture %s : %.0f km/h", name, g.player().kmh());
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
}

} // namespace wfs
