// WaForSpeed — WFS-381 : brume au loin (perspective atmosphérique), calcul de référence src/render/atmosphere_brume.h.
// Vérifie RG-02 (contraste restant à 200 m, 1 km, 5 km ; visibilité), RG-04 (un réglage par carte, du plus sec au plus
// brumeux), la hauteur (moins de brume vers les sommets, intégrale exacte) et la continuité de la formule.
#include <cmath>
#include <cstdio>
#include "../src/render/atmosphere_brume.h"

using namespace wfs;
static int g_echecs = 0;
static void verifier(bool ok, const char* quoi, double v) {
    std::printf("%s %s (%.4f)\n", ok ? "OK  " : "ÉCHEC", quoi, v);
    if (!ok) g_echecs++;
}

int main() {
    // RG-02 : Côte d'Azur par beau temps, œil et objet au niveau de la route (2 m)
    for (const char* c : {"plage", "ville", "campagne", "circuit"}) {
        const brume::Reglage b = brume::reglageCarte(c);
        std::printf("-- carte %s : β0 = %.3g m⁻¹, H = %.0f m, visibilité %.1f km\n", c, b.beta0, b.hauteur, brume::visibilite(b) / 1000);
        verifier(1 - brume::contraste(b, 200, 2, 2) <= 0.03, "moins de 3 % de contraste perdu à 200 m", 1 - brume::contraste(b, 200, 2, 2));
        const double p1 = 1 - brume::contraste(b, 1000, 2, 2), p5 = 1 - brume::contraste(b, 5000, 2, 2);
        verifier(p1 >= 0.10 && p1 <= 0.20, "10 à 20 % de contraste perdu à 1 km", p1);
        verifier(p5 >= 0.40 && p5 <= 0.60, "40 à 60 % de contraste perdu à 5 km", p5);
        verifier(brume::visibilite(b) >= 15000 && brume::visibilite(b) <= 35000, "visibilité de 15 à 35 km", brume::visibilite(b));
        // hauteur : un sommet à 400 m au-dessus de la route reçoit moins de brume qu'un objet au même niveau
        verifier(brume::part(b, 3000, 2, 400) < brume::part(b, 3000, 2, 2), "sommet moins voilé que la plaine à 3 km", brume::part(b, 3000, 2, 400));
        // œil en hauteur (vue de 40 m) : moins de brume qu'au sol, jamais plus
        verifier(brume::part(b, 2000, 40, 40) < brume::part(b, 2000, 2, 2), "œil à 40 m : air moins chargé", brume::part(b, 2000, 40, 40));
        // symétrie de l'intégrale : même épaisseur dans les deux sens
        verifier(std::fabs(brume::epaisseur(b, 3000, 2, 500) - brume::epaisseur(b, 3000, 500, 2)) < 1e-9, "épaisseur symétrique (œil ↔ objet)", brume::epaisseur(b, 3000, 2, 500));
        // continuité autour de Δy = 0 (développement limité du shader)
        const double a = brume::epaisseur(b, 1000, 2, 2), e = brume::epaisseur(b, 1000, 2, 2.05);
        verifier(std::fabs(a - e) / a < 1e-3, "continuité de la formule à Δy ≈ 0", std::fabs(a - e) / a);
        verifier(brume::part(b, 0, 2, 2) == 0 && brume::part(b, 1e7, 2, 2) > 0.999, "0 à l'œil, 1 à l'infini", brume::part(b, 1e7, 2, 2));
    }
    // intégrale exacte : comparée à une somme de 20 000 tranches le long d'un segment montant de 2 m à 600 m
    {
        const brume::Reglage b = brume::reglageCarte("plage");
        const double d = 4000, y0 = 2, y1 = 600; double tau = 0; const int n = 20000;
        for (int i = 0; i < n; i++) { const double y = y0 + (y1 - y0) * (i + 0.5) / n; tau += b.beta0 * std::exp(-y / b.hauteur) * d / n; }
        verifier(std::fabs(tau - brume::epaisseur(b, d, y0, y1)) / tau < 1e-6, "intégrale exacte le long du segment", brume::epaisseur(b, d, y0, y1));
    }
    // RG-04 : un réglage par carte : Corniche (mer) plus brumeuse que la Ville, plus brumeuse que la Campagne (air sec)
    verifier(brume::reglageCarte("plage").beta0 > brume::reglageCarte("ville").beta0 && brume::reglageCarte("ville").beta0 > brume::reglageCarte("campagne").beta0,
             "Corniche > Ville > Campagne", brume::reglageCarte("plage").beta0);
    verifier(brume::reglageCarte(nullptr).beta0 > 0, "carte inconnue : réglage par défaut", brume::reglageCarte(nullptr).beta0);
    std::printf(g_echecs ? "test_brume : %d échec(s)\n" : "test_brume : tout est bon\n", g_echecs);
    return g_echecs ? 1 : 0;
}
