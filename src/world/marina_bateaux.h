// WaForSpeed — WFS-376 (épic E12) : bateaux de la marina de la Corniche, modélisés (fin des « triangles blancs », ANO-206).
// Géométrie calculée sur le processeur seulement (aucun Vulkan) : la carte (plage.cpp) la pose dans le décor préparé au
// chargement (WFS-268), sans appel de dessin en plus ; ce fichier est vérifié par tests/test_marina_bateaux.cpp.
//
// Repère local d'un bateau : mètres, y vers le haut (0 = flottaison), z vers l'avant (étrave en +L/2, tableau arrière en −L/2),
// x sur le travers (le modèle est symétrique). Toutes les surfaces sont des « lofts » d'anneaux à normales lissées (coques,
// rouf, mât, bôme, sac de grand-voile, génois enroulé) ; les câbles sont des lignes de 2 cm (deux quads croisés).
// Mesures (RG-04, RG-05 de regles-de-gestion/WFS-376.md) :
//   voilier de croisière au port : 9,6 m, bau 3,4 m, franc-bord ≈ 1,1 m, mât à 1,38 × L au-dessus de l'eau (13,25 m),
//     bôme à 2,0 m au-dessus du pont, sac de grand-voile ≈ 3,0 × 0,35 × 0,4 m, génois enroulé sur l'étai (Ø 0,15 m),
//     AUCUNE voile hissée (au port, les voiles sont rangées) ;
//   yacht à moteur : 18 m, bau 4,9 m, franc-bord 1,9 m, flybridge ; vedette : le même sans flybridge, posé à l'échelle 0,6.
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"

namespace wfs {
namespace marina {

// couleur d'un sommet : une case de la palette de la carte, ou une couleur choisie à la pose (coque, toiles)
constexpr int COUL_COQUE = -1, COUL_TOILE = -2;

struct Sommet { vec3 p, n; int couleur; };
struct Maille {
    std::vector<Sommet> v;
    std::vector<uint32_t> i;
    int triangles() const { return (int)(i.size() / 3); }
};

// cases de la palette utilisées par les modèles (données par la carte)
struct Couleurs { int gelcoat = 0, pont = 0, vitre = 0, mat = 0, cable = 0, corde = 0, amarre = 0, beton = 0, bois = 0, flotteur = 0, fonte = 0; };

enum Modele { VOILIER, VEDETTE, YACHT, NB_MODELES };

// un modèle : version détaillée (bloc à moins de 200 m) et allégée (au-delà : coque ≤ 12 triangles, mât en 2 quads croisés)
struct Bateau {
    Maille pres, loin;
    float L = 0, B = 0, T = 0;           // longueur hors tout, bau, tirant d'eau (m, échelle 1)
    float francPoupe = 0;                // hauteur du pont au tableau arrière (m au-dessus de l'eau)
    float francEtrave = 0;               // hauteur du pont à l'étrave
    float demiPoupe = 0;                 // demi-largeur au tableau arrière
    float plateforme = 0;                // longueur de la plateforme de bain derrière le tableau (bateaux à moteur)
    float hautMat = 0;                   // haut du mât au-dessus de l'eau (voilier), 0 sinon
};

Bateau construire(Modele m, const Couleurs& c);

// lignes ajoutées à la pose (amarres, pendille, passerelle) : triangles par bateau, pour le budget (RG-10)
int trianglesAmarrage(Modele m, bool aQuai);
// ces lignes, dans le repère du bateau mais en mètres réels (bateau posé à l'échelle s) : ecart = distance du bout de la poupe
// à la ligne d'amarrage (quai ou ponton ; < 0 : au mouillage), yQuai = dessus du quai ou du ponton au-dessus de l'eau
Maille amarrage(const Bateau& b, Modele m, float s, float ecart, float yQuai, const Couleurs& c);

// ---------- équipements du port (vrais objets modélisés, pas de formes nues)
// bitte d'amarrage en fonte (Ø 0,38 m à la base, 0,48 m de haut, tête à rebord), posée sur le quai, base en (0, 0, 0)
Maille bitte(const Couleurs& c);
// ponton flottant de longueur l (le long de +z), 2,2 m de large, dessus à 0,5 m au-dessus de l'eau : platelage de béton à
// bords chanfreinés, lisses de bois, flotteurs, bornes de distribution (eau, électricité) tous les 12 m, pieu de guidage au bout ; passerelle articulée depuis le quai (hautQuai : dessus du quai au-dessus de l'eau, côté z < 0)
Maille ponton(float l, float hautQuai, const Couleurs& c, bool loin);
constexpr float PONTON_LARGEUR = 2.2f, PONTON_HAUT = 0.5f;

// ---------- outils (aussi utilisés par la carte)
// surface lissée entre anneaux de même nombre de points ; sens des triangles : vers l'extérieur de l'axe [a, b]
void loft(Maille& m, const std::vector<std::vector<vec3>>& anneaux, int couleur, bool ferme, vec3 a, vec3 b);
// polygone plan (éventail), tourné vers la direction dehors
void eventail(Maille& m, const std::vector<vec3>& pts, int couleur, vec3 dehors);
// câble ou cordage de largeur l (deux quads croisés, vus de tous les côtés)
void ligne(Maille& m, vec3 a, vec3 b, float l, int couleur);

} // namespace marina
} // namespace wfs
