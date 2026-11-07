// WaForSpeed — chocs de la voiture du joueur avec tout ce qui se touche (WFS-256, ANO-129). En-tête seul (lié partout où
// player.cpp l'est, tests compris).
//  - Registre des obstacles solides du décor : cylindres (poteaux, lampadaires, arbres, feux, piles de portique) et boîtes
//    orientées (bancs, abribus, murs, glissières, bâtiments). Chaque carte les déclare au chargement, par segment de la piste
//    (chocs_cartes.cpp) ; seuls ceux des segments autour de la voiture sont placés dans le monde et testés à chaque pas.
//  - Test continu : la voiture (rectangle orienté, longueur × largeur de la fiche) est testée tous les 10 cm le long du pas
//    de 1/120 s (à 150 km/h : 4 positions par pas) : rien ne se traverse entre deux images (RG-02).
//  - Réaction (RG-03) : impulsion d'un corps rigide au point de contact (coin le plus enfoncé), frottement 0,4 ; restitution
//    qui baisse avec la vitesse d'impact comme sur les vraies voitures (carrosserie qui se déforme : ≈ 0,2 au pas, presque 0
//    au-delà de 50 km/h ; à confirmer, références de reconstitution d'accidents) : la voiture rebondit, tourne ou s'arrête
//    selon la vitesse et l'angle. Le décor dessiné ne bouge pas : tous les obstacles sont fixes (rien de « poussé » qui se
//    séparerait de son image) ; les voitures en sens inverse ont leur vitesse.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace wfs::chocs {

enum class Forme : uint8_t { Cylindre, Boite };
enum class Genre : uint8_t { Poteau, Arbre, Mobilier, Mur, Batiment, Voiture, Ilot, Autre };
inline const char* nomGenre(Genre g) {
    switch (g) { case Genre::Poteau: return "poteau"; case Genre::Arbre: return "arbre"; case Genre::Mobilier: return "mobilier";
                 case Genre::Mur: return "mur"; case Genre::Batiment: return "bâtiment"; case Genre::Voiture: return "voiture";
                 case Genre::Ilot: return "îlot"; default: return "objet"; }
}

struct Obstacle {
    double x = 0, z = 0;           // centre (m, monde)
    double r = 0.15;               // cylindre : rayon (m)
    double hx = 0.5, hz = 0.5;     // boîte : demi-longueurs (m) le long de ses axes
    double angle = 0;              // boîte : axe x de la boîte = (cos angle, sin angle) dans (x, z)
    Forme forme = Forme::Cylindre;
    Genre genre = Genre::Autre;
    double vx = 0, vz = 0;         // vitesse (m/s) de l'obstacle (0 : décor fixe)
};

// obstacle déclaré par la carte, par rapport à la route (comme son décor) : segment de la piste (sur un tour), avance (m) le long
// du segment, latéral (m, + à droite), forme et angle de la boîte par rapport à la direction de la route. La piste ne reboucle
// pas dans le monde (Track::pose) : la position dans le monde est calculée autour de la voiture, à chaque pas, au tour où elle est.
struct ObstacleRoute {
    float along = 0, lat = 0;
    float r = 0.15f, hx = 0.5f, hz = 0.5f, angle = 0;   // angle : 0 = boîte alignée sur la route (hx en travers, hz le long)
    Forme forme = Forme::Cylindre;
    Genre genre = Genre::Autre;
};

class Registre {
public:
    void vider() { parSeg_.clear(); n_ = 0; portee_ = 0; }
    void preparer(int64_t segments) { parSeg_.assign((size_t)std::max<int64_t>(segments, 1), {}); n_ = 0; portee_ = 0; }
    void ajouter(int64_t seg, const ObstacleRoute& o) {
        if (parSeg_.empty()) return;
        const int64_t L = (int64_t)parSeg_.size();
        parSeg_[(size_t)(((seg % L) + L) % L)].push_back(o); n_++;
        portee_ = std::max(portee_, (double)std::abs(o.along) + (o.forme == Forme::Cylindre ? o.r : std::hypot(o.hx, o.hz)));
    }
    size_t taille() const { return n_; }
    // obstacles à moins de `rayon` m (le long de la route) de la distance d (unités du jeu), en coordonnées du monde
    template <class T, class F> void autour(T& track, double d, double rayon, F&& f) const {
        if (n_ == 0) return;
        const int64_t L = (int64_t)parSeg_.size();
        const double seg = 1.2;   // SEG_M (track.h) : 1,2 m par segment
        const int64_t a0 = (int64_t)std::floor((d * 0.006 - rayon - portee_) / seg), a1 = (int64_t)std::floor((d * 0.006 + rayon + portee_) / seg);
        for (int64_t a = a0; a <= a1; a++)
            for (const ObstacleRoute& o : parSeg_[(size_t)(((a % L) + L) % L)]) {
                const auto P = track.pointLisse(a * 200.0 + o.along / 0.006, o.lat);   // a × SEG + avance
                Obstacle w; w.x = P.x; w.z = P.z; w.r = o.r; w.hx = o.hx; w.hz = o.hz; w.angle = P.th + o.angle; w.forme = o.forme; w.genre = o.genre;
                f(w);
            }
    }
private:
    std::vector<std::vector<ObstacleRoute>> parSeg_;
    size_t n_ = 0;
    double portee_ = 0;   // m : avance + étendue maximales (fenêtre de recherche)
};
inline Registre& registre() { static Registre r; return r; }

// ---------------------------------------------------------------- géométrie
// voiture : rectangle orienté (centre cx, cz ; avant = (sin cap, −cos cap) ; demi-longueur hl, demi-largeur hw)
struct Contact { bool touche = false; double nx = 0, nz = 0, prof = 0, px = 0, pz = 0; };   // normale vers la voiture, profondeur, point

// point d'un rectangle orienté le plus proche de (qx, qz) ; repère local : (av, dr) = avant, droite
inline void plusProche(double cx, double cz, double cap, double hl, double hw, double qx, double qz, double& rx, double& rz, bool& dedans) {
    const double fx = std::sin(cap), fz = -std::cos(cap), dx = std::cos(cap), dz = std::sin(cap);
    const double lx = qx - cx, lz = qz - cz, a = lx * fx + lz * fz, b = lx * dx + lz * dz;
    const double ca = std::clamp(a, -hl, hl), cb = std::clamp(b, -hw, hw);
    dedans = std::abs(a) <= hl && std::abs(b) <= hw;
    rx = cx + ca * fx + cb * dx; rz = cz + ca * fz + cb * dz;
}

// contact voiture (rectangle) ↔ obstacle
inline Contact contact(double cx, double cz, double cap, double hl, double hw, const Obstacle& o) {
    Contact c;
    if (o.forme == Forme::Cylindre) {
        double rx, rz; bool dedans;
        plusProche(cx, cz, cap, hl, hw, o.x, o.z, rx, rz, dedans);
        if (dedans) {   // centre du poteau dans la voiture : sortie par la face la plus proche
            const double fx = std::sin(cap), fz = -std::cos(cap), dx = std::cos(cap), dz = std::sin(cap);
            const double lx = o.x - cx, lz = o.z - cz, a = lx * fx + lz * fz, b = lx * dx + lz * dz;
            const double pa = hl - std::abs(a), pb = hw - std::abs(b);
            const double sa = a >= 0 ? 1 : -1, sb = b >= 0 ? 1 : -1;
            if (pa < pb) { c.nx = -sa * fx; c.nz = -sa * fz; c.prof = pa + o.r; } else { c.nx = -sb * dx; c.nz = -sb * dz; c.prof = pb + o.r; }
            c.touche = true; c.px = o.x; c.pz = o.z; return c;
        }
        const double ex = rx - o.x, ez = rz - o.z, d = std::hypot(ex, ez);
        if (d >= o.r) return c;
        c.touche = true; c.nx = ex / std::max(d, 1e-9); c.nz = ez / std::max(d, 1e-9); c.prof = o.r - d; c.px = rx; c.pz = rz;
        return c;
    }
    // boîte ↔ rectangle : séparation sur les 4 axes (théorème des axes séparateurs), axe de moindre recouvrement
    const double fx = std::sin(cap), fz = -std::cos(cap), dx = std::cos(cap), dz = std::sin(cap);
    const double bx = std::cos(o.angle), bz = std::sin(o.angle), ux = -bz, uz = bx;
    const double tx = o.x - cx, tz = o.z - cz;
    const double axes[4][2] = {{fx, fz}, {dx, dz}, {bx, bz}, {ux, uz}};
    double over[4], sgn4[4];
    for (int i = 0; i < 4; i++) {
        const double* ax = axes[i];
        const double ra = hl * std::abs(fx * ax[0] + fz * ax[1]) + hw * std::abs(dx * ax[0] + dz * ax[1]);
        const double rb = o.hx * std::abs(bx * ax[0] + bz * ax[1]) + o.hz * std::abs(ux * ax[0] + uz * ax[1]);
        const double t = tx * ax[0] + tz * ax[1];
        over[i] = ra + rb - std::abs(t); sgn4[i] = t >= 0 ? -1 : 1;
        if (over[i] <= 0) return c;
    }
    // normale : face de la boîte (mur, bâtiment : la voiture glisse le long), sauf si un coin de la boîte est entré dans la
    // voiture (petit objet : borne, banc pris par le coin) : face de la voiture
    bool coinDedans = false;
    for (int i = 0; i < 4 && !coinDedans; i++) {
        const double sa = (i & 1) ? 1 : -1, sb = (i & 2) ? 1 : -1, qx = o.x + sa * o.hx * bx + sb * o.hz * ux, qz = o.z + sa * o.hx * bz + sb * o.hz * uz;
        double rx, rz; plusProche(cx, cz, cap, hl, hw, qx, qz, rx, rz, coinDedans);
    }
    const int i0 = coinDedans ? 0 : 2, k = over[i0] <= over[i0 + 1] ? i0 : i0 + 1;
    const double best = over[k], nx = sgn4[k] * axes[k][0], nz = sgn4[k] * axes[k][1];
    c.touche = true; c.nx = nx; c.nz = nz; c.prof = best;
    // point de contact : le coin le plus enfoncé : coin de la voiture le plus avancé vers la boîte (−n), ou coin de la boîte le
    // plus avancé dans la voiture (+n) quand la boîte est la plus petite dans cette direction
    double mv = 1e18, mb = -1e18, vxp = cx, vzp = cz, bxp = o.x, bzp = o.z;
    for (int i = 0; i < 4; i++) {
        const double sa = (i & 1) ? 1 : -1, sb = (i & 2) ? 1 : -1;
        const double qx = cx + sa * hl * fx + sb * hw * dx, qz = cz + sa * hl * fz + sb * hw * dz;   // coin de la voiture
        const double kv = qx * nx + qz * nz; if (kv < mv) { mv = kv; vxp = qx; vzp = qz; }
        const double rx = o.x + sa * o.hx * bx + sb * o.hz * ux, rz = o.z + sa * o.hx * bz + sb * o.hz * uz;   // coin de la boîte
        const double kb = rx * nx + rz * nz; if (kb > mb) { mb = kb; bxp = rx; bzp = rz; }
    }
    // le coin de la boîte compte s'il est dans la voiture (petite boîte : banc, borne) ; sinon celui de la voiture (mur, bâtiment)
    bool dedans; double tx2, tz2; plusProche(cx, cz, cap, hl, hw, bxp, bzp, tx2, tz2, dedans);
    if (dedans) { c.px = bxp; c.pz = bzp; } else { c.px = vxp; c.pz = vzp; }
    return c;
}

// ---------------------------------------------------------------- réaction (corps rigide, plan)
struct Reponse { double impact = 0; Genre genre = Genre::Autre; bool choc = false; };
constexpr double FROTTEMENT = 0.4;
inline double restitution(double vImpact) { return 0.25 * std::exp(-vImpact / 6.0); }   // 3 m/s : 0,15 ; 14 m/s (50 km/h) : 0,02

// vitesse (vx, vz), lacet r de la voiture (masse m, inertie Iz) au centre (cx, cz) ; contact c avec l'obstacle o
inline Reponse reagir(double& cx, double& cz, double& vx, double& vz, double& r, double m, double Iz, const Contact& c, const Obstacle& o) {
    Reponse rep; rep.genre = o.genre;
    cx += c.nx * (c.prof + 0.005); cz += c.nz * (c.prof + 0.005);   // la voiture ressort de l'obstacle
    const double rx = c.px - cx, rz = c.pz - cz;                      // bras de levier
    // vitesse du point de contact (ω × r en 2D, lacet + = vers la droite : ω = −r autour de y vers le haut, repère du jeu)
    const double pvx = vx - r * rz, pvz = vz + r * rx;
    const double vn = (pvx - o.vx) * c.nx + (pvz - o.vz) * c.nz;
    if (vn >= 0) return rep;   // on s'éloigne déjà
    const double rn = rx * c.nz - rz * c.nx;   // composante du bras utile au lacet
    const double invM = 1 / m, invI = rn * rn / Iz;
    const double J = -(1 + restitution(-vn)) * vn / (invM + invI);
    vx += J * c.nx / m; vz += J * c.nz / m; r += rn * J / Iz;   // couple = rx·Fz − rz·Fx (même convention que pvx / pvz)
    // frottement tangent (borné par μ × J)
    const double tx = -c.nz, tz = c.nx, vt = (pvx - o.vx) * tx + (pvz - o.vz) * tz;
    const double rt = rx * tz - rz * tx, Jt = std::clamp(-vt / (1 / m + rt * rt / Iz), -FROTTEMENT * J, FROTTEMENT * J);
    vx += Jt * tx / m; vz += Jt * tz / m; r += rt * Jt / Iz;
    rep.impact = -vn; rep.choc = true;
    return rep;
}

} // namespace wfs::chocs
