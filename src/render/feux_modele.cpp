// WaForSpeed — banc d'essai des feux arrière (WFS-109, RG-15) : voir feux_modele.h.
#include "feux_modele.h"
#include <cstdlib>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/track.h"
#include "renderer.h"

namespace wfs {

// RG-15 : la Golf du joueur (feu arrière découpé, RG-13) vue de la caméra arrière, en ville : (a) de jour au repos, feux
// éteints ; (b) au freinage, de jour comme de nuit, feux du modèle rouge vif ; (c) en marche arrière, feux de recul blancs
void benchFeuxFrein(Game& g, Renderer& r, const std::string& out) {
    double t = 1200;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto capture = [&](const std::string& nom) {
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, frein %d, recul %d (WFS-109)", nom.c_str(), g.player().kmh(), (int)g.player().braking,
                (int)g.player().reversing);
    };
    for (const char* moment : {"jour", "nuit"}) {
        const bool nuit = moment[0] == 'n';
        if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
        if (!g.init(r, "golf", "ville")) { WFS_ERR("feux : chargement impossible"); continue; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) image(i % 8 == 0);
        if (!nuit) capture("feux-repos-jour");
        // pédale de frein tenue, pilote coupé : 0,2 s de freinage (transition de 80 ms finie)
        g.setAutopilot(false);
        const float W = (float)r.width(), H = (float)r.height();
        g.onTouch({7, W * 0.66f, H * 0.8f, true});
        for (int i = 0; i < 12; i++) image(true);
        capture(std::string("feux-frein-") + moment);
        if (!nuit) {   // marche arrière : frein tenu jusqu'à l'arrêt, puis recul
            for (int i = 0; i < 1800 && !g.player().reversing; i++) image(i % 30 == 0);
            for (int i = 0; i < 90; i++) image(i % 8 == 0);
            capture("feux-recul");
        }
        g.onTouch({7, W * 0.66f, H * 0.8f, false});
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
}

} // namespace wfs
