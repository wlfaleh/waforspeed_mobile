// WFS-258 (point 3 de RG-00b, ANO-138) : direction moins vive à haute vitesse, conduite du jeu (ancienne), aide désactivée,
// ligne droite de la Ville, volant tenu 0,5 s (Golf, Gallardo) :
//  - au-dessus de 130 km/h, un quart de volant demande au plus 40 % de l'accélération latérale du volant à fond (avant :
//    autant que le volant à fond, 2,1 g dès 150 km/h) ; le volant à fond garde toute l'adhérence du jeu ;
//  - sous 100 km/h, rien ne change (zone de WFS-316) ; de 100 à 130 km/h, aucun à-coup (accélération latérale qui ne saute pas
//    de plus de 0,15 g par palier de 2 km/h) ;
//  - l'interrupteur du banc (g_avantWfs258Vif) rend le comportement de main.
// usage : test_direction_vitesse
#include <cstdio>
#include "../src/game/trace_cartes.h"
#include "../src/game/virages_banc.h"
using namespace wfs;
using namespace wfs::banc_virages;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static Track t;
static double lateral(const char* id, double kmh, float volant) {   // accélération latérale la plus forte (g) en 0,5 s
    Player p = voiture(id, Aide::Desactivee, t, 50 / M, 0, kmh);
    double a = 0;
    for (int k = 0; k < 60; k++) { p.update(DT, tenir(p, kmh, volant), t, true); a = std::max(a, std::abs(p.latAcc)); }
    return a / 9.81;
}

int main() {
    const bool mondeDefaut = g_conduiteMonde;
    g_conduiteMonde = false;   // conduite du jeu depuis WFS-258 RG-00
    t.build(mapById("ville"), trace::versionCarte("ville"));
    std::printf("## accélération latérale (g), un quart de volant / volant à fond, 0,5 s\n");
    for (const char* id : {"golf", "gallardo"}) for (double kmh : {50.0, 90.0, 150.0, 190.0, 230.0}) {
        const double q = lateral(id, kmh, 0.25f), f = lateral(id, kmh, 1.f);
        g_avantWfs258Vif = true; const double q0 = lateral(id, kmh, 0.25f), f0 = lateral(id, kmh, 1.f); g_avantWfs258Vif = false;
        std::printf("- %s, %3.0f km/h : quart %.2f g, à fond %.2f g (main : %.2f g / %.2f g)\n", id, kmh, q, f, q0, f0);
        if (kmh < 100) VERIFIE(q == q0 && f == f0, "%s à %.0f km/h : la direction a changé sous 100 km/h", id, kmh);
        else {
            VERIFIE(q <= 0.4 * f, "%s à %.0f km/h : un quart de volant demande %.2f g pour %.2f g à fond", id, kmh, q, f);
            VERIFIE(f >= 0.95 * f0, "%s à %.0f km/h : le volant à fond a perdu de l'adhérence (%.2f g au lieu de %.2f g)", id, kmh, f, f0);
            VERIFIE(q0 > 0.6 * f0, "%s à %.0f km/h : g_avantWfs258Vif ne rejoue pas main", id, kmh);
        }
    }
    double pire = 0, prec = lateral("golf", 98, 0.25f);   // de 98 à 140 km/h : aucun à-coup
    for (double kmh = 100; kmh <= 140; kmh += 2) { const double a = lateral("golf", kmh, 0.25f); pire = std::max(pire, std::abs(a - prec)); prec = a; }
    std::printf("- golf, quart de volant de 98 à 140 km/h : plus grand saut %.3f g par 2 km/h\n", pire);
    VERIFIE(pire <= 0.15, "à-coup de la direction entre 100 et 130 km/h (%.3f g par 2 km/h)", pire);
    g_conduiteMonde = mondeDefaut;
    std::printf("direction moins vive à haute vitesse (WFS-258) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
