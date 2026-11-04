// WaForSpeed — ressenti de conduite (WFS-70) : voir ressenti.h.
// Système « ressenti » (ordre 220, après la course 210) : il lit les compteurs de chocs de la course (accidents, petits
// chocs, frottements : mêmes compteurs que les bruitages de WFS-28), règle secousse et flash, fait vibrer le téléphone et
// dessine le flash. La secousse est appliquée à la caméra par cameraVue (camera_vue.cpp), le ralenti par Course::accident.
#include "ressenti.h"
#include <cmath>
#include <cstdio>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "course_system.h"
#include "direction.h"
#include "game.h"
#include "traffic_system.h"

namespace wfs {

namespace {
Vibreur g_vibreur = nullptr;
vec3 g_dernierDecalage{};   // dernier décalage appliqué à la caméra (journal du banc d'essai)
}

void installerVibreur(Vibreur v) { g_vibreur = v; }

// navigator.vibrate(motif) du jeu web
void vibrer(const int* ms, int n) {
    char buf[96]; int k = 0;
    for (int i = 0; i < n && k < (int)sizeof buf - 8; i++) k += std::snprintf(buf + k, sizeof buf - k, i ? "-%d" : "%d", ms[i]);
    WFS_LOG("vibration %s", n ? buf : "(vide)");
    if (g_vibreur && n > 0) g_vibreur(ms, n);
}

class RessentiSystem : public System {
public:
    const char* name() const override { return "ressenti"; }

    void init(Renderer&, const GameView&) override {
        etat_ = {}; rng_ = Mulberry32(0x70u); g_dernierDecalage = {};
        suivre(activeCourse());
        active_ = this;
    }
    void shutdown(Renderer&) override { if (active_ == this) active_ = nullptr; }

    // après la course (ordre 220 > 210) : chocs du pas qui vient d'être joué
    void collide(double, Player& pl, const GameView& g) override {
        const Course* c = activeCourse();
        if (!c) return;
        // nouvelle course (« Rejouer », menu) : compteurs remis à zéro, aucun effet
        if (c != c_ || c->accidents < accidents_ || c->bumps < bumps_ || c->scrapes < scrapes_) suivre(c);
        const bool reduit = reglagesRessenti().reduireMouvements;
        if (c->accidents > accidents_) {   // playerAccident
            etat_.chocFort(reduit);
            if (reduit) WFS_LOG("ressenti : choc fort (mouvements réduits) — secousse 0, flash %.2f, sans ralenti", etat_.flash);
            else WFS_LOG("ressenti : choc fort — secousse %.2f, flash %.2f, ralenti à %.0f %% pendant %.1f s", etat_.secousse, etat_.flash,
                         c->timeScale * 100, c->slowmoT);
            static const int MOTIF[] = {90, 40, 120};
            vibrer(MOTIF, 3);
        } else if (c->bumps > bumps_) {     // bump(impact)
            etat_.chocLeger(c->lastImpact, reduit);
            WFS_LOG("ressenti : choc léger (impact %.3f) — secousse %.2f, flash %.2f", c->lastImpact, etat_.secousse, etat_.flash);
        }
        if (c->scrapes > scrapes_) {        // frottement (RG-03)
            etat_.frottement(reduit);
            WFS_LOG("ressenti : frottement — secousse %.2f, flash %.2f", etat_.secousse, etat_.flash);
            static const int MOTIF[] = {40};
            vibrer(MOTIF, 1);
        }
        suivre(c);
        // bordure de la route en course (update.js : |playerX| > MAP.edge → shake 0,12)
        const double edge = g.map ? g.map->edge : 1.6;
        if (c->playing() && std::abs(pl.playerX) >= edge && std::abs(pl.speed) > MAX_SPEED * 0.01) etat_.bordure(reduit);
    }

    void update(double dt, const GameView&) override { etat_.avancer(dt); }

    // flash rouge par-dessus la 3D (fx.js), sous le HUD
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Course || g.sceneDediee || etat_.flash <= 0) return;
        r.uiRect(0, 0, g.uiWidth, g.uiHeight, etat_.couleurFlash());
    }

    static RessentiSystem* active_;
    EtatRessenti etat_;
    Mulberry32 rng_{0x70u};   // tirages de la secousse : reproductibles (captures du banc d'essai)

private:
    void suivre(const Course* c) {
        c_ = c;
        accidents_ = c ? c->accidents : 0; bumps_ = c ? c->bumps : 0; scrapes_ = c ? c->scrapes : 0;
    }
    const Course* c_ = nullptr;
    int accidents_ = 0, bumps_ = 0, scrapes_ = 0;
};
RessentiSystem* RessentiSystem::active_ = nullptr;
WFS_SYSTEM(RessentiSystem, 220)   // gameplay (200-299), après la course (210) dont il lit les compteurs

const EtatRessenti& etatRessenti() {
    static const EtatRessenti vide;
    return RessentiSystem::active_ ? RessentiSystem::active_->etat_ : vide;
}

vec3 ressentiDecalageCamera() {
    auto* s = RessentiSystem::active_;
    g_dernierDecalage = s ? s->etat_.decalage(s->rng_, reglagesRessenti().reduireMouvements) : vec3{0, 0, 0};
    return g_dernierDecalage;
}

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=ressenti)
// 1) ressenti-choc : vrai choc arrière contre le trafic (80 km/h plus vite, dans sa file), règles de course actives
//    (décompte puis ralenti) ; capture 3 images après l'accident : secousse et flash ; chrono mesuré pendant le ralenti.
// 2) ressenti-reduit : même scène, « réduire les mouvements » : flash seul, ni secousse ni ralenti.
// 3) ressenti-fleches : mode flèches, flèche gauche appuyée (accélérateur aussi) ; braquage au journal.
void benchRessenti(Game& g, Renderer& r, const std::string& out) {
    const bool regles = courseBenchRulesOn();
    const ReglagesRessenti sauve = reglagesRessenti();
    const double DT = 1.0 / 60;
    auto choc = [&](bool reduit, const char* nom) {
        reglagesRessenti() = ReglagesRessenti{}; reglagesRessenti().reduireMouvements = reduit;
        courseBenchRules(true);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("ressenti : chargement impossible"); return; }
        const Course* k = activeCourse();
        if (!k) { WFS_ERR("ressenti : course inactive"); g.shutdown(); return; }
        g.setAutopilot(true); g.camMode = 0;
        double t = 600;
        for (int i = 0; i < 200; i++) { g.update(DT); t += DT; if (i % 15 == 0) g.render(t); }   // décompte (3 s)
        const int avant = k->accidents;
        trafficBenchPlace(14, 0, 80, true);
        int f = 0;
        for (; f < 150 && k->accidents == avant; f++) { g.update(DT); t += DT; g.render(t); }
        if (k->accidents == avant) WFS_ERR("ressenti : aucun accident en %.1f s", f * DT);
        const double chrono0 = k->raceTime, jeu0 = k->clock;
        for (int i = 0; i < 3; i++) { g.update(DT); t += DT; }
        g.render(t); g.render(t + 0.001);
        r.capturePng(out + "/" + nom + ".png");
        const EtatRessenti& e = etatRessenti();
        WFS_LOG("capture %s : accident après %.2f s, secousse %.2f, flash %.2f, temps ×%.2f, caméra décalée de (%+.3f ; %+.3f) m%s", nom,
                f * DT, e.secousse, e.flash, k->timeScale, g_dernierDecalage.x, g_dernierDecalage.y, reduit ? " (mouvements réduits)" : "");
        for (int i = 3; i < 30; i++) { g.update(DT); t += DT; if (i % 10 == 0) g.render(t); }   // 0,5 s réelle après le choc
        WFS_LOG("ressenti : pendant 0,50 s réelle après le choc, chrono +%.3f s (RG-05 : non allongé), temps de jeu +%.3f s%s",
                k->raceTime - chrono0, k->clock - jeu0, reduit ? "" : " (ralenti)");
        g.shutdown();
    };
    choc(false, "ressenti-choc");
    choc(true, "ressenti-reduit");

    // direction : glissement (par défaut) puis flèches
    reglagesRessenti() = ReglagesRessenti{};
    courseBenchRules(false);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(false); g.camMode = 0;
        const float W = (float)r.width(), H = (float)r.height();
        double t = 700;
        g.onTouch({1, W * 0.20f, H * 0.70f, true}); g.onTouch({1, W * 0.26f, H * 0.70f, true});   // pouce glissé de 6 % de la largeur
        WFS_LOG("direction glissement (par défaut) : consigne %.2f pour un glissement de 6 %% de la largeur", directionBraquage(W));
        g.onTouch({1, W * 0.26f, H * 0.70f, false});
        reglagesRessenti().direction = ModeDirection::Fleches;
        const BoutonsFleches b = boutonsFleches(W, H, g.marges());
        g.onTouch({2, W * 0.89f, H * 0.74f, true});   // accélérateur
        for (int i = 0; i < 60; i++) { g.update(DT); t += DT; if (i % 8 == 0) g.render(t); }
        g.onTouch({3, b.xg + b.c / 2, b.y + b.c / 2, true});   // flèche gauche
        for (int i = 1; i <= 18; i++) {
            g.update(DT); t += DT;
            if (i == 6 || i == 12 || i == 18) WFS_LOG("direction flèches : flèche gauche appuyée depuis %.2f s, braquage %.2f", i * DT, g.player().steer);
        }
        g.render(t); g.render(t + 0.001);
        r.capturePng(out + "/ressenti-fleches.png");
        WFS_LOG("capture ressenti-fleches : flèches gauche (appuyée) et droite, %.0f km/h, braquage %.2f", g.player().kmh(), g.player().steer);
        g.onTouch({3, b.xg + b.c / 2, b.y + b.c / 2, false});
        for (int i = 0; i < 30; i++) { g.update(DT); t += DT; }
        WFS_LOG("direction flèches : 0,50 s après le relâchement, braquage %.2f", g.player().steer);
        g.onTouch({4, b.xd + b.c / 2, b.y + b.c / 2, true});   // flèche droite
        for (int i = 0; i < 18; i++) { g.update(DT); t += DT; }
        WFS_LOG("direction flèches : flèche droite appuyée 0,30 s, braquage %.2f", g.player().steer);
        g.onTouch({4, b.xd + b.c / 2, b.y + b.c / 2, false}); g.onTouch({2, W * 0.89f, H * 0.74f, false});
        g.shutdown();
    } else WFS_ERR("ressenti : chargement impossible");
    reglagesRessenti() = sauve;
    courseBenchRules(regles);
}

} // namespace wfs
