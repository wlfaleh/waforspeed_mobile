// WaForSpeed — HUD de course (ticket WFS-27, épic E5, version réduite du MVP) : temps + checkpoint (haut gauche),
// score + combo (haut droite), vitesse + nitro (bas centre, entre le volant et le frein). Port de js/ui/hud.js (updateHud)
// et de css/style.css (.hud-top, .time-value, .cp-bar, .score-value, .combo, .nitro-bar) du jeu web.
// Système enfichable (ordre 420) ; lecture seule de la course (activeCourse) et du joueur (GameView).
#pragma once
#include <string>
#include "../core/system.h"   // Marges (WFS-429)

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
// panneau du score (haut droite) : SCORE_L × SCORE_H (SCORE_H_COMBO avec le combo), gouttières de WFS-27
constexpr float SCORE_L = 180.f, SCORE_H = 70.f, SCORE_H_COMBO = 92.f, GOUTTIERE_L = 16.f, GOUTTIERE_H = 14.f;
// WFS-429 (ordre de l'utilisateur du 2026-11-11, exception à l'ordre du 2026-11-06 sur la taille des boutons) : les boutons
// « Menu » (pause.cpp) et « Caméra » (camera_vue.cpp) de la course grandissent de 5 % exactement, eux seuls (RG-02)
constexpr float BOUTON_ECHELLE = 1.05f;
// .cam-btn avant WFS-429 : 40 px de haut, icône 18, marges 14, écart icône-texte 6, texte 14, biais 8 ; .hud-btns gap 8 px
constexpr float BOUTON_H = 40.f, BOUTON_ICONE = 18.f, BOUTON_MARGE = 14.f, BOUTON_ECART_ICONE = 6.f, BOUTON_TEXTE = 14.f,
                BOUTON_BIAIS = 8.f, BOUTONS_ECART = 8.f;
// un bouton dessiné : rectangle et mesures (pixels de l'interface, échelle k et BOUTON_ECHELLE comprises)
struct BoutonHud { float x = 0, y = 0, w = 0, h = 0, icone = 0, marge = 0, ecartIcone = 0, texte = 0, biais = 0; };
// RG-01 de WFS-429 : rangée [MENU] [CAMÉRA] [SCORE] en haut à droite, BOUTONS_ECART × k entre chaque, centrée en hauteur sur
// les SCORE_H px du score sans combo ; même place en poursuite (score absent) et pour un gaucher (le score ne bouge pas).
// `libelleCamera` = nom de la vue en capitales (camera_vue.cpp)
void rangeeBoutons(float W, float H, const Marges& m, const std::string& libelleCamera, BoutonHud& menu, BoutonHud& camera);
// mesures d'un bouton (RG-02 : chaque dimension × BOUTON_ECHELLE ; la bordure de 1 px reste à k) ; `largeurTexte` = ui::width du
// libellé à la taille `texte` (rangeeBoutons, hud.cpp) ; position à 0
inline BoutonHud mesuresBouton(float k, float largeurTexte) {
    const float e = k * BOUTON_ECHELLE;
    BoutonHud b;
    b.h = BOUTON_H * e; b.icone = BOUTON_ICONE * e; b.marge = BOUTON_MARGE * e; b.ecartIcone = BOUTON_ECART_ICONE * e;
    b.texte = BOUTON_TEXTE * e; b.biais = BOUTON_BIAIS * e;
    b.w = b.marge + b.icone + b.ecartIcone + largeurTexte + b.marge;
    return b;
}
// RG-01 : pose les deux boutons mesurés (w, h remplis) à gauche du score ; calcul seul (vérifié par tests/test_hud_boutons.cpp)
inline void placerBoutons(float W, float H, const Marges& m, BoutonHud& menu, BoutonHud& camera) {
    const float k = H / 720.f, ecart = BOUTONS_ECART * k;
    const float xScore = W - m.droite - GOUTTIERE_L * k - SCORE_L * k;         // bord gauche du score (hud.cpp)
    const float y = m.haut + GOUTTIERE_H * k + (SCORE_H * k - camera.h) / 2;    // centrés sur les 70 px du score sans combo
    camera.x = xScore - ecart - camera.w; camera.y = y;
    menu.x = camera.x - ecart - menu.w;   menu.y = y;
}
} // namespace hud
class Game;
class Renderer;
// banc d'essai : captures hud-course, hud-temps-bas, hud-nitro, hud-combo (dans outDir), puis g.shutdown()
void benchHud(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
