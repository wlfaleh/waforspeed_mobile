// Vérifie que le trafic natif est identique à celui du jeu web (mode exact), puis que le mode parallèle
// (threads de jobs()) reste cohérent : même résultat à chaque exécution, aucune voiture qui en chevauche une autre.
// Référence : tests/trafic_web_ref.txt, produite par `node tools/qa/trafic_web.cjs <jeu web>/waforspeed`.
// usage : test_traffic tests/trafic_web_ref.txt
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../src/core/jobs.h"
#include "../src/game/traffic.h"
using namespace wfs;

struct Line { std::string map; int tag, i, type, color, lane, target; double z, off, speed; };

static std::vector<Line> run(const char* mapId, uint32_t seed, bool parallel, std::vector<Traffic>* keep = nullptr) {
    static Track tracks[2]; static int used = 0;
    Track& t = tracks[used++ % 2]; t.build(mapById(mapId));
    Traffic tr; tr.parallel = parallel;
    TrafficView pv; pv.position = 0; pv.playerX = 0; pv.speed = MAX_SPEED * 0.4; pv.myLen = webCarLength("golf", carById("golf").d.L);
    tr.reset(t, seed, mapById(mapId).trafficN, pv.position);
    std::vector<Line> out;
    auto snap = [&](int tag) {
        for (size_t i = 0; i < tr.cars.size(); i += 5) {
            const auto& c = tr.cars[i];
            out.push_back({mapId, tag, (int)i, c.type, c.color, c.lane, c.target, c.z, c.offset, c.speed});
        }
    };
    snap(0);
    for (int s = 1; s <= 1200; s++) {
        tr.step(T_STEP, pv); pv.position = t.wrap(pv.position + pv.speed * T_STEP);
        if (s % 600 == 0) snap(s);
        // aucune voiture ne doit en chevaucher une autre dans la même file
        for (size_t a = 0; a < tr.cars.size(); a++) for (size_t b = a + 1; b < tr.cars.size(); b++) {
            const auto &A = tr.cars[a], &B = tr.cars[b];
            if (std::abs(A.offset - B.offset) > 0.2) continue;
            const double gap = std::abs(t.relZ(A.z - B.z)) - (A.len + B.len) / 2;
            if (gap < -1.0 / M) { std::printf("ÉCHEC chevauchement %s pas %d : voitures %zu et %zu (%.2f m)\n", mapId, s, a, b, gap * M); std::exit(3); }
        }
    }
    if (keep) keep->push_back(tr);
    return out;
}

int main(int argc, char** argv) {
    jobs().start();
    std::ifstream f(argc > 1 ? argv[1] : "tests/trafic_web_ref.txt");
    if (!f) { std::printf("ÉCHEC : référence introuvable\n"); return 2; }
    std::vector<Line> ref; std::string s;
    while (std::getline(f, s)) { std::istringstream is(s); Line l; if (is >> l.map >> l.tag >> l.i >> l.type >> l.color >> l.lane >> l.target >> l.z >> l.off >> l.speed) ref.push_back(l); }

    // 1. mode exact : identique au jeu web
    std::vector<Line> got;
    for (auto [m, sd] : {std::pair{"ville", 12345u}, std::pair{"circuit", 777u}}) { auto v = run(m, sd, false); got.insert(got.end(), v.begin(), v.end()); }
    int bad = 0;
    if (got.size() != ref.size()) { std::printf("ÉCHEC : %zu lignes au lieu de %zu\n", got.size(), ref.size()); return 1; }
    for (size_t k = 0; k < ref.size(); k++) {
        const Line &a = got[k], &b = ref[k];
        const bool ok = a.map == b.map && a.tag == b.tag && a.i == b.i && a.type == b.type && a.color == b.color && a.lane == b.lane &&
                        a.target == b.target && std::abs(a.z - b.z) < 0.05 && std::abs(a.off - b.off) < 1e-5 && std::abs(a.speed - b.speed) < 0.01;
        if (!ok && bad++ < 8)
            std::printf("écart %s pas %d voiture %d : natif type %d coul %d file %d->%d z %.3f x %.6f v %.3f | web type %d coul %d file %d->%d z %.3f x %.6f v %.3f\n",
                        b.map.c_str(), b.tag, b.i, a.type, a.color, a.lane, a.target, a.z, a.off, a.speed, b.type, b.color, b.lane, b.target, b.z, b.off, b.speed);
    }
    std::printf("mode exact : %zu voitures comparées au jeu web, %d écart(s)\n", ref.size(), bad);

    // 2. mode parallèle : déterministe (2 exécutions identiques), sans chevauchement (vérifié dans run)
    auto p1 = run("ville", 12345u, true), p2 = run("ville", 12345u, true);
    int diff = 0;
    for (size_t k = 0; k < p1.size(); k++) if (p1[k].z != p2[k].z || p1[k].off != p2[k].off || p1[k].speed != p2[k].speed) diff++;
    std::printf("mode parallèle (%d threads) : %s\n", jobs().workerCount() + 1, diff ? "NON déterministe" : "déterministe, aucun chevauchement");

    // 3. choc : le joueur fonce dans une voiture arrêtée devant lui
    {
        Track t; t.build(mapById("ville"));
        Traffic tr; tr.reset(t, 1, 0, 0);
        TrafficCar c; c.type = 0; c.len = WEB_CARS[0].L / M; c.w = WEB_CARS[0].W / (2 * RW); c.lane = c.target = 1;
        c.z = 6 / M; c.speed = 0; c.want = 0; c.prevDz = c.z; tr.cars.push_back(c);
        Player pl; pl.reset(carById("golf")); pl.speed = MAX_SPEED * 0.6;
        HitEvents total;
        for (int i = 0; i < 120; i++) {
            DriveInput in; pl.update(1.0 / 120, in, t, true);
            HitEvents e = tr.collide(pl, 1.0 / 120); total.accidents += e.accidents; total.bumps += e.bumps;
        }
        const bool ok = total.accidents == 1 && tr.cars[0].crashed && pl.kmh() < 100;   // gros choc : forte perte de vitesse
        std::printf("choc arrière à 156 km/h : %d accident(s), voiture percutée %s, joueur à %.0f km/h → %s\n",
                    total.accidents, tr.cars[0].crashed ? "en glissade" : "intacte", pl.kmh(), ok ? "OK" : "ÉCHEC");
        if (!ok) bad++;
    }
    jobs().stop();
    if (bad || diff) { std::printf("ÉCHEC\n"); return 1; }
    std::printf("OK\n");
    return 0;
}
