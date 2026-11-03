// WaForSpeed — menu principal (WFS-37) et ses appuis (WFS-38) : voir menu.h.
#include "menu.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "chargement.h"

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
enum { SOLO = 0, DUEL = 1, SHOWROOM = 2 };
// zones touchables des 3 choix (RG-02) : les parallélogrammes dessinés à la dernière image (x, y, largeur, hauteur, biais)
struct Zone { float x = 0, y = 0, w = 0, h = 0, biais = 0; bool ok = false; };
Zone g_zones[3];
bool dansZone(const Zone& z, float x, float y) {
    if (!z.ok || y < z.y || y > z.y + z.h || x < z.x) return false;
    return x <= z.x + z.w - z.biais * (y - z.y) / z.h;   // coin bas droit coupé (clip-path)
}
int choixSous(float x, float y) { for (int i = 0; i < 3; i++) if (dansZone(g_zones[i], x, y)) return i; return -1; }
const Choix CHOIX[3] = {   // section #menu de index.html, textes exacts
    {"Course solo", "Bats le chrono, frôle le trafic, enchaîne les checkpoints"},
    {"Duel en ligne", "Défie un ami sur la même route, même trafic"},
    {"Showroom", "Choisis ta voiture et sa couleur"},
};

class Menu : public System {
public:
    const char* name() const override { return "menu"; }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        if (nouveau == Ecran::Menu) t0_ = -1;
        presse_ = -1; doigt_ = -1; bientot_ = -10;
        for (Zone& z : g_zones) z.ok = false;
    }
    // RG-01 : appui = état pressé ; l'action part au relâcher sur le même choix, glisser hors du choix l'annule.
    // RG-05 : sur le menu, tout toucher est gardé (aucune commande de conduite).
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Menu) return false;
        const int sous = choixSous(t.x, t.y);
        if (t.down) {
            if (doigt_ < 0) { doigt_ = t.id; presse_ = sous == DUEL ? -1 : sous; }   // « Duel en ligne » grisé : pas d'état pressé
            else if (t.id == doigt_ && sous != presse_) presse_ = -1;               // glissé hors du choix : annulé
            return true;
        }
        if (t.id != doigt_) return true;
        const int choix = presse_;
        doigt_ = -1; presse_ = -1;
        if (choix < 0 || sous != choix) return true;
        if (choix == SOLO) { WFS_LOG("menu : « Course solo »"); g.allerA(Ecran::Course); }             // RG-03
        else if (choix == SHOWROOM) { WFS_LOG("menu : « Showroom » (bientôt)"); bientot_ = g.time; }  // RG-04
        return true;
    }
    // record de la session (RG-09) : meilleur score des courses terminées
    void update(double, const GameView& g) override {
        const Course* c = activeCourse();
        // score pris à la fin de la course (endRace), pas pendant la roue libre de l'écran de fin (WFS-38)
        const bool fin = c && c->state == RaceState::Over;
        if (fin && !finVue_ && g.player && c->timeLeft < 1e6) best_ = std::max(best_, c->score(*g.player));
        finVue_ = fin;
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady()) return;
        if (t0_ < 0) t0_ = g.time;
        const float W = g.uiWidth, H = g.uiHeight, t = (float)(g.time - t0_);
        const MiseEnPage m = miseEnPage(W, H);
        const float k = m.k, gap = m.gap;
        // .screen : fondu de 300 ms (screen-in) ; voile linear-gradient(100deg, .94 → .78 à 42 % → .12), en 16 bandes
        const float op = ease(t / 0.3f);
        const int N = 16;
        for (int i = 0; i < N; i++) {
            const float u = (i + 0.5f) / N, a = u < 0.42f ? 0.94f + (0.78f - 0.94f) * u / 0.42f : 0.78f + (0.12f - 0.78f) * (u - 0.42f) / 0.58f;
            const float x0 = std::floor(W * i / N), x1 = i == N - 1 ? W : std::floor(W * (i + 1) / N);
            r.uiRect(x0, 0, x1 - x0, H, hex(BG, a * op));
        }
        // mise en page .screen-inner (gap 22 px) : bloc logo, liste, pied, aide tactile
        const float itemH = 70 * k, listeH = 3 * itemH + 2 * 6 * k, piedH = 44 * k, aideH = 2 * 22 * k;
        const float total = m.hauteurBloc + gap + listeH + gap + piedH + gap + aideH;
        float y = std::max(24 * k, (H - total) / 2);
        // slide-in : 320 ms, de −36 px à 0, 40 ms de décalage entre les lignes
        auto glisse = [&](int n, float& dx) { const float e = ease((t - 0.04f * n) / 0.32f); dx = -36 * k * (1 - e); return e * op; };
        float dx;
        float a = glisse(0, dx);
        MiseEnPage ml = m; ml.x += dx;
        y = dessinerBlocLogo(r, ml, y, a);
        // ---------- .menu-list : 3 choix (.mitem) ----------
        const float lw = std::min(460 * k, m.largeur);
        for (int i = 0; i < 3; i++) {
            a = glisse(1 + i, dx);
            const bool presse = i == presse_, grise = i == DUEL;   // .mitem:hover/:active (WFS-38) ; .mitem:disabled (opacité .45)
            const float x = m.x + dx + (presse ? 10 * k : 0), yi = y + i * (itemH + 6 * k);   // translateX(10px)
            g_zones[i] = {m.x + dx, yi, lw, itemH, 14 * k, true};   // zone fixe : l'état pressé ne la déplace pas
            if (grise) a *= 0.45f;
            // fond vitré en parallélogramme (clip-path : coin bas droit coupé de 14 px) ; pressé : rgba(30,33,48,.9)
            const vec4 fond = presse ? vec4{30 / 255.f, 33 / 255.f, 48 / 255.f, 0.9f * a} : vec4{GLASS.x, GLASS.y, GLASS.z, GLASS.w * a};
            const int B = 10;
            for (int b = 0; b < B; b++) {
                const float tb = (b + 0.5f) / B;
                r.uiRect(x, yi + itemH * b / B, lw - 14 * k * tb, itemH / B + 0.5f, fond);
            }
            const bool solo = i == SOLO;   // #startBtn : barre et titre orange
            if (presse) {   // barre gauche cyan --accent-2 avec halo (0 0 12px rgba(0,229,255,.7))
                for (int h = 3; h >= 1; h--) r.uiRect(x - 4 * k * h, yi - 2 * k * h, 3 * k + 8 * k * h, itemH + 4 * k * h, hex(ACCENT2, 0.12f * a));
                r.uiRect(x, yi, 3 * k, itemH, hex(ACCENT2, a));
            } else r.uiRect(x, yi, 3 * k, itemH, solo ? hex(ACCENT, a) : hex(SURFACE2, a));
            ui::text(r, x + 22 * k, yi + 12 * k, 26 * k, ui::majuscules(CHOIX[i].titre), solo ? hex(ACCENT, a) : hex(TEXTE, a), ui::GAUCHE, 0.04f, false);
            ui::text(r, x + 22 * k, yi + 44 * k, 13 * k, CHOIX[i].desc, hex(MUTED, a), ui::GAUCHE, 0, false);
            if (grise)   // RG-04 : « Bientôt » sur le choix grisé
                ui::text(r, x + lw - 30 * k, yi + 18 * k, 12 * k, ui::majuscules("Bientôt"), hex(ACCENT2, a / 0.45f * 0.8f), ui::DROITE, 0.16f, false);
        }
        // RG-04 : bandeau « Bientôt » 1,5 s après « Showroom » (style des bandeaux de course : vitré, bordure gauche cyan)
        if (g.time - bientot_ < 1.5) {
            const float e = ease((float)(g.time - bientot_) / 0.24f), fin = std::min(1.f, (float)(1.5 - (g.time - bientot_)) / 0.26f);
            const float bo = e * fin, bx = m.x + lw + 24 * k + 30 * k * (1 - e), byy = y + 2 * (itemH + 6 * k) + 16 * k;
            const std::string bt = ui::majuscules("Bientôt"), bv = "Showroom";
            const float bw = 14 * k + ui::width(18 * k, bt, 0.06f) + 12 * k + ui::width(14 * k, bv) + 18 * k, bh = 38 * k;
            r.uiRect(bx, byy, bw, bh, {GLASS.x, GLASS.y, GLASS.z, GLASS.w * bo});
            r.uiRect(bx, byy, 4 * k, bh, hex(ACCENT2, bo));
            ui::text(r, bx + 14 * k, byy + 8 * k, 18 * k, bt, hex(TEXTE, bo), ui::GAUCHE, 0.06f, false);
            ui::text(r, bx + 14 * k + ui::width(18 * k, bt, 0.06f) + 12 * k, byy + 11 * k, 14 * k, bv, hex(ACCENT2, bo), ui::GAUCHE, 0, false);
        }
        y += listeH + gap;
        // ---------- .menu-foot : record + ta voiture ----------
        a = glisse(4, dx);
        {
            const float x = m.x + dx;
            ui::text(r, x, y, 11 * k, ui::majuscules("Record"), hex(MUTED, a), ui::GAUCHE, 0.16f, false);
            ui::text(r, x, y + 16 * k, 22 * k, best_ > 0 ? ui::nombre(best_) : "—", hex(TEXTE, a), ui::GAUCHE, 0, false);
            const float x2 = x + std::max(ui::width(22 * k, best_ > 0 ? ui::nombre(best_) : "—"), ui::width(11 * k, "RECORD", 0.16f)) + 28 * k;
            const std::string nom = ui::majuscules(g.car ? g.car->model : "—");
            ui::text(r, x2, y, 11 * k, ui::majuscules("Ta voiture"), hex(MUTED, a), ui::GAUCHE, 0.16f, false);
            ui::text(r, x2, y + 16 * k, 22 * k, nom, hex(TEXTE, a), ui::GAUCHE, 0, false);
            r.uiRect(x2, y + 40 * k, ui::width(22 * k, nom), 2 * k, hex(ACCENT2, a));   // border-bottom cyan de .mycar strong
        }
        y += piedH + gap;
        // ---------- .intro.only-touch : aide des commandes tactiles ----------
        a = glisse(5, dx);
        ui::text(r, m.x + dx, y, 16 * k, "Flèches à gauche : volant.", hex(INTRO, a), ui::GAUCHE, 0, false);
        ui::text(r, m.x + dx, y + 22 * k, 16 * k, "À droite : Gaz, Frein (à l'arrêt : marche arrière), Nitro.", hex(INTRO, a), ui::GAUCHE, 0, false);
    }
private:
    double t0_ = -1, bientot_ = -10;
    long long best_ = 0;
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
    // relâcher → course remise à zéro, décompte 3-2-1-GO de WFS-12
    g.onTouch({2, x, y, false});
    const Course* k = nullptr;
    bool trois = false, go = false;
    for (int i = 0; i < 60 * 5 && !(trois && go); i++) {
        image();
        k = activeCourse();
        const std::string c = k ? k->countText() : "";
        if (!trois && c == "3") { trois = true; capture("menu-depart-3", "décompte « 3 » après le relâcher"); }
        if (!go && c == "GO") { for (int f = 0; f < 6; f++) image(); go = true; capture("menu-depart-go", "« GO », départ de la course"); }
    }
    // menu-bientot : retour au menu, « Showroom » → bandeau « Bientôt » ; « Duel en ligne » grisé (sans effet)
    auMenu();
    centre(DUEL, x, y); g.onTouch({3, x, y, true}); image(); g.onTouch({3, x, y, false}); image();
    centre(SHOWROOM, x, y); g.onTouch({4, x, y, true}); image(); g.onTouch({4, x, y, false});
    for (int i = 0; i < 24; i++) image();
    capture("menu-bientot", "bandeau « Bientôt » après « Showroom », « Duel en ligne » grisé");
    // fin-menu : course depuis le menu, temps forcé à 0,05 s, écran de fin (boutons actifs 0,9 s après la fin)
    centre(SOLO, x, y); g.onTouch({5, x, y, true}); image(); g.onTouch({5, x, y, false});
    g.setAutopilot(true);                        // une vraie petite course : score et record non nuls
    for (int i = 0; i < 60 * 8; i++) image();   // décompte (3 s) puis 5 s de course
    g.setAutopilot(false);
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) image();
    capture("fin-menu", "écran de fin avec « Rejouer » et « Menu »");
    // bouton « Menu » : à droite de « Rejouer » (CourseSystem::drawOver : Rejouer centré, 40 u × 10 u à y = 70 u ; Menu à + 1,05 × 40 u)
    const float u = H / 100, bw = 40 * u, bx = W / 2 - bw / 2 + bw * 1.05f;
    g.onTouch({6, bx + bw * 0.35f, 70 * u + 5 * u, true}); image(); g.onTouch({6, bx + bw * 0.35f, 75 * u, false});
    for (int i = 0; i < 60; i++) image();
    capture("fin-retour-menu", "retour au menu après « Menu »");
    g.shutdown();
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Menu, 450)   // interface (400-499) : après le HUD (420), avant le fondu du chargement (490)

} // namespace wfs
