// WaForSpeed — banc d'essai de la conduite libre (WFS-90) : voir conduite_libre_banc.h. Pilotage par les vraies
// commandes tactiles (Game::onTouch) : glisser à gauche pour braquer, accélérateur à droite.
#include "conduite_libre_banc.h"
#include <algorithm>
#include <cmath>
#include "../core/log.h"
#include "../render/renderer.h"
#include "game.h"
#include "player.h"
#include "traffic_system.h"

namespace wfs {

void benchConduiteLibre(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) { WFS_ERR("conduite libre : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    const float W = (float)r.width(), H = (float)r.height(), x0 = W * 0.2f, yv = H * 0.7f;
    constexpr double PI_D = 3.14159265358979323846;
    double t = 1000;
    bool gaz = false;
    auto commandes = [&](float steer, bool gas) {
        g.onTouch({1, x0, yv, true});                                  // doigt posé : point de départ du glissé
        g.onTouch({1, x0 + std::clamp(steer, -1.f, 1.f) * W * 0.12f, yv, true});
        if (gas != gaz) { g.onTouch({2, W * 0.9f, H * 0.8f, gas}); gaz = gas; }
    };
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    const Player& p = g.player();
    // demi-tour : braquage à fond, gaz par petites touches (sous 4 m/s)
    int n = 0;
    bool demi = false;
    for (; n < 60 * 30 && std::abs(p.heading) < 3.0; n++) {
        commandes(1.f, p.speed * M < 4);
        image(n % 6 == 0);
        if (!demi && std::abs(p.heading) > PI_D / 2) {   // en travers de la route
            demi = true; g.render(t); g.render(t + 0.01);
            r.capturePng(out + "/conduite-demitour.png");
        }
    }
    WFS_LOG("conduite libre : demi-tour en %.1f s, cap %.2f rad (%.0f°), écart latéral %.2f", n / 60.0, p.heading, p.heading * 180 / PI_D, p.playerX);
    // à contresens : retour au milieu de la route et plein gaz (à contresens, dX/dt = −v sin(cap − π))
    for (int i = 0; i < 60 * 4; i++) {
        const double dev = p.heading - (p.heading > 0 ? PI_D : -PI_D);
        commandes((float)(p.playerX * 2.5 - dev * 3), true);
        image(i % 6 == 0);
    }
    g.render(t); g.render(t + 0.01);
    r.capturePng(out + "/conduite-contresens.png");
    int enFace = 0;
    if (const Traffic* tr = activeTraffic())
        for (const auto& c : tr->cars) { const double dz = (c.z - p.position) * M; if (dz < 0 && dz > -150) enFace++; }
    WFS_LOG("capture conduite-contresens : %.0f km/h, cap %.2f rad, dist %.0f m (diminue à contresens), %d voiture(s) du trafic à moins de 150 m devant elle",
            p.kmh(), p.heading, p.dist * M, enFace);
    g.onTouch({1, x0, yv, false}); if (gaz) g.onTouch({2, W * 0.9f, H * 0.8f, false});
    g.shutdown();
}

} // namespace wfs
