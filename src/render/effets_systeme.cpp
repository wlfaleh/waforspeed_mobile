// WaForSpeed — module « effets de conduite » (WFS-65, épic E4), branché par les systèmes enfichables (WFS-30).
// Étincelles aux chocs, fumée (pneus, voitures accidentées), lignes de vitesse sous nitro, vignette, flash.
// Port de js/world/particles.js, js/world/fx.js (drawFx) et des déclencheurs de js/game/update.js / js/world/scene.js.
// Lecture seule de la partie (joueur, règles de course, trafic) : la physique et le chrono ne changent pas (RG-06).
#include "effets_systeme.h"
#include <cmath>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/ressenti.h"   // WFS-70 : « réduire les mouvements »
#include "../game/player.h"
#include "../game/track.h"
#include "../game/traffic_system.h"
#include "effets_particules.h"
#include "renderer.h"

namespace wfs {

static bool g_banc = false;          // banc d'essai : effets seulement dans les scènes de WFS-65 (autres captures inchangées)
static effets::Particules* g_parts = nullptr;
void effetsBanc(bool on) { g_banc = on; }
int effetsEtincelles() { return g_parts ? g_parts->sparks().vivantes() : 0; }
int effetsFumee() { return g_parts ? g_parts->smoke().vivantes() : 0; }

namespace {

constexpr int NIV = 4;               // niveaux de fondu par couleur (un appel de dessin chacun)
constexpr float EXPO = 0.85f;        // exposition de la course (Game::render)

// couleur d'affichage (0-1, comme les couleurs du jeu web) → valeur HDR qui ressort à l'écran avec cette couleur
// (ACES de la passe finale ≈ 0,214·x pour les faibles valeurs, puis gamma ; même conversion que scene_essai.cpp)
vec3 hdr(const vec3& c) {
    auto f = [](float v) { return std::pow(std::max(v, 0.f), 2.2f) / (0.214f * EXPO); };
    return {f(c.x), f(c.y), f(c.z)};
}

class EffetsSystem : public System {
public:
    const char* name() const override { return "effets"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r; parts_.vider(); g_parts = &parts_;
        actif_ = !r.vk().offscreen || g_banc;
        parts_.setReduit(r.gpuLogiciel());   // RG-05 : qualité basse (GPU logiciel) → moitié moins de particules
        // texture radiale (TEXS.radial du jeu web) : blanc, alpha qui décroît du centre au bord
        constexpr int T = 64;
        std::vector<uint8_t> px(T * T * 4);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            const float dx = (x + 0.5f) / T * 2 - 1, dy = (y + 0.5f) / T * 2 - 1, d = std::sqrt(dx * dx + dy * dy);
            const float a = std::clamp(1 - d, 0.f, 1.f);
            uint8_t* p = &px[(size_t)(y * T + x) * 4];
            p[0] = p[1] = p[2] = 255; p[3] = (uint8_t)std::lround(a * a * 255);
        }
        tex_ = r.vk().uploadTexture(px.data(), T, T, false);
        // matériaux : albédo noir (pas d'éclairage), couleur par la lumière émise, transparence par la texture
        for (int c = 0; c < 3; c++) for (int k = 0; k < NIV; k++) {
            Material m; m.color = {0, 0, 0, c == 2 ? 0.45f : 1.f}; m.rough = 1; m.metal = 0;
            const float f = (k + 0.5f) / NIV;   // fondu moyen du niveau
            const vec3 col = c == 0 ? vec3{1, 0.95f, 0.8f} : c == 1 ? vec3{1, 0.6f, 0.1f} : vec3{0.32f, 0.32f, 0.352f};
            m.emissive = hdr(col * f);
            mats_[c][k] = r.makeMaterial(m, &tex_, nullptr, nullptr);
            mats_[c][k].flags |= MAT_BLEND | MAT_DOUBLE;
            for (int b = 0; b < 2; b++) {   // réserve pleine (220 carrés) : jamais réallouée pendant la partie
                meshes_[b][c][k] = r.makeMesh(std::vector<Vertex>(220 * 4), std::vector<uint32_t>(220 * 6), true);
                meshes_[b][c][k].indexCount = 0;
            }
        }
        flash_ = 0; skid_ = 0; course_ = nullptr; crashSmoke_.clear();
    }
    void shutdown(Renderer& r) override {
        for (auto& b : meshes_) for (auto& c : b) for (auto& m : c) r.destroy(m);
        r.vk().destroy(tex_);
        if (g_parts == &parts_) g_parts = nullptr;
    }

    // pas fixe : déclencheurs (comme update.js / scene.js), puis mouvement des particules
    void update(double dtd, const GameView& g) override {
        if (!actif_ || !g.player || !g.track || g.sceneDediee) return;
        const float dt = (float)dtd;
        const Player& pl = *g.player;
        const Course* c = activeCourse();
        if (c != course_ || (c && (c->accidents < acc_ || c->bumps < bumps_ || c->scrapes < scr_))) {   // nouvelle partie
            course_ = c; parts_.vider(); flash_ = 0;
            if (c) { acc_ = c->accidents; bumps_ = c->bumps; scr_ = c->scrapes; }
        }
        if (c) {
            // contactAt du jeu web : à l'avant de la voiture, 0,55 m au-dessus de la route ; vitesse = 60 % de la sienne
            const double L = pl.car ? pl.car->d.L : 4.3;
            const TrackPoint p = g.track->point(pl.dist + L * 0.5 / M, pl.playerX * RW);
            const float v = (float)(pl.speed * M);
            const vec3 at{(float)p.x, (float)p.y + 0.55f, (float)p.z};
            const vec3 vel{(float)std::sin(p.th) * v * 0.6f, 1.f, -(float)std::cos(p.th) * v * 0.6f};
            if (c->accidents > acc_) { parts_.etincelles(at, (float)p.y + 0.05f, vel, 50); flash_ = 1; }        // playerAccident + crash()
            if (c->bumps > bumps_) {                                                                              // bump(impact)
                parts_.etincelles(at, (float)p.y + 0.05f, vel, 16 + (int)std::lround(c->lastImpact * 50));
                flash_ = std::max(flash_, 0.35f);
            }
            if (c->scrapes > scr_) { parts_.etincelles(at, (float)p.y + 0.05f, vel, 14); flash_ = std::max(flash_, 0.25f); }   // frottement (flash 0,25 : WFS-70 RG-03)
            acc_ = c->accidents; bumps_ = c->bumps; scr_ = c->scrapes;
        }
        // fumée des pneus : gros freinage, tête-à-queue, dérapage à haute vitesse (scene.js)
        const double vms = pl.speed * M;
        const bool skid = (pl.braking && vms > 18) || std::abs(pl.spinYaw) > 0.3 || (std::abs(pl.latAcc) > Phys::grip * 0.92 && vms > 20);
        skid_ -= dt;
        if (skid && skid_ <= 0) {
            skid_ = 0.05f;
            const double L = pl.car ? pl.car->d.L : 4.3, W = pl.car ? pl.car->d.W : 1.8;
            for (int sx : {-1, 1}) {
                const TrackPoint q = g.track->point(pl.dist - L * 0.35 / M, pl.playerX * RW + sx * W * 0.4);
                parts_.fumee({(float)q.x, (float)q.y + 0.25f, (float)q.z}, 1);
            }
        }
        // fumée du capot des voitures du trafic accidentées : 5 s, une bouffée toutes les 0,12 s
        if (const Traffic* tr = activeTraffic()) {
            if (crashSmoke_.size() != tr->cars.size()) crashSmoke_.assign(tr->cars.size(), 0.f);
            for (size_t i = 0; i < tr->cars.size(); i++) {
                const TrafficCar& car = tr->cars[i];
                crashSmoke_[i] -= dt;
                if (!car.crashed || car.crashT >= 5 || crashSmoke_[i] > 0) continue;
                const double rel = g.track->relZ(car.z - pl.position);
                if (std::abs(rel) * M > 300) continue;
                crashSmoke_[i] = 0.12f;
                const TrackPoint q = g.track->point(pl.dist + rel - car.len * 0.35, car.offset * RW);
                parts_.fumee({(float)q.x, (float)q.y + 1.1f, (float)q.z}, 1);
            }
        }
        parts_.update(dt);
        flash_ = std::max(0.f, flash_ - dt * 3);   // flash = max(0, flash − dt × 3)
        nitro_ = pl.nitroOn;
    }

    // particules : carrés face à la caméra, groupés par couleur et par niveau de fondu (12 appels de dessin au plus)
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee) return;
        const vec3 right{g.view.at(0, 0), g.view.at(0, 1), g.view.at(0, 2)}, up{g.view.at(1, 0), g.view.at(1, 1), g.view.at(1, 2)};
        const vec3 nrm{g.view.at(2, 0), g.view.at(2, 1), g.view.at(2, 2)};   // vers la caméra
        const int b = buf_; buf_ ^= 1;   // double tampon : l'image précédente peut encore lire l'autre
        for (int c = 0; c < 3; c++) for (int k = 0; k < NIV; k++) { vtx_[c][k].clear(); idx_[c][k].clear(); }
        auto ajoute = [&](int c, const effets::Particule& q, float taille) {
            const int k = std::min(NIV - 1, (int)(q.fondu() * NIV));
            auto& v = vtx_[c][k]; auto& ix = idx_[c][k];
            const uint32_t n = (uint32_t)v.size();
            const float h = taille * 0.5f;
            const vec3 a = q.p - right * h - up * h, bb = q.p + right * h - up * h, cc = q.p + right * h + up * h, d = q.p - right * h + up * h;
            v.push_back({a.x, a.y, a.z, nrm.x, nrm.y, nrm.z, 0, 1}); v.push_back({bb.x, bb.y, bb.z, nrm.x, nrm.y, nrm.z, 1, 1});
            v.push_back({cc.x, cc.y, cc.z, nrm.x, nrm.y, nrm.z, 1, 0}); v.push_back({d.x, d.y, d.z, nrm.x, nrm.y, nrm.z, 0, 0});
            ix.insert(ix.end(), {n, n + 1, n + 2, n, n + 2, n + 3});
        };
        for (const auto& q : parts_.sparks().p) if (q.vivante()) ajoute(q.c.y > 0.9f ? 0 : 1, q, parts_.sparks().taille);
        for (const auto& q : parts_.smoke().p) if (q.vivante()) ajoute(2, q, parts_.smoke().taille);
        for (int c = 0; c < 3; c++) for (int k = 0; k < NIV; k++) {
            Mesh& m = meshes_[b][c][k];
            if (vtx_[c][k].empty()) { m.indexCount = 0; continue; }
            r.updateMesh(m, vtx_[c][k], idx_[c][k]);
            r.draw(m, mats_[c][k], mat4::identity());
        }
    }

    // drawFx : lignes de vitesse sous nitro, vignette (teintée sous nitro), flash rouge des chocs
    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        const float w = g.uiWidth, h = g.uiHeight, s = h / 720.f;
        if (nitro_ && !r.gpuLogiciel() && !reglagesRessenti().reduireMouvements) {   // fx.js : nitroOn && !reduceMotion (WFS-70 RG-04)   // 28 traits tirés à chaque image, rgba(230,240,255,.14), 2 px
            const float u = std::min(w, h), e = std::max(1.f, 1.f * s);
            const vec4 c{230 / 255.f, 240 / 255.f, 1, 0.14f};
            for (int i = 0; i < 28; i++) {
                const float ang = rnd() * 6.2831853f, r0 = u * (0.32f + rnd() * 0.18f), r1 = r0 + u * (0.2f + rnd() * 0.25f);
                const float x0 = w / 2 + std::cos(ang) * r0, y0 = h * 0.45f + std::sin(ang) * r0 * 0.7f;
                const float x1 = w / 2 + std::cos(ang) * r1, y1 = h * 0.45f + std::sin(ang) * r1 * 0.7f;
                const float dx = x1 - x0, dy = y1 - y0, l = std::max(1e-3f, std::sqrt(dx * dx + dy * dy)), nx = -dy / l * e, ny = dx / l * e;
                const UiVertex q[4] = {{x0 + nx, y0 + ny, -1, -1, c.x, c.y, c.z, c.w}, {x1 + nx, y1 + ny, -1, -1, c.x, c.y, c.z, c.w},
                                       {x1 - nx, y1 - ny, -1, -1, c.x, c.y, c.z, c.w}, {x0 - nx, y0 - ny, -1, -1, c.x, c.y, c.z, c.w}};
                r.uiQuad(q);
            }
        }
        // dégradé radial centré en (w/2, 0,55 h) : transparent jusqu'à 0,38 × min(w,h), plein à 0,78 × max(w,h)
        // rgba(5,6,10,ENV.vignette) (0,55 la nuit, 0,3 le jour) ; sous nitro : rgba(0,30,40,.5)
        const vec4 bord = nitro_ ? vec4{0, 30 / 255.f, 40 / 255.f, 0.5f} : vec4{5 / 255.f, 6 / 255.f, 10 / 255.f, g.night ? 0.55f : 0.3f};
        const float cx = w / 2, cy = h * 0.55f, r0 = std::min(w, h) * 0.38f, r1 = std::max(w, h) * 0.78f;
        constexpr int SEG = 64;
        for (int i = 0; i < SEG; i++) {
            const float a0 = i * 6.2831853f / SEG, a1 = (i + 1) * 6.2831853f / SEG;
            const UiVertex q[4] = {{cx + std::cos(a0) * r0, cy + std::sin(a0) * r0, -1, -1, bord.x, bord.y, bord.z, 0},
                                   {cx + std::cos(a0) * r1, cy + std::sin(a0) * r1, -1, -1, bord.x, bord.y, bord.z, bord.w},
                                   {cx + std::cos(a1) * r1, cy + std::sin(a1) * r1, -1, -1, bord.x, bord.y, bord.z, bord.w},
                                   {cx + std::cos(a1) * r0, cy + std::sin(a1) * r0, -1, -1, bord.x, bord.y, bord.z, 0}};
            r.uiQuad(q);
        }
        if (flash_ > 0) r.uiRect(0, 0, w, h, {1, 31 / 255.f, 75 / 255.f, flash_ * 0.28f});   // rgba(255,31,75,flash×0,28)
    }

private:
    float rnd() { graine_ ^= graine_ << 13; graine_ ^= graine_ >> 17; graine_ ^= graine_ << 5; return (graine_ >> 8) * (1.f / 16777216.f); }

    Renderer* r_ = nullptr;
    bool actif_ = true, nitro_ = false;
    effets::Particules parts_;
    Image tex_;
    Material mats_[3][NIV];
    Mesh meshes_[2][3][NIV];
    std::vector<Vertex> vtx_[3][NIV];
    std::vector<uint32_t> idx_[3][NIV];
    int buf_ = 0;
    float flash_ = 0, skid_ = 0;
    const Course* course_ = nullptr;
    int acc_ = 0, bumps_ = 0, scr_ = 0;
    std::vector<float> crashSmoke_;
    uint32_t graine_ = 0x9E3779B9u;
};
WFS_SYSTEM(EffetsSystem, 350)

} // namespace
} // namespace wfs
