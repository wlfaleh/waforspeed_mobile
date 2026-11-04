// WaForSpeed — masquage des objets (WFS-82) : voir masquage.h.
#include "masquage.h"
#include <algorithm>

namespace wfs::masquage {

static Plan plan(float a, float b, float c, float d) {
    const float l = std::sqrt(a * a + b * b + c * c);
    if (l < 1e-12f) return {{0, 0, 0}, 1};   // plan dégénéré : ne masque rien
    return {{a / l, b / l, c / l}, d / l};
}

// Gribb et Hartmann : combinaisons des lignes de la matrice (clip = M · p)
Frustum depuis(const mat4& m) {
    auto L = [&](int r, int c) { return m.at(r, c); };
    Frustum f; f.actif = true;
    f.p[0] = plan(L(3, 0) + L(0, 0), L(3, 1) + L(0, 1), L(3, 2) + L(0, 2), L(3, 3) + L(0, 3));   // x ≥ −w
    f.p[1] = plan(L(3, 0) - L(0, 0), L(3, 1) - L(0, 1), L(3, 2) - L(0, 2), L(3, 3) - L(0, 3));   // x ≤ w
    f.p[2] = plan(L(3, 0) + L(1, 0), L(3, 1) + L(1, 1), L(3, 2) + L(1, 2), L(3, 3) + L(1, 3));   // y ≥ −w
    f.p[3] = plan(L(3, 0) - L(1, 0), L(3, 1) - L(1, 1), L(3, 2) - L(1, 2), L(3, 3) - L(1, 3));   // y ≤ w
    f.p[4] = plan(L(2, 0), L(2, 1), L(2, 2), L(2, 3));                                           // z ≥ 0
    f.p[5] = plan(L(3, 0) - L(2, 0), L(3, 1) - L(2, 1), L(3, 2) - L(2, 2), L(3, 3) - L(2, 3));   // z ≤ w
    return f;
}

bool visible(const Frustum& f, const Sphere& s) {
    if (!f.actif || s.r < 0) return true;
    for (const Plan& p : f.p) if (dot(p.n, s.c) + p.d < -s.r) return false;
    return true;
}

Sphere monde(const mat4& m, const Sphere& s) {
    if (s.r < 0) return s;
    const float sx = length(vec3{m.at(0, 0), m.at(1, 0), m.at(2, 0)}), sy = length(vec3{m.at(0, 1), m.at(1, 1), m.at(2, 1)}),
                sz = length(vec3{m.at(0, 2), m.at(1, 2), m.at(2, 2)});
    return {m.transformPoint(s.c), s.r * std::max({sx, sy, sz})};
}

float distanceMax(float rayon, bool basse, float finBrouillard) {
    if (rayon < 0) return 1e9f;
    if (rayon < 3) return basse ? 150.f : 200.f;
    if (rayon < 10) return basse ? std::min(finBrouillard, 420.f) : finBrouillard;
    return 1e9f;
}

} // namespace wfs::masquage
