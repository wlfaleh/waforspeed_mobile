// WaForSpeed — WFS-559 (épic E23, maquette R19 09) : portique de départ en treillis d'aluminium et ses 5 feux, modélisés au
// niveau du vrai objet (règle de l'utilisateur du 2026-11-09 : aucune forme géométrique nue). Géométrie calculée sur le
// processeur avec les outils de marina_bateaux.h (loft, eventail) et de decor_grandprix_modeles.h (tube, revolution), réutilisés.
// Le modèle sert aussi au portique « ARRIVÉE » de WFS-560 (même treillis, sans caisson de feux : avecFeux = false).
//
// Repère local : mètres, y vers le haut (0 = chaussée), x en travers de la route (vers la droite du pilote), z vers ARRIÈRE
// (vers les voitures qui arrivent sur la ligne) : les feux regardent vers +z. Ligne de départ en z = 0.
// Mesures (fiches techniques publiques des treillis carrés d'événementiel « 290 mm », famille la plus répandue) :
//   - section carrée de 290 mm hors tout ; membrures (tubes porteurs) Ø 50 × 2 mm, entraxe 240 mm ; diagonales Ø 20 × 2 mm,
//     en zigzag sur les 4 faces (≈ 45°, une diagonale tous les ≈ 240 mm) ; traverses d'extrémité Ø 20 mm à chaque bout de tronçon ;
//   - tronçons de 3 m (poutre) et de 2 m (pieds), assemblés par manchons coniques (Ø 60 mm, 120 mm) et broches Ø 12 mm ;
//   - pieds posés sur platines d'acier galvanisé 600 × 600 × 10 mm, coins chanfreinés de 30 mm, 4 boulons M24 ;
//   - feux de départ (disposition des portiques de départ FIA : 5 colonnes de feux côte à côte, lues de toute la grille) :
//     colonnes espacées de 0,9 m, 2 lanternes à LED Ø 200 mm par colonne (lentille bombée, bague, visière de 160 mm), dans un
//     caisson d'aluminium laqué noir de 4 600 × 620 × 220 mm (arêtes arrondies), pendu sous la poutre par 3 colliers.
// Vérifié par tests/test_portique_depart.cpp (mesures ±1 %, budget, normales, aucun triangle dégénéré).
#pragma once
#include "decor_grandprix_modeles.h"   // Maille, tube, revolution (WFS-373)

namespace wfs {
namespace portique_depart {

using Maille = marina::Maille;

// pièces (= matériaux) du modèle : couleur des sommets de la Maille
enum Piece { ALU = 0, ACIER = 1, NOIR = 2, LENTILLE = 3, NB_PIECES = 4 };

constexpr float SECTION = 0.29f;            // côté du treillis hors tout
constexpr float MEMBRURE_D = 0.05f, DIAGONALE_D = 0.02f;
constexpr float ENTRAXE = SECTION - MEMBRURE_D;   // 0,24 m entre axes des membrures
constexpr float PAS_DIAGONALE = 0.24f;
constexpr float TRONCON_POUTRE = 3.f, TRONCON_PIED = 2.f;
constexpr float PLATINE = 0.6f, PLATINE_EP = 0.01f;
constexpr float FEU_D = 0.2f, VISIERE = 0.16f;
constexpr int NB_FEUX = 5;
constexpr float FEU_PAS = 0.9f;              // entraxe des colonnes de feux
constexpr int FEUX_PAR_COLONNE = 2;
constexpr float FEU_PAS_V = 0.27f;           // entraxe vertical des 2 lanternes d'une colonne
constexpr float CAISSON_L = 4.6f, CAISSON_H = 0.62f, CAISSON_P = 0.22f;

struct Modele {
    Maille pieces[NB_PIECES - 1];   // ALU, ACIER, NOIR (les lentilles sont à part : une par feu, allumée ou non)
    Maille lentilles[NB_FEUX];      // une par colonne (ses 2 lanternes), de gauche à droite (vu du pilote)
    vec3 centreFeux{};              // centre de la face des feux (lumière ponctuelle de nuit)
    float ouverture = 0;            // distance entre les faces intérieures des pieds
    float hauteurLibre = 0;         // sous la membrure basse de la poutre
};

// portique d'ouverture libre `ouverture` (faces intérieures des pieds) et de hauteur libre `hauteurLibre` sous la poutre,
// centré en x = 0 ; caisson des 5 feux sous le milieu de la poutre si avecFeux
Modele construire(float ouverture, float hauteurLibre, bool avecFeux = true);

// poutre en treillis carré de SECTION de a à b (axe quelconque) ; e1 : direction d'une face (perpendiculaire à b − a) ;
// tronçons de longueur `troncon` (le dernier plus court), manchons coniques entre deux tronçons
void treillis(Maille& m, vec3 a, vec3 b, vec3 e1, float troncon, int couleur);

// WFS-562 : platine de pied chanfreinée (acier galvanisé, pièce ACIER) et ses 4 boulons, centre au sol en (x, z)
void platine(Maille& m, float x, float z);

} // namespace portique_depart
} // namespace wfs
