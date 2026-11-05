// WaForSpeed — banc d'essai des caméras (WFS-66) : captures camera-poursuite(-arret, -trafic), camera-proche(-arret),
// camera-capot, camera-cockpit(-droit).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// course réelle (Golf, ville) : Poursuite et Proche à l'arrêt pendant le décompte (WFS-105), Poursuite en course, puis appuis
// sur le bouton de vue → Proche, Capot, Cockpit en ligne droite puis en virage (volant braqué), retour à la Poursuite et
// capture dans le trafic, 6 voitures devant (WFS-105) ; la vue d'avant le banc est remise à la fin, puis g.shutdown()
void benchCameras(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
