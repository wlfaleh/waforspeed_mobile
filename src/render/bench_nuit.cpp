// WaForSpeed — scènes du banc d'essai de la nuit (WFS-09, WFS-16) : voir bench_nuit.h.
#include "bench_nuit.h"
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../game/ressenti.h"
#include "../ui/ecrans/reglages.h"
#include "renderer.h"

namespace wfs {

void benchNuit(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    double t = 800;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto capture = [&](const std::string& nom) {
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, %.0f m, frein %d, recul %d", nom.c_str(), g.player().kmh(), g.player().dist * M,
                (int)g.player().braking, (int)g.player().reversing);
    };
    for (const char* carte : {"ville", "campagne", "circuit"}) {
        if (!g.init(r, "golf", carte)) { WFS_ERR("nuit : chargement %s impossible", carte); continue; }
        const std::string base = std::string("course-nuit-") + carte;
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) image(i % 8 == 0);
        capture(base + "-0");
        if (!std::string(carte).compare("ville")) { image(true); capture(base + "-1"); }   // WFS-16 : pas de scintillement
        if (!std::string(carte).compare("ville")) {   // WFS-84 : voitures du trafic vues de derrière (feux nets, lueur douce)
            for (int i = 0; i < 300; i++) image(i % 8 == 0);
            capture("nuit-trafic-arriere");
        }
        g.camMode = 1;
        for (int i = 0; i < 30; i++) image(i % 8 == 0);
        capture(base + "-haut");
        if (!std::string(carte).compare("ville")) {
            // coût GPU (lavapipe) : 125 images en qualité haute (reflets, flaques), puis 125 en qualité basse (sans)
            WFS_LOG("nuit : mesure du coût, qualité haute — début");
            for (int i = 0; i < 125; i++) image(true);
            WFS_LOG("nuit : mesure du coût, qualité haute — fin ; qualité basse — début");
            ecrans::reglages().qualite = ecrans::QUALITE_BASSE;
            for (int i = 0; i < 125; i++) image(true);
            WFS_LOG("nuit : mesure du coût, qualité basse — fin");
            ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
            // freinage (feux stop), puis marche arrière (feux de recul) : pédale de frein tenue, pilote coupé
            g.setAutopilot(false);
            const float W = (float)r.width(), H = (float)r.height();
            g.onTouch({7, W * 0.66f, H * 0.8f, true});
            for (int i = 0; i < 20; i++) image(i % 8 == 0);
            capture(base + "-stop");
            for (int i = 0; i < 1800 && !g.player().reversing; i++) image(i % 30 == 0);
            for (int i = 0; i < 90; i++) image(i % 8 == 0);
            capture(base + "-recul");
            g.onTouch({7, W * 0.66f, H * 0.8f, false});
        }
        g.shutdown();
    }
    // WFS-218 (RG-04) : feux de croisement du joueur en qualité Basse, Ville et Corniche : poursuite (route, façades du bord,
    // coupure), poursuite haute (trottoirs, pieds de façade), puis vue de face à 6,5 m (les deux optiques allumées)
    ecrans::reglages().qualite = ecrans::QUALITE_BASSE;
    for (const char* carte : {"ville", "plage"}) {
        if (!g.init(r, "golf", carte)) { WFS_ERR("nuit : chargement %s impossible", carte); continue; }
        const std::string base = std::string("phares-basse-") + (std::string(carte) == "plage" ? "corniche" : "ville");
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) image(i % 8 == 0);
        capture(base + "-0");
        g.camMode = 1;
        for (int i = 0; i < 30; i++) image(i % 8 == 0);
        capture(base + "-haut");
        const Game::Vue v = g.calculerVue(t);
        g.camMode = 2; g.orbit = std::atan2(v.fwd.x, v.fwd.z);
        capture(base + "-face");
        g.shutdown();
    }
    ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
    unsetenv("WFS_MOMENT");
}

// WFS-142 : même départ, même voiture, même nombre d'images pour chaque état de la chaussée (comparaison avant / après)
void benchRoute(Game& g, Renderer& r, const std::string& out) {
    struct Cas { const char* nom; const char* carte; const char* moment; const char* meteo; int qualite; int images; int cam = 0; };
    const Cas cas[] = {
        {"route-nuit-ville-sec", "ville", "nuit", "sec", ecrans::QUALITE_HAUTE, 150},
        {"route-nuit-ville-pluie", "ville", "nuit", "pluie", ecrans::QUALITE_HAUTE, 150},
        {"route-nuit-ville-pluie-proche", "ville", "nuit", "pluie", ecrans::QUALITE_HAUTE, 150, 3},   // vue basse de la photo
        {"route-nuit-corniche", "plage", "nuit", nullptr, ecrans::QUALITE_HAUTE, 150},
        {"route-jour-sec", "ville", "jour", "sec", ecrans::QUALITE_HAUTE, 150},
        {"route-jour-pluie", "ville", "jour", "pluie", ecrans::QUALITE_HAUTE, 150},
        {"route-nuit-ville-pluie-basse", "ville", "nuit", "pluie", ecrans::QUALITE_BASSE, 150},
        {"route-nuit-ville-pluie-120", "ville", "nuit", "pluie", ecrans::QUALITE_HAUTE, 420},
    };
    const bool reduit = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;   // pas de gouttes à l'écran : seule la chaussée change d'une capture à l'autre
    for (const Cas& c : cas) {
        setenv("WFS_MOMENT", c.moment, 1);
        if (c.meteo) setenv("WFS_METEO", c.meteo, 1); else unsetenv("WFS_METEO");
        ecrans::reglages().qualite = c.qualite;
        if (!g.init(r, "golf", c.carte)) { WFS_ERR("route : chargement %s impossible", c.carte); continue; }
        g.setAutopilot(true); g.camMode = c.cam;
        double t = 800;
        for (int i = 0; i < c.images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); }
        const bool deux = c.images > 300;   // à 120 km/h : deux images proches (scintillement, déformation)
        for (int k = 0; k < (deux ? 2 : 1); k++) {
            if (k) { g.update(1.0 / 60); t += 1.0 / 60; }
            const std::string nom = std::string(c.nom) + (deux ? (k ? "-1" : "-0") : "");
            g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png");
            setenv("WFS_REFLETS", "0", 1);
            g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + "-sansreflet.png");
            unsetenv("WFS_REFLETS");
            WFS_LOG("capture %s : %.0f km/h, %.0f m (météo %s, qualité %d)", nom.c_str(), g.player().kmh(), g.player().dist * M,
                    c.meteo ? c.meteo : "de la carte", c.qualite);
        }
        g.shutdown();
    }
    ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
    reglagesRessenti().reduireMouvements = reduit;
    unsetenv("WFS_METEO"); unsetenv("WFS_MOMENT");
}

} // namespace wfs
