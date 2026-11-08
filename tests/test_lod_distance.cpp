// Effacement au loin par fondu tramé (WFS-265, RG-01) : part visible, codes de fonduLod, combinaison avec WFS-257.
// usage : test_lod_distance
#include <cmath>
#include <cstdio>
#include "../src/render/lod_distance.h"
using namespace wfs::lod_distance;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static bool proche(float a, float b) { return std::fabs(a - b) < 1e-4f; }

int main() {
    // part visible : 1 jusqu'à dMax − 15 m, 0 au-delà de dMax, linéaire entre les deux (fondu de 15 m, RG-01 : 10 à 20 m)
    VERIFIE(FONDU_M >= 10.f && FONDU_M <= 20.f, "fondu de 10 à 20 m (%.0f)", FONDU_M);
    VERIFIE(partVisible(50.f, 90.f) == 1.f && partVisible(75.f, 90.f) == 1.f, "avant le fondu : entière");
    VERIFIE(proche(partVisible(82.5f, 90.f), 0.5f), "milieu du fondu : moitié (%.3f)", partVisible(82.5f, 90.f));
    VERIFIE(partVisible(90.f, 90.f) == 0.f && partVisible(200.f, 90.f) == 0.f, "au-delà : effacée");
    bool monotone = true; float avant = 2.f;
    for (float d = 0; d < 200; d += 0.25f) { const float p = partVisible(d, 90.f); monotone = monotone && p <= avant; avant = p; }
    VERIFIE(monotone, "la part visible ne remonte jamais quand on s'éloigne (aucun objet qui saute)");
    VERIFIE(partVisible(NAN, 90.f) == 1.f && partVisible(1e6f, 1e9f) == 1.f, "distance invalide ou distance maximale infinie : entière");
    // distances des personnes : Basse plus près que les autres niveaux, et jamais au-delà de 150 m (RG-01)
    VERIFIE(personnesMax(true) < personnesMax(false) && personnesMax(false) <= 150.f, "personnes : Basse %.0f m, sinon %.0f m", personnesMax(true), personnesMax(false));
    // codes de fonduLod : −(1 + t) garde la part t ; −(3 + t) garde 1 − t ; 0 rien
    VERIFIE(partDuCode(0.f) == 1.f && proche(partDuCode(-1.25f), 0.25f) && proche(partDuCode(-3.25f), 0.75f), "lecture des codes");
    VERIFIE(combinerFondu(0.f, 1.f) == 0.f && combinerFondu(-3.4f, 1.f) == -3.4f, "entière : code inchangé");
    VERIFIE(proche(partDuCode(combinerFondu(0.f, 0.3f)), 0.3f), "fondu de distance seul");
    VERIFIE(proche(partDuCode(combinerFondu(-1.2f, 0.6f)), 0.2f) && proche(partDuCode(combinerFondu(-1.8f, 0.6f)), 0.6f), "la plus petite part l'emporte (apparition WFS-257)");
    VERIFIE(combinerFondu(0.f, 0.f) < -1.f && combinerFondu(0.f, 0.f) > -1.01f, "part nulle : code valide (≈ rien de visible)");
    if (echecs) { std::printf("test_lod_distance : %d échec(s)\n", echecs); return 1; }
    std::printf("effacement au loin (WFS-265 RG-01) : fondu de 15 m, codes de fonduLod, combinaison avec WFS-257 : OK\n");
    return 0;
}
