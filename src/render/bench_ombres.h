// WaForSpeed — scènes du banc d'essai pour les ombres du soleil (WFS-07)
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
// ombres-a.png / ombres-b.png : deux captures à 1 m d'écart (stabilité des bords) ; ombres-coupees.png : sans soleil (nuit)
void benchShadowScenes(Game& g, Renderer& r, const std::string& out);
void benchOmbres(Game& g, Renderer& r, const std::string& out);   // WFS-227 : banc « ombres » (WFS_BANC=ombres)
}
