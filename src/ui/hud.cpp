// WaForSpeed — HUD de course (WFS-27) : voir hud.h.
#include "hud.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "text.h"
#include "toasts.h"

namespace wfs {
namespace {

// jetons :root du jeu web (codes sRGB, comme ui::CYAN)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, DANGER = 0xFF1F4B, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF,
               SURFACE2 = 0x1E2130, SUCCESS = 0xB6FF00;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};   // --glass rgba(20,22,31,.72)

// halo néon (text-shadow) : copies transparentes du texte autour de sa place
void halo(Renderer& r, float x, float y, float px, const std::string& t, const vec4& c, ui::Align a, float rad) {
    for (int k = 0; k < 12; k++) {
        const float ang = k * 0.5235988f, d = rad * (k % 2 ? 1.f : 0.55f);
        ui::text(r, x + std::cos(ang) * d, y + std::sin(ang) * d, px, t, c, a, 0, false);
    }
}
// .label : capitales, espacement 0,16 em, gris ; 11 px du web portés à 13 px minimum et calés sur les pixels entiers
// (à 11 px sur 720 p, « TEMPS » se lisait « IEMPS »)
void label(Renderer& r, float x, float y, float k, const std::string& t, ui::Align a = ui::GAUCHE, const vec4* c = nullptr) {
    const float px = std::round(std::max(13.f, 11 * k));
    ui::text(r, std::round(x), std::round(y), px, ui::majuscules(t), c ? *c : hex(MUTED), a, 0.16f, false);
}
// barre de progression dégradée cyan → orange (.cp-bar i), tranches bord à bord
void barre(Renderer& r, float x, float y, float w, float h, float p) {
    r.uiRect(x, y, w, h, hex(SURFACE2));
    const float plein = w * std::clamp(p, 0.f, 1.f);
    const int N = 12;
    const vec4 a = hex(ACCENT2), b = hex(ACCENT);
    for (int i = 0; i < N && plein > 0; i++) {
        const float t = (i + 0.5f) / N;
        const float x0 = std::floor(x + plein * i / N), x1 = i == N - 1 ? x + plein : std::floor(x + plein * (i + 1) / N);
        if (x1 > x0) r.uiRect(x0, y, x1 - x0, h, {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1});
    }
}

class Hud : public System {
public:
    const char* name() const override { return "HUD"; }
    void drawUi(Renderer& r, const GameView& g) override {
        const Course* c = activeCourse();
        // RG-01 : seulement en course (pas de menu, pas d'écran de fin, pas de scène dédiée)
        if (!c || !g.player || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        // temps illimité = pas de vraie course (vitrines et scènes du banc sans règles de course, courseBenchRules(false))
        if (c->timeLeft > 1e6) return;
        const Player& pl = *g.player;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;   // 1 px CSS = k pixels logiques (720 p de référence)
        const float gl = 16 * k, gt = 14 * k, pad = 14 * k;           // gouttières --gutter-*, marge intérieure .panel
        // WFS-52 : zone sûre (encoche, coins arrondis) = marges de view.marges (WFS-51) + gouttières de WFS-27 ;
        // marges à 0 : positions de WFS-27 inchangées
        const Marges& mg = g.marges;

        // ---------- haut gauche : temps + checkpoint (.p-time) ----------
        {
            const float w = 200 * k, h = 112 * k, x = mg.gauche + gl, y = mg.haut + gt;
            r.uiRect(x, y, w, h, GLASS);
            label(r, x + pad, y + 10 * k, k, "Temps");
            const bool bas = c->timeLeft < 8;   // .time-value.low
            const std::string t = ui::decimal(std::max(0.0, c->timeLeft), 1);
            const float tp = 48 * k, ty = y + 26 * k;
            halo(r, x + pad, ty, tp, t, bas ? hex(DANGER, 0.12f) : hex(ACCENT2, 0.07f), ui::GAUCHE, 2.5f * k);
            ui::text(r, x + pad, ty, tp, t, bas ? hex(DANGER) : hex(TEXTE), ui::GAUCHE, 0, false);
            label(r, x + pad, y + 80 * k, k, "Checkpoint " + std::to_string(c->level + 1));
            barre(r, x + pad, y + 96 * k, w - 2 * pad, 6 * k, (float)(1 - (c->nextCp - pl.dist) / CHECK_DIST));
        }
        // ---------- haut droite : score + combo (.p-score), au-dessus du bouton Nitro (y < 22 % de H) ----------
        {
            const float w = 180 * k, h = c->combo > 1 ? 92 * k : 70 * k, x = W - mg.droite - gl - w, y = mg.haut + gt;
            r.uiRect(x, y, w, h, GLASS);
            const float xr = x + w - pad;
            label(r, xr, y + 10 * k, k, "Score", ui::DROITE);
            ui::text(r, xr, y + 26 * k, 34 * k, ui::nombre(c->score(pl)), hex(TEXTE), ui::DROITE, 0, false);
            if (c->combo > 1)   // .combo : cyan, 16 px
                ui::text(r, xr, y + 66 * k, 16 * k, "Combo ×" + std::to_string(c->combo), hex(ACCENT2), ui::DROITE, 0.04f, false);
        }
        // ---------- bas centre : vitesse + nitro (entre le volant et le frein) ----------
        {
            // centré dans la zone sûre (entre les marges gauche et droite)
            const float cx = (mg.gauche + W - mg.droite) / 2, w = 230 * k, h = 128 * k, x = cx - w / 2, y = H - mg.bas - gt - h;
            r.uiRect(x, y, w, h, GLASS);
            const int v = std::abs(Course::kmh(pl.speed));
            const bool rouge = v > 300, arriere = pl.speed < -1;
            ui::text(r, cx, y + 8 * k, 44 * k, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::CENTRE, 0, false);
            label(r, cx, y + 56 * k, k, arriere ? "R · km/h" : "km/h", ui::CENTRE);
            // .nitro : libellé + 10 cases (round(nitro × 10) allumées, vert --success ; plus lumineuses en nitro active)
            const vec4 lumineux = hex(SUCCESS);
            label(r, x + pad, y + 78 * k, k, "Nitro", ui::GAUCHE, pl.nitroOn ? &lumineux : nullptr);
            const int on = (int)std::lround(std::clamp(pl.nitro, 0.0, 1.0) * 10);
            const float gap = 4 * k, cw = (w - 2 * pad - 9 * gap) / 10, ch = 14 * k, cy = y + 96 * k;
            for (int i = 0; i < 10; i++) {
                const float xi = x + pad + i * (cw + gap);
                if (i < on && pl.nitroOn) r.uiRect(xi - 2 * k, cy - 2 * k, cw + 4 * k, ch + 4 * k, hex(SUCCESS, 0.35f));   // halo
                r.uiRect(xi, cy, cw, ch, i < on ? hex(SUCCESS, pl.nitroOn ? 1.f : 0.85f) : hex(SURFACE2));
            }
        }
    }
};

} // namespace

WFS_SYSTEM(Hud, 420)   // interface (400-499) : après le texte (400), avant le fondu du chargement (490)

// banc d'essai : course en pilote automatique, puis temps bas, nitro active (touchers gaz + nitro), combo ×2
void benchHud(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("HUD : chargement impossible"); return; }
    const float W = (float)r.width(), H = (float)r.height();
    double t = 600;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t); g.render(t + 0.01);
        r.capturePng(out + "/" + nom + ".png");
        const Course* c = activeCourse();
        WFS_LOG("capture %s : temps %.1f s, checkpoint %d, score %lld, combo %d, %.0f km/h, nitro %.2f%s", nom, c ? c->timeLeft : 0,
                c ? c->level + 1 : 0, c ? c->score(g.player()) : 0, c ? c->combo : 0, g.player().kmh(), g.player().nitro,
                g.player().nitroOn ? " (active)" : "");
    };
    g.setAutopilot(true); g.camMode = 0;
    rouler(60 * 9);                       // décompte (3 s) puis 6 s de course
    capture("hud-course");
    courseBenchSetTime(6.0);              // temps bas : rouge sous 8 s
    rouler(30);
    capture("hud-temps-bas");
    courseBenchSetTime(30.0);
    // nitro : le pilote automatique ne l'utilise pas ; on appuie sur gaz + nitro comme un joueur
    g.setAutopilot(false);
    g.onTouch({7, W * 0.89f, H * 0.74f, true}); g.onTouch({8, W * 0.90f, H * 0.32f, true});
    rouler(24);
    capture("hud-nitro");
    g.onTouch({8, W * 0.90f, H * 0.32f, false});
    // combo ×2 : deux quasi-contacts de suite (événements de règles, comme ceux du trafic)
    if (Course* c = activeCourseMut()) { c->nearMiss(); c->nearMiss(); }
    rouler(12);
    capture("hud-combo");
    // WFS-52 : encoche simulée de 90 px à gauche (WFS_ENCOCHE, sinon 90) : bande marquée, aucun panneau du HUD dedans
    {
        const char* e = std::getenv("WFS_ENCOCHE");
        g.setMarges({e ? (float)std::atoi(e) : 90.f, 0, 0, 0}); g.montrerMarges = true;
        rouler(12);
        capture("encoche-hud");
        g.setMarges({}); g.montrerMarges = false;
    }
    g.onTouch({7, W * 0.89f, H * 0.74f, false});
    g.shutdown();
    courseBenchRules(false);
    benchToasts(g, r, out);   // WFS-47 : bandeaux (toast-checkpoint, toast-pile)
}

} // namespace wfs
