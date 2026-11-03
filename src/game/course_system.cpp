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

// couleurs du skill design-street-racing (tokens.md), en linéaire
static const vec4 FOND{0.0030f, 0.0033f, 0.0052f, 0.82f};    // --bg #0A0B10, voile de l'écran de fin
static bool g_rules = true;   // règles de course actives (voir courseBenchRules)
static const vec4 SURFACE{0.0070f, 0.0080f, 0.0137f, 0.92f}; // --surface #14161F

class CourseSystem : public System {
public:
    const char* name() const override { return "course"; }

    void init(Renderer&, const GameView&) override {
        course_.reset(); restartReq_ = false; active_ = this;
        if (!g_rules) { course_.state = RaceState::Play; course_.clock = 0; course_.timeLeft = 1e9; course_.lastCount = 0; }
    }
    void shutdown(Renderer&) override { if (active_ == this) active_ = nullptr; }

    // après la conduite et les chocs du trafic (ordre 210 > 50) : fin de update() du jeu web
    void collide(double dt, Player& pl, const GameView&) override {
        if (restartReq_) {   // « Rejouer » : même carte, même graine de trafic (RG-08)
            restartReq_ = false;
            pl.reset(*pl.car);
            trafficRestart(pl.position);
            course_.reset();
            WFS_LOG("course : nouvelle course (même graine de trafic)");
            return;
        }
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
        score_ = course_.score(pl); km_ = pl.dist * 0.006 / 1000;
    }

    bool onTouch(const Touch& t, const GameView&) override {
        if (course_.state != RaceState::Over || course_.overTimer < 0.9) return false;
        if (t.down && t.x >= btn_[0] && t.x <= btn_[0] + btn_[2] && t.y >= btn_[1] && t.y <= btn_[1] + btn_[3]) restartReq_ = true;
        return true;   // l'écran de fin garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        // compte à rebours : chiffre énorme au centre (showCount du jeu web)
        const std::string c = course_.countText();
        if (!c.empty()) {
            const bool go = c == "GO";
            ui::text(r, W * 0.5f, H * 0.30f, 34 * u, c, go ? ui::CYAN : ui::ORANGE, ui::CENTRE, 0.04f);
        }
        // écran de fin (endRace / section #over du jeu web), affiché 0,9 s après la fin
        if (course_.state == RaceState::Over && course_.overTimer >= 0.9) drawOver(r, W, H, u);
    }

    static CourseSystem* active_;
    Course course_;

private:
    void drawOver(Renderer& r, float W, float H, float u) {
        r.uiRect(0, 0, W, H, FOND);
        const float pw = std::min(W * 0.86f, 120 * u), px = (W - pw) / 2, py = 10 * u, ph = 80 * u;
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 0.6f * u, ui::ORANGE);   // liseré néon : un seul accent par écran
        const float cx = W / 2;
        ui::text(r, cx, py + 6 * u, 4.2f * u, ui::majuscules("Temps écoulé"), ui::GRIS, ui::CENTRE, 0.16f);
        ui::text(r, cx, py + 13 * u, 18 * u, ui::nombre(score_), ui::BLANC, ui::CENTRE, 0.02f);
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
        btn_[0] = cx - bw / 2; btn_[1] = py + 60 * u; btn_[2] = bw; btn_[3] = bh;
        r.uiRect(btn_[0], btn_[1], bw, bh, ui::ORANGE);
        ui::text(r, cx, btn_[1] + 2.6f * u, 5 * u, ui::majuscules("Rejouer"), {0.003f, 0.003f, 0.005f, 1}, ui::CENTRE, 0.12f, false);
    }
    bool restartReq_ = false;
    long long score_ = 0; double km_ = 0;
    float btn_[4] = {0, 0, 0, 0};
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
    WFS_LOG("capture course-fin : état %s, score %lld, %d checkpoint(s)", k->state == RaceState::Over ? "fin" : "en course",
            k->score(g.player()), k->level);
    g.shutdown();
}

} // namespace wfs
