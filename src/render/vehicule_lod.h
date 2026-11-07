#pragma once
// WaForSpeed — niveaux de détail des voitures (WFS-158, RG-04 ; suite de WFS-134, RG-11 à RG-13).
// 4 niveaux : 0 = modèle complet (.wfm, roues qui tournent), 1 à 3 = versions allégées de <id>.lod (tools/alleger_voiture.mjs :
// mêmes sommets et matériaux, seulement d'autres indices ; jamais de cube). Le niveau dépend de la longueur de la voiture à
// l'écran, en pixels de la scène 3D (seuils ci-dessous) ; entre deux niveaux, fondu tramé écran complémentaire de WFS-134
// (scene.frag) sur une bande de 1/9 du seuil (de 64 à 72 m pour l'ancien passage complète → allégée : même rapport).
// Qualité Basse : complète, puis version en 4 pièces (partsBasse, moins d'appels de dessin) au 1er seuil, comme WFS-134.
// Sans dépendance au moteur (gabarits) : testé seul par tests/test_vehicule_lod.cpp.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace wfs::vehicule_lod {

constexpr int NIVEAUX = 4;
// longueur de la voiture à l'écran (pixels de la scène) sous laquelle le niveau k + 1 remplace le niveau k.
// 60 px ≈ 64 m de la caméra pour une Golf (4,26 m) en poursuite (champ vertical 64°, scène de 1080 px de haut) : l'ancien
// passage de WFS-134 ; 36 px ≈ 105 m ; 22 px ≈ 170 m (voitures dessinées jusqu'à 200 m, masquage::distanceMax).
// WFS-246 (levier 4, ANO-125) : modèle complet (190 à 275 k triangles) seulement au-dessus de 200 px (une Golf à 18 m) :
// avant, chaque adversaire à moins de 64 m gardait le complet (≈ 1 M de triangles par image en Haute). Seuils 2 et 3 un peu
// plus hauts (48 et 26 px, 77 et 140 m) pour garder le même écart entre niveaux. La voiture du joueur (poursuite, cockpit)
// reste toujours au-dessus de 200 px : complète.
constexpr float SEUILS_PX[NIVEAUX - 1] = {200.f, 48.f, 26.f};   // WFS-158 : {60, 36, 22}
constexpr float BANDE = 1.f / 9;   // largeur du fondu, en part du seuil (de S à S × 8/9)

// longueur à l'écran (px) d'un objet de longueur L (m) à la distance d (m) ; focalePx = |proj(1,1)| × hauteur de la scène / 2
inline float pixels(float longueur, float focalePx, float distance) { return longueur * focalePx / std::max(distance, 0.05f); }

// niveaux a (dessiné) et b (fondu) ; t ∈ [0, 1) : 0 = a seul, sinon a sur les pixels où le seuil de tramage ≥ t, b ailleurs
struct Choix { int a = 0, b = 0; float t = 0; };

// nNiveaux : nombre de niveaux disponibles (1 = complet seul). Plus la voiture est petite à l'écran, plus le niveau est haut.
inline Choix choisir(float px, int nNiveaux) {
    nNiveaux = std::clamp(nNiveaux, 1, NIVEAUX);
    for (int k = 0; k + 1 < nNiveaux; k++) {
        const float S = SEUILS_PX[k];
        if (px >= S) return {k, k, 0.f};
        if (px > S * (1.f - BANDE)) return {k, k + 1, std::clamp((S - px) / (S * BANDE), 0.f, 0.999f)};
    }
    return {nNiveaux - 1, nNiveaux - 1, 0.f};
}

// Listes de pièces à dessiner pour un modèle M (champs parts, partsLod, partsBasse, partsNiveaux, bmin, bmax) :
// a, puis b en fondu si t > 0. fantome : complet seul (WFS-134, RG-07) ; lots (police en Basse) : partsBasse seule.
template <class M>
struct Dessin { const decltype(M::parts)* a = nullptr; const decltype(M::parts)* b = nullptr; float t = 0; int niveau = 0; };

template <class M>
Dessin<M> dessin(const M& m, float distance, float focalePx, bool basse, bool lots, bool fantome) {
    using Liste = decltype(M::parts);
    const Liste* l[NIVEAUX] = {&m.parts};
    int n = 1;
    const bool versBasse = basse && !m.partsBasse.empty();
    if (!fantome) {
        const Liste& loin = versBasse ? m.partsBasse : m.partsLod;
        if (!loin.empty()) l[n++] = &loin;
        // niveaux 2 et 3 : qualité Moyenne et plus (en Basse, la version en 4 pièces reste au loin, comme WFS-134)
        for (size_t k = 0; n == (int)k + 2 && !basse && k < m.partsNiveaux.size() && n < NIVEAUX; k++)
            if (!m.partsNiveaux[k].empty()) l[n++] = &m.partsNiveaux[k];
    }
    if (lots && versBasse && !fantome) return {&m.partsBasse, &m.partsBasse, 0.f, 1};
    const float L = std::max({m.bmax.x - m.bmin.x, m.bmax.y - m.bmin.y, m.bmax.z - m.bmin.z});
    const Choix c = choisir(pixels(L, focalePx, distance), n);
    return {l[c.a], l[c.b], c.t, c.a};
}

// Lecture de <id>.lod : 'WFL1' (un niveau, indices u32) ou 'WFL2' (n niveaux, indices u16 ou u32 par maillage).
// complet : une pièce par maillage du .wfm (vertexOffset, material). Sortie : indices de tous les niveaux à la suite
// (relatifs au maillage, comme le .wfm), une pièce par maillage et par niveau dans l'ordre des niveaux, et pour chaque
// niveau la fin de sa plage dans idx. Faux (sorties vidées) si le fichier est illisible ou ne suit pas le .wfm.
template <class Part>
bool lire(const uint8_t* d, size_t n, const std::vector<Part>& complet, std::vector<uint32_t>& idx, std::vector<Part>& parts,
          std::vector<uint32_t>& fins) {
    idx.clear(); parts.clear(); fins.clear();
    size_t p = 0;
    auto u32 = [&](uint32_t& v) { if (p + 4 > n) return false; std::memcpy(&v, d + p, 4); p += 4; return true; };
    auto echec = [&] { idx.clear(); parts.clear(); fins.clear(); return false; };
    if (n < 8) return false;
    const bool v2 = std::memcmp(d, "WFL2", 4) == 0;
    if (!v2 && std::memcmp(d, "WFL1", 4) != 0) return false;
    p = 4;
    uint32_t nMaillages = 0, nNiveaux = 1;
    if (!u32(nMaillages) || nMaillages != complet.size() || (v2 && !u32(nNiveaux)) || nNiveaux < 1 || nNiveaux > NIVEAUX - 1) return echec();
    for (uint32_t k = 0; k < nNiveaux; k++) {
        for (uint32_t i = 0; i < nMaillages; i++) {
            uint32_t ni = 0, octets = 4;
            if (!u32(ni) || (v2 && !u32(octets)) || (octets != 2 && octets != 4) || ni % 3) return echec();
            const size_t taille = (size_t)ni * octets;
            if (taille > n - p) return echec();
            parts.push_back(complet[i]);
            parts.back().firstIndex = (uint32_t)idx.size(); parts.back().indexCount = ni;
            const size_t i0 = idx.size(); idx.resize(i0 + ni);
            if (octets == 4) std::memcpy(&idx[i0], d + p, taille);
            else for (uint32_t j = 0; j < ni; j++) { uint16_t s; std::memcpy(&s, d + p + 2 * (size_t)j, 2); idx[i0 + j] = s; }
            p += taille;
            if (v2) p += (4 - taille % 4) % 4;
        }
        fins.push_back((uint32_t)idx.size());
    }
    return true;
}

// Après les découpes du chargement (feux, tranches des portes : elles restent dans la plage d'indices de leur pièce),
// remet chaque pièce dans son niveau d'après la plage de son premier indice : niveau 1 → premier, niveaux 2… → autres.
// base : décalage des indices ajouté ensuite (les pièces gardent leur firstIndex d'origine + base).
template <class Part>
void repartir(const std::vector<Part>& parts, const std::vector<uint32_t>& fins, uint32_t base, std::vector<Part>& premier,
              std::vector<std::vector<Part>>& autres) {
    premier.clear(); autres.assign(fins.empty() ? 0 : fins.size() - 1, {});
    for (Part q : parts) {
        const size_t k = (size_t)(std::upper_bound(fins.begin(), fins.end(), q.firstIndex) - fins.begin());
        if (k >= fins.size()) continue;
        q.firstIndex += base;
        (k == 0 ? premier : autres[k - 1]).push_back(q);
    }
}

} // namespace wfs::vehicule_lod
