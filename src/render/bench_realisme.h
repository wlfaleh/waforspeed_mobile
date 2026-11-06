// WaForSpeed — WFS-148 : référentiel du réel et banc de fidélité (axe Réalisme, palier 0).
//
// Ce fichier ne contient que des calculs CPU purs (aucun appel Vulkan), utilisés à la fois par le banc
// (bench_realisme.cpp, WFS_BANC=realisme) et par les tests CPU (tests/fidelite_*.cpp) :
//   - soleil() : position du soleil, équations du tableur de la NOAA (domaine public, d'après J. Meeus,
//     Astronomical Algorithms, 1991) ;
//   - couleurs : sRGB ↔ XYZ (CIE 1931 2°), adaptation de Bradford D65 → D50, CIE L*a*b*, écart CIEDE2000
//     (G. Sharma, W. Wu, E. Dalal, « The CIEDE2000 Color-Difference Formula », Color Res. Appl. 30, 2005) ;
//   - lecture des fichiers JSON du référentiel references/reel/ (lecteur borné : un fichier cassé donne une erreur,
//     jamais un plantage) ;
//   - mesures dans les données du jeu : dimensions des voitures (.wfm), largeurs et cycles des lignes de la route
//     (constantes lues dans shaders/route.glsl, WFS-149 et src/world/*.cpp) ;
//   - rapport de fidélité : valeur du jeu, valeur réelle, tolérance, conforme / non conforme / non mesurable, indice
//     par domaine (part des mesures conformes, DA-realisme.md § 4).
// Aucune valeur réelle n'est écrite ici : elles sont toutes dans references/reel/*.json, avec leur source (SOURCES.md).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "../game/roues.h"

namespace wfs {
class Game; class Renderer;
// banc de référence (bench_realisme.cpp) : 12 vues × jour / nuit × Basse / Haute (realisme-<vue>-<moment>-<qualite>.png),
// scène d'étalonnage (realisme-etalon.png), realisme-fidelite.txt et realisme-mesures.txt dans out ; rend le nombre
// d'erreurs (vue impossible à charger, référentiel illisible)
int benchRealisme(Game& g, Renderer& r, const std::string& out);
}

namespace wfs::reel {

constexpr double PI_D = 3.14159265358979323846;
inline double rad(double d) { return d * PI_D / 180.0; }
inline double deg(double r) { return r * 180.0 / PI_D; }

// ===================================================================== soleil (NOAA)
struct Soleil {
    double hauteur = 0;            // hauteur géométrique au-dessus de l'horizon (°), sans réfraction
    double hauteurApparente = 0;   // hauteur corrigée de la réfraction atmosphérique (°), formule du tableur de la NOAA
    double azimut = 0;             // azimut depuis le nord, vers l'est (°), 0 à 360
    double declinaison = 0;        // déclinaison du soleil (°)
    double equationTemps = 0;      // équation du temps (minutes)
};

// jour julien d'une date grégorienne à 0 h UTC (J. Meeus, chap. 7)
inline double jourJulien(int annee, int mois, int jour) {
    if (mois <= 2) { annee -= 1; mois += 12; }
    const int a = annee / 100, b = 2 - a + a / 4;
    return std::floor(365.25 * (annee + 4716)) + std::floor(30.6001 * (mois + 1)) + jour + b - 1524.5;
}

// position du soleil : latitude (° N, négative au sud), longitude (° E, négative à l'ouest), date et heure UTC
// (heure décimale, 0 à 24). Équations du tableur « NOAA Solar Calculator » (gml.noaa.gov/grad/solcalc), précision
// annoncée ± 0,0167° entre 1800 et 2100 pour |latitude| < 72°.
inline Soleil soleil(double lat, double lon, int annee, int mois, int jour, double heureUtc) {
    const double jd = jourJulien(annee, mois, jour) + heureUtc / 24.0;
    const double jc = (jd - 2451545.0) / 36525.0;   // siècles juliens depuis J2000
    const double l0 = std::fmod(280.46646 + jc * (36000.76983 + jc * 0.0003032), 360.0);   // longitude moyenne (°)
    const double m = 357.52911 + jc * (35999.05029 - 0.0001537 * jc);                       // anomalie moyenne (°)
    const double e = 0.016708634 - jc * (0.000042037 + 0.0000001267 * jc);                  // excentricité de l'orbite
    const double c = std::sin(rad(m)) * (1.914602 - jc * (0.004817 + 0.000014 * jc)) + std::sin(rad(2 * m)) * (0.019993 - 0.000101 * jc)
                   + std::sin(rad(3 * m)) * 0.000289;                                         // équation du centre (°)
    const double vraieLong = l0 + c;
    const double omega = 125.04 - 1934.136 * jc;
    const double lambda = vraieLong - 0.00569 - 0.00478 * std::sin(rad(omega));             // longitude apparente (°)
    const double eps0 = 23.0 + (26.0 + (21.448 - jc * (46.815 + jc * (0.00059 - jc * 0.001813))) / 60.0) / 60.0;
    const double eps = eps0 + 0.00256 * std::cos(rad(omega));                               // obliquité corrigée (°)
    const double decl = deg(std::asin(std::sin(rad(eps)) * std::sin(rad(lambda))));
    const double y = std::tan(rad(eps / 2)) * std::tan(rad(eps / 2));
    const double eqt = 4.0 * deg(y * std::sin(2 * rad(l0)) - 2 * e * std::sin(rad(m)) + 4 * e * y * std::sin(rad(m)) * std::cos(2 * rad(l0))
                               - 0.5 * y * y * std::sin(4 * rad(l0)) - 1.25 * e * e * std::sin(2 * rad(m)));   // minutes
    double tst = std::fmod(heureUtc * 60.0 + eqt + 4.0 * lon, 1440.0);   // temps solaire vrai (minutes)
    if (tst < 0) tst += 1440.0;
    const double ha = tst / 4.0 < 0 ? tst / 4.0 + 180.0 : tst / 4.0 - 180.0;   // angle horaire (°)
    const double cz = std::clamp(std::sin(rad(lat)) * std::sin(rad(decl)) + std::cos(rad(lat)) * std::cos(rad(decl)) * std::cos(rad(ha)), -1.0, 1.0);
    const double zen = deg(std::acos(cz));
    Soleil s;
    s.hauteur = 90.0 - zen; s.declinaison = decl; s.equationTemps = eqt;
    // réfraction atmosphérique approchée (tableur de la NOAA), en secondes d'arc
    const double h = s.hauteur, t = std::tan(rad(h));
    double refr = 0;
    if (h > 85.0) refr = 0;
    else if (h > 5.0) refr = 58.1 / t - 0.07 / (t * t * t) + 0.000086 / std::pow(t, 5);
    else if (h > -0.575) refr = 1735.0 + h * (-518.2 + h * (103.4 + h * (-12.79 + h * 0.711)));
    else refr = -20.772 / t;
    s.hauteurApparente = h + refr / 3600.0;
    const double den = std::cos(rad(lat)) * std::sin(rad(zen));
    if (std::fabs(den) < 1e-12) { s.azimut = 0; return s; }   // soleil au zénith ou observateur au pôle : azimut indéfini
    const double ca = std::clamp((std::sin(rad(lat)) * std::cos(rad(zen)) - std::sin(rad(decl))) / den, -1.0, 1.0);
    s.azimut = ha > 0 ? std::fmod(deg(std::acos(ca)) + 180.0, 360.0) : std::fmod(540.0 - deg(std::acos(ca)), 360.0);
    return s;
}

// « 2026-07-15T12:00:00 » → année, mois, jour, heure décimale ; faux si le texte n'a pas cette forme
inline bool lireDateUtc(const std::string& t, int& a, int& mo, int& j, double& h) {
    int hh = 0, mi = 0, se = 0;
    if (std::sscanf(t.c_str(), "%d-%d-%dT%d:%d:%d", &a, &mo, &j, &hh, &mi, &se) != 6) return false;
    if (mo < 1 || mo > 12 || j < 1 || j > 31 || hh < 0 || hh > 23 || mi < 0 || mi > 59 || se < 0 || se > 60) return false;
    h = hh + mi / 60.0 + se / 3600.0;
    return true;
}

// ===================================================================== couleurs (CIE)
struct Lab { double L = 0, a = 0, b = 0; };
// sRGB 8 bits → linéaire (IEC 61966-2-1)
inline double srgbVersLineaire(double c) { c /= 255.0; return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
// sRGB linéaire (primaires et blanc D65 de l'IEC 61966-2-1) → XYZ D65 (Y blanc = 1)
inline void lineaireVersXyz(const double rgb[3], double xyz[3]) {
    xyz[0] = 0.4124564 * rgb[0] + 0.3575761 * rgb[1] + 0.1804375 * rgb[2];
    xyz[1] = 0.2126729 * rgb[0] + 0.7151522 * rgb[1] + 0.0721750 * rgb[2];
    xyz[2] = 0.0193339 * rgb[0] + 0.1191920 * rgb[1] + 0.9503041 * rgb[2];
}
// adaptation chromatique de Bradford D65 → D50 (matrice de Lindbloom, brucelindbloom.com, « Chromatic Adaptation »)
inline void bradfordD65VersD50(const double in[3], double out[3]) {
    out[0] = 1.0478112 * in[0] + 0.0228866 * in[1] - 0.0501270 * in[2];
    out[1] = 0.0295424 * in[0] + 0.9904844 * in[1] - 0.0170491 * in[2];
    out[2] = -0.0092345 * in[0] + 0.0150436 * in[1] + 0.7521316 * in[2];
}
// XYZ → CIE L*a*b* (CIE 15:2004), blanc de référence ICC D50 (X 0,96422, Y 1, Z 0,82521)
inline Lab xyzVersLab(const double xyz[3]) {
    const double w[3] = {0.96422, 1.0, 0.82521}, eps = 216.0 / 24389.0, k = 24389.0 / 27.0;
    double f[3];
    for (int i = 0; i < 3; i++) { const double t = xyz[i] / w[i]; f[i] = t > eps ? std::cbrt(t) : (k * t + 16.0) / 116.0; }
    return {116.0 * f[1] - 16.0, 500.0 * (f[0] - f[1]), 200.0 * (f[1] - f[2])};
}
// CIE L*a*b* D50 → XYZ D50 (inverse de xyzVersLab)
inline void labVersXyz(const Lab& c, double xyz[3]) {
    const double w[3] = {0.96422, 1.0, 0.82521}, eps = 216.0 / 24389.0, k = 24389.0 / 27.0;
    const double fy = (c.L + 16.0) / 116.0, fx = fy + c.a / 500.0, fz = fy - c.b / 200.0;
    const double xr = fx * fx * fx > eps ? fx * fx * fx : (116.0 * fx - 16.0) / k;
    const double yr = c.L > k * eps ? fy * fy * fy : c.L / k;
    const double zr = fz * fz * fz > eps ? fz * fz * fz : (116.0 * fz - 16.0) / k;
    xyz[0] = xr * w[0]; xyz[1] = yr * w[1]; xyz[2] = zr * w[2];
}
// couleur sRGB linéaire (D65) → L*a*b* D50 (même chemin que la charte : Bradford)
inline Lab lineaireVersLab(const double rgb[3]) { double x65[3], x50[3]; lineaireVersXyz(rgb, x65); bradfordD65VersD50(x65, x50); return xyzVersLab(x50); }
inline Lab srgb8VersLab(const double srgb8[3]) {
    const double l[3] = {srgbVersLineaire(srgb8[0]), srgbVersLineaire(srgb8[1]), srgbVersLineaire(srgb8[2])};
    return lineaireVersLab(l);
}
// écart CIEDE2000 (kL = kC = kH = 1), formules de Sharma, Wu et Dalal (2005)
inline double deltaE2000(const Lab& c1, const Lab& c2) {
    const double C1 = std::hypot(c1.a, c1.b), C2 = std::hypot(c2.a, c2.b), Cm = (C1 + C2) / 2;
    const double G = 0.5 * (1 - std::sqrt(std::pow(Cm, 7) / (std::pow(Cm, 7) + std::pow(25.0, 7))));
    const double a1 = (1 + G) * c1.a, a2 = (1 + G) * c2.a;
    const double C1p = std::hypot(a1, c1.b), C2p = std::hypot(a2, c2.b);
    auto teinte = [](double b, double a) { if (a == 0 && b == 0) return 0.0; double h = deg(std::atan2(b, a)); return h < 0 ? h + 360 : h; };
    const double h1 = teinte(c1.b, a1), h2 = teinte(c2.b, a2);
    const double dL = c2.L - c1.L, dC = C2p - C1p;
    double dh = 0;
    if (C1p * C2p != 0) { dh = h2 - h1; if (dh > 180) dh -= 360; else if (dh < -180) dh += 360; }
    const double dH = 2 * std::sqrt(C1p * C2p) * std::sin(rad(dh / 2));
    const double Lm = (c1.L + c2.L) / 2, Cmp = (C1p + C2p) / 2;
    double hm = h1 + h2;
    if (C1p * C2p != 0) { if (std::fabs(h1 - h2) > 180) hm = (h1 + h2 < 360) ? (h1 + h2 + 360) / 2 : (h1 + h2 - 360) / 2; else hm = (h1 + h2) / 2; }
    const double T = 1 - 0.17 * std::cos(rad(hm - 30)) + 0.24 * std::cos(rad(2 * hm)) + 0.32 * std::cos(rad(3 * hm + 6)) - 0.20 * std::cos(rad(4 * hm - 63));
    const double dTheta = 30 * std::exp(-std::pow((hm - 275) / 25, 2));
    const double RC = 2 * std::sqrt(std::pow(Cmp, 7) / (std::pow(Cmp, 7) + std::pow(25.0, 7)));
    const double SL = 1 + 0.015 * (Lm - 50) * (Lm - 50) / std::sqrt(20 + (Lm - 50) * (Lm - 50));
    const double SC = 1 + 0.045 * Cmp, SH = 1 + 0.015 * Cmp * T;
    const double RT = -std::sin(rad(2 * dTheta)) * RC;
    return std::sqrt((dL / SL) * (dL / SL) + (dC / SC) * (dC / SC) + (dH / SH) * (dH / SH) + RT * (dC / SC) * (dH / SH));
}

// ===================================================================== JSON (lecteur borné)
struct Json {
    enum Type { Nul, Bool, Num, Str, Arr, Obj } t = Nul;
    bool b = false; double n = 0; std::string s;
    std::vector<Json> a; std::vector<std::pair<std::string, Json>> o;
    const Json* get(const char* cle) const { if (t != Obj) return nullptr; for (const auto& [k, v] : o) if (k == cle) return &v; return nullptr; }
    bool estNombre() const { return t == Num; }
    double nombre(const char* cle, double defaut) const { const Json* j = get(cle); return j && j->t == Num ? j->n : defaut; }
    std::string texte(const char* cle) const { const Json* j = get(cle); return j && j->t == Str ? j->s : std::string(); }
};

class LecteurJson {
public:
    // le lecteur garde sa copie du texte : il peut être construit depuis une chaîne temporaire (littéral)
    explicit LecteurJson(std::string t) : t_(std::move(t)), p_(t_.data()), f_(t_.data() + t_.size()) {}
    bool lire(Json& out, std::string& erreur) {
        if (!valeur(out, 0)) { erreur = erreur_.empty() ? "JSON invalide" : erreur_; return false; }
        blancs();
        if (p_ != f_) { erreur = "texte après la fin du JSON"; return false; }
        return true;
    }
private:
    std::string t_; const char* p_; const char* f_; std::string erreur_;
    void blancs() { while (p_ < f_ && (*p_ == ' ' || *p_ == '\n' || *p_ == '\r' || *p_ == '\t')) p_++; }
    bool echec(const char* m) { if (erreur_.empty()) erreur_ = m; return false; }
    static void utf8(std::string& s, uint32_t c) {
        if (c < 0x80) s += (char)c;
        else if (c < 0x800) { s += (char)(0xC0 | (c >> 6)); s += (char)(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { s += (char)(0xE0 | (c >> 12)); s += (char)(0x80 | ((c >> 6) & 0x3F)); s += (char)(0x80 | (c & 0x3F)); }
        else { s += (char)(0xF0 | (c >> 18)); s += (char)(0x80 | ((c >> 12) & 0x3F)); s += (char)(0x80 | ((c >> 6) & 0x3F)); s += (char)(0x80 | (c & 0x3F)); }
    }
    bool hex4(uint32_t& c) {
        if (f_ - p_ < 4) return echec("\\u incomplet");
        c = 0;
        for (int i = 0; i < 4; i++) {
            const char h = *p_++; c <<= 4;
            if (h >= '0' && h <= '9') c |= (uint32_t)(h - '0'); else if (h >= 'a' && h <= 'f') c |= (uint32_t)(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') c |= (uint32_t)(h - 'A' + 10); else return echec("\\u invalide");
        }
        return true;
    }
    bool chaine(std::string& s) {
        if (p_ >= f_ || *p_ != '"') return echec("chaîne attendue");
        p_++;
        while (p_ < f_ && *p_ != '"') {
            if ((unsigned char)*p_ < 0x20) return echec("caractère de contrôle dans une chaîne");
            if (*p_ != '\\') { s += *p_++; continue; }
            if (++p_ >= f_) return echec("échappement incomplet");
            const char e = *p_++;
            switch (e) {
                case '"': s += '"'; break; case '\\': s += '\\'; break; case '/': s += '/'; break;
                case 'b': s += '\b'; break; case 'f': s += '\f'; break; case 'n': s += '\n'; break;
                case 'r': s += '\r'; break; case 't': s += '\t'; break;
                case 'u': {
                    uint32_t c; if (!hex4(c)) return false;
                    if (c >= 0xD800 && c < 0xDC00) {   // paire de substitution UTF-16
                        uint32_t d;
                        if (f_ - p_ < 2 || p_[0] != '\\' || p_[1] != 'u') return echec("paire UTF-16 incomplète");
                        p_ += 2; if (!hex4(d) || d < 0xDC00 || d > 0xDFFF) return echec("paire UTF-16 invalide");
                        c = 0x10000 + ((c - 0xD800) << 10) + (d - 0xDC00);
                    }
                    utf8(s, c); break;
                }
                default: return echec("échappement inconnu");
            }
        }
        if (p_ >= f_) return echec("chaîne non fermée");
        p_++;
        return true;
    }
    bool valeur(Json& v, int profondeur) {
        if (profondeur > 64) return echec("JSON trop imbriqué");
        blancs();
        if (p_ >= f_) return echec("fin inattendue");
        const char c = *p_;
        if (c == '{') {
            v.t = Json::Obj; p_++; blancs();
            if (p_ < f_ && *p_ == '}') { p_++; return true; }
            for (;;) {
                blancs(); std::string k; if (!chaine(k)) return false;
                blancs(); if (p_ >= f_ || *p_ != ':') return echec("« : » attendu"); p_++;
                Json x; if (!valeur(x, profondeur + 1)) return false;
                v.o.emplace_back(std::move(k), std::move(x));
                blancs(); if (p_ >= f_) return echec("objet non fermé");
                if (*p_ == ',') { p_++; continue; }
                if (*p_ == '}') { p_++; return true; }
                return echec("« , » ou « } » attendu");
            }
        }
        if (c == '[') {
            v.t = Json::Arr; p_++; blancs();
            if (p_ < f_ && *p_ == ']') { p_++; return true; }
            for (;;) {
                Json x; if (!valeur(x, profondeur + 1)) return false;
                v.a.push_back(std::move(x));
                blancs(); if (p_ >= f_) return echec("tableau non fermé");
                if (*p_ == ',') { p_++; continue; }
                if (*p_ == ']') { p_++; return true; }
                return echec("« , » ou « ] » attendu");
            }
        }
        if (c == '"') { v.t = Json::Str; return chaine(v.s); }
        auto mot = [&](const char* m) { const size_t n = std::strlen(m); if ((size_t)(f_ - p_) >= n && !std::strncmp(p_, m, n)) { p_ += n; return true; } return false; };
        if (mot("true")) { v.t = Json::Bool; v.b = true; return true; }
        if (mot("false")) { v.t = Json::Bool; v.b = false; return true; }
        if (mot("null")) { v.t = Json::Nul; return true; }
        // nombre : la grammaire JSON, sans dépendre de la langue du système
        const char* d = p_;
        if (p_ < f_ && *p_ == '-') p_++;
        if (p_ >= f_ || *p_ < '0' || *p_ > '9') return echec("valeur inattendue");
        while (p_ < f_ && *p_ >= '0' && *p_ <= '9') p_++;
        if (p_ < f_ && *p_ == '.') { p_++; while (p_ < f_ && *p_ >= '0' && *p_ <= '9') p_++; }
        if (p_ < f_ && (*p_ == 'e' || *p_ == 'E')) { p_++; if (p_ < f_ && (*p_ == '+' || *p_ == '-')) p_++; while (p_ < f_ && *p_ >= '0' && *p_ <= '9') p_++; }
        std::istringstream is(std::string(d, p_)); is.imbue(std::locale::classic());
        v.t = Json::Num;
        if (!(is >> v.n)) return echec("nombre invalide");
        return true;
    }
};

inline bool lireTexte(const std::string& chemin, std::string& out) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf(); out = ss.str();
    return out.size() < (64u << 20);   // borne : 64 Mo (le plus gros fichier du référentiel fait ≈ 100 Ko)
}
inline bool lireJson(const std::string& chemin, Json& out, std::string& erreur) {
    std::string t;
    if (!lireTexte(chemin, t)) { erreur = "fichier illisible : " + chemin; return false; }
    LecteurJson l(std::move(t));
    if (!l.lire(out, erreur)) { erreur = chemin + " : " + erreur; return false; }
    return true;
}

// fichiers du référentiel (references/reel/), dans l'ordre du rapport
inline const char* const FICHIERS_REFERENTIEL[] = {"voitures.json", "marquage.json", "soleil_controle.json", "colorchecker.json", "photometrie.json",
                                                   "materiaux.json", "anthropometrie.json", "lieux.json"};

// une mesure du référentiel par son identifiant (dans tous les fichiers chargés)
struct Referentiel {
    std::map<std::string, Json> fichiers;
    std::string erreur;
    bool charger(const std::string& dossier) {
        for (const char* f : FICHIERS_REFERENTIEL) {
            Json j; std::string e;
            if (!lireJson(dossier + "/" + f, j, e)) { erreur = e; return false; }
            fichiers[f] = std::move(j);
        }
        return true;
    }
    const Json* mesure(const std::string& id) const {
        for (const auto& [n, f] : fichiers) if (const Json* ms = f.get("mesures")) for (const Json& m : ms->a) if (m.texte("id") == id) return &m;
        return nullptr;
    }
};

// ===================================================================== rapport de fidélité
enum class Etat { Conforme, NonConforme, NonMesurable };
struct LigneRapport { std::string domaine, id, grandeur, jeu, reel, tolerance, statutReel; Etat etat = Etat::NonMesurable; std::string raison; };

inline std::string fmt(double v, int dec = 3) { char b[64]; std::snprintf(b, sizeof b, "%.*f", dec, v); return b; }

// valeur réelle et tolérance d'une mesure du référentiel, en texte ; bornes [bas, haut] acceptées (tolérance comprise)
inline bool bornes(const Json& m, double& bas, double& haut, std::string& reel, std::string& tol) {
    const Json* t = m.get("tolerance");
    const std::string type = t ? t->texte("type") : "";
    const double tv = t ? t->nombre("valeur", 0) : 0;
    double v0, v1;
    const Json* v = m.get("valeur");
    if (v && v->t == Json::Num) { v0 = v1 = v->n; reel = fmt(v->n, 4); }
    else if (m.get("min") && m.get("max") && m.get("min")->t == Json::Num && m.get("max")->t == Json::Num) {
        v0 = m.get("min")->n; v1 = m.get("max")->n; reel = "[" + fmt(v0, 4) + " ; " + fmt(v1, 4) + "]";
    } else return false;
    if (type == "relative") { bas = v0 * (1 - tv); haut = v1 * (1 + tv); tol = "± " + fmt(tv * 100, 1) + " %"; }
    else if (type == "absolue") { bas = v0 - tv; haut = v1 + tv; tol = "± " + fmt(tv, 4); }
    else return false;
    return true;
}

class Rapport {
public:
    std::vector<LigneRapport> lignes;
    // compare une valeur du jeu à une mesure du référentiel ; facteur : unité du référentiel → unité du jeu (mm → m : 0,001)
    void comparer(const Referentiel& R, const std::string& domaine, const std::string& idReel, double jeu, double facteur, const std::string& unite,
                  const std::string& grandeur = "") {
        LigneRapport l; l.domaine = domaine; l.id = idReel; l.jeu = fmt(jeu, 4) + " " + unite;
        const Json* m = R.mesure(idReel);
        if (!m) { l.grandeur = grandeur; l.raison = "mesure absente du référentiel"; lignes.push_back(l); return; }
        l.grandeur = grandeur.empty() ? m->texte("grandeur") : grandeur;
        l.statutReel = m->texte("statut");
        double bas, haut; std::string reel, tol;
        if (!bornes(*m, bas, haut, reel, tol)) { l.raison = "valeur réelle à sourcer : " + m->texte("note"); lignes.push_back(l); return; }
        l.reel = reel + " " + m->texte("unite") + (facteur != 1 ? " (× " + fmt(facteur, 4) + " → " + unite + ")" : "");
        l.tolerance = tol;
        l.etat = (jeu >= bas * facteur - 1e-9 && jeu <= haut * facteur + 1e-9) ? Etat::Conforme : Etat::NonConforme;
        lignes.push_back(l);
    }
    void nonMesurable(const std::string& domaine, const std::string& id, const std::string& grandeur, const std::string& raison) {
        LigneRapport l; l.domaine = domaine; l.id = id; l.grandeur = grandeur; l.raison = raison; lignes.push_back(l);
    }
    void ajouter(LigneRapport l) { lignes.push_back(std::move(l)); }
    struct Indice {
        int conformes = 0, nonConformes = 0, nonMesurables = 0;
        bool mesurable() const { return conformes + nonConformes > 0; }   // WFS-149 (RG-B, ANO-86)
        double valeur() const { const int n = conformes + nonConformes; return n ? 100.0 * conformes / n : 0; }
        // WFS-149 (RG-B, ANO-86) : un domaine sans aucune mesure possible s'écrit « non mesurable », jamais « 0.0 % »
        std::string enTexte() const { return mesurable() ? fmt(valeur(), 1) + " %" : std::string("non mesurable"); }
    };
    std::map<std::string, Indice> indices() const {
        std::map<std::string, Indice> r;
        for (const auto& l : lignes) { Indice& i = r[l.domaine]; if (l.etat == Etat::Conforme) i.conformes++; else if (l.etat == Etat::NonConforme) i.nonConformes++; else i.nonMesurables++; }
        return r;
    }
    std::string texte(const std::string& titre) const {
        std::ostringstream o;
        o << titre << "\n";
        o << "Indice de fidélité = part des mesures conformes parmi les mesures mesurables (DA-realisme.md § 4) ; « non mesurable » n'entre pas dans l'indice.\n\n";
        o << "== Indices par domaine ==\n";
        Indice tout;
        for (const auto& [d, i] : indices()) {
            o << "  " << d << " : " << i.enTexte() << " (" << i.conformes << " conformes / " << (i.conformes + i.nonConformes) << " mesurables, "
              << i.nonMesurables << " non mesurables)\n";
            tout.conformes += i.conformes; tout.nonConformes += i.nonConformes; tout.nonMesurables += i.nonMesurables;
        }
        o << "  TOTAL : " << tout.enTexte() << " (" << tout.conformes << " / " << (tout.conformes + tout.nonConformes) << ", " << tout.nonMesurables << " non mesurables)\n";
        std::string dom;
        std::vector<LigneRapport> tri = lignes;   // une section par domaine (ordre d'ajout gardé dans chaque domaine)
        std::stable_sort(tri.begin(), tri.end(), [](const LigneRapport& a, const LigneRapport& b) { return a.domaine < b.domaine; });
        for (const auto& l : tri) {
            if (l.domaine != dom) { dom = l.domaine; o << "\n== " << dom << " ==\n"; o << "  id | grandeur | jeu | réel | tolérance | état | statut de la valeur réelle\n"; }
            o << "  " << l.id << " | " << l.grandeur << " | " << (l.jeu.empty() ? "-" : l.jeu) << " | " << (l.reel.empty() ? "-" : l.reel) << " | "
              << (l.tolerance.empty() ? "-" : l.tolerance) << " | "
              << (l.etat == Etat::Conforme ? "CONFORME" : l.etat == Etat::NonConforme ? "NON CONFORME (écart attendu, état zéro)" : "NON MESURABLE")
              << " | " << (l.statutReel.empty() ? "-" : l.statutReel) << (l.raison.empty() ? "" : " — " + l.raison) << "\n";
        }
        return o.str();
    }
};

// ===================================================================== mesures dans les données du jeu
// voiture convertie (.wfm, format de tools/convert_car.mjs) : boîte englobante, largeur sans rétroviseurs, roues
struct MesureVoiture {
    bool ok = false; std::string erreur;
    double longueur = 0, largeur = 0, largeurSansRetro = 0, hauteur = 0;   // m
    bool roues = false; double empattement = 0, voieAv = 0, voieAr = 0, diametreAv = 0, diametreAr = 0;   // m
};
// Largeur sans rétroviseurs : plus grande largeur des sommets situés sous 55 % de la hauteur (les rétroviseurs sont
// au-dessus, à hauteur de vitre) — méthode de mesure de WFS-148, à affiner par WFS-158 si un modèle la met en défaut.
inline MesureVoiture mesurerWfm(const std::string& chemin) {
    MesureVoiture r;
    std::string b;
    if (!lireTexte(chemin, b)) { r.erreur = "fichier illisible"; return r; }
    size_t o = 0;
    auto u32 = [&](uint32_t& x) { if (o + 4 > b.size()) return false; std::memcpy(&x, &b[o], 4); o += 4; return true; };
    if (b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) != 0) { r.erreur = "format inconnu"; return r; }
    o = 4; uint32_t ver, nT, nM, nMe;
    if (!u32(ver) || !u32(nT) || !u32(nM) || !u32(nMe)) { r.erreur = "en-tête tronqué"; return r; }
    float bb[6]; std::memcpy(bb, &b[o], 24); o += 24;
    r.longueur = bb[5] - bb[2]; r.largeur = bb[3] - bb[0]; r.hauteur = bb[4] - bb[1];
    for (uint32_t t = 0; t < nT; t++) { uint32_t n; if (!u32(n) || o + n > b.size()) { r.erreur = "texture tronquée"; return r; } o += n; }
    std::vector<uint32_t> flags;
    for (uint32_t m = 0; m < nM; m++) { if (o + 52 > b.size()) { r.erreur = "matériau tronqué"; return r; } o += 52; uint32_t f; if (!u32(f)) { r.erreur = "matériau tronqué"; return r; } flags.push_back(f); }
    std::vector<std::vector<float>> V; std::vector<std::vector<uint32_t>> I; std::vector<uint32_t> mat;
    for (uint32_t m = 0; m < nMe; m++) {
        uint32_t mi, nv, ni;
        if (!u32(mi) || !u32(nv) || !u32(ni) || o + (size_t)nv * 32 + (size_t)ni * 4 > b.size() || mi >= flags.size()) { r.erreur = "maillage tronqué"; return r; }
        V.emplace_back((size_t)nv * 8); std::memcpy(V.back().data(), &b[o], (size_t)nv * 32); o += (size_t)nv * 32;
        I.emplace_back(ni); std::memcpy(I.back().data(), &b[o], (size_t)ni * 4); o += (size_t)ni * 4;
        mat.push_back(mi);
    }
    const double ySeuil = bb[1] + 0.55 * r.hauteur, cx = 0.5 * (bb[0] + bb[3]);
    double xmax = 0;
    for (const auto& v : V) for (size_t i = 0; i + 7 < v.size(); i += 8) if (v[i + 1] < ySeuil) xmax = std::max(xmax, std::fabs(v[i] - cx));
    r.largeurSansRetro = 2 * xmax;
    std::vector<roues::Maillage> ms;
    for (size_t k = 0; k < V.size(); k++) ms.push_back({V[k].data(), 8, (uint32_t)(V[k].size() / 8), I[k].data(), (uint32_t)I[k].size(), (flags[mat[k]] & 128) != 0});
    const roues::Decoupe d = roues::decouper(ms);
    if (d.n == 4) {
        r.roues = true;
        const auto& w = d.roues;   // LF, RF, LB, RB
        r.empattement = 0.5 * ((w[roues::LB].centre.z + w[roues::RB].centre.z) - (w[roues::LF].centre.z + w[roues::RF].centre.z));
        r.voieAv = std::fabs(w[roues::RF].centre.x - w[roues::LF].centre.x); r.voieAr = std::fabs(w[roues::RB].centre.x - w[roues::LB].centre.x);
        r.diametreAv = w[roues::LF].rayon + w[roues::RF].rayon; r.diametreAr = w[roues::LB].rayon + w[roues::RB].rayon;
    }
    r.ok = true;
    return r;
}

// route : constantes du dessin des lignes (shaders/route.glsl depuis WFS-149) et des passages piétons (src/world/ville.cpp, campagne.cpp)
struct MesureRoute {
    bool lignes = false, pieton = false; std::string erreur;
    double periode = 0, trait = 0, largeurAxe = 0, largeurRive = 0, retraitRive = 0, diviseurVoies = 0;   // m (diviseur : sans unité)
    double bandeLargeur = 0, intervalle = 0, bandeLongueurSeg = 0;                                        // m ; longueur en segments
};
inline bool chercher(const std::string& texte, const char* motif, std::smatch& m) { return std::regex_search(texte, m, std::regex(motif)); }
// passage piéton dessiné par un bloc « case …ZEBRA » : 8 bandes de largeur x1 − x0 réparties sur 2R − marge, longueur en segments
inline bool passagePieton(const std::string& source, const char* marque, double& largeur, double& intervalle, double& longueurSeg) {
    const size_t z = source.find(marque);
    if (z == std::string::npos) return false;
    const std::string bloc = source.substr(z, 900);
    std::smatch m1, m2, m3;
    if (!chercher(bloc, R"(poseAt\(t, a \+ ([0-9]+)\))", m1) || !chercher(bloc, R"(\(2 \* R - ([0-9.]+)f\) / ([0-9]+))", m2) || !chercher(bloc, R"(x1 = x0 \+ ([0-9.]+)f)", m3)) return false;
    longueurSeg = std::stod(m1[1]);
    largeur = std::stod(m3[1]);
    intervalle = (2 * 7.5 - std::stod(m2[1])) / std::stod(m2[2]) - largeur;   // 7,5 m : RW, demi-largeur de route (src/game/track.h)
    return true;
}
inline MesureRoute mesurerRoute(const std::string& sceneFrag, const std::string& villeCpp) {
    MesureRoute r; std::smatch m;
    const bool a = chercher(sceneFrag, R"(fract\(p\.y / ([0-9.]+)\), ([0-9.]+)\))", m);
    if (a) { r.periode = std::stod(m[1]); r.trait = r.periode * std::stod(m[2]); }
    const bool b = chercher(sceneFrag, R"(\(1\.0 - smoothstep\(([0-9.]+), ([0-9.]+), abs\(p\.x - lx\)\)\))", m);
    if (b) r.largeurAxe = std::stod(m[1]) + std::stod(m[2]);   // demi-largeur au milieu de la transition, × 2
    const bool c = chercher(sceneFrag, R"(smoothstep\(([0-9.]+), ([0-9.]+), abs\(abs\(p\.x\) - \(halfW - ([0-9.]+)\)\)\))", m);
    if (c) { r.largeurRive = std::stod(m[1]) + std::stod(m[2]); r.retraitRive = std::stod(m[3]); }
    const bool d = chercher(sceneFrag, R"(float\(k\) \* halfW / ([0-9.]+))", m);
    if (d) r.diviseurVoies = std::stod(m[1]);
    r.lignes = a && b && c && d;
    if (!r.lignes) r.erreur = "constantes des lignes introuvables dans shaders/route.glsl (code modifié : mettre à jour mesurerRoute)";
    r.pieton = passagePieton(villeCpp, "case ville::ZEBRA", r.bandeLargeur, r.intervalle, r.bandeLongueurSeg);
    if (!r.pieton) r.erreur += std::string(r.erreur.empty() ? "" : " ; ") + "passage piéton introuvable dans src/world/ville.cpp";
    return r;
}

// ===================================================================== contrôles du référentiel (test CPU)
// chaque mesure : identifiant unique, domaine, unité, source déclarée dans son fichier et dans SOURCES.md, statut connu,
// tolérance ; valeur présente sauf statut « a_sourcer ». Rend la liste des défauts (vide = référentiel conforme à RG-01).
inline std::vector<std::string> controlerReferentiel(const Referentiel& R, const std::string& sourcesMd) {
    std::vector<std::string> d; std::map<std::string, int> vus;
    const char* statuts[] = {"verifie", "derive", "a_confirmer", "a_sourcer", "conception"};
    for (const auto& [nom, f] : R.fichiers) {
        const Json* src = f.get("sources"); const Json* ms = f.get("mesures");
        if (!src || src->t != Json::Obj) { d.push_back(nom + " : pas de liste « sources »"); continue; }
        if (!ms || ms->t != Json::Arr || ms->a.empty()) { d.push_back(nom + " : pas de mesures"); continue; }
        for (const auto& [sid, s] : src->o) {
            if (s.texte("titre").empty() || s.texte("lien").empty() || s.texte("acces").empty()) d.push_back(nom + " : source " + sid + " sans titre, lien ou accès");
            if (sourcesMd.find("`" + sid + "`") == std::string::npos) d.push_back(nom + " : source " + sid + " absente de SOURCES.md");
        }
        for (const Json& m : ms->a) {
            const std::string id = m.texte("id"), st = m.texte("statut");
            if (id.empty()) { d.push_back(nom + " : mesure sans identifiant"); continue; }
            if (vus[id]++) d.push_back(id + " : identifiant en double");
            if (m.texte("domaine").empty() || m.texte("unite").empty() || m.texte("grandeur").empty()) d.push_back(id + " : domaine, unité ou grandeur manquant");
            if (!src->get(m.texte("source").c_str())) d.push_back(id + " : source « " + m.texte("source") + " » non déclarée");
            if (std::none_of(std::begin(statuts), std::end(statuts), [&](const char* s) { return st == s; })) d.push_back(id + " : statut inconnu « " + st + " »");
            const Json* t = m.get("tolerance");
            if (st != "a_sourcer" && (!t || t->texte("type").empty())) d.push_back(id + " : pas de tolérance");
            const Json* v = m.get("valeur");
            const bool valeur = (v && v->t != Json::Nul) || (m.get("min") && m.get("max"));
            if (st != "a_sourcer" && !valeur) d.push_back(id + " : valeur manquante (statut « " + st + " »)");
            if (st == "a_sourcer" && valeur) d.push_back(id + " : valeur donnée alors que le statut est « a_sourcer »");
        }
    }
    return d;
}

// mesures géométriques (voitures, routes) et soleil calculé : partie du rapport qui se lit dans les données du jeu.
// racine : dossier du dépôt (« . » au banc, WFS_SOURCE_DIR dans les tests) ; cars : identifiants des 20 voitures.
inline void mesuresDonnees(const Referentiel& R, Rapport& rap, const std::string& racine, const std::vector<std::string>& cars) {
    // ---- soleil : soleil() contre les 10 cas de contrôle (M-01)
    for (int k = 1; k <= 10; k++) {
        char id[64]; std::snprintf(id, sizeof id, "soleil.cas%02d", k);
        const Json* h = R.mesure(std::string(id) + ".hauteur");
        int a, mo, j; double hu;
        if (!h || !lireDateUtc(h->texte("utc"), a, mo, j, hu)) { rap.nonMesurable("soleil", id, "cas de contrôle", "cas absent du référentiel"); continue; }
        const Soleil s = soleil(h->nombre("lat", 0), h->nombre("lon", 0), a, mo, j, hu);
        rap.comparer(R, "soleil", std::string(id) + ".hauteur", s.hauteur, 1, "°");
        rap.comparer(R, "soleil", std::string(id) + ".hauteur_apparente", s.hauteurApparente, 1, "°");
        // azimut : écart mesuré sur le ciel (Δaz × cos h), comme test_fidelite_soleil (près du zénith l'azimut n'a plus de sens)
        if (const Json* az = R.mesure(std::string(id) + ".azimut")) {
            double d = std::fabs(s.azimut - az->nombre("valeur", 0)); if (d > 180) d = 360 - d;
            const double surCiel = d * std::cos(rad(s.hauteur)), tol = az->get("tolerance") ? az->get("tolerance")->nombre("valeur", 0) : 0;
            LigneRapport l; l.domaine = "soleil"; l.id = std::string(id) + ".azimut"; l.grandeur = az->texte("grandeur") + " (écart sur le ciel : Δaz × cos h)";
            l.jeu = fmt(s.azimut, 4) + " ° (écart " + fmt(surCiel, 4) + " ° sur le ciel, " + fmt(d, 4) + " ° brut)"; l.reel = fmt(az->nombre("valeur", 0), 4) + " °";
            l.tolerance = "± " + fmt(tol, 2) + " °"; l.statutReel = az->texte("statut"); l.etat = surCiel <= tol ? Etat::Conforme : Etat::NonConforme;
            rap.ajouter(l);
        }
    }
    // ---- voitures (M-02) : modèles chargés (.wfm) contre les fiches constructeur
    for (const std::string& c : cars) {
        const MesureVoiture v = mesurerWfm(racine + "/assets/cars/" + c + ".wfm");
        const std::string p = "voiture." + c + ".";
        if (!v.ok) { rap.nonMesurable("voitures", p + "*", "modèle " + c, "lecture du .wfm impossible : " + v.erreur); continue; }
        rap.comparer(R, "voitures", p + "longueur", v.longueur, 0.001, "m");
        rap.comparer(R, "voitures", p + "largeur", v.largeurSansRetro, 0.001, "m", "largeur sans rétroviseurs (sommets sous 55 % de la hauteur)");
        rap.comparer(R, "voitures", p + "hauteur", v.hauteur, 0.001, "m");
        if (!v.roues) { for (const char* g : {"empattement", "voie_av", "voie_ar", "roue_av", "roue_ar"}) rap.nonMesurable("voitures", p + g, g, "roues du modèle non trouvées (roues::decouper)"); continue; }
        rap.comparer(R, "voitures", p + "empattement", v.empattement, 0.001, "m");
        rap.comparer(R, "voitures", p + "voie_av", v.voieAv, 0.001, "m");
        rap.comparer(R, "voitures", p + "voie_ar", v.voieAr, 0.001, "m");
        rap.comparer(R, "voitures", p + "roue_av", v.diametreAv, 0.001, "m", "diamètre des roues avant (2 × rayon de roues::decouper)");
        rap.comparer(R, "voitures", p + "roue_ar", v.diametreAr, 0.001, "m", "diamètre des roues arrière");
    }
    // ---- routes (M-03) : constantes du dessin contre les normes
    std::string frag, ville;
    const bool okF = lireTexte(racine + "/shaders/route.glsl", frag), okV = lireTexte(racine + "/src/world/ville.cpp", ville);
    if (!okF || !okV) { rap.nonMesurable("routes", "route.*", "lignes et passages piétons", "shaders/route.glsl ou src/world/ville.cpp introuvable depuis " + racine); return; }
    const MesureRoute m = mesurerRoute(frag, ville);
    if (m.lignes) {
        const double hw = 7.5;   // RW (src/game/track.h) : demi-largeur de chaussée passée au shader (Game::render)
        const double centre = 2 * hw / m.diviseurVoies, bord = (hw - m.retraitRive) - hw / m.diviseurVoies;
        rap.comparer(R, "routes", "route.voie.route_principale", centre, 1, "m", "voie centrale (axe à axe des lignes), Campagne et Corniche");
        rap.comparer(R, "routes", "route.voie.route_principale", bord, 1, "m", "voie de bord (ligne de voie à ligne de rive), Campagne et Corniche");
        rap.comparer(R, "routes", "route.voie.agglomeration", centre, 1, "m", "voie centrale, Ville");
        rap.comparer(R, "routes", "route.t1.trait", m.trait, 1, "m", "trait des lignes discontinues, Campagne et Corniche");
        rap.comparer(R, "routes", "route.t1.periode", m.periode, 1, "m", "période des lignes discontinues, Campagne et Corniche");
        rap.comparer(R, "routes", "route.t1.trait_agglo", m.trait, 1, "m", "trait des lignes discontinues, Ville");
        rap.comparer(R, "routes", "route.t1.periode_agglo", m.periode, 1, "m", "période des lignes discontinues, Ville");
        rap.comparer(R, "routes", "route.ligne.largeur_axe", m.largeurAxe, 1, "m", "largeur des lignes de voie");
        rap.comparer(R, "routes", "route.ligne.largeur_rive", m.largeurRive, 1, "m", "largeur des lignes de rive");
    } else rap.nonMesurable("routes", "route.lignes", "lignes de la chaussée", m.erreur);
    if (m.pieton) {
        rap.comparer(R, "routes", "route.pieton.bande_largeur", m.bandeLargeur, 1, "m", "passage piéton : largeur d'une bande (Ville, Campagne)");
        rap.comparer(R, "routes", "route.pieton.intervalle", m.intervalle, 1, "m", "passage piéton : intervalle entre bandes");
        rap.comparer(R, "routes", "route.pieton.bande_longueur_agglo", m.bandeLongueurSeg * 1.2, 1, "m", "passage piéton : longueur des bandes (Ville ; segments de 1,2 m)");
    } else rap.nonMesurable("routes", "route.pieton", "passage piéton", m.erreur);
    std::string campagne; double cl = 0, ci = 0, cs = 0;
    if (lireTexte(racine + "/src/world/campagne.cpp", campagne) && passagePieton(campagne, "case T_ZEBRA", cl, ci, cs))
        rap.comparer(R, "routes", "route.pieton.bande_longueur_hors", cs * 1.2, 1, "m", "passage piéton : longueur des bandes (Campagne, hors agglomération)");
    else rap.nonMesurable("routes", "route.pieton.bande_longueur_hors", "passage piéton de la Campagne", "bloc « case T_ZEBRA » introuvable dans src/world/campagne.cpp");
}

} // namespace wfs::reel
