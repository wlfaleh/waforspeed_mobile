// WFS-548 (interface R19) : constantes communes de src/ui/verre.h : dégradé de la marque (RG-04, mêmes couleurs que la barre de
// chargement d'avant : --accent-2 #00E5FF → --accent #FF6A00), couleurs d'état (RG-05) distinctes et lisibles sur le verre sombre,
// sur-titres gris (RG-03 : jamais bleus ni cyan).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../src/ui/verre.h"
using namespace wfs;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("verre : %-84s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }
static bool proche(const vec4& a, const vec4& b) { return std::fabs(a.x - b.x) < 1e-5f && std::fabs(a.y - b.y) < 1e-5f && std::fabs(a.z - b.z) < 1e-5f && std::fabs(a.w - b.w) < 1e-5f; }
// luminance relative WCAG 2.1 d'un code sRGB
static double lum(unsigned h) {
    auto c = [&](int sh) { const double v = ((h >> sh) & 255) / 255.0; return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * c(16) + 0.7152 * c(8) + 0.0722 * c(0);
}
static double contraste(unsigned a, unsigned b) { const double x = lum(a), y = lum(b); return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05); }

int main() {
    using namespace verre;
    cas("dégradé de la marque : cyan #00E5FF au début, orange #FF6A00 à la fin", proche(degradeMarque(0), hexa(0x00E5FF)) && proche(degradeMarque(1), hexa(0xFF6A00)));
    cas("dégradé borné hors de [0, 1], opacité transmise", proche(degradeMarque(-1, 0.5f), hexa(0x00E5FF, 0.5f)) && proche(degradeMarque(2), hexa(0xFF6A00)));
    {
        const vec4 m = degradeMarque(0.5f);
        cas("milieu du dégradé : moyenne des deux codes (comme la barre de chargement)", std::fabs(m.x - 0.5f) < 1e-3f && std::fabs(m.y - (229 + 106) / 510.f) < 1e-3f);
    }
    cas("couleurs d'état distinctes : toi (orange), perte (rouge), prêt (vert)", TOI != PERTE && PERTE != PRET && TOI != PRET);
    cas("toi : orange (rouge dominant, vert moyen, bleu faible)", ((TOI >> 16) & 255) > 200 && ((TOI >> 8) & 255) > 90 && (TOI & 255) < 100);
    cas("perte : rouge (rouge dominant)", ((PERTE >> 16) & 255) > 200 && ((PERTE >> 8) & 255) < 120);
    cas("prêt : vert (vert dominant)", ((PRET >> 8) & 255) > ((PRET >> 16) & 255) && ((PRET >> 8) & 255) > (PRET & 255));
    {
        const int r = (SUR_TITRE >> 16) & 255, g = (SUR_TITRE >> 8) & 255, b = SUR_TITRE & 255;
        cas("sur-titres gris : composantes à 20 près, jamais cyan", std::abs(r - g) <= 20 && std::abs(g - b) <= 20 && std::abs(r - b) <= 20);
    }
    // contraste sur le verre le plus sombre (#111218) : textes d'état et sur-titres lisibles (WCAG AA ≥ 4,5:1)
    bool lisible = true;
    for (unsigned c : {TOI, PERTE, PRET, SUR_TITRE, INTERTITRE}) lisible = lisible && contraste(c, 0x111218) >= 4.5;
    cas("textes d'état, sur-titres et intertitres lisibles sur le verre sombre (≥ 4,5:1)", lisible);
    std::printf("verre : %s\n", echecs ? "ÉCHEC" : "tout est bon");
    return echecs ? 1 : 0;
}
