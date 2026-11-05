// WaForSpeed — sons de course, 2e passage (WFS-124) : décisions seules (aucun son, aucune dépendance au jeu), appelées par
// SfxEvenements (sfx.cpp) et testées par tests/test_audio.cpp. Le jeu web n'a ni tic du chrono ni bip de combo.
#pragma once
#include <algorithm>
#include <cmath>

namespace wfs::sfxcourse {

constexpr double CHRONO_ALERTE = 8;   // même seuil que le chrono rouge du HUD (hud.cpp : timeLeft < 8)
constexpr int COMBO_MAX_SON = 6;      // au-delà, le son du combo ×6 est rejoué

// RG-01 : le chrono compte vraiment (course Checkpoint en jeu) : pas de duel, de course à 5 ni de poursuite
// (duelFinish > 0), pas de banc sans règles (timeLeft = 1e9), écran Course sans scène dédiée
inline bool chronoActif(bool enJeu, double duelFinish, double timeLeft, bool ecranCourse) {
    return enJeu && duelFinish <= 0 && timeLeft < 1e6 && ecranCourse;
}
// valeur entière affichée par le chrono pour le tic (6,0 → 6 ; 5,4 → 6 ; 0,2 → 1)
inline int chronoAffiche(double timeLeft) { return (int)std::ceil(std::max(0.0, timeLeft)); }

enum Tic { AUCUN = 0, TIC = 1, URGENT = 2 };
// RG-02 : un tic quand la valeur affichée descend et que la nouvelle vaut 1 à 8 ; plus aigu sur 3, 2, 1.
// Un saut de plusieurs secondes = un seul tic ; une remontée (checkpoint) = aucun tic.
inline Tic ticChrono(int avant, int apres) {
    if (apres >= avant || apres < 1 || apres > (int)CHRONO_ALERTE) return AUCUN;
    return apres <= 3 ? URGENT : TIC;
}

// RG-03 : bip de combo quand le combo monte et vaut au moins 2 : rang du son (2 à 6), 0 = aucun
inline int sonCombo(int avant, int apres) {
    if (apres <= avant || apres < 2) return 0;
    return std::min(apres, COMBO_MAX_SON);
}
// fréquence du bip du combo n : 660 Hz × 2^((n − 2) · 2 / 12), un ton de plus par combo
inline float frequenceCombo(int n) { return 660.f * std::pow(2.f, (n - 2) * 2.f / 12.f); }

// RG-04 : combo perdu sur un choc : il passe d'au moins 2 à 0 et un choc est compté au même pas
// (la fin naturelle du combo après 3 s, ou un accident, ne fait aucun son de combo)
inline bool comboPerdu(int comboAvant, int comboApres, int chocsAvant, int chocsApres) {
    return comboAvant >= 2 && comboApres == 0 && chocsApres > chocsAvant;
}

} // namespace wfs::sfxcourse
