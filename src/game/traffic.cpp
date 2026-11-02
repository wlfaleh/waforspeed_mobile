// WaForSpeed — trafic simulé, port de js/game/traffic.js (et des chocs de js/game/update.js).
#include "traffic.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include "../core/jobs.h"

namespace wfs {

static double clampd(double v, double a, double b) { return v < a ? a : (v > b ? b : v); }
static int signi(double v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// nextRand(car) du jeu web
static double nextRand(TrafficCar& car) {
    car.rng = (car.rng ^ (car.rng >> 15)) * 2246822507u + 0x9e3779b9u;
    return car.rng / 4294967296.0;
}

void Traffic::reset(Track& track, uint32_t s, int n, double playerPos, bool menu) {
    track_ = &track;
    cars.clear();
    seed = s ? s : 1;
    clock = sim = menu ? 0 : -3;   // compte à rebours de 3 s avant le départ (trafficClock du jeu web)
    scrapeCd_ = 0;
    addCars(n, playerPos);
}

// addCars(n) du jeu web
void Traffic::addCars(int n, double playerPos) {
    const MapDef& map = *track_->map();
    const double L = track_->length();
    for (int i = 0; i < n; i++) {
        Mulberry32 R(seed * 7919u + (uint32_t)(cars.size() + 1) * 104729u);
        TrafficCar c;
        c.type = (int)std::floor(R.next() * WEB_CAR_COUNT);
        c.len = WEB_CARS[c.type].L / M;
        c.want = MAX_SPEED * (map.trafficMin + R.next() * (map.trafficMax - map.trafficMin));
        c.lane = (int)std::floor(R.next() * LANES);
        double z = 0;
        for (int k = 0; k < 24; k++) {
            z = track_->wrap(playerPos + PLAYER_Z + 6000 + R.next() * (L - 12000));
            bool clash = false;
            for (const auto& o : cars)
                if (o.lane == c.lane && std::abs(track_->relZ(o.z - z)) < (o.len + c.len) / 2 + 30 / M) { clash = true; break; }
            if (!clash) break;
        }
        c.color = (int)std::floor(R.next() * TRAFFIC_PAINT_COUNT);
        c.w = WEB_CARS[c.type].W / (2 * RW);
        c.target = c.lane; c.offset = c.prevOff = LANE_X[c.lane];
        c.z = z; c.speed = c.want;
        c.rng = (uint32_t)(R.next() * 4294967296.0);
        c.think = 1 + R.next() * 4;
        c.prevDz = track_->relZ(z - (playerPos + PLAYER_Z));
        cars.push_back(c);
    }
}

// laneFree(car, lane) : la file est-elle libre autour de la voiture ?
bool Traffic::laneFree(const TrafficCar& car, int lane, const TrafficView& pv) const {
    const double lx = LANE_X[lane];
    for (const auto& o : cars) {
        if (&o == &car || (std::abs(o.offset - lx) > LANE_HALF && o.target != lane)) continue;
        const double dz = track_->relZ(o.z - car.z);
        if (dz >= 0 && dz < (o.len + car.len) / 2 + std::max(8 / M, (car.speed - o.speed) * 1.6)) return false;
        if (dz < 0 && -dz < (o.len + car.len) / 2 + std::max(10 / M, (o.speed - car.speed) * 2.2)) return false;
    }
    if (std::abs(pv.playerX - lx) < LANE_HALF + 0.1 && std::abs(track_->relZ(pv.position - car.z)) < 22 / M) return false;
    return true;
}

// voiture de devant (dans la même file ou en train d'y entrer, + le joueur) pour les voitures [begin, end).
// Ne fait que lire `cars` : peut tourner sur plusieurs threads.
void Traffic::findLeaders(const TrafficView& pv, int begin, int end) {
    for (int i = begin; i < end; i++) {
        const TrafficCar& car = cars[(size_t)i];
        double gap = std::numeric_limits<double>::infinity(), leadSpeed = car.want;
        const double tx = LANE_X[car.target];
        for (const auto& o : cars) {
            if (&o == &car) continue;
            if (std::abs(o.offset - car.offset) > LANE_HALF && std::abs(o.offset - tx) > LANE_HALF) continue;
            const double dz = track_->relZ(o.z - car.z);
            if (dz <= 0) continue;
            const double g = dz - (o.len + car.len) / 2;
            if (g < gap) { gap = g; leadSpeed = o.speed; }
        }
        if (!pv.garage) {
            const double dz = track_->relZ(pv.position - car.z);
            if (dz > 0 && (std::abs(pv.playerX - car.offset) < LANE_HALF || std::abs(pv.playerX - tx) < LANE_HALF)) {
                const double g = dz - (pv.myLen + car.len) / 2;
                if (g < gap) { gap = g; leadSpeed = pv.speed; }
            }
        }
        gap_[(size_t)i] = gap; lead_[(size_t)i] = leadSpeed;
    }
}

// stepTraffic(h) du jeu web.
// Mode exact (parallel = false) : chaque voiture voit les voitures déjà avancées dans ce pas, comme en JS.
// Mode parallèle : toutes les voitures de devant sont cherchées d'abord, sur tous les cœurs, à partir de l'état
// du début du pas ; le reste du pas (vitesses, changements de file) reste dans l'ordre du jeu web.
void Traffic::step(double h, const TrafficView& pv) {
    const size_t n = cars.size();
    gap_.resize(n); lead_.resize(n);
    if (parallel) jobs().parallelFor((int)n, 16, [&](int b, int e) { findLeaders(pv, b, e); });
    for (size_t i = 0; i < n; i++) {
        TrafficCar& car = cars[i];
        if (car.crashed) {
            car.crashT += h;
            car.speed = std::max(0.0, car.speed - MAX_SPEED * 0.55 * h);
            car.offset = clampd(car.offset + car.crashVx * h, -1.12, 1.12);
            car.crashVx *= std::max(0.0, 1 - 2.5 * h);
            car.crashYaw += car.crashSpin * h; car.crashSpin *= std::max(0.0, 1 - 1.8 * h);
            if (car.crashT > 7 && std::abs(track_->relZ(pv.position - car.z)) > 60 / M) {
                car.crashed = false; car.speed = 0; car.signal = 0; car.pending = false;
                int best = 0; double bd = 9;
                for (int l = 0; l < LANES; l++) { const double d = std::abs(LANE_X[l] - car.offset); if (d < bd) { bd = d; best = l; } }
                car.lane = car.target = best;
            }
            car.laneYaw = 0;
            car.z = track_->wrap(car.z + car.speed * h);
            continue;
        }
        if (!parallel) findLeaders(pv, (int)i, (int)i + 1);
        const double gap = gap_[i], leadSpeed = lead_[i];
        // vitesse visée : sa vitesse de croisière, ou celle qui garde la distance de sécurité
        const double safe = SAFE_MIN + car.speed * SAFE_TIME;
        double target = car.want;
        if (gap < safe) target = std::min(target, std::max(0.0, leadSpeed - (safe - gap) * 1.2));
        const double acc = target > car.speed ? MAX_SPEED * 0.1 : MAX_SPEED * 0.7;
        car.speed = std::max(0.0, car.speed + clampd(target - car.speed, -acc * h, acc * h));
        if (gap < 0.8 / M) car.speed = std::min(car.speed, leadSpeed * 0.95);
        // envie de changer de file (bloqué derrière plus lent, ou de temps en temps)
        car.think -= h;
        if (car.think <= 0 && car.target == car.lane && !car.pending) {
            const double r = nextRand(car);
            car.think = 1.5 + r * 4;
            const bool blocked = gap < safe * 1.6 && leadSpeed < car.want * 0.85;
            if (blocked || r < 0.12) {
                const int dir = nextRand(car) < 0.5 ? -1 : 1;
                // d'abord le clignotant, le changement de file vient ~1 s plus tard
                for (int nl : {car.lane + dir, car.lane - dir})
                    if (nl >= 0 && nl < LANES && laneFree(car, nl, pv)) {
                        car.pending = true; car.pendLane = nl; car.pendT = 0.9 + nextRand(car) * 0.5;
                        car.signal = signi(nl - car.lane); break;
                    }
            }
        }
        if (car.pending) {
            car.pendT -= h;
            if (car.pendT <= 0) {
                if (laneFree(car, car.pendLane, pv)) car.target = car.pendLane; else car.signal = 0;
                car.pending = false;
            }
        }
        // abandonne le changement si la file se remplit, sinon glisse vers elle
        if (car.target != car.lane && !laneFree(car, car.target, pv) && std::abs(car.offset - LANE_X[car.lane]) < 0.2) { car.target = car.lane; car.signal = 0; }
        const double d = LANE_X[car.target] - car.offset;
        const double move = clampd(d, -0.5 * h, 0.5 * h);
        car.offset += move;
        car.laneYaw = std::atan2(move * RW, std::max(car.speed * M * h, 1e-3));
        if (std::abs(d) < 0.01) { car.lane = car.target; if (!car.pending) car.signal = 0; }
        car.z = track_->wrap(car.z + car.speed * h);
    }
}

// advanceTraffic() : pas fixes jusqu'à l'horloge (au plus 600 pas d'un coup)
void Traffic::advance(double dt, const TrafficView& pv) {
    clock += dt;
    int steps = (int)std::floor((clock - sim) / T_STEP);
    if (steps > 600) { sim = clock - 600 * T_STEP; steps = 600; }
    for (int i = 0; i < steps; i++) { step(T_STEP, pv); sim += T_STEP; }
}

// crashTraffic(car, impact, side) : la voiture percutée part en glissade
void Traffic::crash(TrafficCar& car, double impact, int side) {
    car.crashed = true; car.crashT = 0;
    car.crashVx = side * (0.4 + impact * 1.6); car.crashYaw = 0; car.crashSpin = side * (1.5 + impact * 7);
    car.target = car.lane;
}

// chocs avec le joueur (hitCar) et quasi-contacts, comme la fin de update() du jeu web
HitEvents Traffic::collide(Player& pl, double dt) {
    HitEvents ev;
    scrapeCd_ -= dt;
    const CarDef& me = *pl.car;
    // emprise réelle de la voiture tournée de « heading » (rectangle orienté)
    const double ch = std::cos(pl.heading), sh = std::abs(std::sin(pl.heading));
    const double pz = pl.position + PLAYER_Z;
    const double myLen = (me.d.L * ch + me.d.W * sh) / M, myW = (me.d.W * ch + me.d.L * sh) / (2 * RW);
    for (auto& car : cars) {
        const double dz = track_->relZ(car.z - pz);
        const double lat = std::abs(pl.playerX - car.offset);
        const double touchLat = myW + car.w, touchZ = (myLen + car.len) / 2;
        if (lat < touchLat && std::abs(dz) < touchZ) {
            // ---------- hitCar ----------
            const int side = signi(pl.playerX - car.offset) ? signi(pl.playerX - car.offset) : 1;
            const double overlapZ = touchZ - std::abs(dz), overlapX = touchLat - lat;
            if (overlapX * RW < overlapZ * M * 0.7) {
                // contact latéral : les carrosseries frottent, on s'écarte l'un de l'autre
                pl.playerX += side * (overlapX + 0.004); car.offset -= side * overlapX * 0.35 * pl.perf.mass;
                pl.heading *= 0.5;
                pl.speed *= 0.94; car.speed *= 0.97;
                if (scrapeCd_ <= 0) { scrapeCd_ = 0.18; ev.scrapes++; ev.shake = std::max(ev.shake, 0.15f); }
            } else if (dz > 0) {
                // on percute l'arrière : la voiture est poussée, on perd notre vitesse
                const double rel = std::max(0.0, pl.speed - car.speed), impact = rel / MAX_SPEED, v0 = car.speed;
                const double back = overlapZ + 0.25 / M;
                pl.position = track_->wrap(pl.position - back); pl.dist = std::max(0.0, pl.dist - back);
                car.speed = v0 + rel * 0.55;
                pl.speed = v0 + rel * clampd(0.04 + 0.08 * pl.perf.mass, 0.06, 0.2);   // une voiture lourde garde plus de vitesse
                if (impact / pl.perf.mass > 0.28) { crash(car, impact, -side); pl.accident(impact, side); ev.accidents++; ev.shake = 0.9f; }
                else {
                    ev.bumps++; ev.shake = std::max(ev.shake, (float)(0.25 + impact * 1.5));
                    if (impact > 0.15) crash(car, impact * 0.5, -side);
                }
            } else {
                // une voiture nous rentre dedans par l'arrière (on a pilé devant elle)
                const double rel = std::max(0.0, car.speed - pl.speed), impact = rel / MAX_SPEED;
                car.z = track_->wrap(pl.position + PLAYER_Z - touchZ - 0.25 / M);
                pl.speed += rel * 0.5; car.speed = pl.speed - rel * 0.1;
                ev.bumps++; ev.shake = std::max(ev.shake, (float)(0.25 + impact * 1.5));
            }
            car.prevDz = track_->relZ(car.z - (pl.position + PLAYER_Z));
            continue;
        }
        // quasi-contact : on double une voiture en la frôlant à plus de la moitié de la vitesse max
        if (car.prevDz > 0 && dz <= 0 && dz > -2000 && lat >= touchLat && lat < touchLat + NEAR_MISS && pl.speed > MAX_SPEED * 0.5 && !car.crashed) {
            ev.nearMiss++;
            pl.nitro = std::min(1.0, pl.nitro + pl.perf.nitroGain);
        }
        car.prevDz = dz;
    }
    return ev;
}

} // namespace wfs
