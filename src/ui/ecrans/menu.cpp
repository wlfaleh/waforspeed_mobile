// WaForSpeed — menu principal (WFS-37) : voir menu.h.
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
const Choix CHOIX[3] = {   // section #menu de index.html, textes exacts
    {"Course solo", "Bats le chrono, frôle le trafic, enchaîne les checkpoints"},
    {"Duel en ligne", "Défie un ami sur la même route, même trafic"},
    {"Showroom", "Choisis ta voiture et sa couleur"},
};

class Menu : public System {
public:
    const char* name() const override { return "menu"; }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override { if (nouveau == Ecran::Menu) t0_ = -1; }
    // record de la session (RG-09) : meilleur score des courses terminées
    void update(double, const GameView& g) override {
        const Course* c = activeCourse();
        if (c && g.player && c->state == RaceState::Over && c->timeLeft < 1e6) best_ = std::max(best_, c->score(*g.player));
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
            const float x = m.x + dx, yi = y + i * (itemH + 6 * k);
            // fond vitré en parallélogramme (clip-path : coin bas droit coupé de 14 px)
            const int B = 10;
            for (int b = 0; b < B; b++) {
                const float tb = (b + 0.5f) / B;
                r.uiRect(x, yi + itemH * b / B, lw - 14 * k * tb, itemH / B + 0.5f, {GLASS.x, GLASS.y, GLASS.z, GLASS.w * a});
            }
            const bool solo = i == 0;   // #startBtn : barre et titre orange
            r.uiRect(x, yi, 3 * k, itemH, solo ? hex(ACCENT, a) : hex(SURFACE2, a));
            ui::text(r, x + 22 * k, yi + 12 * k, 26 * k, ui::majuscules(CHOIX[i].titre), solo ? hex(ACCENT, a) : hex(TEXTE, a), ui::GAUCHE, 0.04f, false);
            ui::text(r, x + 22 * k, yi + 44 * k, 13 * k, CHOIX[i].desc, hex(MUTED, a), ui::GAUCHE, 0, false);
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
    double t0_ = -1;
    long long best_ = 0;
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

} // namespace ecrans

WFS_SYSTEM(ecrans::Menu, 450)   // interface (400-499) : après le HUD (420), avant le fondu du chargement (490)

} // namespace wfs
