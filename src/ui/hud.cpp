// WaForSpeed — HUD de course (WFS-27) : voir hud.h.
#include "hud.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/camera_vue.h"           // WFS-98 : vue cockpit (etatCockpit)
#include "../game/commandes_tactiles.h"   // WFS-98 : place des commandes (appuis du banc)
#include "../game/course_system.h"
#include "../game/course5.h"   // WFS-119 : modeCourse
#include "../game/game.h"
#include "../render/renderer.h"
#include "ecrans/reglages.h"   // WFS-75 : miroirX (lecture seule)
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
void halo(Renderer& r, float x, float y, float px, const std::string& t, const vec4& c, ui::Align a, float rad, bool italique = false) {
    for (int k = 0; k < 12; k++) {
        const float ang = k * 0.5235988f, d = rad * (k % 2 ? 1.f : 0.55f);
        ui::text(r, x + std::cos(ang) * d, y + std::sin(ang) * d, px, t, c, a, 0, false, italique);
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

// banc d'essai (WFS-46) : vitesse affichée forcée (captures hud-arc-120 / hud-arc-320), −1 = vitesse réelle
int g_kmhForce = -1;

// compteur en arc (WFS-46) : drawSpeedo(k) de js/ui/hud.js, canvas 380 px affiché en 190 px (.speedo) → rayon 80 px,
// trait 7 px ; arc de 270° depuis 135° (0,75 π), échelle 0-380, zone rouge au-delà de 300, graduations tous les 20
// (grand trait tous les 60). Dessiné en disques (arc) et petits carrés (graduations) : aucun nouveau shader.
void compteur(Renderer& r, float cx, float cy, float k, int kmh) {
    const float R = 80 * k, ep = 7 * k, a0 = 0.75f * PI, span = 1.5f * PI, vmax = 380;
    const int N = 90;
    auto at = [&](int i) { return a0 + span * i / N; };
    for (int i = 0; i <= N; i++)   // piste rgba(30,33,48,.85), zone rouge rgba(255,31,75,.55) au-delà de 300
        r.uiCircle(cx + std::cos(at(i)) * R, cy + std::sin(at(i)) * R, ep / 2,
                   (float)i / N >= 300 / vmax ? hex(DANGER, 0.55f) : vec4{30 / 255.f, 33 / 255.f, 48 / 255.f, 0.85f});
    const float pct = std::clamp(kmh / vmax, 0.f, 1.f);
    const int n = (int)std::lround(pct * N);
    const bool rouge = kmh > 300;
    for (int pass = 0; pass < 2; pass++)   // halo (shadowBlur 16) puis trait
        for (int i = 0; i <= n && n > 0; i++) {
            const float a = at(i), x = std::cos(a), y = std::sin(a);
            // createLinearGradient(0, s, s, 0) : du bas gauche (cyan) au haut droit (orange)
            const float t = std::clamp(((0.5f + x * 0.42f) + (1 - (0.5f + y * 0.42f))) / 2, 0.f, 1.f);
            const vec4 c0 = hex(ACCENT2), c1 = hex(ACCENT);
            vec4 c = rouge ? hex(DANGER) : vec4{c0.x + (c1.x - c0.x) * t, c0.y + (c1.y - c0.y) * t, c0.z + (c1.z - c0.z) * t, 1};
            if (pass == 0) { c.w = 0.16f; r.uiCircle(cx + x * R, cy + y * R, ep * 1.25f, c); }
            else r.uiCircle(cx + x * R, cy + y * R, ep / 2, c);
        }
    for (int v = 0; v <= 380; v += 20) {   // graduations rgba(242,244,248,.55), trait 1,5 px
        const float a = a0 + span * (v / vmax), r1 = R - 11 * k, r2 = R - ((v % 60 == 0) ? 20 : 15) * k;
        for (int s = 0; s <= 4; s++) {
            const float rr = r1 + (r2 - r1) * s / 4.f, w = 1.6f * k;
            r.uiRect(cx + std::cos(a) * rr - w / 2, cy + std::sin(a) * rr - w / 2, w, w, hex(TEXTE, 0.55f));
        }
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
            const bool ligne = c->duelFinish > 0;   // WFS-110 : course à une ligne d'arrivée (course à 5, duel) : chrono et distance
            const bool bas = !ligne && c->timeLeft < 8;   // .time-value.low
            const std::string t = ligne ? ui::chrono(c->raceTime) : ui::decimal(std::max(0.0, c->timeLeft), 1);
            const float tp = 48 * k, ty = y + 26 * k;
            // WFS-48 : .time-value en italique (font-style: italic du web)
            halo(r, x + pad, ty, tp, t, bas ? hex(DANGER, 0.12f) : hex(ACCENT2, 0.07f), ui::GAUCHE, 2.5f * k, true);
            ui::text(r, x + pad, ty, tp, t, bas ? hex(DANGER) : hex(TEXTE), ui::GAUCHE, 0, false, true);
            if (ligne) { label(r, x + pad, y + 80 * k, k, "Arrivée " + ui::decimal(std::max(0.0, c->duelFinish - pl.dist) * M / 1000, 2) + " km"); barre(r, x + pad, y + 96 * k, w - 2 * pad, 6 * k, (float)std::clamp(pl.dist / c->duelFinish, 0.0, 1.0)); }
            else {
            label(r, x + pad, y + 80 * k, k, "Checkpoint " + std::to_string(c->level + 1));
            barre(r, x + pad, y + 96 * k, w - 2 * pad, 6 * k, (float)(1 - (c->nextCp - pl.dist) / CHECK_DIST));
            }
        }
        // ---------- haut droite : score + combo (.p-score) ----------
        if (modeCourse() != ModeCourse::Poursuite) {   // WFS-119 RG-02 : en poursuite, seul le score de poursuite est affiché
            const float w = 180 * k, h = c->combo > 1 ? 92 * k : 70 * k, x = W - mg.droite - gl - w, y = mg.haut + gt;
            r.uiRect(x, y, w, h, hex(0x0A0B10, 0.85f));   // WFS-135 (ANO-64) : fond opaque, l'enseigne derrière ne se lit plus
            const float xr = x + w - pad;
            label(r, xr, y + 10 * k, k, "Score", ui::DROITE);
            // WFS-48 : .score-value et .combo en italique ; WFS-135 : ombre de 2 px sous les chiffres
            ui::text(r, xr, y + 26 * k + 2 * k, 34 * k, ui::nombre(c->score(pl)), vec4{0, 0, 0, 0.6f}, ui::DROITE, 0, false, true);
            ui::text(r, xr, y + 26 * k, 34 * k, ui::nombre(c->score(pl)), hex(TEXTE), ui::DROITE, 0, false, true);
            if (c->combo > 1)   // .combo : cyan, 16 px
                ui::text(r, xr, y + 66 * k, 16 * k, "Combo ×" + std::to_string(c->combo), hex(ACCENT2), ui::DROITE, 0.04f, false, true);
        }
        const int v = g_kmhForce >= 0 ? g_kmhForce : std::abs(Course::kmh(pl.speed));
        const bool rouge = v > 300, arriere = (pl.speed < -1 || pl.reversing) && g_kmhForce < 0;   // WFS-108 : « R » dès le passage en marche arrière
        const int on = (int)std::lround(std::clamp(pl.nitro, 0.0, 1.0) * 10);   // cases de nitro allumées
        // ---------- vue cockpit (WFS-98, RG-06, ANO-24) : vitesse sous le temps ----------
        // le cadran du tableau de bord 3D donne déjà la vitesse : pas de compteur en arc ; chiffres 28 px et cases de nitro
        // dans un petit panneau sous le temps, sur la rangée du bouton « Menu » (à sa droite, bord droit sur celui du temps)
        if (etatCockpit().actif) {
            const std::string lib = ui::majuscules(arriere ? "R · km/h" : "km/h");
            const float tp = 28 * k, lp = std::round(std::max(13.f, 11 * k)), wd = ui::width(tp, "000", 0), h = 40 * k;
            const float w = pad + wd + 6 * k + ui::width(lp, lib, 0.16f) + pad;
            const float wMenu = pad + 18 * k + 6 * k + ui::width(14 * k, "MENU", 0.1f) + pad;   // .cam-btn#menuBtnHud (pause.cpp)
            const float x = std::max(mg.gauche + gl + 200 * k - w, mg.gauche + gl + wMenu + 8 * k), y = mg.haut + gt + 112 * k + 10 * k;
            r.uiRect(x, y, w, h, GLASS);
            const float ty = y + 17 * k - 0.4768f * tp, base = ty + 0.7288f * tp;   // milieu des capitales à 17 px du haut
            ui::text(r, x + pad + wd, ty, tp, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::DROITE, 0, false, true);
            label(r, x + pad + wd + 6 * k, base - 0.7288f * lp, k, lib);   // même ligne de base que les chiffres
            const float gap = 2 * k, cw = (w - 2 * pad - 9 * gap) / 10, ch = 4 * k, cy = y + 30 * k;
            for (int i = 0; i < 10; i++)
                r.uiRect(x + pad + i * (cw + gap), cy, cw, ch, i < on ? hex(SUCCESS, pl.nitroOn ? 1.f : 0.85f) : hex(SURFACE2));
            return;
        }
        // ---------- bas centre : vitesse + nitro (entre le volant et le frein) ----------
        {
            // centré dans la zone sûre (entre les marges gauche et droite)
            // WFS-46 : compteur en arc (Ø 190 px ≈ 26 % de la hauteur) ; lecture au centre ; nitro dans l'ouverture du bas
            const float cx = (mg.gauche + W - mg.droite) / 2, R = 95 * k, ccy = H - mg.bas - gt - R - 4 * k;
            r.uiCircle(cx, ccy, R - 2 * k, {GLASS.x, GLASS.y, GLASS.z, 0.55f});   // fond discret pour la lisibilité
            compteur(r, cx, ccy, k, v);
            ui::text(r, cx, ccy - 30 * k, 44 * k, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::CENTRE, 0, false, true);   // .speedo-read (WFS-48)
            label(r, cx, ccy + 18 * k, k, arriere ? "R · km/h" : "km/h", ui::CENTRE);
            // .nitro : libellé + 10 cases (round(nitro × 10) allumées, vert --success ; plus lumineuses en nitro active)
            const vec4 lumineux = hex(SUCCESS);
            const float w = 112 * k, x = cx - w / 2, pad = 0;
            label(r, cx, ccy + 44 * k, k, "Nitro", ui::CENTRE, pl.nitroOn ? &lumineux : nullptr);
            const float gap = 2 * k, cw = (w - 2 * pad - 9 * gap) / 10, ch = 9 * k, cy = ccy + 64 * k;
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
    // WFS-98 : appuis au centre des commandes dessinées (pédale de gaz, bouton nitro), sans marges
    const DispositionCommandes dc = dispositionCommandes(W, H, ecrans::margesCommandes(Marges{}));
    const float gx = ecrans::miroirX(dc.gaz.cx(), W), gy = dc.gaz.cy(), nx = ecrans::miroirX(dc.nitro.cx(), W), ny = dc.nitro.cy();
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
    g.onTouch({7, gx, gy, true}); g.onTouch({8, nx, ny, true});
    rouler(24);
    capture("hud-nitro");
    g.onTouch({8, nx, ny, false});
    // combo ×2 : deux quasi-contacts de suite (événements de règles, comme ceux du trafic)
    if (Course* c = activeCourseMut()) { c->nearMiss(); c->nearMiss(); }
    rouler(12);
    capture("hud-combo");
    // WFS-46 : compteur en arc à 120 et 320 km/h (zone rouge)
    g_kmhForce = 120; rouler(6); capture("hud-arc-120");
    g_kmhForce = 320; rouler(6); capture("hud-arc-320");
    g_kmhForce = -1;
    // WFS-52 : encoche simulée de 90 px à gauche (WFS_ENCOCHE, sinon 90) : bande marquée, aucun panneau du HUD dedans
    {
        const char* e = std::getenv("WFS_ENCOCHE");
        g.setMarges({e ? (float)std::atoi(e) : 90.f, 0, 0, 0}); g.montrerMarges = true;
        rouler(12);
        capture("encoche-hud");
        g.setMarges({}); g.montrerMarges = false;
    }
    g.onTouch({7, gx, gy, false});
    g.shutdown();
    courseBenchRules(false);
    benchToasts(g, r, out);   // WFS-47 : bandeaux (toast-checkpoint, toast-pile)
}

} // namespace wfs
