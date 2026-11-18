// WaForSpeed — WFS-562 (RG-05 bis, RG-05 ter ; ANO-279 partie checkpoint, ANO-315) : portique de checkpoint modélisé au niveau
// du vrai objet, à la place des poteaux carrés, de la poutre en boîte, des barres néon cyan et de la bande lumineuse au sol de
// WFS-71 (règle de l'utilisateur du 2026-11-09 : aucune forme géométrique nue ; demande du 2026-11-18 : plus de cyan ni de néon).
//
// Le vrai objet : portique d'événementiel « en but » (goalpost) en treillis carré d'aluminium de 290 mm, deux poutres l'une
// au-dessus de l'autre, et une bâche imprimée tendue entre elles, comme les arches de passage des rallyes, des courses à pied et
// des épreuves de vélo. Le treillis, ses manchons et ses platines sont ceux du portique de départ (WFS-559, réutilisés).
//
// Repère local : mètres, y vers le haut (0 = chaussée), x en travers de la route (vers la droite du pilote), z vers l'ARRIÈRE
// (vers les voitures qui arrivent) : la bâche avant regarde vers +z. Ligne du checkpoint en z = 0.
// Mesures (fiches techniques publiques) :
//   - treillis carré 290 mm (membrures Ø 50 × 2 mm, diagonales Ø 20 mm), platines 600 × 600 × 10 mm : portique_depart_modele.h ;
//   - hauteur libre sous la poutre basse : 5,2 m (au-dessus des 4,0 m d'un poids lourd, directive 96/53/CE, et des 4,5 m de
//     hauteur libre des ouvrages routiers) ; 1,0 m libre entre les deux poutres ;
//   - bâche PVC enduit 510 g/m² (« frontlit », la plus courante en extérieur), 0,5 mm ; ourlet soudé de 40 mm sur le pourtour
//     (double épaisseur) ; œillets laiton nickelé Ø 12 mm intérieur, Ø 24 mm extérieur, un tous les 0,5 m ; tendeurs à boule
//     (sandow Ø 4 mm et boule de Ø 25 mm) autour des membrures ;
//   - impression : « CHECKPOINT N » noir sur fond orange, damier noir et blanc de 0,30 m aux deux bouts ; encre mate, non
//     lumineuse (RG-05 ter) ; la nuit, deux projecteurs LED (boîtier aluminium 300 × 250 × 70 mm à ailettes, lyre, bras de
//     0,6 m serré sur la membrure avant du pied) éclairent la bâche ;
//   - ligne de chronométrage peinte en travers de la chaussée : bande blanche de 0,30 m, peinture thermoplastique de 3 mm.
// Vérifié par tests/test_portique_checkpoint.cpp (mesures ±1 %, budget, normales, aucun triangle dégénéré).
#pragma once
#include "portique_depart_modele.h"   // treillis, platine (WFS-559)

namespace wfs {
namespace portique_checkpoint {

using Maille = marina::Maille;

// pièces (= matériaux) du modèle
// LAITON (œillets) et ATTACHE (tendeurs, même matériau que NOIR) : détails fins, dessinés seulement de près (DETAILS_M)
enum Piece { ALU = 0, ACIER = 1, NOIR = 2, BACHE = 3, ENCRE_NOIRE = 4, ENCRE_BLANCHE = 5, LAITON = 6, VERRE = 7, PEINTURE = 8, ATTACHE = 9, NB_PIECES = 10 };
constexpr float DETAILS_M = 40.f;   // au-delà, un œillet de 24 mm fait moins d'un pixel en 2400 × 1080

constexpr float SECTION = portique_depart::SECTION;
constexpr float HAUTEUR_LIBRE = 5.2f;          // sous la membrure basse de la poutre basse
constexpr float ENTRE_POUTRES = 1.0f;          // jour libre entre les deux poutres
constexpr float BACHE_H = ENTRE_POUTRES + SECTION;   // d'un axe de poutre à l'autre : 1,29 m
constexpr float BACHE_EP = 0.0005f, OURLET = 0.04f;
constexpr float OEILLET_PAS = 0.5f, OEILLET_DI = 0.012f, OEILLET_DE = 0.024f;
constexpr float DAMIER = 0.30f;                // côté d'une case du damier
constexpr int DAMIER_COLONNES = 3;
constexpr float LIGNE_L = 0.30f, LIGNE_EP = 0.003f;   // ligne de chronométrage peinte
constexpr float PROJ_W = 0.30f, PROJ_H = 0.25f, PROJ_P = 0.07f, PROJ_BRAS = 0.6f, PROJ_Y = 3.2f;

struct Modele {
    Maille pieces[NB_PIECES];
    float ouverture = 0;        // entre les faces intérieures des pieds
    float hauteurTotale = 0;    // dessus de la poutre haute
    float bacheL = 0;           // largeur de la bâche
    vec3 bacheCentre{};         // centre de la face avant de la bâche (texte, lumière de nuit)
    vec3 projecteurs[2]{};      // centre de la vitre de chaque projecteur
    vec3 visee[2]{};            // direction de chaque projecteur (unitaire)
};

// portique complet (pieds, deux poutres, bâches avant et arrière, projecteurs) d'ouverture libre `ouverture`, centré en x = 0,
// avec la ligne de chronométrage peinte sur largeurChaussee ; sansPortique : ligne seule (courbe serrée, tunnel)
Modele construire(float ouverture, float largeurChaussee, bool sansPortique = false);

} // namespace portique_checkpoint
} // namespace wfs
