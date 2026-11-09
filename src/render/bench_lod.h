// WaForSpeed — banc des niveaux de détail au loin (WFS-265, RG-01 ; retour du lead dev sur la PR #268) : WFS_BANC=lod.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// captures lod-*.png dans out : vues au téléobjectif à 50, 150 et 300 m des 4 cartes (Haute et Basse), traversée du fondu
// d'effacement en Basse (Ville), feux arrière du trafic de nuit en Basse à 10, 25 et 50 m ;
// WFS-335 : trafic de nuit en Basse de 60 à 170 m, passage de 110 à 40 px (lod-passage-*), Course à 5 de nuit en Basse
// (vues Arrière et Proche) ; rend le nombre d'erreurs
int benchLod(Game& g, Renderer& r, const std::string& out);
}
