// WaForSpeed — WFS-325 (lot 1 de la vue Cockpit 3/3) : pièces propres au modèle de la Golf 8 GTI, de la M4 Competition, de la
// 911 Carrera 4S (992.1) et de la SF90 Stradale : aérateurs (nombre, forme et place du modèle), commandes de la planche, console
// centrale (sélecteur, commandes, rangements, accoudoir), garnitures des portières (poignée, accoudoir, haut-parleurs, décor),
// plafonnier. Géométrie construite par code, sans rendu (vérifiée par tests/test_cockpit.cpp), dans le repère de l'œil
// (x à droite, y en haut, −z devant), comme le reste de l'intérieur (vehicule_cockpit.cpp).
// Les autres voitures ne passent jamais ici : elles gardent exactement les pièces communes de main (RG-08).
// Sources : photos de presse de chaque intérieur (souvenir de l'agent, statut E de WFS-197 : aucune page n'a pu être ouverte,
// réseau des agents limité à GitHub) ; disposition et proportions décrites au journal de WFS-325, cotes estimées (E) à ±1 cm.
#pragma once
#include <array>
#include "vehicule_cockpit.h"
#include "vehicule_cockpit_geo.h"

namespace wfs::cockpit {

// matières des pièces propres ; correspondance avec les matériaux de l'intérieur : Interieur::construire (vehicule_cockpit.cpp)
enum class MatDetail : int {
    PlancheHaut,   // matière moussée grainée du dessus de planche
    PlancheBas,    // plastique grainé du bas de planche, de la console et des portières
    Accent,        // décor du modèle (liseré rouge, carbone, alu brossé)
    Laque,         // noir laqué (panneaux de commandes, décor « piano »)
    Chrome,        // chrome satiné (lunettes d'aérateurs, poignées)
    Lames,         // plastique noir mat (lames, fonds des aérateurs, touches, caoutchouc)
    Cuir,          // cuir (accoudoirs, soufflet, pommeau)
    Tissu,         // tissu, Alcantara (inserts de portière)
    Repere,        // plastique rouge satiné (démarreur, feux de détresse)
    Alu,           // aluminium brossé (grilles de haut-parleur, grille de la SF90)
    Lentille,      // lentilles des liseuses du plafonnier (éteintes)
    Ambiance,      // guide de lumière d'ambiance (éteint le jour, allumé la nuit : vehicule_cockpit.cpp)
    N
};
struct PiecesDetails {
    std::array<Geo, (size_t)MatDetail::N> g;
    Geo& operator[](MatDetail m) { return g[(size_t)m]; }
    const Geo& operator[](MatDetail m) const { return g[(size_t)m]; }
};

// vrai pour les 4 voitures du lot 1 : leurs aérateurs, console, garnitures de portière et plafonnier sont ceux de
// construireDetails, à la place des pièces communes ; faux pour toutes les autres (rien ne change pour elles)
bool detailsPropres(const InterieurDef& d);
// couleur sRGB de l'éclairage d'ambiance du modèle la nuit (0 : sans)
unsigned couleurAmbiance(const InterieurDef& d);
// pièces propres du modèle (vides pour une voiture hors du lot 1)
void construireDetails(const InterieurDef& d, PiecesDetails& p);

} // namespace wfs::cockpit
