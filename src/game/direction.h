// WaForSpeed — modes de direction tactiles (ticket WFS-70, épic E3). Port de js/ui/screens.js du jeu web
// (setSteer, setSteerMode, #steerArrows) :
// - « zone de glissement » (par défaut) : pouce posé dans la partie gauche de l'écran puis glissé (volant analogique,
//   ±12 % de la largeur = butée) ; comportement et dessin identiques à ceux d'avant WFS-70 (code déplacé de Game) ;
// - « flèches » : deux boutons à gauche (flèche gauche, flèche droite) ; consigne −1 / +1 tant qu'on appuie (input.left /
//   input.right du jeu web), 0 au relâchement : Player::update fait monter et revenir le braquage progressivement.
// Le mode se lit dans reglagesRessenti().direction (ressenti.h).
#pragma once
#include "../core/math.h"
#include "../core/system.h"
#include "ressenti.h"

namespace wfs {

struct Touch;
class Renderer;

// ---------- accroches de Game (game.cpp : onTouch, drawControls, lacherCommandes) ----------
bool directionPossede(int id);   // ce doigt pilote déjà la direction
// toucher appuyé / déplacé / relâché ; libre : doigt qui ne tient encore aucune commande. true = toucher de la direction
bool directionToucher(const Touch& t, bool libre, float W, float H, const Marges& m);
float directionBraquage(float W);   // consigne de braquage −1..1 (DriveInput::steer)
void directionDessiner(Renderer& r, float W, float H, const Marges& m);
void directionLacher();             // toutes les commandes relâchées (changement d'écran)

// ---------- fonctions pures (tests/test_course.cpp) ----------
// zone de glissement : (x − x0) / (12 % de la largeur), bornée à ±1
inline float braquageGlisser(float x0, float x, float W) { return clampf((x - x0) / (W * 0.12f), -1.f, 1.f); }
// flèches : (droite ? 1 : 0) − (gauche ? 1 : 0)
inline float braquageFleches(bool gauche, bool droite) { return (droite ? 1.f : 0.f) - (gauche ? 1.f : 0.f); }
// boutons flèches (.arrow-btn : 76 px, 12 px d'écart, en bas à gauche) : carrés de côté c, centrés en y,
// le gauche en xg, le droit en xd (coins hauts-gauches) ; centrés sur la place du volant au repos
struct BoutonsFleches { float xg, xd, y, c; };
inline BoutonsFleches boutonsFleches(float W, float H, const Marges& m) {
    const float u = H / 100.f, c = 19.f * u, gap = 3.f * u, cx = W * 0.18f + m.gauche, cy = H * 0.72f - m.bas;
    return {cx - gap / 2 - c, cx + gap / 2, cy - c / 2, c};
}
// zone de toucher de la direction (les deux modes) : partie gauche de l'écran ; flèches : à gauche ou à droite du milieu
inline bool dansZoneDirection(float x, float W, const Marges& m) { return x < m.gauche + W * 0.45f; }
inline bool flecheDroite(float x, const BoutonsFleches& b) { return x >= b.xd - (b.xd - (b.xg + b.c)) / 2; }

} // namespace wfs
