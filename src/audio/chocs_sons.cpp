// WaForSpeed — son des chocs selon ce qu'on touche (WFS-298) : voir chocs_sons.h.
#include "chocs_sons.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "../game/track.h"   // MAX_SPEED, M (vitesse du jeu ↔ m/s, comme Course::bump)
#include "sfx_synth.h"       // noiseHit : coup sourd actuel (référence de niveau, mêmes réglages que sfx.cpp)

namespace wfs::sonchocs {
namespace {

constexpr float PI_F = 3.14159265358979f, PI2 = 6.28318530718f;
constexpr float FEUILLES = 0.35f;         // feuillage : crête par rapport au tronc [à vérifier]
constexpr float FEUILLES_ATTAQUE = 0.015f;   // s : les feuilles se mettent à trembler
constexpr float FLUTTER_HZ = 12.f;        // feuilles qui battent (modulation lente du bruit)
constexpr float ECLATS_DUREE = 0.3f;      // s : éclats tombés au sol (0,2 à 0,4 s, valeur du PO [à vérifier])
constexpr float ECLATS_MIN = 2000.f, ECLATS_MAX = 8000.f;   // Hz : grains de 2 à 8 kHz (valeur du PO [à vérifier])
constexpr float RUGOSITE = 0.3f;          // part de bruit dans la force du contact (froissement de la tôle, écorce) [à vérifier]

// t60 d'un mode d'une matière de facteur de perte eta (RG-02)
constexpr Mode md(float f, float a, float eta) { return {f, a, 2.2f / (f * eta)}; }

// RG-02 : modes de référence de chaque famille (fréquences et amplitudes [à vérifier] ; décroissances : facteurs de perte sourcés)
// poteau : mât d'éclairage en tube d'acier de 120 mm, 3,5 mm d'épaisseur, 6 m (EN 40 [à vérifier]) : flexion du mât encastré
// (βL = 14,14 ; 17,28 ; 20,42 → 188, 281, 393 Hz) et ovalisation de la paroi (n = 2, 3, 4 → 647, 1 829, 3 507 Hz) ;
// voir references/reel/son/chocs.md
const Matiere MATIERES[NB_FAMILLES] = {
    {"poteau", {md(188, 0.55f, ETA_ACIER_MAT), md(281, 0.45f, ETA_ACIER_MAT), md(393, 0.40f, ETA_ACIER_MAT), md(647, 1.0f, ETA_ACIER_MAT),
                md(1829, 0.6f, ETA_ACIER_MAT), md(3507, 0.35f, ETA_ACIER_MAT)}, 6, 0.00025f, 1.6f},
    {"arbre", {md(310, 1.0f, ETA_BOIS), md(520, 0.6f, ETA_BOIS), md(830, 0.35f, ETA_BOIS)}, 3, 0.0005f, 1.0f},
    // mobilier : tôle mince (banc, poubelle, abribus : plus aiguë que la carrosserie) et lattes de bois
    {"mobilier", {md(480, 1.0f, ETA_ACIER_TOLE), md(810, 0.7f, ETA_ACIER_TOLE), md(1350, 0.5f, ETA_ACIER_TOLE), md(2200, 0.3f, ETA_ACIER_TOLE),
                  md(300, 0.6f, ETA_BOIS), md(760, 0.4f, ETA_BOIS)}, 6, 0.00025f, 0.45f},
    {"béton", {{70, 1.0f, T60_BETON}, {120, 0.7f, T60_BETON}, {190, 0.4f, T60_BETON}}, 3, 0.004f, 0.2f},
    {"voiture", {md(290, 1.0f, ETA_ACIER_TOLE), md(470, 0.7f, ETA_ACIER_TOLE), md(760, 0.5f, ETA_ACIER_TOLE), md(1200, 0.35f, ETA_ACIER_TOLE),
                 md(380, 0.6f, ETA_PLASTIQUE), md(950, 0.4f, ETA_PLASTIQUE)}, 6, 0.00033f, 0.5f},
};

} // namespace

// ---------------------------------------------------------------- RG-01 à RG-03 (calcul pur)
int famillePour(chocs::Genre g) {
    switch (g) {
        case chocs::Genre::Poteau: return POTEAU;
        case chocs::Genre::Arbre: return ARBRE;
        case chocs::Genre::Mobilier: return MOBILIER;
        case chocs::Genre::Mur: case chocs::Genre::Batiment: case chocs::Genre::Ilot: return BETON;
        case chocs::Genre::Voiture: return VOITURE;
        default: return AUCUNE;
    }
}
const char* nomFamille(int f) { return f >= 0 && f < NB_FAMILLES ? MATIERES[f].nom : "aucune"; }
const Matiere& matiere(int f) { return MATIERES[std::clamp(f, 0, NB_FAMILLES - 1)]; }
float t60Retenu(int f) {
    const Matiere& m = matiere(f);
    float t = 0;
    for (int k = 0; k < m.n; k++) t = std::max(t, m.modes[k].t60);
    return t;
}
float dureeContact(int f, float v) { return matiere(f).contact30 * std::pow(V_REF / std::clamp(v, V_MIN, 2 * V_MAX), 0.2f); }

// ---------------------------------------------------------------- RG-04 (calcul pur)
namespace {
void coins(const Rect& r, double cx[4], double cz[4]) {
    const double fx = std::sin(r.cap), fz = -std::cos(r.cap), dx = std::cos(r.cap), dz = std::sin(r.cap);
    for (int i = 0; i < 4; i++) {
        const double a = (i & 1 ? 1 : -1) * r.hl, b = (i & 2 ? 1 : -1) * r.hw;
        cx[i] = r.x + a * fx + b * dx; cz[i] = r.z + a * fz + b * dz;
    }
}
} // namespace

double distance(const Rect& a, const Rect& b, double& qx, double& qz) {
    double best = 1e30, ax[4], az[4], bx[4], bz[4];
    coins(a, ax, az); coins(b, bx, bz);
    bool dedans = false;
    for (int i = 0; i < 4; i++) {   // sommets de a contre b : le point de b est le plus proche du sommet
        double rx, rz; bool in;
        chocs::plusProche(b.x, b.z, b.cap, b.hl, b.hw, ax[i], az[i], rx, rz, in);
        const double d = std::hypot(rx - ax[i], rz - az[i]);
        dedans = dedans || in;
        if (d < best) { best = d; qx = rx; qz = rz; }
    }
    for (int i = 0; i < 4; i++) {   // sommets de b contre a : le sommet de b est le point le plus proche
        double rx, rz; bool in;
        chocs::plusProche(a.x, a.z, a.cap, a.hl, a.hw, bx[i], bz[i], rx, rz, in);
        const double d = std::hypot(rx - bx[i], rz - bz[i]);
        dedans = dedans || in;
        if (d < best) { best = d; qx = bx[i]; qz = bz[i]; }
    }
    return dedans ? -best : best;
}

double distance(const Rect& v, const chocs::Obstacle& o, double& qx, double& qz) {
    if (o.forme == chocs::Forme::Cylindre) {
        double rx, rz; bool in;
        chocs::plusProche(v.x, v.z, v.cap, v.hl, v.hw, o.x, o.z, rx, rz, in);
        const double ex = rx - o.x, ez = rz - o.z, d = std::hypot(ex, ez);
        if (in || d < 1e-9) { qx = o.x; qz = o.z; return -o.r; }
        qx = o.x + ex / d * o.r; qz = o.z + ez / d * o.r;   // point du poteau tourné vers la voiture
        return d - o.r;
    }
    // boîte : axe x = (cos angle, sin angle), demi-longueur hx ; c'est la « droite » d'un rectangle de cap = angle
    Rect b; b.x = o.x; b.z = o.z; b.cap = o.angle; b.hl = o.hz; b.hw = o.hx;
    return distance(v, b, qx, qz);
}

float azimut(const Rect& v, double qx, double qz) {
    const double lx = qx - v.x, lz = qz - v.z;
    return (float)std::atan2(lx * std::cos(v.cap) + lz * std::sin(v.cap), lx * std::sin(v.cap) - lz * std::cos(v.cap));
}

// ---------------------------------------------------------------- build (thread du jeu)
void SonsChocs::build(float rate) {
    pret_.store(false, std::memory_order_release);
    rate_ = rate;
    {   // RG-05 : crête du coup sourd actuel à 30 km/h (sfx.cpp : noiseHit(0,3 s, 200 Hz, Q 0,8, 1, balayage 70 Hz), gain 0,4 + impact)
        const std::vector<float> c = coupSourd(rate, false);
        float p = 0; for (float x : c) p = std::max(p, std::fabs(x));
        const float impact30 = V_REF / (float)(MAX_SPEED * M);
        cible30_ = p * (0.4f + impact30) * std::pow(10.f, ECART_CRETE_DB / 20.f);
    }
    for (int f = 0; f < NB_FAMILLES; f++) {
        const Matiere& m = MATIERES[f];
        for (int v = 0; v < VARIANTES; v++) {
            Variante& V = var_[f][v];
            V.graine = 0x9E3779B9u * (uint32_t)(f * VARIANTES + v + 1) ^ 0x5bd1e995u;
            uint32_t h = V.graine;
            auto u = [&]() { h ^= h << 13; h ^= h >> 17; h ^= h << 5; return (float)(h >> 8) * (1.f / 16777216.f); };
            V.n = m.n;
            for (int k = 0; k < m.n; k++) {   // RG-03 : variante : fréquences ± 5 % ; la décroissance suit 1 / f (T60 = 2,2 / (f · η))
                const float x = 1 + 0.05f * (2 * u() - 1);
                V.modes[k] = {std::min(m.modes[k].f * x, 0.45f * rate), m.modes[k].a, f == BETON ? m.modes[k].t60 : m.modes[k].t60 / x};
            }
            for (int j = 0; j < ECLATS_N; j++) {   // éclats (accident, voiture) : verre et plastique des feux, tombés en 0,3 s
                const float fe = std::min(0.45f * rate, ECLATS_MIN + (ECLATS_MAX - ECLATS_MIN) * u()), tt = u(), a = 0.3f + 0.7f * u();
                const float t60 = t60Mode(fe, j % 2 ? ETA_PLASTIQUE : ETA_VERRE), w = PI2 * fe / rate, r = std::exp(-6.9077553f / (t60 * rate));
                Eclat& e = V.eclats[j];
                e.debut = (int)(ECLATS_DUREE * tt * tt * rate); e.fin = e.debut + (int)(t60 * rate);
                e.c1 = 2 * r * std::cos(w); e.c2 = r * r; e.b = ECLATS * a * std::sin(w);   // crête a × ECLATS (× cible30 / norm au jeu)
            }
            std::sort(V.eclats, V.eclats + ECLATS_N, [](const Eclat& a, const Eclat& b) { return a.debut < b.debut; });
            V.norm = 1;
        }
    }
    for (Voix& x : voix_) x = Voix{};
    // feuillage : passe-bande de 2 à 8 kHz (centre 4 kHz, Q 0,67)
    {
        const float w = PI2 * std::min(4000.f, 0.4f * rate) / rate, c = std::cos(w), al = std::sin(w) / (2 * 0.67f), a0 = 1 + al;
        bpB0_ = al / a0; bpB2_ = -al / a0; bpA1_ = -2 * c / a0; bpA2_ = (1 - al) / a0;
        decFeuilles_ = std::exp(-6.9077553f / (T60_FEUILLES * rate)); attaqueFeuilles_ = std::max(1.f, FEUILLES_ATTAQUE * rate);
    }
    // normalisation : chaque variante rendue une fois à 30 km/h (même synthèse que le jeu), crête ramenée à cible30
    for (int f = 0; f < NB_FAMILLES; f++)
        for (int v = 0; v < VARIANTES; v++) {
            Voix x; demarrer(x, {f, v, V_REF, 1, 1, false});
            x.amp = 1;
            float p = 0, y[BLOC_SYNTH];
            while (x.t < x.fin) { const int n0 = x.t; bloc(x, y, BLOC_SYNTH); for (int i = 0; i < x.t - n0; i++) p = std::max(p, std::fabs(y[i])); }
            crete30_[f][v] = p;
            var_[f][v].norm = p > 1e-9f ? cible30_ / p : 0.f;
        }
    for (int& d : derniere_) d = -1;
    tete_.store(0); queue_.store(0);
    pret_.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------- choc (thread du jeu)
int SonsChocs::choc(int f, float v, float panG, float panD, bool accident) {
    if (f < 0 || f >= NB_FAMILLES || !pret_.load(std::memory_order_acquire)) return -1;
    hasard_ ^= hasard_ << 13; hasard_ ^= hasard_ >> 17; hasard_ ^= hasard_ << 5;
    // RG-03 : variante au hasard, jamais deux fois la même de suite
    const int var = derniere_[f] < 0 ? (int)(hasard_ % VARIANTES) : (derniere_[f] + 1 + (int)(hasard_ % (VARIANTES - 1))) % VARIANTES;
    derniere_[f] = var;
    const uint32_t t = tete_.load(std::memory_order_relaxed);
    if (t - queue_.load(std::memory_order_acquire) >= FILE) return var;   // file pleine (pas de carte son) : ignoré
    file_[t % FILE] = {f, var, v, panG, panD, accident};
    tete_.store(t + 1, std::memory_order_release);
    return var;
}

// ---------------------------------------------------------------- synthèse (thread audio, aussi utilisée par build, banc et tests)
void SonsChocs::demarrer(Voix& x, const Demande& d) {
    const Variante& V = var_[d.f][d.variante];
    x = Voix{};
    x.active = true; x.accident = d.accident && d.f == VOITURE; x.f = d.f; x.v = d.variante;
    x.amp = V.norm * gainVitesse(d.v); x.gG = d.gG; x.gD = d.gD;
    x.nc = std::max(1, (int)std::lround(dureeContact(d.f, d.v) * rate_));   // RG-03 : contact de Hertz (force en demi-sinus)
    x.aire = PI_F / (2.f * (float)x.nc);                                       // impulsion d'aire 1, quelle que soit la durée
    x.fin = (int)(MATIERES[d.f].duree * rate_);
    x.n = V.n;
    for (int k = 0; k < V.n; k++) {   // résonateur à deux pôles : réponse a · r^n · sin((n + 1) w), r = 10^(−3 / (T60 · fe))
        const float w = PI2 * V.modes[k].f / rate_, r = std::exp(-6.9077553f / (V.modes[k].t60 * rate_));
        x.r[k] = {2 * r * std::cos(w), r * r, V.modes[k].a * std::sin(w), 0, 0};
    }
    x.bruit = V.graine | 1u;
    x.flutter = 0.5f;
}

// n trames de la voix (avant amp et master) dans y ; mode par mode sur le bloc (moins de branchements qu'échantillon par échantillon)
void SonsChocs::bloc(Voix& x, float* y, int n) {
    auto bruit = [&]() { x.bruit ^= x.bruit << 13; x.bruit ^= x.bruit >> 17; x.bruit ^= x.bruit << 5; return (float)(x.bruit >> 8) * (2.f / 16777216.f) - 1; };
    n = std::min(n, x.fin - x.t);
    if (n <= 0) return;
    float e[BLOC_SYNTH] = {};
    const int nE = std::clamp(x.nc - x.t, 0, n);   // force du contact (demi-sinus de Hertz) dans ce bloc
    for (int i = 0; i < nE; i++) e[i] = x.aire * std::sin(PI_F * ((float)(x.t + i) + 0.5f) / (float)x.nc) * (1 + RUGOSITE * bruit());
    for (int i = 0; i < n; i++) y[i] = 0;
    // modes deux par deux : deux récurrences indépendantes dans la même boucle (le processeur les calcule en parallèle)
    int k = 0;
    for (; k + 1 < x.n; k += 2) {
        Resonateur &r = x.r[k], &q = x.r[k + 1];
        float a1 = r.y1, a2 = r.y2, b1 = q.y1, b2 = q.y2;
        const float ra = r.c1, rb = r.c2, rc = r.b, qa = q.c1, qb = q.c2, qc = q.b;
        int i = 0;
        for (; i < nE; i++) { const float s = ra * a1 - rb * a2 + rc * e[i], u = qa * b1 - qb * b2 + qc * e[i]; a2 = a1; a1 = s; b2 = b1; b1 = u; y[i] += s + u; }
        for (; i < n; i++) { const float s = ra * a1 - rb * a2, u = qa * b1 - qb * b2; a2 = a1; a1 = s; b2 = b1; b1 = u; y[i] += s + u; }
        r.y1 = a1; r.y2 = a2; q.y1 = b1; q.y2 = b2;
    }
    for (; k < x.n; k++) {
        Resonateur& r = x.r[k];
        float y1 = r.y1, y2 = r.y2;
        const float c1 = r.c1, c2 = r.c2, b = r.b;
        int i = 0;
        for (; i < nE; i++) { const float s = c1 * y1 - c2 * y2 + b * e[i]; y2 = y1; y1 = s; y[i] += s; }
        for (; i < n; i++) { const float s = c1 * y1 - c2 * y2; y2 = y1; y1 = s; y[i] += s; }
        r.y1 = y1; r.y2 = y2;
    }
    if (x.f == ARBRE) {   // feuillage qui tremble : bruit de 2 à 8 kHz, montée en 15 ms, T60 de 0,8 s, battement à 12 Hz
        const float kF = PI2 * FLUTTER_HZ / rate_;
        for (int i = 0; i < n; i++) {
            const float b = bruit();
            const float bp = bpB0_ * b + bpB2_ * x.bpX2 - bpA1_ * x.bpY1 - bpA2_ * x.bpY2;
            x.bpX2 = x.bpX1; x.bpX1 = b; x.bpY2 = x.bpY1; x.bpY1 = bp;
            x.flutter += (0.5f + 0.5f * bruit() - x.flutter) * kF;
            x.envF *= decFeuilles_;
            y[i] += FEUILLES * std::min(1.f, (float)(x.t + i) / attaqueFeuilles_) * x.envF * (0.4f + 1.2f * x.flutter) * bp;
        }
    }
    if (x.accident) {   // éclats de verre et de plastique des feux (accident contre une voiture) : un résonateur par éclat
        const Variante& V = var_[x.f][x.v];
        const float kE = cible30_ / std::max(1e-9f, V.norm);   // crête d'un éclat : ECLATS × celle de la tôle (puis × amp, comme le reste)
        for (int j = 0; j < ECLATS_N && V.eclats[j].debut < x.t + n; j++) {   // triés par début
            const Eclat& c = V.eclats[j];
            const int i0 = std::max(0, c.debut - x.t), i1 = std::min(n, c.fin - x.t);
            float y1 = x.ey1[j], y2 = x.ey2[j];
            for (int i = i0; i < i1; i++) { const float s = c.c1 * y1 - c.c2 * y2 + (x.t + i == c.debut ? c.b * kE : 0.f); y2 = y1; y1 = s; y[i] += s; }
            x.ey1[j] = y1; x.ey2[j] = y2;
        }
    }
    x.t += n;
}

// ---------------------------------------------------------------- mix (thread audio)
void SonsChocs::mix(float* G, float* D, float* bus, int n, float master) {
    if (!pret_.load(std::memory_order_acquire) || n <= 0) return;
    uint32_t q = queue_.load(std::memory_order_relaxed);
    const uint32_t t = tete_.load(std::memory_order_acquire);
    for (; q != t; q++) {   // nouveaux chocs → voix libres (sinon la plus avancée est reprise)
        int libre = 0, loin = -1;
        for (int i = 0; i < VOIX; i++) { if (!voix_[i].active) { libre = i; break; } if (voix_[i].t > loin) { loin = voix_[i].t; libre = i; } }
        demarrer(voix_[libre], file_[q % FILE]);
    }
    queue_.store(q, std::memory_order_release);
    for (Voix& x : voix_) {
        if (!x.active) continue;
        const float kG = x.amp * x.gG * master, kD = x.amp * x.gD * master, kB = 0.5f * (kG + kD);
        for (int a = 0; a < n && x.t < x.fin; a += BLOC_SYNTH) {
            float y[BLOC_SYNTH];
            const int t0 = x.t;
            bloc(x, y, std::min(BLOC_SYNTH, n - a));
            for (int i = 0; i < x.t - t0; i++) { G[a + i] += y[i] * kG; D[a + i] += y[i] * kD; bus[a + i] += y[i] * kB; }   // RG-05 : écho du tunnel, comme WFS-272
        }
        if (x.t >= x.fin) x.active = false;
    }
}

// ---------------------------------------------------------------- mesures (banc, tests)
float niveauTiers(const std::vector<float>& x, size_t a, size_t n, float rate, float f) {
    const float w = PI2 * f / rate, c = std::cos(w), al = std::sin(w) / (2 * 4.3f), a0 = 1 + al;
    const float b0 = al / a0, b2 = -al / a0, a1 = -2 * c / a0, a2 = (1 - al) / a0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0; double e = 0;
    for (size_t i = a; i < a + n && i < x.size(); i++) { const float y = b0 * x[i] + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x[i]; y2 = y1; y1 = y; e += (double)y * y; }
    return 10.f * (float)std::log10(std::max(1e-24, e / (double)std::max<size_t>(1, n)));
}

float mesurerT60(const std::vector<float>& x, float rate, float fBande) {
    std::vector<float> y(x);
    if (fBande > 0) {   // tiers d'octave (Q 4,3), passé deux fois : la bande garde le mode, le filtre s'éteint vite
        for (int passe = 0; passe < 2; passe++) {
            const float w = PI2 * fBande / rate, c = std::cos(w), al = std::sin(w) / (2 * 4.3f), a0 = 1 + al;
            const float b0 = al / a0, b2 = -al / a0, a1 = -2 * c / a0, a2 = (1 - al) / a0;
            float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            for (float& v : y) { const float o = b0 * v + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = v; y2 = y1; y1 = o; v = o; }
        }
    }
    // courbe de décroissance de l'énergie (intégration inverse de Schroeder, insensible aux battements des modes), à partir de la
    // crête ; droite des moindres carrés de −5 à −35 dB (points tous les 0,5 ms)
    size_t ip = 0; for (size_t i = 0; i < y.size(); i++) if (std::fabs(y[i]) > std::fabs(y[ip])) ip = i;
    std::vector<double> cum(y.size() - ip + 1, 0.0);
    for (size_t i = y.size(); i-- > ip;) cum[i - ip] = cum[i - ip + 1] + (double)y[i] * y[i];
    if (cum[0] <= 0) return 0;
    const size_t P = std::max<size_t>(1, (size_t)(0.0005f * rate));
    double sx = 0, sy = 0, sxx = 0, sxy = 0; int n = 0;
    for (size_t i = 0; i < cum.size(); i += P) {
        const double l = 10 * std::log10(std::max(1e-30, cum[i] / cum[0]));
        if (l > -5) continue;
        if (l < -35) break;
        const double xi = (double)i / rate; sx += xi; sy += l; sxx += xi * xi; sxy += xi * l; n++;
    }
    if (n < 3) return 0;
    const double pente = (n * sxy - sx * sy) / std::max(1e-18, n * sxx - sx * sx);   // dB par seconde (< 0)
    return pente < 0 ? (float)(-60.0 / pente) : 0.f;
}

std::vector<float> coupSourd(float rate, bool accident) {
    std::vector<float> b;
    if (accident) { sfx::noiseHit(b, 0, rate, 0.6f, 140, 0.7f, 1, 45); sfx::noiseHit(b, 0, rate, 0.4f, 2600, 1.4f, 0.55f, 7000); sfx::noiseHit(b, 0, rate, 0.25f, 900, 2, 0.4f, 300); }
    else sfx::noiseHit(b, 0, rate, 0.3f, 200, 0.8f, 1, 70);
    return b;
}

} // namespace wfs::sonchocs
