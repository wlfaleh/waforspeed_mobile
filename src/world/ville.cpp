// WaForSpeed — Ville (ticket WFS-06, épic E2) : immeubles, lampadaires et arbres d'alignement le long de la route.
// Port simplifié du quartier « centre » de js/world/decor.js (mêmes pas de placement et mêmes dimensions) :
//   immeubles tous les 11 segments de chaque côté, en retrait de RW + 6,5 m, hauteur 12-105 m ;
//   lampadaires tous les 24 segments en alternance ; arbres d'alignement tous les 16 segments sur le trottoir (RW + 3,6 m).
// Le décor est généré par la graine de la carte, puis construit par morceaux de CH segments (comme la route),
// en parallèle sur les threads de travail ; chaque morceau = 1 maillage par matériau (≈ 6 appels de dessin).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../render/renderer.h"

namespace wfs {
namespace {

enum Part { P_FACADE_A, P_FACADE_B, P_METAL, P_TRUNK, P_LEAF, P_LAMP, P_COUNT };

struct Building { int64_t seg; int side; float len, depth, height; int tint; };
struct Lamp { int64_t seg; int side; };
struct Tree { int64_t seg; int side; float h; };

constexpr int AHEAD = 22, BEHIND = 2;
constexpr float SET_BACK = 6.5f;     // retrait des façades par rapport au bord de la chaussée (jeu web)

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };

// repère local de la route au segment a : origine sur l'axe, r = vers la droite, f = vers l'avant
struct Frame { vec3 o, r, f; };
Frame frameAt(Track& t, double s) {
    TrackPoint p = t.point(s, 0);
    return {{(float)p.x, (float)p.y, (float)p.z}, {(float)std::cos(p.th), 0, (float)std::sin(p.th)}, {(float)std::sin(p.th), 0, (float)-std::cos(p.th)}};
}

void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    uint32_t k = (uint32_t)g.v.size();
    for (auto [p, u] : {std::pair{a, ua}, {b, ub}, {c, uc}, {d, ud}}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}

// boîte orientée : centre au sol c, axes r / f, demi-largeur hx (selon r), demi-longueur hz (selon f), hauteur h
// uv des côtés en mètres / tuile (façades), dessus avec uv minuscule (toit uni)
void box(Geo& g, vec3 c, vec3 r, vec3 f, float hx, float hz, float y0, float h, float tile) {
    vec3 up{0, 1, 0};
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    const float y1 = y0 + h;
    // 4 côtés
    struct S { float x0, z0, x1, z1; vec3 n; float w; } sides[4] = {
        {-hx, -hz, hx, -hz, -f, 2 * hx}, {hx, -hz, hx, hz, r, 2 * hz}, {hx, hz, -hx, hz, f, 2 * hx}, {-hx, hz, -hx, -hz, -r, 2 * hz}};
    for (auto& s : sides) {
        float u1 = s.w / tile, v0 = y0 / tile, v1 = y1 / tile;
        quad(g, P(s.x0, y0, s.z0), P(s.x1, y0, s.z1), P(s.x1, y1, s.z1), P(s.x0, y1, s.z0), s.n, {0, v1}, {u1, v1}, {u1, v0}, {0, v0});
    }
    quad(g, P(-hx, y1, -hz), P(hx, y1, -hz), P(hx, y1, hz), P(-hx, y1, hz), up, {0.01f, 0.01f}, {0.02f, 0.01f}, {0.02f, 0.02f}, {0.01f, 0.02f});
}

// cylindre vertical (8 côtés) : troncs, mâts
void cylinder(Geo& g, vec3 base, float rad, float h, int n = 8) {
    for (int k = 0; k < n; k++) {
        float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, base + d0 * rad, base + d1 * rad, base + d1 * rad + vec3{0, h, 0}, base + d0 * rad + vec3{0, h, 0},
             normalize(d0 + d1), {0, 0}, {1, 0}, {1, 1}, {0, 1});
    }
}

// boule facettée (octaèdre subdivisé, normales lisses) : feuillages
// forme de base calculée une seule fois. Initialisation d'une variable statique locale : garantie sans course par le
// C++ (WFS-32 : l'ancien « if (unit.empty()) » était exécuté par plusieurs threads à la fois → mémoire corrompue).
struct UnitBlob { std::vector<vec3> unit; std::vector<uint32_t> tri; };
static UnitBlob makeUnitBlob() {
    UnitBlob ub;
    {
        std::vector<vec3> v = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        std::vector<uint32_t> t = {2, 4, 0, 2, 0, 5, 2, 5, 1, 2, 1, 4, 3, 0, 4, 3, 5, 0, 3, 1, 5, 3, 4, 1};
        for (int lvl = 0; lvl < 2; lvl++) {
            std::vector<uint32_t> nt; std::map<std::pair<uint32_t, uint32_t>, uint32_t> mid;
            auto m = [&](uint32_t a, uint32_t b) {
                auto key = std::minmax(a, b); auto it = mid.find(key); if (it != mid.end()) return it->second;
                v.push_back(normalize((v[a] + v[b]) * 0.5f)); return mid[key] = (uint32_t)v.size() - 1;
            };
            for (size_t i = 0; i < t.size(); i += 3) {
                uint32_t a = t[i], b = t[i + 1], cc = t[i + 2], ab = m(a, b), bc = m(b, cc), ca = m(cc, a);
                nt.insert(nt.end(), {a, ab, ca, ab, b, bc, ca, bc, cc, ab, bc, ca});
            }
            t = nt;
        }
        ub.unit = v; ub.tri = t;
    }
    return ub;
}
void blob(Geo& g, vec3 c, vec3 s, uint32_t seed) {
    static const UnitBlob shape = makeUnitBlob();
    const std::vector<vec3>& unit = shape.unit;
    const std::vector<uint32_t>& tri = shape.tri;
    Mulberry32 R(seed);
    uint32_t k = (uint32_t)g.v.size();
    for (const vec3& u : unit) {
        float j = 0.86f + 0.28f * (float)R.next();   // bosses irrégulières
        vec3 p = c + vec3{u.x * s.x * j, u.y * s.y * j, u.z * s.z * j};
        vec3 n = normalize(vec3{u.x / s.x, u.y / s.y, u.z / s.z});
        g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x * 0.5f + 0.5f, u.y * 0.5f + 0.5f});
    }
    for (uint32_t x : tri) g.i.push_back(k + x);
}

// façade d'immeuble : béton teinté, grille de fenêtres (2 par tuile de 4 m), quelques vitres plus claires
std::vector<uint8_t> facadeTexture(int variant, int& W, int& H) {
    W = H = 256;
    std::vector<uint8_t> px((size_t)W * H * 4);
    Mulberry32 R(77 + variant * 13);
    const float wall[2][3] = {{0.55f, 0.52f, 0.48f}, {0.36f, 0.38f, 0.42f}};
    float lit[64];
    for (float& l : lit) l = (float)R.next();
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float u = (x + 0.5f) / W * 2, v = (y + 0.5f) / H * 2;   // 2x2 fenêtres par tuile
        int cx = (int)u, cy = (int)v; float fu = u - cx, fv = v - cy;
        bool window = fu > 0.16f && fu < 0.84f && fv > 0.22f && fv < 0.82f;
        bool frame = window && (std::fabs(fu - 0.5f) < 0.025f);
        float n = 0.92f + 0.08f * (float)R.next();
        float c[3];
        if (window && !frame) {
            float l = lit[(cx + cy * 2 + variant * 4) & 63];
            float g = 0.06f + 0.1f * (1 - fv) + (l > 0.82f ? 0.25f : 0);   // reflet du ciel en haut de la vitre
            c[0] = g * 0.8f; c[1] = g * 0.9f; c[2] = g * 1.1f;
        } else for (int k = 0; k < 3; k++) c[k] = wall[variant][k] * n * (fv < 0.12f ? 0.8f : 1);
        uint8_t* p = &px[((size_t)y * W + x) * 4];
        for (int k = 0; k < 3; k++) p[k] = (uint8_t)(std::pow(std::min(1.f, c[k]), 1 / 2.2f) * 255);
        p[3] = 255;
    }
    return px;
}

class Ville : public System {
public:
    const char* name() const override { return "ville"; }

    void init(Renderer& r, const GameView& g) override {
        active_ = g.map && std::strcmp(g.map->id, "ville") == 0;
        if (!active_) return;
        r_ = &r;
        // matériaux
        for (int v = 0; v < 2; v++) {
            int W, H; auto px = facadeTexture(v, W, H);
            facadeTex_[v] = r.vk().uploadTexture(px.data(), (uint32_t)W, (uint32_t)H, true);
            Material m; m.rough = 0.75f; m.metal = 0.f;
            mats_[P_FACADE_A + v] = r.makeMaterial(m, &facadeTex_[v], nullptr, nullptr);
        }
        auto plain = [&](Part p, vec3 c, float rough, float metal, vec3 em = {0, 0, 0}) {
            Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal; m.emissive = em;
            mats_[p] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        plain(P_METAL, {0.08f, 0.09f, 0.11f}, 0.45f, 0.8f);
        plain(P_TRUNK, {0.16f, 0.1f, 0.06f}, 0.9f, 0.f);
        plain(P_LEAF, {0.07f, 0.17f, 0.05f}, 0.85f, 0.f);
        plain(P_LAMP, {1.f, 0.95f, 0.85f}, 0.3f, 0.f, {0.6f, 0.55f, 0.45f});
        place(*g.track);
    }

    // placement de tout le décor de la piste, à partir de la graine de la carte (comme generateDecor)
    void place(Track& t) {
        const int64_t len = (int64_t)t.segmentCount();
        Mulberry32 D(t.map()->seed * 31 + 7);
        for (int side : {-1, 1}) for (int64_t n = side < 0 ? 0 : 5; n < len - 11; n += 11) {
            if (D.next() < 0.08) continue;
            double tall = D.next();
            Building b{n, side, (float)(11 * SEG_M - 0.6 - D.next() * 2), (float)(10 + D.next() * 14),
                       (float)(tall < 0.2 ? 60 + D.next() * 45 : tall < 0.7 ? 26 + D.next() * 30 : 12 + D.next() * 14), (int)(D.next() * 2)};
            blds_.push_back(b);
        }
        std::vector<char> lampAt((size_t)len, 0);
        for (int64_t n = 12; n < len; n += 24) { int side = (n / 24) % 2 ? 1 : -1; lamps_.push_back({n, side}); lampAt[(size_t)n] = 1; }
        for (int side : {-1, 1}) for (int64_t n = side < 0 ? 4 : 10; n + 1 < len; n += 16)
            if (!lampAt[(size_t)n] && !lampAt[(size_t)(n + 1)] && D.next() > 0.2) trees_.push_back({n, side, (float)(5 + D.next() * 2.5)});
        WFS_LOG("Ville : %zu immeubles, %zu lampadaires, %zu arbres", blds_.size(), lamps_.size(), trees_.size());
    }

    // géométrie d'un morceau (segments [c*CH, (c+1)*CH)) — appelée en parallèle, lit seulement la piste
    void buildChunk(Track& t, int64_t c, Geo out[P_COUNT]) {
        const int64_t len = (int64_t)t.segmentCount();
        const int64_t a0 = c * CH, a1 = a0 + CH;
        const int64_t lap = (int64_t)std::floor((double)a0 / len) * len;   // la piste se répète (tours)
        auto inChunk = [&](int64_t seg, int64_t& abs) {
            for (int64_t L : {lap, lap + len}) { int64_t a = seg + L; if (a >= a0 && a < a1) { abs = a; return true; } }
            return false;
        };
        const float R = (float)RW;
        for (const Building& b : blds_) {
            int64_t a; if (!inChunk(b.seg, a)) continue;
            double mid = (a + b.len / SEG_M / 2) * SEG;
            Frame fr = frameAt(t, mid);
            vec3 c = fr.o + fr.r * (b.side * (R + SET_BACK + b.depth / 2));
            box(out[P_FACADE_A + b.tint], c, fr.r, fr.f, b.depth / 2, b.len / 2, -3.f, b.height + 3.f, 4.f);
        }
        for (const Lamp& l : lamps_) {
            int64_t a; if (!inChunk(l.seg, a)) continue;
            Frame fr = frameAt(t, a * SEG);
            vec3 base = fr.o + fr.r * (l.side * (R + 0.8f)) + vec3{0, 0.15f, 0};
            cylinder(out[P_METAL], base, 0.09f, 7.4f, 6);
            // bras vers la chaussée, tête lumineuse
            vec3 armC = fr.o + fr.r * (l.side * (R - 0.55f)) + vec3{0, 0.15f, 0};
            box(out[P_METAL], armC, fr.r, fr.f, 1.4f, 0.06f, 7.3f, 0.12f, 1.f);
            vec3 headC = fr.o + fr.r * (l.side * (R - 1.6f)) + vec3{0, 0.15f, 0};
            box(out[P_LAMP], headC, fr.r, fr.f, 0.45f, 0.18f, 7.18f, 0.12f, 1.f);
        }
        for (const Tree& tr : trees_) {
            int64_t a; if (!inChunk(tr.seg, a)) continue;
            Frame fr = frameAt(t, a * SEG);
            vec3 base = fr.o + fr.r * (tr.side * (R + 3.6f)) + vec3{0, 0.15f, 0};
            box(out[P_METAL], base, fr.r, fr.f, 0.65f, 0.65f, -0.02f, 0.05f, 1.f);   // grille d'arbre
            cylinder(out[P_TRUNK], base, 0.14f, tr.h * 0.62f, 7);
            uint32_t seed = (uint32_t)(tr.seg * 2654435761u) ^ (uint32_t)(tr.side + 2);
            blob(out[P_LEAF], base + vec3{0, tr.h * 0.68f, 0}, {tr.h * 0.3f, tr.h * 0.34f, tr.h * 0.3f}, seed);
            blob(out[P_LEAF], base + fr.f * 0.3f + fr.r * 0.4f + vec3{0, tr.h * 0.86f, 0}, {tr.h * 0.2f, tr.h * 0.2f, tr.h * 0.2f}, seed + 1);
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!active_) return;
        frame_++;
        Track& t = *g.track;
        const int64_t pc = (int64_t)std::floor(g.player->dist / SEG) / CH;
        for (auto it = chunks_.begin(); it != chunks_.end();) {
            if (it->first < pc - BEHIND || it->first > pc + AHEAD) {
                for (Mesh& m : it->second) if (m.vb.buf) graveyard_.push_back({frame_, m});
                it = chunks_.erase(it);
            } else ++it;
        }
        std::vector<int64_t> missing;
        for (int64_t c = pc - BEHIND; c <= pc + AHEAD; c++) if (!chunks_.count(c)) missing.push_back(c);
        if (!missing.empty()) {
            // poses calculées ici (thread principal) ; les threads ne font que lire la piste
            t.pose(missing.front() * CH - 2); t.pose((missing.back() + 1) * CH + 40);
            std::vector<std::array<Geo, P_COUNT>> cpu(missing.size());
            jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunk(t, missing[i], cpu[i].data()); });
            for (size_t i = 0; i < missing.size(); i++) {
                auto& meshes = chunks_[missing[i]];
                for (int p = 0; p < P_COUNT; p++) if (!cpu[i][p].i.empty()) meshes[p] = r.makeMesh(cpu[i][p].v, cpu[i][p].i, true);
            }
        }
        for (auto it = graveyard_.begin(); it != graveyard_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        for (auto& [k, meshes] : chunks_) for (int p = 0; p < P_COUNT; p++) if (meshes[p].indexCount) r.draw(meshes[p], mats_[p], I);
    }

    void shutdown(Renderer& r) override {
        if (!active_) return;
        r.vk().waitIdle();
        for (auto& [k, meshes] : chunks_) for (Mesh& m : meshes) if (m.vb.buf) r.destroy(m);
        chunks_.clear();
        for (auto& g : graveyard_) r.destroy(g.second);
        graveyard_.clear();
        for (Image& im : facadeTex_) r.vk().destroy(im);
        active_ = false;
    }

private:
    bool active_ = false;
    Renderer* r_ = nullptr;
    std::vector<Building> blds_; std::vector<Lamp> lamps_; std::vector<Tree> trees_;
    Material mats_[P_COUNT];
    Image facadeTex_[2];
    std::map<int64_t, std::array<Mesh, P_COUNT>> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
};

} // namespace

WFS_SYSTEM(Ville, 100)

} // namespace wfs
