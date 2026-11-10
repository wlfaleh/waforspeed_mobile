// WaForSpeed — WFS-380 (RG-01) : vagues de la mer (valeurs partagées avec shaders/eau.glsl ; sans dépendance au moteur : tests CPU).
#pragma once
#include <cmath>

namespace wfs::eau {

// RG-01 : vagues (mêmes valeurs que EAU_VAGUES de shaders/eau.glsl ; test_eau vérifie qu'elles sont identiques)
struct Vague { float dx, dz, longueur, amplitude; };
constexpr Vague VAGUES[8] = {
    {0.9659f, 0.2588f, 28.0f, 0.110f}, {0.8090f, 0.5878f, 19.0f, 0.085f}, {0.9945f, -0.1045f, 14.5f, 0.070f},
    {0.5592f, 0.8290f, 9.3f, 0.040f},  {0.8746f, -0.4848f, 6.1f, 0.034f}, {0.3420f, 0.9397f, 3.7f, 0.026f},
    {0.6947f, -0.7193f, 2.3f, 0.019f}, {0.4226f, 0.9063f, 1.4f, 0.012f}};
constexpr float G = 9.81f;
constexpr int VAGUES_BASSE = 4;   // RG-07 : en Basse, les 4 plus longues ; les autres passent dans la rugosité

// période d'une vague en eau profonde (s) : ω² = g k
inline float periode(float longueur) { return std::sqrt(2.f * 3.14159265f * longueur / G); }
// hauteur significative de la mer (m) : 4 écarts types de la hauteur (somme de sinusoïdes : variance Σ A² / 2)
inline float hauteurSignificative(float agitation = 1.f) {
    float v = 0; for (const Vague& w : VAGUES) v += 0.5f * w.amplitude * w.amplitude;
    return 4.f * std::sqrt(v) * agitation;
}

} // namespace wfs::eau
