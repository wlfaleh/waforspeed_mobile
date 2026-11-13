// WaForSpeed — WFS-380 : paramètres de l'eau du jeu (une seule eau, shaders/eau.glsl) et sommets qui les portent.
// RG-05 : mer de la Corniche, bassin de la marina, rivière de la Ville, sillages : même matériau MAT_EAU ; chaque lieu donne ses
// réglages dans les sommets de sa surface, sans shader à part :
//   uv.x = distance au bord de l'eau (m, négative sous le sable), uv.y = abscisse le long du bord (m : lignes d'écume parallèles au bord) ;
//   normale = (pente du fond, ±agitation, 0) : |xz| = pente (m de profondeur par m de large), |y| = agitation (1 pleine mer),
//   y < 0 = eau douce trouble (rivière). Le shader en tire la profondeur (RG-03), l'écume du bord (RG-04) et les vagues (RG-01).
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "renderer.h"
#include "eau_vagues.h"

namespace wfs::eau {

// lieux (agitation) et fonds (pente)
constexpr float AGIT_MER = 1.f, AGIT_PORT = 0.3f, AGIT_RIVIERE = 0.15f;
constexpr float PENTE_RIVIERE = 0.25f;   // berges de la rivière (vase), 4 m de fond au plus (shader)
// sable (plage méditerranéenne : 1 à 3 % ; 3 % → 1,5 m de fond à 50 m du bord), roche (éboulis, pied de falaise), quai (mur vertical)
constexpr float PENTE_SABLE = 0.03f, PENTE_ROCHE = 0.3f, PENTE_QUAI = 1.f;

// sommet de surface d'eau (voir l'en-tête) ; trouble : eau douce chargée (rivière)
inline Vertex sommet(vec3 p, float distBord, float abscisse, float pente, float agitation, bool trouble = false) {
    return {p.x, p.y, p.z, pente, trouble ? -agitation : agitation, 0, distBord, abscisse};
}
// quad d'eau (a, b, c, d dans l'ordre du quad ; distance au bord et abscisse le long du bord à chaque coin) ; Geo : v (Vertex) et i
template <class Geo>
void quad(Geo& g, const vec3 (&p)[4], const float (&dist)[4], const float (&abscisse)[4], float pente, float agitation, bool trouble = false) {
    const uint32_t k = (uint32_t)g.v.size();
    for (int j = 0; j < 4; j++) g.v.push_back(sommet(p[j], dist[j], abscisse[j], pente, agitation, trouble));
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
// D5 (avis de l'agent 3D) : nappe du ressac posée sur le sable (mode 3), un quad par morceau de plage : uv.x = remontée sur la
// plage depuis le bord (m), uv.y = abscisse le long du bord ; normale vers le haut (non lue)
template <class Geo>
void nappe(Geo& g, const vec3 (&p)[4], const float (&remontee)[4], const float (&abscisse)[4]) {
    const uint32_t k = (uint32_t)g.v.size();
    for (int j = 0; j < 4; j++) g.v.push_back({p[j].x, p[j].y, p[j].z, 0, 1, 0, remontee[j], abscisse[j]});
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
// RG-04 (WFS-380) puis WFS-401 (RG-04) : gerbe d'un dauphin qui sort de l'eau ou y rentre, en deux parties dans UN maillage (un seul
// appel de dessin) ; repère : rayon 1 au sol, hauteur 1 (la partie pose l'échelle : rayon et hauteur de la gerbe selon son âge).
//  - nappe d'écume posée sur l'eau, rayon 1 : uv = position sur le disque (|uv| ≤ 1) ; le shader en fait une dentelle au bord
//    déchiré (aucun disque visible) ;
//  - couronne d'eau soulevée (« crown splash » d'un corps qui perce la surface) : paroi qui s'évase de 0,5 à 0,8 du rayon en montant,
//    ANNEAU_N tranches autour, ANNEAU_H rangs en hauteur ; uv = (tour 0..1, 2 + hauteur 0..1) : uv.y ≥ 2 marque la couronne (la
//    normale n'est pas normalisée par le vertex shader, l'échelle de la gerbe la déforme : on ne s'en sert que pour l'éclairage).
//    Le shader la découpe en nappe laiteuse en bas, doigts et gouttes en haut : jamais une paroi pleine (règle des formes nues).
constexpr int ECLAB_DISQUE = 20, ECLAB_ANNEAU_N = 24, ECLAB_ANNEAU_H = 4;
constexpr float ECLAB_R_BAS = 0.5f, ECLAB_R_HAUT = 0.8f;
template <class Geo>
void eclaboussure(Geo& g) {
    constexpr float PI2 = 2 * 3.14159265f;
    const uint32_t c = (uint32_t)g.v.size();
    g.v.push_back({0, 0, 0, 0, 1, 0, 0, 0});
    for (int j = 0; j < ECLAB_DISQUE; j++) {
        const float a = PI2 * j / ECLAB_DISQUE;
        g.v.push_back({std::cos(a), 0, std::sin(a), 0, 1, 0, std::cos(a), std::sin(a)});
    }
    for (int j = 0; j < ECLAB_DISQUE; j++) g.i.insert(g.i.end(), {c, c + 1 + (uint32_t)((j + 1) % ECLAB_DISQUE), c + 1 + (uint32_t)j});
    // couronne : (ANNEAU_N + 1) colonnes (la dernière double la première pour que le tour aille de 0 à 1 sans saut d'uv)
    const uint32_t k0 = (uint32_t)g.v.size();
    for (int h = 0; h <= ECLAB_ANNEAU_H; h++) {
        const float v = (float)h / ECLAB_ANNEAU_H, r = ECLAB_R_BAS + (ECLAB_R_HAUT - ECLAB_R_BAS) * v * v;
        for (int j = 0; j <= ECLAB_ANNEAU_N; j++) {
            const float u = (float)j / ECLAB_ANNEAU_N, a = PI2 * u;
            // normale de la paroi évasée : vers l'extérieur, un peu vers le haut (pente de la couronne)
            const float ny = (ECLAB_R_HAUT - ECLAB_R_BAS) * 2 * v, l = std::sqrt(1 + ny * ny);
            g.v.push_back({r * std::cos(a), v, r * std::sin(a), std::cos(a) / l, ny / l, std::sin(a) / l, u, 2 + v});
        }
    }
    for (int h = 0; h < ECLAB_ANNEAU_H; h++)
        for (int j = 0; j < ECLAB_ANNEAU_N; j++) {
            const uint32_t a = k0 + (uint32_t)(h * (ECLAB_ANNEAU_N + 1) + j), b = a + ECLAB_ANNEAU_N + 1;
            g.i.insert(g.i.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
}
// WFS-401 (RG-04) : hauteur de la couronne (m) selon l'âge de la gerbe (s) : l'eau soulevée monte à ≈ 4,2 m/s et retombe sous la
// pesanteur (sommet ≈ 0,9 m à 0,43 s : gerbe d'un dauphin commun de 2,3 m qui sort à 5,4 m/s) ; 5 cm au plus bas (la nappe)
inline float hauteurGerbe(float age) { return std::fmax(0.05f, age * (4.2f - 4.905f * age)); }

// modes du matériau (cutoff) : surface, sillage, éclaboussure (shaders/eau.glsl)
constexpr float MODE_SURFACE = 0, MODE_SILLAGE = 1, MODE_ECLABOUSSURE = 2, MODE_NAPPE = 3;
inline Material materiau(float mode, bool transparent) {
    Material m; m.color = {1, 1, 1, 1}; m.rough = 0.1f; m.metal = 0; m.cutoff = mode;
    m.flags = MAT_EAU | MAT_DOUBLE | (transparent ? MAT_BLEND : 0u);
    return m;
}

} // namespace wfs::eau
