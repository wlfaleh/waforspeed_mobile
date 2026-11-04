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
#include "../src/game/direction.h"   // WFS-70 : modes de direction (fonctions pures)
#include "../src/game/ressenti.h"    // WFS-70 : secousse, flash, réduire les mouvements
#include "../src/game/commandes_tactiles.h"   // WFS-98 : place des pédales et du nitro, commande sous un doigt
using namespace wfs;

static int bad = 0;
static void check(bool ok, const char* what) { std::printf("%s : %s\n", what, ok ? "OK" : "ÉCHEC"); if (!ok) bad++; }

int testCommandes();   // WFS-108

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
    // WFS-70 RG-05 : le ralenti n'allonge pas le chrono (boucle de Game::update : scaleDt puis pas fixes de 1/120 s)
    {
        Player pl; pl.reset(carById("golf")); Course k; k.reset(); k.state = RaceState::Play; k.clock = 0; k.nextCp = 1e12;
        k.accident();
        const double reste0 = k.timeLeft, step = 1.0 / 120; double acc = 0, jeu = 0;
        for (int f = 0; f < 60; f++) {   // 1 s réelle à 60 images/s
            acc += k.scaleDt(1.0 / 60);
            while (acc >= step) { k.step(step, pl, nullptr, 0, step / k.echelleDt); jeu += step; acc -= step; }
        }
        std::printf("  ralenti : 1 s réelle → chrono %.4f s, temps restant −%.4f s, temps de jeu %.4f s\n", k.raceTime, reste0 - k.timeLeft, jeu);
        check(std::abs(k.raceTime - 1.0) < 0.03 && std::abs((reste0 - k.timeLeft) - 1.0) < 0.03 && jeu < 0.8,
              "WFS-70 RG-05 : 1 s réelle de ralenti = 1 s de chrono (le temps de jeu, lui, est ralenti)");
        Course w; w.reset(); w.state = RaceState::Play; w.nextCp = 1e12; w.step(0.01, pl, nullptr, 0);
        check(std::abs(w.raceTime - 0.01) < 1e-12, "WFS-70 : sans chronoDt, chrono en temps de jeu (course de référence inchangée)");
    }
    // WFS-70 RG-02 à RG-04 : secousse, flash, « réduire les mouvements »
    {
        EtatRessenti e; Mulberry32 rng(1);
        e.chocFort(false);
        check(e.secousse == 0.9f && e.flash == 1.f, "WFS-70 RG-02 choc fort : secousse 0,9, flash 1");
        EtatRessenti l; l.chocLeger(0.2, false);
        check(std::abs(l.secousse - 0.55f) < 1e-6 && l.flash == 0.35f, "WFS-70 RG-02 choc léger : secousse 0,25 + impact × 1,5, flash 0,35");
        EtatRessenti fr; fr.frottement(false); EtatRessenti bo; bo.bordure(false);
        check(fr.secousse == 0.3f && fr.flash == 0.25f && bo.secousse == 0.12f, "WFS-70 RG-03 frottement 0,3 / 0,25, bordure 0,12");
        const vec3 d = e.decalage(rng, false);
        check(std::abs(d.x) <= 0.27f && std::abs(d.y) <= 0.18f && (d.x != 0 || d.y != 0), "WFS-70 secousse : décalage de la caméra borné (±0,27 ; ±0,18 m)");
        e.avancer(0.2);
        check(std::abs(e.secousse - 0.7f) < 1e-6 && std::abs(e.flash - 0.4f) < 1e-6, "WFS-70 décroissance : shake −dt, flash −3 dt");
        EtatRessenti r; r.chocFort(true); r.chocLeger(0.5, true); r.frottement(true); r.bordure(true);
        const vec3 z = e.decalage(rng, true);
        check(r.secousse == 0 && r.flash == 1.f && z.x == 0 && z.y == 0 && z.z == 0,
              "WFS-70 RG-04 réduire les mouvements : secousse nulle (flash gardé)");
        reglagesRessenti().reduireMouvements = true;
        Course k; k.reset(); k.accident();
        check(k.timeScale == 1 && k.slowmoT == 0 && k.accidents == 1, "WFS-70 RG-04 réduire les mouvements : pas de ralenti");
        reglagesRessenti().reduireMouvements = false;
        k.reset(); k.accident();
        check(k.timeScale == 0.35 && k.slowmoT == 0.5, "WFS-70 RG-02 ralenti à 35 % pendant 0,5 s (option désactivée)");
    }
    // WFS-70 RG-08 : modes de direction
    {
        check(reglagesRessenti().direction == ModeDirection::Glisser, "WFS-70 RG-08 mode par défaut : zone de glissement");
        check(std::abs(braquageGlisser(100, 100 + 1280 * 0.06f, 1280) - 0.5f) < 1e-5f && braquageGlisser(100, 900, 1280) == 1.f,
              "WFS-70 glissement : 6 % de la largeur = 0,5, butée à 1");
        check(braquageFleches(true, false) == -1 && braquageFleches(false, true) == 1 && braquageFleches(false, false) == 0,
              "WFS-70 flèches : consigne −1 / +1 / 0");
        Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf"));
        DriveInput in; in.steer = braquageFleches(true, false);
        for (int i = 0; i < 12; i++) pl.update(1.0 / 120, in, t, true);
        const double monte = pl.steer;
        for (int i = 0; i < 60; i++) pl.update(1.0 / 120, in, t, true);
        const double plein = pl.steer;
        in.steer = braquageFleches(false, false);
        for (int i = 0; i < 60; i++) pl.update(1.0 / 120, in, t, true);
        std::printf("  flèches : braquage %.3f après 0,1 s, %.3f après 0,6 s, %.3f 0,5 s après le relâchement\n", monte, plein, pl.steer);
        check(monte < -0.2 && monte > -0.6 && plein == -1.0 && std::abs(pl.steer) < 1e-9,
              "WFS-70 RG-08 flèches : braquage progressif vers −1, retour à 0 au relâchement");
        Marges m; const BoutonsFleches b = boutonsFleches(1280, 720, m);
        check(b.xg + b.c < b.xd && b.xg > 0 && b.y + b.c < 720 && dansZoneDirection(b.xd + b.c, 1280, m) &&
              !flecheDroite(b.xg + b.c / 2, b) && flecheDroite(b.xd + b.c / 2, b), "WFS-70 flèches : deux boutons à gauche, dans la zone de direction");
    }
    // WFS-98 RG-01, RG-03, RG-05, RG-05 bis : place des pédales et du nitro, commande sous chaque doigt (droitier et gaucher)
    {
        // banc = écran des bancs d'essai (720p, 2400 × 1080 avec encoche de 90 px à gauche, WFS_ENCOCHE)
        struct EcranEssai { float W, H; Marges m; bool banc; };
        const EcranEssai ecrans[] = {{1280, 720, {}, true}, {2400, 1080, {90, 0, 0, 0}, true}, {2400, 1080, {0, 0, 90, 0}, false},
                                     {1778, 1000, {0, 0, 0, 24}, false}};
        bool tailles = true, zone = true, dessin = true, gaz = true, appuis = true;
        for (const EcranEssai& e : ecrans)
            for (const bool gaucher : {false, true}) {
                const float W = e.W, H = e.H, k = H / 720.f;   // WFS-108 : les commandes en dp sont au moins à cette taille
                const Marges m = gaucher ? Marges{e.m.droite, e.m.haut, e.m.gauche, e.m.bas} : e.m;   // ecrans::margesCommandes
                const DispositionCommandes d = dispositionCommandes(W, H, m);
                tailles = tailles && d.gaz.w >= 64 * k && d.gaz.h >= 120 * k && d.frein.w >= 84 * k && d.frein.h >= 88 * k &&
                          d.nitro.w >= 68 * k && d.nitro.h == d.nitro.w;
                zone = zone && d.gaz.x + d.gaz.w <= W - m.droite && d.gaz.y + d.gaz.h <= H - m.bas && d.frein.y + d.frein.h <= H - m.bas &&
                       d.nitro.y + d.nitro.h < d.frein.y && d.frein.x + d.frein.w < d.gaz.x &&   // WFS-108 : nitro au-dessus du frein
                       !dansZoneDirection(d.nitro.x, W, m);
                // la zone de toucher peut déborder du dessin, jamais l'inverse : chaque point dessiné donne sa commande
                auto couvre = [&](const Cadre& c, Commande attendue) {
                    for (int i = 0; i <= 8; i++)
                        for (int j = 0; j <= 8; j++)
                            if (commandeSous(c.x + 0.5f + (c.w - 1) * i / 8, c.y + 0.5f + (c.h - 1) * j / 8, W, H, m, gaucher) != attendue)
                                return false;
                    return true;
                };
                dessin = dessin && couvre(d.gaz, Commande::Gaz) && couvre(d.frein, Commande::Frein);
                for (int i = 0; i < 16; i++)
                    for (int j = 0; j <= 4; j++) {
                        const float a = 2 * PI * i / 16, rho = (d.nitro.w / 2 - 0.5f) * j / 4;
                        dessin = dessin && commandeSous(d.nitro.cx() + rho * std::sin(a), d.nitro.cy() + rho * std::cos(a), W, H, m, gaucher) ==
                                               Commande::Nitro;
                    }
                // appui du gaz des bancs : (0,89 W ; 0,74 H), et (0,11 W ; 0,74 H) en gaucher, passé par ecrans::miroirX
                const float xGaz = gaucher ? W - 0.11f * W : 0.89f * W;
                gaz = gaz && commandeSous(xGaz, 0.74f * H, W, H, m, gaucher) == Commande::Gaz;
                // autres appuis des bancs : demi-tour (0,9 W ; 0,8 H) au gaz, frein à 0,66 W, coin haut droit = nitro (nitro-lignes),
                // moitié gauche = direction
                if (e.banc)
                    appuis = appuis && commandeSous(0.9f * W, 0.8f * H, W, H, m, gaucher) == Commande::Gaz &&
                             commandeSous(0.66f * W, 0.8f * H, W, H, m, gaucher) == Commande::Frein &&
                             commandeSous(0.9f * W, 0.32f * H, W, H, m, gaucher) == Commande::Nitro &&
                             commandeSous(0.5f * W, 0.8f * H, W, H, m, gaucher) == Commande::Aucune;
            }
        const DispositionCommandes d = dispositionCommandes(1280, 720, Marges{});
        std::printf("  720p : gaz [%g, %g] × [%g, %g], frein [%g, %g] × [%g, %g], nitro centre (%g, %g) r %g\n", d.gaz.x, d.gaz.x + d.gaz.w,
                    d.gaz.y, d.gaz.y + d.gaz.h, d.frein.x, d.frein.x + d.frein.w, d.frein.y, d.frein.y + d.frein.h, d.nitro.cx(),
                    d.nitro.cy(), d.nitro.w / 2);
        check(tailles, "WFS-98 RG-01 / RG-03 : gaz ≥ 64 × 120, frein ≥ 84 × 88, nitro ≥ 68 px CSS × k");
        check(zone, "WFS-98 RG-05 : pédales et nitro dans la zone sûre, sans chevauchement, hors de la zone de direction");
        check(dessin, "WFS-98 RG-05 bis : tout point dessiné est dans la zone de sa commande (gaz, frein, disque du nitro)");
        check(gaz, "WFS-98 RG-05 bis : (0,89 W ; 0,74 H) au gaz, (0,11 W ; 0,74 H) en gaucher");
        check(appuis, "WFS-98 : appuis des bancs (demi-tour au gaz, frein à 0,66 W, nitro du coin haut droit, rien à gauche)");
    }
    bad += testCommandes();   // WFS-108 : commandes agrandies, frein / marche arrière, gyroscope (tests/test_commandes.cpp)
    if (bad) { std::printf("ÉCHEC (%d)\n", bad); return 1; }
    std::printf("OK\n");
    return 0;
}
