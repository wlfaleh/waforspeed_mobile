// WaForSpeed — module « course » (WFS-12) branché sur le moteur par les systèmes enfichables (WFS-30).
// Règles : course.h. Ce module tient la course solo de la partie : compte à rebours affiché, chrono, checkpoints,
// score, fin de partie (écran de fin avec « Rejouer » : même carte, même graine de trafic).
// Le HUD de course (vitesse, chrono, score en continu) viendra avec WFS-27 : il lira activeCourse().
#pragma once
#include <string>
#include "course.h"

namespace wfs {

class Game;
class Renderer;

// course de la partie active (nullptr si aucune) : lecture pour le trafic, le HUD (WFS-27) et le banc d'essai
const Course* activeCourse();
Course* activeCourseMut();   // module trafic seulement : immobilisation au départ et événements des chocs
// ralenti après un accident : durée réelle de l'image → durée de jeu (appelé par Game::update)
double courseScaleDt(double raw);
// banc d'essai : false = départ lancé, temps illimité, sans ralenti (scènes des autres tickets inchangées) ;
// la scène de la course (benchCourseScene) remet les règles. Vrai par défaut (jeu).
void courseBenchRules(bool on);
// banc d'essai : temps restant forcé (pour voir la fin de partie)
void courseBenchSetTime(double seconds);
// scènes du banc d'essai de la course (captures course-decompte-*.png, course-fin.png), appelées par desktop_main.cpp
void benchCourseScene(Game& g, Renderer& r, const std::string& outDir);

} // namespace wfs
