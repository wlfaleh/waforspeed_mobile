#pragma once
// WaForSpeed — disques de sol au centre des virages serrés de la Corniche (WFS-212, RG-03) : le sol de chaque côté s'arrête à
// 90 % du rayon intérieur ; au centre du virage il restait un trou (on y voyait le fond et la mer). Un disque par segment
// serré (rayon < 40 m), centré sur son propre centre de courbure, de rayon « rayon − RW − 0,6 m » (l'union suit l'arc réel).
// WFS-283 (RG-02) : au carrefour simple qui remplace un rond-point, les segments d'entrée et de sortie progressives (rayon
// 26 à 39 m) ont leur centre loin de celui du virage de 22 m : leur disque débordait sur la voie intérieure (sable posé
// sur la ligne de rive, la bordure et le trottoir). Avec borne = vrai, le rayon de chaque disque est aussi borné par la
// distance réelle à l'axe de toute la section (± 20 segments), moins RW + 0,6 m : aucun disque sur la chaussée.
// En-tête seul : lu par plage.cpp et par tests/test_rien_sur_chaussee.cpp.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../game/track.h"   // Track, TrackPoint, RW, SEG ; trace::Section (trace_cartes.h)

namespace wfs::bouchons {

constexpr double RAYON_SERRE = 40;     // m : segments plus serrés = sommet d'épingle (ou du carrefour simple)
constexpr double MARGE = 0.6;          // m au-delà du bord de la chaussée (RW)
constexpr int64_t AUTOUR = 20;         // segments d'axe regardés de part et d'autre de la section (borne)

struct Disque { int64_t seg, sommet; int side; double cx, cz, r; };

// distance (m, dans le plan) du point (x, z) à l'axe du tracé, segments a0 à a1 (ligne brisée de segment en segment)
inline double distanceAxe(Track& t, double x, double z, int64_t a0, int64_t a1) {
    double best = 1e18;
    TrackPoint p = t.point((double)a0 * SEG, 0);
    for (int64_t a = a0 + 1; a <= a1; a++) {
        const TrackPoint q = t.point((double)a * SEG, 0);
        const double ux = q.x - p.x, uz = q.z - p.z, l2 = ux * ux + uz * uz;
        const double u = l2 > 0 ? std::clamp(((x - p.x) * ux + (z - p.z) * uz) / l2, 0.0, 1.0) : 0.0;
        best = std::min(best, std::hypot(x - (p.x + u * ux), z - (p.z + u * uz)));
        p = q;
    }
    return best;
}

// disques de la section s (épingle, lacets, carrefour simple) ; borne : rayon borné par la distance à la chaussée
inline std::vector<Disque> section(Track& t, const trace::Section& s, bool borne) {
    std::vector<Disque> out;
    for (int64_t n = s.debut; n < s.fin; ) {
        const double c = t.segAt(n).curve;
        const double r = std::abs(c) > 1e-9 ? SEG_M / (std::abs(c) * CURVE_K) : 1e9;
        if (r >= RAYON_SERRE) { n++; continue; }
        int64_t f = n;
        while (f < s.fin && t.segAt(f).curve * c > 0 && SEG_M / (std::abs(t.segAt(f).curve) * CURVE_K) < RAYON_SERRE) f++;
        const int side = c > 0 ? 1 : -1; const int64_t sommet = (n + f) / 2;
        for (int64_t k = n; k < f; k++) {
            const double rayon = SEG_M / (std::abs(t.segAt(k).curve) * CURVE_K);
            double rd = rayon - RW - MARGE;
            const TrackPoint cp = t.point((double)k * SEG, side * rayon);
            if (borne) rd = std::min(rd, distanceAxe(t, cp.x, cp.z, s.debut - AUTOUR, s.fin + AUTOUR) - RW - MARGE);
            if (rd <= 1.0) continue;
            out.push_back({k, sommet, side, cp.x, cp.z, rd});
        }
        n = f;
    }
    return out;
}

}  // namespace wfs::bouchons
