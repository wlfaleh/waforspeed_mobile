// WaForSpeed — banc de conduite de l'aide à la direction (WFS-217) et joueur simulé qui suit la ligne idéale (aussi utilisé
// par tests/test_aide_direction.cpp). Le joueur vise un point du milieu de la route devant lui (poursuite pure, en
// coordonnées du monde, comme un humain qui regarde où va la route) et tient la vitesse de virage normale : celle conseillée
// au trafic (60 km/h dans les épingles, 40 km/h dans l'anneau du rond-point), 70 km/h ailleurs.
// Banc (fin de benchConduiteLibre) : épingle du Grand Prix, 1re épingle des lacets de la Campagne, ronds-points de la Ville
// et de la Corniche, aide activée puis désactivée ; trace de la trajectoire au journal ; captures conduite-epingle et
// conduite-rond-point (aide activée, au milieu du virage, dans la partie) ; les autres cas roulent avec la voiture seule.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include "player.h"
#include "trace_cartes.h"
#include "track.h"

namespace wfs {
class Game; class Renderer;

// vitesse de virage normale (unités/s) : la plus basse conseillée au trafic sur les 40 m qui viennent, 70 km/h au plus
inline double vitesseVirageNormale(const Track& t, double dist, double facteur = 1) {
    double v = 70 / 3.6 / M;
    for (double m = 0; m <= 40; m += 2) v = std::min(v, trace::vitesseTrafic(t, t.wrap(dist + m / M)));
    return v * facteur;
}

// commandes du joueur simulé ; volant = false : volant lâché (vitesse tenue seulement)
inline DriveInput joueurLigneIdeale(const Player& p, Track& t, bool volant = true, double facteur = 1) {
    DriveInput in;
    const double v = std::max(std::abs(p.speed * M), 1.0), cible = vitesseVirageNormale(t, p.dist, facteur);
    in.gas = p.speed < cible * 0.97; in.brake = p.speed > cible * 1.06;
    if (!volant) return in;
    const TrackPoint P = t.point(p.dist, p.playerX * RW);
    const double ld = std::max(5.0, 0.6 * v);   // point visé à 0,6 s devant (5 m au moins)
    const TrackPoint T = t.point(p.dist + ld / M, 0);
    const double psi = P.th + p.heading, dx = T.x - P.x, dz = T.z - P.z;
    const double av = dx * std::sin(psi) - dz * std::cos(psi), dr = dx * std::cos(psi) + dz * std::sin(psi);
    const double a = std::atan2(dr, av), kappa = 2 * std::sin(a) / std::max(1.0, std::hypot(dx, dz));   // courbure voulue (1/m)
    const double roue = std::atan(kappa * p.perf.wb / p.perf.handling), roueMax = Phys::maxSteer / (1 + v / Phys::steerV);
    in.steer = (float)std::clamp(roue / roueMax, -1.0, 1.0);
    return in;
}

// banc d'essai : rend le nombre d'écarts (sorties de route, sauts de cap > 0,05 rad par image)
int benchAideDirection(Game& g, Renderer& r, const std::string& out);

} // namespace wfs
