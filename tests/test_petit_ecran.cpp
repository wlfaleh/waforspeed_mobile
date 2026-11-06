// Téléphone couché (WFS-147, RG-02) : mesure de l'écran en dp, choix de la mise en page petit écran par écran.
// usage : test_petit_ecran
#include <cmath>
#include <cstdio>
#include "../src/ui/petit_ecran.h"
using namespace wfs::ui;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static bool proche(float a, float b) { return std::fabs(a - b) < 1e-3f; }

int main() {
    // banc PC (densité inconnue) : 1280x720 = bureau (mise en page d'avant, captures inchangées) ; 2400x1080 = 420 dpi
    VERIFIE(proche(pixelsParDp(1280, 0.f), 1.f), "1280x720 : 1 px par dp");
    VERIFIE(!petitEcran(1280, 720, 0.f), "1280x720 : mise en page de bureau");
    VERIFIE(proche(pixelsParDp(2400, 0.f), 2.625f), "2400x1080 : 2,625 px par dp (420 dpi)");
    VERIFIE(petitEcran(2400, 1080, 0.f), "2400x1080 à 420 dpi : petit écran (411 dp de haut)");
    VERIFIE(proche(unitePetitEcran(2400, 1080, 0.f), 2.625f), "2400x1080 : 1 dp de la mise en page = 1 dp réel");
    // RG-02 : texte de 14 dp et zone de 44 dp en pixels sur le téléphone de référence
    VERIFIE(proche(TEXTE_MIN_DP * unitePetitEcran(2400, 1080, 0.f), 36.75f), "texte 14 dp = 36,75 px");
    VERIFIE(proche(TOUCHER_MIN_DP * unitePetitEcran(2400, 1080, 0.f), 115.5f), "toucher 44 dp = 115,5 px");
    // Android : la densité lue l'emporte ; tablette (> 520 dp de haut) : mise en page de bureau
    VERIFIE(petitEcran(2400, 1080, 3.f), "480 dpi : petit écran (360 dp)");
    VERIFIE(proche(unitePetitEcran(2400, 1080, 3.f), 2.7f), "360 dp de haut : mise en page réduite à 400 dp (2,7 px)");
    VERIFIE(!petitEcran(2560, 1600, 2.f), "tablette 800 dp : bureau");
    VERIFIE(proche(unitePetitEcran(1920, 1080, 2.625f), 2.4f), "16:9 étroit : 800 dp de large au moins (2,4 px)");
    if (echecs) { std::printf("test_petit_ecran : %d échec(s)\n", echecs); return 1; }
    std::printf("test_petit_ecran : OK\n");
    return 0;
}
