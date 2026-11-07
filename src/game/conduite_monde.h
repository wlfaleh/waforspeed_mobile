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
    double cent = 0, vmax = 0;                        // étalonnage : 0-100 (s) et vitesse maximale (m/s) obtenus
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

// force latérale d'un essieu (N, repère de la roue) : formule magique, raideur tirée de la charge, pic Dy (cercle d'adhérence)
inline double forceLaterale(double alpha, double cy, double Fz, double D, double Dy) {
    if (D <= 1 || Dy <= 0) return 0;
    const double B = cy * Fz / (C_PNEU * D), ba = B * alpha;
    const double phi = ba - E_PNEU * (ba - std::atan(ba));
    return Dy * std::sin(C_PNEU * std::atan(phi));
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

    // ---------- forces longitudinales par essieu : moteur, freins (ABS), antipatinage ----------
    const double part = p.transmission == Transmission::Avant ? 1.0 : p.transmission == Transmission::Arriere ? 0.0 : p.partAvant;
    double Fxf = (Fdrive - Fmoteur * sgn(u)) * part, Fxr = (Fdrive - Fmoteur * sgn(u)) * (1 - part);
    if (c.frein > 0 && au > 0.01) {
        const double Fb = c.frein * 1.3 * mg;   // demande au-delà de l'adhérence : c'est l'ABS qui borne
        Fxf -= sgn(u) * Fb * p.partFreinAvant; Fxr -= sgn(u) * Fb * (1 - p.partFreinAvant);
    }
    const double mt = c.frein > 0 ? 1.0 : p.muTraction;
    // antipatinage et ESP (activés, comme sur les vraies voitures) : la poussée laisse à chaque essieu la force latérale que
    // le virage lui demande (accélération latérale actuelle) ; l'ABS freine à fond mais garde au moins la moitié de
    // l'adhérence pour tourner (la voiture reste dirigeable en freinant dans le virage)
    const double ayAbs = std::abs(e.ay), needF = m * ayAbs * p.b / p.L, needR = m * ayAbs * p.a / p.L;
    auto borne = [&](double Fx, double D, double Dl, double need) {
        const bool frein = Fx * u < 0;   // force qui ralentit
        const double res = frein ? std::min(need, 0.5 * D) : std::min(need, 0.95 * D), q = res / std::max(D, 1.0);
        const double lim = 0.98 * Dl * mt * std::sqrt(std::max(0.0, 1 - q * q));
        return std::clamp(Fx, -lim, lim);
    };
    Fxf = borne(Fxf, Df, Dlf, needF); Fxr = borne(Fxr, Dr, Dlr, needR);

    // ---------- forces latérales (cercle d'adhérence : ce qui sert à freiner ou accélérer ne sert plus à tourner) ----------
    auto reste = [](double D, double Fx, double Dl) { const double q = std::min(1.0, std::abs(Fx) / std::max(Dl, 1.0)); return D * std::sqrt(1 - q * q); };
    const double Dyf = reste(Df, Fxf, Dlf * mt), Dyr = reste(Dr, Fxr, Dlr * mt);
    const double vfl = u * cd + (vl + p.a * e.r) * sd, vft = -u * sd + (vl + p.a * e.r) * cd;   // vitesse de la roue avant, repère de la roue
    const double af = -std::atan2(vft, std::max(std::abs(vfl), 0.5)), ar = -std::atan2(vl - p.b * e.r, std::max(au, 0.5));
    const double Fyf = forceLaterale(af, p.cyAvant, Fzf, Df, Dyf), Fyr = forceLaterale(ar, p.cyArriere, Fzr, Dr, Dyr);

    // ---------- somme des forces (repère de la voiture) ----------
    double Fx_esp = 0;   // freinage de l'ESP (N)
    double Fx = Fxf * cd - Fyf * sd + Fxr;
    const double Fy = Fxf * sd + Fyf * cd + Fyr;
    double Mz = p.a * (Fxf * sd + Fyf * cd) - p.b * Fyr;
    // ESP (activé, comme sur les vraies voitures) : si la voiture tourne plus vite que ce que le volant et l'adhérence
    // justifient (survirage, tête-à-queue qui commence), le freinage d'une roue avant extérieure crée un couple qui la redresse
    // (approché : couple borné par 30 % du poids freiné sur une roue, à la demi-voie) ; il ralentit un peu la voiture
    if (au > V_DYNAMIQUE) {
        const double rRef = std::clamp(u * std::tan(d) / p.L, -p.mu * G / au, p.mu * G / au);
        const double exces = std::abs(e.r) - std::abs(rRef) - 0.05, beta = std::atan2(vl, au);
        if ((exces > 0 && e.r * rRef >= 0) || (std::abs(e.r) > 0.05 && e.r * rRef < 0) || (std::abs(beta) > 0.10 && beta * e.r < 0)) {
            const double voulu = 10.0 * std::max(exces, std::abs(beta) - 0.06) * p.Iz, Fmax = 0.3 * mg;
            const double Mesp = std::min(std::abs(voulu), Fmax * p.voie / 2);
            Mz -= sgn(e.r) * Mesp;
            Fx_esp = Mesp / (p.voie / 2) * sgn(u);
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
    {   // vitesse maximale obtenue : 3 min en plein gaz
        Etat e; Commande cm; cm.gaz = 1; Sol s;
        for (int i = 0; i < 120 * 180; i++) pas(p, e, cm, s, 1.0 / 120);
        p.vmax = e.u();
    }
    return p;
}

// une fois pour les 20 voitures du catalogue (initialisation statique sûre entre threads) ; autre fiche : calculée
inline const Params& params(const CarDef& c) {
    static const auto table = [] { std::array<Params, sizeof(CARS) / sizeof(CARS[0])> t{}; for (size_t i = 0; i < t.size(); i++) t[i] = etalonner(CARS[i]); return t; }();
    const size_t i = (size_t)(&c - CARS);
    if (&c >= CARS && i < table.size()) return table[i];
    thread_local Params autre; autre = etalonner(c); return autre;
}

} // namespace wfs::physique
