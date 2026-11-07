// WaForSpeed — giratoires reconstruits (WFS-248, épic E2 ; refus de WFS-223 par l'utilisateur, ANO-127).
// Cause des défauts (WFS-248, journal) : le tracé 2 fait passer la route TROIS fois au même endroit (courbe d'entrée de 90°,
// ¾ de tour de l'anneau, courbe de sortie de 90°, src/game/trace_cartes.cpp). Chaque passage dessine sa chaussée de 15 m et
// ses marquages de voie : plaques d'asphalte qui se chevauchent, lignes qui se croisent dans l'anneau, quart d'anneau manquant,
// îlot de 5 m seulement, cédez-le-passage de travers, trottoirs et objets posés pour une branche qui retombent sur une autre.
// Reconstruction : un ouvrage complet, posé une fois par giratoire PAR-DESSUS la chaussée de la route (+3 cm), aux dimensions
// d'un giratoire réel (guide du CEREMA « Giratoires en milieu urbain » / SETRA 1998 : rayon extérieur 15 à 25 m, anneau 7 à 9 m,
// îlot central infranchissable entouré d'une couronne franchissable) :
//   - îlot central de R_ILOT, bordure de 15 cm, gazon (le décor de chaque carte y pose fontaine, palmiers…) ;
//   - couronne franchissable en pavés (R_ILOT → R_COURONNE), un peu surélevée ;
//   - anneau d'asphalte continu sur 360° (R_COURONNE → R_EXT, 9 m), sans aucun marquage de voie ;
//   - lignes de rive : intérieure continue, extérieure continue sauf aux deux branches ;
//   - ligne de cédez-le-passage (type T'2 : traits de 0,50 m, intervalles de 0,50 m) en travers de la voie entrante ;
//   - îlots séparateurs peints (hachures blanches à 45°) sur chaque branche, du côté de la voie opposée.
// La voiture du joueur suit l'axe du tracé (anneau de 14 m) : elle reste dans l'anneau (R_COURONNE < 14 ± 1 m < R_EXT).
// Géométrie neutre (triangles étiquetés par surface, repère du monde) : chaque carte la convertit avec ses matériaux.
// dansOuvrage() sert aux cartes à ne rien poser sur l'ouvrage (trottoirs, lampadaires, bas-côtés) en dehors de l'îlot.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"

namespace wfs::giratoire {

constexpr float R_ILOT = 8.0f;        // îlot central (bordure comprise)
constexpr float R_COURONNE = 9.5f;    // couronne franchissable (pavés)
constexpr float R_EXT = 18.5f;        // bord extérieur de l'anneau : anneau de 9 m
constexpr float Y_ANNEAU = 0.03f, Y_MARQUAGE = 0.035f, Y_COURONNE = 0.06f, H_BORDURE = 0.15f;

enum Surface : uint8_t { ASPHALTE, MARQUAGE, PAVES, BORDURE, PELOUSE, NB_SURFACES };
struct Tri { vec3 a, b, c; Surface s; };

struct Giratoire {
    bool ok = false;
    vec3 c{};              // centre de l'anneau (y : niveau de la chaussée)
    int64_t debut = 0, fin = 0, entree = 0, sortie = 0;   // segments (tour) : section, début et fin de l'anneau (¾ de tour)
    float angEntree = 0, angSortie = 0;   // angle polaire (rad, atan2(z, x) autour de c) des deux branches au bord extérieur
    float ecartAxe = 0;    // contrôle : écart max entre l'axe du tracé dans l'anneau et le cercle de 14 m (m)
    std::vector<float> autresBranches;   // angles polaires des branches du décor sans route (amorces) : rive extérieure ouverte
};

// ouvrage du rond-point de la section s (type RondPoint) ; ok = false si la géométrie n'est pas reconnue
Giratoire calculer(Track& t, const trace::Section& s);
// tous les triangles de l'ouvrage (îlot, couronne, anneau, marquages, cédez-le-passage, îlots séparateurs)
void geometrie(const Giratoire& g, Track& t, std::vector<Tri>& out);
// le point q (monde, xz) est-il sur l'ouvrage (anneau, couronne, îlot), à la marge près ? (rien d'autre n'y est posé)
inline bool dansOuvrage(const Giratoire& g, const vec3& q, float marge = 1.0f) {
    if (!g.ok) return false;
    const float dx = q.x - g.c.x, dz = q.z - g.c.z;
    return dx * dx + dz * dz < (R_EXT + marge) * (R_EXT + marge);
}
// coordonnées de texture de l'asphalte de l'ouvrage pour le matériau de la chaussée (F_ROAD : uv = mètres) : v = z local,
// u = 10 + onde triangle de x local dans [0, 20] (continue, jamais près de 0 ni de ±RW/3 ni des rives : aucun marquage peint)
inline vec2 uvChaussee(const Giratoire& g, const vec3& q) {
    const float x = q.x - g.c.x, m = x - 40.f * std::floor(x / 40.f);
    return {10.f + 20.f - std::fabs(m - 20.f), q.z - g.c.z};
}
// distance (m, plan) du point q au centre
inline float distanceCentre(const Giratoire& g, const vec3& q) { const float dx = q.x - g.c.x, dz = q.z - g.c.z; return std::sqrt(dx * dx + dz * dz); }

} // namespace wfs::giratoire
