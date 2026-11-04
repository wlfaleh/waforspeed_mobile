// WaForSpeed — scènes du banc d'essai des roues animées (WFS-60) : roues-arret-1.png, roues-arret-2.png (identiques),
// roues-roule.png (jantes tournées), roues-virage.png (roues avant braquées, arrière droites)
#pragma once
#include <string>
namespace wfs {
class Renderer;
void benchRoues(Renderer& r, const std::string& out);
}
