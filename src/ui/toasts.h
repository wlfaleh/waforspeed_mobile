// WaForSpeed — bandeaux de notification pendant la course (ticket WFS-47, épic E5). Port de js/ui/toasts.js
// (toast(title, value, kind) : pile de 3, 1,9 s) et de css/style.css (.toasts, .toast, .cp, .bad) du jeu web.
// Système enfichable (ordre 425) : les évènements sont déduits des compteurs de la course (activeCourse) d'une mise à jour
// à l'autre — checkpoint (level), quasi-contact (nearCount, combo), accident (accidents), combo perdu (combo à 0 sur un choc).
#pragma once
#include <algorithm>
#include <string>
#include "../core/system.h"   // Marges

namespace wfs {
class Game;
class Renderer;
// banc d'essai : charge sa propre course (golf, ville), captures toast-checkpoint et toast-pile (dans outDir), puis g.shutdown()
void benchToasts(Game& g, Renderer& r, const std::string& outDir);
// WFS-567 : banc « messages » (messages_banc.cpp) : checkpoint, série, Poursuite (départ, renfort, surprise), jour et nuit
int benchMessages(Game& g, Renderer& r, const std::string& outDir);
// WFS-136 (accroche A4) : pousse un message dans la pile (genre 0 gain, 1 checkpoint, 2 mauvais, 3 information), affiché en course
namespace toasts { void pousser(const std::string& titre, const std::string& valeur, int genre); }
// RG-05 de WFS-47 : place de la pile (pixels de l'interface) ; WFS-173 : lue aussi par la mini-carte pour ne pas la recouvrir.
// Bord droit à gauche du bouton Nitro (cercle de 6,5 % de H en (0,90 W − marge droite ; 0,32 H), Game::drawControls) ;
// haut sous le score (gouttière + 110 px de référence)
inline float bandeauxBordDroit(float W, float H, const Marges& m) { return W * 0.90f - m.droite - 6.5f * (H / 100.f) - 12 * (H / 720.f); }
inline float bandeauxHaut(float H, const Marges& m) { return 14 * (H / 720.f) + m.haut + 110 * (H / 720.f); }

// WFS-567 (ANO-315, ANO-290) : style et place des messages de course, partagés par la pile (toasts.cpp), le message de la
// Poursuite et le conseil de départ (portique_depart.cpp). Calcul seul : testé par tests/test_messages.cpp.
// WFS-567 (RG-02, RG-04, RG-05) : encart SCORE / POLICE de la Poursuite (en haut au centre, PoursuiteSystem::drawUi) : bas
// (pixels de l'interface, marge du haut comprise) et demi-largeur ; les messages et le conseil de départ ne passent jamais dessous
inline float poursuiteEncartBas(float H, float margeHaut) { return margeHaut + 17.5f * H / 100.f; }
inline float poursuiteEncartDemi(float H) { return 20.f * H / 100.f; }

namespace messages {
// RG-01 : texte clair sur fond sombre translucide, petit filet de couleur ; aucun cyan (0x00E5FF)
constexpr unsigned TEXTE = 0xF2F4F8, SOMBRE = 0x10131C, ACCENT = 0xFF6A00, DANGER = 0xFF1F4B;
constexpr float FOND_OPACITE = 0.72f;
enum class Repere { Gain, Danger, Info };   // gains (orange), accident / police / perte (rouge), information (blanc)
constexpr unsigned couleurRepere(Repere r) { return r == Repere::Gain ? ACCENT : r == Repere::Danger ? DANGER : TEXTE; }
constexpr int MAX_VISIBLES = 2;             // RG-02 : 2 messages au plus à la fois
// RG-03 : durée de vie et fondus (secondes)
constexpr double ENTREE = 0.18, SORTIE = 0.20, VIE = 2.0;
// mesures en pixels de l'interface, proportionnelles à la hauteur (720 px de référence)
inline float taillePolice(float H) { return 27 * (H / 720.f); }   // RG-03 : 3,75 % de H (≥ 3,5 %)
inline float hauteur(float H) { return 38 * (H / 720.f); }
inline float ecart(float H) { return 6 * (H / 720.f); }
inline float marge(float H) { return 14 * (H / 720.f); }             // espace intérieur de chaque côté du texte
inline float biais(float H) { return 8 * (H / 720.f); }              // bord gauche incliné (design street racing)
struct Rect { float x = 0, y = 0, w = 0, h = 0; };
// RG-02 : zone centrale de la route (voiture et route devant), où aucun message ne se pose
inline Rect zoneRoute(float W, float H) { return {W * 0.25f, H * 0.30f, W * 0.50f, H * 0.40f}; }
inline bool croise(const Rect& a, const Rect& b) { return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h; }
// Haut de la zone des messages : sous la rangée du haut du HUD ; en Poursuite, sous l'encart SCORE / POLICE (encartBas > 0)
inline float haut(float H, const Marges& m, float encartBas) { return std::max(bandeauxHaut(H, m), encartBas > 0 ? encartBas + H / 100.f : 0.f); }
// Place du message de rang 0 (en haut) ou 1, de largeur w. Course : centré. Poursuite (encartDemi > 0) : aligné à gauche, sous
// le chrono et à gauche de l'encart SCORE / POLICE (la raison des pertes s'écrit sous l'encart, au centre).
inline Rect place(float W, float H, const Marges& m, float encartBas, float encartDemi, int rang, float w) {
    const float y = haut(H, m, encartBas) + rang * (hauteur(H) + ecart(H));
    if (encartDemi > 0) return {m.gauche + 16 * (H / 720.f), y, w, hauteur(H)};
    const float xc = W * 0.5f + (m.gauche - m.droite) / 2;
    return {xc - w / 2, y, w, hauteur(H)};
}
// Largeur la plus grande permise : en Poursuite, jusqu'à 1 % de H avant l'encart ; sinon, entre la mini-carte (à droite) et
// son symétrique (à gauche)
inline float largeurMax(float W, float H, const Marges& m, float encartDemi) {
    const float xc = W * 0.5f + (m.gauche - m.droite) / 2;
    if (encartDemi > 0) return xc - encartDemi - H / 100.f - (m.gauche + 16 * (H / 720.f));
    return 2 * (bandeauxBordDroit(W, H, m) - xc);
}
// dessin (toasts.cpp) : fond seul (fond sombre translucide au bord gauche incliné, filet de couleur sur ce bord) ; message
// complet de rang 0 ou 1 (titre et valeur clairs en capitales italiques ; trop large : police réduite jusqu'à 3,5 % de H, puis
// valeur retirée)
void fond(Renderer& r, const Rect& q, float H, Repere rep, float op);
void dessiner(Renderer& r, const GameView& g, float encartBas, float encartDemi, int rang, const std::string& titre,
              const std::string& valeur, Repere rep, float op);
} // namespace messages
} // namespace wfs
