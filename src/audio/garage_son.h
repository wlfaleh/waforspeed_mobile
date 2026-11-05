// WaForSpeed — showroom sonore (WFS-127) : au garage, la voiture affichée fait entendre son moteur : démarreur (sfx.cpp,
// SfxId::Demarreur), puis un coup de gaz et le ralenti. Calcul seul (aucune dépendance au jeu) : appelé par AudioSystem
// (audio_system.cpp), testé par tests/test_audio.cpp. Le jeu web n'a pas de son au garage.
#pragma once
#include <algorithm>
#include <cmath>

namespace wfs::garageson {

constexpr float DEMARREUR = 0.45f;      // durée du démarreur (s) : le coup de gaz part juste après
constexpr float RALENTI = 0.12f, POINTE = 0.65f;
constexpr float MONTEE = 0.30f, DESCENTE = 0.90f;   // RG-04 : 0,12 → 0,65 en 0,30 s, puis → 0,12 en 0,90 s
constexpr float ECART_DEMARREUR = 0.25f;            // RG-05 : au plus un démarreur toutes les 0,25 s

// RG-04 : régime du coup de gaz, t en s depuis le début du coup de gaz (avant 0 : ralenti)
inline float regimeGarage(float t) {
    if (t <= 0) return RALENTI;
    if (t < MONTEE) { const float k = t / MONTEE; return RALENTI + (POINTE - RALENTI) * k * (2 - k); }   // montée rapide qui s'arrondit
    if (t < MONTEE + DESCENTE) { const float k = (t - MONTEE) / DESCENTE; return POINTE - (POINTE - RALENTI) * (1 - (1 - k) * (1 - k)); }
    return RALENTI;
}
// pédale « virtuelle » du coup de gaz : enfoncée pendant la montée (son « en charge »), relâchée ensuite
inline float chargeGarage(float t) { return t > 0 && t < MONTEE ? 1.f : 0.f; }

// cibles du son du moteur pour un régime r et une charge l (mêmes formules que EngineModel::update, voiture à l'arrêt)
struct Cibles { float rate, loGain, loFreq, hiGain, hiFreq, hiTau; };
inline Cibles cibles(float r, float l, float pitch) {
    Cibles c;
    c.rate = std::clamp(pitch * (0.55f + r * 1.15f), 0.4f, 2.6f);
    c.loGain = (0.13f + r * 0.1f) * (1 - l * 0.55f);
    c.loFreq = 500 + r * 1600;
    c.hiGain = l * (0.17f + r * 0.2f);
    c.hiFreq = 2200 + r * 6000;
    c.hiTau = 0.04f;
    return c;
}

} // namespace wfs::garageson
