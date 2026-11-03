// Règles de course (WFS-12) : la course automatique native doit donner exactement les mêmes checkpoints, temps,
// score et quasi-contacts que le jeu web (référence : tests/course_web_ref.txt, produite par
// `node tests/course_web.cjs <jeu web>/waforspeed`), puis vérifications unitaires des règles de gestion.
// usage : test_course tests/course_web_ref.txt
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "../src/game/course.h"
using namespace wfs;

static int bad = 0;
static void check(bool ok, const char* what) { std::printf("%s : %s\n", what, ok ? "OK" : "ÉCHEC"); if (!ok) bad++; }

int main(int argc, char** argv) {
    // 1. course automatique : identique au jeu web, ligne par ligne (tolérance nulle)
    std::ifstream f(argc > 1 ? argv[1] : "tests/course_web_ref.txt");
    if (!f) { std::printf("ÉCHEC : référence introuvable\n"); return 2; }
    std::vector<std::string> ref, got; std::string s;
    while (std::getline(f, s)) if (!s.empty()) ref.push_back(s);
    for (auto [m, c, sd] : {std::tuple{"ville", "golf", 12345u}, std::tuple{"circuit", "m4", 777u}}) {
        auto v = simulateCourse(m, c, sd); got.insert(got.end(), v.begin(), v.end());
    }
    int diff = 0;
    for (size_t k = 0; k < std::max(ref.size(), got.size()); k++) {
        const std::string a = k < got.size() ? got[k] : "(rien)", b = k < ref.size() ? ref[k] : "(rien)";
        std::printf("  natif %-48s | web %s%s\n", a.c_str(), b.c_str(), a == b ? "" : "   ← ÉCART");
        if (a != b) diff++;
    }
    std::printf("course automatique : %zu lignes comparées au jeu web, %d écart(s)\n", ref.size(), diff);
    if (diff) bad++;

    // 2. RG-01 départ, RG-02 checkpoints
    Course c; c.reset();
    check(c.timeLeft == 30 && c.state == RaceState::Count && c.clock == -3, "RG-01 départ : 30 s, compte à rebours de 3 s");
    check(Course::checkpointBonus(1) == 11.0, "RG-02 checkpoint 1 : +11 s");
    check(std::abs(Course::checkpointBonus(10) - 7.4) < 1e-9 && Course::checkpointBonus(11) == 7.0,
          "RG-02 checkpoint 10 : +7,4 s (11 − 9×0,4), checkpoint 11 : +7 s");
    check(Course::checkpointBonus(30) == 5.0, "RG-02 plancher de 5 s");
    {   // checkpoint réel : la voiture passe 600 m
        Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf")); Course k; k.reset();
        k.state = RaceState::Play; k.clock = 0;
        pl.dist = CHECK_DIST; k.step(0.01, pl, nullptr, 0);
        check(k.level == 1 && std::abs(k.timeLeft - (30 - 0.01 + 11)) < 1e-9 && k.nextCp == 2 * CHECK_DIST, "RG-02 passage du checkpoint 1 : +11 s, level 1");
    }
    // compte à rebours : 3, 2, 1 puis GO, vitesse nulle
    {
        Player pl; pl.reset(carById("golf")); Course k; k.reset(); std::string seen, last;
        for (int i = 0; i < 400; i++) {
            pl.speed = 5000; k.hold(pl);
            if (k.state == RaceState::Count && pl.speed != 0) { seen = "vitesse non nulle"; break; }
            k.step(1.0 / 120, pl, nullptr, 0);
            const std::string t = k.countText();
            if (!t.empty() && t != last) { seen += (seen.empty() ? "" : ","); seen += t; }
            last = t;
        }
        std::printf("  décompte : %s\n", seen.c_str());
        check(seen == "3,2,1,GO" && k.state == RaceState::Play, "RG-01 décompte 3-2-1-GO, voiture immobile");
    }
    // RG-05 quasi-contacts : combo
    {
        Course k; k.reset();
        k.nearMiss(); const long long b1 = k.bonus; k.nearMiss(); const long long b2 = k.bonus; k.nearMiss();
        check(b1 == 250 && b2 - b1 == 500 && k.bonus - b2 == 750 && k.combo == 3 && k.nearCount == 3,
              "RG-05 quasi-contact ×3 : 250, 500 puis 750 points (1 500 au total)");
        Player pl; pl.reset(carById("golf"));
        k.state = RaceState::Play;
        for (int i = 0; i < 361; i++) k.step(1.0 / 120, pl, nullptr, 0);
        k.nearMiss();
        check(k.combo == 1, "RG-05 combo remis à 1 après 3 s sans quasi-contact");
        k.nearMiss(); k.bump(0.05); const int c1 = k.combo; k.bump(0.09);
        check(c1 == 2 && k.combo == 0, "RG-06 choc léger : combo gardé à 0,05, perdu à 0,09");
    }
    // RG-06 accident et décor
    {
        Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf")); Course k; k.reset();
        pl.position = pl.dist = 5000; pl.speed = MAX_SPEED; pl.playerX = 1.2;
        k.crash(pl, t);
        check(pl.dist == 4700 && pl.speed == MAX_SPEED / 8 && pl.controlLock > 0 && pl.playerSpin < 0 && k.timeScale == 0.35 && k.slowmoT == 0.5,
              "RG-06 décor : recul de 300, vitesse MAX/8, tête-à-queue, ralenti ×0,35");
        double game = 0; for (int i = 0; i < 60; i++) game += k.scaleDt(1.0 / 120);
        check(std::abs(game - 0.5 * 0.35) < 0.01, "RG-06 ralenti : 0,5 s réelles ≈ 0,175 s de jeu");
    }
    // RG-03 fin, RG-04 score et vitesse
    {
        Player pl; pl.reset(carById("golf")); Course k; k.reset(); k.state = RaceState::Play; k.timeLeft = 0.005;
        pl.dist = 12345 / 0.006; k.nextCp = 1e12; k.bonus = 750;   // pas de checkpoint pendant ce pas
        k.step(1.0 / 120, pl, nullptr, 0);
        check(k.state == RaceState::Over && k.timeLeft == 0, "RG-03 fin quand le temps est écoulé");
        check(k.score(pl) == 12345LL * 5 + 750, "RG-04 score = floor(dist×0,006)×5 + bonus");
        check(Course::kmh(MAX_SPEED) == 260 && Course::kmh(MAX_SPEED / 2) == 130, "RG-04 km/h = round(speed/MAX×260)");
    }
    // RG-07 nitro (Player) : consommation 0,26/s, seulement au-delà de 0,01
    {
        Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf"));
        DriveInput in; in.gas = true; in.nitro = true;
        for (int i = 0; i < 120; i++) pl.update(1.0 / 120, in, t, true);
        check(std::abs(pl.nitro - (0.4 - 0.26)) < 1e-4, "RG-07 nitro : 0,26 consommés en 1 s");
        pl.nitro = 0.005; pl.update(1.0 / 120, in, t, true);
        check(!pl.nitroOn, "RG-07 nitro inactive sous 0,01");
    }
    if (bad) { std::printf("ÉCHEC (%d)\n", bad); return 1; }
    std::printf("OK\n");
    return 0;
}
