// WaForSpeed — WFS-293 (RG-10) : l'indicateur à 10-20 img/s (écart à une référence à 120 Hz, src/game/fluidite_reference.h) voit les
// sauts : virage de 40 m de rayon à 20 m/s (voiture vue par une caméra qui la suit, ici position dans le repère de la caméra qui
// tourne lentement), référence à 120 Hz, images à 15 img/s irrégulières (± 30 %) ; sans saut : écart < 0,5 mm ; un saut de 5 mm
// injecté sur une image : mesuré ≥ 4 mm ; image hors de la référence : ignorée.
#include <cmath>
#include <cstdio>
#include "../src/game/fluidite_reference.h"

using namespace wfs;
static int g_controles = 0, g_echecs = 0;
#define VERIFIE(c, ...) do { g_controles++; if (!(c)) { g_echecs++; std::printf("[ÉCHEC] " __VA_ARGS__); std::printf("\n"); } else { std::printf("[OK] " __VA_ARGS__); std::printf("\n"); } } while (0)

// position de la voiture dans le repère de la caméra : la caméra suit avec retard, la voiture balaie un peu le cadre
static vec3 vue(double t) { return {(float)(0.6 * std::sin(0.7 * t)), (float)(-1.2 + 0.05 * std::sin(1.3 * t)), (float)(-6.0 - 0.3 * std::cos(0.5 * t))}; }

int main() {
    std::vector<fluidite::Pose> ref;
    for (int k = 0; k <= 120 * 10; k++) ref.push_back({k / 120.0, vue(k / 120.0)});
    std::vector<fluidite::Pose> img;
    unsigned graine = 12345; double t = 0.5;
    while (t < 9.5) { graine = graine * 1103515245u + 12345u; t += (1.0 / 15) * (0.7 + 0.6 * ((graine >> 8) & 0xffff) / 65535.0); img.push_back({t, vue(t)}); }
    const double sans = fluidite::ecartMaxMm(img, ref);
    VERIFIE(sans < 0.5, "sans saut : écart %.3f mm (< 0,5)", sans);
    std::vector<fluidite::Pose> saut = img;
    saut[saut.size() / 2].p = saut[saut.size() / 2].p + vec3{0.005f, 0, 0};
    const double avec = fluidite::ecartMaxMm(saut, ref);
    VERIFIE(avec >= 4.0, "saut de 5 mm injecté : mesuré %.2f mm (≥ 4)", avec);
    std::vector<fluidite::Pose> dehors = img; dehors.push_back({20.0, vec3{1, 1, 1}});
    int n = 0; const double h = fluidite::ecartMaxMm(dehors, ref, &n);
    VERIFIE(h < 0.5 && n == (int)img.size(), "image hors de la référence ignorée (%d images comptées, %.3f mm)", n, h);
    std::printf("test_fluidite_reference : %d contrôles, %d échec(s)\n", g_controles, g_echecs);
    return g_echecs ? 1 : 0;
}
