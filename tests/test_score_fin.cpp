// WaForSpeed — WFS-537 : calculs purs de l'écran de fin du score (compte « +N ★ ») et du compteur d'étoiles du menu.
#include <cstdio>
#include <limits>
#include <string>
#include "../src/game/score_fin_calc.h"

using namespace wfs::score_fin;

static int g_echecs = 0;
static void verifier(bool ok, const char* quoi) {
    std::printf("%s : %s\n", ok ? "OK " : "ÉCHEC", quoi);
    if (!ok) g_echecs++;
}

int main() {
    // RG-02 : de 0 à N en moins de 1 s, toujours croissant, jamais au-delà de N
    verifier(DUREE_COMPTE < 1.0, "RG-02 : le compte dure moins de 1 s");
    verifier(etoilesAffichees(25, 0) == 0, "RG-02 : 0 au début");
    verifier(etoilesAffichees(25, -1) == 0, "temps négatif : 0");
    verifier(etoilesAffichees(25, std::numeric_limits<double>::quiet_NaN()) == 0, "temps NaN : 0");
    verifier(etoilesAffichees(25, DUREE_COMPTE) == 25, "RG-02 : N à la fin du compte");
    verifier(etoilesAffichees(25, 0.999) == 25, "RG-02 : N avant 1 s");
    verifier(etoilesAffichees(25, 30) == 25, "N après la fin (pas de dépassement)");
    verifier(etoilesAffichees(0, 0.5) == 0 && etoilesAffichees(-3, 0.5) == 0, "0 ou négatif : 0");
    verifier(etoilesAffichees(40, 0.4) > 0 && etoilesAffichees(40, 0.4) < 40, "à mi-compte : entre 0 et N");
    bool croissant = true; int avant = 0;
    for (int i = 0; i <= 100; i++) { const int v = etoilesAffichees(40, i * 0.01); if (v < avant || v > 40) croissant = false; avant = v; }
    verifier(croissant, "compte croissant, borné à N");
    // RG-08 : grands nombres lisibles dans les deux langues
    verifier(nombreEtoiles(0, false) == "0" && nombreEtoiles(0, true) == "0", "RG-08 : 0");
    verifier(nombreEtoiles(250, false) == "250" && nombreEtoiles(250, true) == "250", "RG-08 : 250");
    verifier(nombreEtoiles(12450, false) == "12\xE2\x80\xAF" "450", "RG-08 : « 12 450 » en français (espace fine insécable)");
    verifier(nombreEtoiles(12450, true) == "12,450", "RG-08 : « 12,450 » en anglais");
    verifier(nombreEtoiles(1000000000000LL, true) == "1,000,000,000,000", "RG-08 : solde maximal (10^12) en anglais");
    verifier(nombreEtoiles(-1234, true) == "-1,234", "négatif (jamais affiché, sans plantage)");
    verifier(nombreEtoiles(std::numeric_limits<long long>::min(), true).size() > 20, "plus petit entier : sans débordement");
    // accord : « 1 étoile », « 0 étoile », « 2 étoiles » ; « 1 star », « 0 stars »
    verifier(singulier(1, false) && singulier(0, false) && !singulier(2, false) && !singulier(12450, false), "singulier en français (0 et 1)");
    verifier(singulier(1, true) && !singulier(0, true) && !singulier(2, true), "singulier en anglais (1 seulement)");
    std::printf("%s (%d échec(s))\n", g_echecs ? "ROUGE" : "VERT", g_echecs);
    return g_echecs ? 1 : 0;
}
