// WaForSpeed — menu principal (WFS-37) et ses appuis (WFS-38) : voir menu.h.
#include "paquets.h"   // WFS-306 (accroche) : proposition des paquets ouverte depuis le bandeau
#include "menu.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/sauvegarde.h"   // WFS-24
#include "../../game/course5.h"   // WFS-110 : choix du mode
#include "../../render/renderer.h"
#include "../text.h"
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)
#include "../../game/direction.h"   // WFS-146 (RG-05) : réglages Direction et Gyroscope, dessin2d
#include "chargement.h"
#include "reglages.h"
#include "terrain.h"
#include "../garage.h"   // WFS-11
#include "../credits.h"   // WFS-406 (accroche E8) : bouton et écran « Crédits »
#include "../../platform/manette.h"   // WFS-22 : focus de la manette

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
               INTRO = 0xC9CDDA;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};

// cubic-bezier(.2, .9, .1, 1) (--ease du web), résolu par dichotomie sur x
float ease(float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto bz = [](float a, float b, float s) { const float u = 1 - s; return 3 * u * u * s * a + 3 * u * s * s * b + s * s * s; };
    float lo = 0, hi = 1;
    for (int i = 0; i < 20; i++) { const float m = (lo + hi) / 2; (bz(0.2f, 0.1f, m) < t ? lo : hi) = m; }
    return bz(0.9f, 1.f, (lo + hi) / 2);
}

struct Choix { const char* titre; const char* desc; };
enum { SOLO = 0, DUEL = 1, SHOWROOM = 2, TA_VOITURE = 3, CREDITS = 4 };   // WFS-96 : 4e zone = « Ta voiture » (pied du menu) ; WFS-406 : bouton « Crédits »
// zones touchables des 3 choix (RG-02) : les parallélogrammes dessinés à la dernière image (x, y, largeur, hauteur, biais)
struct Zone { float x = 0, y = 0, w = 0, h = 0, biais = 0; bool ok = false; };
Zone g_zones[4];
long long g_best = 0;   // record de la session : gardé quand la partie est reconstruite sur une autre carte (WFS-41)
bool dansZone(const Zone& z, float x, float y) {
    if (!z.ok || y < z.y || y > z.y + z.h || x < z.x) return false;
    return x <= z.x + z.w - z.biais * (y - z.y) / z.h;   // coin bas droit coupé (clip-path)
}
int choixSous(float x, float y) { for (int i = 0; i < 4; i++) if (dansZone(g_zones[i], x, y)) return i; return -1; }
const Choix CHOIX[3] = {   // section #menu de index.html, textes exacts
    {"Course solo", "Bats le chrono, frôle le trafic, enchaîne les checkpoints"},
    {"Duel en ligne", "Défie un ami où qu'il soit, sans trafic"},   // WFS-135 (ANO-63)
    {"Showroom", "Choisis ta voiture et sa couleur"},
};

class Menu : public System {
public:
    const char* name() const override { return "menu"; }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        if (nouveau == Ecran::Menu) t0_ = -1;
        presse_ = -1; doigt_ = -1;
        for (Zone& z : g_zones) z.ok = false;
    }
    // RG-01 : appui = état pressé ; l'action part au relâcher sur le même choix, glisser hors du choix l'annule.
    // RG-05 : sur le menu, tout toucher est gardé (aucune commande de conduite).
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran == Ecran::Menu && credits::ouvert()) { doigt_ = -1; presse_ = -1; return credits::toucher(t.id, t.x, t.y, t.down); }   // WFS-406 : écran Crédits par-dessus
        if (g.ecran != Ecran::Menu || toucheReglages(t.x, t.y, t.id) || toucheBandeauPaquets(t.x, t.y)) return false;   // WFS-68 : bouton et panneau des réglages (WFS-290 : et le doigt posé sur « Réglages ») ; WFS-306 : proposition des paquets
        if (choixModeOuvert()) { doigt_ = -1; presse_ = -1; return false; }   // WFS-110 : panneau des modes par-dessus
        int sous = choixSous(t.x, t.y);
        if (sous < 0 && credits::dansBouton(t.x, t.y)) sous = CREDITS;   // WFS-406
        if (t.down) {
            if (doigt_ < 0) { doigt_ = t.id; presse_ = sous; }   // WFS-83 : « Duel en ligne » actif (plus grisé)
            else if (t.id == doigt_ && sous != presse_) presse_ = -1;               // glissé hors du choix : annulé
            return true;
        }
        if (t.id != doigt_) return true;
        const int choix = presse_;
        doigt_ = -1; presse_ = -1;
        if (choix < 0 || sous != choix) return true;
        if (choix == SOLO) { WFS_LOG("menu : « Course solo »"); if (!ouvrirChoixMode()) g.allerA(Ecran::Garage); }   // WFS-110 : modes             // RG-03 ; WFS-11 : garage d'abord
        else if (choix == DUEL) { WFS_LOG("menu : « Duel en ligne »"); modeCourse() = ModeCourse::Duel; g.allerA(Ecran::Salon); }   // WFS-83 : écran Salon
        else if (choix == SHOWROOM) { WFS_LOG("menu : « Showroom »"); garageRegarder(); g.allerA(Ecran::Garage); }   // WFS-96 (RG-01)
        else if (choix == CREDITS) { WFS_LOG("menu : « Crédits »"); credits::ouvrir(); }   // WFS-406 (RG-02)
        else if (choix == TA_VOITURE) { WFS_LOG("menu : « Ta voiture »"); garageRegarder(); g.allerA(Ecran::Garage); }   // #myCarBtn
        return true;
    }
    // record de la session (RG-09) : meilleur score des courses terminées
    void update(double, const GameView& g) override {
        const Course* c = activeCourse();
        // score pris à la fin de la course (endRace), pas pendant la roue libre de l'écran de fin (WFS-38)
        const bool fin = c && c->state == RaceState::Over;
        if (fin && !finVue_ && g.player && c->timeLeft < 1e6) g_best = std::max(g_best, c->score(*g.player));
        finVue_ = fin;
        g_best = std::max(g_best, sauvegarde().best);   // WFS-24 : record gardé d'un lancement à l'autre
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady()) return;
        if (t0_ < 0) t0_ = g.time;
        const float W = g.uiWidth, H = g.uiHeight, t = (float)(g.time - t0_);
        // WFS-147 (RG-01, RG-02, RG-06, ANO-17, ANO-19) : sur téléphone couché, mise en page propre au menu, dessinée en dp
        // (k = pixels par dp), d'après celle de WFS-102 (6d3ed61) : logo de 56 dp, choix de 52 dp (titre 21 dp,
        // description 14 dp), pied 40 dp, aide en 14 dp : 380 dp de haut en tout ; écran de bureau : inchangé
        const bool cp = ui::petitEcran(W, H);
        const MiseEnPage m = cp ? miseEnPagePetit(W, H, 56, 10) : miseEnPage(W, H);
        const float k = m.k, gap = m.gap;
        // .screen : fondu de 300 ms (screen-in) ; voile linear-gradient(100deg, .94 → .78 à 42 % → .12) ;
        // WFS-96 (RG-03) : dégradé par sommets (2 quadrilatères), plus de bandes visibles
        const float op = ease(t / 0.3f);
        const vec4 bg = hex(BG);
        const float xs[3] = {0, W * 0.42f, W}, as[3] = {0.94f * op, 0.78f * op, 0.12f * op};
        for (int i = 0; i < 2; i++) {
            const UiVertex q[4] = {{xs[i], 0, -1, -1, bg.x, bg.y, bg.z, as[i]}, {xs[i + 1], 0, -1, -1, bg.x, bg.y, bg.z, as[i + 1]},
                                   {xs[i + 1], H, -1, -1, bg.x, bg.y, bg.z, as[i + 1]}, {xs[i], H, -1, -1, bg.x, bg.y, bg.z, as[i]}};
            r.uiQuad(q);
        }
        if (choixModeOuvert()) return;   // WFS-141 (RG-04, ANO-54) : panneau des modes ouvert : ni textes ni aide du menu dessous
        if (credits::ouvert()) return;   // WFS-406 : écran Crédits par-dessus (système Credits, 470) : rien du menu dessous
        // mise en page .screen-inner (gap 22 px) : bloc logo, liste, pied, aide tactile
        // WFS-168 (RG-01) : taille des choix d'avant WFS-147 (unité kb = H / 720, comme à 349f7d0) ; textes en dp gardés
        const float kb = H / 720.f;
        const float itemH = 70 * kb, ecart = (cp ? 4 : 6) * k, listeH = 3 * itemH + 2 * ecart, piedH = (cp ? 40 : 44) * k;
        const float aideP = (cp ? 14 : 16) * k, aideL = (cp ? 18 : 22) * k, aideH = 2 * aideL;
        const float total = m.hauteurBloc + gap + listeH + gap + piedH + gap + aideH;
        float y = std::max((cp ? 8 : 24) * k, (H - total) / 2);
        // slide-in : 320 ms, de −36 px à 0, 40 ms de décalage entre les lignes
        auto glisse = [&](int n, float& dx) { const float e = ease((t - 0.04f * n) / 0.32f); dx = -36 * k * (1 - e); return e * op; };
        float dx;
        float a = glisse(0, dx);
        MiseEnPage ml = m; ml.x += dx;
        y = dessinerBlocLogo(r, ml, y, a);
        // ---------- .menu-list : 3 choix (.mitem) ----------
        const float lw = std::min(460 * kb, m.largeur);   // WFS-168 : largeur d'avant WFS-147
        for (int i = 0; i < 3; i++) {
            a = glisse(1 + i, dx);
            const bool presse = i == presse_, grise = false;   // WFS-83 : plus de choix grisé   // .mitem:hover/:active (WFS-38) ; .mitem:disabled (opacité .45)
            const float x = m.x + dx + (presse ? 10 * k : 0), yi = y + i * (itemH + ecart);   // translateX(10px)
            g_zones[i] = {m.x + dx, yi, lw, itemH, 14 * kb, true};   // zone fixe : l'état pressé ne la déplace pas
            manette::focus(g_zones[i]);   // WFS-22
            if (grise) a *= 0.45f;
            // fond vitré en parallélogramme (clip-path : coin bas droit coupé de 14 px) ; pressé : rgba(30,33,48,.9)
            const vec4 fond = presse ? vec4{30 / 255.f, 33 / 255.f, 48 / 255.f, 0.9f * a} : vec4{GLASS.x, GLASS.y, GLASS.z, GLASS.w * a};
            {   // WFS-146 (RG-04, ANO-20) : un seul polygone (avant : 10 bandes translucides qui se chevauchaient)
                const vec2 p[4] = {{x, yi}, {x + lw, yi}, {x + lw - 14 * kb, yi + itemH}, {x, yi + itemH}};
                dessin2d::polygone(r, p, 4, dessin2d::uni(fond));
            }
            const bool solo = i == SOLO;   // #startBtn : barre et titre orange
            if (presse) {   // barre gauche cyan --accent-2 avec halo (0 0 12px rgba(0,229,255,.7))
                for (int h = 3; h >= 1; h--) r.uiRect(x - 4 * k * h, yi - 2 * k * h, 3 * k + 8 * k * h, itemH + 4 * k * h, hex(ACCENT2, 0.12f * a));
                r.uiRect(x, yi, 3 * k, itemH, hex(ACCENT2, a));
            } else r.uiRect(x, yi, 3 * k, itemH, solo ? hex(ACCENT, a) : hex(SURFACE2, a));
            ui::text(r, x + 22 * kb, yi + 12 * kb, (cp ? 21 : 26) * k, ui::majuscules(CHOIX[i].titre), solo ? hex(ACCENT, a) : hex(TEXTE, a), ui::GAUCHE, 0.04f, false);
            ui::text(r, x + 22 * kb, yi + 44 * kb, (cp ? 14 : 13) * k, CHOIX[i].desc, hex(MUTED, a), ui::GAUCHE, 0, false);
        }
        y += listeH + gap;
        // ---------- .menu-foot : record + ta voiture ----------
        a = glisse(4, dx);
        {
            const float x = m.x + dx;
            const float lp = (cp ? 12 : 11) * k, vp = (cp ? 20 : 22) * k, dv = (cp ? 15 : 16) * k;   // libellé, valeur
            ui::text(r, x, y, lp, ui::majuscules("Record"), hex(MUTED, a), ui::GAUCHE, 0.16f, false);
            ui::text(r, x, y + dv, vp, g_best > 0 ? ui::nombre(g_best) : "—", hex(TEXTE, a), ui::GAUCHE, 0, false);
            const float x2 = x + std::max(ui::width(vp, g_best > 0 ? ui::nombre(g_best) : "—"), ui::width(lp, "RECORD", 0.16f)) + 28 * k;
            const std::string nom = ui::majuscules(g.car ? modele(*g.car) : "—");
            ui::text(r, x2, y, lp, ui::majuscules("Ta voiture"), hex(MUTED, a), ui::GAUCHE, 0.16f, false);
            const bool presse = presse_ == TA_VOITURE;   // #myCarBtn : appui = ouvre le garage (WFS-96)
            ui::text(r, x2, y + dv, vp, nom, hex(presse ? ACCENT2 : TEXTE, a), ui::GAUCHE, 0, false);
            const float nw = std::max(ui::width(vp, nom), ui::width(lp, "TA VOITURE", 0.16f));
            r.uiRect(x2, y + dv + vp + 2 * k, ui::width(vp, nom), 2 * k, hex(ACCENT2, a));   // border-bottom cyan de .mycar strong
            g_zones[TA_VOITURE] = {x2 - 6 * kb, y - 6 * kb, nw + 12 * kb, 50 * kb, 0, true};   // WFS-168 : zone d'avant WFS-147 (44 + 6)
            manette::focus(g_zones[TA_VOITURE]);
        }
        y += piedH + gap;
        // ---------- .intro.only-touch : aide des commandes tactiles ----------
        a = glisse(5, dx);
        // WFS-146 (RG-05, ANO-32) : la 1re ligne suit les réglages Direction et Gyroscope (taille, couleur, positions inchangées)
        const bool fleches = reglagesRessenti().direction == ModeDirection::Fleches, gyro = gyroscopeActif();   // capteur présent et réglage non coupé
        const char* volant = fleches ? (gyro ? "Flèches à gauche ou inclinaison : volant." : "Flèches à gauche : volant.")
                                     : (gyro ? "Glisse à gauche ou incline le téléphone pour tourner." : "Glisse à gauche pour tourner.");
        ui::text(r, m.x + dx, y, aideP, volant, hex(INTRO, a), ui::GAUCHE, 0, false);
        ui::text(r, m.x + dx, y + aideL, aideP, "À droite : Gaz, Frein (à l'arrêt : marche arrière), Nitro.", hex(INTRO, a), ui::GAUCHE, 0, false);
        // WFS-406 (RG-02) : en bas à droite, style du bouton « Réglages ». WFS-415 (RG-05, accroche) : dessiné APRÈS les choix,
        // pour que « Course solo » reste la 1re zone de la manette (manette::focus) et du parcours d'endurance (CI #993, #994)
        credits::dessinerBouton(r, W, H, g.marges, presse_ == CREDITS);
    }
private:
    double t0_ = -1;
    bool finVue_ = false;
    int presse_ = -1, doigt_ = -1;   // choix à l'état pressé, doigt qui le tient (WFS-38)
};

} // namespace

void benchMenu(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) { WFS_ERR("menu : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);   // la scène se pose
    g.allerA(Ecran::Menu);
    const double t = 700;
    g.render(t);                                        // entrée dans le menu (début des animations)
    g.render(t + 0.1); r.capturePng(out + "/menu-anim.png");
    WFS_LOG("capture menu-anim : écran %s, 100 ms après l'entrée", nomEcran(g.ecran()));
    g.render(t + 1.0); g.render(t + 1.01); r.capturePng(out + "/menu-0.png");
    WFS_LOG("capture menu-0 : écran %s, menu posé (1 s)", nomEcran(g.ecran()));
    g.shutdown();
}

// WFS-38 : parcours tactile du menu et de l'écran de fin, touchers simulés comme ceux d'un doigt
void benchMenuTactile(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("menu tactile : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(false); g.camMode = 0;
    const float W = (float)r.width(), H = (float)r.height();
    double t = 800;
    int n = 0;   // rendu une image sur 10 seulement (comme les autres scènes) : le banc reste rapide en 2400x1080
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi);
    };
    auto centre = [](int i, float& x, float& y) { x = g_zones[i].x + g_zones[i].w * 0.4f; y = g_zones[i].y + g_zones[i].h * 0.5f; };
    // WFS-41 : écran Terrain → appui sur « Rouler » (carte inchangée : ville), partie reconstruite au pas suivant
    auto rouler = [&]() {
        for (int i = 0; i < 3; i++) image();
        g.render(t); float rx, ry;
        if (centreRouler(rx, ry)) { g.onTouch({20, rx, ry, true}); image(); g.onTouch({20, rx, ry, false}); image(); }
    };
    auto auMenu = [&]() { g.allerA(Ecran::Menu); for (int i = 0; i < 60; i++) image(); };   // 1 s : menu posé
    float x, y;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    auMenu();
    // glisser hors du choix annule : appui sur « Course solo », glissé vers le bas de l'écran, relâcher → on reste au menu
    centre(SOLO, x, y);
    g.onTouch({1, x, y, true}); image(); g.onTouch({1, x, H * 0.95f, true}); image(); g.onTouch({1, x, H * 0.95f, false});
    for (int i = 0; i < 5; i++) image();
    WFS_LOG("menu tactile : appui puis glissé hors de « Course solo » → écran %s (annulé)", nomEcran(g.ecran()));
    // menu-appui : « Course solo » tenu
    g.onTouch({2, x, y, true}); image();
    capture("menu-appui", "« Course solo » à l'état pressé");
    // relâcher → garage (WFS-11) → « Choisir » → course remise à zéro, décompte 3-2-1-GO de WFS-12
    g.onTouch({2, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({7, gx, gy, true}); image(); g.onTouch({7, gx, gy, false}); }
    rouler();   // WFS-41 : garage → terrain → « Rouler »
    const Course* k = nullptr;
    bool trois = false, go = false;
    for (int i = 0; i < 60 * 5 && !(trois && go); i++) {
        image();
        k = activeCourse();
        const std::string c = k ? k->countText() : "";
        if (!trois && c == "3") { trois = true; capture("menu-depart-3", "décompte « 3 » après le relâcher"); }
        if (!go && c == "GO") { for (int f = 0; f < 6; f++) image(); go = true; capture("menu-depart-go", "« GO », départ de la course"); }
    }
    // fin-menu : course depuis le menu, temps forcé à 0,05 s, écran de fin (boutons actifs 0,9 s après la fin)
    centre(SOLO, x, y); g.onTouch({5, x, y, true}); image(); g.onTouch({5, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({8, gx, gy, true}); image(); g.onTouch({8, gx, gy, false}); }   // WFS-11
    rouler();   // WFS-41 : garage → terrain → « Rouler »
    g.setAutopilot(true);                        // une vraie petite course : score et record non nuls
    for (int i = 0; i < 60 * 8; i++) image();   // décompte (3 s) puis 5 s de course
    g.setAutopilot(false);
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) image();
    capture("fin-menu", "écran de fin avec « Rejouer » et « Menu »");
    // bouton « Menu » (CourseSystem::drawOver, WFS-57 : paire centrée, Rejouer 40 u + 2 u + Menu 28 u, à y = 70 u)
    const float u = H / 100, bw = 40 * u, bx = W / 2 - (bw * 1.7f + 2 * u) / 2 + bw + 2 * u;
    g.onTouch({6, bx + bw * 0.35f, 70 * u + 5 * u, true}); image(); g.onTouch({6, bx + bw * 0.35f, 75 * u, false});
    for (int i = 0; i < 60; i++) image();
    capture("fin-retour-menu", "retour au menu après « Menu »");
    // WFS-96 : « Showroom » → garage en mode regarder ; voiture suivante ; « Choisir » → retour au menu avec la voiture
    centre(SHOWROOM, x, y); g.onTouch({9, x, y, true}); image(); g.onTouch({9, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("suivant", gx, gy)) { g.onTouch({10, gx, gy, true}); image(); g.onTouch({10, gx, gy, false}); }
    for (int i = 0; i < 40; i++) image();
    capture("menu-showroom", "garage ouvert depuis « Showroom », voiture suivante");
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({11, gx, gy, true}); image(); g.onTouch({11, gx, gy, false}); }
    for (int i = 0; i < 60; i++) image();
    capture("showroom-choisir", "retour au menu après « Choisir », nouvelle voiture sous « Ta voiture »");
    WFS_LOG("menu : « Choisir » depuis le Showroom → écran %s", nomEcran(g.ecran()));
    // « Ta voiture » (pied du menu) → garage en mode regarder ; « Retour » → menu
    centre(TA_VOITURE, x, y); g.onTouch({12, x, y, true}); image(); g.onTouch({12, x, y, false});
    for (int i = 0; i < 30; i++) image();
    WFS_LOG("menu : « Ta voiture » → écran %s", nomEcran(g.ecran()));
    if (float gx, gy; garageBouton("retour", gx, gy)) { g.onTouch({13, gx, gy, true}); image(); g.onTouch({13, gx, gy, false}); }
    for (int i = 0; i < 30; i++) image();
    WFS_LOG("menu : « Retour » du garage → écran %s", nomEcran(g.ecran()));
    g.shutdown();
    unsetenv("WFS_MOMENT");   // WFS-41 : moment choisi à l'écran Terrain
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Menu, 450)   // interface (400-499) : après le HUD (420), avant le fondu du chargement (490)

} // namespace wfs
