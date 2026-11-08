#pragma once
// WaForSpeed — WFS-273 (RG-01, décision du PO du 2026-11-08 06:40 UTC) : choix du modèle de chaque arbre en exemplaire.
// Une essence a MODELES modèles de détail ; deux arbres à moins de DISTANCE_M l'un de l'autre n'ont jamais le même modèle avec
// des lacets à moins de ECART_LACET l'un de l'autre (aucun clone visible côte à côte). Pur calcul (test : tests/test_arbres_variantes.cpp).
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../core/math.h"

namespace wfs::arbres {

constexpr int MODELES = 3;                       // modèles de détail par essence (version allégée : un seul)
constexpr float DISTANCE_M = 10.f;               // voisinage où deux clones se verraient
constexpr float ECART_LACET = 30.f * 3.14159265f / 180.f;
constexpr float TOUR = 6.28318531f;

struct Arbre { vec3 pied; float lacet; uint32_t graine; int modele = 0; };

// écart d'angle (0 à π)
inline float ecartAngle(float a, float b) {
    float d = std::fmod(std::fabs(a - b), TOUR);
    return d > TOUR * 0.5f ? TOUR - d : d;
}

// clone : même modèle, à moins de DISTANCE_M (au sol), lacets à moins de ECART_LACET
inline bool clone(const Arbre& a, const Arbre& b) {
    const float dx = a.pied.x - b.pied.x, dz = a.pied.z - b.pied.z;
    return a.modele == b.modele && dx * dx + dz * dz < DISTANCE_M * DISTANCE_M && ecartAngle(a.lacet, b.lacet) < ECART_LACET;
}

// v : arbres d'une essence, dans l'ordre de la route. Pour chaque arbre, les voisins déjà choisis (grille de cases de DISTANCE_M :
// tous les voisins au sol, épingles comprises) ; on garde le premier couple (modèle, lacet) sans clone, en partant du modèle
// graine % MODELES et du lacet tiré, puis en tournant le lacet par pas de PAS_LACET (au plus près du tirage). Capacité : 3 modèles
// × 12 lacets distincts de 30° = 36 voisins à moins de 10 m. Rend le nombre d'arbres dont le lacet a été tourné.
constexpr float PAS_LACET = TOUR / 24;   // 15°
inline int choisirModeles(std::vector<Arbre>& v) {
    std::unordered_map<uint64_t, std::vector<uint32_t>> grille;
    auto cle = [](int64_t x, int64_t z) { return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z; };
    std::vector<uint32_t> voisins;
    int tournes = 0;
    for (size_t i = 0; i < v.size(); i++) {
        Arbre& a = v[i];
        const int64_t cx = (int64_t)std::floor(a.pied.x / DISTANCE_M), cz = (int64_t)std::floor(a.pied.z / DISTANCE_M);
        voisins.clear();
        for (int64_t x = cx - 1; x <= cx + 1; x++) for (int64_t z = cz - 1; z <= cz + 1; z++) {
            const auto it = grille.find(cle(x, z));
            if (it == grille.end()) continue;
            for (uint32_t j : it->second) {
                const float dx = a.pied.x - v[j].pied.x, dz = a.pied.z - v[j].pied.z;
                if (dx * dx + dz * dz < DISTANCE_M * DISTANCE_M) voisins.push_back(j);
            }
        }
        const float lacet0 = a.lacet;
        bool ok = false;
        for (int t = 0; t < 24 && !ok; t++) {   // 0, +15°, −15°, +30°, −30°…
            const float l = t == 0 ? lacet0 : std::fmod(lacet0 + (float)((t + 1) / 2) * PAS_LACET * (t % 2 ? 1.f : -1.f) + 2 * TOUR, TOUR);
            for (int k = 0; k < MODELES && !ok; k++) {
                const int m = (int)((a.graine + (uint32_t)k) % MODELES);
                ok = true;
                for (uint32_t j : voisins) if (v[j].modele == m && ecartAngle(l, v[j].lacet) < ECART_LACET) { ok = false; break; }
                if (ok) { a.modele = m; a.lacet = l; }
            }
        }
        if (a.lacet != lacet0) tournes++;
        grille[cle(cx, cz)].push_back((uint32_t)i);
    }
    return tournes;
}

// nombre de couples de clones (contrôle au chargement, journal) : grille de cases de DISTANCE_M
inline int compterClones(const std::vector<Arbre>& v) {
    std::unordered_map<uint64_t, std::vector<uint32_t>> grille;
    auto cle = [](int64_t x, int64_t z) { return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z; };
    int n = 0;
    for (size_t i = 0; i < v.size(); i++) {
        const int64_t cx = (int64_t)std::floor(v[i].pied.x / DISTANCE_M), cz = (int64_t)std::floor(v[i].pied.z / DISTANCE_M);
        for (int64_t x = cx - 1; x <= cx + 1; x++) for (int64_t z = cz - 1; z <= cz + 1; z++) {
            const auto it = grille.find(cle(x, z));
            if (it != grille.end()) for (uint32_t j : it->second) n += clone(v[i], v[j]);
        }
        grille[cle(cx, cz)].push_back((uint32_t)i);
    }
    return n;
}

// matrice d'un exemplaire : pied, lacet (autour de y), échelle uniforme (hauteur de l'arbre / hauteur du modèle)
inline mat4 matriceArbre(vec3 pied, float lacet, float echelle) { return translate(pied) * rotateY(lacet) * scale({echelle, echelle, echelle}); }

} // namespace wfs::arbres
