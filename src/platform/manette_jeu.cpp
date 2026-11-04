// WaForSpeed — manettes de jeu (WFS-22 / WFS-23) : branchement sur la partie, focus des menus, banc d'essai.
#include "manette_jeu.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>   // unsetenv
#include "manette.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/camera_vue.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"

namespace wfs {
namespace manette {
namespace {

Etat g_prec;                 // état à l'image précédente (fronts des boutons)
bool g_arme = true;          // réarmement du stick dans les menus
int g_focus = -1;            // bouton qui a le focus
Ecran g_ecranFocus = Ecran::Chargement;
size_t g_nbZones = 0;
constexpr int DOIGT = 9000;  // identifiant des touchers simulés (aucun doigt réel n'a cet identifiant)

void toucher(Game& g, const Zone& z) {
    const float x = z.x + z.w / 2, y = z.y + z.h / 2;
    g.onTouch({DOIGT, x, y, true});
    g.onTouch({DOIGT, x, y, false});
}

} // namespace

int focusCourant() { return g_focus; }

void appliquer(Game& g) {
    if (deconnexionRecue()) {   // RG-05 : la manette part en course → pause automatique
        WFS_LOG("manette : déconnectée (écran %s)", nomEcran(g.ecran()));
        if (g.ecran() == Ecran::Course) g.allerA(Ecran::Pause);
    }
    g.modeManette = actif();
    if (!actif()) { g.manette = DriveInput{}; g_prec = etat(); g_focus = -1; return; }
    const Etat& e = etat();
    auto front = [&](Bouton b) { return e.bouton[b] && !g_prec.bouton[b]; };
    const std::vector<Zone>& z = zonesPrecedentes();
    // nouvel écran (ou boutons apparus : écran de fin) : focus sur le premier bouton déclaré
    if (g.ecran() != g_ecranFocus || (g_nbZones == 0) != z.empty()) { g_focus = z.empty() ? -1 : 0; g_ecranFocus = g.ecran(); }
    g_nbZones = z.size();
    int dx, dy;
    directionMenu(e, g_prec, g_arme, dx, dy);
    if (!z.empty()) {   // RG-04 : menus
        g_focus = std::clamp(g_focus, 0, (int)z.size() - 1);
        if (dx || dy) g_focus = voisin(z, g_focus, dx, dy);
        g.manette = DriveInput{};
        if (front(A)) { WFS_LOG("manette : A sur le bouton %d (écran %s)", g_focus, nomEcran(g.ecran())); toucher(g, z[g_focus]); }
        else if (front(B)) {
            const auto r = std::find_if(z.begin(), z.end(), [](const Zone& q) { return q.retour; });
            if (r != z.end()) { WFS_LOG("manette : B (retour, écran %s)", nomEcran(g.ecran())); toucher(g, *r); }
        } else if (front(START) && g.ecran() == Ecran::Pause) g.allerA(Ecran::Course);
    } else if (g.ecran() == Ecran::Course) {   // RG-01 / RG-02 : conduite
        conduite(e, g.manette);
        if (front(Y)) { const VueCamera v = vueSuivante(); WFS_LOG("manette : vue %s", VUES[(int)v].nom); }
        if (front(START)) { WFS_LOG("manette : Start → pause"); g.allerA(Ecran::Pause); }
    } else {
        g.manette = DriveInput{};
        if (front(START) && g.ecran() == Ecran::Pause) g.allerA(Ecran::Course);
    }
    g_prec = e;
}

namespace {
// focus visible (RG-04) : cadre cyan (--accent-2) avec halo autour du bouton choisi, par-dessus les écrans (ordre 489)
class FocusManette : public System {
public:
    const char* name() const override { return "manette"; }
    void drawUi(Renderer& r, const GameView& g) override {
        const std::vector<Zone>& z = zonesImage();
        if (actif() && g_focus >= 0 && g_focus < (int)z.size()) {
            const Zone& b = z[g_focus];
            const float k = g.uiHeight / 720.f, e = 3 * k;
            for (int h = 3; h >= 1; h--) {   // halo : 0 0 12px rgba(0, 229, 255, .7)
                const float m = e + 3 * k * h;
                cadre(r, b.x - m, b.y - m, b.w + 2 * m, b.h + 2 * m, 3 * k, {0, 0.898f, 1, 0.12f});
            }
            cadre(r, b.x - e, b.y - e, b.w + 2 * e, b.h + 2 * e, e, {0, 0.898f, 1, 1});
        }
        finImage();
    }
private:
    static void cadre(Renderer& r, float x, float y, float w, float h, float e, const vec4& c) {
        r.uiRect(x, y, w, e, c); r.uiRect(x, y + h - e, w, e, c);
        r.uiRect(x, y + e, e, h - 2 * e, c); r.uiRect(x + w - e, y + e, e, h - 2 * e, c);
    }
};
} // namespace

// ---------------------------------------------------------------- banc d'essai
void benchManette(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);   // vraie course : décompte, chrono, HUD (boutons tactiles du HUD masqués à la manette)
    double t = 1200;
    int n = 0;
    auto image = [&](int rendu = 10) { appliquer(g); g.update(1.0 / 60); t += 1.0 / 60; if (++n % rendu == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, focus %d, %s", nom, nomEcran(g.ecran()), g_focus, quoi);
    };
    auto appui = [&](Bouton b) { g.render(t); bouton(b, true); image(); bouton(b, false); for (int i = 0; i < 20; i++) image(); };
    // stick X brut qui donne la consigne s après la zone morte (inverse de zoneMorte)
    auto stick = [](float s) { return s == 0 ? 0.f : std::copysign(ZONE_MORTE + std::fabs(s) * (1 - ZONE_MORTE), s); };

    // 1) course automatique de référence (pilote automatique), puis la même conduite donnée par la manette :
    //    gâchette droite à fond, stick gauche = consigne du pilote automatique (milieu de la voie)
    const int PAS = 1200;   // 20 s : décompte, puis ligne droite et premiers virages
    if (!g.init(r, "golf", "ville")) { WFS_ERR("manette : chargement impossible"); return; }
    g.setAutopilot(true); g.camMode = 0;
    for (int i = 0; i < PAS; i++) g.update(1.0 / 60);
    const double dAuto = g.player().dist * M, vAuto = g.player().kmh();
    g.shutdown();
    if (!g.init(r, "golf", "ville")) return;
    g.setAutopilot(false); g.camMode = 0;
    float sMax = 0;
    for (int i = 0; i < PAS; i++) {
        const Player& p = g.player();
        const float s = std::clamp((float)((0.0 - p.playerX) * 3.0 - p.heading * 4.0), -1.f, 1.f);
        sMax = std::max(sMax, std::fabs(s));
        axes(stick(s), 0, 0, 1, 0, 0);
        image(i > PAS - 40 ? 8 : 1000000);
    }
    const double dMan = g.player().dist * M, vMan = g.player().kmh();
    WFS_LOG("manette : course automatique %.0f m %.0f km/h ; même course à la manette %.0f m %.0f km/h (écart %.1f %%, braquage max %.2f, mode manette %s)",
            dAuto, vAuto, dMan, vMan, 100 * std::fabs(dMan - dAuto) / std::max(1.0, dAuto), sMax, g.modeManette ? "oui" : "non");
    capture("manette-course", "course à la manette : aucune commande tactile affichée");
    // 2) Y : vue suivante (capot), Y Y : retour à la poursuite
    appui(Y); appui(Y); appui(Y);
    // 3) Start : pause avec le focus sur « Reprendre » ; croix à droite : « Recommencer »
    axes(0, 0, 0, 0, 0, 0);
    appui(START);
    for (int i = 0; i < 20; i++) image(1);
    capture("manette-pause", "pause ouverte par Start, focus sur « Reprendre »");
    bouton(DROITE, true); image(1); bouton(DROITE, false); for (int i = 0; i < 5; i++) image(1);
    WFS_LOG("manette : croix à droite dans la pause → focus %d", g_focus);
    bouton(GAUCHE, true); image(1); bouton(GAUCHE, false); for (int i = 0; i < 5; i++) image(1);
    appui(A);
    WFS_LOG("manette : A sur « Reprendre » → écran %s", nomEcran(g.ecran()));
    // 4) manette débranchée en course → pause automatique ; un toucher de l'écran rend la main au tactile
    deconnexion(); image(1); image(1);
    WFS_LOG("manette : débranchée en course → écran %s, mode manette %s", nomEcran(g.ecran()), g.modeManette ? "oui" : "non");
    g.allerA(Ecran::Course); for (int i = 0; i < 5; i++) image(1);
    // 5) menu principal : focus sur « Course solo », puis croix vers le bas (« Duel en ligne »), puis retour
    bouton(START, true); image(1); bouton(START, false); image(1);   // la manette reprend la main (pause)
    g.allerA(Ecran::Menu); for (int i = 0; i < 70; i++) image(1);
    capture("manette-menu-focus", "menu principal, focus visible sur « Course solo »");
    bouton(BAS, true); image(1); bouton(BAS, false); image(1);
    WFS_LOG("manette : croix en bas dans le menu → focus %d", g_focus);
    bouton(HAUT, true); image(1); bouton(HAUT, false); image(1);
    // 6) A sur « Course solo » → garage (focus sur « Choisir ») ; B → retour au menu ; A, A → terrain ; A → course
    appui(A); for (int i = 0; i < 30; i++) image(1);
    capture("manette-garage-focus", "garage ouvert par A, focus sur « Choisir »");
    appui(B);
    WFS_LOG("manette : B dans le garage → écran %s", nomEcran(g.ecran()));
    for (int i = 0; i < 40; i++) image(1);
    appui(A); for (int i = 0; i < 30; i++) image(1);
    appui(A); for (int i = 0; i < 30; i++) image(1);
    capture("manette-terrain-focus", "choix du terrain, focus sur « Rouler »");
    appui(A); for (int i = 0; i < 30; i++) image(1);
    WFS_LOG("manette : A sur « Rouler » → écran %s", nomEcran(g.ecran()));
    // 7) retour au tactile : un toucher de l'écran → commandes tactiles de nouveau affichées
    toucherEcran(); image(1);
    WFS_LOG("manette : toucher de l'écran → mode manette %s", g.modeManette ? "oui" : "non");
    g.shutdown();
    courseBenchRules(false);
    unsetenv("WFS_MOMENT");   // choisi à l'écran Terrain (WFS-41)
}

} // namespace manette

WFS_SYSTEM(manette::FocusManette, 489)   // interface (400-499) : par-dessus les écrans, sous le fondu du chargement (490)

} // namespace wfs
