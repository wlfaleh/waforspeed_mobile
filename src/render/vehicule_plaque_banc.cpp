// WaForSpeed — WFS-483 : banc de la plaque 3D du joueur (WFS_BANC=plaque). Captures demandées par le ticket :
//   - plaque-<id>-ar-jour / -av-jour / -ar-nuit : 208, GT86, T-Cross (id tcross) de 3/4 arrière et de 3/4 avant (Ville) ;
//   - plaque-<id>-poursuite-jour / -nuit : en course, caméra de poursuite (vue arrière) ;
//   - plaque-<id>-zoom : gros plan de 3/4 arrière (relief des caractères, vis, porte-plaque, jonction avec la caisse) ;
//   - plaque-1car / plaque-8car : plaques « 7 » et « WFS-2026 » (1 et 8 caractères) en gros plan ;
//   - plaque-garage, plaque-garage-ar : garage de la 208 de 3/4 avant, puis de 3/4 arrière ; plaque-planche-<id> : les autres voitures permises, 3/4 arrière (RG-04 : toutes).
//   - plaque-mesure (WFS_BANC=plaque-mesure, RG-07) : Corniche, Basse, 208 en course, caméra de poursuite ; 180 images de
//     chauffe puis 30 paires de blocs de 20 images chronométrées (update + render), plaques dessinées et masquées en alternance
//     (même partie, même scène : la dérive de la machine s'annule) ; écart moyen par image et erreur type au journal et dans
//     plaque-mesure.txt.
// WFS_PLAQUE_VOITURES="id id …" : seulement ces voitures ; WFS_PLAQUE_PLANCHE=1 : avec la planche ; WFS_PLAQUE_AVANT=1 : sans les
// plaques 3D (images « avant », identiques à main : le banc n'existe pas sur main). Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "renderer.h"
#include "vehicule_plaque.h"

namespace wfs {
namespace {

void image(Game& g, Renderer& r, const std::string& fichier, double t) {
    g.render(t); g.render(t + 0.001); g.render(t + 0.002);
    r.capturePng(fichier);
    WFS_LOG("capture %s (plaque « %s »)", fichier.substr(fichier.rfind('/') + 1).c_str(), plaque_joueur::texteAffiche().c_str());
}

void orbite(Game& g, float angle, float recul, float hauteur) { g.camMode = 2; g.orbit = angle; g.orbitRecul = recul; g.orbitHauteur = hauteur; }

bool voitureDemandee(const char* id) {
    const char* e = std::getenv("WFS_PLAQUE_VOITURES");
    if (!e) return true;
    std::istringstream s(e); std::string x;
    while (s >> x) if (x == id) return true;
    return false;
}

// course : la voiture roule quelques secondes (pilote automatique), caméra de poursuite proche
void poursuite(Game& g, Renderer& r, const std::string& fichier) {
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    for (int k = 0; k < 420; k++) { g.update(1.0 / 60); if (k % 10 == 0) g.render(970 + k / 60.0); }
    image(g, r, fichier, 977);
    g.setAutopilot(false);
}

} // namespace

void benchPlaque(Game& g, Renderer& r, const std::string& out) {
    const float recul0 = g.orbitRecul, haut0 = g.orbitHauteur;
    // WFS_PLAQUE_AVANT=1 : plaques 3D masquées, la plaque d'origine du modèle reste (rendu de main, pour l'avant / après de la PR)
    const bool avant = std::getenv("WFS_PLAQUE_AVANT") != nullptr;
    plaque_joueur::masquerBanc(avant);
    const char* principales[] = {"p208", "gt86", "tcross"};
    for (const char* id : principales) {
        if (!voitureDemandee(id)) continue;
        for (int nuit = 0; nuit < 2; nuit++) {
            if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
            if (!g.init(r, id, "ville")) { WFS_ERR("plaque : chargement %s impossible", id); continue; }
            const std::string base = out + "/plaque-" + id;
            orbite(g, 0.55f, 5.2f, 1.25f); image(g, r, base + "-ar-" + (nuit ? "nuit" : "jour") + ".png", 900);
            if (!nuit) {
                orbite(g, 3.14159f - 0.6f, 5.2f, 1.25f); image(g, r, base + "-av-jour.png", 901);
                orbite(g, 0.15f, 2.95f, 0.62f); image(g, r, base + "-zoom.png", 902);
            }
            poursuite(g, r, base + "-poursuite-" + (nuit ? "nuit" : "jour") + ".png");
            g.shutdown();
        }
    }
    unsetenv("WFS_MOMENT");
    // 1 et 8 caractères (RG-03) : texte court centré, texte long à la taille fixe
    if (voitureDemandee("p208") && g.init(r, "p208", "ville")) {
        plaque_joueur::texteBanc("7");
        orbite(g, 0.05f, 3.1f, 0.62f); image(g, r, out + "/plaque-1car.png", 910);
        plaque_joueur::texteBanc("WFS-2026");
        image(g, r, out + "/plaque-8car.png", 911);
        plaque_joueur::texteBanc(nullptr);
        // garage (RG-06 : lisible au garage)
        g.allerA(Ecran::Garage);
        for (int k = 0; k < 40; k++) { g.update(1.0 / 60); if (k % 10 == 0) g.render(950 + k / 60.0); }
        image(g, r, out + "/plaque-garage.png", 951);
        // RG-06 : garage de 3/4 arrière : la voiture tourne seule (0,35 rad/s) ; ≈ 7 s de plus montrent l'arrière
        for (int k = 0; k < 420; k++) { g.update(1.0 / 60); if (k % 30 == 0) g.render(952 + k / 60.0); }
        image(g, r, out + "/plaque-garage-ar.png", 960);
        g.shutdown();
    }
    // les autres voitures permises (RG-04 : plaque sur toutes les voitures du garage)
    if (std::getenv("WFS_PLAQUE_PLANCHE")) {
        for (int i = 0; i < CAR_COUNT; i++) {
            const char* id = CARS[i].id;
            if (voitureRetiree(id) || !voitureDemandee(id) || !plaque_joueur::places(id)) continue;
            if (!g.init(r, id, "ville")) continue;
            orbite(g, 0.55f, 5.2f, 1.25f); image(g, r, out + "/plaque-planche-" + id + "-ar.png", 920);
            orbite(g, 3.14159f - 0.6f, 5.2f, 1.25f); image(g, r, out + "/plaque-planche-" + id + "-av.png", 921);
            g.shutdown();
        }
    }
    g.orbitRecul = recul0; g.orbitHauteur = haut0; g.camMode = 0;
    plaque_joueur::masquerBanc(false);
}

void benchPlaqueMesure(Game& g, Renderer& r, const std::string& out) {
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
    if (!g.init(r, "p208", "plage")) { WFS_ERR("plaque-mesure : chargement impossible"); R.qualite = q0; return; }
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    double t = 1000;
    for (int k = 0; k < 180; k++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }
    // 30 paires de blocs de 20 images (avec / sans, ordre alterné d'une paire à l'autre) : écart moyen et erreur type des paires
    constexpr int PAIRES = 30, IMAGES = 20;
    std::vector<double> ecarts;
    double somme[2] = {0, 0};
    for (int p = 0; p < PAIRES; p++) {
        double bloc[2] = {0, 0};
        for (int k = 0; k < 2; k++) {
            const int avec = (k + p) % 2;   // 1 : plaques dessinées
            plaque_joueur::masquerBanc(!avec);
            for (int i = 0; i < IMAGES; i++) {
                const double t0 = nowSeconds();
                g.update(1.0 / 60); g.render(t); t += 1.0 / 60;
                bloc[avec] += (nowSeconds() - t0) * 1000;
            }
        }
        ecarts.push_back((bloc[1] - bloc[0]) / IMAGES);
        somme[0] += bloc[0]; somme[1] += bloc[1];
    }
    plaque_joueur::masquerBanc(false);
    double m = 0, v = 0;
    for (double e : ecarts) m += e;
    m /= ecarts.size();
    for (double e : ecarts) v += (e - m) * (e - m);
    const double et = std::sqrt(v / (ecarts.size() - 1) / ecarts.size());
    const double avec = somme[1] / (PAIRES * IMAGES), sans = somme[0] / (PAIRES * IMAGES);
    char b[320];
    std::snprintf(b, sizeof b, "plaque-mesure : Basse, Corniche, 208, poursuite : avec plaques %.2f ms, sans %.2f ms par image ; écart %+.2f ms ± %.2f ms "
                  "(erreur type, %d paires de %d images) = %+.2f %% du temps d'image", avec, sans, m, et, PAIRES, IMAGES, 100 * m / sans);
    WFS_LOG("%s", b);
    std::ofstream(out + "/plaque-mesure.txt") << b << "\n";
    g.setAutopilot(false);
    g.shutdown();
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
