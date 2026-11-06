// WaForSpeed — ville vivante (WFS-71) : outils partagés (enseignes.h) et panneaux néon de la route (DECOR.board du jeu web).
#include "enseignes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../../third_party/stb/stb_truetype.h"   // implémentation dans src/ui/text.cpp
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/track.h"
#include "ville_quartiers.h"
#include "zones.h"

namespace wfs {
namespace vie {

vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}

Repere repere(Track& t, double s) {
    TrackPoint p = t.point(s, 0);
    return {{(float)p.x, (float)p.y, (float)p.z}, {(float)std::cos(p.th), 0, (float)std::sin(p.th)}, {(float)std::sin(p.th), 0, (float)-std::cos(p.th)}};
}

// ---------------------------------------------------------------- géométrie
static void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    const uint32_t k = (uint32_t)g.v.size();
    for (auto [p, u] : {std::pair{a, ua}, {b, ub}, {c, uc}, {d, ud}}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
void boite(Geo& g, vec3 c, vec3 r, vec3 f, float w, float h, float d, vec2 uv) {
    const vec3 up{0, 1, 0};
    const float hx = w / 2, hz = d / 2;
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    struct S { float x0, z0, x1, z1; vec3 n; } cotes[4] = {{-hx, -hz, hx, -hz, -f}, {hx, -hz, hx, hz, r}, {hx, hz, -hx, hz, f}, {-hx, hz, -hx, -hz, -r}};
    for (auto& s : cotes) quad(g, P(s.x0, 0, s.z0), P(s.x1, 0, s.z1), P(s.x1, h, s.z1), P(s.x0, h, s.z0), s.n, uv, uv, uv, uv);
    quad(g, P(-hx, h, -hz), P(hx, h, -hz), P(hx, h, hz), P(-hx, h, hz), up, uv, uv, uv, uv);
}
void cylindre(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n) {
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, c + d0 * rayon, c + d1 * rayon, c + d1 * rayon + vec3{0, h, 0}, c + d0 * rayon + vec3{0, h, 0}, normalize(d0 + d1), uv, uv, uv, uv);
        const uint32_t b = (uint32_t)g.v.size();   // dessus
        for (vec3 p : {c + vec3{0, h, 0}, c + d1 * rayon + vec3{0, h, 0}, c + d0 * rayon + vec3{0, h, 0}}) g.v.push_back({p.x, p.y, p.z, 0, 1, 0, uv.x, uv.y});
        g.i.insert(g.i.end(), {b, b + 1, b + 2});
    }
}
void cone(Geo& g, vec3 c, float rayon, float h, vec2 uv, int n) {
    const vec3 sommet = c + vec3{0, h, 0};
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        const vec3 nm = normalize(normalize(d0 + d1) * h + vec3{0, rayon, 0});
        const uint32_t b = (uint32_t)g.v.size();
        for (vec3 p : {c + d0 * rayon, sommet, c + d1 * rayon}) g.v.push_back({p.x, p.y, p.z, nm.x, nm.y, nm.z, uv.x, uv.y});
        g.i.insert(g.i.end(), {b, b + 1, b + 2});
        const uint32_t d = (uint32_t)g.v.size();   // dessous (ombre du parasol)
        for (vec3 p : {c + d0 * rayon, c + d1 * rayon, c}) g.v.push_back({p.x, p.y, p.z, 0, -1, 0, uv.x, uv.y});
        g.i.insert(g.i.end(), {d, d + 1, d + 2});
    }
}
void plan(Geo& g, vec3 c, vec3 x, vec3 y, vec3 n, float w, float h, float u0, float v0, float u1, float v1) {
    const vec3 hx = x * (w / 2), hy = y * (h / 2);
    quad(g, c - hx - hy, c + hx - hy, c + hx + hy, c - hx + hy, n, {u0, v1}, {u1, v1}, {u1, v0}, {u0, v0});
}

// ---------------------------------------------------------------- palette (cases de 8 × 8 texels, 8 × 8 cases)
vec2 Palette::uv(unsigned hex) {
    auto it = std::find(couleurs_.begin(), couleurs_.end(), hex);
    int i = (int)(it - couleurs_.begin());
    if (it == couleurs_.end()) { if (couleurs_.size() >= 64) i = 0; else { couleurs_.push_back(hex); i = (int)couleurs_.size() - 1; } }
    return {((i % 8) + 0.5f) / 8.f, ((i / 8) + 0.5f) / 8.f};
}
bool Palette::construire(Renderer& r) {
    std::vector<uint8_t> px(64 * 64 * 4, 255);
    for (int i = 0; i < 64; i++) {
        const unsigned h = i < (int)couleurs_.size() ? couleurs_[(size_t)i] : 0x808080;
        for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
            uint8_t* p = &px[((size_t)((i / 8) * 8 + y) * 64 + (i % 8) * 8 + x) * 4];
            p[0] = (h >> 16) & 255; p[1] = (h >> 8) & 255; p[2] = h & 255; p[3] = 255;
        }
    }
    img_ = r.vk().uploadTexture(px.data(), 64, 64, true);
    Material m; m.rough = 0.7f; m.metal = 0.f; m.flags = MAT_DOUBLE;
    materiau = r.makeMaterial(m, &img_, nullptr, nullptr);
    return img_.img != VK_NULL_HANDLE;
}
void Palette::detruire(Renderer& r) { if (img_.img) r.vk().destroy(img_); img_ = Image{}; couleurs_.clear(); }

Material neon(Renderer& r, unsigned hex, float emission) {
    Material m; const vec3 c = lin(hex);
    m.color = {c.x, c.y, c.z, 1}; m.rough = 0.4f; m.emissive = c * emission; m.flags = MAT_DOUBLE;
    return r.makeMaterial(m, nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------------- atlas de textes
namespace {
constexpr int AW = 2048, LIGNE = 96, PX = 72;   // 2 colonnes de 1024 px ; hauteur selon le nombre de textes
std::vector<uint32_t> codepoints(const std::string& s) {
    std::vector<uint32_t> out;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = (unsigned char)s[i];
        uint32_t cp = c; int n = 1;
        if (c >= 0xF0) { cp = c & 7; n = 4; } else if (c >= 0xE0) { cp = c & 15; n = 3; } else if (c >= 0xC0) { cp = c & 31; n = 2; }
        for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | ((unsigned char)s[i + k] & 63);
        out.push_back(cp); i += (size_t)n;
    }
    return out;
}
} // namespace

void AtlasTextes::ajouter(const std::string& t) { if (std::find(textes_.begin(), textes_.end(), t) == textes_.end()) textes_.push_back(t); }

bool AtlasTextes::construire(Renderer& r) {
    std::vector<uint8_t> ttf;
    stbtt_fontinfo info;
    if (!readAsset("fonts/Rajdhani-Bold.ttf", ttf) || !stbtt_InitFont(&info, ttf.data(), 0)) { WFS_ERR("vie : police introuvable"); return false; }
    const int lignes = std::max(1, (int)(textes_.size() + 1) / 2), AH = lignes * LIGNE;
    std::vector<uint8_t> px((size_t)AW * AH * 4, 0);
    for (size_t i = 0; i < (size_t)AW * AH; i++) { px[i * 4] = px[i * 4 + 1] = px[i * 4 + 2] = 255; }
    const float sc = stbtt_ScaleForPixelHeight(&info, (float)PX);
    int asc, desc, gap; stbtt_GetFontVMetrics(&info, &asc, &desc, &gap);
    for (size_t t = 0; t < textes_.size() && t < (size_t)(2 * lignes); t++) {
        const int col = (int)t / lignes, row = (int)t % lignes, x0 = col * (AW / 2) + 8, y0 = row * LIGNE;
        const auto cps = codepoints(textes_[t]);
        float x = (float)x0;
        const int base = y0 + (LIGNE - PX) / 2 + (int)(asc * sc);
        for (size_t k = 0; k < cps.size(); k++) {
            int adv, lsb; stbtt_GetCodepointHMetrics(&info, (int)cps[k], &adv, &lsb);
            int bx0, by0, bx1, by1; stbtt_GetCodepointBitmapBox(&info, (int)cps[k], sc, sc, &bx0, &by0, &bx1, &by1);
            const int w = bx1 - bx0, h = by1 - by0;
            if (w > 0 && h > 0 && x + bx0 + w < col * (AW / 2) + AW / 2 - 8) {
                std::vector<uint8_t> bm((size_t)w * h);
                stbtt_MakeCodepointBitmap(&info, bm.data(), w, h, w, sc, sc, (int)cps[k]);
                for (int yy = 0; yy < h; yy++) for (int xx = 0; xx < w; xx++) {
                    const int X = (int)x + bx0 + xx, Y = base + by0 + yy;
                    if (X < 0 || Y < 0 || X >= AW || Y >= AH) continue;
                    uint8_t& a = px[((size_t)Y * AW + X) * 4 + 3];
                    a = std::max(a, bm[(size_t)yy * w + xx]);
                }
            }
            x += adv * sc + 1.5f;   // léger espacement des lettres (enseignes)
            if (k + 1 < cps.size()) x += stbtt_GetCodepointKernAdvance(&info, (int)cps[k], (int)cps[k + 1]) * sc;
        }
        Texte T;
        T.u0 = (x0 - 4.f) / AW; T.u1 = (x + 4.f) / AW; T.v0 = (float)y0 / AH; T.v1 = (float)(y0 + LIGNE) / AH;
        T.aspect = (x - x0 + 8.f) / LIGNE; T.ok = true;
        places_[textes_[t]] = T;
    }
    img_ = r.vk().uploadTexture(px.data(), AW, AH, false);
    WFS_LOG("vie : %zu textes d'enseignes dans l'atlas", places_.size());
    return img_.img != VK_NULL_HANDLE;
}
const Texte& AtlasTextes::texte(const std::string& t) const {
    static const Texte vide;
    auto it = places_.find(t);
    return it == places_.end() ? vide : it->second;
}
const Material& AtlasTextes::materiau(Renderer& r, unsigned hex, float emission, float seuil) {
    auto it = mats_.find(hex);
    if (it != mats_.end()) return it->second;
    Material m; const vec3 c = lin(hex);
    m.color = {c.x, c.y, c.z, 1}; m.rough = 0.5f; m.emissive = c * emission; m.cutoff = seuil; m.flags = MAT_MASK | MAT_DOUBLE;   // WFS-94 : seuil réglable (circuit)
    return mats_[hex] = r.makeMaterial(m, &img_, nullptr, nullptr);
}
void AtlasTextes::detruire(Renderer& r) { if (img_.img) r.vk().destroy(img_); img_ = Image{}; mats_.clear(); places_.clear(); textes_.clear(); }

} // namespace vie

// ---------------------------------------------------------------- panneaux néon le long de la route (DECOR.board)
namespace {
using namespace vie;
// BOARD_TEXTS de js/world/decor.js
const std::pair<const char*, unsigned> BOARD_TEXTS[] = {{"WAFORSPEED", 0xFF6A00}, {"GARAGE 88", 0x00E5FF}, {"TURBO CAFÉ", 0xFF6A00},
                                                         {"NUIT BLANCHE", 0x00E5FF}, {"PNEUS 24/7", 0x00E5FF}};
constexpr int NB_BOARDS = 5;
struct Panneau { int64_t seg; int side, texte; };

class Enseignes : public System {
public:
    const char* name() const override { return "enseignes"; }
    void init(Renderer& r, const GameView& g) override {
        actif_ = false;
        if (!g.map || !g.track) return;
        const bool ville = !std::strcmp(g.map->id, "ville"), plage = !std::strcmp(g.map->id, "plage");
        if (!ville && !plage) return;   // RG-09 : Ville et Corniche (Grand Prix : WFS-53)
        const int64_t len = (int64_t)g.track->segmentCount();
        if (ville) {   // panneaux du plan des quartiers (city.centre : tous les 130 à 180 segments, côté tiré, pas sur un lampadaire)
            for (const wfs::ville::Panneau& b : wfs::ville::plan(*g.map, len).panneaux) panneaux_.push_back({b.seg, b.side, b.texte});
        } else {   // Corniche : côté terre tous les 180 segments
            Mulberry32 D(g.map->seed * 31 + 23);
            for (int64_t n = 60; n < len - 40; n += 180) {
                if (n % 24 >= 10 && n % 24 <= 14) n += 6;   // pas sur un lampadaire
                panneaux_.push_back({n, -1, (int)(D.next() * NB_BOARDS)});
            }
        }
        for (auto& b : BOARD_TEXTS) atlas_.ajouter(b.first);
        if (!atlas_.construire(r)) { panneaux_.clear(); return; }
        uvPoteau_ = palette_.uv(0x2a2d33); uvFond_ = palette_.uv(0x0b0c14);
        palette_.construire(r);
        for (int i = 0; i < NB_BOARDS; i++) { neons_[i] = neon(r, BOARD_TEXTS[i].second); atlas_.materiau(r, BOARD_TEXTS[i].second); }
        Track& t = *g.track;
        for (const Panneau& b : panneaux_) {   // géométrie construite une fois (≈ 1 panneau / 200 m)
            const Repere p = repere(t, b.seg * SEG);
            const float s = (float)b.side, R = (float)RW;
            boite(fixe_, lateral(p, s * (R + 1), 0.15f), p.r, p.f, 0.3f, 6.2f, 0.3f, uvPoteau_);
            boite(fixe_, lateral(p, s * (R + 5), 0.15f), p.r, p.f, 0.3f, 6.2f, 0.3f, uvPoteau_);
            boite(fixe_, lateral(p, s * (R + 2), 6.f), p.r, p.f, 8.6f, 3.6f, 0.3f, uvFond_);
            boite(lumiere_[b.texte], lateral(p, s * (R + 2), 9.6f), p.r, p.f, 8.6f, 0.14f, 0.34f, {0, 0});
            boite(lumiere_[b.texte], lateral(p, s * (R + 2), 5.88f), p.r, p.f, 8.6f, 0.14f, 0.34f, {0, 0});
            // texte des deux côtés du panneau (face à la circulation dans les deux sens)
            const Texte& T = atlas_.texte(BOARD_TEXTS[b.texte].first);
            const float h = 2.0f, w = std::min(7.8f, h * T.aspect);
            for (int face : {-1, 1}) {
                const vec3 c = lateral(p, s * (R + 2), 7.8f) + p.f * (0.17f * face);
                plan(texte_[b.texte], c, p.r * (float)-face, {0, 1, 0}, p.f * (float)face, w, h, T.u0, T.v0, T.u1, T.v1);
            }
        }
        if (!fixe_.i.empty()) mFixe_ = r.makeMesh(fixe_.v, fixe_.i);
        for (int i = 0; i < NB_BOARDS; i++) {
            if (!lumiere_[i].i.empty()) mLumiere_[i] = r.makeMesh(lumiere_[i].v, lumiere_[i].i);
            if (!texte_[i].i.empty()) mTexte_[i] = r.makeMesh(texte_[i].v, texte_[i].i);
        }
        actif_ = true;
        WFS_LOG("vie : %zu panneaux néon (%s)", panneaux_.size(), g.map->id);
    }
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee) return;
        const mat4 I = mat4::identity();
        if (mFixe_.indexCount) r.draw(mFixe_, palette_.materiau, I);
        for (int i = 0; i < NB_BOARDS; i++) {
            if (mLumiere_[i].indexCount) r.draw(mLumiere_[i], neons_[i], I);
            if (mTexte_[i].indexCount) r.draw(mTexte_[i], atlas_.materiau(r, BOARD_TEXTS[i].second), I);
        }
    }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        for (Mesh* m : {&mFixe_}) if (m->indexCount) r.destroy(*m);
        for (int i = 0; i < NB_BOARDS; i++) { if (mLumiere_[i].indexCount) r.destroy(mLumiere_[i]); if (mTexte_[i].indexCount) r.destroy(mTexte_[i]); }
        atlas_.detruire(r); palette_.detruire(r);
        actif_ = false;
    }
private:
    bool actif_ = false;
    std::vector<Panneau> panneaux_;
    AtlasTextes atlas_; Palette palette_;
    vec2 uvPoteau_, uvFond_;
    Geo fixe_, lumiere_[NB_BOARDS], texte_[NB_BOARDS];
    Mesh mFixe_, mLumiere_[NB_BOARDS], mTexte_[NB_BOARDS];
    Material neons_[NB_BOARDS];
};

} // namespace

WFS_SYSTEM(Enseignes, 107)   // décor (100-199) : après les piétons (106)

} // namespace wfs
