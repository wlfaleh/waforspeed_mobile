// WaForSpeed — test WFS-245 : textes du HUD lisibles sur téléphone (RG-01, RG-02, RG-04 ; arbitrage du PO du 2026-11-07 17:30 UTC).
// Vérifie par le calcul, à 1080 px de haut (k = 1,5) et 420 dpi (16,5 px/mm) : capitales des étiquettes ≥ 20 px, chiffres d'ARRIVÉE
// ≥ 30 px et plus grands que leur étiquette, surface des panneaux par vue ≤ avant, contrastes WCAG 2.1 ≥ 4,5 : 1 au pire.
#include <cmath>
#include <cstdio>
#include "../src/ui/hud.h"

using namespace wfs;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// WCAG 2.1 : luminance relative d'une couleur sRGB (0..1 par canal), contraste (L1 + 0,05) / (L2 + 0,05)
static double lin(double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
struct Rgb { double r, g, b; };
static Rgb rgb(unsigned h) { return {((h >> 16) & 255) / 255.0, ((h >> 8) & 255) / 255.0, (h & 255) / 255.0}; }
static double luminance(Rgb c) { return 0.2126 * lin(c.r) + 0.7152 * lin(c.g) + 0.0722 * lin(c.b); }
static double contraste(Rgb a, Rgb b) { const double x = luminance(a), y = luminance(b); return (std::fmax(x, y) + 0.05) / (std::fmin(x, y) + 0.05); }
// fond semi-transparent posé sur une couleur, mélangé en sRGB (comme l'interface : image d'échange UNORM)
static Rgb sur(Rgb f, double a, Rgb d) { return {f.r * a + d.r * (1 - a), f.g * a + d.g * (1 - a), f.b * a + d.b * (1 - a)}; }

int main() {
    const double k = 1080.0 / 720.0, pxParMm = 420.0 / 25.4;
    const double etiquette = hud::CAPITALES * std::round(hud::ETIQUETTE_PX * k), arrivee = hud::CAPITALES * std::round(hud::ARRIVEE_PX * k);
    std::printf("capitales des étiquettes %.1f px (%.2f mm), chiffres d'ARRIVÉE %.1f px (%.2f mm), score %.1f px, vitesse cockpit %.1f px\n",
                etiquette, etiquette / pxParMm, arrivee, arrivee / pxParMm, hud::CAPITALES * hud::SCORE_PX * k, hud::CAPITALES * hud::VITESSE_COCKPIT_PX * k);
    VERIFIE(etiquette >= 20 && etiquette / pxParMm >= 1.2, "RG-01 : capitales des étiquettes %.1f px (≥ 20 px, 1,2 mm)", etiquette);
    VERIFIE(arrivee >= 30 && arrivee / pxParMm >= 1.8, "RG-01 : chiffres d'ARRIVÉE %.1f px (≥ 30 px, 1,8 mm)", arrivee);
    VERIFIE(hud::ARRIVEE_PX > hud::ETIQUETTE_PX, "RG-02 : distance plus grande que son étiquette");
    VERIFIE(hud::SCORE_PX > hud::ETIQUETTE_PX && hud::VITESSE_COCKPIT_PX > hud::ETIQUETTE_PX, "chiffres plus grands que les étiquettes");
    VERIFIE(hud::ETIQUETTE_PX < hud::CHRONO_PX && hud::ARRIVEE_PX < hud::CHRONO_PX, "le chrono reste le plus grand texte du panneau");
    // RG-04 : panneau du temps : 3 lignes (chrono, ARRIVÉE, barre) dans la hauteur, surface ≤ avant (le score ne change pas)
    const double haut = 10 + hud::CAPITALES * hud::CHRONO_PX + 9 + hud::CAPITALES * hud::ARRIVEE_PX + 7 + 6;
    VERIFIE(haut + 4 <= hud::TEMPS_H, "RG-04 : contenu du panneau du temps %.1f px de référence pour %.0f", haut, hud::TEMPS_H);
    const double avant = hud::AVANT_TEMPS_L * hud::AVANT_TEMPS_H, apres = hud::TEMPS_L * hud::TEMPS_H;
    std::printf("panneau du temps : %.0f px² de référence avant, %.0f après\n", avant, apres);
    VERIFIE(apres <= avant && hud::TEMPS_H <= 86, "RG-04 : panneau du temps %.0f px² (avant %.0f), hauteur %.0f (≤ 86)", apres, avant, hud::TEMPS_H);
    // contrastes au pire : panneau devant du blanc ; compteur devant du blanc (voiture blanche) et devant un béton clair
    const Rgb blanc{1, 1, 1}, beton = rgb(0xC8C8C8);
    const double cPanneau = contraste(rgb(hud::ETIQUETTE), sur(rgb(hud::FOND), hud::FOND_ALPHA, blanc));
    const double cArc = contraste(rgb(hud::ETIQUETTE_ARC), sur(rgb(hud::DISQUE), hud::DISQUE_ALPHA, blanc));
    const double cArcBeton = contraste(rgb(hud::ETIQUETTE_ARC), sur(rgb(hud::DISQUE), hud::DISQUE_ALPHA, beton));
    const double cAvant = contraste(rgb(0x8A90A6), sur(rgb(0x14161F), 0.72, blanc));
    std::printf("contrastes : étiquettes %.2f : 1 (avant %.2f : 1), compteur %.2f : 1 sur du blanc, %.2f : 1 sur un béton clair\n", cPanneau, cAvant, cArc, cArcBeton);
    VERIFIE(cPanneau >= 4.5, "contraste des étiquettes %.2f : 1 (≥ 4,5)", cPanneau);
    VERIFIE(cArc >= 4.5 && cArcBeton >= 4.5, "contraste du compteur %.2f / %.2f : 1 (≥ 4,5)", cArc, cArcBeton);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("HUD lisible : OK\n");
    return 0;
}
