// WaForSpeed — scènes du banc d'essai de la nuit (WFS-09, WFS-16) : voir bench_nuit.h.
#include "bench_nuit.h"
#include <cstdlib>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/track.h"
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
    unsetenv("WFS_MOMENT");
}

} // namespace wfs
