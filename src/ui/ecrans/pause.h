// WaForSpeed — pause en course (ticket WFS-39, épic E8). Port de #menuBtnHud et de la section #pause du jeu web
// (index.html, js/ui/screens.js : pauseGame / resumeGame / startRace / toMenu ; css/style.css : .cam-btn, .btn, .btn--ghost).
// Système enfichable « Pause » (ordre 455) :
//   - en course : bouton « ‖ Menu » orange en haut à gauche, dans la zone sûre (sous le panneau du temps) ;
//   - écran Pause (Ecran::Pause, simulation figée par Game::update) : « Course en pause », « Réglages »,
//     boutons « Reprendre », « Recommencer », « Quitter la course ».
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
// banc d'essai : captures pause-bouton (encoche simulée de 90 px), pause-0, pause-reprise, pause-recommencer,
// pause-quitter, puis g.shutdown()
void benchPause(Game& g, Renderer& r, const std::string& outDir);
} // namespace ecrans
} // namespace wfs
