// WaForSpeed — WFS-233 : enfants et familles dans les rues (épic E11). Règles pures (sans rendu), testées par
// tests/fidelite_humains_familles.cpp et appliquées par les piétons (pietons.cpp) :
//   - RG-02 : un enfant marche toujours à côté d'un adulte (même sens, même vitesse), côté façades, jamais sur la chaussée ;
//   - RG-03 : les enfants REMPLACENT des adultes (un marcheur adulte devient l'enfant d'un autre) : le total ne change pas ;
//   - part d'enfants par lieu (avis expert-realisme-3d, INSEE ≈ 17 % de moins de 15 ans en France) : Ville 10 %, village de la
//     Campagne 12 %, Corniche et Grand Prix 15 % ; jamais près d'une route rapide (les marcheurs de l'autoroute n'existent pas).
// 8 morphotypes (assets/people/humains/e04g … e14f) : garçon / fille de 4, 7, 10 et 14 ans, statures CDC 2000.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs::familles {

constexpr int NB_ENFANTS = 8;
constexpr const char* ENFANTS[NB_ENFANTS] = {"e04g", "e04f", "e07g", "e07f", "e10g", "e10f", "e14g", "e14f"};
constexpr int AGES[4] = {4, 7, 10, 14};
inline int age(int morpho) { return AGES[std::clamp(morpho, 0, NB_ENFANTS - 1) / 2]; }
inline bool fille(int morpho) { return morpho % 2 == 1; }
// main dans la main à 4 et 7 ans ; à 10 et 14 ans, l'enfant marche à côté (avis expert-realisme-3d)
inline bool tientLaMain(int morpho) { return age(morpho) <= 7; }
// vitesse de marche libre (m/s) : Sutherland et al. 1980, Lythgo et al. 2009 (avis expert-realisme-3d, « à confirmer ») ;
// la famille marche au pas de l'enfant, bornée à 0,9 m/s au moins
inline double vitesseMax(int morpho) { const int a = age(morpho); return a <= 4 ? 1.0 : a <= 7 ? 1.1 : 1.15; }
constexpr double VITESSE_MIN = 0.9;
constexpr double ECART_LAT_M = 0.65;   // entre les axes du parent et de l'enfant (demi-épaules 0,18 + 0,11 m et marge)
constexpr double AVANCE_M = 0.15;      // l'enfant marche un peu devant (avis expert-realisme-3d : 0,1 à 0,2 m)

// part des piétons qui sont des enfants, selon la carte
inline double partEnfants(const char* carte) {
    const std::string c = carte ? carte : "";
    return c == "ville" ? 0.10 : c == "campagne" ? 0.12 : (c == "plage" || c == "circuit") ? 0.15 : 0.0;
}

// un marcheur candidat : position (s en tronçons de 1,2 m, lat en m depuis l'axe), bande du trottoir |lat| ∈ [latMin, latMax]
struct Marcheur { double s = 0, lat = 0, v = 1.1; int dir = 1; double latMin = 0, latMax = 0; bool candidat = false; };
struct Lien { int parent = -1, enfant = -1, morpho = 0; };

// Forme les familles parmi les marcheurs candidats (dans l'ordre de la liste, déterministe pour une graine) : `total` = nombre
// de piétons de la carte (assis et baigneurs compris), `part` = part visée d'enfants. Deux candidats voisins de la liste
// forment une famille : le premier reste adulte (parent), le second devient l'enfant, replacé à côté du parent. Bande trop
// étroite pour deux (< ECART_LAT_M) : pas de famille. Renvoie les liens ; les marcheurs sont modifiés en place.
inline std::vector<Lien> former(std::vector<Marcheur>& m, int total, double part, uint32_t graine, double segM) {
    std::vector<Lien> liens;
    std::vector<int> cand;
    for (int i = 0; i < (int)m.size(); i++) if (m[(size_t)i].candidat && m[(size_t)i].latMax - m[(size_t)i].latMin >= ECART_LAT_M) cand.push_back(i);
    const int voulu = (int)std::floor(part * total + 0.5), paires = (int)cand.size() / 2;
    if (voulu <= 0 || paires <= 0) return liens;
    Mulberry32 R(graine * 2654435761u + 233u);
    const double p = std::min(1.0, (double)voulu / paires);
    for (int k = 0; k + 1 < (int)cand.size() && (int)liens.size() < voulu; k += 2) {
        if (R.next() >= p) continue;
        Marcheur& P = m[(size_t)cand[(size_t)k]];
        Marcheur& E = m[(size_t)cand[(size_t)k + 1]];
        const int morpho = std::min((int)(R.next() * NB_ENFANTS), NB_ENFANTS - 1);
        const double cote = P.lat >= 0 ? 1 : -1;
        // le parent côté chaussée, l'enfant côté façades ; tous deux dans la bande du trottoir du parent
        const double inter = std::clamp(std::fabs(P.lat), P.latMin, P.latMax - ECART_LAT_M);
        P.lat = cote * inter;
        P.v = std::clamp(P.v, VITESSE_MIN, vitesseMax(morpho));
        E = P;
        E.lat = cote * (inter + ECART_LAT_M);
        E.s = P.s + P.dir * AVANCE_M / segM;
        liens.push_back({cand[(size_t)k], cand[(size_t)k + 1], morpho});
    }
    return liens;
}

// cadence de l'animation de marche (multiplicateur du clip) : pas sans glissement, comme les adultes (v / 1,1 m/s), le clip
// de l'enfant avançant déjà de `kBassin` fois la foulée adulte (translation de la racine mise à l'échelle du bassin)
inline double cadence(double v, double kBassin) { return v / (1.1 * std::max(kBassin, 0.3)); }

// teint de l'enfant = celui de son parent (pixel près) : le parent (homme.wfp / femme.wfp, Quaternius) a une peau
// « texture × peau » ; la peau médiane de leur atlas (mains, médiane des texels mesurée pour WFS-233) vaut (169, 114, 80) en sRGB ;
// l'enfant (MakeHuman) a une teinte unie (222, 178, 150) : multiplicateur = peau du parent × atlas / teinte unie
inline vec3 peauEnfant(const vec3& peauParent) {
    return {peauParent.x * 169.f / 222.f, peauParent.y * 114.f / 178.f, peauParent.z * 80.f / 150.f};
}

} // namespace wfs::familles
