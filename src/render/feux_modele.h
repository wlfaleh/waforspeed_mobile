// WaForSpeed — feux arrière des modèles 3D (WFS-109, épic E4).
//   - RG-13 : découpe des feux arrière au chargement. Certains modèles (Golf, 911…) n'ont qu'un seul matériau de feux,
//     émissif blanc, pour l'avant et l'arrière : leurs triangles situés au-delà de z = 0,4 L (arrière vers +z) forment une
//     pièce à part, marquée feu arrière, dessinée comme un feu MAT_TAIL (rouge, allumé au freinage, de jour comme de nuit).
//   - RG-03 : 3e feu stop (petite pièce MAT_TAIL centrée, en haut de la lunette) : allumé au freinage seulement.
//   - RG-02 : transition douce (≈ 80 ms) entre feu éteint / de position et feu stop.
// Calcul CPU seul, sans Vulkan (testé par tests/test_feux_modele.cpp) ; le banc d'essai des feux est dans feux_modele.cpp.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs {
class Game; class Renderer;
// WFS-109 (RG-15) : captures feux-frein-jour, feux-frein-nuit, feux-recul (voiture du joueur, caméra arrière)
void benchFeuxFrein(Game& g, Renderer& r, const std::string& out);
// WFS-170 (RG-04, RG-08) : vitrine-<id>-feux des 20 voitures : vue arrière de trois quarts, frein tenu (feux stop allumés),
// de jour. WFS_VOITURES="id id …" (essai local) : ces voitures seulement, avec aussi leur vitrine-<id>-0.
void benchVitrinesFeux(Game& g, Renderer& r, const std::string& out);
}

namespace wfs::feux_modele {

// réglages (RG-06)
constexpr float SEUIL_Z = 0.4f;          // RG-13 : triangles au-delà de 0,4 L (derrière l'essieu arrière, ≈ 0,3 L)
constexpr float HAUTEUR_MAX = 0.5f;      // RG-13 : pas de découpe si la pièce fait plus de 0,5 m de haut (lampAt du web)
constexpr float TRANSITION_S = 0.08f;    // RG-02 : 80 ms entre feu de position et feu stop
constexpr float TROISIEME_LARGEUR = 0.6f, TROISIEME_CENTRE = 0.1f;   // RG-03 : 3e feu stop = pièce étroite et centrée

constexpr uint32_t DRAPEAU_TAIL = 4, DRAPEAU_HEAD = 8;   // MAT_TAIL / MAT_HEAD (renderer.h, format .wfm)
enum : int8_t { PIECE_NORMALE = 0, PIECE_FEU_ARRIERE = 1, PIECE_TROISIEME_FEU = 2 };

// un matériau du .wfm : drapeaux d'origine, émission, boîte de toutes ses pièces (repère du modèle)
struct Materiau { uint32_t flags = 0; vec3 emissif{}; vec3 bmin{1e9f, 1e9f, 1e9f}, bmax{-1e9f, -1e9f, -1e9f}; };
// WFS-171 (RG-03, ANO-78) : recul = centre du feu de recul dans le bloc découpé (côté +x ; l'autre côté en miroir)
struct Resultat { int materiau = -1; uint32_t triangles = 0; vec3 bmin{1e9f, 1e9f, 1e9f}, bmax{-1e9f, -1e9f, -1e9f}; int troisieme = -1;
                  vec3 recul{}; bool reculTrouve = false; };

// ---------- WFS-171 : valeurs photométriques réelles des feux (sources : règlements ONU-CEE, textes consolidés) ----------
// R7 (feux de position et stop) : feu de position R1 4 à 17 cd dans l'axe, feu stop S1 60 à 260 cd ; un feu stop
// combiné au feu de position doit être au moins 5 fois plus intense (R7 § 6). R23 (feux de recul) : 80 cd au moins
// dans l'axe, 300 cd au plus au-dessus de l'horizontale, 600 cd au plus en dessous. Ces règlements ne mesurent que
// ±10 à 15° autour de l'axe ; vers la chaussée (30 à 80° sous l'horizontale), on prend 30 % de la valeur typique de
// l'axe (estimation, diffusion du cabochon) : position 10 cd → 3 cd, stop 100 cd → 30 cd (rapport 10 ≥ 5 de R7).
constexpr float CD_POSITION_SOL = 3.f, CD_STOP_SOL = 30.f;   // intensité d'un feu vers la chaussée (cd)
constexpr float CD_RECUL_SOL = 40.f;                          // feu de recul vers la chaussée (cd, R23 : 80-600)
// Étalon de la scène : lampadaire de ville 1,1 (portée 26 m, décroissance 2) à 7,1 m → 0,58 sous le mât ≡ 20 lux
// (DA-realisme § 3 : 10 à 30 lux sur la chaussée) : 1 unité d'éclairement du moteur ≈ 35 lux
constexpr float LUX_PAR_UNITE = 35.f;
constexpr float ALBEDO_ASPHALTE = 0.10f;                      // DA-realisme § 3 : asphalte usé 0,10-0,18
constexpr float PI_F = 3.14159265f;

// RG-01 : éclairement (lux par cd) d'un point de la chaussée à s m derrière la glace du feu et u m de côté, feu à h m
// de haut : E = I(α) cos β / r², cos β = h / r (chaussée horizontale) ; I(α) = I (0,5 + 0,5 cos α), α depuis l'axe du
// feu (horizontal, vers l'arrière) : moitié diffusée par le cabochon, moitié dans l'axe. Maximum au pied du pare-chocs,
// décroissance continue (aucun bord net) ; nul devant la glace (s < 0).
inline float eclairementSol(float s, float u, float h) {
    if (s < 0) return 0;
    const float r2 = s * s + u * u + h * h, r = std::sqrt(r2);
    return (0.5f + 0.5f * s / r) * h / (r2 * r);
}
// luminance ajoutée à la chaussée (unités HDR du moteur) pour un éclairement en lux : ρ / π × E / LUX_PAR_UNITE
inline float luminanceSol(float lux) { return ALBEDO_ASPHALTE / PI_F * lux / LUX_PAR_UNITE; }

// feu MAT_TAIL accepté par le bloc WFS-09 de model_loader.cpp : bloc bas (< 0,5 m) dans le dernier tiers (z > 0,3 L)
inline bool feuArriereValide(const Materiau& m, float L) {
    return (m.flags & DRAPEAU_TAIL) && m.bmax.x >= m.bmin.x && m.bmax.y - m.bmin.y < HAUTEUR_MAX && m.bmin.z > L * 0.3f;
}
// RG-03 : 3e feu stop = feu arrière étroit et centré (giulia : matériau 40, 0,3 m de large, à 1,09 m de haut)
inline bool troisiemeFeu(const Materiau& m, float L) {
    return feuArriereValide(m, L) && m.bmax.x - m.bmin.x < TROISIEME_LARGEUR && std::abs(m.bmin.x + m.bmax.x) < TROISIEME_CENTRE * 2;
}

// RG-02 : niveau du feu stop (0 = repos, 1 = stop) après dt secondes ; montée et descente linéaires en 80 ms ;
// dt < 0 (première image) ou trop grand (voiture revue après une pause) : niveau atteint tout de suite
inline float transition(float niveau, bool stop, float dt) {
    const float cible = stop ? 1.f : 0.f;
    if (dt < 0 || dt > 0.25f) return cible;
    const float pas = dt / TRANSITION_S;
    return niveau < cible ? std::min(cible, niveau + pas) : std::max(cible, niveau - pas);
}

// WFS-171 (signalé par WFS-170) : boîte des pièces « feu arrière » marquées au chargement (correctifs de WFS-170 : V60,
// GT86, Abarth, A45) : position mesurée du feu, comme la découpe ; sans elle, le halo de repli flottait hors de la caisse
template <class Part, class Pos>
void boiteFeuxMarques(const std::vector<uint32_t>& idx, const std::vector<Part>& parts, Pos pos, vec3& bmin, vec3& bmax) {
    for (const Part& p : parts) {
        if (p.feuArriere != PIECE_FEU_ARRIERE) continue;
        for (uint32_t k = 0; k < p.indexCount; k++) {
            const vec3 q = pos((size_t)p.vertexOffset + idx[p.firstIndex + k]);
            bmin = {std::min(bmin.x, q.x), std::min(bmin.y, q.y), std::min(bmin.z, q.z)};
            bmax = {std::max(bmax.x, q.x), std::max(bmax.y, q.y), std::max(bmax.z, q.z)};
        }
    }
}
// WFS-171 : repli sans pièce de feu (S3, X5, Mustang…) : arrière de la caisse à la hauteur des feux (±6 cm), dans les
// 60 derniers cm : demi-largeur (sans les rétroviseurs, à l'avant) et surface arrière réelles, au lieu de la boîte
// entière (largeur avec rétroviseurs, pare-chocs le plus en arrière) qui posait le halo dans le vide
template <class Pos>
bool arriereALaHauteur(size_t nv, Pos pos, float y, float zArr, float& demiLargeur, float& zSurface) {
    demiLargeur = 0; zSurface = -1e9f;
    for (size_t v = 0; v < nv; v++) {
        const vec3 q = pos(v);
        if (std::abs(q.y - y) > 0.06f || q.z < zArr - 0.6f) continue;
        demiLargeur = std::max(demiLargeur, std::abs(q.x)); zSurface = std::max(zSurface, q.z);
    }
    return demiLargeur > 0.3f;
}

// RG-13 / RG-03 : découpe d'un modèle. Part : firstIndex, indexCount, vertexOffset, material, feuArriere (ModelPart) ;
// pos(v) : position du sommet v (indice absolu). Au plus une pièce de plus par pièce du matériau choisi (même matériau,
// mêmes tampons) ; un modèle qui a déjà un feu MAT_TAIL valide n'est pas découpé (son 3e feu stop est seulement marqué).
template <class Part, class Pos>
Resultat decouper(std::vector<uint32_t>& idx, std::vector<Part>& parts, Pos pos, const std::vector<Materiau>& mats, float L) {
    Resultat res;
    bool tail = false;
    for (size_t mi = 0; mi < mats.size(); mi++) {
        tail = tail || feuArriereValide(mats[mi], L);
        if (troisiemeFeu(mats[mi], L)) res.troisieme = (int)mi;
    }
    if (res.troisieme >= 0) for (Part& p : parts) if ((int)p.material == res.troisieme) p.feuArriere = PIECE_TROISIEME_FEU;
    if (tail) return res;
    auto arriere = [&](const Part& p, uint32_t t) {
        float z = 0;
        for (int j = 0; j < 3; j++) z += pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]).z;
        return z / 3 > SEUIL_Z * L;
    };
    // candidat : matériau de feux non MAT_TAIL (drapeau MAT_HEAD d'origine ou émissif) ; on garde celui qui a le plus de
    // triangles à l'arrière, si sa pièce arrière reste basse
    for (size_t mi = 0; mi < mats.size(); mi++) {
        const Materiau& m = mats[mi];
        if ((m.flags & DRAPEAU_TAIL) || !((m.flags & DRAPEAU_HEAD) || std::max({m.emissif.x, m.emissif.y, m.emissif.z}) > 0)) continue;
        Resultat r; r.materiau = (int)mi;
        for (const Part& p : parts) {
            if (p.material != mi) continue;
            for (uint32_t t = 0; t < p.indexCount / 3; t++) {
                if (!arriere(p, t)) continue;
                r.triangles++;
                for (int j = 0; j < 3; j++) {
                    const vec3 q = pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]);
                    r.bmin = {std::min(r.bmin.x, q.x), std::min(r.bmin.y, q.y), std::min(r.bmin.z, q.z)};
                    r.bmax = {std::max(r.bmax.x, q.x), std::max(r.bmax.y, q.y), std::max(r.bmax.z, q.z)};
                }
            }
        }
        if (r.triangles && r.bmax.y - r.bmin.y <= HAUTEUR_MAX && r.triangles > res.triangles) { r.troisieme = res.troisieme; res = r; }
    }
    if (res.materiau < 0) return res;
    // WFS-171 (RG-03) : feu de recul dans la partie intérieure basse du bloc (côté +x) : 30 % intérieurs de la largeur du
    // bloc, au tiers bas de leur hauteur, au ras de la glace (Golf VII : le recul est dans la partie du feu posée sur le hayon)
    float xin = 1e9f, xout = -1e9f;
    auto chaqueSommet = [&](auto&& f) {
        for (const Part& p : parts) {
            if ((int)p.material != res.materiau) continue;
            for (uint32_t t = 0; t < p.indexCount / 3; t++)
                if (arriere(p, t)) for (int j = 0; j < 3; j++) f(pos((size_t)p.vertexOffset + idx[p.firstIndex + t * 3 + j]));
        }
    };
    chaqueSommet([&](const vec3& q) { if (q.x > 0) { xin = std::min(xin, q.x); xout = std::max(xout, q.x); } });
    if (xout > xin) {
        const float lim = xin + 0.3f * (xout - xin);
        float ylo = 1e9f, yhi = -1e9f, zmax = -1e9f;
        chaqueSommet([&](const vec3& q) { if (q.x > 0 && q.x <= lim) { ylo = std::min(ylo, q.y); yhi = std::max(yhi, q.y); zmax = std::max(zmax, q.z); } });
        res.recul = {xin + 0.15f * (xout - xin), ylo + 0.3f * (yhi - ylo), zmax};
        res.reculTrouve = true;
    }
    // pièces du matériau choisi : triangles de l'avant d'un côté, de l'arrière de l'autre (indices regroupés)
    std::vector<uint32_t> idx2; idx2.reserve(idx.size());
    std::vector<Part> parts2; parts2.reserve(parts.size() + 4);
    for (const Part& p : parts) {
        const bool coupe = (int)p.material == res.materiau;
        for (int cote = 0; cote < (coupe ? 2 : 1); cote++) {
            Part q = p; q.firstIndex = (uint32_t)idx2.size();
            for (uint32_t t = 0; t < p.indexCount / 3; t++)
                if (!coupe || arriere(p, t) == (cote == 1)) for (int j = 0; j < 3; j++) idx2.push_back(idx[p.firstIndex + t * 3 + j]);
            q.indexCount = (uint32_t)idx2.size() - q.firstIndex;
            if (cote == 1) q.feuArriere = PIECE_FEU_ARRIERE;
            if (q.indexCount) parts2.push_back(q);
        }
    }
    idx.swap(idx2); parts.swap(parts2);
    return res;
}

} // namespace wfs::feux_modele
