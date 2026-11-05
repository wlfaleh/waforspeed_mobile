// WaForSpeed — tracé de la piste (port fidèle de js/world/track.js du jeu web).
// La piste est une suite de segments de SEG unités (1,2 m) avec une courbure et une altitude ;
// pose(a) donne la position 3D et le cap au début du segment absolu a (la route ne reboucle jamais en 3D).
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "../core/configgame.h"   // WFS-128

namespace wfs {

// constantes du jeu web (js/core/config.js)
constexpr double SEG = 200.0;
constexpr double MAX_SPEED = SEG * 60.0;            // unités/s (≈ 260 km/h)
constexpr double M = 0.006;                          // 1 unité = 6 mm
constexpr double SEG_M = SEG * M;                    // 1,2 m par segment
constexpr double RW = 7.5;                           // demi-largeur de route (m)
constexpr double CURVE_K = 0.003, ELEV = 0.5;
constexpr int CH = 32;                               // segments par morceau de route (rendu)
inline const double& CHECK_DIST = g_configJeu.checkpoint.distanceU;   // WFS-128 : 100 000 (600 m), configgame.json
constexpr double KM = 1000.0 / 0.006;

struct MapDef {
    const char* id; const char* name;
    uint32_t seed; std::vector<int> curves; int hills; int sections; double edge;
    int trafficN; double trafficMin, trafficMax;
};
const MapDef& mapById(const char* id);

struct Segment { double curve; double y1, y2; };
struct Pose { double x, y, z, th; };
struct TrackPoint { double x, y, z, th, slope; };

class Track {
public:
    void build(const MapDef& map);
    const Segment& segAt(int64_t a) const { int64_t n = (int64_t)segs_.size(); return segs_[(size_t)(((a % n) + n) % n)]; }
    const Segment& findSegment(double z) const { return segAt((int64_t)std::floor(z / SEG)); }
    double length() const { return length_; }
    // wrap() du jeu web, même calcul ((z % L) + L) % L : les arrondis sont identiques (trafic au bit près)
    double wrap(double z) const { return std::fmod(std::fmod(z, length_) + length_, length_); }
    double relZ(double d) const { d = wrap(d); return d > length_ / 2 ? d - length_ : d; }
    const Pose& pose(int64_t a);
    // point sur la route : s = abscisse absolue (unités), lat = mètres à droite du centre
    TrackPoint point(double s, double lat);
    size_t segmentCount() const { return segs_.size(); }
    const MapDef* map() const { return map_; }
private:
    void addSegment(double curve, double y);
    void addRoad(int enter, int hold, int leave, double curve, double y);
    double lastY() const { return segs_.empty() ? 0.0 : segs_.back().y2; }
    std::vector<Segment> segs_;
    double length_ = 0;
    const MapDef* map_ = nullptr;
    // poses : indices >= 0 dans fwd_, < 0 dans bwd_ (bwd_[0] = pose -1)
    std::vector<Pose> fwd_, bwd_;
};

} // namespace wfs
