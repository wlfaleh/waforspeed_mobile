// Conduite libre (WFS-90) : demi-tour jusqu'à 180° depuis l'arrêt, marche arrière sans arrêt complet, conduite à
// contresens (cap retenu autour de ±π, dist qui diminue : checkpoints et chrono ne comptent que dans le bon sens),
// position et dist cohérentes ; conduite normale inchangée (cap borné à 0,5 rad comme le jeu web).
// usage : test_conduite_libre
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <cstdio>
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/track.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// WFS-188 (RG-02 à RG-04, RG-06, RG-08) : chaque voiture du catalogue, mesurée avec Player (pas de 1/60 s, ligne droite :
// écart latéral et cap remis à 0 à chaque pas) : 0-100 plein gaz sans nitro, vitesse max (90 s), 100 → 0 frein tenu
static void mesurerVoitures(Track& t) {
    struct Ligne { const char* id; double t100, vmax, arret; float a0; };
    std::vector<Ligne> L;
    const double dt = 1.0 / 60;
    for (const CarDef& c : CARS) {
        Player p; p.reset(c);
        DriveInput gaz; gaz.gas = true;
        double t100 = -1, temps = 0;
        for (int i = 0; i < 90 * 60; i++) {
            p.update(dt, gaz, t, true); p.playerX = 0; p.heading = 0; temps += dt;
            if (t100 < 0 && p.kmh() >= 100) t100 = temps;
        }
        const double vmax = p.kmh();
        // 100 → 0 : lancée à 100 km/h, frein tenu jusqu'à l'arrêt (la marche arrière enchaînée sous 3 m/s ne compte plus)
        Player f; f.reset(c); f.speed = 100 / 3.6 / M;
        DriveInput frein; frein.brake = true;
        const double d0 = f.dist;
        for (int i = 0; i < 30 * 60 && f.speed > 0; i++) { f.update(dt, frein, t, true); f.playerX = 0; f.heading = 0; }
        L.push_back({c.id, t100, vmax, (f.dist - d0) * M, f.perf.a0});
    }
    std::printf("WFS-188 : voiture | 0-100 jeu / fiche | Vmax jeu / fiche | 100-0\n");
    double court = 1e9, long_ = 0;
    for (size_t i = 0; i < L.size(); i++) {
        const CarDef& c = CARS[i]; const Ligne& l = L[i];
        std::printf("  %-9s | %5.2f s / %4.1f s (%+5.1f %%) | %5.1f / %3d km/h (%5.1f %%) | %4.1f m | a0 %.2f\n", l.id, l.t100, c.spec.s100,
                    (l.t100 / c.spec.s100 - 1) * 100, l.vmax, c.spec.vmax, l.vmax / c.spec.vmax * 100, l.arret, l.a0);
        VERIFIE(l.t100 > 0 && std::abs(l.t100 / c.spec.s100 - 1) <= 0.10, "%s : 0-100 %.2f s (fiche %.1f s)", l.id, l.t100, c.spec.s100);
        VERIFIE(l.vmax >= 0.98 * c.spec.vmax && l.vmax <= 1.01 * c.spec.vmax, "%s : Vmax %.1f km/h (fiche %d)", l.id, l.vmax, c.spec.vmax);
        VERIFIE(l.arret >= 33 && l.arret <= 40, "%s : 100-0 en %.1f m (33 à 40)", l.id, l.arret);
        court = std::min(court, l.arret); long_ = std::max(long_, l.arret);
    }
    // ordre RG-04 : SF90, 911, Gallardo parmi les 5 plus courtes ; X5 et RR Sport parmi les 3 plus longues ; écart ≥ 4 m
    std::vector<Ligne> tri = L;
    std::sort(tri.begin(), tri.end(), [](const Ligne& a, const Ligne& b) { return a.arret < b.arret; });
    auto rang = [&](const char* id) { for (size_t i = 0; i < tri.size(); i++) if (!std::strcmp(tri[i].id, id)) return (int)i; return -1; };
    VERIFIE(rang("sf90") < 5 && rang("p911") < 5 && rang("gallardo") < 5, "freinage : SF90 %d, 911 %d, Gallardo %d (attendu < 5)", rang("sf90"), rang("p911"), rang("gallardo"));
    VERIFIE(rang("x5") >= (int)tri.size() - 3 && rang("rrsport") >= (int)tri.size() - 3, "freinage : X5 %d, RR Sport %d (attendu parmi les 3 plus longues)", rang("x5"), rang("rrsport"));
    VERIFIE(long_ - court >= 4, "freinage : écart %.1f m (au moins 4)", long_ - court);
    const Ligne& compass = L[(size_t)(&carById("compass") - CARS)];
    VERIFIE(compass.t100 <= 10.5, "Compass : 0-100 %.2f s (10,5 s au plus)", compass.t100);
    // nitro : la vitesse avec nitro ne baisse pas de plus de 5 % (rapport 1,3 sur le plafond)
    for (const CarDef& c : CARS) {
        Player p; p.reset(c); p.nitro = 1e9;
        DriveInput n; n.gas = true; n.nitro = true;
        for (int i = 0; i < 90 * 60; i++) { p.update(dt, n, t, true); p.playerX = 0; p.heading = 0; p.nitro = 1; }
        const double ancien = c.spec.vmax / 260.0 * MAX_SPEED * 1.3 * M * 3.6;   // plafond nitro d'avant (top × 1,3)
        VERIFIE(p.kmh() >= 0.95 * ancien * 0.94, "%s : vitesse avec nitro %.0f km/h (avant ≈ %.0f)", c.id, p.kmh(), ancien * 0.94);
    }
    std::printf("WFS-188 : 20 voitures mesurées (100-0 de %.1f à %.1f m)\n", court, long_);
}

int main() {
    static Track t; t.build(mapById("ville"));
    mesurerVoitures(t);
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
