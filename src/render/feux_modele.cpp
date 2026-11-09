// WaForSpeed — banc d'essai des feux arrière (WFS-109, RG-15) : voir feux_modele.h.
#include "feux_modele.h"
#include <cstdlib>
#include <sstream>
#include "../core/log.h"
#include "../game/cars.h"
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


void benchVitrinesFeux(Game& g, Renderer& r, const std::string& out) {
    std::vector<std::string> ids;
    const char* choix = std::getenv("WFS_VOITURES");
    if (choix) { std::istringstream is(choix); std::string id; while (is >> id) ids.push_back(id); }
    else for (const CarDef& c : CARS) ids.push_back(c.id);
    if (std::getenv("WFS_VITRINE_NUIT")) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");   // WFS-193 (banc) : vitrines de nuit
    for (const std::string& id : ids) {
        if (!g.init(r, id.c_str(), "ville")) { WFS_ERR("vitrine feux : chargement de %s impossible", id.c_str()); continue; }
        if (choix) { g.camMode = 2; g.orbit = 0.7f; g.render(0); g.render(0); r.capturePng(out + "/vitrine-" + id + "-0.png"); }
        // WFS-192 (accroche, essai local) : logos vus de près : WFS_VITRINE_ANGLES="<angle de l'orbite en rad> …" (0 : arrière,
        // 3,14 : avant, 1,57 et 4,71 : côtés) : captures vitrine-<id>-a<n> ; rien sans la variable
        if (const char* an = std::getenv("WFS_VITRINE_ANGLES")) {
            std::istringstream ia(an); float a; int k = 0; g.camMode = 2;
            while (ia >> a) { g.orbit = a; g.render(0); g.render(0); r.capturePng(out + "/vitrine-" + id + "-a" + std::to_string(k++) + ".png"); }
            // WFS-193 (banc) : vue basse, au niveau du pare-chocs : WFS_VITRINE_BAS="<angle> <hauteur m> <recul m>" → vitrine-<id>-bas
            // WFS-358 : plusieurs vues séparées par « ; » → vitrine-<id>-bas, -bas1, -bas2…
            if (const char* bas = std::getenv("WFS_VITRINE_BAS")) {
                std::istringstream vues(bas); std::string vue; int k = 0;
                const float h0 = g.orbitHauteur, d0 = g.orbitRecul;   // valeurs d'avant, remises après les captures
                while (std::getline(vues, vue, ';')) {
                    std::istringstream ib(vue); float h = 0.6f, d = 4.f; if (!(ib >> g.orbit >> h >> d)) continue;
                    g.orbitHauteur = h; g.orbitRecul = d; g.render(0); g.render(0);
                    r.capturePng(out + "/vitrine-" + id + "-bas" + (k ? std::to_string(k) : "") + ".png"); k++;
                }
                g.orbitHauteur = h0; g.orbitRecul = d0;
            }
        }
        double t = 1300;
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 120; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); }
        g.setAutopilot(false);
        const float W = (float)r.width(), H = (float)r.height();
        g.onTouch({7, W * 0.66f, H * 0.8f, true});   // pédale de frein tenue : feux stop allumés (transition de 80 ms finie)
        for (int i = 0; i < 12; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); }
        g.camMode = 2; g.orbit = 0.45f;              // vitrine, de trois quarts arrière
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/vitrine-" + id + "-feux.png");
        WFS_LOG("capture vitrine-%s-feux : frein %d (WFS-170)", id.c_str(), (int)g.player().braking);
        g.onTouch({7, W * 0.66f, H * 0.8f, false});
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");   // WFS-193 : rien de laissé aux bancs suivants
}

} // namespace wfs
