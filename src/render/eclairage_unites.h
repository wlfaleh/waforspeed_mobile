// WaForSpeed — WFS-151 (épic E13, ST-1) : unités photométriques, exposition physique, ciel de référence et calculs de
// l'éclairage d'image (IBL). Calcul seul (aucun appel Vulkan) : utilisé par l'éclairage du jeu (eclairage_physique.cpp),
// par le banc (eclairage_banc.cpp) et par le test CPU tests/fidelite_lumiere.cpp.
//
// Conventions (RG-01) :
//   - éclairement E en lux (lx), luminance L en candelas par mètre carré (cd/m²), EV100 en valeurs d'exposition à 100 ISO ;
//   - surface lambertienne d'albédo ρ éclairée par E : L = ρ E / π ;
//   - le tampon HDR du jeu reçoit des luminances « pré-exposées » : valeur = L × k, k = exposition(EV100) (Lagarde et
//     de Rousiers, « Moving Frostbite to Physically Based Rendering 3.0 », SIGGRAPH 2014, § 4.3 et 5.1) ;
//   - couleurs en sRGB linéaire (primaires et blanc D65 de l'IEC 61966-2-1), luminance relative Y (CIE 1931 2°).
// Chaque constante physique cite sa source (DA-realisme.md, RC-04).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace wfs::lumiere {

constexpr double PI_L = 3.14159265358979323846;

// ===================================================================== exposition (RG-02)
// EV100 d'un réglage d'appareil : EV100 = log2(N² / t) − log2(S / 100) (ISO 2720:1974, ouverture N, vitesse t en s, sensibilité S)
inline double ev100(double ouverture, double vitesse, double iso) { return std::log2(ouverture * ouverture / vitesse) - std::log2(iso / 100.0); }
// exposition par saturation (ISO 12232:2006) : luminance qui sature le capteur Lmax = 78 / (S q) × N² / t avec q = 0,65,
// soit Lmax = 1,2 × 2^EV100 à 100 ISO ; le tampon reçoit L / Lmax (Lagarde et de Rousiers 2014, § 5.1, équation 70)
inline double exposition(double ev) { return 1.0 / (1.2 * std::exp2(ev)); }
// posemètre en réflexion (ISO 2720:1974, constante d'étalonnage K = 12,5 cd s m⁻² ISO) : EV100 = log2(L S / K), S = 100
constexpr double K_POSEMETRE = 12.5;
inline double ev100Luminance(double luminanceMoyenne) { return std::log2(std::max(luminanceMoyenne, 1e-9) * 100.0 / K_POSEMETRE); }
inline double luminanceEv100(double ev) { return std::exp2(ev) * K_POSEMETRE / 100.0; }

// bornes de l'exposition automatique par moment (EV100) : jour 10 (intérieur d'un tunnel, ciel très couvert) à 15 (soleil
// de midi, DA-realisme.md § 3 : EV ≈ 15 ; couvert EV 12) ; nuit 3 à 5 (rue éclairée la nuit : EV 3-5, DA-realisme.md § 3)
constexpr double EV_JOUR_MIN = 10, EV_JOUR_MAX = 15, EV_NUIT_MIN = 3, EV_NUIT_MAX = 5;
// EV de référence des intensités artistiques existantes (néons, enseignes, phares, lampadaires) : réglées à l'œil pour une
// image exposée comme EV100 15 le jour (soleil 3,2 × exposition 0,85 = soleil de 102 klx à EV100 14,94) et EV100 4 la nuit
// (milieu de la plage de la rue éclairée). Leur luminance physique en découle (RG-06) : L = e × exposition d'origine / k(EV ref)
constexpr double EV_REF_JOUR = 15, EV_REF_NUIT = 4;

// adaptation de l'œil (RG-02) : constantes de temps 0,5 s vers le clair (EV qui monte), 2 s vers le sombre
constexpr double TAU_VERS_CLAIR = 0.5, TAU_VERS_SOMBRE = 2.0;
inline double adapter(double evActuel, double evCible, double dt) {
    if (dt <= 0) return evActuel;
    const double tau = evCible > evActuel ? TAU_VERS_CLAIR : TAU_VERS_SOMBRE;
    return evActuel + (evCible - evActuel) * (1.0 - std::exp(-dt / tau));
}

// histogramme pondéré (moyenne pondérée au centre) : moyenne du log2 de la luminance entre deux centiles du poids cumulé
// (80 % et 98,3 % : réglages par défaut de l'adaptation de l'exposition d'Unreal Engine 4, « Eye Adaptation », LowPercent /
// HighPercent) ; rend la luminance moyenne (cd/m²) de la bande
struct Cellule { double log2L, poids; };
inline double moyenneHistogramme(std::vector<Cellule> c, double bas = 0.80, double haut = 0.983) {
    double tot = 0; for (const Cellule& x : c) tot += std::max(x.poids, 0.0);
    if (c.empty() || tot <= 0) return 0;
    std::sort(c.begin(), c.end(), [](const Cellule& a, const Cellule& b) { return a.log2L < b.log2L; });
    double cum = 0, s = 0, w = 0;
    for (const Cellule& x : c) {
        const double p = std::max(x.poids, 0.0) / tot, a = cum, b = cum + p;
        const double dedans = std::max(0.0, std::min(b, haut) - std::max(a, bas));   // part de la cellule dans la bande
        s += dedans * x.log2L; w += dedans; cum = b;
    }
    if (w <= 0) return std::exp2(c.back().log2L);
    return std::exp2(s / w);
}

// ===================================================================== soleil (RG-01)
// constante solaire lumineuse hors atmosphère : 133,8 klx (S. Darula, R. Kittler, C. A. Gueymard, « Reference luminous
// solar constant and solar luminance for illuminance calculations », J. Illum. Eng. Soc. 34(1), 2005)
constexpr double E_SOLAIRE_HORS_ATMOSPHERE = 133800.0;
// trouble lumineux de Linke d'une atmosphère claire et sèche : T_v = 2 (conception WFS-151 : avec ce trouble, le modèle
// donne 101 klx au sol horizontal pour un soleil à 70° et 3,6 klx sous incidence normale à 2° : références lum.soleil_midi
// 100 000 lx et lum.soleil_couchant 3 000 lx de references/reel/photometrie.json)
constexpr double TROUBLE_CLAIR = 2.0;
// demi-angle apparent du soleil : 0,2665° (diamètre moyen 32′, Williams, « Sun Fact Sheet », NASA NSSDC) ; angle solide
constexpr double SOLEIL_RAYON_RAD = 0.2665 * PI_L / 180.0;
inline double angleSolideSoleil() { return 2 * PI_L * (1 - std::cos(SOLEIL_RAYON_RAD)); }   // ≈ 6,8 × 10⁻⁵ sr

// masse d'air relative (F. Kasten, A. T. Young, « Revised optical air mass tables and approximation formula », Applied
// Optics 28(22), 1989), hauteur du soleil h en degrés
inline double masseAir(double hDeg) {
    if (hDeg <= -0.5) return 0;
    const double h = std::max(hDeg, 0.0);
    return 1.0 / (std::sin(h * PI_L / 180.0) + 0.50572 * std::pow(h + 6.07995, -1.6364));
}
// éclairement normal direct du soleil (lx) : E = E0 exp(−a_v m T_v), coefficient d'extinction lumineuse
// a_v = 1 / (9,9 + 0,043 m) (M. Navvab, M. Karayel, E. Ne'eman, S. Selkowitz, « Analysis of atmospheric turbidity for
// daylight calculations », Energy and Buildings 6, 1984) ; 0 sous l'horizon
inline double eclairementSoleilNormal(double hDeg, double trouble = TROUBLE_CLAIR) {
    const double m = masseAir(hDeg);
    if (m <= 0) return 0;
    const double av = 1.0 / (9.9 + 0.043 * m);
    return E_SOLAIRE_HORS_ATMOSPHERE * std::exp(-av * m * trouble);
}

// ===================================================================== ciel de référence (RG-01, M-04)
// Distribution relative des luminances : ciel clair normalisé CIE type 12 (CIE S 011/E:2003 = ISO 15469:2004, « Spatial
// distribution of daylight — CIE standard general sky », ciel clair à faible trouble) :
//   L(γ, Z) / L_z = f(γ) φ(Z) / (f(Z_s) φ(0)), φ(Z) = 1 + a exp(b / cos Z), f(χ) = 1 + c [exp(dχ) − exp(dπ/2)] + e cos² χ
// γ : écart angulaire au soleil, Z : distance zénithale du point, Z_s : distance zénithale du soleil.
struct CieCiel { double a, b, c, d, e; };
constexpr CieCiel CIE_TYPE_12{-1.0, -0.32, 10.0, -3.0, 0.45};
inline double cieF(double chi, const CieCiel& k = CIE_TYPE_12) { return 1 + k.c * (std::exp(k.d * chi) - std::exp(k.d * PI_L / 2)) + k.e * std::cos(chi) * std::cos(chi); }
inline double ciePhi(double Z, const CieCiel& k = CIE_TYPE_12) {
    const double cz = std::cos(Z);
    return cz <= 1e-6 ? 1.0 : 1 + k.a * std::exp(k.b / cz);
}
// niveau absolu : éclairement diffus horizontal du ciel clair, ciel ENTIER (plan horizontal dégagé) :
// E_d(h) = 0,8 + 15,5 √(sin h) klx (H. Krochmann, M. Seidl, « Quantitative data on daylight for illuminating engineering », Lighting
// Res. Technol. 6, 1974 ; référentiel : lum.ciel_clair.plein_ciel, ± 15 %, « à confirmer ») : 15 825 lx à h = 70°.
// WFS-214 (RG-02, ANO-109, arbitrage du PO du 2026-11-09, option A, avis expert-realisme-3d) : avant, cette forme était ramenée à
// lum.ciel_clair.intensite = 6 000 lx, valeur mesurée À L'OMBRE (l'obstacle cache le soleil, sa couronne et une partie du ciel) : le ciel
// entier était 2,6 fois trop sombre (zénith 2 669 cd/m² à h 70°, ciel de midi gris-bleu sombre ; ombres 4 IL sous le soleil au lieu de
// 2,5 à 3). Les 6 000 lx restent le CONTRÔLE « point à l'ombre » : au pied d'un mur qui cache la moitié du ciel côté soleil, le même
// ciel donne 5 498 lx (−8 %, test_fidelite_lumiere).
constexpr double E_CIEL_CLAIR_MIDI = 6000.0, H_MIDI_DEG = 70.0;   // contrôle à l'ombre (lum.ciel_clair.intensite)
constexpr double E_CIEL_PLEIN_MIDI = 15825.0;                     // plein ciel à h = 70° (lum.ciel_clair.plein_ciel, Krochmann 1974)
inline double eclairementCielDiffus(double hDeg) {
    if (hDeg <= 0) return 0;
    return 1000.0 * (0.8 + 15.5 * std::sqrt(std::sin(hDeg * PI_L / 180.0)));
}
// luminance relative f(γ) φ(Z) d'une direction (y vers le haut) pour un soleil de direction s (unitaire)
inline double cieRelative(const double d[3], const double s[3], const CieCiel& k = CIE_TYPE_12) {
    const double cg = std::clamp(d[0] * s[0] + d[1] * s[1] + d[2] * s[2], -1.0, 1.0);
    return cieF(std::acos(cg), k) * ciePhi(std::acos(std::clamp(d[1], 0.0, 1.0)), k);
}
// facteur K tel que L(direction) = K f(γ) φ(Z) (cd/m²) donne E_d(h) sur le plan horizontal : K = E_d / ∫ f φ cos Z dω
// (intégrale numérique sur l'hémisphère, 64 × 128 cellules) ; L_z = K f(Z_s) φ(0)
inline double cieFacteur(double hDeg, const CieCiel& k = CIE_TYPE_12) {
    const double Ed = eclairementCielDiffus(hDeg);
    if (Ed <= 0) return 0;
    const double hs = hDeg * PI_L / 180.0, s[3] = {std::cos(hs), std::sin(hs), 0};
    const int NZ = 64, NA = 128; double integ = 0;
    for (int i = 0; i < NZ; i++) {
        const double Z = (i + 0.5) / NZ * PI_L / 2, dZ = PI_L / 2 / NZ;
        for (int j = 0; j < NA; j++) {
            const double A = (j + 0.5) / NA * 2 * PI_L, dA = 2 * PI_L / NA;
            const double d[3] = {std::sin(Z) * std::cos(A), std::cos(Z), std::sin(Z) * std::sin(A)};
            integ += cieRelative(d, s, k) * std::cos(Z) * std::sin(Z) * dZ * dA;
        }
    }
    return Ed / integ;
}
inline double cieLuminanceZenith(double hDeg) { const double Zs = (90 - hDeg) * PI_L / 180.0; return cieFacteur(hDeg) * cieF(Zs) * ciePhi(0); }

// ===================================================================== couleur des lumières
// lieu de Planck en chromaticité CIE 1931 xy (B. Kang, O. Moon, C. Hong, H. Lee, B. Cho, Y. Kim, « Design of advanced color
// temperature control system for HDTV applications », J. Korean Phys. Soc. 41(6), 2002 ; 1 667 à 25 000 K)
inline void planckXy(double T, double& x, double& y) {
    T = std::clamp(T, 1667.0, 25000.0);
    const double t = 1e3 / T, t2 = t * t, t3 = t2 * t;
    x = T <= 4000 ? -0.2661239 * t3 - 0.2343589 * t2 + 0.8776956 * t + 0.179910 : -3.0258469 * t3 + 2.1070379 * t2 + 0.2226347 * t + 0.240390;
    const double x2 = x * x, x3 = x2 * x;
    if (T <= 2222) y = -1.1063814 * x3 - 1.34811020 * x2 + 2.18555832 * x - 0.20219683;
    else if (T <= 4000) y = -0.9549476 * x3 - 1.37418593 * x2 + 2.09137015 * x - 0.16748867;
    else y = 3.0817580 * x3 - 5.87338670 * x2 + 3.75112997 * x - 0.37001483;
}
// couleur sRGB linéaire de luminance relative Y = 1 d'un corps noir à T (XYZ → sRGB linéaire : inverse de la matrice de
// l'IEC 61966-2-1) ; une composante négative (hors gamut) est ramenée à 0
inline void couleurTemperature(double T, double rgb[3]) {
    double x, y; planckXy(T, x, y);
    const double X = x / y, Y = 1.0, Z = (1 - x - y) / y;
    rgb[0] = std::max(0.0, 3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z);
    rgb[1] = std::max(0.0, -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z);
    rgb[2] = std::max(0.0, 0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z);
    const double l = 0.2126729 * rgb[0] + 0.7151522 * rgb[1] + 0.0721750 * rgb[2];
    if (l > 0) for (int i = 0; i < 3; i++) rgb[i] /= l;
}
inline double luminanceRgb(const double c[3]) { return 0.2126729 * c[0] + 0.7151522 * c[1] + 0.0721750 * c[2]; }
// températures du référentiel (photometrie.json) : soleil de midi 5 778 K, pleine lune 4 100 K
constexpr double T_SOLEIL_MIDI = 5778, T_PLEINE_LUNE = 4100;
// éclairements nocturnes du référentiel (photometrie.json) : pleine lune 0,32 lx, ciel de nuit claire sans lune 0,002 lx
constexpr double E_PLEINE_LUNE = 0.32, E_NUIT_SANS_LUNE = 0.002;

// ===================================================================== harmoniques sphériques d'ordre 2 (RG-03)
// base réelle à 9 coefficients et convolution par le cosinus tronqué : Â0 = π, Â1 = 2π/3, Â2 = π/4 (R. Ramamoorthi,
// P. Hanrahan, « An Efficient Representation for Irradiance Environment Maps », SIGGRAPH 2001)
inline void baseSh9(double x, double y, double z, double Y[9]) {
    Y[0] = 0.282095;
    Y[1] = 0.488603 * y; Y[2] = 0.488603 * z; Y[3] = 0.488603 * x;
    Y[4] = 1.092548 * x * y; Y[5] = 1.092548 * y * z; Y[6] = 0.315392 * (3 * z * z - 1); Y[7] = 1.092548 * x * z; Y[8] = 0.546274 * (x * x - y * y);
}
constexpr double SH_A[9] = {PI_L, 2 * PI_L / 3, 2 * PI_L / 3, 2 * PI_L / 3, PI_L / 4, PI_L / 4, PI_L / 4, PI_L / 4, PI_L / 4};
struct Sh9 { double c[9][3] = {}; };   // coefficients de la luminance (rgb)
// WFS-322 (ANO-174) : éclairage indirect diffus affiché, rapproché de la cible (mélange des sondes A et B) avec une constante de temps
// tau (s) : l'arrivée d'une sonde (remplacement de A, première capture après la sonde du ciel) ne change plus la lumière en une image.
// dt ≤ 0 (image redessinée au même temps : bancs) : une image compte pour 1/60 s ; init : valeur prise telle quelle (début, remise à zéro)
constexpr double SH_TAU_S = 0.6;
inline void lisserSh9(Sh9& affiche, const Sh9& cible, double dt, double tau, bool init) {
    const double a = init ? 1.0 : 1.0 - std::exp(-(dt > 0 ? dt : 1.0 / 60) / (tau > 1e-6 ? tau : 1e-6));
    for (int i = 0; i < 9; i++) for (int q = 0; q < 3; q++) affiche.c[i][q] += (cible.c[i][q] - affiche.c[i][q]) * a;
}
// éclairement E(n) (lx si la luminance est en cd/m²) reçu par une surface de normale n
inline void irradianceSh9(const Sh9& sh, double x, double y, double z, double E[3]) {
    double Y[9]; baseSh9(x, y, z, Y);
    for (int c = 0; c < 3; c++) { E[c] = 0; for (int i = 0; i < 9; i++) E[c] += SH_A[i] * sh.c[i][c] * Y[i]; E[c] = std::max(E[c], 0.0); }
}

// ===================================================================== cubemaps sur le CPU
// convention des cubemaps Vulkan (cube_faces.h) : texel (x, y) de la face f ↔ (s, t) = ((x + 0,5) / N, (y + 0,5) / N)
inline void directionTexel(int f, double s, double t, double d[3]) {
    const double sc = 2 * s - 1, tc = 2 * t - 1;
    switch (f) {
        case 0: d[0] = 1; d[1] = -tc; d[2] = -sc; break;
        case 1: d[0] = -1; d[1] = -tc; d[2] = sc; break;
        case 2: d[0] = sc; d[1] = 1; d[2] = tc; break;
        case 3: d[0] = sc; d[1] = -1; d[2] = -tc; break;
        case 4: d[0] = sc; d[1] = -tc; d[2] = 1; break;
        default: d[0] = -sc; d[1] = -tc; d[2] = -1; break;
    }
    const double l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    d[0] /= l; d[1] /= l; d[2] /= l;
}
// face et (s, t) d'une direction (spécification Vulkan, « Cube Map Face Selection »)
inline int faceDirection(const double d[3], double& s, double& t) {
    const double ax = std::fabs(d[0]), ay = std::fabs(d[1]), az = std::fabs(d[2]);
    int f; double sc, tc, ma;
    if (ax >= ay && ax >= az) { ma = ax; if (d[0] > 0) { f = 0; sc = -d[2]; tc = -d[1]; } else { f = 1; sc = d[2]; tc = -d[1]; } }
    else if (ay >= az) { ma = ay; if (d[1] > 0) { f = 2; sc = d[0]; tc = d[2]; } else { f = 3; sc = d[0]; tc = -d[2]; } }
    else { ma = az; if (d[2] > 0) { f = 4; sc = d[0]; tc = -d[1]; } else { f = 5; sc = -d[0]; tc = -d[1]; } }
    s = (sc / ma + 1) / 2; t = (tc / ma + 1) / 2;
    return f;
}
// angle solide d'un texel (x, y) d'une face de côté N (intégrale exacte de la projection sur le cube)
inline double angleSolideTexel(int x, int y, int N) {
    auto a = [](double u, double v) { return std::atan2(u * v, std::sqrt(u * u + v * v + 1)); };
    const double u0 = 2.0 * x / N - 1, u1 = 2.0 * (x + 1) / N - 1, v0 = 2.0 * y / N - 1, v1 = 2.0 * (y + 1) / N - 1;
    return a(u0, v0) - a(u0, v1) - a(u1, v0) + a(u1, v1);
}

// cube en flottants (rgb), une chaîne de niveaux par face : niveau 0 de côté N, puis N / 2… jusqu'à 1
struct CubeF {
    std::vector<int> cotes;                  // côté de chaque niveau
    std::vector<std::vector<float>> faces;   // [niveau × 6 + face] : N × N × 3
    int niveaux() const { return (int)cotes.size(); }
    std::vector<float>& face(int niveau, int f) { return faces[(size_t)niveau * 6 + f]; }
    const std::vector<float>& face(int niveau, int f) const { return faces[(size_t)niveau * 6 + f]; }
    void allouer(int N, int nNiveaux) {
        cotes.clear(); faces.clear();
        for (int m = 0, c = N; m < nNiveaux; m++, c = std::max(1, c / 2)) { cotes.push_back(c); for (int f = 0; f < 6; f++) faces.emplace_back((size_t)c * c * 3, 0.f); }
    }
    // niveaux 1… par moyenne 2 × 2 du niveau précédent
    void reduire() {
        for (int m = 1; m < niveaux(); m++) for (int f = 0; f < 6; f++) {
            const int c = cotes[m], p = cotes[m - 1];
            const std::vector<float>& src = face(m - 1, f); std::vector<float>& dst = face(m, f);
            for (int y = 0; y < c; y++) for (int x = 0; x < c; x++) for (int k = 0; k < 3; k++) {
                const int x0 = std::min(2 * x, p - 1), x1 = std::min(2 * x + 1, p - 1), y0 = std::min(2 * y, p - 1), y1 = std::min(2 * y + 1, p - 1);
                dst[((size_t)y * c + x) * 3 + k] = 0.25f * (src[((size_t)y0 * p + x0) * 3 + k] + src[((size_t)y0 * p + x1) * 3 + k] + src[((size_t)y1 * p + x0) * 3 + k] + src[((size_t)y1 * p + x1) * 3 + k]);
            }
        }
    }
    // lecture bilinéaire dans une face (bords bloqués), niveau fractionnaire (trilinéaire)
    void lire(const double d[3], double niveau, double out[3]) const {
        niveau = std::clamp(niveau, 0.0, (double)niveaux() - 1);
        const int m0 = (int)std::floor(niveau), m1 = std::min(m0 + 1, niveaux() - 1);
        const double fm = niveau - m0;
        double s, t; const int f = faceDirection(d, s, t);
        double a[3] = {0, 0, 0};
        for (int k = 0; k < 2; k++) {
            const int m = k ? m1 : m0; const double w = k ? fm : 1 - fm;
            if (w <= 0) continue;
            const int c = cotes[m]; const std::vector<float>& F = face(m, f);
            const double px = std::clamp(s * c - 0.5, 0.0, c - 1.0), py = std::clamp(t * c - 0.5, 0.0, c - 1.0);
            const int x0 = (int)px, y0 = (int)py, x1 = std::min(x0 + 1, c - 1), y1 = std::min(y0 + 1, c - 1);
            const double fx = px - x0, fy = py - y0;
            for (int q = 0; q < 3; q++) {
                const double v = (F[((size_t)y0 * c + x0) * 3 + q] * (1 - fx) + F[((size_t)y0 * c + x1) * 3 + q] * fx) * (1 - fy)
                               + (F[((size_t)y1 * c + x0) * 3 + q] * (1 - fx) + F[((size_t)y1 * c + x1) * 3 + q] * fx) * fy;
                a[q] += w * v;
            }
        }
        out[0] = a[0]; out[1] = a[1]; out[2] = a[2];
    }
};

// projection d'un cube (niveau 0) sur la base SH9 : L_lm = Σ L(ω) Y_lm(ω) dω
inline Sh9 projeterSh9(const CubeF& c) {
    Sh9 sh; const int N = c.cotes[0];
    for (int f = 0; f < 6; f++) {
        const std::vector<float>& F = c.face(0, f);
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            double d[3]; directionTexel(f, (x + 0.5) / N, (y + 0.5) / N, d);
            double Y[9]; baseSh9(d[0], d[1], d[2], Y);
            const double w = angleSolideTexel(x, y, N);
            for (int i = 0; i < 9; i++) for (int q = 0; q < 3; q++) sh.c[i][q] += F[((size_t)y * N + x) * 3 + q] * Y[i] * w;
        }
    }
    return sh;
}

// ---------------------------------------------------------------- GGX (même modèle que shaders/eclairage.glsl)
// suite de Hammersley (base 2) et tirage d'importance de la normale de facette GGX (B. Karis, « Real Shading in Unreal Engine
// 4 », SIGGRAPH 2013) ; alpha = rugosité² (paramétrage de scene.frag)
inline void hammersley(uint32_t i, uint32_t n, double& u, double& v) {
    uint32_t b = i; b = (b << 16) | (b >> 16); b = ((b & 0x55555555u) << 1) | ((b & 0xAAAAAAAAu) >> 1);
    b = ((b & 0x33333333u) << 2) | ((b & 0xCCCCCCCCu) >> 2); b = ((b & 0x0F0F0F0Fu) << 4) | ((b & 0xF0F0F0F0u) >> 4);
    b = ((b & 0x00FF00FFu) << 8) | ((b & 0xFF00FF00u) >> 8);
    u = (double)i / n; v = b * 2.3283064365386963e-10;
}
inline void echantillonGgx(double u, double v, double alpha, const double N[3], double H[3]) {
    const double phi = 2 * PI_L * u, ct = std::sqrt((1 - v) / (1 + (alpha * alpha - 1) * v)), st = std::sqrt(1 - ct * ct);
    const double up[3] = {std::fabs(N[1]) < 0.999 ? 0.0 : 1.0, std::fabs(N[1]) < 0.999 ? 1.0 : 0.0, 0};
    double T[3] = {up[1] * N[2] - up[2] * N[1], up[2] * N[0] - up[0] * N[2], up[0] * N[1] - up[1] * N[0]};
    const double l = std::sqrt(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]); for (double& x : T) x /= l;
    const double B[3] = {N[1] * T[2] - N[2] * T[1], N[2] * T[0] - N[0] * T[2], N[0] * T[1] - N[1] * T[0]};
    const double hx = st * std::cos(phi), hy = st * std::sin(phi);
    for (int k = 0; k < 3; k++) H[k] = T[k] * hx + B[k] * hy + N[k] * ct;
}
inline double dGgx(double NoH, double alpha) { const double a2 = alpha * alpha, d = NoH * NoH * (a2 - 1) + 1; return a2 / (PI_L * d * d); }
// visibilité de Smith (Schlick-GGX, k = alpha / 2), identique à V_Smith de eclairage.glsl : G / (4 NoV NoL)
inline double visSmith(double NoV, double NoL, double alpha) { const double k = alpha * 0.5; return 0.25 / std::max((NoV * (1 - k) + k) * (NoL * (1 - k) + k), 1e-4); }

// table BRDF du « split-sum » (Karis 2013) : pour (NoV, rugosité), A et B tels que ∫ f cos = F0 A + B (F de Schlick)
inline void tableBrdf(double NoV, double rugosite, int echantillons, double& A, double& B) {
    NoV = std::max(NoV, 1e-4);
    const double alpha = std::max(rugosite * rugosite, 0.002);
    const double V[3] = {std::sqrt(1 - NoV * NoV), 0, NoV}, N[3] = {0, 0, 1};
    A = B = 0;
    for (int i = 0; i < echantillons; i++) {
        double u, v, H[3]; hammersley((uint32_t)i, (uint32_t)echantillons, u, v); echantillonGgx(u, v, alpha, N, H);
        const double VoH = V[0] * H[0] + V[1] * H[1] + V[2] * H[2];
        const double L[3] = {2 * VoH * H[0] - V[0], 2 * VoH * H[1] - V[1], 2 * VoH * H[2] - V[2]};
        const double NoL = L[2], NoH = H[2];
        if (NoL <= 0 || VoH <= 0) continue;
        // estimateur : f cos / pdf, pdf = D NoH / (4 VoH) → V_Smith × 4 VoH NoL / NoH
        const double g = visSmith(NoV, NoL, alpha) * 4 * VoH * NoL / std::max(NoH, 1e-6);
        const double fc = std::pow(1 - VoH, 5);
        A += (1 - fc) * g; B += fc * g;
    }
    A /= echantillons; B /= echantillons;
}

// préfiltrage GGX d'un cube (niveaux de la source déjà réduits) : niveau m de la sortie = rugosité m / (M − 1), N = V = R ;
// lecture de la source au niveau donné par l'angle solide de l'échantillon (« filtered importance sampling »,
// J. Křivánek, M. Colbert, « Real-time Shading with Filtered Importance Sampling », EGSR 2008)
inline double rugositeNiveau(int m, int M) { return M > 1 ? (double)m / (M - 1) : 0.0; }
inline void prefiltrerTexel(const CubeF& src, const double R[3], double rugosite, int echantillons, double out[3]) {
    out[0] = out[1] = out[2] = 0;
    if (rugosite <= 0) { src.lire(R, 0, out); return; }
    const double alpha = std::max(rugosite * rugosite, 0.002), N0 = src.cotes[0];
    const double omegaP = 4 * PI_L / (6 * N0 * N0);
    double tot = 0;
    for (int i = 0; i < echantillons; i++) {
        double u, v, H[3]; hammersley((uint32_t)i, (uint32_t)echantillons, u, v); echantillonGgx(u, v, alpha, R, H);
        const double VoH = R[0] * H[0] + R[1] * H[1] + R[2] * H[2];
        const double L[3] = {2 * VoH * H[0] - R[0], 2 * VoH * H[1] - R[1], 2 * VoH * H[2] - R[2]};
        const double NoL = R[0] * L[0] + R[1] * L[1] + R[2] * L[2];
        if (NoL <= 0) continue;
        const double pdf = dGgx(VoH, alpha) / 4;   // N = V : NoH = VoH
        const double omegaS = 1.0 / (echantillons * pdf + 1e-9);
        const double niveau = 0.5 * std::log2(std::max(omegaS / omegaP, 1.0)) + 1;
        double c[3]; src.lire(L, niveau, c);
        for (int q = 0; q < 3; q++) out[q] += c[q] * NoL;
        tot += NoL;
    }
    if (tot > 0) for (int q = 0; q < 3; q++) out[q] /= tot;
}

// demi-flottant (IEEE 754 binaire16) depuis un flottant : arrondi au plus près, saturé à 65 504
inline uint16_t versDemi(float f) {
    if (!(f == f)) return 0x7E00;
    f = std::clamp(f, -65504.f, 65504.f);
    uint32_t x; std::memcpy(&x, &f, 4);
    const uint32_t signe = (x >> 16) & 0x8000u; int e = (int)((x >> 23) & 0xFF) - 127 + 15; uint32_t m = x & 0x7FFFFFu;
    if (e <= 0) {   // sous-normal ou zéro
        if (e < -10) return (uint16_t)signe;
        m |= 0x800000u; const int dec = 14 - e; uint32_t h = m >> dec; if ((m >> (dec - 1)) & 1u) h++;
        return (uint16_t)(signe | h);
    }
    uint32_t h = signe | ((uint32_t)e << 10) | (m >> 13);
    if (m & 0x1000u) h++;   // arrondi (peut passer à l'exposant suivant : correct)
    return (uint16_t)h;
}

} // namespace wfs::lumiere
