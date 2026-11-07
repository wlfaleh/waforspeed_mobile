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
    bool tenue = false;           // WFS-72 : un homme et une femme habillés (deux tenues fixées en dur)
    bool humains = false;         // WFS-209 : planche des 12 humains réalistes (assets/people/humains, MakeHuman CC0)
} g_etat;

// WFS-72 : deux tenues fixées en dur, valeurs prises dans les palettes de pedLook du jeu web (js/world/people.js :
// PED_SKIN, PED_HAIR, PED_TOPS, PED_BOTTOMS, PED_SHOES, PED_SWIM ; manches 0,45 = courtes ; bas 1,9 = pantalon, 0,72 = short)
vec3 hexC(unsigned h) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}; }
ApparencePersonnage tenue(bool femme, const vec4& corps) {
    ApparencePersonnage a;
    a.corps = corps;
    if (!femme) {   // haut bleu à manches courtes, pantalon beige, chaussures blanches
        a.peau = {0.93f, 0.84f, 0.76f}; a.cheveux = hexC(0x3e2817); a.haut = hexC(0x2b4a7a); a.bas = hexC(0xc2b08c);
        a.chaussures = hexC(0xf0f0ec); a.maillot = hexC(0x1d6fb8); a.vetement = {0.45f, 1.9f, 1, 0};
    } else {        // haut rose à manches courtes, short marine, chaussures noires
        a.peau = {0.6f, 0.45f, 0.36f}; a.cheveux = hexC(0x17110d); a.haut = hexC(0xd06a8a); a.bas = hexC(0x2b3a55);
        a.chaussures = hexC(0x18181a); a.maillot = hexC(0xd42a2f); a.vetement = {0.45f, 0.72f, 1, 0};
    }
    return a;
}

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
        if (g_etat.tenue) { dessinerTenues(r); return; }
        if (g_etat.humains) { dessinerHumains(r); return; }   // WFS-209 (accroche)
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

    void shutdown(Renderer& r) override {
        if (homme_.ok) detruirePersonnage(r, homme_);
        if (femme_.ok) detruirePersonnage(r, femme_);
        for (Personnage& h : humains_) if (h.ok) detruirePersonnage(r, h);   // WFS-209
        charge_ = chargeF_ = chargeH_ = false; poses_.clear();
    }

    // WFS-209 : les 12 humains réalistes en rangée (hommes h01-h06 puis femmes f01-f06), clip g_etat.clip, habillés (tenues de WFS-72)
    void dessinerHumains(Renderer& r) {
        static const char* const IDS[12] = {"h01", "h02", "h03", "h04", "h05", "h06", "f01", "f02", "f03", "f04", "f05", "f06"};
        if (!chargeH_) {
            chargeH_ = true;
            for (int i = 0; i < 12; i++)
                if (chargerPersonnage(r, std::string("people/humains/") + IDS[i] + ".wfp", humains_[i])) preparerPose(humains_[i], posesHumains_[i]);
        }
        const vec3 droite{std::cos(cap_), 0, std::sin(cap_)};
        for (int i = 0; i < 12; i++) {
            Personnage& h = humains_[i];
            if (!h.ok) continue;
            echantillonner(h, clipParNom(h, g_etat.clip), g_etat.temps + i * 0.21, posesHumains_[i]);
            const mat4 monde = translate(base_ + droite * ((i - 5.5f) * 0.75f)) * rotateY(-cap_ + 3.14159265f);
            const ApparencePersonnage a = tenue(i >= 6, h.corps);
            if (const Plage* pl = plageParNom(h, "corps-0"))
                r.drawSkinned(h.maillage, h.materiau, monde, posesHumains_[i].palette.data(), (int)posesHumains_[i].palette.size(), pl->premier, pl->nombre, &a);
        }
    }

    // WFS-72 : homme et femme côte à côte, debout (Idle_Loop), habillés
    void dessinerTenues(Renderer& r) {
        if (!chargeF_) { chargeF_ = true; chargerPersonnage(r, "people/femme.wfp", femme_); preparerPose(femme_, poseF_); preparerPose(homme_, poseH_); }
        if (!femme_.ok) return;
        const vec3 droite{std::cos(cap_), 0, std::sin(cap_)};
        struct Qui { Personnage* p; PoseSquelette* pose; const char* coiffure; float dx; bool femme; };
        for (const Qui& q : {Qui{&homme_, &poseH_, "Hair_SimpleParted-0", -0.55f, false}, Qui{&femme_, &poseF_, "Hair_Long-0", 0.55f, true}}) {
            echantillonner(*q.p, clipParNom(*q.p, "Idle_Loop"), g_etat.temps, *q.pose);
            const mat4 monde = translate(base_ + droite * q.dx) * rotateY(-cap_ + 3.14159265f);
            const ApparencePersonnage a = tenue(q.femme, q.p->corps);
            for (const char* nom : {"corps-0", q.coiffure})
                if (const Plage* pl = plageParNom(*q.p, nom))
                    r.drawSkinned(q.p->maillage, q.p->materiau, monde, q.pose->palette.data(), (int)q.pose->palette.size(), pl->premier, pl->nombre, &a);
        }
    }

private:
    Personnage homme_, femme_; std::vector<PoseSquelette> poses_; PoseSquelette poseH_, poseF_; bool charge_ = false, chargeF_ = false;
    Personnage humains_[12]; PoseSquelette posesHumains_[12]; bool chargeH_ = false;   // WFS-209
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
    g_etat.tenue = true;   // WFS-72 : homme et femme habillés
    capture("squelette-tenue", "Idle_Loop", 0.5, 1);
    g_etat.tenue = false;
    // WFS-209 (accroche) : planche des 12 humains réalistes (debout, assis) : realisme-pietons-humains-*
    g_etat.humains = true; g_etat.instances = 12;
    capture("realisme-pietons-humains-debout", "Idle_Loop", 0.5, 12);
    capture("realisme-pietons-humains-assis", "Sitting_Idle_Loop", 0.5, 12);
    g_etat.humains = false; g_etat.instances = 1;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; i++) { g_etat.temps = 0.3 + i / 30.0; g.render(960 + i / 30.0); }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 20;
    WFS_LOG("squelette : 8 instances (corps + coiffure), échantillonnage CPU %.3f ms par image, image complète %.1f ms (banc, GPU logiciel)",
            g_etat.msEchantillon, ms);
    g_etat.actif = false; g_etat.instances = 1;
    g.shutdown();
}

} // namespace wfs
