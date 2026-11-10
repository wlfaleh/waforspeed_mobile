// WaForSpeed — port de la Ville modélisé au niveau du vrai objet (ticket WFS-370, épic E15, file Ville).
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : plus aucune forme géométrique nue. Le port était fait de boîtes (conteneurs,
// grues, grillage) : chaque objet est ici modélisé par le code, pièce par pièce, aux mesures du vrai objet (sources dans
// decor_ville_port.cpp). Aucun modèle libre CC0 / CC BY n'est accessible par les sources permises (regles-reseau.md : GitHub
// seulement) aux bonnes mesures : RG-03 du ticket. Les entrepôts, eux, sont des bâtiments du kit d'architecture (style
// industriel, archi_carte.h), posés par ville.cpp.
// Toute la géométrie est écrite dans les tampons de la carte (morceaux préparés au chargement, WFS-268) : rien n'est créé en course.
#pragma once
#include "mobilier_ville.h"

namespace wfs::port {

using mobilier::Repere;
using mobilier::Sortie;

// conteneur ISO de 40 pieds (12,192 × 2,438 × 2,591 m) : poteaux d'angle, pièces de coin, longerons, parois ondulées,
// portes à 4 barres de verrouillage côté +z ; repère : origine au sol au centre, z le long du conteneur
// (loin : parois lisses, mêmes poteaux et longerons, barres sans ferrures)
void conteneur(Sortie s, const Repere& R, vec2 peinture, vec2 noir, bool loin);
constexpr float CONT_L = 12.192f, CONT_W = 2.438f, CONT_H = 2.591f;   // ISO 668 (série 1, type 1AA)

// portique de quai (grue à conteneurs) : repère au sol sur le rail côté terre, x vers le quai (côté eau), z le long du quai ;
// hauteur de levage h (m) ; pieds, poutres, flèche et avant-bec, chevalet, haubans, salle des machines, chariot, cabine, palonnier
void grue(Sortie s, Sortie verre, const Repere& R, float h, vec2 acier, vec2 blanc, vec2 noir, bool loin);
constexpr float GRUE_ECARTEMENT = 30.48f;   // écartement des rails (100 pieds)
constexpr float GRUE_AVANT = 45.f, GRUE_ARRIERE = 15.f;   // portée côté eau au-delà du rail, côté terre (m)

// clôture en panneaux rigides à double fil (h 2,03 m, maille 200 × 50 mm) de a à b (monde, au sol), poteau en a ;
// ext : côté extérieur (vers la rue)
void cloture(Sortie s, vec3 a, vec3 b, vec3 ext, vec2 peinture, bool loin);

}  // namespace wfs::port
