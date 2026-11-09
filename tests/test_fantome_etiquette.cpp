// Fantôme du record (WFS-318) : réglage Affiché par défaut, temps de l'étiquette « 1:12,4 », fondu de l'étiquette.
// usage : test_fantome_etiquette
#include <cmath>
#include <cstdio>
#include "../src/game/fantome.h"
using namespace wfs::fantome;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    VERIFIE(affiche(), "RG-01 : Affiché par défaut");
    VERIFIE(tempsRecord(72.4) == "1:12,4", "72,4 s → %s", tempsRecord(72.4).c_str());
    VERIFIE(tempsRecord(5.04) == "0:05,0" && tempsRecord(59.96) == "1:00,0" && tempsRecord(605.5) == "10:05,5", "arrondis : %s %s %s",
            tempsRecord(5.04).c_str(), tempsRecord(59.96).c_str(), tempsRecord(605.5).c_str());
    VERIFIE(tempsRecord(-3) == "0:00,0", "temps négatif → 0:00,0");
    VERIFIE(opaciteEtiquette(0) == 1.f && opaciteEtiquette(40) == 1.f && opaciteEtiquette(80) == 1.f, "pleine de 0 à 80 m");
    VERIFIE(std::fabs(opaciteEtiquette(140) - 0.5f) < 1e-4f && opaciteEtiquette(200) == 0.f && opaciteEtiquette(250) == 0.f, "fondu de 80 à 200 m");
    VERIFIE(std::fabs(opaciteEtiquette(-5) - 0.5f) < 1e-4f && opaciteEtiquette(-10) == 0.f && opaciteEtiquette(-30) == 0.f, "derrière : fondu de 0 à −10 m");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("fantôme du record (étiquette, réglage) : OK\n");
    return 0;
}
