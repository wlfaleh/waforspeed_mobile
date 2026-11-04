// WaForSpeed — scènes du banc d'essai des effets de conduite (WFS-65) : voir bench_effets.h.
#include "bench_effets.h"
#include "../core/log.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../game/traffic_system.h"
#include "effets_systeme.h"
#include "renderer.h"

namespace wfs {

void benchEffets(Game& g, Renderer& r, const std::string& out) {
    effetsBanc(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("effets : chargement impossible"); effetsBanc(false); return; }
    g.setAutopilot(true); g.camMode = 1;   // caméra haute : on voit l'avant de la voiture et la route devant
    double t = 700;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    for (int i = 0; i < 120; i++) image(i % 8 == 0);
    // choc : 80 km/h plus vite que la voiture de devant, dans sa file, sans freiner (même scène que trafic-choc)
    const Course* c = activeCourse();
    const int avant = c ? c->accidents + c->bumps + c->scrapes : 0;
    trafficBenchPlace(14, 0, 80, true);
    int n = 0;
    for (; n < 180; n++) { image(true); if (c && c->accidents + c->bumps + c->scrapes > avant) break; }
    for (int k = 0; k < 3; k++) image(true);   // étincelles encore groupées au point de contact (0,05 s)
    g.render(t + 0.001);
    r.capturePng(out + "/choc-etincelles.png");
    WFS_LOG("capture choc-etincelles : choc après %.2f s (accidents %d, chocs %d, frottements %d), %d étincelles, %.0f km/h",
            n / 60.0, c ? c->accidents : 0, c ? c->bumps : 0, c ? c->scrapes : 0, effetsEtincelles(), g.player().kmh());
    // fumée : voiture percutée en glissade (capot qui fume) et pneus de la voiture en tête-à-queue
    for (int k = 0; k < 60; k++) image(k % 4 == 0);
    g.render(t + 0.001);
    r.capturePng(out + "/accident-fumee.png");
    WFS_LOG("capture accident-fumee : %d bouffées de fumée, %d étincelles", effetsFumee(), effetsEtincelles());
    g.shutdown();
    // nitro : appui maintenu sur le bouton nitro (au-dessus de l'accélérateur) ; le pilote automatique est coupé pendant
    // l'appui (il force nitro = false, comme autopilot() du jeu web) : ligne droite, 0,75 s
    g.init(r, "golf", "ville"); g.setAutopilot(true); g.camMode = 0;
    for (int i = 0; i < 90; i++) image(i % 8 == 0);
    const float W = (float)r.width(), H = (float)r.height();
    g.setAutopilot(false);
    g.onTouch({7, W * 0.90f, H * 0.32f, true});
    for (int i = 0; i < 45; i++) image(i % 8 == 0);
    g.render(t + 0.001);
    r.capturePng(out + "/nitro-lignes.png");
    WFS_LOG("capture nitro-lignes : nitro %s, réserve %.2f, %.0f km/h", g.player().nitroOn ? "active" : "inactive", g.player().nitro, g.player().kmh());
    g.onTouch({7, W * 0.90f, H * 0.32f, false});
    g.shutdown();
    effetsBanc(false);
}

} // namespace wfs
