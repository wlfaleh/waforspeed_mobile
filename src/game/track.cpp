#include "track.h"
#include <cstring>
#include <numbers>
#include <cmath>

namespace wfs {

// cartes du jeu web (js/world/maps.js) : mêmes graines → mêmes routes
static const MapDef MAPS[] = {
    {"ville", "Ville néon", 88, {2, 3, 4, 5}, 70, 36, 1.7, 42, 0.28, 0.55},
    {"plage", "Corniche", 301, {1, 2, 3}, 0, 32, 1.6, 34, 0.3, 0.55},
    {"campagne", "Campagne", 512, {2, 3, 4}, 120, 36, 1.38, 26, 0.28, 0.5},
    {"circuit", "Grand Prix", 777, {3, 5, 6}, 12, 34, 1.66, 16, 0.5, 0.78},
};
const MapDef& mapById(const char* id) {
    for (const auto& m : MAPS) if (std::strcmp(m.id, id) == 0) return m;
    return MAPS[0];
}

static double easeIn(double a, double b, double p) { return a + (b - a) * p * p; }
static double easeInOut(double a, double b, double p) { return a + (b - a) * ((-std::cos(p * std::numbers::pi) / 2) + 0.5); }

void Track::addSegment(double curve, double y) { segs_.push_back({curve, lastY(), y}); }

void Track::addRoad(int enter, int hold, int leave, double curve, double y) {
    double startY = lastY(), endY = startY + y * SEG;
    int total = enter + hold + leave;
    for (int n = 0; n < enter; n++) addSegment(easeIn(0, curve, (double)n / enter), easeInOut(startY, endY, (double)n / total));
    for (int n = 0; n < hold; n++) addSegment(curve, easeInOut(startY, endY, (double)(enter + n) / total));
    for (int n = 0; n < leave; n++) addSegment(easeInOut(curve, 0, (double)n / leave), easeInOut(startY, endY, (double)(enter + hold + n) / total));
}

void Track::build(const MapDef& map, int version) {
    map_ = &map;
    segs_.clear(); sections_.clear();
    version_ = version == 2 ? 2 : 1;
    if (version_ == 2) sections_ = trace::construire(*this, map);   // WFS-136 : tracé en sections (graine gardée pour le reste)
    else {
    Mulberry32 R(map.seed);
    addRoad(30, 60, 30, 0, 0);
    for (int i = 0; i < map.sections; i++) {
        double t = R.next();
        double hill = R.next() < 0.5 ? std::round((R.next() - 0.5) * map.hills) : 0;
        if (t < 0.22) addRoad(25, 40 + (int)(R.next() * 40), 25, 0, hill);
        else if (t < 0.7) {
            double dir = R.next() < 0.5 ? -1 : 1;
            double c = map.curves[(size_t)std::floor(R.next() * map.curves.size())];
            addRoad(30, 30 + (int)(R.next() * 50), 30, dir * c, hill);
        } else {
            double c = map.curves[0] + R.next() * 2;
            addRoad(20, 25, 20, -c, hill / 2); addRoad(20, 25, 20, c, 0); addRoad(20, 25, 20, -c, -hill / 2);
        }
    }
    addRoad(50, 50, 80, 0, -lastY() / SEG);
    addRoad(20, 30, 20, 0, 0);
    }
    length_ = segs_.size() * SEG;
    fwd_.clear(); bwd_.clear();
    fwd_.push_back({0, segAt(0).y1 * M * ELEV, 0, 0});
}

const Pose& Track::pose(int64_t a) {
    while ((int64_t)fwd_.size() - 1 < a) {
        int64_t i = (int64_t)fwd_.size() - 1;
        const Pose q = fwd_.back();
        const Segment& s = segAt(i);
        fwd_.push_back({q.x + std::sin(q.th) * SEG_M, segAt(i + 1).y1 * M * ELEV, q.z - std::cos(q.th) * SEG_M, q.th + s.curve * CURVE_K});
    }
    while (-(int64_t)bwd_.size() > a) {
        int64_t i = -(int64_t)bwd_.size();   // pose connue la plus basse
        const Pose q = i == 0 ? fwd_[0] : bwd_.back();
        const Segment& s = segAt(i - 1);
        double th = q.th - s.curve * CURVE_K;
        bwd_.push_back({q.x - std::sin(th) * SEG_M, s.y1 * M * ELEV, q.z + std::cos(th) * SEG_M, th});
    }
    return a >= 0 ? fwd_[(size_t)a] : bwd_[(size_t)(-a - 1)];
}

TrackPoint Track::point(double s, double lat) {
    double fa = s / SEG; int64_t a = (int64_t)std::floor(fa); double f = fa - a;
    Pose p0 = pose(a), p1 = pose(a + 1);
    TrackPoint o;
    o.th = p0.th + (p1.th - p0.th) * f;
    o.x = p0.x + (p1.x - p0.x) * f + std::cos(o.th) * lat;
    o.z = p0.z + (p1.z - p0.z) * f + std::sin(o.th) * lat;
    o.y = p0.y + (p1.y - p0.y) * f;
    // WFS-221 (RG-02 b) : pente lissée : la pente d'un segment était constante et changeait d'un coup au segment suivant (la
    // voiture basculait par à-coups dans les montées) ; ici, pente aux lignes de segment = moyenne des deux segments voisins,
    // interpolée le long du segment (continue). Position (y) inchangée : la route et le décor ne bougent pas.
    if (!penteLissee) { o.slope = std::atan2(p1.y - p0.y, SEG_M); return o; }
    const double ym = pose(a - 1).y, y2 = pose(a + 2).y;
    const double g0 = (p1.y - ym) / (2 * SEG_M), g1 = (y2 - p0.y) / (2 * SEG_M);
    o.slope = std::atan(g0 + (g1 - g0) * f);
    return o;
}

TrackPoint Track::pointLisse(double s, double lat) {
    if (!penteLissee) return point(s, lat);   // banc « fluidite », mesure « avant »
    const double fa = s / SEG; const int64_t a = (int64_t)std::floor(fa); const double f = fa - a;
    const Pose q0 = pose(a - 1), q1 = pose(a), q2 = pose(a + 1), q3 = pose(a + 2);
    // Catmull-Rom uniforme : valeur et dérivée (par unité de f) ; passe par q1 (f = 0) et q2 (f = 1) ; pente (y)
    auto cr = [f](double p0, double p1, double p2, double p3, double& d) {
        const double f2 = f * f, f3 = f2 * f;
        d = 0.5 * ((-p0 + p2) + 2 * (2 * p0 - 5 * p1 + 4 * p2 - p3) * f + 3 * (-p0 + 3 * p1 - 3 * p2 + p3) * f2);
        return 0.5 * (2 * p1 + (-p0 + p2) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f2 + (-p0 + 3 * p1 - 3 * p2 + p3) * f3);
    };
    // WFS-293 (RG-04) : B-spline cubique uniforme (courbure continue) pour la position au sol et le cap. Cause de l'à-coup en virage
    // (banc « fluidite ») : avec Catmull-Rom, la courbure (dérivée seconde) saute à chaque segment de 1,2 m ; un point écarté de l'axe
    // (voiture, caméra) est décalé le long d'un cap dont la variation saute aussi : sa vitesse change d'un coup tous les 1,2 m (point
    // à vitesse constante, 2 m de l'axe, virage serré : 7,8 mm à 30 img/s, 6,6 mm à 60). B-spline : 0,65 et 0,12 mm (épingle). La
    // B-spline ne passe pas exactement par les poses : écart de L²/(6R) au plus (≈ 6 mm pour un rayon de 40 m), invisible.
    auto bs = [f](double p0, double p1, double p2, double p3, double& d) {
        const double f2 = f * f, f3 = f2 * f;
        d = (-p0 + p2) * 0.5 + (p0 - 2 * p1 + p2) * f + (-p0 + 3 * p1 - 3 * p2 + p3) * 0.5 * f2;
        return ((p0 + 4 * p1 + p2) + (-3 * p0 + 3 * p2) * f + (3 * p0 - 6 * p1 + 3 * p2) * f2 + (-p0 + 3 * p1 - 3 * p2 + p3) * f3) / 6.0;
    };
    double dx, dy, dz, dth;
    const double x = bs(q0.x, q1.x, q2.x, q3.x, dx), y = cr(q0.y, q1.y, q2.y, q3.y, dy), z = bs(q0.z, q1.z, q2.z, q3.z, dz);
    TrackPoint o;
    // sens de th : x += sin(th), z −= cos(th) le long du tracé (Track::pose) ; caps déroulés autour de q1 (continuité du tour : th de
    // point() ± π gardée)
    const double t0 = q1.th + std::remainder(q0.th - q1.th, 2 * PI), t2 = q1.th + std::remainder(q2.th - q1.th, 2 * PI),
                 t3 = t2 + std::remainder(q3.th - q2.th, 2 * PI), thP = q1.th + (q2.th - q1.th) * f;
    o.th = thP + std::remainder(bs(t0, q1.th, t2, t3, dth) - thP, 2 * PI);
    o.x = x + std::cos(o.th) * lat; o.z = z + std::sin(o.th) * lat; o.y = y;
    o.slope = std::atan2(dy, std::hypot(dx, dz));
    return o;
}

} // namespace wfs
