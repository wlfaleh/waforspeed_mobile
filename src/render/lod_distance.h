// WaForSpeed — effacement au loin par fondu tramé (WFS-265, RG-01, épic E4).
// Au-delà de leur distance maximale, les personnes et les petits objets ne sont plus dessinés ; sur les FONDU_M derniers mètres,
// un fondu tramé écran (fonduLod de shaders/vehicule.glsl, mêmes codes que les niveaux de détail des voitures de WFS-134) les
// efface peu à peu : aucun objet qui saute (RG-01), aucune personne qui disparaît d'un coup (WFS-257). La simulation, le nombre
// de personnes et les ombres proches ne changent pas.
// Sans dépendance au moteur : testé seul par tests/test_lod_distance.cpp.
#pragma once
#include <algorithm>

namespace wfs::lod_distance {

constexpr float FONDU_M = 15.f;   // largeur du fondu (RG-01 : 10 à 20 m)
// personnes (maillages à squelette) : Basse 90 m (≈ 8 px de haut sur la scène de la Basse, 540 px), sinon 150 m (RG-01) ; 60 m essayé : −4 % de triangles seulement (piétons proches)
constexpr float PERSONNES_BASSE_M = 90.f, PERSONNES_M = 150.f;
inline float personnesMax(bool basse) { return basse ? PERSONNES_BASSE_M : PERSONNES_M; }

// part visible (0 à 1) d'un objet à la distance d (m) dont la distance maximale est dMax : 1 avant dMax − FONDU_M, 0 au-delà de dMax
inline float partVisible(float d, float dMax) {
    if (!(d == d) || !(dMax == dMax)) return 1.f;
    if (d <= dMax - FONDU_M) return 1.f;
    if (d >= dMax) return 0.f;
    return std::clamp((dMax - d) / FONDU_M, 0.f, 1.f);
}

// code de tramage de fonduLod (pc.emissive.w) : −(1 + t) garde la part t des pixels (t ∈ ]0, 1[), −(3 + t) en garde 1 − t ;
// 0 = aucun tramage. Combine un fondu déjà demandé (apparition d'une personne, WFS-257) avec la part visible due à la distance :
// la plus petite des deux parts l'emporte.
inline float partDuCode(float code) {
    if (code > -0.5f) return 1.f;
    const float w = -code;
    return w < 2.5f ? std::clamp(w - 1.f, 0.f, 1.f) : std::clamp(1.f - (w - 3.f), 0.f, 1.f);
}
inline float combinerFondu(float code, float part) {
    if (part >= 1.f) return code;
    const float p = std::min(partDuCode(code), std::clamp(part, 0.f, 1.f));
    return -(1.f + std::clamp(p, 0.001f, 0.999f));
}

} // namespace wfs::lod_distance
