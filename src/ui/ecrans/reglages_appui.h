// WaForSpeed — appuis sur « Réglages » et « Fermer » (WFS-290, ANO-159) : calculs seuls, testés par tests/test_reglages_appui.cpp.
//   - cible d'un bouton = son dessin élargi à 48 dp de haut au moins et de 4 dp de chaque côté (règle Android, RG-03) ;
//   - anti-rebond : un appui qui ouvre ou ferme le panneau moins de 0,3 s après le précédent est ignoré (RG-02 : sur téléphone,
//     « Fermer » est dessiné presque à la place de « Réglages », un 2e appui rapide refermait ou rouvrait aussitôt).
#pragma once

namespace wfs::ecrans::appui {

constexpr float CIBLE_DP = 48.f, MARGE_DP = 4.f;
constexpr double ANTI_REBOND_S = 0.30;

// (zx, zy, zw, zh) : dessin du bouton (pixels) ; dp : pixels par dp ; (x, y) : appui. Un bouton non dessiné (zw ≤ 0) n'a pas de cible.
inline bool dansCible(float zx, float zy, float zw, float zh, float dp, float x, float y) {
    if (zw <= 0) return false;
    const float h = zh > CIBLE_DP * dp ? zh : CIBLE_DP * dp, cy = zy + zh / 2, m = MARGE_DP * dp;
    return x >= zx - m && x <= zx + zw + m && y >= cy - h / 2 && y <= cy + h / 2;
}

// maintenant, dernier : secondes ; vrai si l'appui peut ouvrir ou fermer le panneau
inline bool basculementPermis(double maintenant, double dernier) { return maintenant - dernier >= ANTI_REBOND_S - 1e-9; }

} // namespace wfs::ecrans::appui
