// WaForSpeed — HUD de course (ticket WFS-27, épic E5, version réduite du MVP) : temps + checkpoint (haut gauche),
// score + combo (haut droite), vitesse + nitro (bas centre, entre le volant et le frein). Port de js/ui/hud.js (updateHud)
// et de css/style.css (.hud-top, .time-value, .cp-bar, .score-value, .combo, .nitro-bar) du jeu web.
// Système enfichable (ordre 420) ; lecture seule de la course (activeCourse) et du joueur (GameView).
#pragma once
#include <string>

namespace wfs {
namespace hud {
// WFS-245 (arbitrage du PO du 2026-11-07 17:30 UTC ; vérifié par tests/test_hud_lisible.cpp) ; tailles en px de référence (720 p,
// × k = hauteur / 720) :
// RG-04 : panneau du temps en haut à gauche : largeur au moins TEMPS_L (plus si le texte le demande), hauteur TEMPS_H (avant :
//         200 × 112), marge intérieure ; lu aussi par la carte (minicarte.h) ; score : 180 × 70 (92 avec le combo), inchangé
constexpr float TEMPS_L = 236.f, TEMPS_H = 84.f, TEMPS_MARGE = 12.f, AVANT_TEMPS_L = 200.f, AVANT_TEMPS_H = 112.f;
// RG-01 : capitales et chiffres de Rajdhani Bold = CAPITALES × la taille passée à ui::text (643 / 1 276 unités), ligne de base à
//         BASE × la taille sous le haut de la boîte ; étiquettes 27 (avant : 11, porté à 13), valeur d'ARRIVÉE 40, chiffres du
//         score 40 (avant 34), vitesse de la vue Cockpit 40 (avant 28), chrono 48 et vitesse du compteur 44 (inchangés)
constexpr float CAPITALES = 0.504f, BASE = 0.7288f;
constexpr float ETIQUETTE_PX = 27.f, ARRIVEE_PX = 40.f, SCORE_PX = 40.f, VITESSE_COCKPIT_PX = 40.f, CHRONO_PX = 48.f;
// contraste ≥ 4,5 : 1 au pire (fond blanc derrière le panneau ; mélange en sRGB : image d'échange UNORM, vk.cpp) : étiquettes
// ETIQUETTE sur FOND à FOND_ALPHA (5,9 : 1) ; dans le compteur, ETIQUETTE_ARC sur son disque (DISQUE à DISQUE_ALPHA, avant 0,55),
// devant du blanc (voiture blanche derrière) : #D0D4DE (4,8 : 1) plutôt que le #C4C9D6 proposé (4,3 : 1 sur du blanc)
constexpr unsigned ETIQUETTE = 0xA6ADC2, ETIQUETTE_ARC = 0xD0D4DE, FOND = 0x0A0B10, DISQUE = 0x14161F;
constexpr float FOND_ALPHA = 0.85f, DISQUE_ALPHA = 0.72f;
} // namespace hud
class Game;
class Renderer;
// banc d'essai : captures hud-course, hud-temps-bas, hud-nitro, hud-combo (dans outDir), puis g.shutdown()
void benchHud(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
