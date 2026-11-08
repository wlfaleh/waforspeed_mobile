// WaForSpeed — WFS-269 : occlusion ambiante précalculée du décor fixe (src/render/eclairage_precalcule.*).
// Scène d'essai : un sol de 40 × 40 m, un mur de 6 m de haut et 20 m de long, un auvent de 3 × 6 m à 3 m de haut.
// Vérifié : sol dégagé ≈ ciel entier ; pied du mur nettement plus sombre ; dessous de l'auvent plus sombre encore ; mur dégagé
// uniforme (les rayons vers le bas sont libres : la sonde tient déjà compte du sol) ; jamais sous le plancher ; même résultat à chaque calcul ; la végétation (recoit = false) n'est pas
// modifiée ; durée mesurée (RG-04).
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/render/eclairage_precalcule.h"

using namespace wfs;
static int g_controles = 0, g_echecs = 0;
#define VERIFIE(c, ...) do { g_controles++; if (!(c)) { g_echecs++; std::printf("[ÉCHEC] " __VA_ARGS__); std::printf("\n"); } else { std::printf("[OK] " __VA_ARGS__); std::printf("\n"); } } while (0)

// quadrilatère subdivisé (pas de 0,5 m), normale n
static void quad(std::vector<Vertex>& v, std::vector<uint32_t>& idx, vec3 o, vec3 u, vec3 w, int nu, int nw, vec3 n) {
    const uint32_t base = (uint32_t)v.size();
    for (int j = 0; j <= nw; j++)
        for (int i = 0; i <= nu; i++) {
            const vec3 p = o + u * ((float)i / nu) + w * ((float)j / nw);
            Vertex s{}; s.px = p.x; s.py = p.y; s.pz = p.z; s.nx = n.x; s.ny = n.y; s.nz = n.z; v.push_back(s);
        }
    for (int j = 0; j < nw; j++)
        for (int i = 0; i < nu; i++) {
            const uint32_t a = base + (uint32_t)(j * (nu + 1) + i), b = a + 1, c = a + (uint32_t)(nu + 1), d = c + 1;
            idx.insert(idx.end(), {a, b, d, a, d, c});
        }
}

static float aoPres(const std::vector<Vertex>& v, vec3 p) {   // occlusion du sommet le plus proche de p
    float best = 1e9f, ao = -1;
    for (const Vertex& s : v) {
        const float d = (s.px - p.x) * (s.px - p.x) + (s.py - p.y) * (s.py - p.y) + (s.pz - p.z) * (s.pz - p.z);
        if (d < best) { best = d; ao = precalcul::occlusionSommet(s); }
    }
    return ao;
}

int main() {
    std::vector<Vertex> sol, mur, auvent, feuille; std::vector<uint32_t> iSol, iMur, iAuvent, iFeuille;
    quad(sol, iSol, {-20, 0, -20}, {40, 0, 0}, {0, 0, 40}, 80, 80, {0, 1, 0});
    quad(mur, iMur, {-10, 0, 0}, {20, 0, 0}, {0, 6, 0}, 40, 12, {0, 0, 1});          // mur dans le plan z = 0, face vers +z
    quad(auvent, iAuvent, {12, 3, -3}, {6, 0, 0}, {0, 0, 6}, 12, 12, {0, -1, 0});    // auvent à 3 m, face vers le bas
    quad(feuille, iFeuille, {-15, 2, 10}, {2, 0, 0}, {0, 2, 0}, 4, 4, {0, 0, 1});    // « végétation » : ne reçoit pas
    const std::vector<Vertex> feuilleAvant = feuille;
    auto lots = [&]() {
        return std::vector<precalcul::Lot>{{&sol, &iSol, true, true}, {&mur, &iMur, true, true}, {&auvent, &iAuvent, true, true}, {&feuille, &iFeuille, true, false}};
    };
    std::vector<precalcul::Lot> L = lots();
    const precalcul::Bilan b = precalcul::occlusionDecor(L);
    std::printf("WFS-269 : %zu voxels, %zu sommets, occlusion moyenne %.3f, %.1f ms\n", b.voxels, b.sommets, b.moyenne, b.ms);

    const float libre = aoPres(sol, {-15, 0, -15}), piedMur = aoPres(sol, {0, 0, 0.5f}), sousAuvent = aoPres(sol, {15, 0, 0}),
                hautMur = aoPres(mur, {0, 6, 0}), basMur = aoPres(mur, {0, 0.5f, 0});
    std::printf("sol dégagé %.2f, pied du mur %.2f, sous l'auvent %.2f, mur en haut %.2f, mur en bas %.2f\n", libre, piedMur, sousAuvent, hautMur, basMur);
    VERIFIE(libre > 0.95f, "sol dégagé : ciel entier (%.2f)", libre);
    VERIFIE(piedMur < libre - 0.2f, "pied du mur plus sombre que le sol dégagé (%.2f contre %.2f)", piedMur, libre);
    VERIFIE(sousAuvent < piedMur, "dessous de l'auvent plus sombre que le pied du mur (%.2f contre %.2f)", sousAuvent, piedMur);
    VERIFIE(std::fabs(hautMur - basMur) < 0.05f, "mur dégagé : uniforme de haut en bas, le sol ne l'assombrit pas (%.2f / %.2f)", hautMur, basMur);
    const float murLoin = aoPres(mur, {-9, 3, 0});
    VERIFIE(murLoin > 0.9f, "mur dégagé : pas assombri par le sol (rayons vers le bas libres) (%.2f)", murLoin);
    float mini = 2;
    for (const auto* v : {&sol, &mur, &auvent}) for (const Vertex& s : *v) mini = std::min(mini, precalcul::occlusionSommet(s));
    VERIFIE(mini >= precalcul::OCCLUSION_MIN - 1e-4f, "jamais sous le plancher %.2f (minimum %.3f)", precalcul::OCCLUSION_MIN, mini);
    bool inchange = true;
    for (size_t k = 0; k < feuille.size(); k++) inchange = inchange && feuille[k].nx == feuilleAvant[k].nx && feuille[k].nz == feuilleAvant[k].nz;
    VERIFIE(inchange, "végétation (recoit = false) : normales inchangées (sa longueur porte sa teinte, MAT_VTINT)");

    // reproductible : même scène, même résultat (normales remises à l'unité)
    std::vector<float> premier; for (const Vertex& s : sol) premier.push_back(precalcul::occlusionSommet(s));
    for (auto* v : {&sol, &mur, &auvent}) for (Vertex& s : *v) { const float l = precalcul::occlusionSommet(s); s.nx /= l; s.ny /= l; s.nz /= l; }
    std::vector<precalcul::Lot> L2 = lots();
    precalcul::occlusionDecor(L2);
    bool pareil = true;
    for (size_t k = 0; k < sol.size(); k++) pareil = pareil && std::fabs(precalcul::occlusionSommet(sol[k]) - premier[k]) < 1e-5f;
    VERIFIE(pareil, "reproductible : même occlusion au second calcul");
    VERIFIE(b.ms < 2000, "durée du calcul de la scène d'essai : %.1f ms", b.ms);

    std::printf("fidelite_lumiere_precalcule : %d contrôles, %d échec(s)\n", g_controles, g_echecs);
    return g_echecs ? 1 : 0;
}
