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

// vitesse de virage normale (unités/s) : la plus basse conseillée au trafic sur les 40 m qui viennent, 70 km/h au plus.
// WFS-241 : et celle qu'un conducteur prend vraiment : 0,7 g d'accélération latérale au plus dans le rayon du virage
// (épingle de 20 m : 42 km/h ; le trafic y est conseillé à 60 km/h, soit 1,4 g, impossible pour une vraie voiture)
inline double vitesseVirageNormale(const Track& t, double dist, double facteur = 1) {
    double v = 70 / 3.6 / M;
    for (double m = 0; m <= 40; m += 2) {
        const double z = t.wrap(dist + m / M), k = std::abs(t.findSegment(z).curve) * CURVE_K / SEG_M;   // courbure (1/m)
        v = std::min(v, trace::vitesseTrafic(t, z));
        if (k > 1e-4) v = std::min(v, std::sqrt(0.7 * 9.81 / k) / M);
    }
    return v * facteur;
}

// commandes du joueur simulé ; volant = false : volant lâché (vitesse tenue seulement)
inline DriveInput joueurLigneIdeale(const Player& p, Track& t, bool volant = true, double facteur = 1) {
    DriveInput in;
    const double v = std::max(std::abs(p.speed * M), 1.0), cible = vitesseVirageNormale(t, p.dist, facteur);
    in.gas = p.speed < cible * 0.97; in.brake = p.speed > cible * 1.06;
    if (!volant) return in;
    const TrackPoint P = t.point(p.dist, p.playerX * RW);
    const double ld = std::max(5.0, (p.modeleMonde() ? 0.8 : 0.6) * v);   // point visé à 0,6 s devant (0,8 s : WFS-241, conduite réelle), 5 m au moins
    const TrackPoint T = t.point(p.dist + ld / M, 0);
    const double psi = P.th + p.heading, dx = T.x - P.x, dz = T.z - P.z;
    const double av = dx * std::sin(psi) - dz * std::cos(psi), dr = dx * std::cos(psi) + dz * std::sin(psi);
    const double a = std::atan2(dr, av), kappa = 2 * std::sin(a) / std::max(1.0, std::hypot(dx, dz));   // courbure voulue (1/m)
    in.steer = (float)p.braquagePour(kappa, v);   // WFS-241 : selon le modèle de conduite en cours
    return in;
}

// WFS-241 : pilote automatique (démonstrations, bancs) avec la conduite réelle. Il ne peut plus passer les virages pied au
// plancher (la route ne porte plus la voiture), et sa direction proportionnelle (« −écart − cap », game.cpp, trafic) oscille
// à grande vitesse. Ici, seulement pour la voiture du joueur en conduite réelle :
//  - vitesse permise : 0,6 g d'accélération latérale au plus dans le rayon de chaque virage des 600 m qui viennent, freinage
//    de 0,7 g pour l'atteindre avant le virage ;
//  - direction : la file voulue est relue dans la commande reçue (évitement du trafic : « (file − écart) × 5 − cap × 4 »),
//    puis visée 0,8 s devant (poursuite pure, comme le joueur simulé) ; sans trafic, la voiture va au centre.
inline double vitessePilote(const Track& t, double dist) { return vitesseVirageAide(t, dist, 0.6, 0.7); }   // m/s (player.h, WFS-258)
inline void limiterPilote(const Player& p, Track& t, DriveInput& in) {
    if (!p.modeleMonde()) return;
    const double v = p.speed * M, vp = vitessePilote(t, p.dist);
    if (v > vp) { in.gas = false; in.nitro = false; in.brake = v > vp * 1.03; }
    if (std::abs(p.heading) > 1.2) return;   // demi-tour, contresens : commande reçue gardée
    // file voulue, ramenée à la file du trafic la plus proche (−2/3, 0, 2/3 : LANE_X) : elle ne suit pas la voiture qui élargit
    const double brute = p.playerX + (in.steer + 4 * p.heading) / 5;
    const double file = std::abs(brute) < 1.0 / 3 ? 0.0 : (brute > 0 ? 2.0 / 3 : -2.0 / 3);   // demi-largeurs
    const double vv = std::max(std::abs(v), 1.0), ld = std::max(8.0, 0.8 * vv);
    const TrackPoint P = t.point(p.dist, p.playerX * RW), T = t.point(p.dist + ld / M, file * RW);
    const double psi = P.th + p.heading, dx = T.x - P.x, dz = T.z - P.z;
    const double av = dx * std::sin(psi) - dz * std::cos(psi), dr = dx * std::cos(psi) + dz * std::sin(psi);
    in.steer = (float)p.braquagePour(2 * dr / std::max(1.0, av * av + dr * dr), vv);
}

// banc d'essai : rend le nombre d'écarts (sorties de route, sauts de cap > 0,05 rad par image)
int benchAideDirection(Game& g, Renderer& r, const std::string& out);

} // namespace wfs
