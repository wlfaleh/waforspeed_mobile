// WaForSpeed — WFS-484 : banc des plaques des autres voitures (WFS_BANC=plaques-autres), Ville, Golf du joueur, vers 30 km/h.
// Captures demandées par le ticket (vue Proche, caméra derrière le joueur ; jour et nuit) :
//   - plaques-trafic-<moment> : une voiture du trafic 8 m devant le joueur, dans sa file ;
//   - plaques-trafic-avant-<moment> / -arriere-<moment> : gros plans à 4 m des plaques avant et arrière d'une voiture du trafic ;
//   - plaques-police-<moment>, -avant-<moment>, -arriere-<moment> : voiture de police de la Poursuite, mêmes vues ;
//   - plaques-course5-jour : Course à 5, les 4 adversaires devant (plaques arrière) ; plaques-course5-arriere-jour : gros plan ;
//   - plaques-fantome-jour : fantôme du record (plaque du joueur, translucide) 5 m devant.
// WFS_PLAQUES_AUTRES_AVANT=1 : plaques des autres voitures masquées (images « avant » : rendu de main, où ce banc n'existe pas).
// WFS_BANC=plaques-autres-mesure (RG-05) : Corniche, Basse, Poursuite (trafic dense de la Poursuite en Basse, police), vue
// Poursuite ; 180 images de chauffe puis 30 paires de blocs de 20 images chronométrées (update + render), plaques des autres
// voitures dessinées et masquées en alternance (même partie : la dérive de la machine s'annule) ; écart au journal et dans
// plaques-autres-mesure.txt (exigence : ≤ +0,5 ms).
// Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche). N'utilise que des accroches de banc déjà sur main
// (trafficBenchPlace, course5BancSuiveurs, poursuiteBancPolice, courseBancFantome).
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "../core/platform.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/fantome.h"
#include "../game/game.h"
#include "../game/roues_banc.h"
#include "../game/traffic.h"
#include "../game/traffic_system.h"
#include "../game/cars.h"
#include "renderer.h"
#include "vehicule_plaques_autres.h"

namespace wfs {

const std::vector<course5::Ordinateur>* course5Ordinateurs();   // course5_system.cpp (lecture seule)

void benchPlaquesAutres(Game& g, Renderer& r, const std::string& out) {
    const bool avant = std::getenv("WFS_PLAQUES_AUTRES_AVANT") != nullptr;
    plaques_autres::masquerBanc(avant);
    const ModeCourse mode0 = modeCourse();
    const VueCamera vue0 = vueCamera();
    constexpr double PAS = 1.0 / 60;
    courseBenchRules(true);
    double t = 1400;
    double cible = 30;   // km/h : le pilote automatique (qui freine derrière une voiture) roule jusqu'à cette vitesse
    auto pas = [&](bool rendu) {
        g.setAutopilot(g.player().kmh() < cible);
        g.update(PAS); t += PAS;
        if (rendu) g.render(t);
    };
    auto image = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + PAS); t += PAS;
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s (%s) : %s, %.0f km/h", nom.c_str(), avant ? "avant : plaques masquées" : "après", quoi, g.player().kmh());
    };
    // caméra « vitrine » (elle vise toujours le joueur) : point de vue à avantM devant et droiteM à droite de sa voiture, à
    // hauteur m. Gros plans : la voiture visée est placée entre la caméra et le joueur, à 4 m de la caméra, basse (plaque au-dessus du compteur du HUD)
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.camMode = 2; g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    auto demarrer = [&](ModeCourse mode, bool nuit) {
        setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        modeCourse() = mode; cible = 30;
        if (mode == ModeCourse::Course5) kmCourse5() = 10;
        if (!g.init(r, "golf", "ville")) { WFS_ERR("plaques-autres : chargement de la Ville impossible"); return false; }
        choisirVue(VueCamera::Proche);
        g.allerA(Ecran::Course); g.camMode = 0;
        for (int i = 0; i < 60 * 25 && g.player().kmh() < 29; i++) pas(i % 8 == 0);   // départ, puis 30 km/h
        return true;
    };
    // voiture du trafic placée par trafficBenchPlace : écart (m, > 0 : devant le joueur) et décalage latéral (m)
    auto trafic = [&](double& devant, double& cote) {
        const int k = trafficBenchCar();
        const Traffic* tf = activeTraffic();
        if (!tf || k < 0 || k >= (int)tf->cars.size()) { WFS_ERR("plaques-autres : aucune voiture du trafic placée"); return false; }
        const TrafficCar& c = tf->cars[(size_t)k];
        devant = (c.z - g.player().position) * M; cote = (c.offset - g.player().playerX) * RW;
        WFS_LOG("plaques-autres : trafic n° %d (%s) « %s » à %.1f m (latéral %.1f m)", k, WEB_CARS[c.type].id,
                plaques_autres::texteCase(plaques_autres::TRAFIC_0 + k % plaques_autres::TRAFIC_N).c_str(), devant, cote);
        return true;
    };
    for (int nuit = 0; nuit < 2; nuit++) {
        const std::string moment = nuit ? "nuit" : "jour";
        // ---- trafic, vue Proche : une voiture 8 m devant, dans la file du joueur (pilote automatique : il la suit)
        if (demarrer(ModeCourse::Checkpoint, nuit)) {
            cible = 300; trafficBenchPlace(8, -0.3, 0, true);   // joueur 2,2 m à côté de sa file : la voiture n'est pas cachée
            for (int i = 0; i < 20; i++) pas(i % 4 == 0);
            double devant = 0, cote = 0;
            if (trafic(devant, cote)) {
                g.camMode = 0; image("plaques-trafic-" + moment, "trafic devant, vue Proche");
                viser(devant + 4.0, cote + 0.9, 0.35f); pas(false); image("plaques-trafic-avant-" + moment, "plaque avant du trafic, 4 m");
            }
            g.shutdown();
        }
        // ---- trafic : gros plan de la plaque arrière (la voiture 8 m derrière le joueur, la caméra 4 m derrière elle)
        if (demarrer(ModeCourse::Checkpoint, nuit)) {
            cible = 300; trafficBenchPlace(-8, 0, 0, true);
            for (int i = 0; i < 20; i++) pas(i % 4 == 0);
            double devant = 0, cote = 0;
            if (trafic(devant, cote)) { viser(devant - 4.0, cote - 0.9, 0.35f); pas(false); image("plaques-trafic-arriere-" + moment, "plaque arrière du trafic, 4 m"); }
            g.shutdown();
        }
        // ---- police de la Poursuite : 8 m devant (vue Proche), puis gros plans avant et arrière
        if (demarrer(ModeCourse::Poursuite, nuit)) {
            const double px = g.player().playerX + (g.player().playerX > 0 ? -0.3 : 0.3), cotePolice = (px - g.player().playerX) * RW;
            auto tenir = [&](double ecart, int n) { for (int i = 0; i < n; i++) { poursuiteBancPolice(true, ecart, px); pas(i % 4 == 0); } poursuiteBancPolice(true, ecart, px); };
            tenir(8, 40); g.camMode = 0; image("plaques-police-" + moment, "police devant, vue Proche");   // police 2,2 m à côté de la file du joueur
            viser(8 + 4.0, cotePolice + 0.9, 0.35f); tenir(8, 2); image("plaques-police-avant-" + moment, "plaque avant de la police, 4 m");
            tenir(-8, 40); viser(-8 - 4.0, cotePolice - 0.9, 0.35f); tenir(-8, 2); image("plaques-police-arriere-" + moment, "plaque arrière de la police, 4 m");
            poursuiteBancPolice(false, 0, 0);
            g.shutdown();
        }
    }
    unsetenv("WFS_MOMENT"); setenv("WFS_MOMENT", "jour", 1);
    // ---- Course à 5 : les 4 adversaires devant, dans les 3 files (vue Proche), puis gros plan d'un adversaire derrière
    if (demarrer(ModeCourse::Course5, false)) {
        course5BancSuiveurs({{8, 0}, {8, 2}, {14, 1}, {20, 0}});
        for (int i = 0; i < 40; i++) pas(i % 4 == 0);
        g.camMode = 0; image("plaques-course5-jour", "4 adversaires devant, vue Proche");
        course5BancSuiveurs({{-8, 1}, {14, 0}, {14, 2}, {20, 1}});
        for (int i = 0; i < 20; i++) pas(i % 4 == 0);
        double cote5 = 0, ecart5 = -8;   // place réelle de l'adversaire derrière le joueur (file, écart)
        if (const auto* os = course5Ordinateurs())
            for (const course5::Ordinateur& o : *os) if (o.g.rd < g.player().dist) { cote5 = (o.g.x - g.player().playerX) * RW; ecart5 = (o.g.rd - g.player().dist) * M; }
        viser(ecart5 - 4.0, cote5 - 0.9, 0.35f); pas(false); image("plaques-course5-arriere-jour", "plaque arrière d'un adversaire, 4 m");
        course5BancSuiveurs({});
        g.shutdown();
    }
    // ---- fantôme du record (plaque du joueur, translucide) 5 m devant, vue Proche
    if (demarrer(ModeCourse::Checkpoint, false)) {
        fantome::Trace tr; tr.terrain = "ville"; tr.car = "m4"; tr.peinture = 0x3a6ea5;
        const double v = g.player().speed, px = g.player().playerX + (g.player().playerX > 0 ? -0.3 : 0.3);   // 2,2 m à côté : pas caché par le joueur
        for (int k = 0; k < 400; k++) tr.e.push_back({g.player().dist + 5 / M + v * k * fantome::PAS_S, px, 0, v});
        courseBancFantome(tr);
        for (int i = 0; i < 30; i++) pas(i % 4 == 0);
        g.camMode = 0; image("plaques-fantome-jour", "fantôme devant (plaque du joueur), vue Proche");
        WFS_LOG("plaques-autres : fantôme « %s »", plaques_autres::texteCase(plaques_autres::FANTOME).c_str());
        g.shutdown();
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    g.setAutopilot(false); g.camMode = 0;
    modeCourse() = mode0; choisirVue(vue0);
    courseBenchRules(false);
    plaques_autres::masquerBanc(false);
}

void benchPlaquesAutresMesure(Game& g, Renderer& r, const std::string& out) {
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    const ModeCourse mode0 = modeCourse();
    R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
    modeCourse() = ModeCourse::Poursuite;
    setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", "jour", 1);
    if (!g.init(r, "golf", "plage")) { WFS_ERR("plaques-autres-mesure : chargement impossible"); R.qualite = q0; modeCourse() = mode0; return; }
    choisirVue(VueCamera::Poursuite);
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    double t = 1000;
    for (int k = 0; k < 180; k++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }
    constexpr int PAIRES = 30, IMAGES = 20;
    std::vector<double> ecarts;
    double somme[2] = {0, 0};
    for (int p = 0; p < PAIRES; p++) {
        double bloc[2] = {0, 0};
        for (int k = 0; k < 2; k++) {
            const int avec = (k + p) % 2;   // 1 : plaques des autres voitures dessinées
            plaques_autres::masquerBanc(!avec);
            for (int i = 0; i < IMAGES; i++) {
                const double t0 = nowSeconds();
                g.update(1.0 / 60); g.render(t); t += 1.0 / 60;
                bloc[avec] += (nowSeconds() - t0) * 1000;
            }
        }
        ecarts.push_back((bloc[1] - bloc[0]) / IMAGES);
        somme[0] += bloc[0]; somme[1] += bloc[1];
    }
    // appels de dessin et triangles ajoutés (compte exact, sans bruit) : même image dessinée sans puis avec les plaques
    double appels[2] = {0, 0}, triangles[2] = {0, 0};
    constexpr int ECHANTILLONS = 20;
    for (int k = 0; k < ECHANTILLONS; k++) {
        for (int i = 0; i < 6; i++) g.update(1.0 / 60);
        for (int avec = 0; avec < 2; avec++) {
            plaques_autres::masquerBanc(!avec);
            g.render(t); uint32_t a = 0, tri = 0; r.fileScene(a, tri);
            appels[avec] += a; triangles[avec] += tri;
        }
        t += 0.1;
    }
    plaques_autres::masquerBanc(false);
    double m = 0, v = 0;
    for (double e : ecarts) m += e;
    m /= ecarts.size();
    for (double e : ecarts) v += (e - m) * (e - m);
    const double et = std::sqrt(v / (ecarts.size() - 1) / ecarts.size());
    const double avec = somme[1] / (PAIRES * IMAGES), sans = somme[0] / (PAIRES * IMAGES);
    const Traffic* tf = activeTraffic();
    char b[640];
    std::snprintf(b, sizeof b, "plaques-autres-mesure : Basse, Corniche, Poursuite (%zu voitures de trafic) : avec plaques %.2f ms, sans %.2f ms par image ; "
                  "écart %+.2f ms ± %.2f ms (erreur type, %d paires de %d images) = %+.2f %% du temps d'image ; exigence RG-05 : ≤ +0,5 ms ; "
                  "appels de dessin de la scène : %.1f sans, %.1f avec (%+.1f) ; triangles : %.0f sans, %.0f avec (%+.0f) (moyenne de %d images)",
                  tf ? tf->cars.size() : (size_t)0, avec, sans, m, et, PAIRES, IMAGES, 100 * m / sans,
                  appels[0] / ECHANTILLONS, appels[1] / ECHANTILLONS, (appels[1] - appels[0]) / ECHANTILLONS,
                  triangles[0] / ECHANTILLONS, triangles[1] / ECHANTILLONS, (triangles[1] - triangles[0]) / ECHANTILLONS, ECHANTILLONS);
    WFS_LOG("%s", b);
    std::ofstream(out + "/plaques-autres-mesure.txt") << b << "\n";
    g.setAutopilot(false);
    g.shutdown();
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    modeCourse() = mode0;
}

} // namespace wfs
