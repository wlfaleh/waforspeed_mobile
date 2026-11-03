// WaForSpeed — végétation réaliste (WFS-34) : port fidèle de vegLimb / vegCore / vegCard / vegBroadleaf (js/world/life.js).
// L'ordre des tirages aléatoires est celui du jeu web (mêmes arbres pour la même graine).
#include "vegetation.h"
#include <algorithm>
#include <cmath>

namespace wfs {
namespace veg {
namespace {

constexpr float TAU = 6.283f;   // valeur du jeu web
constexpr float E = 3.f / 1024.f;

float rnd(Mulberry32& R) { return (float)R.next(); }

// luminosité du jeu web (espace d'affichage) → facteur linéaire porté par la longueur de la normale
vec3 lit(vec3 n, float k) { return n * std::pow(std::max(k, 0.f), 2.2f); }

void vert(Geo& g, vec3 p, vec3 n, float u, float v) { g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u, v}); }

vec3 randUnit(Mulberry32& R) {
    const float z = rnd(R) * 2 - 1, t = rnd(R) * TAU, s = std::sqrt(1 - z * z);
    return {s * std::cos(t), z, s * std::sin(t)};
}

// luminosité de vCol(c, k, R) : j = 0,92..1,08 (le léger écart de vert du jeu web est tiré mais pas appliqué)
float colK(float k, Mulberry32& R) { const float j = 0.92f + rnd(R) * 0.16f; rnd(R); return k * j; }

// repère d'une carte de normale n, tournée de spin autour de n (V reste proche de la verticale)
void basis(vec3 n, float spin, vec3& U, vec3& V) {
    const vec3 t = std::fabs(n.y) < 0.96f ? vec3{0, 1, 0} : vec3{1, 0, 0};
    const vec3 u = normalize(cross(t, n)), v = cross(n, u);
    const float c = std::cos(spin), s = std::sin(spin);
    U = u * c + v * s; V = v * c - u * s;
}

// branche ou tronc : cylindre effilé de a vers b, écorce tachetée (u = 0,25 écorce 1, 0,75 écorce 2 ; luminosité = |normale|)
void limb(Geo& g, vec3 a, vec3 b, float r0, float r1, int sides, Mulberry32& R, float mottle) {
    const vec3 d = normalize(b - a);
    const vec3 e1 = normalize(cross(std::fabs(d.y) < 0.9f ? vec3{0, 1, 0} : vec3{1, 0, 0}, d)), e2 = cross(d, e1);
    struct C { float u, k; };
    auto cc = [&]() { const float u = rnd(R) < 0.45f ? 0.75f : 0.25f; return C{u, 1 - mottle / 2 + rnd(R) * mottle}; };
    std::vector<C> ca(sides + 1), cb(sides + 1);
    for (int i = 0; i <= sides; i++) { ca[i] = cc(); cb[i] = cc(); }
    ca[sides] = ca[0]; cb[sides] = cb[0];
    const uint32_t k0 = (uint32_t)g.v.size();
    for (int i = 0; i <= sides; i++) {
        const float t = (float)i / sides * TAU;
        const vec3 n = e1 * std::cos(t) + e2 * std::sin(t);
        vert(g, a + n * r0, lit(n, ca[i].k), ca[i].u, 0.5f);
        vert(g, b + n * r1, lit(n, cb[i].k), cb[i].u, 0.5f);
    }
    for (int i = 0; i < sides; i++) {
        const uint32_t a0 = k0 + 2 * i, b0 = a0 + 1, a1 = a0 + 2, b1 = a0 + 3;
        g.i.insert(g.i.end(), {a0, a1, b1, a0, b1, b0});
    }
}

// masse au cœur du houppier (vegCore) : évite de voir le ciel à travers. Sans couleur par sommet, une boule lisse se
// voyait (retours QA WFS-34) : elle est donc dessinée avec la texture de feuilles découpée (même matériau que les cartes),
// projetée sur le plan dominant de chaque face, et assombrie (intérieur du houppier) → masse de feuillage dense.
void core(Geo& g, vec3 c, vec3 s, float k) {
    static const float P = 1.618034f;
    static const vec3 V[12] = {{-1, P, 0}, {1, P, 0}, {-1, -P, 0}, {1, -P, 0}, {0, -1, P}, {0, 1, P},
                               {0, -1, -P}, {0, 1, -P}, {P, 0, -1}, {P, 0, 1}, {-P, 0, -1}, {-P, 0, 1}};
    static const uint8_t F[60] = {0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                                  3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1};
    const float inv = 1.f / std::sqrt(1 + P * P);
    const float span = 2.4f * std::max(s.x, std::max(s.y, s.z));   // taille couverte par une case de l'atlas (m)
    auto cell = [&](float a, float b) { return vec2{std::clamp(0.25f + a / span * 0.5f, E, 0.5f - E), std::clamp(0.25f + b / span * 0.5f, E, 0.5f - E)}; };
    for (int f = 0; f < 60; f += 3) {
        vec3 p[3];
        for (int j = 0; j < 3; j++) { const vec3& u = V[F[f + j]]; p[j] = c + vec3{u.x * s.x, u.y * s.y, u.z * s.z} * inv; }
        const vec3 fn = cross(p[1] - p[0], p[2] - p[0]);
        const float ax = std::fabs(fn.x), ay = std::fabs(fn.y), az = std::fabs(fn.z);
        const uint32_t k0 = (uint32_t)g.v.size();
        for (int j = 0; j < 3; j++) {
            const vec3& u = V[F[f + j]];
            const vec3 n = normalize(vec3{u.x / s.x, u.y / s.y + 0.6f / std::max(s.y, 1e-3f), u.z / s.z});
            const vec3 d = p[j] - c;
            const vec2 uv = ay >= ax && ay >= az ? cell(d.x, d.z) : ax >= az ? cell(d.z, -d.y) : cell(d.x, -d.y);
            vert(g, p[j], lit(n, k), uv.x, uv.y);
        }
        g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2});
    }
}

// carte de feuilles (vegCard → vegRibbon → vegStrip) : base au milieu du bas, U = demi-largeur, V = base → sommet.
// Case 0 de l'atlas ; luminosité kb en bas, kt en haut.
void card(Geo& g, vec3 base, vec3 U, vec3 V, vec3 n, float kb, float kt) {
    const float u0 = E, u1 = 0.5f - E, vb = 0.5f - E, vt = E;
    const uint32_t k0 = (uint32_t)g.v.size();
    vert(g, base - U, lit(n, kb), u0, vb);
    vert(g, base + U, lit(n, kb), u1, vb);
    vert(g, base + V + U, lit(n, kt), u1, vt);
    vert(g, base + V - U, lit(n, kt), u0, vt);
    g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
}

// bande de feuillage entre deux bords (vegStrip, texture le long de v), case t de l'atlas, de a0 à a1 en travers
void strip(Geo& g, const vec3* EA, const vec3* EB, int npts, int t, const vec3* nrm, const float* k, float a0, float a1) {
    const int cx = t % 2, cy = t >> 1;
    const float u0 = cx * 0.5f + E, u1 = cx * 0.5f + 0.5f - E, vb = (cy + 1) * 0.5f - E, vt = cy * 0.5f + E;
    auto uv = [&](float s, float w) { return vec2{u0 + (u1 - u0) * w, vb + (vt - vb) * s}; };
    const int n = npts - 1;
    for (int i = 0; i < n; i++) {
        const float s0 = (float)i / n, s1 = (float)(i + 1) / n;
        const vec2 ua = uv(s0, a0), ub = uv(s0, a1), uc = uv(s1, a1), ud = uv(s1, a0);
        const uint32_t k0 = (uint32_t)g.v.size();
        vert(g, EA[i], lit(nrm[i], k[i]), ua.x, ua.y); vert(g, EB[i], lit(nrm[i], k[i]), ub.x, ub.y);
        vert(g, EB[i + 1], lit(nrm[i + 1], k[i + 1]), uc.x, uc.y); vert(g, EA[i + 1], lit(nrm[i + 1], k[i + 1]), ud.x, ud.y);
        g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
    }
}

} // namespace

// vegPalm (js/world/life.js) : stipe en ns tronçons courbés, palmes en demi-cases 2 de l'atlas (folioles relevées en V)
vec3 palmier(Geo& bark, Geo& leaf, Geo& dead, vec3 b, float h, float yaw, float leanIn, Mulberry32& R, bool lo) {
    const float lean = std::fabs(leanIn);
    const int ns = lo ? 4 : 9;
    vec3 prev = b;
    for (int i = 0; i < ns; i++) {
        const float t = (float)(i + 1) / ns, bend = lean * t * t * h * 0.5f;
        const vec3 pt{b.x + std::sin(yaw) * bend, b.y + h * t, b.z + std::cos(yaw) * bend};
        const float r0 = 0.27f - 0.08f * ((float)i / ns) + (i == 0 ? 0.08f : 0), r1 = 0.27f - 0.08f * t;
        limb(bark, prev, pt, r0, r1, lo ? 5 : 8, R, 0.12f);
        prev = pt;
    }
    const vec3 top = prev;
    if (!lo) for (int k = 0; k < 5; k++) rnd(R);   // positions des noix (dessinées par l'appelant) : mêmes tirages
    const int nf = lo ? 7 : 15;
    for (int k = 0; k < nf; k++) {
        const float az = yaw + k * 2.4f + rnd(R) * 0.3f, ca = std::cos(az), sa = std::sin(az);
        const bool young = k < 3, isDead = !young && !lo && k >= nf - 2;
        const float el = young ? 0.9f + rnd(R) * 0.35f : isDead ? -1.1f : 0.15f + rnd(R) * 0.45f;
        const float len = h * (0.36f + rnd(R) * 0.12f) * (young ? 0.75f : 1.f);
        const vec3 dir{ca * std::cos(el), std::sin(el), sa * std::cos(el)}, T{-sa, 0, ca};
        if (!isDead && !young) { rnd(R); rnd(R); }   // teinte de la palme (portée par le matériau)
        const int segs = lo ? 2 : 4;
        vec3 pts[5], L[5], Rr[5], nrm[5]; float kk[5];
        for (int i = 0; i <= segs; i++) {
            const float s = (float)i / segs, w = len * 0.26f * std::sin(3.14159265f * (0.15f + 0.85f * s)) + 0.08f;
            pts[i] = {top.x + dir.x * len * s, top.y + dir.y * len * s - len * s * s * (isDead ? 0.1f : 0.42f), top.z + dir.z * len * s};
            L[i] = pts[i] + T * (-w * 0.94f) + vec3{0, w * 0.32f, 0};
            Rr[i] = pts[i] + T * (w * 0.94f) + vec3{0, w * 0.32f, 0};
            nrm[i] = normalize(vec3{dir.x * 0.4f, 1, dir.z * 0.4f});
            kk[i] = colK(0.68f + 0.32f * s, R);
        }
        Geo& g = isDead ? dead : leaf;
        strip(g, L, pts, segs + 1, 2, nrm, kk, 0.f, 0.5f);    // demi-palme gauche
        strip(g, pts, Rr, segs + 1, 2, nrm, kk, 0.5f, 1.f);   // demi-palme droite
    }
    return top;
}

vec3 hex(uint32_t c) { return {((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f, (c & 255) / 255.f}; }

void broadleaf(const Out& o, vec3 b, float h, Mulberry32& R, const Broadleaf& p, bool lo) {
    Geo& S = *o.bark;
    const float rT = p.trunkR + h * 0.017f, th = h * p.trunkH;
    const float lx = (rnd(R) - 0.5f) * h * 0.05f, lz = (rnd(R) - 0.5f) * h * 0.05f;
    const vec3 top{b.x + lx, b.y + th, b.z + lz};
    limb(S, b, b + vec3{0, 0.6f, 0}, rT * 1.45f, rT * 1.05f, lo ? 5 : 8, R, 0.25f);   // empattement
    limb(S, b + vec3{0, 0.55f, 0}, top, rT * 1.05f, rT * 0.72f, lo ? 5 : 8, R, 0.25f);
    const vec3 cc{top.x, b.y + h * p.cy, top.z};
    const float Rx = h * p.cw, Ry = h * p.ch;
    const int n = lo ? 3 : p.clusters + (int)(rnd(R) * 3);
    for (int k = 0; k < n; k++) {
        const float az = (float)k / n * TAU + rnd(R) * 0.8f;
        const float el = k == 0 && !lo ? 1.f : 0.15f + rnd(R) * 0.7f;
        const float hr = k == 0 && !lo ? 0.1f : 0.62f + rnd(R) * 0.3f;
        const float sp = Rx * hr * (lo ? 0.55f : 0.85f);
        const vec3 cpos{cc.x + std::cos(az) * sp, cc.y + (el - 0.45f) * Ry * 0.9f, cc.z + std::sin(az) * sp};
        const float ri = (lo ? 0.7f : 0.5f + rnd(R) * 0.15f) * Rx;
        if (!lo) limb(S, top + vec3{0, -0.3f, 0}, cpos + vec3{0, -ri * 0.3f, 0}, rT * 0.62f, rT * 0.16f, 5, R, 0.2f);
                core(*o.leaf, cpos + vec3{0, ri * 0.1f, 0}, vec3{ri * 0.42f, ri * 0.36f, ri * 0.42f}, colK(1, R) * 0.55f);
        const int m = lo ? 6 : p.cards;
        for (int j = 0; j < m; j++) {
            vec3 d = randUnit(R); d.y = d.y * 0.9f + 0.05f;
            const vec3 dn = normalize(d);
            const float px = rnd(R), py = rnd(R), pz = rnd(R);
            const vec3 pos{cpos.x + dn.x * ri * (0.45f + px * 0.4f), cpos.y + dn.y * ri * 0.75f * (0.45f + py * 0.4f),
                           cpos.z + dn.z * ri * (0.45f + pz * 0.4f)};
            const float s = ri * (lo ? 1.3f : 0.95f + rnd(R) * 0.35f);
            const vec3 nn = normalize(dn + randUnit(R) * 0.35f);
            vec3 U, V; basis(nn, (rnd(R) - 0.5f) * 1.3f, U, V);
            // normale d'éclairage : vers l'extérieur du houppier (forme globale) + un peu de la masse locale + ciel
            const vec3 ln = normalize(vec3{(pos.x - cc.x) / Rx + dn.x * 0.6f, (pos.y - cc.y) / Ry * 0.8f + dn.y * 0.6f + 0.3f,
                                           (pos.z - cc.z) / Rx + dn.z * 0.6f});
            const float hk = std::clamp((pos.y - (cc.y - Ry)) / (2 * Ry), 0.f, 1.f), out = std::max(0.f, dot(ln, dn));
            const float kc = colK(0.62f + 0.3f * hk + 0.14f * out, R);
            card(*o.leaf, pos - V * (s * 0.55f), U * (s * 0.55f), V * (s * 1.1f), ln, kc * 0.82f, kc);
        }
    }
}

void platane(Geo* bark, Geo* core, Geo* leaf[2], vec3 b, float itH, Mulberry32& R, bool lod) {
    const float h = itH * 1.4f + 1.2f;
    const int ti = std::min(1, (int)(rnd(R) * 2));   // pick(VEG_TINT.plane, R)
    Broadleaf p; p.trunkR = 0.06f; p.trunkH = 0.45f; p.cw = 0.36f; p.ch = 0.26f; p.cy = 0.7f; p.clusters = 6; p.cards = 11;
    broadleaf({bark, core, leaf[ti]}, b, h, R, p, lod);
}

} // namespace veg
} // namespace wfs
