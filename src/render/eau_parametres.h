// WaForSpeed — WFS-380 : paramètres de l'eau du jeu (une seule eau, shaders/eau.glsl) et sommets qui les portent.
// RG-05 : mer de la Corniche, bassin de la marina, rivière et port de la Ville, sillages : même matériau MAT_EAU ; chaque lieu
// donne ses réglages dans les sommets de sa surface, sans shader à part :
//   uv.x = distance au bord de l'eau (m, négative sous le sable), uv.y = pente du fond (m de profondeur par m de large),
//   longueur de la normale (vers le haut) = agitation (1 pleine mer). Le shader en tire la profondeur (RG-03), l'écume du bord (RG-04)
//   et la hauteur des vagues (RG-01).
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "renderer.h"
#include "eau_vagues.h"

namespace wfs::eau {

// lieux (agitation) et fonds (pente)
constexpr float AGIT_MER = 1.f, AGIT_PORT = 0.3f, AGIT_RIVIERE = 0.15f;
// sable (plage méditerranéenne : 1 à 3 % ; 3 % → 1,5 m de fond à 50 m du bord), roche (éboulis, pied de falaise), quai (mur vertical)
constexpr float PENTE_SABLE = 0.03f, PENTE_ROCHE = 0.3f, PENTE_QUAI = 1.f;

// sommet de surface d'eau : normale vers le haut de longueur = agitation
inline Vertex sommet(vec3 p, float distBord, float pente, float agitation) {
    return {p.x, p.y, p.z, 0, agitation, 0, distBord, pente};
}
// quad d'eau (a, b, c, d dans l'ordre du quad, distances au bord à chaque coin) ; G : maillage avec v (Vertex) et i (indices)
template <class Geo>
void quad(Geo& g, const vec3 (&p)[4], const float (&dist)[4], float pente, float agitation) {
    const uint32_t k = (uint32_t)g.v.size();
    for (int j = 0; j < 4; j++) g.v.push_back(sommet(p[j], dist[j], pente, agitation));
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}

// RG-04 : sillage en V derrière un bateau (repère local : avant vers +z, posé sur l'eau à y), uv : x latéral −1..1, y 0 → 1 vers
// l'arrière ; `n` tranches pour que la largeur suive le V. Ouverture de Kelvin : 19,5° de chaque côté (largeur = 0,71 × distance).
template <class Geo>
void sillage(Geo& g, float y, float longueur, float largeurDepart, int n = 8) {
    const uint32_t k0 = (uint32_t)g.v.size();
    for (int j = 0; j <= n; j++) {
        const float v = (float)j / n, z = -1.6f - longueur * v, l = largeurDepart + 0.354f * longueur * v;
        g.v.push_back({-l, y, z, 0, 1, 0, -1, v});
        g.v.push_back({l, y, z, 0, 1, 0, 1, v});
    }
    for (int j = 0; j < n; j++) {
        const uint32_t a = k0 + 2 * j;
        g.i.insert(g.i.end(), {a, a + 2, a + 3, a, a + 3, a + 1});
    }
}
// RG-04 : éclaboussure (rayon 1, centre au repère), uv = position sur le disque : le shader fond le bord (aucun disque visible)
template <class Geo>
void eclaboussure(Geo& g, int n = 14) {
    const uint32_t c = (uint32_t)g.v.size();
    g.v.push_back({0, 0, 0, 0, 1, 0, 0, 0});
    for (int j = 0; j < n; j++) {
        const float a = 2 * 3.14159265f * j / n;
        g.v.push_back({std::cos(a), 0, std::sin(a), 0, 1, 0, std::cos(a), std::sin(a)});
    }
    for (int j = 0; j < n; j++) g.i.insert(g.i.end(), {c, c + 1 + (uint32_t)((j + 1) % n), c + 1 + (uint32_t)j});
}

// modes du matériau (cutoff) : surface, sillage, éclaboussure (shaders/eau.glsl)
constexpr float MODE_SURFACE = 0, MODE_SILLAGE = 1, MODE_ECLABOUSSURE = 2;
inline Material materiau(float mode, bool transparent) {
    Material m; m.color = {1, 1, 1, 1}; m.rough = 0.1f; m.metal = 0; m.cutoff = mode;
    m.flags = MAT_EAU | MAT_DOUBLE | (transparent ? MAT_BLEND : 0u);
    return m;
}

} // namespace wfs::eau
