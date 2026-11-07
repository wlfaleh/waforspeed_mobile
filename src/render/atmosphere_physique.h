// WaForSpeed — WFS-207 (épic E13, lot 1 de WFS-152) : soleil et lune calculés, atmosphère physique, brouillard de distance.
// En-tête seul (le moteur et tests/fidelite_lumiere.cpp l'incluent, aucune règle de construction à changer).
//
// RG-01 : astres à leur vraie place pour un lieu et une date fictifs (côte méditerranéenne, été) :
//   - soleil : reel::soleil() (équations du tableur de la NOAA, bench_realisme.h, déjà validé par test_fidelite_soleil) ;
//   - lune : algorithme « basse précision » de l'Astronomical Almanac (J. Meeus, Astronomical Algorithms, 2e éd. 1998,
//     chap. 47 version courte : termes principaux de la longitude, de la latitude et de la parallaxe ; ≈ 0,3°), parallaxe
//     topocentrique, phase (chap. 48 : angle de phase i ≈ 180° − élongation, fraction éclairée k = (1 + cos i) / 2) ;
//   - repère du monde : nord = −Z, est = +X, haut = +Y (azimut compté du nord vers l'est).
// RG-02 : diffusion de Rayleigh et de Mie, absorption de l'ozone, diffusion multiple (S. Hillaire, « A Scalable and
//   Production Ready Sky and Atmosphere Rendering Technique », EGSR 2020 : coefficients du tableau 1, facteur de diffusion
//   multiple Ψms = L₂ / (1 − f_ms), éq. 7 à 10). Calcul sur le CPU, seulement quand l'astre change de place (cache) ;
//   rend la couleur du ciel au zénith et à l'horizon, la transmittance vers le soleil (couleur du soleil au sol) et le
//   coefficient d'extinction au sol (brouillard de distance, RG-03). Canaux R, V, B = 680, 550, 440 nm (convention de
//   Hillaire), pris comme primaires sRGB linéaires.
//   Aérosols : épaisseur optique 0,09 à 550 nm (H 1,2 km), cohérente avec le trouble de Linke 2 du soleil de WFS-151
//   (eclairementSoleilNormal) ; visibilité météorologique ≈ 44 km (Koschmieder, V = 3,912 / β).
#pragma once
#include <algorithm>
#include <cmath>
#include "../core/math.h"
#include "bench_realisme.h"   // reel::soleil (NOAA), reel::jourJulien

namespace wfs::atmosphere {

constexpr double PI_A = 3.14159265358979323846;
inline double rad(double d) { return d * PI_A / 180.0; }
inline double deg(double r) { return r * 180.0 / PI_A; }
inline double norm360(double a) { a = std::fmod(a, 360.0); return a < 0 ? a + 360.0 : a; }

// ---------------------------------------------------------------- lieu et date fictifs (lot 1 : deux moments)
// Côte méditerranéenne (43,55° N, 7,02° E, comme la Corniche), 29 juin 2026 (pleine lune ce jour-là, Meeus chap. 49 :
// vérifié par la fraction éclairée calculée ci-dessous) ; jour : midi solaire (11 h 36 UTC, soleil à 69,7° : ≈ 100 000 lx au sol,
// EV100 ≈ 15, comme l'étalon de WFS-151 ; RG-04) ; nuit : 0 h 30 le 30 juin (22 h 30 UTC). Les autres moments : WFS-208.
struct Lieu { double lat = 43.55, lon = 7.02; int annee = 2026, mois = 6, jour = 29; double heureJourUtc = 11.6, heureNuitUtc = 22.5; };
inline const Lieu& lieu() { static const Lieu l; return l; }

// direction du monde d'un astre (hauteur, azimut en degrés) : nord = −Z, est = +X
inline vec3 directionMonde(double hauteur, double azimut) {
    const double h = rad(hauteur), a = rad(azimut);
    return normalize(vec3{(float)(std::sin(a) * std::cos(h)), (float)std::sin(h), (float)(-std::cos(a) * std::cos(h))});
}

struct Astre { double hauteur = 0, azimut = 0; vec3 dir{0, 1, 0}; double phase = 0, fraction = 1; };   // phase : angle de phase (°), lune

inline Astre soleil(double heureUtc) {
    const Lieu& L = lieu();
    const reel::Soleil s = reel::soleil(L.lat, L.lon, L.annee, L.mois, L.jour, heureUtc);
    Astre a; a.hauteur = s.hauteurApparente; a.azimut = s.azimut; a.dir = directionMonde(a.hauteur, a.azimut);
    return a;
}

// lune : Astronomical Almanac, basse précision (Meeus chap. 47, version courte), coordonnées topocentriques
inline Astre lune(double heureUtc) {
    const Lieu& L = lieu();
    const double jd = reel::jourJulien(L.annee, L.mois, L.jour) + heureUtc / 24.0, T = (jd - 2451545.0) / 36525.0;
    const double lam = 218.32 + 481267.881 * T + 6.29 * std::sin(rad(135.0 + 477198.87 * T)) - 1.27 * std::sin(rad(259.3 - 413335.36 * T))
                     + 0.66 * std::sin(rad(235.7 + 890534.22 * T)) + 0.21 * std::sin(rad(269.9 + 954397.74 * T))
                     - 0.19 * std::sin(rad(357.5 + 35999.05 * T)) - 0.11 * std::sin(rad(186.5 + 966404.03 * T));
    const double bet = 5.13 * std::sin(rad(93.3 + 483202.02 * T)) + 0.28 * std::sin(rad(228.2 + 960400.89 * T))
                     - 0.28 * std::sin(rad(318.3 + 6003.15 * T)) - 0.17 * std::sin(rad(217.6 - 407332.21 * T));
    const double par = 0.9508 + 0.0518 * std::cos(rad(135.0 + 477198.87 * T)) + 0.0095 * std::cos(rad(259.3 - 413335.36 * T))
                     + 0.0078 * std::cos(rad(235.7 + 890534.22 * T)) + 0.0028 * std::cos(rad(269.9 + 954397.74 * T));
    const double eps = 23.439 - 0.0000004 * (jd - 2451545.0);
    // écliptique → équateur
    const double l = rad(lam), b = rad(bet), e = rad(eps);
    const double x = std::cos(b) * std::cos(l), y = std::cos(e) * std::cos(b) * std::sin(l) - std::sin(e) * std::sin(b),
                 z = std::sin(e) * std::cos(b) * std::sin(l) + std::cos(e) * std::sin(b);
    const double ra = std::atan2(y, x), dec = std::asin(z);
    // temps sidéral, angle horaire, hauteur et azimut (géocentriques), puis parallaxe : h' = h − π cos h
    const double gmst = norm360(280.46061837 + 360.98564736629 * (jd - 2451545.0));
    const double ha = rad(gmst + L.lon) - ra, phi = rad(L.lat);
    const double sh = std::sin(phi) * std::sin(dec) + std::cos(phi) * std::cos(dec) * std::cos(ha);
    double h = deg(std::asin(std::clamp(sh, -1.0, 1.0)));
    const double az = norm360(deg(std::atan2(-std::cos(dec) * std::sin(ha), std::sin(dec) * std::cos(phi) - std::cos(dec) * std::cos(ha) * std::sin(phi))));
    h -= par * std::cos(rad(h));
    // phase : élongation ψ (lune – soleil), angle de phase i ≈ 180° − ψ, fraction éclairée (1 + cos i) / 2
    const double jc = T, l0 = norm360(280.46646 + jc * 36000.76983), m = rad(357.52911 + jc * 35999.05029);
    const double lamS = l0 + 1.914602 * std::sin(m) + 0.019993 * std::sin(2 * m);
    const double psi = std::acos(std::clamp(std::cos(b) * std::cos(l - rad(lamS)), -1.0, 1.0));
    Astre a; a.hauteur = h; a.azimut = az; a.dir = directionMonde(h, az);
    a.phase = 180.0 - deg(psi); a.fraction = (1.0 + std::cos(rad(a.phase))) / 2.0;
    return a;
}

// ---------------------------------------------------------------- atmosphère (Hillaire 2020, tableau 1)
constexpr double R_SOL = 6360e3, R_HAUT = 6460e3;                                        // m
constexpr double RAYLEIGH[3] = {5.802e-6, 13.558e-6, 33.1e-6}, H_RAYLEIGH = 8000;        // diffusion (m⁻¹), hauteur d'échelle
constexpr double MIE_DIFF = 6.75e-5, MIE_EXT = 7.5e-5, H_MIE = 1200, G_MIE = 0.8;        // τ(550 nm) = 0,09 (trouble de Linke ≈ 2)
constexpr double OZONE[3] = {0.650e-6, 1.881e-6, 0.085e-6};                              // absorption (m⁻¹), couche 10-40 km

struct Milieu { double diffR, diffM, ext[3]; };
inline Milieu milieu(double alt) {
    const double r = std::exp(-alt / H_RAYLEIGH), m = std::exp(-alt / H_MIE), o = std::max(0.0, 1.0 - std::fabs(alt - 25e3) / 15e3);
    Milieu s; s.diffR = r; s.diffM = MIE_DIFF * m;
    for (int c = 0; c < 3; c++) s.ext[c] = RAYLEIGH[c] * r + MIE_EXT * m + OZONE[c] * o;
    return s;
}
// distance jusqu'au haut de l'atmosphère (rayon partant de l'altitude alt, cosinus mu avec la verticale) ; −1 si le sol
inline double longueur(double alt, double mu, bool& sol) {
    const double r = R_SOL + alt, d = r * r * (mu * mu - 1) + R_SOL * R_SOL;
    sol = mu < 0 && d >= 0;
    if (sol) return -r * mu - std::sqrt(d);
    return -r * mu + std::sqrt(r * r * (mu * mu - 1) + R_HAUT * R_HAUT);
}
inline double altitude(double alt0, double mu, double t) { const double r = R_SOL + alt0; return std::sqrt(r * r + t * t + 2 * r * t * mu) - R_SOL; }
// transmittance de l'altitude alt vers la direction de cosinus mu (0 si le rayon touche le sol)
inline void transmittance(double alt, double mu, double T[3], int n = 24) {
    bool sol; const double L = longueur(alt, mu, sol);
    if (sol) { T[0] = T[1] = T[2] = 0; return; }
    double tau[3] = {0, 0, 0};
    for (int i = 0; i < n; i++) { const Milieu s = milieu(altitude(alt, mu, (i + 0.5) * L / n)); for (int c = 0; c < 3; c++) tau[c] += s.ext[c] * L / n; }
    for (int c = 0; c < 3; c++) T[c] = std::exp(-tau[c]);
}
inline double phaseRayleigh(double c) { return 3.0 / (16.0 * PI_A) * (1 + c * c); }
inline double phaseMie(double c) { const double g = G_MIE, k = 1 + g * g - 2 * g * c; return 3.0 / (8.0 * PI_A) * (1 - g * g) * (1 + c * c) / ((2 + g * g) * std::pow(k, 1.5)); }

struct Ciel {
    double zenith[3] = {0, 0, 0}, horizon[3] = {0, 0, 0};   // luminance (cd/m²) par lux d'éclairement normal du soleil, sRGB linéaire
    double soleil[3] = {1, 1, 1};                           // transmittance vers le soleil (couleur du soleil au sol)
    double ext[3] = {0, 0, 0}, extY = 0;                    // extinction au sol (m⁻¹) par canal et pondérée en luminance
    double psiMs[3] = {0, 0, 0};                            // diffusion multiple (Hillaire, éq. 10) au sol, par lux
};
inline double lumY(const double c[3]) { return 0.2126729 * c[0] + 0.7151522 * c[1] + 0.0721750 * c[2]; }

// luminance du ciel vue du sol (1,5 m) dans la direction d, soleil s ; psi : diffusion multiple isotrope
inline void luminance(const vec3& d, const vec3& s, const double psi[3], double out[3], int n = 32) {
    const double alt0 = 1.5; bool sol;
    const double L = longueur(alt0, d.y, sol), mus = s.y, nu = dot(d, s), pR = phaseRayleigh(nu), pM = phaseMie(nu);
    double tau[3] = {0, 0, 0}; out[0] = out[1] = out[2] = 0;
    for (int i = 0; i < n; i++) {
        const double t = (i + 0.5) * L / n, dt = L / n, alt = altitude(alt0, d.y, t);
        const Milieu m = milieu(alt);
        double Ts[3]; transmittance(alt, (mus * (R_SOL + alt0) + t * nu) / (R_SOL + alt), Ts, 12);   // vers le soleil depuis le point
        for (int c = 0; c < 3; c++) {
            const double Tv = std::exp(-(tau[c] + m.ext[c] * dt * 0.5));
            const double S = RAYLEIGH[c] * m.diffR * (pR * Ts[c] + psi[c]) + m.diffM * (pM * Ts[c] + psi[c]);
            out[c] += Tv * S * dt;
            tau[c] += m.ext[c] * dt;
        }
    }
}

// Ψms au sol (Hillaire 2020, éq. 7 à 10) : 64 directions (spirale de Fibonacci), diffusion d'ordre 2 isotrope
inline void diffusionMultiple(const vec3& s, double psi[3]) {
    const int N = 64; double L2[3] = {0, 0, 0}, f[3] = {0, 0, 0};
    for (int k = 0; k < N; k++) {
        const double y = 1 - 2 * (k + 0.5) / N, r = std::sqrt(1 - y * y), a = k * 2.399963229728653;
        const vec3 d{(float)(r * std::cos(a)), (float)y, (float)(r * std::sin(a))};
        bool sol; const double L = longueur(1.5, d.y, sol); const int n = 16;
        double tau[3] = {0, 0, 0};
        for (int i = 0; i < n; i++) {
            const double t = (i + 0.5) * L / n, dt = L / n, alt = altitude(1.5, d.y, t);
            const Milieu m = milieu(alt);
            double Ts[3]; transmittance(alt, (s.y * (R_SOL + 1.5) + t * dot(d, s)) / (R_SOL + alt), Ts, 8);
            for (int c = 0; c < 3; c++) {
                const double Tv = std::exp(-tau[c]), sig = RAYLEIGH[c] * m.diffR + m.diffM;
                L2[c] += Tv * sig * Ts[c] / (4 * PI_A) * dt / N * 4 * PI_A / (4 * PI_A);   // phase isotrope, moyenne sur la sphère
                f[c] += Tv * sig * dt / N;                                               // f_ms : part rediffusée (isotrope)
                tau[c] += m.ext[c] * dt;
            }
        }
    }
    for (int c = 0; c < 3; c++) psi[c] = L2[c] / std::max(1e-6, 1 - f[c]);
}

// couleurs du ciel pour le soleil (ou la lune) de direction s ; calcul de quelques millisecondes, à garder en cache
inline Ciel ciel(const vec3& s) {
    Ciel C;
    diffusionMultiple(s, C.psiMs);
    luminance({0, 1, 0}, s, C.psiMs, C.zenith);
    // horizon : moyenne sur 8 azimuts à 1° de hauteur (le dégradé du jeu n'a qu'une couleur d'horizon)
    for (int k = 0; k < 8; k++) {
        const double a = k * PI_A / 4, h = rad(1.0);
        double c[3]; luminance(vec3{(float)(std::cos(a) * std::cos(h)), (float)std::sin(h), (float)(std::sin(a) * std::cos(h))}, s, C.psiMs, c);
        for (int q = 0; q < 3; q++) C.horizon[q] += c[q] / 8;
    }
    transmittance(1.5, s.y, C.soleil, 48);
    const Milieu m = milieu(1.5);
    for (int c = 0; c < 3; c++) C.ext[c] = m.ext[c];
    C.extY = lumY(C.ext);
    return C;
}

// chromaticité CIE 1931 (x, y) d'une couleur sRGB linéaire (D65)
inline void chromaticite(const double c[3], double& x, double& y) {
    const double X = 0.4124564 * c[0] + 0.3575761 * c[1] + 0.1804375 * c[2], Y = 0.2126729 * c[0] + 0.7151522 * c[1] + 0.0721750 * c[2],
                 Z = 0.0193339 * c[0] + 0.1191920 * c[1] + 0.9503041 * c[2], s = std::max(X + Y + Z, 1e-12);
    x = X / s; y = Y / s;
}

} // namespace wfs::atmosphere
