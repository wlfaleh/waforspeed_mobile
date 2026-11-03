// WaForSpeed — banc d'essai du point d'extension « maillages animés par squelette » (WFS-63) : voir bench_squelette.h.
#include "bench_squelette.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/track.h"
#include "squelette.h"

namespace wfs {

namespace {
struct Etat {
    bool actif = false;
    int instances = 1;            // 1 personnage, ou une rangée de 8 pour mesurer le coût (RG-05)
    std::string clip = "Idle_Loop";
    double temps = 0;
    double msEchantillon = 0;     // dernier coût CPU de echantillonner (toutes instances)
} g_etat;

// système de test : inactif hors banc (aucun chargement, aucun effet sur le jeu) ; actif, il prend la scène de l'image
// (même ciel, soleil, ombres et brouillard que la course) et pose un ou plusieurs personnages sur la route
class SqueletteEssai : public System {
public:
    const char* name() const override { return "squelette-essai"; }

    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (!g_etat.actif || !g.track || !g.player) return false;
        const TrackPoint p = g.track->point(g.player->dist, 0);
        base_ = {(float)p.x, (float)p.y, (float)p.z}; cap_ = (float)p.th;
        // caméra de face, à hauteur de poitrine, 3,4 m devant le personnage (8 instances : plus loin et plus haut)
        const float d = g_etat.instances > 1 ? 9.f : 3.4f, hy = g_etat.instances > 1 ? 2.2f : 1.15f;
        const vec3 avant{std::sin(cap_), 0, -std::cos(cap_)};
        const vec3 look = base_ + vec3{0, 0.9f, 0}, eye = base_ + avant * d + vec3{0, hy, 0};
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        fp.view = lookAt(eye, look, {0, 1, 0});
        fp.proj = perspectiveVk(40.f * 3.14159265f / 180.f, aspect, 0.1f);
        fp.camPos = eye; fp.hasFocus = true; fp.focus = base_;
        s.route = true; s.cible = look;   // route visible (sol, ombres), ni voiture du joueur ni commandes
        return true;
    }

    void render(Renderer& r, const GameView& g) override {
        if (!g_etat.actif || !g.sceneDediee) return;
        if (!charge_) { charge_ = true; chargerPersonnage(r, "people/homme.wfp", homme_); }
        if (!homme_.ok) return;
        const int n = std::clamp(g_etat.instances, 1, SKIN_MAX_INSTANCES);
        if ((int)poses_.size() < n) { poses_.resize((size_t)n); for (PoseSquelette& p : poses_) preparerPose(homme_, p); }
        const int clip = clipParNom(homme_, g_etat.clip);
        const Plage* corps = plageParNom(homme_, "corps-0");
        const Plage* coiffure = plageParNom(homme_, "Hair_SimpleParted-0");
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) echantillonner(homme_, clip, g_etat.temps + i * 0.137, poses_[(size_t)i]);
        g_etat.msEchantillon = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        for (int i = 0; i < n; i++) {
            // le personnage regarde vers +Z dans son repère : tourné pour faire face à la caméra (sens de la route)
            const float dx = n > 1 ? (i - (n - 1) * 0.5f) * 0.9f : 0.f;
            const vec3 droite{std::cos(cap_), 0, std::sin(cap_)};
            const mat4 monde = translate(base_ + droite * dx) * rotateY(-cap_ + 3.14159265f);
            const PoseSquelette& p = poses_[(size_t)i];
            for (const Plage* pl : {corps, coiffure})
                if (pl) r.drawSkinned(homme_.maillage, homme_.materiau, monde, p.palette.data(), (int)p.palette.size(), pl->premier, pl->nombre);
        }
    }

    void shutdown(Renderer& r) override { if (homme_.ok) detruirePersonnage(r, homme_); charge_ = false; poses_.clear(); }

private:
    Personnage homme_; std::vector<PoseSquelette> poses_; bool charge_ = false;
    vec3 base_{}; float cap_ = 0;
};
} // namespace

WFS_SYSTEM(SqueletteEssai, 905)

void benchSquelette(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) { WFS_ERR("squelette : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g_etat.actif = true;
    auto capture = [&](const char* nom, const char* clip, double t, int n) {
        g_etat.clip = clip; g_etat.temps = t; g_etat.instances = n;
        g.render(950 + t); g.render(950.01 + t);
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : clip %s à %.2f s, %d instance(s), échantillonnage CPU %.3f ms", nom, clip, t, n, g_etat.msEchantillon);
    };
    capture("squelette-essai", "Idle_Loop", 0.5, 1);
    capture("squelette-marche-0", "Walk_Loop", 0.10, 1);
    capture("squelette-marche-1", "Walk_Loop", 0.75, 1);
    // RG-05 : 8 instances complètes (corps + coiffure) dans l'image : coût CPU de l'échantillonnage et durée d'une image
    capture("squelette-8", "Walk_Loop", 0.3, 8);
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; i++) { g_etat.temps = 0.3 + i / 30.0; g.render(960 + i / 30.0); }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 20;
    WFS_LOG("squelette : 8 instances (corps + coiffure), échantillonnage CPU %.3f ms par image, image complète %.1f ms (banc, GPU logiciel)",
            g_etat.msEchantillon, ms);
    g_etat.actif = false; g_etat.instances = 1;
    g.shutdown();
}

} // namespace wfs
