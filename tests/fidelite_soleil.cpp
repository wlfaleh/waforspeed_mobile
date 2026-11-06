// WFS-148 (RG-02, M-01) : soleil() (équations de la NOAA, src/render/bench_realisme.h) contre les 10 cas de contrôle du
// référentiel (references/reel/soleil_controle.json : algorithme NREL SPA, Reda et Andreas 2004, ± 0,0003°) :
// hauteur géométrique, hauteur apparente et azimut à ± 0,05° (azimut : écart mesuré sur le ciel, Δaz × cos h). Un écart rend le test rouge.
// usage : test_fidelite_soleil [dossier du dépôt]   (par défaut WFS_SOURCE_DIR, fixé par CMakeLists.txt)
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/render/bench_realisme.h"
using namespace wfs::reel;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    Json j; std::string e;
    if (!lireJson(racine + "/references/reel/soleil_controle.json", j, e)) { std::printf("ÉCHEC : %s\n", e.c_str()); return 1; }
    const Json* ms = j.get("mesures");
    VERIFIE(ms && ms->t == Json::Arr && ms->a.size() == 30, "30 valeurs attendues (10 cas × hauteur, hauteur apparente, azimut)");
    if (!ms) return 1;
    int n = 0; double pire = 0;
    for (const Json& m : ms->a) {
        int a, mo, jr; double h;
        const std::string id = m.texte("id");
        VERIFIE(lireDateUtc(m.texte("utc"), a, mo, jr, h), "%s : date illisible", id.c_str());
        const Soleil s = soleil(m.nombre("lat", 0), m.nombre("lon", 0), a, mo, jr, h);
        const double ref = m.nombre("valeur", -999), tol = m.get("tolerance") ? m.get("tolerance")->nombre("valeur", 0) : 0;
        double jeu = 0;
        if (id.find(".hauteur_apparente") != std::string::npos) jeu = s.hauteurApparente;
        else if (id.find(".hauteur") != std::string::npos) jeu = s.hauteur;
        else { jeu = s.azimut; }
        double ecart = std::fabs(jeu - ref), brut = ecart;
        if (id.find(".azimut") != std::string::npos) {
            // azimut : écart sur le cercle, puis mesuré sur le ciel (× cos de la hauteur) : près du zénith, l'azimut n'a plus de
            // sens (cas 4, 88° de haut : 0,065° d'azimut = 0,002° sur le ciel) ; l'écart brut est affiché
            if (ecart > 180) ecart = 360 - ecart;
            brut = ecart; ecart *= std::cos(rad(s.hauteur));
        }
        pire = std::max(pire, ecart);
        VERIFIE(tol > 0 && ecart <= tol, "%s : soleil() %.4f°, référence %.4f°, écart %.4f° > %.2f°", id.c_str(), jeu, ref, ecart, tol);
        std::printf("%s (%g° N, %g° E, %s UTC) : soleil() %.4f°, NREL SPA %.4f°, écart %.4f°%s\n", id.c_str(), m.nombre("lat", 0), m.nombre("lon", 0),
                    m.texte("utc").c_str(), jeu, ref, ecart, brut != ecart ? (" (brut " + std::to_string(brut).substr(0, 6) + "°)").c_str() : "");
        n++;
    }
    // cas limites : jour julien de J2000 (Meeus, ex. 7.a : 2000-01-01 12 h TT = 2451545,0), soleil sous l'horizon la nuit
    VERIFIE(std::fabs(jourJulien(2000, 1, 1) + 0.5 - 2451545.0) < 1e-9, "jour julien de J2000");
    VERIFIE(soleil(43.5, 7.0, 2026, 7, 15, 21.5).hauteur < 0, "soleil sous l'horizon à 23 h 30 (heure d'été) sur la Corniche");
    int a, mo, jr; double h;
    VERIFIE(!lireDateUtc("2026-13-01T00:00:00", a, mo, jr, h) && !lireDateUtc("n'importe quoi", a, mo, jr, h), "dates invalides refusées");
    std::printf("soleil : %d valeurs comparées, plus grand écart %.4f° (tolérance 0,05°)\n", n, pire);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_fidelite_soleil : OK\n");
    return 0;
}
