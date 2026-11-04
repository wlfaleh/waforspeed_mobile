// WaForSpeed — règles de la course solo (WFS-12) : voir course.h.
#include "course.h"
#include <cstdio>

namespace wfs {

// resetRace('play') du jeu web (partie « règles ») puis startRace() : state = 'count'
void Course::reset() {
    state = RaceState::Count; clock = -3;
    timeLeft = 30; nextCp = CHECK_DIST; level = 0; bonus = 0; raceTime = 0;
    combo = 0; comboTimer = 0; nearCount = 0; maxKmh = 0; overTimer = 0; accidents = 0;
    bumps = 0; scrapes = 0; lastImpact = 0;   // WFS-28
    lastCount = -1; goShown = 0; timeScale = 1; slowmoT = 0;
    duelFinish = 0;   // WFS-21 : course solo par défaut
}

// quasi-contact (boucle des voitures de update()) : combo +1 si le précédent date de moins de 3 s
void Course::nearMiss() {
    combo = comboTimer > 0 ? combo + 1 : 1; comboTimer = 3;
    bonus += 250LL * combo; nearCount++;
}

// bump(impact) : petit choc, le combo est perdu au-delà de 0,08
void Course::bump(double impact) {
    if (impact > 0.08) { combo = 0; comboTimer = 0; }
    bumps++; lastImpact = impact;   // WFS-28 : bruitage du petit choc (volume 0,4 + impact)
}

// playerAccident(impact, side) : partie « règles » (le tête-à-queue est fait par Player::accident)
void Course::accident() {
    timeScale = 0.35; slowmoT = 0.5;
    combo = 0; comboTimer = 0;
    accidents++;
}

// crash() : collision avec un décor (lampadaire, panneau)
void Course::crash(Player& pl, Track& t) {
    const double back = 300; const int side = pl.playerX > 0 ? -1 : 1;
    pl.speed = MAX_SPEED / 8;
    pl.position = t.wrap(pl.position - back); pl.dist = std::max(0.0, pl.dist - back);
    pl.accident(0.45, side);
    accident();
}

void Course::applyHits(const HitEvents& e) {
    scrapes += e.scrapes;   // WFS-28 : frottements latéraux (au plus un toutes les 0,18 s, Traffic)
    for (int i = 0; i < e.count; i++) {
        const HitEvent& h = e.seq[i];
        if (h.kind == 'n') nearMiss();
        else if (h.kind == 'b') bump(h.impact);
        else if (h.kind == 'a') accident();
    }
}

void Course::hold(Player& pl) const {
    if (state != RaceState::Count) return;
    pl.speed = 0; pl.nitroOn = false; pl.nitro = 0.4;
}

// fin de update() du jeu web
void Course::step(double dt, const Player& pl, Traffic* traffic, double playerPos) {
    const bool playing = state == RaceState::Play;   // état en début d'image, comme `playing` du jeu web
    clock += dt;                                       // trafficClock += dt
    if (comboTimer > 0) { comboTimer -= dt; if (comboTimer <= 0) combo = 0; }
    if (goShown > 0) goShown -= dt;
    if (state == RaceState::Count) {
        const int n = clock < 0 ? (int)std::ceil(-clock) : 0;
        if (n != lastCount) lastCount = n;
        if (clock >= 0) { state = RaceState::Play; goShown = 0.7; }
    }
    if (playing && duelFinish > 0) {   // WFS-21 : raceTime = max(0, trafficClock) ; dist ≥ FINISH → finishDuel()
        raceTime = std::max(0.0, clock);
        maxKmh = std::max(maxKmh, kmh(pl.speed));
        if (pl.dist >= duelFinish) state = RaceState::Over;
    } else if (playing) {
        raceTime += dt;
        maxKmh = std::max(maxKmh, kmh(pl.speed));
        timeLeft -= dt;
        if (pl.dist >= nextCp) {
            const double add = std::max(5.0, 11 - level * 0.4);
            timeLeft += add; level++; nextCp += CHECK_DIST;
            if (traffic && traffic->cars.size() < 110) traffic->addCars(6, playerPos);
        }
        if (timeLeft <= 0) { timeLeft = 0; state = RaceState::Over; }   // endRace()
    } else if (state == RaceState::Over) overTimer += dt;
}

// boucle de main.js : le ralenti dure 0,5 s puis le temps revient doucement à la normale
double Course::scaleDt(double raw) {
    if (slowmoT > 0) slowmoT -= raw; else timeScale += (1 - timeScale) * std::min(1.0, raw * 4);
    return raw * timeScale;
}

std::string Course::countText() const {
    if (state == RaceState::Count) return lastCount > 0 ? std::to_string(lastCount) : std::string();
    if (state == RaceState::Play && goShown > 0) return "GO";
    return std::string();
}

// même scénario et même format que tests/course_web.cjs (l'ordre des étapes suit update() du jeu web :
// trafic, conduite, chocs, règles)
std::vector<std::string> simulateCourse(const char* mapId, const char* carId, uint32_t seed) {
    std::vector<std::string> out;
    const MapDef& map = mapById(mapId);
    static Track t; t.build(map);
    Player pl; pl.reset(carById(carId));
    Traffic tr; tr.parallel = false; tr.reset(t, seed, map.trafficN, 0);
    Course c; c.reset();
    const double dt = 1.0 / 120;
    const double myLen = webCarLength(pl.car->id, pl.car->d.L);
    int lastLevel = 0, accidents = 0, n = 0;
    std::string counts;
    char buf[256];
    while (c.state != RaceState::Over && n < 120 * 400) {
        DriveInput in;
        in.gas = true; in.steer = (pl.playerX < -0.08 ? 1.f : 0.f) - (pl.playerX > 0.08 ? 1.f : 0.f); in.nitro = pl.nitro > 0.5;
        const double lockBefore = pl.controlLock;
        TrafficView pv; pv.position = pl.position; pv.playerX = pl.playerX; pv.speed = pl.speed; pv.myLen = myLen;
        tr.advance(dt, pv);
        const bool playing = c.playing();
        pl.update(dt, in, t, playing);
        c.hold(pl);
        if (playing) c.applyHits(tr.collide(pl, dt)); else tr.watch(pl);
        const int before = c.lastCount;
        c.step(dt, pl, &tr, pl.position);
        if (c.lastCount != before) { if (!counts.empty()) counts += ','; counts += c.lastCount ? std::to_string(c.lastCount) : "GO"; }
        n++;
        if (pl.controlLock > lockBefore + 0.3) accidents++;
        if (c.level != lastLevel) {
            lastLevel = c.level;
            std::snprintf(buf, sizeof buf, "cp %s %d %d %.4f %lld %d", mapId, c.level, n, c.timeLeft, c.score(pl), c.nearCount);
            out.push_back(buf);
        }
    }
    out.push_back(std::string("decompte ") + mapId + " " + counts);
    std::snprintf(buf, sizeof buf, "fin %s %d %d %lld %lld %d %d %lld %d", mapId, n, c.level, c.score(pl), c.bonus, c.nearCount, c.maxKmh,
                  (long long)std::floor(pl.dist), accidents);
    out.push_back(buf);
    return out;
}

} // namespace wfs
