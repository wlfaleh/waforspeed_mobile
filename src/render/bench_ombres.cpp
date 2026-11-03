// WaForSpeed — scènes du banc d'essai pour les ombres du soleil (WFS-07)
#include "bench_ombres.h"
#include "../core/log.h"
#include "../game/game.h"
#include "../game/track.h"
#include "renderer.h"

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
    // diagnostic WFS-42 (temporaire) : vue de vitrine de la Golf, en masquant une à une ses pièces transparentes,
    // pour trouver la pièce qui porte la trame de points de la lunette arrière
    const int savedCam = g.camMode; const float savedOrbit = g.orbit;
    g.camMode = 2; g.orbit = 0.7f;
    for (int part : {-1, 5, 8, 13, 17, 12, 10}) {
        r.debugHidePart = part;
        g.render(310); g.render(310.02);
        r.capturePng(out + "/diag42-golf-sans-" + (part < 0 ? std::string("rien") : std::to_string(part)) + ".png");
    }
    r.debugHidePart = -1; g.camMode = savedCam; g.orbit = savedOrbit;
}

} // namespace wfs
