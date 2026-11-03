// WaForSpeed — module « course » (WFS-12) : voir course_system.h.
#include "course_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <tuple>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "game.h"
#include "traffic_system.h"

namespace wfs {

// couleurs du skill design-street-racing (tokens.md), codes sRGB
static const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f};     // --bg #0A0B10 (code sRGB, comme ui::CYAN), voile de l'écran de fin
static bool g_rules = true;   // règles de course actives (voir courseBenchRules)
static const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1};    // --text #F2F4F8
static const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};  // --surface #14161F

class CourseSystem : public System {
public:
    const char* name() const override { return "course"; }

    void init(Renderer&, const GameView&) override {
        course_.reset(); restartReq_ = false; active_ = this;
        if (!g_rules) { course_.state = RaceState::Play; course_.clock = 0; course_.timeLeft = 1e9; course_.lastCount = 0; }
    }
    void shutdown(Renderer&) override { if (active_ == this) active_ = nullptr; }
    void onEcran(Ecran a, Ecran n, const GameView&) override { if (a == Ecran::Menu && n == Ecran::Course) restartReq_ = true; }   // WFS-38 : départ du menu = « Rejouer »

    // après la conduite et les chocs du trafic (ordre 210 > 50) : fin de update() du jeu web
    void collide(double dt, Player& pl, const GameView& g) override {
        if (restartReq_) {   // « Rejouer » : même carte, même graine de trafic (RG-08)
            restartReq_ = false;
            pl.reset(*pl.car);
            trafficRestart(pl.position);
            course_.reset();
            WFS_LOG("course : nouvelle course (même graine de trafic)");
            return;
        }
        if (g.ecran == Ecran::Menu) return;   // WFS-38 : course figée derrière le menu (pas de partie fantôme)
        const RaceState before = course_.state; const int cp = course_.level;
        course_.step(dt, pl, activeTrafficMut(), pl.position);
        if (course_.level != cp)
            WFS_LOG("course : checkpoint %d à %.1f s de course, +%.1f s, reste %.2f s, score %lld", course_.level, course_.raceTime,
                    Course::checkpointBonus(course_.level), course_.timeLeft, course_.score(pl));
        if (before != RaceState::Over && course_.state == RaceState::Over)
            WFS_LOG("course : temps écoulé — score %lld, %.2f km, %d km/h max, %d quasi-contacts, %d checkpoints", course_.score(pl),
                    pl.dist * 0.006 / 1000, course_.maxKmh, course_.nearCount, course_.level);
        // partie finie : la voiture roule seule à 62 % de la vitesse max (« auto » du jeu web)
        if (course_.state == RaceState::Over) pl.speed += (MAX_SPEED * 0.62 - pl.speed) * std::min(1.0, dt);
        if (course_.state != RaceState::Over) { score_ = course_.score(pl); km_ = pl.dist * 0.006 / 1000; }   // figés à la fin (endRace)
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Course || course_.state != RaceState::Over || course_.overTimer < 0.9) return false;
        // zones de toucher = rectangles dessinés par drawOver (une seule source, WFS-57)
        if (t.down && rejouer_.contient(t.x, t.y)) restartReq_ = true;
        if (t.down && menu_.contient(t.x, t.y)) g.allerA(Ecran::Menu);   // WFS-38
        return true;   // l'écran de fin garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.ecran != Ecran::Course || restartReq_) return;   // WFS-38 : ni décompte ni fin sur le menu
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        // compte à rebours : chiffre énorme au centre (showCount du jeu web)
        const std::string c = course_.countText();
        if (!c.empty()) {
            // .countdown span : chiffres --text #F2F4F8 avec halo cyan (0 0 30px rgba(0,229,255,.6)) ;
            // .countdown.go : « GO » --accent orange avec halo orange (--glow-accent). Halo = copies transparentes autour.
            const bool go = c == "GO";
            const float px = 34 * u, x = W * 0.5f, y = H * 0.30f;
            halo(r, x, y, px, c, go ? vec4{1.f, 0.416f, 0.f, 0.10f} : vec4{0.f, 0.898f, 1.f, 0.10f}, ui::CENTRE, 0.04f, px * 0.035f);
            ui::text(r, x, y, px, c, go ? ui::ORANGE : TEXTE, ui::CENTRE, 0.04f, false);
        }
        // écran de fin (endRace / section #over du jeu web), affiché 0,9 s après la fin
        if (course_.state == RaceState::Over && course_.overTimer >= 0.9) drawOver(r, W, H, u);
    }

    static CourseSystem* active_;
    Course course_;

private:
    // halo néon (text-shadow du web) : 12 copies transparentes du texte sur deux cercles autour de sa place
    static void halo(Renderer& r, float x, float y, float px, const std::string& t, const vec4& c, ui::Align a, float esp, float rad) {
        for (int k = 0; k < 12; k++) {
            const float ang = k * 0.5235988f, d = rad * (k % 2 ? 1.f : 0.55f);
            ui::text(r, x + std::cos(ang) * d, y + std::sin(ang) * d, px, t, c, a, esp, false);
        }
    }
    // parallélogramme incliné (clip-path du bouton .btn : 14 px de biais) dessiné en bandes horizontales
    static void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
        const int N = 16;
        for (int i = 0; i < N; i++) {
            const float t = (i + 0.5f) / N, dx = biais * (1 - t);   // haut décalé à droite, bas à gauche
            r.uiRect(x + dx, y + h * i / N, w - biais, h / N + 0.5f, c);
        }
    }
    void drawOver(Renderer& r, float W, float H, float u) {
        r.uiRect(0, 0, W, H, FOND);
        const float pw = std::min(W * 0.86f, 120 * u), px = (W - pw) / 2, py = 10 * u, ph = 80 * u;
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 0.6f * u, ui::ORANGE);   // liseré néon : un seul accent par écran
        const float cx = W / 2;
        // .eyebrow : cyan --accent-2, espacement 0,22 em
        ui::text(r, cx, py + 6 * u, 4.2f * u, ui::majuscules("Temps écoulé"), ui::CYAN, ui::CENTRE, 0.22f, false);
        // .score-big : orange --accent avec halo (--glow-accent)
        halo(r, cx, py + 13 * u, 18 * u, ui::nombre(score_), {1.f, 0.416f, 0.f, 0.08f}, ui::CENTRE, 0.02f, 0.8f * u);
        ui::text(r, cx, py + 13 * u, 18 * u, ui::nombre(score_), ui::ORANGE, ui::CENTRE, 0.02f, false);
        char kmh[32]; std::snprintf(kmh, sizeof kmh, "%d km/h", course_.maxKmh);
        const std::string vals[4] = {ui::decimal(km_, 2) + " km", kmh, ui::nombre(course_.nearCount), ui::nombre(course_.level)};
        const char* labs[4] = {"Distance", "Vitesse max", "Quasi-contacts", "Checkpoints"};
        for (int i = 0; i < 4; i++) {
            const float x = px + pw * (0.125f + 0.25f * i);
            ui::text(r, x, py + 38 * u, 3.4f * u, ui::majuscules(labs[i]), ui::GRIS, ui::CENTRE, 0.16f);
            ui::text(r, x, py + 43 * u, 6.5f * u, vals[i], ui::BLANC, ui::CENTRE);
        }
        // bouton « Rejouer » (biseau orange, texte sombre)
        const float bw = 40 * u, bh = 10 * u;
        // WFS-57 (ANO-14) : paire « Rejouer » + « Menu » (0,7 × la largeur, 2 u d'écart) centrée sur le panneau
        const float mw = bw * 0.7f, gap = 2 * u, bx = cx - (bw + gap + mw) / 2, by = py + 60 * u;
        rejouer_ = {bx, by, bw, bh};
        menu_ = {bx + bw + gap, by, mw, bh};
        biseau(r, rejouer_.x, by, bw, bh, 14 * H / 720, ui::ORANGE);   // 14 px CSS (référence 720 p)
        ui::text(r, rejouer_.x + bw / 2, by + 2.6f * u, 5 * u, ui::majuscules("Rejouer"), {10 / 255.f, 11 / 255.f, 16 / 255.f, 1}, ui::CENTRE, 0.12f, false);
        biseau(r, menu_.x, by, mw, bh, 14 * H / 720, {30 / 255.f, 33 / 255.f, 48 / 255.f, 1});   // WFS-38 : « Menu » (.btn--ghost, #1E2130)
        ui::text(r, menu_.x + mw / 2, by + 2.6f * u, 5 * u, ui::majuscules("Menu"), TEXTE, ui::CENTRE, 0.12f, false);
    }
    bool restartReq_ = false;
    long long score_ = 0; double km_ = 0;
    struct Bouton {
        float x = 0, y = 0, w = 0, h = 0;
        bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
    };
    Bouton rejouer_, menu_;   // boutons de l'écran de fin, posés à chaque dessin (WFS-57)
};
CourseSystem* CourseSystem::active_ = nullptr;
WFS_SYSTEM(CourseSystem, 210)   // gameplay (200-299), après le trafic (50) dont il lit les chocs

const Course* activeCourse() { return CourseSystem::active_ ? &CourseSystem::active_->course_ : nullptr; }
Course* activeCourseMut() { return CourseSystem::active_ ? &CourseSystem::active_->course_ : nullptr; }
double courseScaleDt(double raw) { return CourseSystem::active_ && g_rules ? CourseSystem::active_->course_.scaleDt(raw) : raw; }
void courseBenchRules(bool on) { g_rules = on; }
void courseBenchSetTime(double s) { if (auto* c = activeCourseMut()) c->timeLeft = s; }

// banc d'essai : 1) course de référence (mêmes valeurs que le jeu web), 2) décompte « 3 » puis « GO »,
// 3) course en pilote automatique avec checkpoints, 4) écran de fin
void benchCourseScene(Game& g, Renderer& r, const std::string& out) {
    for (auto [m, c, sd] : {std::tuple{"ville", "golf", 12345u}, std::tuple{"circuit", "m4", 777u}})
        for (const auto& line : simulateCourse(m, c, sd)) WFS_LOG("course de référence : %s", line.c_str());
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("course : chargement impossible"); return; }
    const Course* k = activeCourse();
    if (!k) { WFS_ERR("course : module inactif"); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 300;
    bool shot3 = false, shotGo = false;
    for (int i = 0; i < 60 * 5 && !(shot3 && shotGo); i++) {
        g.update(1.0 / 60); t += 1.0 / 60;
        const std::string c = k->countText();
        if (!shot3 && c == "3") { g.render(t); g.render(t + 0.01); r.capturePng(out + "/course-decompte-3.png"); shot3 = true;
            WFS_LOG("capture course-decompte-3 : décompte « 3 », %.2f s avant le départ, %.0f km/h", -k->clock, g.player().kmh()); }
        if (!shotGo && c == "GO") {
            for (int f = 0; f < 6; f++) { g.update(1.0 / 60); t += 1.0 / 60; }
            g.render(t); g.render(t + 0.01); r.capturePng(out + "/course-decompte-go.png"); shotGo = true;
            WFS_LOG("capture course-decompte-go : « %s », %.2f s après le départ, %.0f km/h", k->countText().c_str(), k->clock, g.player().kmh());
        }
    }
    // 25 s de course automatique : checkpoints au journal (WFS_LOG du module)
    for (int i = 0; i < 60 * 25; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(t + i / 60.0); }
    WFS_LOG("course automatique : %.1f s, %d checkpoint(s), reste %.2f s, score %lld, %d quasi-contacts, %d accidents",
            k->raceTime, k->level, k->timeLeft, k->score(g.player()), k->nearCount, k->accidents);
    // fin de partie : temps forcé à 0,05 s, puis écran de fin 1,2 s après
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t + 30 + i / 60.0); }
    g.render(t + 32); g.render(t + 32.01);
    r.capturePng(out + "/course-fin.png");
    WFS_LOG("capture course-fin : état %s, %d checkpoint(s), %d km/h max (score affiché = score à la fin, ci-dessus)",
            k->state == RaceState::Over ? "fin" : "en course", k->level, k->maxKmh);
    g.shutdown();
}

} // namespace wfs
