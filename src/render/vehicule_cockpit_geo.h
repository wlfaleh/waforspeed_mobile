// WaForSpeed — primitives du cockpit construites par code (géométries de three.js : BoxGeometry, RingGeometry, PlaneGeometry,
// TorusGeometry, CylinderGeometry). Déplacées de src/game/camera_vue.cpp (WFS-66) par WFS-197 pour être partagées avec les
// intérieurs par modèle (vehicule_cockpit.*), sans être recopiées. Repère de la caméra : x à droite, y en haut, −z devant.
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "renderer.h"   // Vertex

namespace wfs::cockpit {

struct Geo {
    std::vector<Vertex> v; std::vector<uint32_t> i;
    void sommet(const mat4& M, vec3 p, vec3 n) {
        const vec3 q = M.transformPoint(p), m = normalize(M.transformDir(n));
        v.push_back({q.x, q.y, q.z, m.x, m.y, m.z, 0, 0});
    }
    void quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d) { i.insert(i.end(), {a, b, c, a, c, d}); }
};
// BoxGeometry(w, h, d)
inline void boite(Geo& g, const mat4& M, float w, float h, float d) {
    const float x = w / 2, y = h / 2, z = d / 2;
    const vec3 n[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const vec3& f : n) {
        // deux axes du plan de la face
        const vec3 u = std::fabs(f.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, t = cross(f, u);
        const uint32_t b = (uint32_t)g.v.size();
        for (int k = 0; k < 4; k++) {
            const float su = (k == 1 || k == 2) ? 1.f : -1.f, st = k >= 2 ? 1.f : -1.f;
            const vec3 p = f + u * su + t * st;
            g.sommet(M, {p.x * x, p.y * y, p.z * z}, f);
        }
        g.quad(b, b + 1, b + 2, b + 3);
    }
}
// CircleGeometry(r, n) / RingGeometry(ri, ro, n) dans le plan xy, face vers +z
inline void anneau(Geo& g, const mat4& M, float ri, float ro, int n) {
    const uint32_t b = (uint32_t)g.v.size();
    for (int k = 0; k <= n; k++) {
        const float a = 2 * PI * k / n, c = std::cos(a), s = std::sin(a);
        g.sommet(M, {c * ri, s * ri, 0}, {0, 0, 1}); g.sommet(M, {c * ro, s * ro, 0}, {0, 0, 1});
    }
    for (int k = 0; k < n; k++) g.quad(b + 2 * k, b + 2 * k + 1, b + 2 * k + 3, b + 2 * k + 2);
}
// PlaneGeometry(w, h) dans le plan xy
inline void plan(Geo& g, const mat4& M, float w, float h) {
    const uint32_t b = (uint32_t)g.v.size();
    g.sommet(M, {-w / 2, -h / 2, 0}, {0, 0, 1}); g.sommet(M, {w / 2, -h / 2, 0}, {0, 0, 1});
    g.sommet(M, {w / 2, h / 2, 0}, {0, 0, 1}); g.sommet(M, {-w / 2, h / 2, 0}, {0, 0, 1});
    g.quad(b, b + 1, b + 2, b + 3);
}
// TorusGeometry(R, r, radial, tubular) : anneau dans le plan xy
inline void tore(Geo& g, const mat4& M, float R, float r, int radial, int tub) {
    const uint32_t b = (uint32_t)g.v.size();
    for (int j = 0; j <= radial; j++) for (int k = 0; k <= tub; k++) {
        const float u = 2 * PI * k / tub, w = 2 * PI * j / radial;
        const vec3 c{std::cos(u) * R, std::sin(u) * R, 0};
        const vec3 p{(R + r * std::cos(w)) * std::cos(u), (R + r * std::cos(w)) * std::sin(u), r * std::sin(w)};
        g.sommet(M, p, normalize(p - c));
    }
    for (int j = 0; j < radial; j++) for (int k = 0; k < tub; k++) {
        const uint32_t a = b + j * (tub + 1) + k, c = a + tub + 1;
        g.quad(a, c, c + 1, a + 1);
    }
}
// CylinderGeometry(rHaut, rBas, h, n), axe y, avec ses deux disques
inline void cylindre(Geo& g, const mat4& M, float rh, float rb, float h, int n) {
    const uint32_t b = (uint32_t)g.v.size();
    for (int k = 0; k <= n; k++) {
        const float a = 2 * PI * k / n, c = std::cos(a), s = std::sin(a);
        const vec3 nr = normalize({c, (rb - rh) / h, s});
        g.sommet(M, {c * rh, h / 2, s * rh}, nr); g.sommet(M, {c * rb, -h / 2, s * rb}, nr);
    }
    for (int k = 0; k < n; k++) g.quad(b + 2 * k, b + 2 * k + 1, b + 2 * k + 3, b + 2 * k + 2);
    for (int haut = 0; haut < 2; haut++) {
        const float y = haut ? h / 2 : -h / 2, rr = haut ? rh : rb;
        const uint32_t c0 = (uint32_t)g.v.size();
        g.sommet(M, {0, y, 0}, {0, haut ? 1.f : -1.f, 0});
        for (int k = 0; k <= n; k++) { const float a = 2 * PI * k / n; g.sommet(M, {std::cos(a) * rr, y, std::sin(a) * rr}, {0, haut ? 1.f : -1.f, 0}); }
        for (int k = 0; k < n; k++) g.i.insert(g.i.end(), {c0, c0 + 1 + k, c0 + 2 + k});
    }
}


} // namespace wfs::cockpit
