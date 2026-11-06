// WaForSpeed — bandeaux de notification pendant la course (ticket WFS-47, épic E5). Port de js/ui/toasts.js
// (toast(title, value, kind) : pile de 3, 1,9 s) et de css/style.css (.toasts, .toast, .cp, .bad) du jeu web.
// Système enfichable (ordre 425) : les évènements sont déduits des compteurs de la course (activeCourse) d'une mise à jour
// à l'autre — checkpoint (level), quasi-contact (nearCount, combo), accident (accidents), combo perdu (combo à 0 sur un choc).
#pragma once
#include <string>
#include "../core/system.h"   // Marges

namespace wfs {
class Game;
class Renderer;
// banc d'essai : charge sa propre course (golf, ville), captures toast-checkpoint et toast-pile (dans outDir), puis g.shutdown()
void benchToasts(Game& g, Renderer& r, const std::string& outDir);
// WFS-136 (accroche A4) : pousse un bandeau dans la pile (genre 0 normal, 1 checkpoint, 2 mauvais), affiché en course
namespace toasts { void pousser(const std::string& titre, const std::string& valeur, int genre); }
// RG-05 de WFS-47 : place de la pile (pixels de l'interface) ; WFS-173 : lue aussi par la mini-carte pour ne pas la recouvrir.
// Bord droit à gauche du bouton Nitro (cercle de 6,5 % de H en (0,90 W − marge droite ; 0,32 H), Game::drawControls) ;
// haut sous le score (gouttière + 110 px de référence)
inline float bandeauxBordDroit(float W, float H, const Marges& m) { return W * 0.90f - m.droite - 6.5f * (H / 100.f) - 12 * (H / 720.f); }
inline float bandeauxHaut(float H, const Marges& m) { return 14 * (H / 720.f) + m.haut + 110 * (H / 720.f); }
} // namespace wfs
