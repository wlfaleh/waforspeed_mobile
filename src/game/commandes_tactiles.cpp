// WaForSpeed — commandes tactiles de la course (WFS-98) : voir commandes_tactiles.h.
#include "commandes_tactiles.h"
#include <algorithm>
#include <cmath>
#include "../render/renderer.h"
#include "../ui/ecrans/reglages.h"   // WFS-75 : miroirX, margesCommandes (lecture seule)
#include "../ui/text.h"

namespace wfs {

namespace {

constexpr int MAXP = 64;   // sommets d'un polygone de dessin2d

vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
vec2 operator*(vec2 a, float s) { return {a.x * s, a.y * s}; }
float dot2(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
float cross2(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
vec2 unit(vec2 a) { const float l = std::sqrt(dot2(a, a)); return l > 1e-6f ? a * (1.f / l) : vec2{0, 0}; }
vec2 polaire(float cx, float cy, float rayon, float a) { return {cx + std::sin(a) * rayon, cy - std::cos(a) * rayon}; }

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
vec4 alpha(vec4 c, float a) { c.w = a; return c; }
vec4 melange(const vec4& a, const vec4& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

UiVertex sv(vec2 p, const vec4& c, float a) { return {p.x, p.y, -1, -1, c.x, c.y, c.z, a}; }
UiVertex sv(vec2 p, const vec4& c) { return sv(p, c, c.w); }
void quad(Renderer& r, const UiVertex& a, const UiVertex& b, const UiVertex& c, const UiVertex& d) {
    const UiVertex q[4] = {a, b, c, d};
    r.uiQuad(q);   // triangles (a, b, c) et (a, c, d)
}
void tri(Renderer& r, const UiVertex& a, const UiVertex& b, const UiVertex& c) { quad(r, a, b, c, c); }

// segments d'un arc de rayon r couvrant l'angle da : ≈ 3 px par segment, entre 12 et 64 pour un tour
int segments(float rayon, float da) {
    const float tour = std::clamp(2 * PI * rayon / 3, 12.f, 64.f);
    return std::max(2, (int)std::ceil(std::fabs(da) / (2 * PI) * tour));
}

// normales extérieures des arêtes d'un polygone horaire à l'écran
void normales(const vec2* p, int n, vec2* nr) {
    for (int i = 0; i < n; i++) { const vec2 e = p[(i + 1) % n] - p[i]; nr[i] = unit({e.y, -e.x}); }
}
// sommet i décalé de d (positif : vers l'extérieur) le long de la bissectrice, onglet borné à ×3
vec2 decale(const vec2* p, const vec2* nr, int n, int i, float d) {
    const vec2 a = nr[(i + n - 1) % n], b = nr[i], m = unit(a + b);
    if (m.x == 0 && m.y == 0) return p[i] + b * d;
    return p[i] + m * (d * std::min(3.f, 1.f / std::max(dot2(m, b), 1e-3f)));
}

// frange extérieure d'un polygone quelconque (horaire, convexe ou non) : arête par arête, coins saillants complétés
void frangeAretes(Renderer& r, const vec2* p, int n, float d, const vec4& c) {
    vec2 nr[MAXP];
    normales(p, n, nr);
    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n, l = (j + 1) % n;
        quad(r, sv(p[i], c), sv(p[i] + nr[i] * d, c, 0), sv(p[j] + nr[i] * d, c, 0), sv(p[j], c));
        if (cross2(p[j] - p[i], p[l] - p[j]) > 0) tri(r, sv(p[j], c), sv(p[j] + nr[i] * d, c, 0), sv(p[j] + nr[j] * d, c, 0));
    }
}

// jetons du skill design-street-racing (codes sRGB)
const vec4 TEXTE = hex(0xF2F4F8), SUR_ACCENT = hex(0x0A0B10), ACCENT = hex(0xFF6A00), DANGER = hex(0xFF1F4B),
           ICONE_FREIN = hex(0xFF6A85), SUCCES = hex(0xB6FF00);

// .nt-icon (index.html l. 93, viewBox 24) : flamme aplatie (courbes de Bézier découpées à ≈ 0,1 unité) et triangulée.
// Le tracé se recoupe en (12,84 ; 5,87) : corps + pointe, deux polygones horaires.
constexpr vec2 FLAMME[40] = {
    {13.10f, 7.15f}, {13.70f, 8.40f}, {13.81f, 7.43f}, {14.14f, 6.49f}, {14.67f, 5.60f}, {15.40f, 4.80f}, {15.57f, 5.96f},
    {15.99f, 7.34f}, {17.99f, 11.16f}, {18.65f, 13.05f}, {18.79f, 14.12f}, {18.76f, 15.19f}, {18.62f, 15.86f}, {18.23f, 16.82f},
    {17.66f, 17.68f}, {16.92f, 18.40f}, {16.34f, 18.78f}, {15.40f, 19.20f}, {15.44f, 18.24f}, {15.16f, 17.36f}, {14.82f, 16.84f},
    {13.45f, 15.48f}, {12.85f, 14.75f}, {12.39f, 13.89f}, {12.20f, 13.20f}, {11.27f, 14.12f}, {10.60f, 15.18f}, {10.36f, 16.12f},
    {10.40f, 17.20f}, {9.82f, 16.84f}, {9.08f, 16.13f}, {8.55f, 15.26f}, {8.25f, 14.28f}, {8.20f, 13.60f}, {8.31f, 12.50f},
    {8.49f, 11.84f}, {8.91f, 10.94f}, {9.68f, 9.85f}, {12.07f, 7.12f}, {12.84f, 5.87f}};
constexpr unsigned char FLAMME_TRI[38][3] = {
    {1, 2, 3}, {1, 3, 4}, {1, 4, 5}, {1, 5, 6}, {1, 6, 7}, {1, 7, 8}, {1, 8, 9}, {1, 9, 10}, {1, 10, 11}, {1, 11, 12},
    {1, 12, 13}, {1, 13, 14}, {0, 1, 14}, {0, 14, 15}, {0, 15, 16}, {16, 17, 18}, {0, 16, 18}, {0, 18, 19}, {0, 19, 20},
    {39, 0, 20}, {39, 20, 21}, {39, 21, 22}, {39, 22, 23}, {39, 23, 24}, {39, 24, 25}, {39, 25, 26}, {39, 26, 27},
    {27, 28, 29}, {39, 27, 29}, {39, 29, 30}, {39, 30, 31}, {39, 31, 32}, {39, 32, 33}, {33, 34, 35}, {33, 35, 36},
    {33, 36, 37}, {33, 37, 38}, {33, 38, 39}};
constexpr vec2 POINTE[7] = {{12.84f, 5.87f}, {12.81f, 5.15f}, {12.88f, 4.14f}, {13.20f, 2.50f}, {13.31f, 3.32f}, {13.31f, 4.08f}, {13.13f, 5.11f}};
constexpr unsigned char POINTE_TRI[5][3] = {{6, 0, 1}, {6, 1, 2}, {6, 2, 3}, {6, 3, 4}, {4, 5, 6}};

// polygone rempli (triangles donnés) + frange : icône pleine (fill)
template <int N, int T>
void icone(Renderer& r, const vec2 (&pts)[N], const unsigned char (&tris)[T][3], float cx, float cy, float s, const vec4& c) {
    vec2 q[N];
    for (int i = 0; i < N; i++) q[i] = {cx + (pts[i].x - 12) * s, cy + (pts[i].y - 12) * s};
    for (const auto& t : tris) tri(r, sv(q[t[0]], c), sv(q[t[1]], c), sv(q[t[2]], c));
    frangeAretes(r, q, N, 1.f, c);
}

// .nt-icon : flamme et barre « M6.6 19.5h10.8v2H6.6z », centrées en (cx, cy), s = pixels par unité du viewBox
void iconeNitro(Renderer& r, float cx, float cy, float s, const vec4& c) {
    icone(r, FLAMME, FLAMME_TRI, cx, cy, s, c);
    icone(r, POINTE, POINTE_TRI, cx, cy, s, c);
    auto P = [&](float x, float y) { return vec2{cx + (x - 12) * s, cy + (y - 12) * s}; };
    const vec2 barre[4] = {P(6.6f, 19.5f), P(17.4f, 19.5f), P(17.4f, 21.5f), P(6.6f, 21.5f)};
    dessin2d::polygone(r, barre, 4, dessin2d::uni(c));
}
// icône du gaz (index.html l. 103, « m6 12.5 6-6 6 6M6 18.5l6-6 6 6 ») : double chevron montant, trait 2,4
void iconeGaz(Renderer& r, float cx, float cy, float s, const vec4& c) {
    auto P = [&](float x, float y) { return vec2{cx + (x - 12) * s, cy + (y - 12) * s}; };
    const vec2 a[3] = {P(6, 12.5f), P(12, 6.5f), P(18, 12.5f)}, b[3] = {P(6, 18.5f), P(12, 12.5f), P(18, 18.5f)};
    dessin2d::trait(r, a, 3, 2.4f * s, c);
    dessin2d::trait(r, b, 3, 2.4f * s, c);
}
// icône du frein (l. 98) : disque « circle r 5,6 » et deux étriers « M5.2 6.2a9 9 0 0 0 0 11.6M18.8 6.2a9 9 0 0 1 0 11.6 »
// (arcs de rayon 9 centrés en (12,08 ; 12) et (11,92 ; 12), ±40° autour de l'horizontale), trait 2,4
void iconeFrein(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = 2.4f * s, deg = PI / 180;
    dessin2d::anneau(r, cx, cy, (5.6f - 1.2f) * s, (5.6f + 1.2f) * s, c);
    dessin2d::arc(r, cx + 0.08f * s, cy, 9 * s, ep, 229.9f * deg, 310.1f * deg, c);
    dessin2d::arc(r, cx - 0.08f * s, cy, 9 * s, ep, 49.9f * deg, 130.1f * deg, c);
}

// RG-01 / RG-02 : pédale .pedal (coins coupés 10 px, fond dégradé, bordure 1 px, icône 26 px et libellé 13 px en bas)
void pedale(Renderer& r, const Cadre& c, float k, bool gaz, bool appui) {
    const float cx = c.x + c.w / 2, bas = c.y + c.h, w2 = c.w / 2, h = c.h, b = 10 * k;
    // .pedal.down : perspective(420px) rotateX(16deg) scale(.97), origine au milieu du bas ; u depuis le centre, v vers le haut
    const float sn = std::sin(16 * PI / 180), cs = std::cos(16 * PI / 180), P = 420 * k;
    auto echelle = [&](float v) { return appui ? 0.97f * P / (P + 0.97f * v * sn) : 1.f; };
    auto place = [&](float u, float v) -> vec2 {
        if (!appui) return {cx + u, bas - v};
        const float f = echelle(v);
        return {cx + u * f, bas - v * cs * f};
    };
    auto eclat = [&](vec4 v) {   // .pedal.down : filter: brightness(1.15)
        if (appui) { v.x = std::min(1.f, v.x * 1.15f); v.y = std::min(1.f, v.y * 1.15f); v.z = std::min(1.f, v.z * 1.15f); }
        return v;
    };
    const vec2 poly[6] = {place(-w2 + b, h), place(w2 - b, h), place(w2, h - b), place(w2, 0), place(-w2, 0), place(-w2, h - b)};
    const float haut = poly[0].y;
    if (appui && gaz) {   // .pedal--gas.down : --glow-accent (0 0 12px rgba(255,106,0,.55), 0 0 32px rgba(255,106,0,.25))
        dessin2d::halo(r, poly, 6, 30 * k, alpha(ACCENT, 0.14f));
        dessin2d::halo(r, poly, 6, 12 * k, alpha(ACCENT, 0.3f));
    }
    dessin2d::polygone(r, poly, 6, {eclat({44 / 255.f, 48 / 255.f, 66 / 255.f, 0.78f}), eclat({16 / 255.f, 18 / 255.f, 26 / 255.f, 0.86f}), haut, bas});
    if (appui) {   // .pedal-fill : remplissage de bas en haut (linear-gradient(0deg, …))
        const vec4 f = gaz ? ACCENT : DANGER;
        dessin2d::polygone(r, poly, 6, {eclat(alpha(f, gaz ? 0.25f : 0.2f)), eclat(alpha(f, gaz ? 0.9f : 0.85f)), haut, bas});
    }
    dessin2d::contour(r, poly, 6, k, appui ? (gaz ? ACCENT : DANGER) : (gaz ? alpha(ACCENT, 0.55f) : vec4{1, 1, 1, 0.12f}));
    // padding-bottom 10 px + bordure 1 px ; libellé (line-height 1) puis icône, 6 px d'écart (flex, justify-content: flex-end)
    const vec4 ci = appui ? SUR_ACCENT : (gaz ? ACCENT : ICONE_FREIN), ct = appui ? SUR_ACCENT : TEXTE;
    const vec2 io = place(0, 43 * k);
    const float s = 26 * k / 24 * echelle(43 * k);
    if (gaz) iconeGaz(r, io.x, io.y, s, ci); else iconeFrein(r, io.x, io.y, s, ci);
    const vec2 lo = place(0, 17.9f * k);   // milieu des capitales du libellé
    const float px = std::round(std::max(13.f, 13 * k)) * echelle(17.9f * k);
    float ly = lo.y - 0.4768f * px;
    if (!appui) ly = std::round(ly);
    ui::text(r, lo.x, ly, px, gaz ? "GAZ" : "FREIN", ct, ui::CENTRE, 0.14f, false);
}

// RG-03 : bouton .nitro-touch (disque 68 px, anneau de charge vert acide .nt-ring, flamme .nt-icon, libellé .nt-label)
void boutonNitro(Renderer& r, float cx, float cy, float k, const EtatCommandes& e) {
    const int nq = (int)std::lround(std::clamp(e.reserve, 0.f, 1.f) * 40);   // pas de 2,5 % (hud.js)
    const bool vide = nq < 2;
    auto filtre = [&](vec4 v) {   // .nitro-touch.empty : grayscale(.8) brightness(.8)
        if (!vide) return v;
        const float l = 0.2126f * v.x + 0.7152f * v.y + 0.0722f * v.z;
        return vec4{(v.x * 0.2f + l * 0.8f) * 0.8f, (v.y * 0.2f + l * 0.8f) * 0.8f, (v.z * 0.2f + l * 0.8f) * 0.8f, v.w};
    };
    const float sc = e.nitro ? 0.93f : 1.f, u = k * sc, R = 34 * u;   // .nitro-touch.down : scale(.93)
    const vec4 vert = filtre(SUCCES);
    if (e.nitroActive) {   // .nitro-touch.active : 0 0 16px rgba(182,255,0,.6), 0 0 36px rgba(182,255,0,.25)
        dessin2d::couronne(r, cx, cy, R, R + 36 * u, alpha(vert, 0.12f), alpha(vert, 0));
        dessin2d::couronne(r, cx, cy, R, R + 16 * u, alpha(vert, 0.3f), alpha(vert, 0));
    } else {               // box-shadow 0 4px 16px rgba(0,0,0,.45)
        dessin2d::couronne(r, cx, cy + 4 * u, 30 * u, 48 * u, {0, 0, 0, 0.3f}, {0, 0, 0, 0});
    }
    // radial-gradient(circle at 50% 38%, rgba(44,50,40,.9), rgba(14,16,22,.9) 68%)
    dessin2d::disqueRadial(r, cx, cy, R, cx, cy - 8.16f * u, 36.8f * u, filtre({44 / 255.f, 50 / 255.f, 40 / 255.f, 0.9f}),
                           filtre({14 / 255.f, 16 / 255.f, 22 / 255.f, 0.9f}));
    // conic-gradient(var(--success) n × 360deg, rgba(255,255,255,.1) 0), masqué sous 27,5 px
    const float a = 2 * PI * nq / 40.f;
    if (nq > 0) dessin2d::anneau(r, cx, cy, 27.5f * u, R, vert, 0, a);
    if (nq < 40) dessin2d::anneau(r, cx, cy, 27.5f * u, R, filtre({1, 1, 1, 0.1f}), a, 2 * PI);
    iconeNitro(r, cx, cy - 4 * u, 26 * u / 24, vert);   // 26 px, margin-top −8 px
    const float px = std::round(std::max(11.f, 10 * k)) * sc;   // 10 px du web, 11 au moins pour rester lisible
    ui::text(r, cx, cy + 14.3f * u - 0.4768f * px, px, "NITRO", vert, ui::CENTRE, 0.14f, false);
}

} // namespace

void commandesDessiner(Renderer& r, float W, float H, const Marges& marges, const EtatCommandes& e) {
    const DispositionCommandes d = dispositionCommandes(W, H, ecrans::margesCommandes(marges));
    // WFS-75 : chaque commande passe en miroir autour de son centre (gaucher), sans être retournée
    auto miroir = [&](Cadre c) { c.x = ecrans::miroirX(c.cx(), W) - c.w / 2; return c; };
    pedale(r, miroir(d.frein), d.k, false, e.frein);
    pedale(r, miroir(d.gaz), d.k, true, e.gaz);
    const Cadre n = miroir(d.nitro);
    boutonNitro(r, n.cx(), n.cy(), d.k, e);
}

// ---------------------------------------------------------------- dessin2d
namespace dessin2d {

vec4 Teinte::en(float y) const { return melange(haut, bas, y1 > y0 ? clampf((y - y0) / (y1 - y0), 0.f, 1.f) : 0.f); }
Teinte uni(const vec4& c) { return {c, c, 0, 1}; }

void polygone(Renderer& r, const vec2* p, int n, const Teinte& t) {
    if (n < 3 || n > MAXP) return;
    vec2 nr[MAXP], in[MAXP], ex[MAXP];
    normales(p, n, nr);
    for (int i = 0; i < n; i++) { in[i] = decale(p, nr, n, i, -0.5f); ex[i] = decale(p, nr, n, i, 0.5f); }
    for (int i = 1; i + 1 < n; i += 2) {   // éventail rentré d'un demi-pixel, deux triangles par uiQuad
        const int j = std::min(i + 2, n - 1);
        quad(r, sv(in[0], t.en(in[0].y)), sv(in[i], t.en(in[i].y)), sv(in[i + 1], t.en(in[i + 1].y)), sv(in[j], t.en(in[j].y)));
    }
    for (int i = 0; i < n; i++) {          // frange d'un pixel : opaque dedans, transparente dehors
        const int j = (i + 1) % n;
        quad(r, sv(in[i], t.en(in[i].y)), sv(ex[i], t.en(ex[i].y), 0), sv(ex[j], t.en(ex[j].y), 0), sv(in[j], t.en(in[j].y)));
    }
}

void contour(Renderer& r, const vec2* p, int n, float ep, const vec4& c) {
    if (n < 3 || n > MAXP) return;
    float a = c.w;
    if (ep < 1) { a *= std::max(ep, 0.f); ep = 1; }   // trait fin : 1 px plus transparent
    vec2 nr[MAXP], o[MAXP], e[MAXP], i0[MAXP], i1[MAXP];
    normales(p, n, nr);
    for (int i = 0; i < n; i++) {
        o[i] = decale(p, nr, n, i, 0.5f); e[i] = decale(p, nr, n, i, -0.5f);
        i0[i] = decale(p, nr, n, i, -(ep - 0.5f)); i1[i] = decale(p, nr, n, i, -(ep + 0.5f));
    }
    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n;
        quad(r, sv(e[i], c, a), sv(o[i], c, 0), sv(o[j], c, 0), sv(e[j], c, a));
        if (ep > 1) quad(r, sv(i0[i], c, a), sv(e[i], c, a), sv(e[j], c, a), sv(i0[j], c, a));
        quad(r, sv(i1[i], c, 0), sv(i0[i], c, a), sv(i0[j], c, a), sv(i1[j], c, 0));
    }
}

void halo(Renderer& r, const vec2* p, int n, float flou, const vec4& c) {
    if (n < 3 || n > MAXP || flou <= 0) return;
    vec2 nr[MAXP], m[MAXP], o[MAXP];
    normales(p, n, nr);
    for (int i = 0; i < n; i++) { m[i] = decale(p, nr, n, i, flou * 0.4f); o[i] = decale(p, nr, n, i, flou); }
    const float a = c.w, am = c.w * 0.35f;   // décroissance en deux pentes (flou gaussien approché)
    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n;
        quad(r, sv(p[i], c, a), sv(m[i], c, am), sv(m[j], c, am), sv(p[j], c, a));
        quad(r, sv(m[i], c, am), sv(o[i], c, 0), sv(o[j], c, 0), sv(m[j], c, am));
    }
}

void biseau(Renderer& r, float x, float y, float w, float h, float biais, const Teinte& t) {
    const vec2 p[4] = {{x + biais, y}, {x + w, y}, {x + w - biais, y + h}, {x, y + h}};
    polygone(r, p, 4, t);
}

void couronne(Renderer& r, float cx, float cy, float r0, float r1, const vec4& c0, const vec4& c1) {
    const int n = segments(r1, 2 * PI);
    for (int i = 0; i < n; i++) {
        const float a = 2 * PI * i / n, b = 2 * PI * (i + 1) / n;
        quad(r, sv(polaire(cx, cy, r0, a), c0), sv(polaire(cx, cy, r1, a), c1), sv(polaire(cx, cy, r1, b), c1), sv(polaire(cx, cy, r0, b), c0));
    }
}

void anneau(Renderer& r, float cx, float cy, float r0, float r1, const vec4& c, float a0, float a1) {
    float a = c.w;
    if (r1 - r0 < 1) { const float m = (r0 + r1) / 2; a *= std::max(r1 - r0, 0.f); r0 = m - 0.5f; r1 = m + 0.5f; }
    const float q0 = std::max(0.f, r0 - 0.5f), q1 = r0 + 0.5f, q2 = r1 - 0.5f, q3 = r1 + 0.5f;
    const int n = segments(r1, a1 - a0);
    for (int i = 0; i < n; i++) {
        const float ta = a0 + (a1 - a0) * i / n, tb = a0 + (a1 - a0) * (i + 1) / n;
        auto bande = [&](float ra, float aa, float rb, float ab) {
            quad(r, sv(polaire(cx, cy, ra, ta), c, aa), sv(polaire(cx, cy, rb, ta), c, ab), sv(polaire(cx, cy, rb, tb), c, ab),
                 sv(polaire(cx, cy, ra, tb), c, aa));
        };
        bande(q0, 0, q1, a);
        if (q2 > q1) bande(q1, a, q2, a);
        bande(q2, a, q3, 0);
    }
}

void disqueRadial(Renderer& r, float cx, float cy, float R, float gx, float gy, float gr, const vec4& c0, const vec4& c1) {
    const int n = segments(R, 2 * PI), M = 4;   // rayons depuis le centre du dégradé, M anneaux le long de chaque rayon
    const vec2 g{gx, gy};
    auto teinte = [&](vec2 q) { const vec2 d = q - g; return melange(c0, c1, std::min(1.f, std::sqrt(dot2(d, d)) / std::max(gr, 1e-3f))); };
    for (int i = 0; i < n; i++) {
        const float ta = 2 * PI * i / n, tb = 2 * PI * (i + 1) / n;
        const vec2 pa = polaire(cx, cy, R - 0.5f, ta), pb = polaire(cx, cy, R - 0.5f, tb);
        for (int m = 0; m < M; m++) {
            const float t0 = (float)m / M, t1 = (float)(m + 1) / M;
            const vec2 a0 = g + (pa - g) * t0, a1 = g + (pa - g) * t1, b0 = g + (pb - g) * t0, b1 = g + (pb - g) * t1;
            if (m == 0) tri(r, sv(g, c0), sv(a1, teinte(a1)), sv(b1, teinte(b1)));
            else quad(r, sv(a0, teinte(a0)), sv(a1, teinte(a1)), sv(b1, teinte(b1)), sv(b0, teinte(b0)));
        }
        const vec4 ca = teinte(pa), cb = teinte(pb);   // frange du bord
        quad(r, sv(pa, ca), sv(polaire(cx, cy, R + 0.5f, ta), ca, 0), sv(polaire(cx, cy, R + 0.5f, tb), cb, 0), sv(pb, cb));
    }
}

void trait(Renderer& r, const vec2* p, int n, float ep, const vec4& c) {
    const float hw = ep / 2, hi = std::max(0.f, hw - 0.5f), ho = hw + 0.5f;
    for (int i = 0; i + 1 < n; i++) {
        const vec2 a = p[i], b = p[i + 1], d = unit(b - a), nn{-d.y, d.x};
        quad(r, sv(a - nn * hi, c), sv(b - nn * hi, c), sv(b + nn * hi, c), sv(a + nn * hi, c));
        quad(r, sv(a + nn * hi, c), sv(b + nn * hi, c), sv(b + nn * ho, c, 0), sv(a + nn * ho, c, 0));
        quad(r, sv(a - nn * hi, c), sv(b - nn * hi, c), sv(b - nn * ho, c, 0), sv(a - nn * ho, c, 0));
    }
    for (int i = 0; i < n; i++) r.uiCircle(p[i].x, p[i].y, ho, c);   // bouts et jointures ronds
}

void arc(Renderer& r, float cx, float cy, float rayon, float ep, float a0, float a1, const vec4& c) {
    const float hw = ep / 2;
    anneau(r, cx, cy, rayon - hw, rayon + hw, c, a0, a1);
    for (float a : {a0, a1}) { const vec2 q = polaire(cx, cy, rayon, a); r.uiCircle(q.x, q.y, hw + 0.5f, c); }
}

} // namespace dessin2d

} // namespace wfs
