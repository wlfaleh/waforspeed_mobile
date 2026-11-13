// WaForSpeed — garage : bancs d'essai (WFS-534, sortis de garage.cpp sans changement) : benchGarage (captures garage-*,
// showroom-suivante-*, course-p911-gris) et benchGarageTour (WFS-404 : une capture par place du garage). Voir garage.h.
#include <cstring>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "ecrans/terrain.h"   // WFS-172 (banc) : « Rouler » après « Choisir »
#include "garage.h"
#include "garage_commun.h"
#include "text.h"

namespace wfs::ecrans {
using namespace garage;

// WFS-404 (revue de la PR #337) : tour complet du garage, une capture par place (« NN / 17 »), flèche « suivante » : on passe aux
// places où étaient l'A 45, l'Elantra N et le X5 (retirées) sans les voir : garage-tour-NN-<id>
void benchGarageTour(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("garage (tour) : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 950;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Garage);
    for (int i = 0; i < 40; i++) image();
    if (!garageEnCours()) { WFS_ERR("garage (tour) : système absent"); g.shutdown(); return; }
    for (int k = 0; k < voituresPermises().n; k++) {
        for (int i = 0; i < 45; i++) image();   // fin du glissement latéral
        const char* id = CARS[garageEnCours()->index()].id;
        const std::string nom = "garage-tour-" + deuxChiffres(k + 1) + "-" + id;
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : place %d / %d%s", nom.c_str(), placeDe(garageEnCours()->index()) + 1, voituresPermises().n, voitureRetiree(id) ? " — VOITURE RETIRÉE" : "");
        float x, y;
        if (!garageBouton("suivant", x, y)) { WFS_ERR("garage (tour) : flèche « suivante » absente"); break; }
        g.onTouch({40 + k, x, y, true}); image(); g.onTouch({40 + k, x, y, false}); image();
    }
    g.shutdown();
}

void benchGarage(Game& g, Renderer& r, const std::string& out, const std::string& suf) {
    ui::chargerPolice(r);   // déjà chargée après l'écran de chargement ; nécessaire en WFS_BANC=garage
    if (!g.init(r, "golf", "ville")) { WFS_ERR("garage : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 900;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + suf + ".png");
        WFS_LOG("capture %s%s : écran %s, %s", nom.c_str(), suf.c_str(), nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, float x, float y) { g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image(); };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.allerA(Ecran::Garage);
    for (int i = 0; i < 40; i++) image();   // panneau posé
    if (!garageEnCours()) { WFS_ERR("garage : système absent"); g.shutdown(); return; }
    const float H = (float)r.height(), W = (float)r.width();
    // garage-0/1/2 : 3 angles de la Golf (le plateau tourne seul : 0,35 rad/s ; glisser = tourner)
    for (int k = 0; k < 3; k++) {
        if (k > 0) {   // glisser lent de 180 px (0,01 rad / px) sur la voiture
            const float x0 = W * 0.4f, y0 = H * 0.5f, d = 180 * H / 720;
            g.onTouch({9, x0, y0, true}); image();
            for (int s = 1; s <= 30; s++) { g.onTouch({9, x0 + d * s / 30, y0, true}); image(); }
            g.onTouch({9, x0 + d, y0, false}); image();
        }
        capture("garage-" + std::to_string(k), k == 0 ? "Golf sur le plateau, fiche complète" : "autre angle (glisser)");
    }
    // garage-peinture-0/1 : deux peintures différentes (rouge, puis turquoise)
    garageEnCours()->choisirPeinture(peintureParId("rouge")); for (int i = 0; i < 12; i++) image();
    capture("garage-peinture-0", "Golf en Rouge");
    garageEnCours()->choisirPeinture(peintureParId("turquoise")); for (int i = 0; i < 12; i++) image();
    capture("garage-peinture-1", "Golf en Turquoise");
    // garage-m4 : geste rapide vers la gauche (< 320 ms, > 70 px) → voiture suivante… jusqu'à la M4
    float x, y;
    while (garageEnCours() && std::strcmp(CARS[garageEnCours()->index()].id, "m4") != 0) {
        g.onTouch({10, W * 0.6f, H * 0.5f, true}); g.update(1.0 / 60); t += 1.0 / 60; g.render(t);
        g.onTouch({10, W * 0.6f - 160 * H / 720, H * 0.5f, true}); g.update(1.0 / 60); t += 1.0 / 60; g.render(t);
        g.onTouch({10, W * 0.6f - 160 * H / 720, H * 0.5f, false});
        for (int i = 0; i < 8; i++) image();
        if (garageEnCours()->index() == 0) break;   // tour complet sans M4
    }
    for (int i = 0; i < 60; i++) image();   // fin du glissement latéral
    capture("garage-m4", "BMW M4, nom et index à jour (geste rapide)");
    // flèche « suivant » : Porsche 911
    if (garageBouton("suivant", x, y)) { toucher(11, x, y); for (int i = 0; i < 60; i++) image(); capture("garage-suivant", "flèche suivante"); }
    // planche des 16 peintures (Golf)
    if (garageBouton("precedent", x, y)) { toucher(12, x, y); toucher(13, x, y); for (int i = 0; i < 30; i++) image(); }
    auto planche = [&](const std::string& nom) {
        FrameParams fp; SceneDediee s; GameView v; v.ecran = Ecran::Garage; v.uiWidth = W; v.uiHeight = H;
        garageEnCours()->reglerScene(fp, s, v);
        const vec3 eye{0, 17, 9}, look{0, 0, 0.3f};
        fp.view = lookAt(eye, look, {0, 1, 0}); fp.camPos = eye;
        fp.proj = perspectiveVk(36.f * PI / 180.f, W / H, 0.1f);
        fp.fogNear = 30; fp.fogFar = 60;
        if (r.beginFrame(fp)) { garageEnCours()->dessinerPlanche(r); r.endFrame(); }
        if (r.beginFrame(fp)) { garageEnCours()->dessinerPlanche(r); r.endFrame(); }
        r.capturePng(out + "/" + nom + suf + ".png");
        WFS_LOG("capture %s%s : 16 peintures sur la %s", nom.c_str(), suf.c_str(), CARS[garageEnCours()->index()].model);
    };
    planche("garage-peintures");
    // « Choisir » : la Golf en Turquoise part en course (décompte puis course)
    if (garageBouton("choisir", x, y)) {
        toucher(14, x, y);
        // WFS-172 (RG-01, ANO-80) : « Choisir » mène au terrain ; le banc appuie sur « Rouler » avant les 5 s de course
        for (int i = 0; i < 20; i++) image();
        g.render(t); float rx, ry;
        if (centreRouler(rx, ry)) toucher(16, rx, ry); else WFS_ERR("garage : « Rouler » absent (écran %s)", nomEcran(g.ecran()));
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        g.setAutopilot(true);
        for (int i = 0; i < 60 * 5; i++) image();
        g.setAutopilot(false);
        capture("garage-course", "course avec la voiture et la peinture choisies");
    }
    // « Retour » : du garage au menu sans changement
    g.allerA(Ecran::Garage); for (int i = 0; i < 30; i++) image();
    if (garageBouton("retour", x, y)) { toucher(15, x, y); for (int i = 0; i < 30; i++) image(); WFS_LOG("garage : « Retour » → écran %s", nomEcran(g.ecran())); }
    g.allerA(Ecran::Garage); for (int i = 0; i < 30; i++) image();
    // WFS-113 : 3 appuis sur « suivante », chacun avec des micro-mouvements du doigt (Android : ACTION_MOVE = appui) :
    // exactement une voiture de plus à chaque appui (showroom-suivante-1/2/3)
    for (int k = 1; k <= 3 && garageBouton("suivant", x, y); k++) {
        const int avant = garageEnCours() ? garageEnCours()->index() : -1;
        g.onTouch({20 + k, x, y, true});
        for (int m = 0; m < 6; m++) { g.onTouch({20 + k, x + (m % 2 ? 2.f : -2.f), y + 1.f, true}); image(); }
        g.onTouch({20 + k, x, y, false});
        for (int i = 0; i < 40; i++) image();
        capture("showroom-suivante-" + std::to_string(k), "une voiture de plus par appui malgré les micro-mouvements");
        WFS_LOG("showroom (WFS-113) : appui %d sur « suivante » → %s (%d → %d)", k, garageEnCours() ? CARS[garageEnCours()->index()].model : "?", avant + 1,
                garageEnCours() ? garageEnCours()->index() + 1 : 0);
    }
    // WFS-182 (RG-04, ANO-92) : planche des 16 peintures sur la 911, puis la 911 en « Gris » en course (peinture appliquée)
    for (int k = 0; k < CAR_COUNT && garageEnCours() && std::strcmp(CARS[garageEnCours()->index()].id, "p911") != 0 && garageBouton("suivant", x, y); k++) {
        toucher(30, x, y); for (int i = 0; i < 40; i++) image();
    }
    if (garageEnCours() && !std::strcmp(CARS[garageEnCours()->index()].id, "p911")) {
        planche("garage-peintures-p911");
        garageEnCours()->choisirPeinture(peintureParId("gris")); for (int i = 0; i < 12; i++) image();
        float rx, ry;
        if (garageBouton("choisir", x, y)) { toucher(31, x, y); for (int i = 0; i < 20; i++) image(); g.render(t); }
        if (centreRouler(rx, ry)) toucher(32, rx, ry); else WFS_ERR("garage : « Rouler » absent pour la 911 (écran %s)", nomEcran(g.ecran()));
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        g.setAutopilot(true); for (int i = 0; i < 60 * 5; i++) image(); g.setAutopilot(false);
        capture("course-p911-gris", "Porsche 911 en Gris en course (WFS-182)");
    } else WFS_ERR("garage : Porsche 911 introuvable au garage (WFS-182)");
    g.shutdown();
}

} // namespace wfs::ecrans
