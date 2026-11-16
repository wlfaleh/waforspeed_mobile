// WaForSpeed — pause en course (WFS-39) : voir pause.h.
#include "pause.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/camera_vue.h"   // WFS-429 : nom de la vue (largeur du bouton Caméra, même rangée)
#include "../../game/course_system.h"
#include "../../game/course5.h"   // WFS-557 : mode en cours (sur-titre de la pause)
#include "../../game/track.h"     // WFS-557 : nom de la carte (MapDef)
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../hud.h"    // WFS-429 : rangée des boutons à côté du score
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../verre.h"   // WFS-548 (R19) : panneau de verre, sur-titre gris, fond flouté
#include "../langue.h"   // WFS-505 : textes de la pause en français ou en anglais
#include "textes_banc.h"   // WFS-505 : banc « textes » (journal des textes de cet écran)
#include "../../game/commandes_tactiles.h"   // WFS-146 : dessin2d::biseau (un seul polygone)
#include "reglages.h"
#include "../../platform/manette.h"   // WFS-22

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// jetons :root du jeu web (codes sRGB)
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, ACCENT = 0xFF6A00;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// zones touchables posées à la dernière image (bouton Menu de la course, 3 boutons de la pause), lues par le banc
Zone g_menu, g_boutons[3];
enum { REPRENDRE = 0, RECOMMENCER = 1, QUITTER = 2 };
// index.html, #pause .actions ; WFS-505 (RG-01) : clés des tables de textes (« Reprendre », « Recommencer », « Quitter la course »)
const char* TEXTES[3] = {"pause.reprendre", "pause.recommencer", "pause.quitter"};

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
        if (g.ecran != Ecran::Pause || toucheReglages(t.x, t.y, t.id)) return false;   // WFS-68 : bouton et panneau des réglages
        if (!t.down) { if (t.id == doigt_) doigt_ = -1; return true; }
        if (t.id == doigt_) return true;   // le doigt qui a ouvert la pause est encore posé
        if (g_boutons[REPRENDRE].contient(t.x, t.y)) { WFS_LOG("pause : « Reprendre »"); g.allerA(Ecran::Course); }
        else if (g_boutons[RECOMMENCER].contient(t.x, t.y)) { WFS_LOG("pause : « Recommencer »"); recommencer_ = true; g.allerA(Ecran::Menu); }
        else if (g_boutons[QUITTER].contient(t.x, t.y)) { WFS_LOG("pause : « Quitter la course »"); g.allerA(Ecran::Menu); }
        return true;   // RG-02 : l'écran de pause garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady()) return;
        const textes::Journal journal;   // WFS-505 : textes notés au banc « textes »
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        if (boutonVisible(g)) dessinerBouton(r, g, k);
        if (g.ecran == Ecran::Pause) dessinerPause(r, g, W, H, k);
    }

private:
    // .cam-btn#menuBtnHud : vitré, bordure 1 px, biais, icône ‖ + « MENU » orange.
    // WFS-429 (RG-01, RG-02) : en haut à droite, à gauche du bouton Caméra et du score ([MENU] [CAMÉRA] [SCORE]), +5 %
    // (place et mesures : hud::rangeeBoutons, la même que pour le bouton Caméra)
    void dessinerBouton(Renderer& r, const GameView& g, float k) {
        hud::BoutonHud b, cam;
        hud::rangeeBoutons(g.uiWidth, g.uiHeight, g.marges, ui::majuscules(VUES[(int)vueCamera()].nom), b, cam);
        const std::string t = ui::majuscules(ui::tr("pause.menu"));   // WFS-505
        const float x = b.x, y = b.y, w = b.w, h = b.h, ic = b.icone;
        g_menu = {x, y, w, h};
        biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, b.biais, BORD);   // border 1px var(--border)
        biseau(r, x, y, w, h, b.biais, GLASS);
        const vec4 o = hex(ACCENT);
        const float ix = x + b.marge, iy = y + (h - ic) / 2;   // icône pause : deux barres (path M6 4h4v16H6zM14 4h4v16h-4z)
        r.uiRect(ix + ic * 6 / 24, iy + ic * 4 / 24, ic * 4 / 24, ic * 16 / 24, o);
        r.uiRect(ix + ic * 14 / 24, iy + ic * 4 / 24, ic * 4 / 24, ic * 16 / 24, o);
        ui::text(r, std::round(ix + ic + b.ecartIcone), std::round(y + (h - b.texte) / 2), b.texte, t, o, ui::GAUCHE, 0.1f, false);
    }

    // WFS-557 (maquette 19, arbitrage du PO du 2026-11-16 : maquette à 100 %, boutons compris) : course floutée derrière,
    // fenêtre de verre 830 × 390 centrée ; sur-titre « Course en pause · <mode> · <carte> » ; « PAUSE » ; ligne de stats
    // (temps, checkpoint, points) ; « Reprendre » rond (Start/Stop) au centre ; « Recommencer » ↺ et « Quitter la course »
    // (bord rouge) en boutons de verre en dessous. Mesures relevées sur la maquette (1600 × 720, unité k = H / 720).
    void dessinerPause(Renderer& r, const GameView& g, float W, float H, float k) {
        verre::fondFloute(r);
        verre::assombrir(r, W, H, 0.38f);
        if (reglagesOuverts()) return;   // les Réglages ouverts par-dessus : leur fenêtre de verre seule (la pause se voyait au travers)
        const float cx = W / 2;
        const float pw = 830 * k, ph = 390 * k, py = 165 * k;
        verre::panneau(r, cx - pw / 2, py, pw, ph, {18 * k, 1.5f * k});
        // sur-titre : « Course en pause · Checkpoint · Ville néon »
        std::string sur = ui::tr("pause.surtitre");
        if (const char* mode = cleMode()) sur += " · " + ui::tr(mode);
        if (g.map && g.map->name) sur += std::string(" · ") + g.map->name;
        verre::surTitre(r, cx, 193 * k, 16 * k, sur, ui::CENTRE);
        ui::text(r, cx, 206 * k, 82 * k, ui::majuscules(ui::tr("pause.titre")), hex(TEXTE), ui::CENTRE, 0.01f, false, true);
        dessinerStats(r, g, cx, 314 * k, k);
        // boutons : zones de toucher = boutons dessinés (≥ 44 px), aucun chevauchement
        const float d = 128 * k;
        g_boutons[REPRENDRE] = {cx - d / 2, 390 * k - d / 2, d, d};
        const float bh = 51 * k, by = 470 * k, gap = 18 * k, w1 = 211 * k, w2 = 256 * k, x1 = cx - (w1 + gap + w2) / 2 + 2 * k;
        g_boutons[RECOMMENCER] = {x1, by, w1, bh};
        g_boutons[QUITTER] = {x1 + w1 + gap, by, w2, bh};
        for (int i = 0; i < 3; i++) manette::focus(g_boutons[i], i == REPRENDRE);   // WFS-22 : B = reprendre
        const Zone& z = g_boutons[REPRENDRE];
        ui::boutons::startStop(r, z.x, z.y, z.w, z.h, ui::majuscules(ui::tr(TEXTES[REPRENDRE])), 1, false, "", 22 * k);
        const float tp = 22 * k;
        ui::boutons::verreIcone(r, x1, by, w1, bh, ui::boutons::Icone::Recommencer, ui::majuscules(ui::tr(TEXTES[RECOMMENCER])), 1, false, tp,
                                {12 * k, 0, nullptr, 0.05f});
        const vec4 rouge = verre::hexa(verre::PERTE);
        ui::boutons::verreIcone(r, x1 + w1 + gap, by, w2, bh, ui::boutons::Icone::Sortie, ui::majuscules(ui::tr(TEXTES[QUITTER])), 1, false, tp,
                                {12 * k, verre::PERTE, &rouge, 0.05f});
    }

    // clé du mode en cours (sur-titre) : Checkpoint, Course à 5, Poursuite ; duel : rien
    static const char* cleMode() {
        switch (modeCourse()) {
            case ModeCourse::Checkpoint: return "mode.checkpoint";
            case ModeCourse::Course5: return "mode.course5";
            case ModeCourse::Poursuite: return "mode.poursuite";
            default: return nullptr;
        }
    }

    // ligne de stats de la maquette 19 : « 0:57,9 temps   CP 2   1 045 pts » ; chiffres en Rajdhani italique, unités en Inter grise,
    // posées sur la même ligne de base ; le mode Checkpoint est sans fin : le checkpoint s'écrit sans total (CP n)
    void dessinerStats(Renderer& r, const GameView& g, float cx, float base, float k) {
        const Course* c = activeCourse();
        if (!c || !g.player) return;
        const bool ligne = c->duelFinish > 0 || modeCourse() == ModeCourse::Poursuite;
        const std::string temps = ui::chrono(ligne ? c->raceTime : std::max(0.0, c->timeLeft));   // « 0:57,9 » (maquette 19)
        char cp[24]; std::snprintf(cp, sizeof cp, "CP %d", c->level + 1);
        const std::string pts = ui::nombre(c->score(*g.player));
        struct Bloc { std::string chiffre, unite; } blocs[3] = {{temps, ui::tr("pause.temps")}, {ligne ? "" : cp, ""}, {pts, ui::tr("pause.pts")}};
        const float cpx = 32 * k, upx = 15 * k, ecartU = 4 * k, ecartB = 36 * k;
        // hauteurs : haut du texte = ligne de base − capitales (Rajdhani ≈ 0,72 px du haut de ligne, Inter ≈ 0,76)
        float total = 0, lw[3][2] = {};
        int n = 0;
        for (auto& b : blocs) {
            if (b.chiffre.empty()) continue;
            lw[n][0] = ui::width(cpx, b.chiffre, 0);
            { ui::AvecPolice p(ui::Police::Inter); lw[n][1] = b.unite.empty() ? 0 : ui::width(upx, b.unite, 0) + ecartU; }
            total += lw[n][0] + lw[n][1] + (n ? ecartB : 0);
            n++;
        }
        float x = cx - total / 2;
        int i = 0;
        for (auto& b : blocs) {
            if (b.chiffre.empty()) continue;
            ui::text(r, x, base - 0.72f * cpx, cpx, b.chiffre, hex(0xE6E8EE), ui::GAUCHE, 0, false, true);
            x += lw[i][0];
            if (!b.unite.empty()) { ui::AvecPolice p(ui::Police::Inter); ui::text(r, x + ecartU, base - 0.76f * upx, upx, b.unite, hex(0xC3C7D1), ui::GAUCHE, 0, false); }
            x += lw[i][1] + ecartB;
            i++;
        }
    }

    bool recommencer_ = false;
    int doigt_ = -1;
};

} // namespace

// WFS-505 (banc « textes ») : centre d'un bouton à la dernière image : « menu » (en course), « reprendre », « recommencer », « quitter »
bool pauseBouton(const std::string& nom, float& x, float& y) {
    const Zone* z = nom == "menu" ? &g_menu : nom == "reprendre" ? &g_boutons[REPRENDRE] : nom == "recommencer" ? &g_boutons[RECOMMENCER]
                  : nom == "quitter" ? &g_boutons[QUITTER] : nullptr;
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2; return true;
}

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
