// WaForSpeed — scènes du banc d'essai de la nuit (WFS-09, WFS-16) : course-nuit-ville-0 / -1 (deux images proches),
// course-nuit-ville-haut, course-nuit-ville-stop, course-nuit-ville-recul, course-nuit-campagne-0 / -haut,
// course-nuit-circuit-0 / -haut ; coût GPU des reflets au journal. Appelées par src/platform/desktop_main.cpp
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchNuit(Game& g, Renderer& r, const std::string& out);
}
