// WaForSpeed — mesure des voitures comme une fiche constructeur (WFS-158, épic E10, ST-1 « audit »).
// WFS-148 mesure la boîte englobante du .wfm : une antenne, un rétroviseur ou quelques sommets égarés suffisent à la fausser
// (Abarth : antenne de toit → 1,84 m de haut pour 1,485 m réels ; la largeur « sans rétroviseurs » prend alors les
// rétroviseurs, car son seuil de 55 % est calculé sur cette hauteur fausse). Ici, chaque dimension est l'endroit où la
// voiture présente une vraie surface : on range les triangles du plus extrême au moins extrême et on retient la position
// à laquelle leur surface projetée cumulée atteint SURFACE_MIN (une antenne ou un fil n'a presque pas de surface).
// La longueur hors tout reste celle de la boîte (pare-chocs à pare-chocs, plaques comprises : rien de fin ne dépasse en long,
// et la conversion met déjà chaque modèle à la longueur du catalogue). Roues (empattement, voies, diamètres) : roues::decouper
// (WFS-60), comme WFS-148. Sol : bas des pneus.
// En-tête seul, calcul sur le processeur, sans GPU : utilisé par tests/fidelite_voitures.cpp.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../game/roues.h"

namespace wfs::vehicule {

// ---- réglages de la mesure (justifiés dans les commentaires ; à changer seulement avec un ticket E10)
constexpr double SURFACE_MIN = 0.05;      // m² : 500 cm², moins d'1 % d'un toit (≈ 1,8 m²), plus qu'une antenne ou un aileron de requin (≈ 100 cm²)
constexpr double SEUIL_RETRO = 0.55;      // largeur mesurée sous 55 % de la hauteur : les rétroviseurs sont à hauteur de vitre (WFS-148)
constexpr double SURFACE_MIN_BAS = 0.005; // m² : 50 cm² pour le point bas de la caisse (garde au sol : le point le plus bas compte)
constexpr double TOL_PNEU_M = 0.01;       // RG-02 : un pneu est posé si son bas est à ±1 cm du sol

// ---- lecture d'un .wfm (format de tools/convert_car.mjs, version 1 : sommets x y z nx ny nz u v, en mètres, avant vers −Z)
struct ModeleWfm {
    uint32_t version = 0;
    float boite[6] = {};                               // min x, y, z puis max x, y, z (en-tête du fichier)
    std::vector<std::vector<float>> sommets;           // par maillage (un maillage par matériau)
    std::vector<std::vector<uint32_t>> indices;
    std::vector<uint32_t> drapeaux;                    // drapeaux du matériau de chaque maillage (MatFlags ; 128 = roue)
};

// lecture bornée : un fichier tronqué ou d'un autre format rend false et une raison, jamais un plantage
inline bool lireWfm(const std::string& chemin, ModeleWfm& m, std::string& erreur) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) { erreur = "fichier illisible"; return false; }
    const std::string b((std::istreambuf_iterator<char>(f)), {});
    size_t o = 0;
    auto u32 = [&](uint32_t& x) { if (o + 4 > b.size()) return false; std::memcpy(&x, &b[o], 4); o += 4; return true; };
    if (b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) != 0) { erreur = "format inconnu"; return false; }
    o = 4;
    uint32_t nT, nM, nMe;
    if (!u32(m.version) || !u32(nT) || !u32(nM) || !u32(nMe)) { erreur = "en-tête tronqué"; return false; }
    std::memcpy(m.boite, &b[o], 24); o += 24;
    for (uint32_t t = 0; t < nT; t++) { uint32_t n; if (!u32(n) || o + n > b.size()) { erreur = "texture tronquée"; return false; } o += n; }
    std::vector<uint32_t> drapeauxMat;
    for (uint32_t i = 0; i < nM; i++) {
        uint32_t d;
        if (o + 52 > b.size()) { erreur = "matériau tronqué"; return false; }
        o += 52;
        if (!u32(d)) { erreur = "matériau tronqué"; return false; }
        drapeauxMat.push_back(d);
    }
    for (uint32_t k = 0; k < nMe; k++) {
        uint32_t mi, nv, ni;
        if (!u32(mi) || !u32(nv) || !u32(ni) || mi >= drapeauxMat.size() || o + (size_t)nv * 32 + (size_t)ni * 4 > b.size()) { erreur = "maillage tronqué"; return false; }
        m.sommets.emplace_back((size_t)nv * 8); std::memcpy(m.sommets.back().data(), &b[o], (size_t)nv * 32); o += (size_t)nv * 32;
        m.indices.emplace_back(ni); std::memcpy(m.indices.back().data(), &b[o], (size_t)ni * 4); o += (size_t)ni * 4;
        for (uint32_t i : m.indices.back()) if (i >= nv) { erreur = "indice hors du maillage"; return false; }
        m.drapeaux.push_back(drapeauxMat[mi]);
    }
    return true;
}

// ---- mesures
struct Dimensions {
    double longueurBoite = 0, largeurBoite = 0, hauteurBoite = 0;   // boîte englobante (méthode de WFS-148), m
    double longueur = 0, largeur = 0, hauteur = 0;                   // comme une fiche : hors tout, sans rétroviseurs, hors antenne, m
    double hauteurBrute = 0;                                         // plus haut sommet au-dessus du sol (antenne comprise), m
    bool roues = false;
    double empattement = 0, voieAv = 0, voieAr = 0, diametreAv = 0, diametreAr = 0;   // m
    double sol = 0;              // y du sol = bas des pneus (le plus bas des 4), m
    double basPneus[4] = {};     // y du bas de chaque pneu (LF, RF, LB, RB), m
    double ecartPneuMax = 0;     // RG-02 : plus grand écart d'un bas de pneu au sol (0 = 4 pneus posés), m
    double ecartSolBoite = 0;    // y du sol − bas de la boîte (> 0 : quelque chose dépasse sous les pneus), m
    double gardeAuSol = 0;       // point bas de la caisse entre les essieux, au-dessus du sol, m
};

namespace detail {
struct Tri { double cle; double surface; };
// position où la surface cumulée des triangles rangés par « cle » décroissante atteint smin (ou la dernière s'il n'y en a pas assez)
inline double seuilSurface(std::vector<Tri>& t, double smin) {
    std::sort(t.begin(), t.end(), [](const Tri& a, const Tri& b) { return a.cle > b.cle; });
    double s = 0;
    for (const Tri& x : t) { s += x.surface; if (s >= smin) return x.cle; }
    return t.empty() ? 0 : t.back().cle;
}
// surface d'un triangle projeté sur un plan (axes a et b : 0 = x, 1 = y, 2 = z)
inline double surfaceProjetee(const float* p0, const float* p1, const float* p2, int a, int b) {
    return 0.5 * std::fabs((double)(p1[a] - p0[a]) * (p2[b] - p0[b]) - (double)(p2[a] - p0[a]) * (p1[b] - p0[b]));
}
} // namespace detail

inline Dimensions mesurer(const ModeleWfm& m) {
    using detail::Tri;
    Dimensions d;
    const float* bb = m.boite;
    d.longueurBoite = bb[5] - bb[2]; d.largeurBoite = bb[3] - bb[0]; d.hauteurBoite = bb[4] - bb[1];
    // roues (WFS-60) : centre, rayon, et quels triangles appartiennent à une roue
    std::vector<roues::Maillage> ms;
    for (size_t k = 0; k < m.sommets.size(); k++)
        ms.push_back({m.sommets[k].data(), 8, (uint32_t)(m.sommets[k].size() / 8), m.indices[k].data(), (uint32_t)m.indices[k].size(), (m.drapeaux[k] & 128) != 0});
    const roues::Decoupe r = roues::decouper(ms);
    d.sol = bb[1];
    if (r.n == 4) {
        d.roues = true;
        const auto& w = r.roues;
        d.empattement = 0.5 * ((w[roues::LB].centre.z + w[roues::RB].centre.z) - (w[roues::LF].centre.z + w[roues::RF].centre.z));
        d.voieAv = std::fabs(w[roues::RF].centre.x - w[roues::LF].centre.x); d.voieAr = std::fabs(w[roues::RB].centre.x - w[roues::LB].centre.x);
        d.diametreAv = w[roues::LF].rayon + w[roues::RF].rayon; d.diametreAr = w[roues::LB].rayon + w[roues::RB].rayon;
        double sol = 1e9;
        for (int i = 0; i < 4; i++) { d.basPneus[i] = w[i].centre.y - w[i].rayon; sol = std::min(sol, d.basPneus[i]); }
        d.sol = sol;
        for (int i = 0; i < 4; i++) d.ecartPneuMax = std::max(d.ecartPneuMax, d.basPneus[i] - sol);
    }
    d.ecartSolBoite = d.sol - bb[1];
    const double cx = 0.5 * (bb[0] + bb[3]);
    // hauteur : triangles rangés par leur point le plus haut, surface vue de dessus (plan x-z)
    std::vector<Tri> haut, cote, bas;
    double yMax = bb[1];
    for (size_t k = 0; k < m.sommets.size(); k++) {
        const float* v = m.sommets[k].data(); const auto& I = m.indices[k];
        for (size_t t = 0; t + 2 < I.size(); t += 3) {
            const float *a = v + 8 * I[t], *b = v + 8 * I[t + 1], *c = v + 8 * I[t + 2];
            yMax = std::max({yMax, (double)a[1], (double)b[1], (double)c[1]});
            haut.push_back({std::max({a[1], b[1], c[1]}) * 1.0, detail::surfaceProjetee(a, b, c, 0, 2)});
        }
    }
    d.hauteurBrute = yMax - d.sol;
    d.hauteur = detail::seuilSurface(haut, SURFACE_MIN) - d.sol;
    d.longueur = d.longueurBoite;
    // largeur sans rétroviseurs : triangles entièrement sous 55 % de la hauteur (mesurée ci-dessus), surface vue de côté (plan y-z)
    const double ySeuil = d.sol + SEUIL_RETRO * d.hauteur;
    // garde au sol : triangles de la caisse (hors roues) entre les deux essieux et entre les roues, surface vue de dessous
    const double zAv = r.n == 4 ? 0.5 * (r.roues[roues::LF].centre.z + r.roues[roues::RF].centre.z) : bb[2];
    const double zAr = r.n == 4 ? 0.5 * (r.roues[roues::LB].centre.z + r.roues[roues::RB].centre.z) : bb[5];
    const double xIn = r.n == 4 ? 0.5 * std::min(d.voieAv, d.voieAr) - 0.15 : 0.5 * d.largeurBoite;   // 15 cm : demi-largeur d'un pneu
    for (size_t k = 0; k < m.sommets.size(); k++) {
        const float* v = m.sommets[k].data(); const auto& I = m.indices[k];
        const bool aDecoupe = r.n == 4 && k < r.roueDuTriangle.size() && !r.roueDuTriangle[k].empty();
        for (size_t t = 0; t + 2 < I.size(); t += 3) {
            const float *a = v + 8 * I[t], *b = v + 8 * I[t + 1], *c = v + 8 * I[t + 2];
            if (std::max({a[1], b[1], c[1]}) < ySeuil)
                cote.push_back({std::max({std::fabs(a[0] - cx), std::fabs(b[0] - cx), std::fabs(c[0] - cx)}), detail::surfaceProjetee(a, b, c, 1, 2)});
            const bool roue = (m.drapeaux[k] & 128) != 0 || (aDecoupe && r.roueDuTriangle[k][t / 3] >= 0);
            const double zc = (a[2] + b[2] + c[2]) / 3.0, xc = std::fabs((a[0] + b[0] + c[0]) / 3.0 - cx);
            if (!roue && zc > zAv && zc < zAr && xc < xIn) bas.push_back({-(double)std::min({a[1], b[1], c[1]}), detail::surfaceProjetee(a, b, c, 0, 2)});
        }
    }
    d.largeur = 2 * detail::seuilSurface(cote, SURFACE_MIN);
    d.gardeAuSol = bas.empty() ? 0 : -detail::seuilSurface(bas, SURFACE_MIN_BAS) - d.sol;
    return d;
}

// ---- diagnostic de l'audit (ST-1) : ce qu'une correction permise par RG-01 (échelle globale, position des roues) peut faire
// valeur du jeu, bornes réelles (tolérance comprise) et valeur nominale (valeur de la fiche, ou milieu de l'intervalle)
struct Ecart { double jeu = 0, bas = 0, haut = 0, nominal = 0; bool connu = false; };
// écart relatif signé à l'intervalle [bas, haut] (0 dedans)
inline double ecartRelatif(const Ecart& e) {
    if (!e.connu) return 0;
    if (e.jeu < e.bas) return e.jeu / e.bas - 1;
    if (e.jeu > e.haut) return e.jeu / e.haut - 1;
    return 0;
}
// meilleure échelle globale pour la carrosserie (longueur, largeur, hauteur) : facteur s qui met le plus de dimensions dans
// leur tolérance, puis, à égalité, qui rapproche le plus les dimensions de leur valeur nominale (moindres carrés des écarts
// relatifs) ; résidu = plus grand écart relatif à la tolérance qui reste après s (0 : une échelle globale suffit, RG-01)
struct Echelle { double s = 1, residu = 0; int dedans = 0, connues = 0; };
inline Echelle meilleureEchelle(const Ecart* e, int n) {
    Echelle best; best.dedans = -1;
    double meilleurCarre = 1e18;
    for (int i = 0; i < n; i++) best.connues += e[i].connu;
    for (int k = 0; k <= 800; k++) {   // s de 0,80 à 1,20 par pas de 0,0005
        const double s = 0.80 + 0.0005 * k;
        int dedans = 0; double pire = 0, carre = 0;
        for (int i = 0; i < n; i++) if (e[i].connu) {
            Ecart x = e[i]; x.jeu *= s;
            const double r = std::fabs(ecartRelatif(x)), q = x.jeu / x.nominal - 1;
            dedans += r == 0; pire = std::max(pire, r); carre += q * q;
        }
        if (dedans > best.dedans || (dedans == best.dedans && carre < meilleurCarre)) { best.s = s; best.residu = pire; best.dedans = dedans; meilleurCarre = carre; }
    }
    return best;
}

} // namespace wfs::vehicule
