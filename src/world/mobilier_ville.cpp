// WaForSpeed — mobilier urbain de la Ville modélisé au niveau du vrai objet (ticket WFS-369, épic E15) : voir mobilier_ville.h.
// Mesures (sources, ±1 %) :
//   - banc public : longueur 1,80 m, profondeur 0,60 m, assise à 0,45 m, dossier à 0,85 m ; lattes de 95 × 35 mm
//     (NF EN 581-1 et 581-2 ; catalogues des bancs « ville de Paris » à flancs en fonte) ;
//   - abribus : 4,0 × 1,8 m, 2,55 m de haut, poteaux 80 × 80 mm, verre trempé 10 mm, panneau d'affichage 120 × 176 cm
//     (format « sucette » 2 m², caisson 1,32 × 1,92 m) ; banc à 0,46 m ;
//   - grille de parc : 1,15 m, barreaux carrés de 16 mm tous les 13 cm (vide < 11 cm, NF P01-012), lisses plates 40 × 12 mm,
//     poteaux 80 mm à pomme ;
//   - lampadaire de rue : mât conique galvanisé de 7,4 m (Ø 180 mm au pied, 120 mm en tête), crosse de 2,4 m, luminaire
//     routier de 0,84 × 0,34 m (EN 40, catalogues des luminaires à LED de voirie) ;
//   - panneau rond B21c1 « sens giratoire » : Ø 700 mm (gamme normale, IISR 4e partie), bord roulé, colliers ;
//   - feu tricolore R11 : 3 feux Ø 200 mm, caisson 0,30 × 0,95 m, visières de 180 mm, mât de 3,6 m ;
//   - terrasse : table de bistrot Ø 0,70 m à 0,75 m, chaise de bistrot (assise 0,45 m, 0,42 × 0,42 m, dossier 0,86 m),
//     parasol Ø 2,7 m à 8 baleines (bas de toile à 2,0 m, mât bois Ø 38 mm, pied lesté Ø 0,50 m) ;
//   - jardinière en terre cuite Ø 0,56 m, 0,52 m, buis taillé Ø 0,60 m.
#include "mobilier_ville.h"
#include <algorithm>
#include <cmath>

namespace wfs::mobilier {
namespace {

constexpr vec3 HAUT{0, 1, 0};

// petit hachage déterministe (forme du buis : même résultat à chaque partie)
float bruit(uint32_t a, uint32_t b, uint32_t g) {
    uint32_t h = a * 374761393u + b * 668265263u + g * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xffff) / 65535.f;
}

uint32_t sommet(Sortie& s, vec3 p, vec3 n, vec2 uv) {
    s.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y});
    return (uint32_t)s.v.size() - 1;
}

// polygone plan convexe : normale (méthode de Newell) tournée à l'opposé du point « dedans »
void poly(Sortie& s, const vec3* p, int n, vec3 dedans, vec2 uv) {
    vec3 c{0, 0, 0}, nn{0, 0, 0};
    for (int k = 0; k < n; k++) c = c + p[k];
    c = c * (1.f / (float)n);
    for (int k = 0; k < n; k++) nn = nn + cross(p[k] - c, p[(k + 1) % n] - c);
    if (dot(nn, nn) < 1e-14f) return;   // face dégénérée (chanfrein nul)
    nn = normalize(nn);
    const bool inverse = dot(nn, c - dedans) < 0;
    if (inverse) nn = nn * -1.f;
    const uint32_t k0 = (uint32_t)s.v.size();
    for (int k = 0; k < n; k++) sommet(s, p[k], nn, uv);
    for (int k = 1; k + 1 < n; k++) {
        if (inverse) s.i.insert(s.i.end(), {k0, k0 + (uint32_t)k + 1, k0 + (uint32_t)k});
        else s.i.insert(s.i.end(), {k0, k0 + (uint32_t)k, k0 + (uint32_t)k + 1});
    }
}

// deux anneaux de m sommets (indices a0.., b0..) reliés ; sens choisi d'après la normale des sommets
void relier(Sortie& s, uint32_t a0, uint32_t b0, int m) {
    // premier quadrilatère non dégénéré : sens de référence
    bool inverse = false;
    for (int i = 0; i + 1 < m; i++) {
        const Vertex &A = s.v[a0 + i], &A1 = s.v[a0 + i + 1], &B = s.v[b0 + i], &B1 = s.v[b0 + i + 1];
        const vec3 pa{A.px, A.py, A.pz}, pa1{A1.px, A1.py, A1.pz}, pb{B.px, B.py, B.pz}, pb1{B1.px, B1.py, B1.pz};
        const vec3 g = cross(pa1 - pa, pb1 - pa) + cross(pb1 - pa, pb - pa);
        if (dot(g, g) < 1e-16f) continue;
        const vec3 n{A.nx + B1.nx, A.ny + B1.ny, A.nz + B1.nz};
        inverse = dot(g, n) < 0;
        break;
    }
    for (int i = 0; i + 1 < m; i++) {
        const uint32_t a = a0 + i, a1 = a + 1, b = b0 + i, b1 = b + 1;
        if (inverse) s.i.insert(s.i.end(), {a, b1, a1, a, b, b1});
        else s.i.insert(s.i.end(), {a, a1, b1, a, b1, b});
    }
}

void paveF(Sortie& s, const Repere& F, vec3 h, float ch, vec2 uv) {
    ch = std::min(ch, 0.45f * std::min({h.x, h.y, h.z}));
    auto P = [&](int sx, int sy, int sz, int ax) {
        return F.p({sx * (h.x - (ax == 0 ? 0 : ch)), sy * (h.y - (ax == 1 ? 0 : ch)), sz * (h.z - (ax == 2 ? 0 : ch))});
    };
    const vec3 c = F.o;
    for (int sg : {-1, 1}) {   // 6 faces
        const vec3 fx[4] = {P(sg, -1, -1, 0), P(sg, 1, -1, 0), P(sg, 1, 1, 0), P(sg, -1, 1, 0)};
        const vec3 fy[4] = {P(-1, sg, -1, 1), P(1, sg, -1, 1), P(1, sg, 1, 1), P(-1, sg, 1, 1)};
        const vec3 fz[4] = {P(-1, -1, sg, 2), P(1, -1, sg, 2), P(1, 1, sg, 2), P(-1, 1, sg, 2)};
        poly(s, fx, 4, c, uv); poly(s, fy, 4, c, uv); poly(s, fz, 4, c, uv);
    }
    for (int a : {-1, 1}) for (int b : {-1, 1}) {   // 12 chanfreins
        const vec3 ez[4] = {P(a, b, -1, 0), P(a, b, 1, 0), P(a, b, 1, 1), P(a, b, -1, 1)};
        const vec3 ey[4] = {P(a, -1, b, 0), P(a, 1, b, 0), P(a, 1, b, 2), P(a, -1, b, 2)};
        const vec3 ex[4] = {P(-1, a, b, 1), P(1, a, b, 1), P(1, a, b, 2), P(-1, a, b, 2)};
        poly(s, ez, 4, c, uv); poly(s, ey, 4, c, uv); poly(s, ex, 4, c, uv);
    }
    for (int sx : {-1, 1}) for (int sy : {-1, 1}) for (int sz : {-1, 1}) {   // 8 coins
        const vec3 t[3] = {P(sx, sy, sz, 0), P(sx, sy, sz, 1), P(sx, sy, sz, 2)};
        poly(s, t, 3, c, uv);
    }
}

// barreau carré de demi-côté hc de a à b (monde), sans dessous, pointe de lance en haut (hauteur pointe)
void barreau(Sortie& s, vec3 a, vec3 b, vec3 u, vec3 w, float hc, float pointe, vec2 uv) {
    const vec3 c = (a + b) * 0.5f;
    const vec3 co[4] = {u * hc + w * hc, u * -hc + w * hc, u * -hc + w * -hc, u * hc + w * -hc};
    for (int k = 0; k < 4; k++) {
        const vec3 q[4] = {a + co[k], a + co[(k + 1) % 4], b + co[(k + 1) % 4], b + co[k]};
        poly(s, q, 4, c, uv);
    }
    const vec3 tip = b + HAUT * pointe, cb = b + HAUT * (pointe * 0.3f);
    for (int k = 0; k < 4; k++) {   // fer de lance : base élargie (1,5 ×) puis pointe
        const vec3 q[3] = {b + co[k] * 1.5f, b + co[(k + 1) % 4] * 1.5f, tip};
        poly(s, q, 3, cb, uv);
    }
}

}  // namespace

Repere repere(vec3 o, vec3 x, vec3 z) {
    vec3 zh = normalize(vec3{z.x, 0, z.z});
    vec3 xh = x - zh * dot(x, zh);
    xh = normalize(vec3{xh.x, 0, xh.z});
    return {o, xh, HAUT, zh};
}

void pave(Sortie s, const Repere& R, vec3 c, vec3 h, float ch, vec2 uv) { paveF(s, {R.p(c), R.x, R.y, R.z}, h, ch, uv); }

void barre(Sortie s, const Repere& R, vec3 a, vec3 b, vec3 n, float l, float e, float ch, vec2 uv) {
    const vec3 A = R.p(a), B = R.p(b);
    const float L = length(B - A);
    if (L < 1e-4f) return;
    const vec3 y = (B - A) * (1.f / L);
    vec3 z = R.d(n);
    z = normalize(z - y * dot(z, y));
    const vec3 x = cross(y, z);
    paveF(s, {(A + B) * 0.5f, x, y, z}, {l * 0.5f, L * 0.5f, e * 0.5f}, ch, uv);
}

void tour(Sortie s, const Repere& R, vec3 c, const std::vector<vec2>& q, int n, vec2 uv, int plis, float amplitude) {
    const int m = (int)q.size();
    if (m < 2 || n < 3) return;
    std::vector<vec2> ns((size_t)m - 1);   // normale de chaque tronçon du profil (vers l'extérieur)
    for (int j = 0; j + 1 < m; j++) {
        const float dr = q[j + 1].x - q[j].x, dy = q[j + 1].y - q[j].y, l = std::sqrt(dr * dr + dy * dy);
        ns[(size_t)j] = l > 1e-7f ? vec2{dy / l, -dr / l} : vec2{0, 1};
    }
    const float lisse = 0.82f;   // angle < 35° : arête adoucie ; au-delà, arête vive
    auto moyenne = [&](vec2 a, vec2 b) {
        if (a.x * b.x + a.y * b.y < lisse) return a;
        const float x = a.x + b.x, y = a.y + b.y, l = std::sqrt(x * x + y * y);
        return vec2{x / l, y / l};
    };
    const vec3 o = R.p(c);
    for (int j = 0; j + 1 < m; j++) {
        if (q[j].x < 1e-6f && q[j + 1].x < 1e-6f) continue;
        const vec2 n0 = j > 0 ? moyenne(ns[(size_t)j], ns[(size_t)j - 1]) : ns[(size_t)j];
        const vec2 n1 = j + 2 < m ? moyenne(ns[(size_t)j], ns[(size_t)j + 1]) : ns[(size_t)j];
        uint32_t anneau[2] = {0, 0};
        for (int e = 0; e < 2; e++) {
            const vec2 p = q[(size_t)(j + e)], nn = e ? n1 : n0;
            anneau[e] = (uint32_t)s.v.size();
            for (int i = 0; i <= n; i++) {
                const float t = 2 * PI * (float)i / (float)n, ct = std::cos(t), st = std::sin(t);
                const float f = plis > 0 ? 1.f + amplitude * std::fabs(std::cos(0.5f * (float)plis * t)) : 1.f;
                const vec3 rd = R.x * ct + R.z * st;
                sommet(s, o + rd * (p.x * f) + R.y * p.y, normalize(rd * nn.x + R.y * nn.y), uv);
            }
        }
        relier(s, anneau[0], anneau[1], n + 1);
    }
}

void tube(Sortie s, const Repere& R, const std::vector<vec3>& pts, float r, int n, vec2 uv, bool bouts) {
    const int m = (int)pts.size();
    if (m < 2) return;
    std::vector<vec3> W((size_t)m), T((size_t)m);
    for (int k = 0; k < m; k++) W[(size_t)k] = R.p(pts[(size_t)k]);
    for (int k = 0; k < m; k++) {
        const vec3 a = W[(size_t)std::max(0, k - 1)], b = W[(size_t)std::min(m - 1, k + 1)];
        T[(size_t)k] = normalize(b - a);
    }
    vec3 u = normalize(cross(T[0], std::fabs(T[0].y) < 0.9f ? HAUT : vec3{1, 0, 0}));
    std::vector<uint32_t> anneaux((size_t)m);
    for (int k = 0; k < m; k++) {
        const vec3 t = T[(size_t)k];
        u = normalize(u - t * dot(u, t));
        const vec3 w = cross(t, u);
        anneaux[(size_t)k] = (uint32_t)s.v.size();
        for (int i = 0; i <= n; i++) {
            const float a = 2 * PI * (float)i / (float)n;
            const vec3 d = u * std::cos(a) + w * std::sin(a);
            sommet(s, W[(size_t)k] + d * r, d, uv);
        }
    }
    for (int k = 0; k + 1 < m; k++) relier(s, anneaux[(size_t)k], anneaux[(size_t)k + 1], n + 1);
    if (!bouts) return;
    for (int e : {0, m - 1}) {   // bouchons
        std::vector<vec3> c;
        for (int i = 0; i < n; i++) { const Vertex& v = s.v[anneaux[(size_t)e] + (uint32_t)i]; c.push_back({v.px, v.py, v.pz}); }
        poly(s, c.data(), n, W[(size_t)e] + T[(size_t)e] * (e == 0 ? r : -r), uv);
    }
}

// ---------------------------------------------------------------- modèles

void banc(Sortie s, const Repere& R, vec2 bois, vec2 fonte) {
    const vec3 Z{0, 0, 1};
    for (float z : {-0.78f, 0.78f}) {   // flancs en fonte
        barre(s, R, {-0.25f, 0, z}, {-0.22f, 0.43f, z}, Z, 0.055f, 0.04f, 0.008f, fonte);       // pied avant
        barre(s, R, {0.25f, 0, z}, {0.19f, 0.42f, z}, Z, 0.055f, 0.04f, 0.008f, fonte);         // pied arrière
        barre(s, R, {-0.29f, 0.41f, z}, {0.21f, 0.40f, z}, Z, 0.05f, 0.045f, 0.008f, fonte);    // support d'assise
        barre(s, R, {0.16f, 0.40f, z}, {0.29f, 0.87f, z}, Z, 0.05f, 0.045f, 0.008f, fonte);     // montant du dossier
        barre(s, R, {-0.23f, 0.13f, z}, {0.23f, 0.13f, z}, Z, 0.03f, 0.035f, 0.006f, fonte);    // entretoise
        for (float x : {-0.245f, 0.245f}) pave(s, R, {x, 0.012f, z}, {0.045f, 0.012f, 0.035f}, 0.005f, fonte);   // patins
        tube(s, R, {{0.225f, 0.62f, z}, {0.05f, 0.665f, z}, {-0.17f, 0.655f, z}, {-0.255f, 0.62f, z}, {-0.27f, 0.55f, z}, {-0.255f, 0.44f, z}},
             0.019f, 6, fonte);   // accoudoir cintré
    }
    for (float x : {-0.235f, -0.125f, -0.015f, 0.095f})   // 4 lattes d'assise 95 × 35 mm, jour de 15 mm
        pave(s, R, {x, 0.445f, 0}, {0.0475f, 0.0175f, 0.92f}, 0.006f, bois);
    const vec2 a{0.205f, 0.49f}, b{0.30f, 0.86f};   // 3 lattes de dossier, devant les montants, suivant leur pente
    const float L = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
    const vec2 d{(b.x - a.x) / L, (b.y - a.y) / L}, nf{-d.y, d.x};   // nf : vers l'avant du dossier
    for (float t : {0.17f, 0.52f, 0.87f}) {
        const vec3 c{a.x + d.x * L * t + nf.x * 0.04f, a.y + d.y * L * t + nf.y * 0.04f, 0};
        paveF(s, {R.p(c), R.d({nf.x, nf.y, 0}), R.d({d.x, d.y, 0}), R.z}, {0.0175f, 0.0475f, 0.92f}, 0.006f, bois);
    }
}

void abribus(Sortie s, Sortie verre, const Repere& R, vec2 alu, vec2 affiche, vec2 bois) {
    const float xb = 0.80f;   // plan des poteaux et du vitrage du fond
    const vec3 Z{0, 0, 1};
    for (float z : {-1.95f, 0.f, 1.95f}) {
        pave(s, R, {xb, 1.24f, z}, {0.04f, 1.24f, 0.04f}, 0.008f, alu);                         // poteau 80 × 80
        barre(s, R, {xb + 0.04f, 2.38f, z}, {-0.86f, 2.44f, z}, Z, 0.09f, 0.05f, 0.01f, alu);   // console du toit
        pave(s, R, {xb, 0.006f, z}, {0.07f, 0.006f, 0.07f}, 0.003f, alu);                        // platine
    }
    pave(s, R, {-0.02f, 2.505f, 0}, {0.95f, 0.025f, 2.10f}, 0.012f, alu);   // toit
    pave(s, R, {-0.95f, 2.47f, 0}, {0.025f, 0.07f, 2.12f}, 0.012f, alu);    // bandeau avant
    pave(s, R, {0.91f, 2.47f, 0}, {0.025f, 0.06f, 2.12f}, 0.012f, alu);     // chéneau arrière
    for (float zc : {-0.975f, 0.975f}) {   // vitrage du fond, deux travées, profils haut et bas
        pave(verre, R, {xb, 1.21f, zc}, {0.005f, 1.07f, 0.905f}, 0.002f, {0, 0});
        pave(s, R, {xb, 0.12f, zc}, {0.025f, 0.025f, 0.91f}, 0.006f, alu);
        pave(s, R, {xb, 2.30f, zc}, {0.025f, 0.025f, 0.91f}, 0.006f, alu);
    }
    pave(verre, R, {0.27f, 1.21f, 1.95f}, {0.48f, 1.07f, 0.005f}, 0.002f, {0, 0});   // paroi latérale vitrée
    pave(s, R, {-0.23f, 1.24f, 1.95f}, {0.03f, 1.24f, 0.03f}, 0.006f, alu);
    pave(s, R, {0.27f, 0.12f, 1.95f}, {0.50f, 0.025f, 0.025f}, 0.006f, alu);
    pave(s, R, {0.27f, 2.30f, 1.95f}, {0.50f, 0.025f, 0.025f}, 0.006f, alu);
    // panneau d'affichage double face au bout −z : caisson 1,32 × 1,92 × 0,15 m, affiche 1,20 × 1,76 m sous verre, deux pieds
    pave(s, R, {0.20f, 1.18f, -2.06f}, {0.66f, 0.96f, 0.075f}, 0.03f, alu);
    for (float sg : {-1.f, 1.f}) pave(s, R, {0.20f, 1.18f, -2.06f + sg * 0.073f}, {0.60f, 0.88f, 0.004f}, 0.002f, affiche);
    for (float x : {-0.30f, 0.70f}) pave(s, R, {x, 0.11f, -2.06f}, {0.04f, 0.11f, 0.05f}, 0.008f, alu);
    // banc : 3 lattes sur deux pieds
    for (float x : {0.48f, 0.58f, 0.68f}) pave(s, R, {x, 0.462f, 0}, {0.045f, 0.0175f, 1.10f}, 0.006f, bois);
    for (float z : {-0.95f, 0.95f}) {
        barre(s, R, {0.60f, 0, z}, {0.59f, 0.43f, z}, Z, 0.06f, 0.04f, 0.008f, alu);
        pave(s, R, {0.58f, 0.432f, z}, {0.19f, 0.012f, 0.025f}, 0.005f, alu);
    }
}

void grille(Sortie s, vec3 a, vec3 b, vec3 ext, vec2 fonte) {
    const vec3 dh{b.x - a.x, 0, b.z - a.z};
    const float Lh = length(dh), dy = b.y - a.y;
    if (Lh < 0.3f) return;
    const Repere R = repere(a, ext, dh);
    pave(s, R, {0, 0.575f, 0}, {0.04f, 0.575f, 0.04f}, 0.008f, fonte);   // poteau 80 mm
    tour(s, R, {0, 1.15f, 0}, {{0.05f, 0}, {0.05f, 0.02f}, {0.03f, 0.035f}, {0.042f, 0.07f}, {0.03f, 0.11f}, {0, 0.15f}}, 8, fonte);   // pomme
    const vec3 X{1, 0, 0};
    for (float y : {0.12f, 1.0f}) barre(s, R, {0, y, 0.04f}, {0, y + dy * (Lh - 0.04f) / Lh, Lh - 0.04f}, X, 0.04f, 0.012f, 0.003f, fonte);
    const int nb = std::max(1, (int)std::floor((Lh - 0.08f) / 0.13f));
    for (int k = 0; k < nb; k++) {
        const float z = 0.04f + ((float)k + 0.5f) * (Lh - 0.08f) / (float)nb, y0 = 0.05f + dy * z / Lh;
        barreau(s, R.p({0, y0, z}), R.p({0, y0 + 1.05f, z}), R.x, R.z, 0.008f, 0.07f, fonte);
    }
}

void fontaine(Sortie s, const Repere& R, float rb, float k, vec2 pierre, vec2 eau) {
    const int n = 48;
    tour(s, R, {0, 0, 0}, {{rb + 0.10f, 0}, {rb + 0.10f, 0.08f}, {rb + 0.02f, 0.12f}, {rb + 0.02f, 0.46f}, {rb + 0.07f, 0.49f}, {rb + 0.09f, 0.535f},
                           {rb + 0.07f, 0.58f}, {rb + 0.02f, 0.60f}, {rb - 0.28f, 0.60f}, {rb - 0.30f, 0.57f}, {rb - 0.30f, 0.40f}}, n, pierre);   // bassin mouluré
    const float ri = rb - 0.30f;
    tour(s, R, {0, 0, 0}, {{ri, 0.45f}, {ri * 0.66f, 0.453f}, {ri * 0.33f, 0.448f}, {0, 0.451f}}, n, eau);   // eau, légère houle
    const float kh = std::clamp(rb / 4.f, 0.6f, 1.f);
    auto P = [&](std::initializer_list<vec2> l) { std::vector<vec2> v; for (vec2 q : l) v.push_back({q.x * kh, 0.40f + (q.y - 0.40f) * k}); return v; };
    tour(s, R, {0, 0, 0}, P({{0.62f, 0.40f}, {0.62f, 0.52f}, {0.52f, 0.58f}, {0.50f, 0.64f}, {0.40f, 0.70f}, {0.30f, 0.78f}, {0.27f, 0.90f},
                             {0.25f, 1.45f}, {0.29f, 1.50f}, {0.34f, 1.56f}, {0.34f, 1.62f}, {0.20f, 1.66f}}), 24, pierre);   // piédestal
    tour(s, R, {0, 0, 0}, P({{0.20f, 1.66f}, {0.40f, 1.70f}, {0.70f, 1.80f}, {0.92f, 1.90f}, {1.02f, 1.97f}, {1.03f, 2.02f}, {0.98f, 2.045f},
                             {0.93f, 1.98f}}), 32, pierre);   // vasque à lèvre
    tour(s, R, {0, 0, 0}, P({{0.93f, 1.975f}, {0.5f, 1.977f}, {0, 1.975f}}), 32, eau);
    tour(s, R, {0, 0, 0}, P({{0.13f, 1.97f}, {0.13f, 2.05f}, {0.08f, 2.10f}, {0.12f, 2.20f}, {0.11f, 2.31f}, {0.06f, 2.40f}, {0, 2.46f}}),
         16, pierre, 8, 0.12f);   // pomme de pin à écailles
}

void lampadaire(Sortie m, Sortie l, const Repere& R, float portee, bool lod) {
    const int n = lod ? 6 : 12;
    const vec2 u{0, 0};
    tour(m, R, {0, 0, 0}, {{0.16f, 0}, {0.16f, 0.025f}, {0.13f, 0.04f}, {0.115f, 0.06f}, {0.11f, 0.55f}, {0.095f, 0.62f}, {0.09f, 0.66f},
                           {0.06f, 7.25f}, {0.064f, 7.27f}, {0.064f, 7.31f}, {0, 7.33f}}, n, u);   // mât conique, collerette, chapeau
    const float L = portee;
    tube(m, R, {{0.0f, 7.05f, 0}, {0.05f, 7.30f, 0}, {0.22f, 7.43f, 0}, {0.55f, 7.48f, 0}, {L * 0.6f, 7.47f, 0}, {L - 0.45f, 7.41f, 0}}, 0.032f, lod ? 4 : 8, u);   // crosse
    pave(m, R, {L - 0.08f, 7.35f, 0}, {0.42f, 0.055f, 0.17f}, 0.045f, u);   // corps du luminaire, arêtes arrondies
    pave(m, R, {L - 0.13f, 7.405f, 0}, {0.28f, 0.03f, 0.12f}, 0.028f, u);    // capot
    pave(l, R, {L - 0.04f, 7.29f, 0}, {0.30f, 0.012f, 0.12f}, 0.008f, u);    // vasque (verre éclairant)
    if (!lod) pave(m, R, {-0.109f, 0.33f, 0}, {0.006f, 0.15f, 0.045f}, 0.004f, u);   // trappe de visite, côté trottoir
}

void panneauRond(Sortie mat, Sortie s, const Repere& R, float h, vec2 fond, vec2 bord, vec2 dos) {
    tour(mat, R, {0, 0, 0}, {{0.034f, 0}, {0.034f, h + 0.36f}, {0.037f, h + 0.365f}, {0.037f, h + 0.38f}, {0, h + 0.39f}}, 8, {0, 0});   // mât Ø 68, bouchon
    const Repere F{R.p({0.05f, h, 0}), R.y, R.x, R.z};   // axe du panneau : R.x (vers les conducteurs)
    tour(s, F, {0, 0, 0}, {{0, -0.003f}, {0.345f, -0.003f}}, 32, dos);                                         // dos galvanisé
    tour(s, F, {0, 0, 0}, {{0.345f, -0.003f}, {0.352f, 0.004f}, {0.343f, 0.011f}, {0.325f, 0.012f}}, 32, bord); // bord roulé
    tour(s, F, {0, 0, 0}, {{0.325f, 0.012f}, {0, 0.012f}}, 32, fond);                                           // fond
    for (float dy : {-0.17f, 0.17f}) pave(mat, R, {0.042f, h + dy, 0}, {0.012f, 0.022f, 0.045f}, 0.006f, {0, 0});   // colliers
}

Repere repereOptique(const Repere& R, int k) { return {R.p({0.275f, 3.425f - 0.30f * (float)k, 0}), R.y, R.x, R.z}; }

void optique(Sortie s, const Repere& R, int k) {
    tour(s, repereOptique(R, k), {0, 0, 0}, {{0.10f, 0}, {0.10f, 0.004f}, {0.07f, 0.012f}, {0, 0.016f}}, 16, {0, 0});   // optique Ø 200, bombée
}

void feuTricolore(Sortie mat, Sortie s, const Repere& R, vec2 noir) {
    tour(mat, R, {0, 0, 0}, {{0.075f, 0}, {0.075f, 0.02f}, {0.062f, 0.035f}, {0.06f, 3.55f}, {0.066f, 3.57f}, {0, 3.6f}}, 10, {0, 0});   // mât
    for (float y : {2.80f, 3.45f}) pave(mat, R, {0.085f, y, 0}, {0.03f, 0.025f, 0.03f}, 0.006f, {0, 0});   // pattes de fixation
    pave(s, R, {0.12f, 3.125f, 0}, {0.006f, 0.55f, 0.26f}, 0.02f, noir);   // écran de contraste
    pave(s, R, {0.20f, 3.125f, 0}, {0.075f, 0.475f, 0.15f}, 0.03f, noir);  // caisson
    for (int k = 0; k < 3; k++) {   // les optiques (allumées ou éteintes) sont dessinées à part : optique()
        const float y = 3.425f - 0.30f * (float)k;
        tour(s, repereOptique(R, k), {0, 0, 0}, {{0.118f, -0.004f}, {0.118f, 0.002f}, {0.10f, 0.004f}}, 16, noir);   // bague de l'optique
        for (int j = 0; j < 5; j++) {   // visière : demi-tube de 180 mm au-dessus du feu
            const float t0 = PI * (float)j / 5.f, t1 = PI * (float)(j + 1) / 5.f, tm = (t0 + t1) / 2, r = 0.115f;
            const vec3 nn{0, std::sin(tm), std::cos(tm)};
            barre(s, R, {0.275f, y + r * std::sin(tm), r * std::cos(tm)}, {0.455f, y + r * std::sin(tm), r * std::cos(tm)}, nn,
                  2 * r * std::sin((t1 - t0) / 2) + 0.004f, 0.004f, 0.001f, noir);
        }
    }
}

void tableBistrot(Sortie s, const Repere& R, vec2 plateau, vec2 alu) {
    tour(s, R, {0, 0, 0}, {{0.022f, 0.715f}, {0.33f, 0.715f}, {0.35f, 0.725f}, {0.35f, 0.742f}, {0.338f, 0.75f}, {0.022f, 0.75f}, {0.022f, 0.715f}},
         24, plateau);   // plateau Ø 0,70, chant arrondi, trou du parasol
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})
        tube(s, R, {{sx * 0.21f, 0, sz * 0.21f}, {sx * 0.165f, 0.70f, sz * 0.165f}}, 0.0135f, 6, alu);   // pieds évasés Ø 27
    for (int k = 0; k < 4; k++) {   // ceinture sous le plateau
        const float a0 = PI * 0.5f * (float)k + PI * 0.25f, a1 = a0 + PI * 0.5f, r = 0.233f;
        const vec3 p0{std::cos(a0) * r, 0.695f, std::sin(a0) * r}, p1{std::cos(a1) * r, 0.695f, std::sin(a1) * r};
        barre(s, R, p0, p1, {std::cos((a0 + a1) / 2), 0, std::sin((a0 + a1) / 2)}, 0.03f, 0.012f, 0.003f, alu);
    }
}

void chaiseBistrot(Sortie s, const Repere& R, vec2 cannage, vec2 alu) {
    const float r = 0.011f;
    for (float sx : {-1.f, 1.f}) {
        tube(s, R, {{sx * 0.20f, 0, 0.20f}, {sx * 0.19f, 0.44f, 0.185f}}, r, 6, alu);                                              // pied avant
        tube(s, R, {{sx * 0.20f, 0, -0.21f}, {sx * 0.19f, 0.44f, -0.19f}, {sx * 0.186f, 0.62f, -0.215f}, {sx * 0.17f, 0.86f, -0.245f}}, r, 6, alu);   // pied arrière et montant
        tube(s, R, {{sx * 0.197f, 0.15f, 0.195f}, {sx * 0.197f, 0.15f, -0.205f}}, 0.008f, 5, alu);                                // barreau
    }
    tube(s, R, {{-0.19f, 0.44f, 0.185f}, {0.19f, 0.44f, 0.185f}, {0.19f, 0.44f, -0.19f}, {-0.19f, 0.44f, -0.19f}, {-0.19f, 0.44f, 0.185f}}, 0.01f, 6, alu, false);   // cadre d'assise
    pave(s, R, {0, 0.455f, -0.003f}, {0.2f, 0.014f, 0.2f}, 0.012f, cannage);   // assise tressée
    for (float y : {0.66f, 0.81f}) {   // dossier : deux bandes tressées cintrées
        const float zb = -0.215f - (y - 0.62f) * 0.125f;
        const vec3 p[3] = {{-0.18f, y, zb}, {0, y, zb - 0.035f}, {0.18f, y, zb}};
        for (int k = 0; k < 2; k++) {
            const vec3 d = p[k + 1] - p[k];
            barre(s, R, p[k], p[k + 1], {-d.z, 0, d.x}, 0.06f, 0.012f, 0.004f, cannage);
        }
    }
}

namespace {
// mât, pied lesté et embout d'un parasol (communs à l'ouvert et au fermé)
void mat_parasol(Sortie& s, const Repere& R, vec2 bois, vec2 metal, float coulisse) {
    tour(s, R, {0, 0, 0}, {{0.25f, 0}, {0.25f, 0.035f}, {0.23f, 0.06f}, {0.08f, 0.085f}, {0.05f, 0.10f}, {0.045f, 0.22f}, {0.03f, 0.24f}, {0.019f, 0.24f}}, 24, metal);   // pied lesté
    tour(s, R, {0, 0, 0}, {{0.019f, 0.23f}, {0.019f, 2.38f}, {0.024f, 2.40f}, {0.012f, 2.42f}}, 10, bois);                          // mât Ø 38
    tour(s, R, {0, 0, 0}, {{0.03f, 2.42f}, {0.03f, 2.47f}, {0.015f, 2.50f}, {0.021f, 2.53f}, {0, 2.565f}}, 10, bois);               // embout
    tour(s, R, {0, 0, 0}, {{0.03f, coulisse - 0.04f}, {0.03f, coulisse + 0.04f}, {0.019f, coulisse + 0.05f}}, 10, metal);           // coulisse
}
float hautToile(float r) { return 2.42f - 0.42f * std::pow(r / 1.35f, 1.3f); }
}  // namespace

void parasolOuvert(Sortie s, const Repere& R, vec2 t0, vec2 t1, vec2 bois, vec2 metal) {
    constexpr int NB = 8, NR = 5, NU = 4;
    constexpr float RAY = 1.35f;
    mat_parasol(s, R, bois, metal, 1.86f);
    auto pt = [&](int k, float r, float u) {   // toile : corde droite entre deux baleines, creux de 3,5 cm au milieu
        const float a0 = 2 * PI * (float)k / NB, a1 = 2 * PI * (float)(k + 1) / NB;
        const float x = std::cos(a0) * (1 - u) + std::cos(a1) * u, z = std::sin(a0) * (1 - u) + std::sin(a1) * u;
        return vec3{x * r, hautToile(r) - 0.035f * (r / RAY) * std::sin(PI * u), z * r};
    };
    for (int k = 0; k < NB; k++) {
        const vec2 uv = k % 2 ? t1 : t0;
        uint32_t prec = 0;
        for (int i = 0; i <= NR; i++) {
            const float r = 0.03f + (RAY - 0.03f) * (float)i / NR, dr = 0.01f;
            const uint32_t a = (uint32_t)s.v.size();
            for (int j = 0; j <= NU; j++) {
                const float u = (float)j / NU, du = 0.01f;
                const vec3 p = pt(k, r, u), pr = pt(k, r + dr, u) - pt(k, r - dr, u), pu = pt(k, r, std::min(1.f, u + du)) - pt(k, r, std::max(0.f, u - du));
                vec3 nn = normalize(cross(pu, pr));
                if (nn.y < 0) nn = nn * -1.f;
                sommet(s, R.p(p), R.d(nn), uv);
            }
            if (i > 0) relier(s, prec, a, NU + 1);
            prec = a;
        }
        // lambrequin de 15 cm sous le bord, pan par pan
        const vec3 A = pt(k, RAY, 0), B = pt(k, RAY, 1), bas{0, -0.15f, 0};
        const vec3 q[4] = {R.p(A), R.p(B), R.p(B + bas), R.p(A + bas)};
        poly(s, q, 4, R.p({0, 2.0f, 0}), uv);
        // baleine (sous la toile) et sa jambe de force depuis la coulisse
        const float an = 2 * PI * (float)k / NB, ca = std::cos(an), sa = std::sin(an);
        std::vector<vec3> bal;
        for (float r : {0.03f, 0.45f, 0.9f, RAY}) bal.push_back({ca * r, hautToile(r) - 0.012f, sa * r});
        tube(s, R, bal, 0.0065f, 4, metal);
        tube(s, R, {{ca * 0.03f, 1.88f, sa * 0.03f}, {ca * 0.70f, hautToile(0.70f) - 0.018f, sa * 0.70f}}, 0.0055f, 4, metal);
    }
}

void parasolFerme(Sortie s, const Repere& R, vec2 toile, vec2 bois, vec2 metal) {
    mat_parasol(s, R, bois, metal, 1.20f);
    tour(s, R, {0, 0, 0}, {{0.02f, 1.25f}, {0.05f, 1.40f}, {0.072f, 1.70f}, {0.078f, 1.95f}, {0.064f, 2.20f}, {0.036f, 2.36f}, {0.022f, 2.42f}},
         16, toile, 8, 0.28f);   // toile roulée sur ses 8 baleines
    tour(s, R, {0, 0, 0}, {{0.088f, 1.87f}, {0.09f, 1.875f}, {0.09f, 1.925f}, {0.088f, 1.93f}}, 16, metal);   // sangle
}

void photophore(Sortie s, const Repere& R) {
    tour(s, R, {0, 0, 0}, {{0.034f, 0}, {0.04f, 0.008f}, {0.04f, 0.082f}, {0.035f, 0.09f}, {0, 0.09f}}, 12, {0, 0});
}

void jardiniere(Sortie s, const Repere& R, vec2 pot, vec2 terre, vec2 buis0, vec2 buis1, uint32_t graine) {
    tour(s, R, {0, 0, 0}, {{0.17f, 0}, {0.17f, 0.02f}, {0.19f, 0.04f}, {0.25f, 0.44f}, {0.275f, 0.455f}, {0.28f, 0.50f}, {0.272f, 0.52f},
                           {0.24f, 0.52f}, {0.235f, 0.48f}}, 20, pot);   // pot à bord roulé
    tour(s, R, {0, 0, 0}, {{0.235f, 0.48f}, {0, 0.49f}}, 20, terre);
    // buis taillé : 7 touffes bosselées qui se chevauchent (feuillage dense, pas une sphère lisse)
    const vec3 centres[7] = {{0, 0.80f, 0}, {0.13f, 0.74f, 0.05f}, {-0.12f, 0.76f, 0.07f}, {0.03f, 0.75f, -0.14f}, {-0.05f, 0.93f, -0.03f}, {0.08f, 0.90f, 0.08f}, {-0.02f, 0.66f, 0.12f}};
    const float rayons[7] = {0.25f, 0.17f, 0.18f, 0.17f, 0.15f, 0.14f, 0.15f};
    constexpr int NA = 10, NL = 7;
    for (int b = 0; b < 7; b++) {
        const vec2 uv = b % 2 ? buis1 : buis0;
        uint32_t prec = 0;
        for (int i = 0; i <= NL; i++) {
            const float th = PI * (float)i / NL;
            const uint32_t a = (uint32_t)s.v.size();
            for (int j = 0; j <= NA; j++) {
                const float ph = 2 * PI * (float)(j % NA) / NA;
                const vec3 d{std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph)};
                const float k = 1.f + 0.16f * (bruit((uint32_t)i, (uint32_t)(j % NA), graine * 7u + (uint32_t)b) - 0.5f) * std::sin(th);
                sommet(s, R.p(centres[b] + d * (rayons[b] * k)), R.d(d), uv);
            }
            if (i > 0) relier(s, prec, a, NA + 1);
            prec = a;
        }
    }
}

}  // namespace wfs::mobilier
