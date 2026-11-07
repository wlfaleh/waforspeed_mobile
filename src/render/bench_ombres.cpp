// WaForSpeed — scènes du banc d'essai pour les ombres du soleil (WFS-07)
#include "bench_ombres.h"
#include "../core/log.h"
#include "../game/game.h"
#include "../game/track.h"
#include "renderer.h"
#include <cstdio>

namespace wfs {

void benchShadowScenes(Game& g, Renderer& r, const std::string& out) {
    g.setAutopilot(true);
    g.camMode = 0;
    for (int i = 0; i < 60; i++) { g.update(1.0 / 60); g.render(300 + i / 60.0); }
    // 1re capture, puis on avance d'1 m exactement (au pas de conduite) pour la 2e
    g.render(301); g.render(301.02);
    r.capturePng(out + "/ombres-a.png");
    const double d0 = g.player().dist * M;
    int steps = 0;
    while (g.player().dist * M < d0 + 1.0 && steps < 600) { g.update(1.0 / 120); steps++; }
    g.render(301.5); g.render(301.52);
    r.capturePng(out + "/ombres-b.png");
    WFS_LOG("capture ombres-a / ombres-b : %.2f m d'écart, %.0f km/h, carte d'ombre %u", g.player().dist * M - d0, g.player().kmh(), r.shadowMapSize());
    // sans ombres du soleil (comme la nuit) : seule l'ombre de contact reste sous la voiture
    r.forceNoShadows = true;
    g.render(302); g.render(302.02);
    r.capturePng(out + "/ombres-coupees.png");
    r.forceNoShadows = false;
    // coût : quelques images de plus pour que le journal donne la moyenne de la passe d'ombre
    for (int i = 0; i < 130; i++) g.render(303 + i / 60.0);
}

// WFS-227 : banc « ombres » (WFS_BANC=ombres) : captures de WFS-07 puis travelling (RG-01 : aucun scintillement) : 60 images
// à 60 img/s, voiture lancée en ligne droite, une sur dix gardée (realisme-ombres-travelling-00 à -05), vue Proche
void benchOmbres(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) return;
    benchShadowScenes(g, r, out);
    g.setAutopilot(true); g.camMode = 0;
    double t = 400;
    for (int i = 0; i < 240; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
    for (int i = 0; i < 60; i++) {
        g.update(1.0 / 60); t += 1.0 / 60; g.render(t);
        if (i % 10 == 0) { char nom[96]; std::snprintf(nom, sizeof nom, "/realisme-ombres-travelling-%02d.png", i / 10); r.capturePng(out + nom); }
    }
    WFS_LOG("capture realisme-ombres-travelling : %.0f km/h, carte d'ombre %u (WFS-227)", g.player().kmh(), r.shadowMapSize());
    g.shutdown();
}

} // namespace wfs
