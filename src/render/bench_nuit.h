// WaForSpeed — scènes du banc d'essai de la nuit (WFS-09, WFS-16) : course-nuit-ville-0 / -1 (deux images proches), phares-basse-ville / -corniche -0 / -face (WFS-218),
// course-nuit-ville-haut, course-nuit-ville-stop, course-nuit-ville-recul, course-nuit-campagne-0 / -haut,
// course-nuit-circuit-0 / -haut ; coût GPU des reflets au journal. Appelées par src/platform/desktop_main.cpp
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchNuit(Game& g, Renderer& r, const std::string& out);
// WFS-142 (RG-08, RG-09) : chaussée sèche / mouillée, même caméra : route-nuit-ville-sec, route-nuit-ville-pluie, route-nuit-ville-pluie-proche (vue basse),
// route-nuit-corniche, route-jour-sec, route-jour-pluie, route-nuit-ville-pluie-basse, route-nuit-ville-pluie-120-0 / -1 ;
// chacune aussi en « -sansreflet » (même image, reflet de la chaussée coupé : WFS_REFLETS=0) pour la mesure de RG-08
void benchRoute(Game& g, Renderer& r, const std::string& out);
// WFS-382 : banc court des phares de nuit (WFS_BANC=phares) : phares-poursuite-proche / -face / -basse (capture 1, police
// derrière le joueur), phares-ville-face (capture 2, voiture vue de face), phares-campagne-capot (capture 3, hors lampadaires),
// phares-campagne-capot-pluie et phares-ville-face-pluie (capture 4), témoins de jour phares-ville-jour-proche / -face
void benchPhares(Game& g, Renderer& r, const std::string& out);
}
