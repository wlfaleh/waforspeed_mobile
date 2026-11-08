// WaForSpeed — WFS-273 (RG-01, décision du PO du 2026-11-08 06:40 UTC) : arbres en exemplaires (src/world/arbres_variantes.h).
// Vérifié sur une forêt dense tirée au hasard (2 000 arbres sur 400 × 60 m) : aucun clone (même modèle, < 10 m, lacets à
// moins de 30°) ; nombre d'arbres et pieds inchangés ; lacet seulement tourné par pas de 15° ; les 3 modèles servent ;
// la matrice d'un exemplaire redonne le pied, la hauteur et (avec le facteur de la normale) la teinte de l'arbre.
#include <cmath>
#include <cstdio>
#include "../src/render/renderer.h"
#include "../src/world/arbres_variantes.h"

using namespace wfs;
static int g_controles = 0, g_echecs = 0;
#define VERIFIE(c, ...) do { g_controles++; if (!(c)) { g_echecs++; std::printf("[ÉCHEC] " __VA_ARGS__); std::printf("\n"); } else { std::printf("[OK] " __VA_ARGS__); std::printf("\n"); } } while (0)

int main() {
    Mulberry32 R(12345);
    std::vector<arbres::Arbre> v;
    for (int i = 0; i < 2000; i++)   // dans l'ordre de la route (x croissant)
        v.push_back({{(float)i * 0.2f + (float)R.next() * 3, 0, (float)R.next() * 60 - 30}, (float)R.next() * arbres::TOUR, (uint32_t)(R.next() * 4e9), 0});
    const std::vector<arbres::Arbre> avant = v;
    const int tournes = arbres::choisirModeles(v);
    int clones = 0;
    for (size_t i = 0; i < v.size(); i++) for (size_t j = i + 1; j < v.size(); j++) clones += arbres::clone(v[i], v[j]);
    VERIFIE(clones == 0 && arbres::compterClones(v) == 0, "aucun clone (même modèle, < 10 m, lacets à moins de 30°) : %d (compterClones : %d)", clones, arbres::compterClones(v));
    bool pieds = v.size() == avant.size(), pas60 = true;
    int n[arbres::MODELES] = {};
    for (size_t i = 0; i < v.size(); i++) {
        pieds = pieds && v[i].pied.x == avant[i].pied.x && v[i].pied.z == avant[i].pied.z;
        const float d = std::fmod(v[i].lacet - avant[i].lacet + 10 * arbres::TOUR, arbres::PAS_LACET);
        pas60 = pas60 && (d < 1e-3f || d > arbres::PAS_LACET - 1e-3f);
        n[v[i].modele]++;
    }
    VERIFIE(pieds, "nombre d'arbres et pieds inchangés (%zu)", v.size());
    VERIFIE(pas60, "lacet tourné seulement par pas de 15° (%d arbres tournés)", tournes);
    VERIFIE(n[0] > 400 && n[1] > 400 && n[2] > 400, "les 3 modèles servent (%d / %d / %d)", n[0], n[1], n[2]);
    // exemplaire : pied (12, 2, -5), lacet 1,1 rad, arbre de 14 m pour un modèle de 10 m, teinte 0,93
    const float h = 14, h0 = 10, tk = 0.93f, s = h / h0;
    const Instance ex = instance(arbres::matriceArbre({12, 2, -5}, 1.1f, s), tk / s);
    const vec3 pied{ex.l0[3], ex.l1[3], ex.l2[3]};
    const float echelle = std::sqrt(ex.l0[1] * ex.l0[1] + ex.l1[1] * ex.l1[1] + ex.l2[1] * ex.l2[1]);
    VERIFIE(length(pied - vec3{12, 2, -5}) < 1e-5f, "pied redonné par l'exemplaire");
    VERIFIE(std::fabs(echelle * h0 - h) < 1e-4f, "hauteur redonnée (%.3f m)", echelle * h0);
    VERIFIE(std::fabs(ex.x[0] * echelle - tk) < 1e-5f, "teinte redonnée (%.3f)", ex.x[0] * echelle);
    std::printf("test_arbres_variantes : %d contrôles, %d échec(s)\n", g_controles, g_echecs);
    return g_echecs ? 1 : 0;
}
