// WaForSpeed — WFS-212 (RG-01 à RG-04) : « rien sur la chaussée » là où le tracé se replie sur lui-même (rond-point, épingle,
// lacets) : un trottoir, un bas-côté ou un objet du décor posé pour un segment peut tomber sur la chaussée d'une AUTRE partie
// du tracé (au rond-point, l'anneau fait le tour de l'îlot et son trottoir extérieur coupait la route d'entrée).
// Calcul seul (aucun appel au rendu) : utilisé par la Ville (ville.cpp), le décor des cartes (decor_carte.cpp) et testé par
// tests/test_rien_sur_chaussee.cpp.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <utility>
#include <vector>
#include "../core/math.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"

namespace wfs::chaussee {

constexpr double ABORDS_M = 80;   // axe gardé de part et d'autre de la section (branches d'entrée et de sortie)
constexpr double PROPRE_M = 30;   // partie du tracé du segment lui-même (jamais « une autre » chaussée)

struct Axe { int64_t c = 0, rayonSeg = 0; std::vector<std::pair<int64_t, vec3>> pts; };   // centre (segment), portée, axe (segment, point)

inline int64_t modL(int64_t a, int64_t L) { return ((a % L) + L) % L; }
inline int64_t ecartSigne(int64_t a, int64_t b, int64_t L) { int64_t d = modL(a - b, L); return d > L / 2 ? d - L : d; }

// axe de la route autour de chaque section repliée du tracé (types demandés), ± ABORDS_M
inline std::vector<Axe> axes(Track& t, std::initializer_list<trace::Type> types) {
    std::vector<Axe> out;
    const int64_t L = (int64_t)t.segmentCount(), m = (int64_t)(ABORDS_M / SEG_M);
    for (const trace::Section& s : trace::sections(t)) {
        bool ok = false; for (trace::Type ty : types) ok = ok || s.type == ty;
        if (!ok) continue;
        Axe a; a.c = (s.debut + s.fin) / 2; a.rayonSeg = (s.fin - s.debut) / 2 + m + (int64_t)(PROPRE_M / SEG_M);
        for (int64_t k = s.debut - m; k <= s.fin + m; k++) {
            const TrackPoint p = t.point((double)k * SEG, 0);
            a.pts.push_back({modL(k, L), vec3{(float)p.x, (float)p.y, (float)p.z}});
        }
        out.push_back(std::move(a));
    }
    return out;
}

// le segment seg est-il dans la zone d'une des sections ?
inline bool proche(const std::vector<Axe>& ax, int64_t L, int64_t seg) {
    const int64_t sm = modL(seg, L);
    for (const Axe& a : ax) if (std::llabs(ecartSigne(sm, a.c, L)) <= a.rayonSeg) return true;
    return false;
}

// le point q (monde) est-il à moins de RW + marge de l'axe d'une AUTRE partie du tracé que celle du segment seg ?
inline bool surAutre(const std::vector<Axe>& ax, int64_t L, const vec3& q, int64_t seg, float marge) {
    const int64_t sm = modL(seg, L), propre = (int64_t)(PROPRE_M / SEG_M);
    const float lim2 = ((float)RW + marge) * ((float)RW + marge);
    for (const Axe& a : ax) {
        if (std::llabs(ecartSigne(sm, a.c, L)) > a.rayonSeg) continue;
        for (const auto& [sp, o] : a.pts)
            if (std::llabs(ecartSigne(sp, sm, L)) >= propre && (o.x - q.x) * (o.x - q.x) + (o.z - q.z) * (o.z - q.z) < lim2) return true;
    }
    return false;
}

} // namespace wfs::chaussee
