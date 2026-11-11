// WaForSpeed — volant de la vue Cockpit, construit par code sans rendu (WFS-414), pour être vérifié par tests/test_cockpit.cpp :
// RG-05 (branches attachées au moyeu et à la jante, cotes du moyeu ; volants des autres voitures identiques à main) et RG-07
// (haut de la jante devant la visière du combiné). vehicule_cockpit.cpp en fait des pièces (un maillage par matériau) attachées
// au volant. Repère du volant : plan xy, +z vers le conducteur, centre de la jante à l'origine.
#pragma once
#include <array>
#include "vehicule_cockpit.h"
#include "vehicule_cockpit_geo.h"

namespace wfs::cockpit {

// volant de série (VolantSerie, volantSeriePour, cotes du moyeu) : vehicule_cockpit.h ; autres cotes (RG-02, RG-03) : ici
constexpr float BRANCHE_W0 = 0.055f, BRANCHE_W1 = 0.035f, BRANCHE_T = 0.020f;   // RG-02 : 55 mm à la sortie de la coque, 35 mm à la jante ; épaisseur
constexpr float BRANCHE_DANS = 0.012f;   // RG-02 : la branche naît 12 mm à l'intérieur du bord du coussin (≥ 10 mm)
constexpr float BRANCHE_Z0 = -0.022f, BRANCHE_Z1 = -0.004f;   // milieu de la branche (z) au départ (dans la coque) et à la jante

// pièces du volant, une par matériau (même ordre de dessin qu'avant WFS-414 ; les 4 dernières : volant de série seulement)
struct PiecesVolant {
    Geo jante, moyeu, branches, coutures[3], repere, palettes, manettino, molettes;
    Geo coque, joint, plaques, touches;
};
void construireVolant(const InterieurDef& d, PiecesVolant& p);

// demi-largeur du coussin à la hauteur y (m), trapèze compris (forme du contour : volant de série)
float demiLargeurCoussin(const VolantSerie& s, float y);

// RG-07 : ligne de la visière du combiné (repère de l'œil), même calcul qu'avant WFS-414 pour les voitures sans volant de série ;
// ev = épaisseur, largeur = largeur totale (m)
struct Visiere { std::array<vec3, 4> pts; float ev, largeur; };
Visiere ligneVisiere(const InterieurDef& d);

} // namespace wfs::cockpit
