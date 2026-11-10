// WaForSpeed — WFS-381 : brume au loin (perspective atmosphérique), calcul de référence src/render/atmosphere_brume.h.
// Vérifie RG-02 (contraste restant à 200 m, 1 km, 5 km ; visibilité), RG-04 (un réglage par carte, du plus sec au plus
// brumeux), la hauteur (moins de brume vers les sommets, intégrale exacte) et la continuité de la formule.
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/render/atmosphere_brume.h"

using namespace wfs;
static int g_echecs = 0;
static void verifier(bool ok, const char* quoi, double v) {
    std::printf("%s %s (%.4f)\n", ok ? "OK  " : "ÉCHEC", quoi, v);
    if (!ok) g_echecs++;
}

int main() {
    // RG-02 (2e décision du PO, 2026-11-10 07:30:48, option C : « jour de brume de chaleur ») : bornes de perte de contraste
    // à l'horizontale, œil et objet à 2 m (chaque borne = V ± 10 %) ; premier plan (< 100 m) : 5 % au plus sur toutes les cartes
    struct Borne { double min, max; };
    struct Cas { const char* carte; double V; Borne b02, b1, b2; double min5, max5; };
    const Cas CAS[] = {
        {"plage", 8000, {0.085, 0.105}, {0.35, 0.42}, {0.58, 0.67}, 0.89, 1.00},
        {"ville", 10000, {0.065, 0.085}, {0.29, 0.36}, {0.50, 0.59}, 0.83, 0.89},
        {"circuit", 11000, {0.060, 0.080}, {0.27, 0.33}, {0.47, 0.55}, 0.80, 0.87},
        {"campagne", 12000, {0.055, 0.075}, {0.25, 0.31}, {0.44, 0.52}, 0.77, 0.84},
        {"neige", 10000, {0.065, 0.085}, {0.29, 0.36}, {0.50, 0.59}, 0.83, 0.89},   // carte non citée : V 10 km
    };
    for (const Cas& k : CAS) {
        const char* c = k.carte;
        const brume::Reglage b = brume::reglageCarte(c);
        std::printf("-- carte %s : β0 = %.3g m⁻¹, H = %.0f m, visibilité %.1f km\n", c, b.beta0, b.hauteur, brume::visibilite(b) / 1000);
        auto perte = [&](double d) { return 1 - brume::contraste(b, d, 2, 2); };
        verifier(std::fabs(brume::visibilite(b) / k.V - 1) <= 0.02, "visibilité de la carte (RG-02)", brume::visibilite(b));
        verifier(perte(100) <= 0.05, "premier plan : au plus 5 % perdu à 100 m", perte(100));
        verifier(perte(200) >= k.b02.min && perte(200) <= k.b02.max, "perte à 200 m dans les bornes", perte(200));
        verifier(perte(1000) >= k.b1.min && perte(1000) <= k.b1.max, "perte à 1 km dans les bornes", perte(1000));
        verifier(perte(2000) >= k.b2.min && perte(2000) <= k.b2.max, "perte à 2 km dans les bornes", perte(2000));
        verifier(perte(5000) >= k.min5 && perte(5000) <= k.max5, "perte à 5 km dans les bornes", perte(5000));
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
