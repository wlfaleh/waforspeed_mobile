// WaForSpeed — pièces modelées de la vue Cockpit, construites par code (WFS-323) : déplacées de vehicule_cockpit.cpp par
// WFS-414 (sans aucun changement) pour être partagées avec le volant (vehicule_cockpit_volant.cpp), dont la géométrie se calcule
// sans rendu et se vérifie par tests/test_cockpit.cpp. Repère de la pièce : celui de la matrice M passée à chaque fonction.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "vehicule_cockpit_geo.h"

namespace wfs::cockpit::formes {

constexpr float GRAIN = 12.f;   // répétitions du grain par mètre (motif de 8 cm, cellules de 2,6 mm : grain réel du cuir et du plastique)

inline void ajouterUV(Geo& g, const mat4& M, vec3 p, vec3 n, float u, float v) {
    g.sommet(M, p, n); g.v.back().u = u; g.v.back().v = v;
}

// ---------- WFS-323 : pièces modelées (règle de l'utilisateur du 2026-11-09 : aucune forme géométrique nue dans le monde 3D) ----------
// Chaque pièce ajoutée ou reprise par WFS-323 a les arêtes arrondies et chanfreinées d'une pièce moulée ou gainée (rayons de 2 à
// 40 mm, comme les vraies garnitures : moulage du plastique, couture du cuir) au lieu d'une boîte à arêtes vives.

// triangle orienté : face avant (sens trigonométrique, comme boite()) du côté où pointent les normales des sommets ; les pièces
// sont en double face (MAT_DOUBLE), et le shader retourne la normale d'une face vue de dos : un mauvais sens l'assombrirait
inline void triOriente(Geo& g, uint32_t a, uint32_t b, uint32_t c) {
    const Vertex &A = g.v[a], &B = g.v[b], &C = g.v[c];
    const vec3 pa{A.px, A.py, A.pz}, pb{B.px, B.py, B.pz}, pc{C.px, C.py, C.pz};
    const vec3 n{A.nx + B.nx + C.nx, A.ny + B.ny + C.ny, A.nz + B.nz + C.nz};
    if (dot(cross(pb - pa, pc - pa), n) >= 0) g.i.insert(g.i.end(), {a, b, c}); else g.i.insert(g.i.end(), {a, c, b});
}
inline void quadOriente(Geo& g, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { triOriente(g, a, b, c); triOriente(g, a, c, d); }

// contour d'un rectangle à coins arrondis (w × d, rayon r) dans le plan (x, z) : points et normales vers l'extérieur ;
// `n` pas par coin ; rayon égal à la demi-largeur : contour en stade (bouts ronds)
struct Contour { std::vector<vec2> p, n; };
inline Contour contourArrondi(float w, float d, float r, int n = 5) {
    r = std::clamp(r, 0.f, std::min(w, d) * 0.5f - 1e-4f);
    Contour c;
    const vec2 centres[4] = {{w / 2 - r, d / 2 - r}, {-w / 2 + r, d / 2 - r}, {-w / 2 + r, -d / 2 + r}, {w / 2 - r, -d / 2 + r}};
    for (int k = 0; k < 4; k++) for (int j = 0; j <= n; j++) {
        const float a = (k + (float)j / n) * PI * 0.5f;
        const vec2 dir{std::cos(a), std::sin(a)};
        c.p.push_back({centres[k].x + dir.x * r, centres[k].y + dir.y * r}); c.n.push_back(dir);
    }
    return c;
}

// pièce moulée : contour arrondi w × d (plan x, z du repère M), épaisseur e le long de y, arêtes chanfreinées de b (dessus et
// dessous), dessous réduit de `dos` (dépouille du moulage) ; uv en mètres pour le grain
inline void dalle(Geo& g, const mat4& M, float w, float d, float e, float r, float b, float dos = 1.f) {
    b = std::clamp(b, 0.f, std::min({e * 0.5f, w * 0.25f, d * 0.25f}));
    const Contour c = contourArrondi(w, d, r);
    const size_t n = c.p.size();
    // anneaux : (retrait, hauteur, échelle, pente de la normale)
    struct Anneau { float ins, y, k, ny; };
    const Anneau A[4] = {{b, e / 2, 1, 0.7f}, {0, e / 2 - b, 1, 0}, {0, -e / 2 + b, dos, 0}, {b, -e / 2, dos, -0.7f}};
    uint32_t debut[4];
    for (int a = 0; a < 4; a++) {
        debut[a] = (uint32_t)g.v.size();
        for (size_t k = 0; k < n; k++) {
            const vec2 p = c.p[k], m = c.n[k];
            const vec3 q{(p.x - m.x * A[a].ins) * A[a].k, A[a].y, (p.y - m.y * A[a].ins) * A[a].k};
            const vec3 nn = normalize(vec3{m.x * (1 - std::fabs(A[a].ny)), A[a].ny, m.y * (1 - std::fabs(A[a].ny))});
            ajouterUV(g, M, q, nn, q.x * GRAIN, (q.z + q.y) * GRAIN);
        }
    }
    for (int a = 0; a < 3; a++) for (size_t k = 0; k < n; k++) {
        const uint32_t i0 = debut[a] + (uint32_t)k, i1 = debut[a] + (uint32_t)((k + 1) % n);
        quadOriente(g, i0, i1, i1 + (uint32_t)n, i0 + (uint32_t)n);
    }
    for (int haut = 0; haut < 2; haut++) {   // faces du dessus et du dessous (normales plates)
        const Anneau& an = A[haut ? 0 : 3];
        const float s = haut ? 1.f : -1.f;
        const uint32_t c0 = (uint32_t)g.v.size();
        ajouterUV(g, M, {0, an.y, 0}, {0, s, 0}, 0, 0);
        for (size_t k = 0; k < n; k++) {
            const vec3 q{(c.p[k].x - c.n[k].x * an.ins) * an.k, an.y, (c.p[k].y - c.n[k].y * an.ins) * an.k};
            ajouterUV(g, M, q, {0, s, 0}, q.x * GRAIN, q.z * GRAIN);
        }
        for (size_t k = 0; k < n; k++) triOriente(g, c0, c0 + 1 + (uint32_t)k, c0 + 1 + (uint32_t)((k + 1) % n));
    }
}
// même pièce, face principale (dessus de la dalle) tournée vers +z du repère M (écrans, rétroviseur, airbag, boîte à gants)
inline void dalleFace(Geo& g, const mat4& M, float w, float h, float e, float r, float b, float dos = 1.f) {
    dalle(g, M * rotateX(PI / 2), w, h, e, r, b, dos);
}

// repère d'une pièce le long d'un segment (même convention que barre() : y le long du segment, x = y × réf, z = x × y)
inline mat4 repereSegment(vec3 a, vec3 b) {
    const vec3 y = normalize(b - a), ref = std::fabs(y.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 0, 1};
    const vec3 x = normalize(cross(y, ref)), z = cross(x, y);
    mat4 B = mat4::identity();
    B.at(0, 0) = x.x; B.at(1, 0) = x.y; B.at(2, 0) = x.z;
    B.at(0, 1) = y.x; B.at(1, 1) = y.y; B.at(2, 1) = y.z;
    B.at(0, 2) = z.x; B.at(1, 2) = z.y; B.at(2, 2) = z.z;
    return translate(a) * B;
}
// profilé : section arrondie w (axe x du repère) × t (axe z), rayon r, balayée de a à b, bouts fermés par une calotte
// chanfreinée (montants, traverse, rails de toit, branches du volant, bras d'essuie-glace)
inline void profile(Geo& g, const mat4& M, vec3 a, vec3 b, float w, float t, float r, float tB = -1, float wB = -1) {
    const float l = length(b - a);
    if (l < 1e-5f) return;
    if (tB < 0) tB = t;
    if (wB < 0) wB = w;
    const mat4 R = M * repereSegment(a, b);
    const Contour c = contourArrondi(w, t, r, 4);
    const size_t n = c.p.size();
    const float bout = std::min(r * 0.6f, l * 0.2f);   // calotte : la section se resserre aux deux bouts
    const float ys[4] = {0, bout, l - bout, l}, ks[4] = {0.6f, 1, 1, 0.6f};
    uint32_t debut[4];
    for (int s = 0; s < 4; s++) {
        debut[s] = (uint32_t)g.v.size();
        const float f = ys[s] / l, kx = (w + (wB - w) * f) / w, kz = (t + (tB - t) * f) / t;
        const float ny = s == 0 ? -0.6f : s == 3 ? 0.6f : 0.f;
        for (size_t k = 0; k < n; k++) {
            const vec3 q{c.p[k].x * kx * ks[s], ys[s], c.p[k].y * kz * ks[s]};
            const vec3 nn = normalize(vec3{c.n[k].x * (1 - std::fabs(ny)), ny, c.n[k].y * (1 - std::fabs(ny))});
            ajouterUV(g, R, q, nn, (c.p[k].x + c.p[k].y) * GRAIN, ys[s] * GRAIN);
        }
    }
    for (int s = 0; s < 3; s++) for (size_t k = 0; k < n; k++) {
        const uint32_t i0 = debut[s] + (uint32_t)k, i1 = debut[s] + (uint32_t)((k + 1) % n);
        quadOriente(g, i0, i1, i1 + (uint32_t)n, i0 + (uint32_t)n);
    }
    for (int s : {0, 3}) {
        const uint32_t c0 = (uint32_t)g.v.size();
        ajouterUV(g, R, {0, ys[s], 0}, {0, s == 0 ? -1.f : 1.f, 0}, 0, 0);
        for (size_t k = 0; k < n; k++) triOriente(g, c0, debut[s] + (uint32_t)k, debut[s] + (uint32_t)((k + 1) % n));
    }
}
// pièce moulée balayée le long d'une ligne brisée (visière du combiné) : à chaque station, section arrondie de largeur w (en
// travers, axe x) et d'épaisseur e (verticale), rayon r ; normales lissées entre stations ; bouts fermés par une calotte chanfreinée
inline void lisse(Geo& g, const mat4& M, const std::vector<vec3>& pts, float w, float e, float r) {
    if (pts.size() < 2) return;
    const Contour c = contourArrondi(w, e, r, 4);
    const size_t n = c.p.size(), m = pts.size();
    std::vector<uint32_t> debut;
    std::vector<vec3> ts(m), us(m), ss(m);
    for (size_t i = 0; i < m; i++) {
        const vec3 t = normalize(pts[std::min(i + 1, m - 1)] - pts[i > 0 ? i - 1 : 0]);
        const vec3 u = normalize(vec3{0, 1, 0} - t * dot(vec3{0, 1, 0}, t));
        ts[i] = t; us[i] = u; ss[i] = cross(t, u);
    }
    auto anneau = [&](size_t i, float k, vec3 decal, float nt) {
        debut.push_back((uint32_t)g.v.size());
        for (size_t j = 0; j < n; j++) {
            const vec3 q = pts[i] + decal + ss[i] * (c.p[j].x * k) + us[i] * (c.p[j].y * k);
            const vec3 nn = normalize((ss[i] * c.n[j].x + us[i] * c.n[j].y) * (1 - std::fabs(nt)) + ts[i] * nt);
            ajouterUV(g, M, q, nn, q.x * GRAIN, (q.y + q.z) * GRAIN);
        }
    };
    const float ch = std::min(r, e * 0.4f);
    anneau(0, 0.8f, ts[0] * -ch, -0.6f);
    for (size_t i = 0; i < m; i++) anneau(i, 1.f, {}, 0.f);
    anneau(m - 1, 0.8f, ts[m - 1] * ch, 0.6f);
    for (size_t a = 0; a + 1 < debut.size(); a++) for (size_t j = 0; j < n; j++) {
        const uint32_t i0 = debut[a] + (uint32_t)j, i1 = debut[a] + (uint32_t)((j + 1) % n);
        quadOriente(g, i0, i1, i1 + (uint32_t)n, i0 + (uint32_t)n);
    }
    for (size_t bout : {(size_t)0, debut.size() - 1}) {
        const size_t i = bout == 0 ? 0 : m - 1;
        const vec3 centre = pts[i] + ts[i] * (bout == 0 ? -ch : ch);
        const uint32_t c0 = (uint32_t)g.v.size();
        ajouterUV(g, M, centre, ts[i] * (bout == 0 ? -1.f : 1.f), 0, 0);
        for (size_t j = 0; j < n; j++) triOriente(g, c0, debut[bout] + (uint32_t)j, debut[bout] + (uint32_t)((j + 1) % n));
    }
}

// cadre arrondi (lunette d'aérateur, entourage d'écran) : contour extérieur w × h, bande de largeur ep, profondeur pr, face
// tournée vers +z du repère, bords intérieur et extérieur arrondis (un pas de biseau)
inline void cadreArrondi(Geo& g, const mat4& M, float w, float h, float r, float ep, float pr) {
    const Contour ce = contourArrondi(w, h, r), ci = contourArrondi(w - 2 * ep, h - 2 * ep, std::max(0.f, r - ep));
    const size_t n = ce.p.size();
    const float b = std::min(ep * 0.35f, pr * 0.4f);
    // anneaux : extérieur arrière, extérieur avant (biseau), intérieur avant (biseau), intérieur arrière
    auto anneau = [&](const Contour& c, float ins, float z, float nz, float sens) {
        const uint32_t d0 = (uint32_t)g.v.size();
        for (size_t k = 0; k < n; k++) {
            const vec3 q{c.p[k].x - c.n[k].x * ins * sens, c.p[k].y - c.n[k].y * ins * sens, z};
            const vec3 nn = normalize(vec3{c.n[k].x * sens * (1 - std::fabs(nz)), c.n[k].y * sens * (1 - std::fabs(nz)), nz});
            ajouterUV(g, M, q, nn, q.x * GRAIN, q.y * GRAIN);
        }
        return d0;
    };
    const uint32_t a0 = anneau(ce, 0, -pr, 0, 1), a1 = anneau(ce, 0, -b, 0, 1), a2 = anneau(ce, b, 0, 0.7f, 1),
                   a3 = anneau(ci, b, 0, 0.7f, -1), a4 = anneau(ci, 0, -b, 0, -1), a5 = anneau(ci, 0, -pr, 0, -1);
    for (auto [u, v] : {std::pair{a0, a1}, {a1, a2}, {a2, a3}, {a3, a4}, {a4, a5}})
        for (size_t k = 0; k < n; k++) {
            const uint32_t k1 = (uint32_t)((k + 1) % n);
            quadOriente(g, u + (uint32_t)k, u + k1, v + k1, v + (uint32_t)k);
        }
}

// jante du volant (plan xy, face +z vers le conducteur) : tube de rayon r autour d'un cercle de rayon R, aplati en bas
// (méplat : la ligne médiane ne descend pas sous −R × (1 − meplat), raccord arrondi) ; uv pour le grain du cuir
inline vec2 ligneJante(float a, float R, float meplat, float meplatHaut = 0) {
    vec2 p{std::cos(a) * R, std::sin(a) * R};
    if (meplatHaut > 0) {   // WFS-201 : même raccord en haut (208)
        const float haut = R * (1 - meplatHaut), raccord = R * 0.08f;
        if (p.y > haut - raccord) {
            const float t = std::clamp((p.y - (haut - raccord)) / (2 * raccord), 0.f, 1.f);
            p.y = p.y + (haut - p.y) * t * t * (3 - 2 * t) * (p.y > haut ? 1.f : 0.5f);
            if (p.y > haut) p.y = haut;
        }
    }
    if (meplat > 0) {
        const float bas = -R * (1 - meplat), raccord = R * 0.08f;
        if (p.y < bas + raccord) {   // fonction douce (min lissé) entre le cercle et la droite du méplat
            const float t = std::clamp((bas + raccord - p.y) / (2 * raccord), 0.f, 1.f);
            p.y = p.y + (bas - p.y) * t * t * (3 - 2 * t) * (p.y < bas ? 1.f : 0.5f);
            if (p.y < bas) p.y = bas;
        }
    }
    return p;
}
inline void jante(Geo& g, const mat4& M, float R, float r, float meplat, float a0, float a1, int n, int m, float meplatHaut = 0) {
    const uint32_t b = (uint32_t)g.v.size();
    for (int k = 0; k <= n; k++) {
        const float a = a0 + (a1 - a0) * k / n;
        const vec2 c = ligneJante(a, R, meplat, meplatHaut), c2 = ligneJante(a + 0.01f, R, meplat, meplatHaut);
        const vec2 t{c2.x - c.x, c2.y - c.y};
        const float tl = std::max(1e-6f, std::sqrt(t.x * t.x + t.y * t.y));
        const vec3 nr{t.y / tl, -t.x / tl, 0};   // normale dans le plan, vers l'extérieur
        for (int j = 0; j <= m; j++) {
            const float w = 2 * PI * j / m;
            const vec3 n3 = nr * std::cos(w) + vec3{0, 0, 1} * std::sin(w);
            ajouterUV(g, M, vec3{c.x, c.y, 0} + n3 * r, n3, (a - a0) * R * GRAIN, (float)j / m * 2 * PI * r * GRAIN);
        }
    }
    for (int k = 0; k < n; k++) for (int j = 0; j < m; j++) {
        const uint32_t a = b + k * (m + 1) + j, c = a + m + 1;
        g.quad(a, c, c + 1, a + 1);
    }
}

} // namespace wfs::cockpit::formes
