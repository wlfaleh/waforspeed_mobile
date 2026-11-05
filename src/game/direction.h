// WaForSpeed — modes de direction tactiles (ticket WFS-70, épic E3). Port de js/ui/screens.js du jeu web
// (setSteer, setSteerMode, #steerArrows) :
// - « zone de glissement » (par défaut) : pouce posé dans la partie gauche de l'écran puis glissé (volant analogique,
//   ±12 % de la largeur = butée) ; comportement et dessin identiques à ceux d'avant WFS-70 (code déplacé de Game) ;
// - « flèches » : deux boutons à gauche (flèche gauche, flèche droite) ; consigne −1 / +1 tant qu'on appuie (input.left /
//   input.right du jeu web), 0 au relâchement : Player::update fait monter et revenir le braquage progressivement.
// Le mode se lit dans reglagesRessenti().direction (ressenti.h).
// WFS-108 : gyroscope (incliner le téléphone comme un volant), combiné avec le mode tactile choisi : tant qu'aucun doigt ne
// braque, la consigne vient de l'inclinaison ; réétalonné (zéro = position du moment) à chaque changement d'écran, donc au
// départ de chaque course et à chaque reprise. Flèches agrandies en dp (uniteCommandes, commandes_tactiles.h).
#pragma once
#include <cmath>
#include "../core/math.h"
#include "../core/system.h"
#include "commandes_tactiles.h"   // WFS-108 : uniteCommandes (dp)
#include "../platform/gyroscope.h"   // WFS-108 : inclinaison publiée par la plateforme
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
// boutons flèches (.arrow-btn : 76 dp, 12 dp d'écart, en bas à gauche) : carrés de côté c, centrés en y,
// le gauche en xg, le droit en xd (coins hauts-gauches) ; centrés sur la place du volant au repos, jamais à moins de 16 dp
// du bord gauche ni de 14 dp du bas (WFS-108 : taille en dp, × 1,25 en « Boutons : Grands »)
struct BoutonsFleches { float xg, xd, y, c; };
inline BoutonsFleches boutonsFleches(float W, float H, const Marges& m) {
    const float u = uniteCommandes(W, H), c = std::ceil(76.f * u), gap = std::round(12.f * u);
    const float cx = std::max(W * 0.18f + m.gauche, m.gauche + 16 * u + c + gap / 2);
    const float cy = std::min(H * 0.72f - m.bas, H - m.bas - 14 * u - c / 2);
    return {cx - gap / 2 - c, cx + gap / 2, cy - c / 2, c};
}

// ---------- WFS-108 : gyroscope ----------
// réglages : niveau 0 = coupé, 1 doux, 2 moyen, 3 fort (inclinaison pour braquer à fond : 28°, 20°, 14°) ;
// zone morte 0 petite, 1 moyenne, 2 grande (1,5°, 3°, 5°). Mémorisés dans la sauvegarde (clés gyro, gyroZone).
struct ReglagesGyro { int niveau = 0, zone = 1; };
inline ReglagesGyro& reglagesGyro() { static ReglagesGyro g; return g; }
constexpr float GYRO_PLEIN[4] = {0, 28, 20, 14}, GYRO_ZONE[3] = {1.5f, 3, 5};
// consigne −1..1 : écart d'angle (degrés, ramené entre −180 et 180) entre l'inclinaison et le zéro, sans la zone morte,
// rapporté à l'inclinaison de braquage à fond. Tourner le téléphone dans le sens des aiguilles d'une montre = droite (+).
inline float braquageGyro(float angle, float zero, float pleinDeg, float zoneDeg) {
    float d = (angle - zero) * 180.f / PI;
    while (d > 180) d -= 360;
    while (d < -180) d += 360;
    const float a = std::fabs(d);
    if (a <= zoneDeg || pleinDeg <= zoneDeg) return 0.f;
    return clampf((d > 0 ? 1.f : -1.f) * (a - zoneDeg) / (pleinDeg - zoneDeg), -1.f, 1.f);
}
// réglage différent de « coupé » et capteur disponible
inline bool gyroscopeActif() { return reglagesGyro().niveau > 0 && gyroscope::etat().capteur; }
// zéro de l'inclinaison (position du téléphone au départ de la course)
struct ZeroGyro { bool pret = false; float angle = 0; };
inline ZeroGyro& zeroGyro() { static ZeroGyro z; return z; }
// réétalonnage : la position du moment devient le zéro (directionLacher, à chaque changement d'écran)
inline void gyroscopeEtalonner() { const gyroscope::Etat& e = gyroscope::etat(); zeroGyro() = {e.valide, e.angle}; }
// consigne du gyroscope : 0 si coupé, sans angle, ou pas encore étalonné (étalonnage à la première valeur reçue)
inline float braquageIncline() {
    const gyroscope::Etat& e = gyroscope::etat();
    if (!gyroscopeActif() || !e.valide) return 0.f;
    if (!zeroGyro().pret) { zeroGyro() = {true, e.angle}; return 0.f; }
    const ReglagesGyro& R = reglagesGyro();
    return braquageGyro(e.angle, zeroGyro().angle, GYRO_PLEIN[R.niveau < 1 ? 1 : (R.niveau > 3 ? 3 : R.niveau)],
                        GYRO_ZONE[R.zone < 0 ? 0 : (R.zone > 2 ? 2 : R.zone)]);
}
// zone de toucher de la direction (les deux modes) : partie gauche de l'écran ; flèches : à gauche ou à droite du milieu
inline bool dansZoneDirection(float x, float W, const Marges& m) { return x < m.gauche + W * 0.45f; }
inline bool flecheDroite(float x, const BoutonsFleches& b) { return x >= b.xd - (b.xd - (b.xg + b.c)) / 2; }

} // namespace wfs
