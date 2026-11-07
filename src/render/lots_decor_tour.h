// WaForSpeed — décor fixe préparé au chargement (WFS-268) : passage d'un tour au suivant (sans le renderer : testé seul,
// tests/lots_decor_test.cpp).
// Constat (WFS-268, journal) : la route ne se referme pas en 3D, mais chaque tour est EXACTEMENT le précédent, tourné autour
// de y et décalé : monde(tour k) = R(k·θ) (x − p0) + q_k, avec p0 le début du tour 0, q_k le début du tour k et θ la
// différence de cap entre le début du tour 1 et celui du tour 0 (signe vérifié sur un point du tour, convention de Track).
#pragma once
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include "../core/math.h"
#include "../game/track.h"

namespace wfs::lots {

struct Tour {
    int64_t L = 0;      // segments d'un tour
    double theta = 0;   // rotation d'un tour (rad, autour de y)
    vec3 p0{};          // début du tour 0 (monde)
    double ecart = 0;   // contrôle : écart (m) entre un point du tour 1 et le même point du tour 0 transporté
};

// mesure du passage d'un tour au suivant (poses calculées ici : appeler sur le thread principal)
inline Tour mesurerTour(Track& t) {
    Tour tr; tr.L = (int64_t)t.segmentCount();
    if (tr.L <= 0) return tr;
    const TrackPoint a = t.point(0, 0), b = t.point((double)tr.L * SEG, 0);
    tr.p0 = {(float)a.x, (float)a.y, (float)a.z};
    tr.theta = b.th - a.th;
    const TrackPoint pa = t.point((double)(tr.L / 3) * SEG, 12), pb = t.point((double)(tr.L + tr.L / 3) * SEG, 12);
    auto ecart = [&](double th) {
        const double dx = pa.x - a.x, dz = pa.z - a.z, c = std::cos(th), s = std::sin(th);
        return std::hypot(b.x + c * dx - s * dz - pb.x, b.z + s * dx + c * dz - pb.z);
    };
    if (ecart(-tr.theta) < ecart(tr.theta)) tr.theta = -tr.theta;
    tr.ecart = ecart(tr.theta);
    return tr;
}

// matrice tour k → monde (tour 0 : identité)
inline mat4 matriceDuTour(Track& t, const Tour& tr, int64_t k) {
    if (k == 0 || tr.L <= 0) return mat4::identity();
    const TrackPoint q = t.point((double)(k * tr.L) * SEG, 0);   // début du tour k
    const vec3 qk{(float)q.x, (float)q.y, (float)q.z};
    // notre R : x' = c·x − s·z, z' = s·x + c·z ; rotateY(a) de math.h : x' = c·x + s·z, z' = −s·x + c·z → rotateY(−k·θ)
    return translate(qk) * rotateY((float)(-(double)k * tr.theta)) * translate(tr.p0 * -1.f);
}

// WFS-268 (RG-08) : matrices des tours pour les systèmes qui préparent leur contenu du tour 0 au chargement (immeubles du kit,
// commerces) ; cache des 8 derniers tours, aucune allocation
struct MatricesTours {
    Tour tour{};
    void mesurer(Track& t) { tour = mesurerTour(t); for (Case& c : cases_) c = Case{}; }
    int64_t L() const { return tour.L; }
    int64_t tourDe(int64_t a) const { const int64_t n = tour.L; return n <= 0 ? 0 : (a >= 0 ? a / n : -((-a + n - 1) / n)); }
    const mat4& matrice(Track& t, int64_t k) {
        Case& c = cases_[(size_t)(((k % 8) + 8) % 8)];
        if (c.k != k) { c.k = k; c.m = matriceDuTour(t, tour, k); }
        return c.m;
    }
private:
    struct Case { int64_t k = INT64_MIN; mat4 m = mat4::identity(); };
    std::array<Case, 8> cases_{};
};

} // namespace wfs::lots
