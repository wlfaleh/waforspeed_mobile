// Conduite libre (WFS-90) : demi-tour jusqu'à 180° depuis l'arrêt, marche arrière sans arrêt complet, conduite à
// contresens (cap retenu autour de ±π, dist qui diminue : checkpoints et chrono ne comptent que dans le bon sens),
// position et dist cohérentes ; conduite normale inchangée (cap borné à 0,5 rad comme le jeu web).
// usage : test_conduite_libre
#include <cmath>
#include <cstdio>
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/track.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    static Track t; t.build(mapById("ville"));
    const double dt = 1.0 / 120, PI_D = 3.14159265358979323846;
    // écart du cap au sens de la route le plus proche (0 ou ±π)
    auto ecart = [&](const Player& p) { const double a = std::abs(p.heading); return a > PI_D / 2 ? PI_D - a : a; };

    // 1) conduite normale à pleine vitesse, braquage à fond : cap toujours borné (≤ 0,5 rad), comme le jeu web
    {
        Player p; p.reset(carById("golf"));
        DriveInput in; in.gas = true;
        double maxH = 0;
        for (int i = 0; i < 120 * 10; i++) {   // ligne droite jusqu'à 20 m/s, puis braquage à fond
            in.steer = p.speed * M > 20 ? 1.f : 0.f;
            p.update(dt, in, t, true);
            if (p.speed * M > 12) maxH = std::max(maxH, std::abs(p.heading));
        }
        VERIFIE(maxH > 0.1, "le scénario doit braquer au-dessus de 12 m/s (cap %.3f)", maxH);
        VERIFIE(maxH <= 0.5 + 1e-9, "cap à pleine vitesse %.3f > 0,5", maxH);
        std::printf("conduite normale : cap au plus %.3f rad (borne 0,5) : OK\n", maxH);
    }

    // 2) demi-tour depuis l'arrêt : braquer à fond et avancer doucement (gaz coupé au-dessus de 4 m/s)
    Player p; p.reset(carById("golf"));
    int pas = 0;
    for (; pas < 120 * 30 && std::abs(p.heading) < 3.0; pas++) {
        DriveInput in; in.steer = 1; in.gas = p.speed * M < 4;
        p.update(dt, in, t, true);
    }
    VERIFIE(std::abs(p.heading) >= 3.0, "demi-tour non atteint : cap %.2f rad après %.1f s", p.heading, pas * dt);
    std::printf("demi-tour depuis l'arrêt : cap %.2f rad (%.0f°) en %.1f s, écart latéral %.2f : OK\n", p.heading, p.heading * 180 / PI_D, pas * dt, p.playerX);

    // 3) roule à contresens : plein gaz 6 s en revenant au milieu de la route (le demi-tour finit sur le bas-côté : le
    // rayon de braquage dépasse la demi-largeur) → vite, cap retenu autour de ±π, dist diminue
    const double d0 = p.dist, pos0 = p.position;
    double maxEcart = 0;
    for (int i = 0; i < 120 * 6; i++) {
        DriveInput in; in.gas = true;
        const double dev = p.heading - (p.heading > 0 ? PI_D : -PI_D);   // à contresens, dX/dt = −v sin(dev)
        in.steer = (float)std::max(-1.0, std::min(1.0, p.playerX * 2.5 - dev * 3));
        p.update(dt, in, t, true);
        if (p.speed * M > 14) maxEcart = std::max(maxEcart, ecart(p));   // au-dessus de la conduite libre (10 m/s)
    }
    VERIFIE(p.speed * M > 20, "vitesse à contresens %.1f m/s", p.speed * M);
    VERIFIE(std::abs(p.heading) > PI_D / 2 && maxEcart <= 0.5 + 1e-9, "cap à contresens : %.2f rad, écart max %.3f", p.heading, maxEcart);
    VERIFIE(p.dist < d0 - 50 / M, "dist doit diminuer à contresens (%.0f → %.0f)", d0, p.dist);
    VERIFIE(std::abs(t.relZ(p.position - t.wrap(pos0 + (p.dist - d0)))) < 1e-3, "position et dist incohérentes");
    std::printf("contresens : %.0f km/h, cap %.2f rad (écart à l'axe ≤ %.2f), %.0f m parcourus à rebours, position = dist : OK\n",
                p.kmh(), p.heading, maxEcart, (d0 - p.dist) * M);

    // 4) marche arrière sans arrêt complet : à 2 m/s, frein → la vitesse passe directement sous 0 et plafonne à REV_MAX
    {
        Player q; q.reset(carById("golf")); q.speed = 2 / M;
        DriveInput in; in.brake = true;
        q.update(dt, in, t, true);
        VERIFIE(q.reversing && !q.braking, "marche arrière enchaînée sous 3 m/s");
        for (int i = 0; i < 120 * 4; i++) q.update(dt, in, t, true);
        VERIFIE(std::abs(q.speed - REV_MAX) < 1e-6 && q.speed < 0, "vitesse arrière %.1f km/h", q.kmh());
        Player r2; r2.reset(carById("golf")); r2.speed = 20 / M;
        r2.update(dt, in, t, true);
        VERIFIE(r2.braking && !r2.reversing, "à 20 m/s le frein freine (pas de marche arrière)");
        std::printf("marche arrière : enchaînée sous 3 m/s, %.0f km/h au plus ; à 72 km/h le frein freine : OK\n", q.kmh());
    }

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("conduite libre : tout est OK\n");
    return 0;
}
