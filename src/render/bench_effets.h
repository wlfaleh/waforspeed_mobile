// WaForSpeed — scènes du banc d'essai des effets de conduite (WFS-65) : choc-etincelles.png, accident-fumee.png,
// nitro-lignes.png ; WFS-107 : braquage-{gauche,droite}-{100,150,250}-{jour,nuit}.png (fumée des pneus aux flèches) ;
// WFS-224 : accident-fumee-nuit.png, fumee-derapage-{jour,nuit}.png (volutes en vrai dérapage) ;
// appelées par src/platform/desktop_main.cpp
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchEffets(Game& g, Renderer& r, const std::string& out);
}
