// WaForSpeed — carte Corniche (plage), ticket WFS-14, épic E2.
// Port du décor « beach » de js/world/decor.js et de l'ambiance envFor (js/world/maps.js) :
//   - bas-côtés par zone (GROUND.beach) : côté mer promenade, sable, sable mouillé (corniche, village), quai (marina),
//     rochers (côte rocheuse) ; côté terre dunes puis collines ; au-delà, l'océan (plan d'eau #1e7fa8 de jour) ;
//   - palmiers (vegPalm, src/world/vegetation.cpp) des deux côtés, parasols et transats sur le sable ;
//   - ambiance de la plage : brouillard 950 m de jour (#cfe3f0), nuit #141428 à 640 m (WFS_MOMENT=nuit au banc d'essai).
// Couleurs unies (sol, parasols, mâts, transats, gaines, noix) : une seule palette (texture 32 × 1) → 1 maillage par
// morceau ; avec l'océan, le stipe et les palmes : 5 appels de dessin par morceau (≤ 300 au total, journal du banc).
// Placement des objets généré par la graine de la carte (mulberry32(seed · 31 + 7)), avec les pas du jeu web.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "vegetation.h"
#include "zones.h"

namespace wfs {
namespace {

enum Part { P_UNI, P_OCEAN, P_STIPE, P_PALME, P_SECHE, P_COUNT };
constexpr int AHEAD = 22, BEHIND = 2;
constexpr double LOD_FAR_M = 200;     // au-delà : palmiers allégés, petits objets (parasols) non dessinés
constexpr float OCEAN_Y = -1.75f;     // niveau de la mer sous la route (le sable mouillé y plonge)
constexpr float OCEAN_W = 1400.f;     // largeur du plan d'eau (m)

// palette des couleurs unies (GCOL, GCOL2, C du jeu web) ; indice = case de la texture 32 × 1
enum Col { C_PROM, C_SABLE, C_MOUILLE, C_DUNE, C_COLLINE, C_ROCHER, C_QUAI, C_HERBE, C_MUR, C_TRANSAT, C_GAINE, C_NOIX,
           C_PARASOL0, C_NB = C_PARASOL0 + 5 };
const uint32_t PALETTE[C_NB] = {0xb8ab8c, 0xcdb683, 0xa38d60, 0x9aa65e, 0x7a8a4c, 0x7d7a70, 0x9a958a, 0x5f6a3a,
                                0xf2efe6, 0xf2efe6, veg::PALM_SHEATH, veg::PALM_COCO,
                                0xe2262b, 0xffd23a, 0x1d8fb8, 0xffffff, 0xff6a00};
constexpr int PAL_W = 32;   // cases de la palette (≥ C_NB)
static_assert(C_NB <= PAL_W, "palette trop petite");
inline vec2 palUv(int c) { return {(c + 0.5f) / PAL_W, 0.5f}; }

using Geo = veg::Geo;

struct Palm { int64_t seg; float lat, h, lean, rot; };          // lat : signé (> 0 côté mer, à droite)
struct Umbrella { int64_t seg; float lat, along; int col; };

// bande de sol : de a à b (m depuis l'axe, côté positif), hauteurs ya → yb, couleur
struct Band { float a, b, ya, yb; int col; };

// GROUND.beach du jeu web, à partir du bord extérieur du trottoir de Game (RW + 4 m) ; côté s = +1 mer, −1 terre
int profil(const char* zone, int s, Band out[4]) {
    const float R = (float)RW, W0 = R + 4.f;
    int n = 0;
    if (s > 0) {
        if (!std::strcmp(zone, "marina")) { out[n++] = {W0, R + 7, 0.1f, 0.1f, C_QUAI}; out[n++] = {R + 7, R + 7.2f, 0.1f, -1.6f, C_QUAI}; out[n++] = {R + 7.2f, R + 40, -1.6f, -1.9f, C_ROCHER}; }
        else if (!std::strcmp(zone, "rochers")) { out[n++] = {W0, R + 40, -0.18f, -1.8f, C_ROCHER}; out[n++] = {R + 40, R + 46, -1.8f, -2.4f, C_ROCHER}; }
        else { out[n++] = {W0, R + 5, 0.1f, 0.1f, C_PROM}; out[n++] = {R + 5, R + 38, 0.05f, -1.1f, C_SABLE}; out[n++] = {R + 38, R + 48, -1.1f, -1.9f, C_MOUILLE}; }
    } else {
        if (!std::strcmp(zone, "rochers")) { out[n++] = {W0, R + 40, 0.1f + (9 - 0.1f) * (1 / 37.f), 9, C_HERBE}; out[n++] = {R + 40, R + 170, 9, 45, C_ROCHER}; out[n++] = {R + 170, R + 264, 45, 52, C_ROCHER}; }
        else {
            const int c = !std::strcmp(zone, "village") || !std::strcmp(zone, "marina") ? C_PROM : C_DUNE;
            out[n++] = {W0, R + 30, 0.1f + 1.5f / 27.f, 1.6f, c}; out[n++] = {R + 30, R + 170, 1.6f, 30, C_COLLINE}; out[n++] = {R + 170, R + 264, 30, 36, C_COLLINE};
        }
    }
    return n;
}
// sandY du jeu web : hauteur du sable côté mer (corniche) à la distance lat
float sableY(float lat) {
    Band b[4] = {}; const int n = profil("corniche", 1, b);
    for (int i = 0; i < n; i++) if (lat >= b[i].a && lat <= b[i].b) return b[i].ya + (b[i].yb - b[i].ya) * (lat - b[i].a) / (b[i].b - b[i].a);
    return -1.9f;
}

struct Frame { vec3 o, r, f; };
Frame frameAt(Track& t, double s) {
    TrackPoint p = t.point(s, 0);
    return {{(float)p.x, (float)p.y, (float)p.z}, {(float)std::cos(p.th), 0, (float)std::sin(p.th)}, {(float)std::sin(p.th), 0, (float)-std::cos(p.th)}};
}

void tri(Geo& g, vec3 a, vec3 b, vec3 c, vec2 uv) {
    const vec3 n = normalize(cross(b - a, c - a));
    const uint32_t k = (uint32_t)g.v.size();
    for (const vec3& p : {a, b, c}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2});
}
void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv) { tri(g, a, b, c, uv); tri(g, a, c, d, uv); }

// boîte orientée posée en c (axes r, f, demi-tailles hx, hz, hauteur h)
void box(Geo& g, vec3 c, vec3 r, vec3 f, float hx, float hz, float h, vec2 uv) {
    const vec3 up{0, 1, 0};
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    quad(g, P(-hx, 0, -hz), P(hx, 0, -hz), P(hx, h, -hz), P(-hx, h, -hz), uv);
    quad(g, P(hx, 0, -hz), P(hx, 0, hz), P(hx, h, hz), P(hx, h, -hz), uv);
    quad(g, P(hx, 0, hz), P(-hx, 0, hz), P(-hx, h, hz), P(hx, h, hz), uv);
    quad(g, P(-hx, 0, hz), P(-hx, 0, -hz), P(-hx, h, -hz), P(-hx, h, hz), uv);
    quad(g, P(-hx, h, -hz), P(hx, h, -hz), P(hx, h, hz), P(-hx, h, hz), uv);
}
// cylindre vertical, cône (parasol), boule (gaines, noix)
void cylinder(Geo& g, vec3 b, float rad, float h, int n, vec2 uv) {
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, b + d1 * rad, b + d0 * rad, b + d0 * rad + vec3{0, h, 0}, b + d1 * rad + vec3{0, h, 0}, uv);
    }
}
void cone(Geo& g, vec3 b, float rad, float h, int n, vec2 uv) {
    const vec3 top = b + vec3{0, h, 0};
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 p0 = b + vec3{std::cos(a0), 0, std::sin(a0)} * rad, p1 = b + vec3{std::cos(a1), 0, std::sin(a1)} * rad;
        tri(g, p1, p0, top, uv);   // dessus
        tri(g, p0, p1, b, uv);     // dessous (ombre du parasol vue d'en bas)
    }
}
void ball(Geo& g, vec3 c, vec3 s, vec2 uv) {
    const int nu = 8, nv = 5;
    auto P = [&](int i, int j) {
        const float u = 2 * PI * i / nu, v = PI * j / nv;
        return c + vec3{std::sin(v) * std::cos(u) * s.x, std::cos(v) * s.y, std::sin(v) * std::sin(u) * s.z};
    };
    for (int j = 0; j < nv; j++) for (int i = 0; i < nu; i++) {
        const vec3 a = P(i, j), b = P(i + 1, j), cc = P(i + 1, j + 1), d = P(i, j + 1);
        if (j > 0) tri(g, a, b, cc, uv);
        if (j < nv - 1) tri(g, a, cc, d, uv);
    }
}

vec3 lin(vec3 c) { return {std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f)}; }
vec3 linHex(uint32_t h) { return lin(veg::hex(h)); }

class Plage : public System {
public:
    const char* name() const override { return "plage"; }

    void init(Renderer& r, const GameView& g) override {
        active_ = g.map && std::strcmp(g.map->id, "plage") == 0;
        if (!active_) return;
        const char* m = std::getenv("WFS_MOMENT");   // banc d'essai : WFS_MOMENT=nuit (choix à l'écran : WFS-41)
        night_ = m && std::strcmp(m, "nuit") == 0;
        // palette des couleurs unies (valeurs d'affichage du jeu web : texture sRGB)
        uint8_t px[PAL_W * 4] = {};
        for (int c = 0; c < C_NB; c++) {
            const uint32_t h = PALETTE[c];
            px[c * 4] = (h >> 16) & 255; px[c * 4 + 1] = (h >> 8) & 255; px[c * 4 + 2] = h & 255; px[c * 4 + 3] = 255;
        }
        palTex_ = r.vk().uploadTexture(px, PAL_W, 1, true);
        Material mu; mu.rough = 0.85f; mu.flags = MAT_DOUBLE;   // petits objets vus de tous les côtés
        mats_[P_UNI] = r.makeMaterial(mu, &palTex_, nullptr, nullptr);
        Material mo; const vec3 w = linHex(night_ ? 0x0a1828 : 0x1e7fa8);   // ENV.water
        mo.color = {w.x, w.y, w.z, 1}; mo.rough = 0.12f; mo.flags = MAT_DOUBLE;
        mats_[P_OCEAN] = r.makeMaterial(mo, nullptr, nullptr, nullptr);
        initVegetation(r);
        place(*g.track);
    }

    // stipe (2 texels : BARK.palm / palm2), palmes (atlas du jeu web, case 2) vertes et sèches
    void initVegetation(Renderer& r) {
        std::vector<uint8_t> png; int w = 0, h = 0, n = 0;
        stbi_uc* px = readAsset("world/feuillage.png", png) ? stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4) : nullptr;
        if (px) { leafTex_ = r.vk().uploadTexture(px, (uint32_t)w, (uint32_t)h, true); stbi_image_free(px); }
        else WFS_ERR("Atlas de feuillage introuvable (world/feuillage.png)");
        auto leafMat = [&](vec3 tint) {
            Material m; const vec3 c = lin(tint); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.8f; m.cutoff = 0.5f;
            m.flags = MAT_MASK | MAT_DOUBLE | MAT_VTINT;
            return r.makeMaterial(m, leafTex_.img ? &leafTex_ : nullptr, nullptr, nullptr);
        };
        mats_[P_PALME] = leafMat(veg::TINT_PALM);
        mats_[P_SECHE] = leafMat(veg::hex(veg::PALM_DEAD) * 1.4f);
        uint8_t bark[8];
        for (int k = 0; k < 2; k++) {
            const vec3 c = veg::hex(k ? veg::BARK_PALM2 : veg::BARK_PALM);
            bark[k * 4] = (uint8_t)(c.x * 255 + 0.5f); bark[k * 4 + 1] = (uint8_t)(c.y * 255 + 0.5f); bark[k * 4 + 2] = (uint8_t)(c.z * 255 + 0.5f); bark[k * 4 + 3] = 255;
        }
        barkTex_ = r.vk().uploadTexture(bark, 2, 1, true);
        Material mb; mb.rough = 0.9f; mb.flags = MAT_VTINT;
        mats_[P_STIPE] = r.makeMaterial(mb, &barkTex_, nullptr, nullptr);
    }

    // placement (generateDecor, beach) : pas du jeu web pour chaque zone, tirages de la graine de la carte
    void place(Track& t) {
        const MapDef& map = *t.map();
        const int64_t len = (int64_t)t.segmentCount();
        Mulberry32 D(map.seed * 31 + 7);
        auto d = [&]() { return (float)D.next(); };
        const float R = (float)RW;
        // plages [z0, z1) de même zone
        std::vector<std::pair<int64_t, int64_t>> ranges;
        for (int64_t n = 0; n < len; n++) if (n == 0 || zoneIndex(map, n, len) != zoneIndex(map, n - 1, len)) ranges.push_back({n, len});
        for (size_t i = 0; i + 1 < ranges.size(); i++) ranges[i].second = ranges[i + 1].first;
        for (auto [z0, z1] : ranges) {
            const char* z = zoneId(map, z0, len);
            const bool sable = !std::strcmp(z, "corniche") || !std::strcmp(z, "village");
            if (sable) {   // corniche (et village, même plage) : palmiers et parasols côté mer
                for (int64_t n = z0 + 6; n < z1; n += 9 + (int64_t)(d() * 4)) palms_.push_back({n, R + 3.2f, 7 + d() * 4, (d() - 0.5f) * 0.5f, d() * 6});
                for (int64_t n = z0 + 3; n < z1; n += 7 + (int64_t)(d() * 6)) { const float lat = R + 12 + d() * 20; const int col = (int)(d() * 5); umbrellas_.push_back({n, lat, (d() - 0.5f) * 4, col}); }
            }
            // côté terre : palmiers sur la dune (toutes les zones sauf la côte rocheuse, plus clairsemée)
            const int pas = !std::strcmp(z, "rochers") ? 30 : 14;
            for (int64_t n = z0 + 9; n < z1; n += pas + (int64_t)(d() * 8)) palms_.push_back({n, -(R + 5 + d() * 3), 6 + d() * 4, (d() - 0.5f) * 0.4f, d() * 6});
        }
        WFS_LOG("Plage : %zu palmiers, %zu parasols, %s", palms_.size(), umbrellas_.size(), night_ ? "nuit" : "jour");
    }

    bool remplaceSol(const GameView&) const override { return active_; }

    // ambiance de la plage (envFor) : brouillard plus loin de jour, nuit bleutée
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!active_ || g.sceneDediee) return;
        // Game : brouillard exponentiel 0,0009 pour la ville (fogFar 620 m du jeu web) → même rapport pour la plage
        if (!night_) {
            fp.fogColor = linHex(0xcfe3f0);
            fp.fogDensity = 0.0009f * 620.f / 950.f;
        } else {
            const float k = 2.2f;   // éclaircit les couleurs de nuit du jeu web (son étalonnage n'est pas porté)
            fp.skyTop = linHex(0x04040a) * k; fp.skyHorizon = linHex(0x2b2240) * k; fp.ground = linHex(0x0a0a10) * k;
            fp.fogColor = linHex(0x141428) * k; fp.fogDensity = 0.0009f * 620.f / 640.f;
            fp.sunDir = normalize(vec3{-0.4f, 0.8f, 0.3f}); fp.sunColor = linHex(0x8a96ff); fp.sunIntensity = 0.9f;   // lune
            fp.ambient = 0.7f; fp.exposure = 1.1f; fp.night = true; fp.shadows = false;
        }
    }

    // géométrie d'un morceau (segments [c·CH, (c+1)·CH)) — appelée en parallèle, lit seulement la piste
    void buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]) {
        const MapDef& map = *t.map();
        const int64_t len = (int64_t)t.segmentCount();
        const int64_t a0 = c * CH, a1 = a0 + CH;
        // sol et océan, segment par segment (profil de la zone du segment)
        for (int64_t a = a0; a < a1; a++) {
            const double s0 = (double)a * SEG, s1 = (double)(a + 1) * SEG;
            const char* z = zoneId(map, a, len);
            for (int side : {-1, 1}) {
                Band b[4] = {}; const int n = profil(z, side, b);
                for (int i = 0; i < n; i++) {
                    auto P = [&](double s, float lat, float y) { TrackPoint p = t.point(s, side * lat); return vec3{(float)p.x, (float)p.y + y, (float)p.z}; };
                    vec3 A = P(s0, b[i].a, b[i].ya), B = P(s0, b[i].b, b[i].yb), C = P(s1, b[i].b, b[i].yb), Dd = P(s1, b[i].a, b[i].ya);
                    if (side > 0) quad(out[P_UNI], A, B, C, Dd, palUv(b[i].col)); else quad(out[P_UNI], A, Dd, C, B, palUv(b[i].col));   // face vers le haut
                }
            }
            // océan côté mer
            auto Q = [&](double s, float lat) { TrackPoint p = t.point(s, lat); return vec3{(float)p.x, (float)p.y + OCEAN_Y, (float)p.z}; };
            quad(out[P_OCEAN], Q(s0, (float)RW + 40), Q(s0, (float)RW + OCEAN_W), Q(s1, (float)RW + OCEAN_W), Q(s1, (float)RW + 40), {0, 0});
        }
        const int64_t lap = (int64_t)std::floor((double)a0 / len) * len;
        auto inChunk = [&](int64_t seg, int64_t& abs) {
            for (int64_t L : {lap, lap + len}) { const int64_t a = seg + L; if (a >= a0 && a < a1) { abs = a; return true; } }
            return false;
        };
        for (const Palm& pm : palms_) {
            int64_t a; if (!inChunk(pm.seg, a)) continue;
            const int side = pm.lat > 0 ? 1 : -1;
            TrackPoint p = t.point((double)a * SEG, pm.lat);
            Band b[4] = {}; const int n = profil(zoneId(map, a, len), side, b);
            float gy = 0.15f;   // trottoir de Game
            const float al = std::fabs(pm.lat);
            for (int i = 0; i < n; i++) if (al >= b[i].a && al <= b[i].b) gy = b[i].ya + (b[i].yb - b[i].ya) * (al - b[i].a) / (b[i].b - b[i].a);
            const vec3 base{(float)p.x, (float)p.y + gy - 0.05f, (float)p.z};
            Mulberry32 R((uint32_t)((int)(pm.h * 977) + (int)(pm.rot * 1000)));
            const vec3 top = veg::palmier(out[P_STIPE], out[P_PALME], out[P_SECHE], base, pm.h, pm.rot, pm.lean, R, lod);
            ball(out[P_UNI], top + vec3{0, -0.25f, 0}, {0.42f, 0.55f, 0.42f}, palUv(C_GAINE));   // gaines des palmes
            if (!lod) {
                Mulberry32 Rn((uint32_t)((int)(pm.h * 31) + 7));
                for (int k = 0; k < 5; k++) { const float an = (float)Rn.next() * 6.283f; ball(out[P_UNI], top + vec3{std::cos(an) * 0.3f, -0.55f, std::sin(an) * 0.3f}, {0.22f, 0.32f, 0.22f}, palUv(C_NOIX)); }
            }
        }
        if (!lod) for (const Umbrella& u : umbrellas_) {   // petits objets : pas au-delà de 200 m
            int64_t a; if (!inChunk(u.seg, a)) continue;
            const Frame fr = frameAt(t, (double)a * SEG);
            const vec3 b = fr.o + fr.r * u.lat + fr.f * u.along + vec3{0, sableY(u.lat), 0};
            cylinder(out[P_UNI], b, 0.04f, 2.3f, 6, palUv(C_MUR));
            cone(out[P_UNI], b + vec3{0, 1.9f, 0}, 1.5f, 0.55f, 10, palUv(C_PARASOL0 + u.col));
            box(out[P_UNI], b + fr.r * 1.2f, fr.r, fr.f, 0.35f, 0.95f, 0.3f, palUv(C_TRANSAT));
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!active_ || g.sceneDediee) return;
        frame_++;
        Track& t = *g.track;
        const int64_t pc = (int64_t)std::floor(g.player->dist / SEG) / CH;
        for (auto it = chunks_.begin(); it != chunks_.end();) {
            const bool far = std::abs(it->first - pc) * CH * SEG_M > LOD_FAR_M;
            if (it->first < pc - BEHIND || it->first > pc + AHEAD || far != it->second.lod) {
                for (Mesh& m : it->second.m) if (m.vb.buf) graveyard_.push_back({frame_, m});
                it = chunks_.erase(it);
            } else ++it;
        }
        std::vector<int64_t> missing; std::vector<char> lods;
        for (int64_t c = pc - BEHIND; c <= pc + AHEAD; c++) if (!chunks_.count(c)) { missing.push_back(c); lods.push_back(std::abs(c - pc) * CH * SEG_M > LOD_FAR_M); }
        if (!missing.empty()) {
            // poses calculées ici (thread principal) ; les threads ne font que lire la piste
            t.pose(missing.front() * CH - 2); t.pose((missing.back() + 1) * CH + 40);
            std::vector<std::array<Geo, P_COUNT>> cpu(missing.size());
            jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunk(t, missing[i], lods[i], cpu[i].data()); });
            for (size_t i = 0; i < missing.size(); i++) {
                Chunk& ch = chunks_[missing[i]]; ch.lod = lods[i];
                for (int p = 0; p < P_COUNT; p++) if (!cpu[i][p].i.empty()) ch.m[p] = r.makeMesh(cpu[i][p].v, cpu[i][p].i, true);
            }
        }
        for (auto it = graveyard_.begin(); it != graveyard_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        int draws = 0;
        for (auto& [k, ch] : chunks_) for (int p = 0; p < P_COUNT; p++) if (ch.m[p].indexCount) { r.draw(ch.m[p], mats_[p], I); draws++; }
        lastDraws_ = draws;
        if (frame_ % 300 == 1) WFS_LOG("Plage : %d appels de dessin (%zu morceaux)", draws, chunks_.size());
    }

    void shutdown(Renderer& r) override {
        if (!active_) return;
        WFS_LOG("Plage : %d appels de dessin à la dernière image", lastDraws_);
        r.vk().waitIdle();
        for (auto& [k, ch] : chunks_) for (Mesh& m : ch.m) if (m.vb.buf) r.destroy(m);
        chunks_.clear();
        for (auto& gm : graveyard_) r.destroy(gm.second);
        graveyard_.clear();
        for (Image* im : {&palTex_, &leafTex_, &barkTex_}) if (im->img) r.vk().destroy(*im);
        active_ = false;
    }

private:
    bool active_ = false, night_ = false;
    std::vector<Palm> palms_; std::vector<Umbrella> umbrellas_;
    Material mats_[P_COUNT];
    Image palTex_{}, leafTex_{}, barkTex_{};
    struct Chunk { std::array<Mesh, P_COUNT> m{}; bool lod = false; };
    std::map<int64_t, Chunk> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
    int lastDraws_ = 0;
};

} // namespace

WFS_SYSTEM(Plage, 105)

} // namespace wfs
