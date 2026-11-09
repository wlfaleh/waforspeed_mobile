// WaForSpeed — mesures de la direction dans les virages (WFS-295, ANO-168), sans GPU : partagées par le banc WFS_BANC=virages
// (virages_banc.cpp : captures dans la partie et vues de dessus) et tests/test_virages_fleches.cpp.
//  - rayon réel du cercle décrit volant à fond (dans le monde), au centre, au bord, sur le bas-côté ;
//  - épingles des lacets (R = 22 m ; WFS-379 : col de la Campagne, la Corniche n'en a plus) ;
//  - joueur simulé aux flèches (−1, 0, +1) ou au volant (« Glisser ») qui vise un point de sa file 9 m devant.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "cars.h"
#include "player.h"
#include "track.h"

namespace wfs::banc_virages {

constexpr double DT = 1.0 / 120;

struct PoseMonde { double x, z, cap; };
inline PoseMonde pose(const Player& p, Track& t) {
    const TrackPoint P = t.pointLisse(p.dist, p.playerX * RW);
    return {P.x, P.z, P.th + p.heading};
}
inline Player voiture(const char* id, Aide a, Track& t, double s, double n, double kmh, double cap = 0) {
    Player p; p.reset(carById(id)); p.aide = a;
    p.dist = s; p.position = t.wrap(s); p.playerX = n / RW; p.heading = cap; p.speed = kmh / 3.6 / M;
    return p;
}
// vitesse tenue au gaz seulement (le frein enchaîne sur la marche arrière à basse vitesse)
inline DriveInput tenir(const Player& p, double kmh, float steer) { DriveInput in; in.steer = steer; in.gas = p.kmh() < kmh; return in; }

// courbure (1/m) de l'axe en s (unités du jeu) ; > 0 : virage à droite
inline double courbure(Track& t, double s) { return std::remainder(t.point(s + 2 / M, 0).th - t.point(s - 2 / M, 0).th, 2 * 3.14159265358979) / 4; }

// épingles des lacets (R = 22 m) : virages serrés (R < 40 m) de la 1re section « lacets » de la piste ; sens 1 = à droite,
// −1 = à gauche. WFS-379 : la Corniche n'a plus de lacets ; les bancs et tests de la direction prennent ceux du col de la
// Campagne (mêmes épingles de 165° et 22 m, inchangées)
struct Epingle { double debut, fin; int sens; };
inline std::vector<Epingle> epingles(Track& t) {
    std::vector<Epingle> e; bool dans = false; Epingle c{};
    const trace::Section* l = nullptr;
    for (const trace::Section& s : t.sections()) if (s.type == trace::Type::Lacets) { l = &s; break; }
    if (!l) return e;
    for (double s = l->debut * SEG; s < l->fin * SEG; s += 1 / M) {
        const double k = courbure(t, s);
        if (std::abs(k) > 1 / 40.0) { if (!dans) { dans = true; c.debut = s; c.sens = k > 0 ? 1 : -1; } c.fin = s; }
        else if (dans) { dans = false; e.push_back(c); }
    }
    if (dans) e.push_back(c);
    return e;
}

// joueur simulé : vise un point de la file `file` (demi-largeurs) 9 m devant ; flèches (−1, 0, +1) ou volant proportionnel
inline float piloter(const Player& p, Track& t, double file, bool fleches) {
    const PoseMonde P = pose(p, t);
    const TrackPoint T = t.point(p.dist + 9 / M, file * RW);
    const double dx = T.x - P.x, dz = T.z - P.z, av = dx * std::sin(P.cap) - dz * std::cos(P.cap), dr = dx * std::cos(P.cap) + dz * std::sin(P.cap);
    const double angle = std::atan2(dr, av);
    return fleches ? (angle > 0.03 ? 1.f : angle < -0.03 ? -1.f : 0.f) : (float)std::clamp(angle * 3, -1.0, 1.0);
}

// rayon réel du cercle décrit volant à fond (sens 1 à droite, −1 à gauche), entre 0,6 et 1,0 s (braquage établi) ;
// trace : positions du monde (pour la vue de dessus), si demandée
inline double rayon(const char* id, Track& t, double s, double n, double kmh, float sens, double duree = 1.0, std::vector<PoseMonde>* trace = nullptr) {
    Player p = voiture(id, Aide::Desactivee, t, s, n, kmh);
    double arc = 0, cap0 = 0, cap1 = 0; PoseMonde a = pose(p, t);
    const int nb = (int)std::lround(std::max(duree, 1.0) * 120);
    for (int k = 0; k < nb; k++) {
        p.update(DT, tenir(p, kmh, sens), t, true);
        const PoseMonde b = pose(p, t);
        if (trace) trace->push_back(b);
        if (k == 72) cap0 = b.cap;
        if (k > 72 && k < 120) { arc += std::hypot(b.x - a.x, b.z - a.z); cap1 = b.cap; }
        a = b;
    }
    return arc / std::max(1e-6, std::abs(std::remainder(cap1 - cap0, 2 * 3.14159265358979)));
}

// rayon de la fiche, au centre de la voiture : diamètre entre trottoirs ≈ 4,15 × empattement (comme physique::params)
inline double rayonFiche(const char* id) {
    const physique::Params& P = physique::params(carById(id));
    const double Rc = 4.15 * P.L / 2, Rr = std::sqrt(std::max(1.0, Rc * Rc - P.L * P.L)) - P.voie / 2;
    return std::hypot(Rr, P.L / 2);
}

} // namespace wfs::banc_virages

namespace wfs {
class Game; class Renderer;
int benchVirages(Game& g, Renderer& r, const std::string& out);   // banc WFS_BANC=virages (virages_banc.cpp) : rend le nombre d'écarts
} // namespace wfs
