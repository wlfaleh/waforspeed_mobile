// WaForSpeed — HUD de course (ticket WFS-27, épic E5, version réduite du MVP) : temps + checkpoint (haut gauche),
// score + combo (haut droite), vitesse + nitro (bas centre, entre le volant et le frein). Port de js/ui/hud.js (updateHud)
// et de css/style.css (.hud-top, .time-value, .cp-bar, .score-value, .combo, .nitro-bar) du jeu web.
// Système enfichable (ordre 420) ; lecture seule de la course (activeCourse) et du joueur (GameView).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// banc d'essai : captures hud-course, hud-temps-bas, hud-nitro, hud-combo (dans outDir), puis g.shutdown()
void benchHud(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
