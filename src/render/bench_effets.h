// WaForSpeed — scènes du banc d'essai des effets de conduite (WFS-65) : choc-etincelles.png, accident-fumee.png,
// nitro-lignes.png ; WFS-107 : braquage-{gauche,droite}-{30,120,250}-{jour,nuit}.png (fumée des pneus aux flèches) ;
// appelées par src/platform/desktop_main.cpp
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchEffets(Game& g, Renderer& r, const std::string& out);
}
