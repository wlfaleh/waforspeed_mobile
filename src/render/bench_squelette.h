// WaForSpeed — banc d'essai du point d'extension « maillages animés par squelette » (WFS-63).
// Système de test « squelette-essai » (ordre 905) : inactif hors banc ; actif, il prend la scène (ciel, soleil, ombres,
// brouillard de la course) et pose le personnage `homme` du jeu web sur la route.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// captures squelette-essai (Idle_Loop), squelette-marche-0 / -1 (Walk_Loop à deux instants), squelette-8 (coût, RG-05),
// puis g.shutdown()
void benchSquelette(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
