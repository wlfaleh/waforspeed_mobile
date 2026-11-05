// WaForSpeed — feux arrière des modèles 3D (WFS-109, épic E4).
//   - RG-13 : découpe des feux arrière au chargement. Certains modèles (Golf, 911…) n'ont qu'un seul matériau de feux,
//     émissif blanc, pour l'avant et l'arrière : leurs triangles situés au-delà de z = 0,4 L (arrière vers +z) forment une
//     pièce à part, marquée feu arrière, dessinée comme un feu MAT_TAIL (rouge, allumé au freinage, de jour comme de nuit).
//   - RG-03 : 3e feu stop (petite pièce MAT_TAIL centrée, en haut de la lunette) : allumé au freinage seulement.
//   - RG-02 : transition douce (≈ 80 ms) entre feu éteint / de position et feu stop.
// Calcul CPU seul, sans Vulkan (testé par tests/test_feux_modele.cpp) ; le banc d'essai des feux est dans feux_modele.cpp.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs {
class Game; class Renderer;
// WFS-109 (RG-15) : captures feux-frein-jour, feux-frein-nuit, feux-recul (voiture du joueur, caméra arrière)
void benchFeuxFrein(Game& g, Renderer& r, const std::string& out);
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
struct Resultat { int materiau = -1; uint32_t triangles = 0; vec3 bmin{1e9f, 1e9f, 1e9f}, bmax{-1e9f, -1e9f, -1e9f}; int troisieme = -1; };

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
