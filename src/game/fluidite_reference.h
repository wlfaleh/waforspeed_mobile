#pragma once
// WaForSpeed — WFS-293 (RG-07, décision du PO du 2026-11-08 22:41 UTC) : à-coup mesuré contre une référence à 120 Hz.
// À 10-20 img/s, l'indicateur de WFS-247 (écart à une cubique passant par 4 images voisines) a un plancher d'≈ 8 mm en virage :
// 4 images espacées de 66 ms couvrent 0,27 s de virage. Ici : la même course est rejouée à 120 Hz (mêmes états de simulation,
// mêmes commandes, même caméra et même filtre calculés à chaque pas) ; pour chaque image rendue, on compare la position de la
// voiture vue par la caméra (repère de la caméra) à celle de la référence au même instant simulé (interpolée entre deux pas de
// 8,3 ms : erreur ≈ a·dt²/8 < 0,1 mm). Pur calcul (test : tests/test_fluidite_reference.cpp).
#include <algorithm>
#include <cmath>
#include <vector>
#include "../core/math.h"

namespace wfs::fluidite {

// un instant : temps simulé (s), position de la voiture dans le repère de la caméra (m)
struct Pose { double t = 0; vec3 p{}; };

// position de la référence à l'instant t (interpolation linéaire entre les deux pas qui l'encadrent) ; ref triée par t croissant
inline bool referenceA(const std::vector<Pose>& ref, double t, vec3& p) {
    if (ref.size() < 2 || t < ref.front().t || t > ref.back().t) return false;
    const auto it = std::lower_bound(ref.begin(), ref.end(), t, [](const Pose& a, double v) { return a.t < v; });
    if (it == ref.begin()) { p = it->p; return true; }
    const Pose& b = *it; const Pose& a = *(it - 1);
    const double u = b.t > a.t ? (t - a.t) / (b.t - a.t) : 0.0;
    p = a.p + (b.p - a.p) * (float)u;
    return true;
}

// plus grand écart (mm) entre les images rendues et la référence au même instant ; images hors de la référence ignorées
inline double ecartMaxMm(const std::vector<Pose>& images, const std::vector<Pose>& ref, int* comptees = nullptr) {
    double m = 0; int n = 0;
    for (const Pose& im : images) {
        vec3 r;
        if (!referenceA(ref, im.t, r)) continue;
        m = std::max(m, (double)length(im.p - r) * 1000.0); n++;
    }
    if (comptees) *comptees = n;
    return m;
}

} // namespace wfs::fluidite
