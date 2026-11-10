// WaForSpeed — feux de croisement du joueur (WFS-218, épic E4) : deux phares réels, coupure du faisceau.
// Répartition d'intensité d'un feu de croisement (règlements ONU n° 112 et n° 48, classe B, circulation à droite) :
//   - coupure horizontale à −0,57° (pente de 1 %, réglage d'usine R48 § 6.2.6) ; côté droit (bas-côté), la coupure remonte
//     à 15° depuis le coude (axe V-V) jusqu'à l'« épaule » (+1°, à confirmer : R112 ne fixe que des points de mesure) ;
//   - au-dessus de la coupure : 1,5 % de la crête (point B50L de R112 : ≤ 0,4 lx à 25 m pour ≈ 12 000 cd au cœur, soit
//     ≈ 250 cd ≈ 2 %) ; transition de ±0,3° (gradient de netteté G ≥ 0,13 par 0,1° de R112, annexe 9) ;
//   - cœur juste sous la coupure (jusqu'à −1,2°), puis décroissance douce vers le bas (premier plan) ; large en azimut
//     (≈ moitié à ±18°, 12 % à 30°, 2,5 % à 45° (WFS-226 : flanc des voitures voisines), nul au-delà de 80°) : bas-côtés et pieds de façade proches restent éclairés (RG-02).
// Intensité réelle : I(el, az) = I0 × intensiteRelative(el, az) en candelas, éclairement E = I cos θ / d² (lumière
// physique, 1/d²) ; le moteur reçoit E / LUX_PAR_UNITE (étalon des lampadaires, feux_modele.h).
// Flux (DA-realisme § 3 et § 4, references/reel/photometrie.json) : flux total des DEUX phares = flux d'un feu de
// croisement sourcé (PB-LUM : halogène 1500 lm, LED 3000 lm), à ±5 % → I0 = flux / 2 / INTEGRALE_SR.
// Le shader (shaders/eclairage.glsl, feuCroisement) reprend exactement ces constantes : toute modification se fait des deux
// côtés (tests/test_phares.cpp vérifie le flux, la coupure et la portée sur ce modèle CPU).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include "../core/math.h"

namespace wfs::phares {

constexpr float COUPURE_DEG = -0.57f;       // RG-01 : coupure à −1 % (−0,57°)
constexpr float RELEVE_PENTE = 0.26795f;     // RG-01 : tan 15°, remontée côté droit
constexpr float EPAULE_DEG = 1.0f;           // RG-01 : haut de la remontée côté droit (à confirmer)
constexpr float TRANSITION_DEG = 0.3f;       // RG-02 : bord doux de la coupure (±0,3°)
constexpr float RESIDU = 0.015f;             // RG-01 : lumière parasite au-dessus de la coupure (B50L)
constexpr float NOYAU_DEG = 1.2f, CHUTE_DEG = 2.5f;   // cœur sous la coupure, puis chute vers le premier plan
constexpr float LARGEUR_DEG = 18.f, BORD_DEBUT_DEG = 60.f, BORD_FIN_DEG = 80.f;   // étalement en azimut
constexpr float INTEGRALE_SR = 0.07185f;     // ∫ intensiteRelative cos(el) dΩ (sr), vérifiée par tests/test_phares.cpp
constexpr float FLUX_HALOGENE_LM = 1500.f, FLUX_LED_LM = 3000.f;   // lum.phare_halogene / lum.phare_led.intensite (typique)
constexpr float PORTEE_M = 200.f;            // fenêtre de calcul (au-delà : < 0,3 lx, effacé par le brouillard)
// teintes (rgb linéaire, luminance 1) : corps noir 3500 K (halogène, 3400-4000 K) et 5700 K (LED, 5000-6000 K),
// CIE 1931 → sRGB linéaire (x, y = 0,4045 ; 0,3915 et 0,3279 ; 0,3375)
inline const vec3 TEINTE_HALOGENE{1.5507f, 0.8964f, 0.4043f}, TEINTE_LED{1.1163f, 0.9757f, 0.8981f};

inline float lisse(float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3 - 2 * t); }

// élévation et azimut (degrés, azimut > 0 vers la droite) depuis l'axe du phare ; 1 au cœur du faisceau
inline float intensiteRelative(float el, float az) {
    const float lim = COUPURE_DEG + std::min(std::max(az, 0.f) * RELEVE_PENTE, EPAULE_DEG - COUPURE_DEG);
    const float coupure = 1.f - (1.f - RESIDU) * lisse(lim - TRANSITION_DEG, lim + TRANSITION_DEG, el);
    const float t = (-el - NOYAU_DEG) / CHUTE_DEG, vertical = el > -NOYAU_DEG ? 1.f : 1.f / (1.f + t * t);
    const float a = az / LARGEUR_DEG, horizontal = (1.f - lisse(BORD_DEBUT_DEG, BORD_FIN_DEG, std::abs(az))) / (1.f + a * a * a * a);
    return vertical * horizontal * coupure;
}

// RG-00 / graphisme réaliste : LED ou halogène selon la génération du modèle (équipement de série, à confirmer par voiture ;
// la Gallardo a des phares au xénon, absents de la base sourcée : halogène, la teinte sourcée la plus proche du bas)
inline bool led(const char* id) {
    static const char* const HALOGENE[] = {"abarth", "compass", "gt86", "gallardo"};   // WFS-359 : « tcross » = Brezza 2022, projecteurs à DEL
    if (!id) return false;
    for (const char* h : HALOGENE) if (!std::strcmp(id, h)) return false;
    return true;
}
// crête d'un phare (cd) : la moitié du flux de la paire répartie selon intensiteRelative
inline float creteCd(bool estLed) { return (estLed ? FLUX_LED_LM : FLUX_HALOGENE_LM) * 0.5f / INTEGRALE_SR; }

// ---------- WFS-226 : phares des autres voitures (trafic, adversaires, police, camion) ----------
// Chaque voiture dessinée la nuit, sauf le joueur, est notée par Renderer::feuxVoiture (repère de ses optiques). Le
// système de nuit en tire, à l'image suivante, les vraies lumières des plus proches (RG-02 : 2 en Basse, 4 sinon, un
// projecteur par voiture au milieu de ses deux optiques, flux de la paire) ; les autres reçoivent la lumière au sol calculée
// avec la même répartition (eclairementSol), dessinée par le système de nuit à la même image.
struct PhareAutre {
    vec3 centre;        // milieu des deux optiques (monde)
    vec3 avant, droite; // axes de la voiture (normés)
    float demiEcart;    // demi-écart entre les optiques (m)
    float hauteur;      // hauteur des optiques au-dessus de la chaussée (m)
    vec3 sol;           // point de la chaussée sous le centre
};
constexpr int AUTRES_MAX = 64;
constexpr int AUTRES_ECLAIRANTS_BASSE = 2, AUTRES_ECLAIRANTS = 4;   // RG-02
constexpr float AUTRES_PORTEE_SPOT = 70.f;   // voitures à vraie lumière : à moins de 70 m du joueur
inline std::vector<PhareAutre>& autres() { static std::vector<PhareAutre> v; return v; }
inline void noterAutre(const PhareAutre& p) { auto& v = autres(); if ((int)v.size() < AUTRES_MAX) v.push_back(p); }

// éclairement (lx) de la chaussée en (x à droite, z devant) dans le repère de la voiture, par ses deux phares
// (paire LED, comme les voitures du trafic : RG-01 « même photométrie »)
inline float eclairementSol(float x, float z, float demiEcart, float h) {
    if (z <= 0.05f) return 0;
    float e = 0;
    for (float c : {-demiEcart, demiEcart}) {
        const float dx = x - c, d2 = dx * dx + z * z + h * h, d = std::sqrt(d2);
        const float el = -std::asin(h / d) * 57.29578f, az = std::atan2(dx, z) * 57.29578f;
        e += creteCd(true) * intensiteRelative(el, az) * (h / d) / d2;
    }
    return e;
}

} // namespace wfs::phares
