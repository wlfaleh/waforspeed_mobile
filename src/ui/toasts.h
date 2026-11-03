// WaForSpeed — bandeaux de notification pendant la course (ticket WFS-47, épic E5). Port de js/ui/toasts.js
// (toast(title, value, kind) : pile de 3, 1,9 s) et de css/style.css (.toasts, .toast, .cp, .bad) du jeu web.
// Système enfichable (ordre 425) : les évènements sont déduits des compteurs de la course (activeCourse) d'une mise à jour
// à l'autre — checkpoint (level), quasi-contact (nearCount, combo), accident (accidents), combo perdu (combo à 0 sur un choc).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// banc d'essai : charge sa propre course (golf, ville), captures toast-checkpoint et toast-pile (dans outDir), puis g.shutdown()
void benchToasts(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
