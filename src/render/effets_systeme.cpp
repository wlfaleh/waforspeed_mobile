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
static int g_dernierChoc = 0;
static float g_capChoc = 0;
static bool g_bancDerapage = false;   // WFS-224 : banc seulement, fumée des pneus forcée
void effetsBanc(bool on) { g_banc = on; }
void effetsBancDerapage(bool on) { g_bancDerapage = on; }
int effetsEtincelles() { return g_parts ? g_parts->sparks().vivantes() : 0; }
int effetsFumee() { return g_parts ? g_parts->smoke().vivantes() : 0; }
int effetsDernierChoc() { return g_dernierChoc; }
float effetsCapDernierChoc() { return g_capChoc; }

namespace {

constexpr int NIV = 6;               // WFS-224 : niveaux d'opacité de la fumée (un appel de dessin chacun)
constexpr float ALPHA_FUMEE = 0.53f; // WFS-292 (RG-01) : opacité d'une volute au plus dense : 0,44 à la naissance (voluteOpacite ≈ 0,82), 0,30 avant
constexpr float ALBEDO_FUMEE = 0.75f; // WFS-292 (RG-01) : fumée de pneu blanche à gris clair (gouttelettes d'huile et caoutchouc : diffuse claire)
constexpr float EXPO = 0.85f;        // exposition de la course (Game::render)

// WFS-101 : voiture du trafic au contact du joueur juste après hitCar (qui l'a repoussée de 0,25 m) : la plus proche,
// à moins de 1 m en long et de 0,6 m en travers au-delà du contact (emprise tournée de Traffic::collide) ;
// nullptr : aucune (choc contre un décor). dz : écart en long (relZ, > 0 si elle est devant).
const TrafficCar* voitureAuContact(const Player& pl, const Track& t, double& dz) {
    const Traffic* tr = activeTraffic();
    if (!tr || !pl.car) return nullptr;
    const double ch = std::cos(pl.heading), sh = std::abs(std::sin(pl.heading));
    const double myLen = (pl.car->d.L * ch + pl.car->d.W * sh) / M, myW = (pl.car->d.W * ch + pl.car->d.L * sh) / (2 * RW);
    const TrafficCar* best = nullptr;
    double ecart = 1e9;
    for (const TrafficCar& car : tr->cars) {
        const double d = t.relZ(car.z - (pl.position + PLAYER_Z));
        const double gz = (std::abs(d) - (myLen + car.len) / 2) * M, gx = (std::abs(pl.playerX - car.offset) - (myW + car.w)) * RW;
        if (gz > 1 || gx > 0.6) continue;
        const double e = std::max(gz, 0.0) + std::max(gx, 0.0);
        if (e < ecart) { ecart = e; best = &car; dz = d; }
    }
    return best;
}

class EffetsSystem : public System {
public:
    const char* name() const override { return "effets"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r; parts_.vider(); g_parts = &parts_;
        actif_ = !r.vk().offscreen || g_banc;
        parts_.setReduit(r.gpuLogiciel());   // RG-05 : qualité basse (GPU logiciel) → moitié moins de particules
        // WFS-224 (RG-02) : texture de volute (128 × 128) : blanc, alpha gaussien à bord nul, modulé par un bruit lisse
        // (voluteAlpha) ; avant : TEXS.radial du jeu web, disque plein au centre (« boules grises », ANO-120)
        constexpr int T = 128;
        std::vector<uint8_t> px(T * T * 4);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            const float dx = (x + 0.5f) / T * 2 - 1, dy = (y + 0.5f) / T * 2 - 1;
            uint8_t* p = &px[(size_t)(y * T + x) * 4];
            p[0] = p[1] = p[2] = 255; p[3] = (uint8_t)std::lround(effets::voluteAlpha(dx, dy) * 255);
        }
        tex_ = r.vk().uploadTexture(px.data(), T, T, false);
        // fumée : PointsMaterial du jeu web (non éclairé) : albédo noir et metal 1 → ni diffus, ni reflet du ciel, ni
        // reflet du soleil (f0 = 0, rugosité 1) : seule la lumière émise reste (avec metal 0, le reflet ajoutait ≈ 0,01 à
        // 0,03 en HDR, un quart de la couleur de jour). WFS-107 (RG-04) : couleur du jeu web convertie par versHdr (comme
        // les étincelles de WFS-101) ; l'ancienne conversion (ACES ≈ 0,214·x) la rendait ≈ 4,5 fois trop claire : gris
        // presque blanc à l'écran (« feu blanc » au braquage). Mélange normal.
        // WFS-224 (RG-02) : la volute s'efface par son opacité (niveau k : ALPHA_FUMEE × (k + 0,5) / NIV), plus par sa
        // couleur ; deux jeux de matériaux : jour (gris du jeu web) et nuit, où la fumée derrière la voiture n'est éclairée
        // que par les feux arrière : gris très sombre teinté de rouge (pas de gris clair dans le noir).
        // WFS-292 (RG-01, RG-02, ANO-165) : la lumière émise fixe (gris du jeu web converti à l'exposition 0,85) ne suivait plus
        // l'exposition physique (WFS-207 / WFS-230) : de jour, la fumée sortait plus sombre que l'asphalte, invisible. La volute
        // est maintenant une matière diffuse éclairée par la scène (soleil, ciel, phares, lampadaires) : claire de jour, visible
        // la nuit seulement là où une lumière l'éclaire, jamais un gris lumineux dans le noir. MAT_VTINT : Lambert sans reflet,
        // même éclairage des deux faces (normale des sommets vers le haut, longueur 1 : nuage éclairé par-dessus).
        for (int k = 0; k < NIV; k++) {
            for (int nuit = 0; nuit < 2; nuit++) {
                Material m; m.color = {ALBEDO_FUMEE, ALBEDO_FUMEE, ALBEDO_FUMEE * 1.02f, ALPHA_FUMEE * (k + 0.5f) / NIV}; m.rough = 1; m.metal = 0;
                mats_[nuit][k] = r.makeMaterial(m, &tex_, nullptr, nullptr);
                mats_[nuit][k].flags |= MAT_BLEND | MAT_DOUBLE | MAT_VTINT;
            }
            for (int b = 0; b < 2; b++) {   // réserve pleine (140 bouffées) : jamais réallouée pendant la partie
                meshes_[b][k] = r.makeMesh(std::vector<Vertex>(140 * 4), std::vector<uint32_t>(140 * 6), true);
                meshes_[b][k].indexCount = 0;
            }
        }
        // WFS-101 (RG-01) : étincelles additives (AdditiveBlending du jeu web), comme les halos des feux : dégradé radial
        // et brouillard faits par scene.frag (F_ADD), couleur HDR de chaque carré portée par la normale des sommets
        Material add; add.flags = MAT_ADD | MAT_BLEND;
        matEtincelles_ = r.makeMaterial(add, nullptr, nullptr, nullptr);
        flash_ = 0; skid_ = 0; course_ = nullptr; crashSmoke_.clear(); vitessePrec_ = 0; g_dernierChoc = 0; g_capChoc = 0;
    }
    void shutdown(Renderer& r) override {
        for (auto& b : meshes_) for (auto& m : b) r.destroy(m);
        r.vk().destroy(tex_);
        for (auto& j : mats_) for (Material& m : j) r.destroy(m);
        r.destroy(matEtincelles_);   // WFS-187 : jeux de descripteurs rendus
        if (g_parts == &parts_) g_parts = nullptr;
    }

    // pas fixe : déclencheurs (comme update.js / scene.js), puis mouvement des particules
    void update(double dtd, const GameView& g) override {
        if (!actif_ || !g.player || !g.track || g.sceneDediee) return;
        parts_.setReduireMouvements(reglagesRessenti().reduireMouvements);   // WFS-107 (RG-06) : ni étincelle ni fumée
        const float dt = (float)dtd;
        const Player& pl = *g.player;
        const Course* c = activeCourse();
        if (c != course_ || (c && (c->accidents < acc_ || c->bumps < bumps_ || c->scrapes < scr_))) {   // nouvelle partie
            course_ = c; parts_.vider(); flash_ = 0;
            if (c) { acc_ = c->accidents; bumps_ = c->bumps; scr_ = c->scrapes; }
        }
        if (c) {
            const bool accident = c->accidents > acc_, choc = c->bumps > bumps_, frotte = c->scrapes > scr_;
            if (accident) flash_ = 1;                                  // playerAccident
            if (choc) flash_ = std::max(flash_, 0.35f);                // bump(impact)
            if (frotte) flash_ = std::max(flash_, 0.25f);              // frottement (flash 0,25 : WFS-70 RG-03)
            if ((accident || choc || frotte) && !reglagesRessenti().reduireMouvements) {   // emit() : rien si reduceMotion
                // WFS-101 (RG-01) : contactAt(dz, car.offset) du jeu web, à mi-chemin des deux voitures, 0,55 m au-dessus
                // de la route ; décor (crash()) : contactAt(0, playerX). Vitesse : 60 % de celle du joueur au moment du
                // choc (avant que hitCar ne la réduise), + 1 m/s vers le haut.
                double dz = 0;
                const TrafficCar* car = voitureAuContact(pl, *g.track, dz);
                const TrackPoint p = g.track->point(pl.dist + (car ? dz : 0) / 2, (pl.playerX + (car ? car->offset : pl.playerX)) / 2 * RW);
                const float v = (float)((car ? vitessePrec_ : pl.speed) * M);
                const vec3 at{(float)p.x, (float)p.y + 0.55f, (float)p.z};
                const vec3 vel{(float)std::sin(p.th) * v * 0.6f, 1.f, -(float)std::cos(p.th) * v * 0.6f};
                using effets::Choc;
                const int n = accident ? (car ? effets::nombreEtincelles(Choc::ArrierePercute, (pl.controlLock - 0.6) / 1.2)   // impact de Player::accident
                                              : effets::nombreEtincelles(Choc::Decor, 0))
                            : choc ? effets::nombreEtincelles(car && dz < 0 ? Choc::PercuteParArriere : Choc::ArrierePercute, c->lastImpact)
                                   : effets::nombreEtincelles(Choc::Frottement, 0);
                g_dernierChoc = parts_.etincelles(at, (float)p.y + 0.05f, vel, n); g_capChoc = (float)p.th;
                if (g_banc) WFS_LOG("effets : %s, %d étincelles (jeu web), %d émises", accident ? (car ? "accident" : "accident (décor)") : choc ? (car && dz < 0 ? "choc par l'arrière" : "choc") : "frottement",
                                    n, g_dernierChoc);
            }
            acc_ = c->accidents; bumps_ = c->bumps; scr_ = c->scrapes;
        }
        vitessePrec_ = pl.speed;
        // fumée des pneus. WFS-224 (RG-01, ANO-120) : seulement en vrai dérapage (effets::vraiDerapage) ; avant, le seuil
        // du jeu web (accélération latérale > 0,92 × adhérence au-delà de 72 km/h, ou frein au-delà de 65 km/h) fumait à
        // chaque coup de volant à grande vitesse et à chaque freinage. Conduite du jeu web (sans état du monde) : tête-à-queue seul.
        const double vms = pl.speed * M;
        const bool monde = pl.modeleMonde();
        const double derive = monde ? std::atan2(pl.monde.v(), std::max(std::abs(pl.monde.u()), 0.1)) : 0.0;
        const bool skid = effets::vraiDerapage(vms, derive, monde ? pl.monde.sigmaAr : 0.0,
                                               monde ? std::max(pl.monde.glissementAv, pl.monde.glissementAr) : 0.0, pl.spinYaw) || (g_banc && g_bancDerapage);
        skid_ -= dt;
        if (skid && skid_ <= 0) {
            skid_ = 0.035f;   // WFS-292 (RG-01) : une volute toutes les 35 ms par roue (50 ms avant) : nappe continue, ≤ 140 vivantes
            const double L = pl.car ? pl.car->d.L : 4.3, W = pl.car ? pl.car->d.W : 1.8;
            // WFS-224 (RG-02) : la volute part avec 40 % de la vitesse de la voiture (air entraîné par le sillage juste
            // derrière la caisse, ordre de grandeur des mesures de sillage de véhicule) : elle traîne derrière la voiture au
            // lieu de filer d'un coup vers la caméra
            const vec3 sillage = monde ? vec3{(float)pl.monde.vx, 0.f, (float)pl.monde.vz} * 0.4f : vec3{};
            for (int sx : {-1, 1}) {
                const TrackPoint q = g.track->point(pl.dist - L * 0.35 / M, pl.playerX * RW + sx * W * 0.4);
                parts_.fumee({(float)q.x, (float)q.y + 0.25f, (float)q.z}, 1, (float)q.y, sillage);
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
                parts_.fumee({(float)q.x, (float)q.y + 1.1f, (float)q.z}, 1, (float)q.y);
            }
        }
        parts_.update(dt);
        flash_ = std::max(0.f, flash_ - dt * 3);   // flash = max(0, flash − dt × 3)
        nitro_ = pl.nitroOn;
    }

    // fumée : carrés face à la caméra groupés par niveau de fondu (4 appels de dessin au plus) ;
    // étincelles : un seul lot de carrés additifs (WFS-101)
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee) return;
        const vec3 right{g.view.at(0, 0), g.view.at(0, 1), g.view.at(0, 2)}, up{g.view.at(1, 0), g.view.at(1, 1), g.view.at(1, 2)};
        const vec3 nrm{g.view.at(2, 0), g.view.at(2, 1), g.view.at(2, 2)};   // vers la caméra
        auto coins = [&](const vec3& c, float h, vec3 q[4]) {
            q[0] = c - right * h - up * h; q[1] = c + right * h - up * h; q[2] = c + right * h + up * h; q[3] = c - right * h + up * h;
        };
        // WFS-101 (RG-01) : points de 0,22 (makeParticleSystem(220, 0,22), sizeAttenuation du jeu web : 0,22 × (H/2) /
        // profondeur pixels, soit un carré de 0,22 / proj[1][1] m), jamais moins de 3 px à 720 p au loin ;
        // couleur × fondu (k du jeu web) convertie en HDR pour ressortir à l'écran comme au jeu web
        const float p11 = std::max(1e-3f, std::abs(g.proj.at(1, 1)));
        etincelles_.clear();
        for (const auto& q : parts_.sparks().p) {
            if (!q.vivante()) continue;
            const float d = std::max(0.f, dot(g.camPos - q.p, nrm));   // profondeur dans l'axe de la caméra
            const float taille = std::max(parts_.sparks().taille, d * 3.f / 360.f) / p11;
            const vec3 col = effets::versHdr(q.c * q.fondu(), EXPO);
            vec3 c4[4]; coins(q.p, taille * 0.5f, c4);
            static constexpr float UV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
            for (int i = 0; i < 4; i++) etincelles_.push_back({c4[i].x, c4[i].y, c4[i].z, col.x, col.y, col.z, UV[i][0], UV[i][1]});
        }
        if (!etincelles_.empty()) r.drawQuads(etincelles_.data(), (int)etincelles_.size() / 4, matEtincelles_);
        const int b = buf_; buf_ ^= 1;   // double tampon : l'image précédente peut encore lire l'autre
        for (int k = 0; k < NIV; k++) { vtx_[k].clear(); idx_[k].clear(); }
        // fumée : carré face à la caméra de 2,4 / proj[1][1] m (WFS-107 RG-05 : points de 2,4 avec sizeAttenuation du jeu
        // web, même règle que les étincelles : ≈ 1,33 m en vue Proche (58°) ; avant : 2,4 m, ≈ 1,8 fois trop grand) ;
        // centre remonté pour que son bas reste au-dessus de la route (sinon la route le coupe net : bandes à bord
        // horizontal sur les captures)
        // WFS-224 (RG-02) : chaque volute grandit (voluteEchelle) ; son opacité (apparition, effacement, proximité de la
        // caméra) choisit le niveau ; trop transparente, elle n'est pas dessinée
        const float base = parts_.smoke().taille / p11;
        for (auto q : parts_.smoke().p) {
            if (!q.vivante()) continue;
            const float taille = base * effets::voluteEchelle(q.fondu()), hs = taille * 0.5f * std::abs(up.y);
            q.p.y = std::max(q.p.y, q.floor + hs);
            const float d = dot(g.camPos - q.p, nrm);
            const float a = effets::voluteOpacite(q.fondu()) * effets::voluteProche(d);
            const int k = std::min(NIV - 1, (int)(a * NIV));
            if (a * NIV < 0.5f) continue;
            auto& v = vtx_[k]; auto& ix = idx_[k];
            const uint32_t n = (uint32_t)v.size();
            vec3 c4[4]; coins(q.p, taille * 0.5f, c4);
            // WFS-292 : normale vers le haut (MAT_VTINT : sa longueur 1 garde l'albédo) : volute éclairée comme un nuage
            v.push_back({c4[0].x, c4[0].y, c4[0].z, 0, 1, 0, 0, 1}); v.push_back({c4[1].x, c4[1].y, c4[1].z, 0, 1, 0, 1, 1});
            v.push_back({c4[2].x, c4[2].y, c4[2].z, 0, 1, 0, 1, 0}); v.push_back({c4[3].x, c4[3].y, c4[3].z, 0, 1, 0, 0, 0});
            ix.insert(ix.end(), {n, n + 1, n + 2, n, n + 2, n + 3});
        }
        for (int k = 0; k < NIV; k++) {
            Mesh& m = meshes_[b][k];
            if (vtx_[k].empty()) { m.indexCount = 0; continue; }
            r.updateMesh(m, vtx_[k], idx_[k]);
            r.draw(m, mats_[g.night ? 1 : 0][k], mat4::identity());
        }
        if (nitro_ && !r.gpuLogiciel() && !reglagesRessenti().reduireMouvements && g.ecran == Ecran::Course)
            lignesNitro(r, g, right, up, nrm);
    }

    // drawFx du jeu web : 28 traits tirés à chaque image, rgba(230,240,255,.14) : α × 2,5 = 0,35 (WFS-101 RG-02, ANO-25 :
    // invisibles sur téléphone), 2 px à 720 p, autour du point (w/2, 0,45 h). WFS-224 (RG-05, ANO-122) : dessinés DANS la
    // scène (carrés additifs à 1,5 m devant la caméra, comme les étincelles), donc sous toute l'interface (commandes,
    // compteur, classement, carte) ; avant : quadrilatères d'interface, par-dessus le bouton Nitro et visibles à travers le
    // HUD. Le trait reste à la même place à l'écran : point (x, y) de l'écran → point à la profondeur D du cône de vue.
    void lignesNitro(Renderer& r, const GameView& g, const vec3& right, const vec3& up, const vec3& nrm) {
        const float w = g.uiWidth, h = g.uiHeight, s = h / 720.f;
        const float p00 = std::max(1e-3f, std::abs(g.proj.at(0, 0))), p11 = std::max(1e-3f, std::abs(g.proj.at(1, 1)));
        constexpr float D = 1.5f;
        auto monde = [&](float x, float y) {   // pixel d'interface (y vers le bas) → point du monde à D m devant la caméra
            const float nx = x / w * 2 - 1, ny = 1 - y / h * 2;
            return g.camPos - nrm * D + right * (nx * D / p00) + up * (ny * D / p11);
        };
        const float u = std::min(w, h), e = std::max(1.f, 1.f * s);
        const vec3 col = effets::versHdr(vec3{230 / 255.f, 240 / 255.f, 1.f} * 0.35f, EXPO);
        lignes_.clear();
        for (int i = 0; i < 28; i++) {
            const float ang = rnd() * 6.2831853f, r0 = u * (0.32f + rnd() * 0.18f), r1 = r0 + u * (0.2f + rnd() * 0.25f);
            const float x0 = w / 2 + std::cos(ang) * r0, y0 = h * 0.45f + std::sin(ang) * r0 * 0.7f;
            const float x1 = w / 2 + std::cos(ang) * r1, y1 = h * 0.45f + std::sin(ang) * r1 * 0.7f;
            const float dx = x1 - x0, dy = y1 - y0, l = std::max(1e-3f, std::sqrt(dx * dx + dy * dy)), nx = -dy / l * e, ny = dx / l * e;
            const vec3 c4[4] = {monde(x0 + nx, y0 + ny), monde(x1 + nx, y1 + ny), monde(x1 - nx, y1 - ny), monde(x0 - nx, y0 - ny)};
            for (const vec3& c : c4) lignes_.push_back({c.x, c.y, c.z, col.x, col.y, col.z, 0.5f, 0.5f});   // uv au centre : trait plein
        }
        r.drawQuads(lignes_.data(), (int)lignes_.size() / 4, matEtincelles_);
    }

    // drawFx : vignette (teintée sous nitro), flash rouge des chocs
    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.ecran != Ecran::Course) return;
        const float w = g.uiWidth, h = g.uiHeight;
        // WFS-224 (RG-05) : les lignes de vitesse du nitro sont dessinées dans la scène (lignesNitro, render)
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
    Material mats_[2][NIV], matEtincelles_;   // fumée : [jour, nuit][niveau d'opacité]
    Mesh meshes_[2][NIV];
    std::vector<Vertex> vtx_[NIV], etincelles_, lignes_;
    std::vector<uint32_t> idx_[NIV];
    int buf_ = 0;
    float flash_ = 0, skid_ = 0;
    double vitessePrec_ = 0;           // vitesse du joueur au pas précédent (avant le choc : contactAt du jeu web)
    const Course* course_ = nullptr;
    int acc_ = 0, bumps_ = 0, scr_ = 0;
    std::vector<float> crashSmoke_;
    uint32_t graine_ = 0x9E3779B9u;
};
WFS_SYSTEM(EffetsSystem, 350)

} // namespace
} // namespace wfs
