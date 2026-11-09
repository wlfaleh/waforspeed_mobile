// Appuis sur « Réglages » et « Fermer » (WFS-290, ANO-159) : cible de 48 dp avec marges de 4 dp, anti-rebond de 0,3 s.
// usage : test_reglages_appui
#include <cstdio>
#include "../src/ui/ecrans/reglages_appui.h"
using namespace wfs::ecrans::appui;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // « Réglages » sur téléphone (2,62 px/dp) : dessin de 215 × 66 px (82 × 25 dp) en (2000, 37)
    const float dp = 2.625f, x = 2000, y = 37, w = 215, h = 66, cy = y + h / 2;
    VERIFIE(dansCible(x, y, w, h, dp, x + w / 2, cy), "centre du dessin");
    VERIFIE(dansCible(x, y, w, h, dp, x, y) && dansCible(x, y, w, h, dp, x + w, y + h), "coins du dessin");
    const float demi = 24 * dp;   // 48 dp de haut, centrés sur le dessin
    VERIFIE(dansCible(x, y, w, h, dp, x + w / 2, cy - demi + 0.5f) && dansCible(x, y, w, h, dp, x + w / 2, cy + demi - 0.5f), "cible de 48 dp de haut");
    VERIFIE(!dansCible(x, y, w, h, dp, x + w / 2, cy - demi - 1) && !dansCible(x, y, w, h, dp, x + w / 2, cy + demi + 1), "au-delà de 48 dp : non");
    VERIFIE(dansCible(x, y, w, h, dp, x - 4 * dp + 0.5f, cy) && dansCible(x, y, w, h, dp, x + w + 4 * dp - 0.5f, cy), "marges de 4 dp");
    VERIFIE(!dansCible(x, y, w, h, dp, x - 4 * dp - 1, cy) && !dansCible(x, y, w, h, dp, x + w + 4 * dp + 1, cy), "au-delà des 4 dp : non");
    VERIFIE(dansCible(x, y, w, 200, dp, x + 1, y + 199) && !dansCible(x, y, w, 200, dp, x + 1, y + 201), "bouton déjà plus haut que 48 dp : sa hauteur");
    VERIFIE(!dansCible(x, y, 0, h, dp, x, y), "bouton pas encore dessiné : pas de cible");
    // anti-rebond
    VERIFIE(!basculementPermis(10.29, 10.0), "0,29 s après : refusé");
    VERIFIE(basculementPermis(10.30, 10.0), "0,30 s après : accepté");
    VERIFIE(basculementPermis(12.0, 10.0), "2 s après : accepté");
    VERIFIE(basculementPermis(0.0, -1e9), "premier appui : accepté");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("appuis des réglages : OK\n");
    return 0;
}
