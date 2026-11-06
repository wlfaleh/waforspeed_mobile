// WaForSpeed — pause en course (WFS-39) : voir pause.h.
#include "pause.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "../../game/commandes_tactiles.h"   // WFS-146 : dessin2d::biseau (un seul polygone)
#include "reglages.h"
#include "../../platform/manette.h"   // WFS-22

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// jetons :root du jeu web (codes sRGB)
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// zones touchables posées à la dernière image (bouton Menu de la course, 3 boutons de la pause), lues par le banc
Zone g_menu, g_boutons[3];
enum { REPRENDRE = 0, RECOMMENCER = 1, QUITTER = 2 };
const char* TEXTES[3] = {"Reprendre", "Recommencer", "Quitter la course"};   // index.html, #pause .actions

// parallélogramme (clip-path des .btn / .cam-btn) en bandes horizontales : haut décalé à droite de `biais`
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    // WFS-146 (RG-04, ANO-20) : un seul polygone (avant : 16 bandes translucides qui se chevauchaient → fines lignes sombres)
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}

// bouton « Menu » visible : course réelle en cours (décompte ou course), pas l'écran de fin, pas les scènes du banc sans règles
bool boutonVisible(const GameView& g) {
    const Course* c = activeCourse();
    return c && g.ecran == Ecran::Course && !g.sceneDediee && c->state != RaceState::Over && c->timeLeft < 1e6 && !manette::actif();   // WFS-22 : masqué à la manette
}

class Pause : public System {
public:
    const char* name() const override { return "pause"; }

    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        if (nouveau == Ecran::Pause) WFS_LOG("pause : course en pause (chrono %.2f s)", activeCourse() ? activeCourse()->timeLeft : 0.0);
        if (ancien == Ecran::Pause && nouveau == Ecran::Course) WFS_LOG("pause : reprise (chrono %.2f s)", activeCourse() ? activeCourse()->timeLeft : 0.0);
        // « Recommencer » : Pause → Menu → Course, appliqués avant l'image suivante (le menu n'est jamais dessiné) ;
        // l'entrée en course depuis le menu remet la course à zéro (même chemin que « Rejouer », WFS-38)
        if (ancien == Ecran::Pause && nouveau == Ecran::Menu && recommencer_) { recommencer_ = false; g.allerA(Ecran::Course); }
        if (nouveau != Ecran::Course) g_menu = {};
        if (nouveau != Ecran::Pause) for (Zone& z : g_boutons) z = {};
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran == Ecran::Course) {   // RG-01 / RG-02 : bouton « Menu » → pause (pauseGame)
            if (t.down && boutonVisible(g) && g_menu.contient(t.x, t.y)) { doigt_ = t.id; g.allerA(Ecran::Pause); return true; }
            return t.id == doigt_;        // le doigt du bouton ne pilote pas la voiture
        }
        if (g.ecran != Ecran::Pause || toucheReglages(t.x, t.y)) return false;   // WFS-68 : bouton et panneau des réglages
        if (!t.down) { if (t.id == doigt_) doigt_ = -1; return true; }
        if (t.id == doigt_) return true;   // le doigt qui a ouvert la pause est encore posé
        if (g_boutons[REPRENDRE].contient(t.x, t.y)) { WFS_LOG("pause : « Reprendre »"); g.allerA(Ecran::Course); }
        else if (g_boutons[RECOMMENCER].contient(t.x, t.y)) { WFS_LOG("pause : « Recommencer »"); recommencer_ = true; g.allerA(Ecran::Menu); }
        else if (g_boutons[QUITTER].contient(t.x, t.y)) { WFS_LOG("pause : « Quitter la course »"); g.allerA(Ecran::Menu); }
        return true;   // RG-02 : l'écran de pause garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        if (boutonVisible(g)) dessinerBouton(r, g, k);
        if (g.ecran == Ecran::Pause) dessinerPause(r, W, H, k);
    }

private:
    // .cam-btn#menuBtnHud : vitré, bordure 1 px, biais 8 px, 40 px de haut, icône ‖ 18 px + « MENU » 14 px orange.
    // RG-01 : en haut à gauche dans la zone sûre (marges de WFS-51), sous le panneau du temps (14 + 112 px de WFS-27) + 10 px
    void dessinerBouton(Renderer& r, const GameView& g, float k) {
        const float h = 40 * k, ic = 18 * k, pad = 14 * k, gap = 6 * k, tp = 14 * k;
        const std::string t = ui::majuscules("Menu");
        const float w = pad + ic + gap + ui::width(tp, t, 0.1f) + pad;
        const float x = g.marges.gauche + 16 * k, y = g.marges.haut + 14 * k + 112 * k + 10 * k;
        g_menu = {x, y, w, h};
        biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, 8 * k, BORD);   // border 1px var(--border)
        biseau(r, x, y, w, h, 8 * k, GLASS);
        const vec4 o = hex(ACCENT);
        const float ix = x + pad, iy = y + (h - ic) / 2;   // icône pause : deux barres (path M6 4h4v16H6zM14 4h4v16h-4z)
        r.uiRect(ix + ic * 6 / 24, iy + ic * 4 / 24, ic * 4 / 24, ic * 16 / 24, o);
        r.uiRect(ix + ic * 14 / 24, iy + ic * 4 / 24, ic * 4 / 24, ic * 16 / 24, o);
        ui::text(r, std::round(ix + ic + gap), std::round(y + (h - tp) / 2), tp, t, o, ui::GAUCHE, 0.1f, false);
    }

    // section #pause : voile, .eyebrow « Course en pause » (cyan), .big « Pause » (ANO-34), .actions : .btn + 2 × .btn--ghost
    void dessinerPause(Renderer& r, float W, float H, float k) {
        r.uiRect(0, 0, W, H, hex(BG, 0.82f));
        const float cx = W / 2;
        ui::text(r, cx, H * 0.24f, 14 * k, ui::majuscules("Course en pause"), hex(ACCENT2), ui::CENTRE, 0.22f, false);
        ui::text(r, cx, H * 0.30f, 64 * k, ui::majuscules("Pause"), hex(TEXTE), ui::CENTRE, 0.02f, false);
        // boutons de l'écran de fin (WFS-38 / WFS-57) : hauteur 10 % de H, biais 14 px, texte 5 % de H, espacement 0,12 em,
        // largeur = texte + 2 × 28 px ; rangée centrée, 2 % de H d'écart
        const float u = H / 100, bh = 10 * u, tp = 5 * u, gap = 2 * u;
        float wb[3], total = 2 * gap;
        for (int i = 0; i < 3; i++) { wb[i] = ui::width(tp, ui::majuscules(TEXTES[i]), 0.12f) + 56 * k; total += wb[i]; }
        float x = cx - total / 2;
        const float y = H * 0.56f;
        for (int i = 0; i < 3; i++) {
            g_boutons[i] = {x, y, wb[i], bh}; manette::focus(g_boutons[i], i == REPRENDRE);   // WFS-22 : B = reprendre
            biseau(r, x, y, wb[i], bh, 14 * k, i == REPRENDRE ? hex(ACCENT) : hex(SURFACE2));
            ui::text(r, x + wb[i] / 2, y + 2.6f * u, tp, ui::majuscules(TEXTES[i]), i == REPRENDRE ? hex(BG) : hex(TEXTE), ui::CENTRE, 0.12f, false);
            x += wb[i] + gap;
        }
    }

    bool recommencer_ = false;
    int doigt_ = -1;
};

} // namespace

// banc d'essai (WFS-39) : touchers simulés avec Game::onTouch, chrono relevé au journal
void benchPause(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("pause : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 900;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto chrono = [&]() { const Course* c = activeCourse(); return c ? c->timeLeft : 0.0; };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, chrono %.2f s, %s", nom, nomEcran(g.ecran()), chrono(), quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    for (int i = 0; i < 60 * 5; i++) image();   // décompte (3 s) puis 2 s de course
    // pause-bouton : encoche simulée de 90 px à gauche, bouton dans la zone sûre
    g.setMarges({90, 0, 0, 0}); g.montrerMarges = true;
    image(); capture("pause-bouton", "bouton « Menu » en course, encoche simulée de 90 px");
    g.setMarges({}); g.montrerMarges = false;
    g.render(t);   // zone du bouton reposée sans marge
    const double avant = chrono();
    toucher(1, g_menu);
    capture("pause-0", "écran de pause, 3 boutons");
    for (int i = 0; i < 120; i++) image();      // 2 s en pause : rien ne bouge
    const double pendant = chrono();
    g.render(t);
    toucher(2, g_boutons[REPRENDRE]);
    capture("pause-reprise", "reprise");
    WFS_LOG("pause : chrono avant la pause %.2f s, après 2 s de pause %.2f s, à la reprise %.2f s (%s)", avant, pendant, chrono(),
            std::fabs(pendant - avant) < 1e-9 ? "figé, identique" : "DIFFÉRENT");
    // Recommencer : nouvelle course (décompte, chrono 30 s)
    for (int i = 0; i < 60; i++) image();
    g.render(t); toucher(3, g_menu); g.render(t); toucher(4, g_boutons[RECOMMENCER]);
    for (int i = 0; i < 6; i++) image();
    capture("pause-recommencer", "nouvelle course après « Recommencer »");
    // Quitter la course → menu
    for (int i = 0; i < 60 * 4; i++) image();
    g.render(t); toucher(5, g_menu); g.render(t); toucher(6, g_boutons[QUITTER]);
    for (int i = 0; i < 60; i++) image();
    capture("pause-quitter", "retour au menu après « Quitter la course »");
    g.shutdown();
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Pause, 455)   // interface (400-499) : après le menu (450), avant le fondu du chargement (490)

} // namespace wfs
