// WaForSpeed — banc d'essai de la conduite libre (WFS-90) : conduite-demitour.png (voiture en travers pendant le
// demi-tour), conduite-contresens.png (voiture retournée sur la route, caméra derrière elle, trafic qui arrive en face)
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchConduiteLibre(Game& g, Renderer& r, const std::string& out);
}
