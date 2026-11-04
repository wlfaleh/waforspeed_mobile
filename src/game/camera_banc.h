// WaForSpeed — banc d'essai des caméras (WFS-66) : captures camera-poursuite, camera-capot, camera-cockpit.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// course réelle (Golf, ville) : poursuite, puis appui sur le bouton de vue → capot, puis cockpit en virage (volant braqué),
// puis retour à la poursuite ; la vue d'avant le banc est remise à la fin, puis g.shutdown()
void benchCameras(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
