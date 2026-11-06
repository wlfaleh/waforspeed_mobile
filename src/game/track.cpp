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
    o.slope = std::atan2(p1.y - p0.y, SEG_M);
    return o;
}

} // namespace wfs
