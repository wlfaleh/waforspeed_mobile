// WaForSpeed — conduite réaliste de la voiture du joueur, en coordonnées du monde (WFS-241, étape 1 du chantier
// « Conduite réaliste », skill expert-conduite). Remplace, pour la voiture du joueur, la conduite d'arcade héritée du jeu web
// (voiture calculée par rapport à l'axe de la route, « la route tourne sous la voiture ») :
//  - position (x, z) et cap réels dans le monde, vitesses longitudinale / latérale, vitesse de lacet ;
//  - modèle « bicyclette » à deux essieux : transfert de charge longitudinal et latéral (approché), pneus en « formule
//    magique » simplifiée (Pacejka réduite) avec limite μ × charge, sensibilité à la charge et cercle d'adhérence ;
//  - moteur (courbe de puissance de Leyland : P(x) = Pmax · (x + x² − x³), x = régime / régime de puissance maxi), boîte
//    automatique (rapports en progression géométrique), traînée aérodynamique, résistance au roulement, frein moteur, pente ;
//  - freins répartis 65 / 35 avec ABS (force de chaque essieu bornée par son adhérence : pas de blocage) et antipatinage ;
//  - direction : angle des roues réel (rayon de braquage), réduit avec la vitesse (assistance variable) ;
//  - en dessous de 2 m/s, modèle cinématique (pas de glisse) : manœuvres et marche arrière exactes, pas d'instabilité.
// Tout est en unités SI (m, s, kg, N, rad), pas fixe (1/120 s), calcul en double, sans hasard : même entrée, même trajectoire
// (duel en ligne). La route ne sert qu'au sol (adhérence, pente) : c'est Player (player.cpp) qui projette la position sur
// la route (distance parcourue, écart latéral, cap relatif) pour tout le reste du jeu (rendu, caméra, trafic, course).
// Valeurs : la fiche de chaque voiture (cars.h : puissance, masse, 0-100, vitesse maximale, empattement, dimensions) fait foi ;
// les valeurs usuelles sans fiche (répartition des masses, transmission, centre de gravité, pneus) sont notées « à confirmer ».
// En-tête seul : lié partout où player.cpp l'est (tests CPU compris), sans changer CMakeLists.txt.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include "cars.h"

namespace wfs::physique {

constexpr double G = 9.81;            // m/s²
constexpr double RHO_AIR = 1.2;       // kg/m³ (air à 20 °C)
constexpr double CRR_ROUTE = 0.012;   // résistance au roulement, pneu sur enrobé (valeur usuelle 0,010 à 0,015)
constexpr double CRR_HERBE = 0.15;    // bas-côté (herbe, terre : 0,1 à 0,3, à confirmer)
constexpr double MU_HERBE = 0.6;      // adhérence du bas-côté (herbe sèche ≈ 0,5 à 0,7, à confirmer)
constexpr double V_CINEMATIQUE = 2.0, V_DYNAMIQUE = 5.0;   // m/s : passage progressif du modèle cinématique au modèle à pneus

enum class Transmission : uint8_t { Avant, Arriere, Integrale };

// paramètres physiques d'une voiture (SI), dérivés de la fiche puis étalonnés sur le 0-100 et la vitesse maximale
struct Params {
    double masse = 1500, L = 2.6, a = 1.1, b = 1.5;   // kg ; empattement ; centre de gravité → essieu avant / arrière (m)
    double h = 0.5, voie = 1.55, Iz = 2500;           // hauteur du centre de gravité (m), voie (m), inertie de lacet (kg·m²)
    double mu = 1.0, muLong = 1.0;                    // adhérence latérale / longitudinale sur route sèche
    double muTraction = 1.0;                          // étalonnage : adhérence au départ (× muLong) des voitures limitées par l'adhérence
    double cyAvant = 17, cyArriere = 19;              // rigidité de dérive normalisée (N/rad par N de charge ; arrière plus raide : sous-vireur stable)
    double pmax = 150e3, wP = 680, wMax = 760;        // W ; régimes de puissance maxi et de coupure (rad/s)
    int rapports = 6; double G[8] = {};               // rapports de démultiplication totaux (boîte × pont), 1er = G[0]
    double rayonRoue = 0.33, CdA = 0.7, kPuissance = 1.0;   // m ; m² ; facteur d'étalonnage sur le 0-100 (rendement compris)
    Transmission transmission = Transmission::Avant; double partAvant = 1;   // intégrale : part du couple à l'avant
    double braquageMax = 0.52;                        // rad : angle des roues à fond, à l'arrêt (rayon de braquage)
    double partFreinAvant = 0.65;
    double cent = 0;                                  // étalonnage : 0-100 obtenu (s) ; vitesse maximale : vitesseMaxi() (banc)
};

// état de la voiture dans le monde. Repère du jeu (Track::pose) : avant = (sin cap, −cos cap), droite = (cos cap, sin cap) ;
// un cap qui augmente = la voiture tourne à droite ; braquage positif = à droite.
struct Etat {
    double x = 0, z = 0, cap = 0;      // m, m, rad
    double vx = 0, vz = 0;             // vitesse dans le monde (m/s)
    double r = 0;                      // vitesse de lacet (rad/s, + = à droite)
    double ax = 0, ay = 0;             // accélérations longitudinale / latérale lissées (m/s², transfert de charge)
    int rapport = 1; double regime = 0;   // rapport engagé (1..n, −1 marche arrière), régime moteur (tr/min)
    // WFS-253 : aides au dernier pas (mesures du banc, voyant éventuel) : glissement des roues avant / arrière (0 = roule,
    // 1 = bloquée au freinage ; > 0 aussi en patinage), ABS, antipatinage et ESP en action
    double glissementAv = 0, glissementAr = 0;
    bool absActif = false, tcActif = false, espActif = false;
    double sigmaAv = 0, sigmaAr = 0;   // WFS-255 (accroche) : glissement total normalisé de chaque essieu (> 1 : au-delà du pic, le pneu crisse)
    double u() const { return vx * std::sin(cap) - vz * std::cos(cap); }   // vitesse longitudinale (m/s, < 0 en arrière)
    double v() const { return vx * std::cos(cap) + vz * std::sin(cap); }   // vitesse latérale (m/s, + = vers la droite)
};

struct Commande {
    double braquage = 0;    // angle des roues avant (rad, + = à droite)
    double gaz = 0;         // 0..1
    double frein = 0;       // 0..1 (ABS)
    bool recul = false;     // marche arrière engagée (gaz = recul)
    double nitro = 1;       // multiplicateur de poussée (1 = sans nitro)
    // WFS-253 : aides de la voiture, toujours activées dans le jeu (comme sur les 20 voitures réelles) ; désactivables
    // seulement au banc pour les mesurer (aucun réglage nouveau pour le joueur sans l'accord du PO)
    bool abs = true, antipatinage = true, esp = true;
};

struct Sol { double mu = 1, crr = CRR_ROUTE, pente = 0; };   // pente (rad) dans le sens de marche de la voiture

void pas(const Params& p, Etat& e, const Commande& c, const Sol& s, double dt);

namespace interne {
constexpr double PI_D = 3.14159265358979323846;
constexpr double C_PNEU = 1.35, E_PNEU = -1.0;   // formule magique latérale (forme) : pic vers 7° de dérive, 85 % du pic en glisse
constexpr double SENSIBILITE = 0.10;             // perte d'adhérence avec la charge (μ baisse de 10 % à 2 × la charge nominale)
constexpr double PART_ROULIS_AVANT = 0.55;       // part du transfert de charge latéral prise par l'essieu avant (barres antiroulis)
constexpr double V_RECUL_MAX = 31 / 3.6;         // m/s : marche arrière (31 km/h au plus, comme REV_MAX du jeu)

inline double sgn(double v) { return v > 0 ? 1.0 : (v < 0 ? -1.0 : 0.0); }

// WFS-253 : pneus. Glissement longitudinal au pic : Pacejka, courbe « asphalte sec » (B 10, C 1,9, D 1, E 0,97 : pic à 18 %,
// roue bloquée à 91 % du pic). ABS et antipatinage tiennent le glissement à 15 %, dans la bande de 10 à 20 % des ABS
// (Bosch, « Automotive Handbook » : régulation autour du pic de la courbe adhérence / glissement).
constexpr double K_PIC = 0.18, K_ABS = 0.15;
// dérive au pic de la forme latérale : x = B·α tel que x − E(x − atan x) = tan(π / 2C) (formule magique), calculé une fois
inline const double X_PIC = [] {
    const double cible = std::tan(3.14159265358979323846 / (2 * C_PNEU));
    double lo = 0, hi = 20;
    for (int i = 0; i < 60; i++) { const double x = (lo + hi) / 2; ((x - E_PNEU * (x - std::atan(x))) < cible ? lo : hi) = x; }
    return hi;
}();
// forme de la formule magique, normalisée (σ = 1 au pic : forme = 1 ; roue bloquée σ ≈ 5,6 : 0,89)
inline double formeNormalisee(double sg) {
    const double x = sg * X_PIC, phi = x - E_PNEU * (x - std::atan(x));
    return std::sin(C_PNEU * std::atan(phi));
}

// valeurs usuelles par modèle (à confirmer, references/reel/SOURCES.md) : part du poids sur l'avant, transmission
struct Usuel { const char* id; double avant; Transmission tr; double partAvant; };
inline constexpr Usuel USUELS[] = {
    {"golf", 0.61, Transmission::Avant, 1}, {"m4", 0.52, Transmission::Arriere, 0}, {"p911", 0.39, Transmission::Integrale, 0.3},
    {"gallardo", 0.43, Transmission::Integrale, 0.3}, {"p208", 0.62, Transmission::Avant, 1}, {"abarth", 0.64, Transmission::Avant, 1},
    {"tcross", 0.61, Transmission::Avant, 1}, {"compass", 0.58, Transmission::Integrale, 0.5}, {"s3", 0.59, Transmission::Integrale, 0.5},
    {"x5", 0.50, Transmission::Integrale, 0.4}, {"giulia", 0.50, Transmission::Arriere, 0}, {"a45", 0.59, Transmission::Integrale, 0.5},
    {"model3", 0.48, Transmission::Integrale, 0.4}, {"corsa", 0.62, Transmission::Avant, 1}, {"gt86", 0.53, Transmission::Arriere, 0},
    {"elantra", 0.61, Transmission::Avant, 1}, {"v60", 0.58, Transmission::Integrale, 0.5}, {"rrsport", 0.50, Transmission::Integrale, 0.5},
    {"mustang", 0.57, Transmission::Arriere, 0}, {"sf90", 0.45, Transmission::Integrale, 0.3},
};

// capacité d'adhérence d'un essieu (N) : deux roues, charge Fz (N) dont dT passe d'une roue à l'autre, sensibilité à la charge
inline double capacite(double mu, double Fz, double dT, double Fz0) {
    const double g = std::max(0.0, Fz / 2 + dT), d = std::max(0.0, Fz / 2 - dT);
    return mu * std::max(0.0, g * (1 - SENSIBILITE * g / Fz0) + d * (1 - SENSIBILITE * d / Fz0));
}

// 0-100 km/h (s) en plein gaz sur le plat, même pas que la partie (1/120 s)
inline double tempsCent(const Params& p) {
    Etat e; Commande c; c.gaz = 1; Sol s;
    double t = 0;
    while (e.u() < 100 / 3.6 && t < 40) { pas(p, e, c, s, 1.0 / 120); t += 1.0 / 120; }
    return t;
}

// traînée calée pour que la puissance aux roues équilibre traînée et roulement à la vitesse maximale de la fiche
inline void calerCdA(Params& p, double vmax) {
    p.CdA = std::max(0.2, 2 * (p.kPuissance * p.pmax / vmax - CRR_ROUTE * p.masse * G) / (RHO_AIR * vmax * vmax));
}
} // namespace interne

// angle des roues maximal à la vitesse v (m/s) : à l'arrêt, braquage de la fiche ; plus vite, la limite d'adhérence
// (L · μ · g / v²) plus la dérive des pneus avant au maximum d'accroche : le pouce à fond = la voiture à la limite
inline double braquageMax(const Params& p, double v) {
    return std::min(p.braquageMax, p.L * p.mu * G / std::max(v * v, 1e-6) + 0.10);
}

inline void pas(const Params& p, Etat& e, const Commande& c, const Sol& s, double dt) {
    using namespace interne;
    const double m = p.masse, sn = std::sin(e.cap), cs = std::cos(e.cap);
    const double u = e.vx * sn - e.vz * cs, vl = e.vx * cs + e.vz * sn, au = std::abs(u);
    const double d = std::clamp(c.braquage, -p.braquageMax, p.braquageMax), cd = std::cos(d), sd = std::sin(d);

    // ---------- charges des essieux : transfert longitudinal (freinage, accélération) et latéral (virage) ----------
    const double mg = m * G, Fz0 = mg / 4;
    const double Fzf = std::clamp(mg * p.b / p.L - m * e.ax * p.h / p.L, 0.05 * mg, 0.95 * mg), Fzr = mg - Fzf;
    const double dT = m * e.ay * p.h / p.voie;
    const double Df = capacite(s.mu * p.mu, Fzf, dT * PART_ROULIS_AVANT, Fz0), Dr = capacite(s.mu * p.mu, Fzr, dT * (1 - PART_ROULIS_AVANT), Fz0);
    const double kL = p.muLong / p.mu, Dlf = Df * kL, Dlr = Dr * kL;

    // ---------- moteur et boîte automatique ----------
    double Fdrive = 0, Fmoteur = 0;   // poussée aux roues motrices (N, + = vers l'avant) ; frein moteur (N, s'oppose à u)
    const double wRoue = u / p.rayonRoue, Tp = p.pmax / p.wP;
    auto couple = [&](double we) { const double x = we / p.wP; return we >= p.wMax * 1.02 ? 0.0 : Tp * std::max(0.0, 1 + x - x * x); };
    if (c.recul) {
        e.rapport = -1;
        const double we = std::max(std::abs(wRoue) * p.G[0], 0.5 * p.wP);
        e.regime = we * 60 / (2 * PI_D);
        if (u > -V_RECUL_MAX) Fdrive = -c.gaz * p.kPuissance * couple(we) * p.G[0] / p.rayonRoue;
    } else {
        if (e.rapport < 1) e.rapport = 1;
        int& g = e.rapport;
        // boîte automatique : passe le rapport qui pousse le plus fort (gaz) ; sans gaz, rétrograde sous 45 % du régime de puissance
        auto force = [&](int r) { return couple(std::max(0.0, wRoue) * p.G[r - 1]) * p.G[r - 1]; };
        if (g < p.rapports && (wRoue * p.G[g - 1] >= p.wMax || (c.gaz > 0 && force(g + 1) >= force(g)))) g++;
        while (g > 1 && wRoue * p.G[g - 2] < 0.95 * p.wMax &&
               (c.gaz > 0 ? force(g - 1) > 1.15 * force(g) : wRoue * p.G[g - 1] < 0.45 * p.wP)) g--;
        const double wb = std::max(0.0, wRoue) * p.G[g - 1];
        const double we = g == 1 && c.gaz > 0 ? std::max(wb, 0.5 * p.wP) : std::max(wb, 90.0);   // embrayage qui patine au départ ; ralenti
        e.regime = we * 60 / (2 * PI_D);
        Fdrive = c.gaz * p.kPuissance * c.nitro * couple(we) * p.G[g - 1] / p.rayonRoue;
        if (c.gaz <= 0 && u > 1) Fmoteur = 0.10 * Tp * p.G[g - 1] / p.rayonRoue * std::min(1.0, wb / p.wP);
    }

    // ---------- WFS-253 : pneus en glissement combiné, ABS, antipatinage, ESP ----------
    // Chaque essieu a un glissement longitudinal κ (0 = la roue roule, 1 = bloquée) et une dérive α. Méthode du glissement
    // normalisé (Pacejka, « Tyre and Vehicle Dynamics », 3e éd. 2012, § 4.3) : σx = κ / κ_PIC, σy = tan α / tan α_pic,
    // σ = √(σx² + σy²) ; la force totale vaut D × forme(σ) (forme = 1 au pic, σ = 1) et se répartit selon σx / σ et σy / σ :
    // ce qui sert à freiner ou à pousser ne sert plus à tourner, et une roue bloquée (σx ≈ 5,6) ne dirige presque plus.
    const double vfl = u * cd + (vl + p.a * e.r) * sd, vft = -u * sd + (vl + p.a * e.r) * cd;   // vitesse de la roue avant, repère de la roue
    const double af = -std::atan2(vft, std::max(std::abs(vfl), 0.5)), ar = -std::atan2(vl - p.b * e.r, std::max(au, 0.5));
    const double part = p.transmission == Transmission::Avant ? 1.0 : p.transmission == Transmission::Arriere ? 0.0 : p.partAvant;
    double Fbf = 0, Fbr = 0;   // demande de freinage (N, ≥ 0)
    if (c.frein > 0 && au > 0.01) { const double Fb = c.frein * 1.3 * mg; Fbf = Fb * p.partFreinAvant; Fbr = Fb * (1 - p.partFreinAvant); }
    struct Essieu { double Fx = 0, Fy = 0, k = 0, sg = 0; bool abs = false, tc = false; };
    auto essieu = [&](double moteur, double frein, double alpha, double cy, double Fz, double D, double Dl) {
        Essieu r;
        if (D <= 1) return r;
        const double tp = std::tan(X_PIC * C_PNEU * D / std::max(cy * Fz, 1.0)), sy = std::tan(alpha) / std::max(tp, 1e-4);
        // force (N) au glissement k (≥ 0), dans le sens de la demande ; mt : adhérence au départ étalonnée (poussée seulement)
        auto forces = [&](double k, double mt, double& fy) {
            const double sx = k / K_PIC, sg = std::hypot(sx, sy);
            if (sg < 1e-9) { fy = 0; return 0.0; }
            const double F = formeNormalisee(sg);
            fy = D * F * sy / sg;
            return Dl * mt * F * sx / sg;
        };
        const double sensU = u >= 0 ? 1.0 : -1.0;
        const double dem = frein > 0 ? frein : std::abs(moteur), sensF = frein > 0 ? -sensU : (moteur >= 0 ? 1.0 : -1.0);
        const bool freine = frein > 0;
        const double mt = freine ? 1.0 : p.muTraction;
        double kmax;
        if (freine) kmax = c.abs ? K_ABS : 1.0;   // ABS : glissement tenu à 15 % (bande de 10 à 20 %) ; sans ABS : la roue se bloque
        else {
            kmax = c.antipatinage ? K_ABS : 1.0;   // antipatinage : même limite de glissement ; sans : les roues motrices patinent
            // ESP, action sur le moteur (Bosch) : en virage, la poussée ne dépasse pas le pic du pneu (σ ≤ 1) : pas de sous-virage de puissance
            if (c.esp && std::abs(sy) < 1) kmax = std::min(kmax, K_PIC * std::sqrt(1 - sy * sy));
        }
        double fy = 0, fx = dem > 0 ? forces(kmax, mt, fy) : 0;
        if (dem <= 0) { forces(0, mt, fy); r.Fy = fy; r.sg = std::abs(sy); return r; }
        if (fx <= dem) {   // demande au-delà de ce que le pneu donne à kmax : ABS ou antipatinage en action, ou roue bloquée / qui patine
            r.k = kmax; r.abs = freine && c.abs; r.tc = !freine && c.antipatinage;
            if (!freine && !c.antipatinage) {   // sans antipatinage : la roue patine d'autant plus que le couple dépasse l'adhérence
                double f0 = 0; const double pic = forces(K_PIC, mt, f0);
                r.k = std::clamp(K_PIC * dem / std::max(pic, 1.0), K_PIC, 1.0); fx = forces(r.k, mt, fy);
            }
        } else {           // glissement juste suffisant pour la demande (dichotomie : la force croît avec k jusqu'au pic)
            double lo = 0, hi = kmax;
            for (int it = 0; it < 18; it++) { const double mid = (lo + hi) / 2; double f2; (forces(mid, mt, f2) < dem ? lo : hi) = mid; }
            r.k = hi; fx = forces(hi, mt, fy);
        }
        r.Fx = sensF * fx; r.Fy = fy; r.sg = std::hypot(r.k / K_PIC, sy);
        return r;
    };
    const double moteurTotal = Fdrive - Fmoteur * sgn(u);
    const Essieu ea = essieu(moteurTotal * part, Fbf, af, p.cyAvant, Fzf, Df, Dlf), eb = essieu(moteurTotal * (1 - part), Fbr, ar, p.cyArriere, Fzr, Dr, Dlr);
    const double Fxf = ea.Fx, Fxr = eb.Fx, Fyf = ea.Fy, Fyr = eb.Fy;
    e.glissementAv = ea.k; e.glissementAr = eb.k; e.sigmaAv = ea.sg; e.sigmaAr = eb.sg; e.absActif = ea.abs || eb.abs; e.tcActif = ea.tc || eb.tc; e.espActif = false;

    // ---------- somme des forces (repère de la voiture) ----------
    double Fx_esp = 0;   // freinage de l'ESP (N)
    double Fx = Fxf * cd - Fyf * sd + Fxr;
    const double Fy = Fxf * sd + Fyf * cd + Fyr;
    double Mz = p.a * (Fxf * sd + Fyf * cd) - p.b * Fyr;
    // ESP, action sur les freins (survirage) : lacet visé (Rajamani, « Vehicle Dynamics and Control », 2e éd. 2012, § 8.4) =
    // celui du modèle bicyclette en régime établi, u·δ / (L + K_us·u²), borné par μ·g / u ; dérive bornée par atan(0,02 μ g)
    // (≈ 11° sur route sèche). Au-delà, le freinage d'une roue avant extérieure crée le couple qui ramène le lacet (correcteur
    // du premier ordre, constante de temps 0,1 s : réglage, pas un seuil), borné par l'adhérence de cette roue.
    if (c.esp && au > V_DYNAMIQUE) {
        const double mu = s.mu * p.mu, Caf = p.cyAvant * mg * p.b / p.L, Car = p.cyArriere * mg * p.a / p.L;
        const double Kus = m / p.L * (p.b / Caf - p.a / Car);
        double rT = std::clamp(u * d / (p.L + Kus * u * u), -mu * G / au, mu * G / au);
        const double beta = std::atan2(vl, au), betaMax = std::atan(0.02 * mu * G);
        if (std::abs(beta) > betaMax) rT *= std::max(0.0, 1 - (std::abs(beta) - betaMax) / betaMax);
        const bool survire = (e.r * rT >= 0 && std::abs(e.r) > std::abs(rT)) || (e.r * rT < 0);
        if (survire && std::abs(e.r - rT) > 1e-3) {
            const double Mvoulu = -p.Iz * (e.r - rT) / 0.1, Mmax = 0.5 * Dlf * p.voie / 2;
            const double Mesp = std::clamp(Mvoulu, -Mmax, Mmax);
            Mz += Mesp; Fx_esp = std::abs(Mesp) / (p.voie / 2) * sgn(u);
            e.espActif = std::abs(Mesp) > 0.02 * Mmax;
        }
    }
    const double resist = 0.5 * RHO_AIR * p.CdA * u * au + (au > 0.01 ? s.crr * mg * sgn(u) : 0);
    Fx -= resist + mg * std::sin(s.pente) + Fx_esp;

    // ---------- intégration (semi-implicite) ----------
    const double fx = sn, fz = -cs, rx = cs, rz = sn;   // avant, droite
    e.vx += (Fx * fx + Fy * rx) / m * dt; e.vz += (Fx * fz + Fy * rz) / m * dt;
    e.r += Mz / p.Iz * dt;
    double u1 = e.vx * fx + e.vz * fz;
    // arrêt : freins, frottements et frein moteur ne font jamais repartir la voiture en sens inverse
    const bool pousseAvant = Fdrive > 0, pousseArriere = Fdrive < 0;
    if ((u > 0 && u1 < 0 && !pousseArriere) || (u < 0 && u1 > 0 && !pousseAvant) || (au < 0.01 && !pousseAvant && !pousseArriere && std::abs(u1) < 0.05)) {
        e.vx -= u1 * fx; e.vz -= u1 * fz; u1 = 0;
    }
    // basse vitesse : modèle cinématique (roues arrière sans dérive), passage progressif vers le modèle à pneus
    const double wk = std::clamp((std::abs(u1) - V_CINEMATIQUE) / (V_DYNAMIQUE - V_CINEMATIQUE), 0.0, 1.0);
    const double rk = u1 * std::tan(d) / p.L;
    e.r = wk * e.r + (1 - wk) * rk;
    e.cap += e.r * dt;
    if (e.cap > PI_D * 64 || e.cap < -PI_D * 64) e.cap = std::remainder(e.cap, 2 * PI_D);
    const double sn2 = std::sin(e.cap), cs2 = std::cos(e.cap);
    double u2 = e.vx * sn2 - e.vz * cs2, v2 = e.vx * cs2 + e.vz * sn2;
    if (wk < 1) {   // la vitesse suit l'axe de la voiture (norme gardée : pas de perte de vitesse dans un virage lent)
        const double vk = p.b * e.r, n2 = e.vx * e.vx + e.vz * e.vz;
        const double uk = sgn(u2) * std::sqrt(std::max(0.0, n2 - vk * vk));
        u2 = wk * u2 + (1 - wk) * uk; v2 = wk * v2 + (1 - wk) * vk;
        e.vx = u2 * sn2 + v2 * cs2; e.vz = -u2 * cs2 + v2 * sn2;
    }
    e.x += e.vx * dt; e.z += e.vz * dt;
    const double k = std::min(1.0, dt * 20);   // accélérations lissées (20 /s) : le transfert de charge suit sans boucle algébrique
    e.ax += (Fx / m - e.ax) * k; e.ay += (Fy / m - e.ay) * k;
}

// paramètres d'une voiture tirés de sa fiche, étalonnés : vitesse maximale exacte (traînée), 0-100 de la fiche (puissance
// aux roues, et adhérence au départ pour les voitures que l'adhérence limite)
inline Params etalonner(const CarDef& c) {
    using namespace interne;
    Params p;
    const Usuel* us = nullptr;
    for (const Usuel& x : USUELS) if (std::strcmp(x.id, c.id) == 0) us = &x;
    const double avant = us ? us->avant : 0.58;
    p.transmission = us ? us->tr : Transmission::Avant; p.partAvant = us ? us->partAvant : 1;
    p.masse = c.spec.kg + 75;   // fiche (à vide) + conducteur
    p.L = c.d.wb; p.a = p.L * (1 - avant); p.b = p.L * avant;
    p.h = 0.37 * c.d.H;         // centre de gravité ≈ 37 % de la hauteur (à confirmer) : 0,54 m pour la Golf, 0,67 m pour le Range Rover
    p.voie = 0.86 * c.d.W;
    p.Iz = p.masse * p.a * p.b; // inertie de lacet ≈ m · a · b (approximation usuelle)
    p.mu = 0.88 + 0.022 * c.grip; p.muLong = p.mu * 1.08;   // grip 5 (SUV) → 0,99 ; grip 10 (sportive) → 1,10 (à confirmer)
    p.pmax = c.spec.ch * 735.5;
    p.wP = 6500 * 2 * PI_D / 60; p.wMax = 1.12 * p.wP;     // puissance maxi à 6 500 tr/min, coupure à 7 280 (à confirmer)
    p.rayonRoue = c.d.rw > 0.2 ? c.d.rw : 0.33;
    const double vmax = c.spec.vmax / 3.6;
    p.rapports = c.spec.vmax >= 280 ? 7 : c.spec.ch < 160 ? 5 : 6;
    const double gHaut = p.wP * p.rayonRoue / vmax, v1 = std::max(50 / 3.6, 0.24 * vmax), gBas = p.wMax * p.rayonRoue / v1;
    for (int i = 0; i < p.rapports; i++) p.G[i] = gBas * std::pow(gHaut / gBas, (double)i / (p.rapports - 1));
    // rayon de braquage : diamètre entre trottoirs ≈ 4,15 × empattement (Golf 10,9 m, 500 9,5 m, Range Rover 12,5 m)
    const double Rc = 4.15 * p.L / 2, Rr = std::sqrt(std::max(1.0, Rc * Rc - p.L * p.L)) - p.voie / 2;
    p.braquageMax = std::atan(p.L / std::max(Rr, 2.0));
    double lo = 0.4, hi = 1.3;
    p.kPuissance = hi; calerCdA(p, vmax);
    if (tempsCent(p) > c.spec.s100) {   // même à 1,3 × la puissance, trop lent : l'adhérence au départ limite (propulsion puissante)
        double ml = 1.0, mh = 1.8;
        for (int it = 0; it < 24; it++) { p.muTraction = (ml + mh) / 2; (tempsCent(p) > c.spec.s100 ? ml : mh) = p.muTraction; }
        p.muTraction = mh;
    }
    for (int it = 0; it < 28; it++) {
        p.kPuissance = (lo + hi) / 2; calerCdA(p, vmax);
        (tempsCent(p) > c.spec.s100 ? lo : hi) = p.kPuissance;
    }
    p.kPuissance = hi; calerCdA(p, vmax);
    p.cent = tempsCent(p);
    return p;
}

// vitesse maximale obtenue (m/s) : 3 min en plein gaz (banc seulement : trop long pour le chargement d'une partie)
inline double vitesseMaxi(const Params& p) {
    Etat e; Commande cm; cm.gaz = 1; Sol s;
    for (int i = 0; i < 120 * 180; i++) pas(p, e, cm, s, 1.0 / 120);
    return e.u();
}

// étalonnage d'une voiture du catalogue à sa première utilisation seulement (WFS-253 : ≈ 0,1 s par voiture sur PC ; les 20
// d'un coup prendraient plusieurs secondes au téléphone), sûr entre threads ; autre fiche : calculée à chaque appel
inline const Params& params(const CarDef& c) {
    constexpr size_t N = sizeof(CARS) / sizeof(CARS[0]);
    static std::array<Params, N> table{};
    static std::array<std::once_flag, N> faits;
    const size_t i = (size_t)(&c - CARS);
    if (&c >= CARS && i < N) { std::call_once(faits[i], [&] { table[i] = etalonner(CARS[i]); }); return table[i]; }
    thread_local Params autre; autre = etalonner(c); return autre;
}

} // namespace wfs::physique
