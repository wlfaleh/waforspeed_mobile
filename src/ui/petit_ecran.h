// WaForSpeed — téléphone couché : mesure de l'écran en dp (ticket WFS-147, épic E8 ; ANO-17, 18, 19, 38, 42, 47).
// Ce n'est PAS une échelle de toute l'interface (leçon de WFS-102, annulé) : chaque écran garde sa mise en page de bureau
// (k = H / 720) et choisit lui-même, s'il le veut, une mise en page « petit écran » dessinée en dp. Les écrans qui ne
// l'appellent pas (garage, HUD) ne changent pas.
//   pixelsParDp : densité lue par WFS-108 sur Android (densiteCommandes, pixels de l'interface par dp) ; banc PC :
//                 2,625 (420 dpi, le téléphone 2400x1080 de RG-02) si W ≥ 2000, sinon 1 (captures 1280x720 inchangées).
//   petitEcran  : écran de 520 dp de haut au plus (@media (max-height: 520px) du web).
//   unitePetitEcran : pixels par dp de la mise en page petit écran, réduite si l'écran fait moins de 400 dp de haut
//                 ou de 800 dp de large (les mises en page petit écran sont dessinées pour tenir dans 800 x 400 dp).
// Calcul seul : testé par tests/test_petit_ecran.cpp.
#pragma once
#include <algorithm>
#include "../game/commandes_tactiles.h"   // densiteCommandes (WFS-108)

namespace wfs::ui {

constexpr float DPI_BANC_2400 = 420.f;           // RG-02 : 2400x1080 à 420 dpi
constexpr float HAUTEUR_PETIT_ECRAN = 520.f;     // dp
constexpr float HAUTEUR_MISE_EN_PAGE = 400.f, LARGEUR_MISE_EN_PAGE = 800.f;   // dp : place minimale des mises en page petit écran
constexpr float TEXTE_MIN_DP = 14.f, TOUCHER_MIN_DP = 44.f;   // RG-02

inline float pixelsParDp(float W, float densite) {
    if (densite > 0) return densite;
    return W >= 2000 ? DPI_BANC_2400 / 160.f : 1.f;
}
inline float pixelsParDp(float W) { return pixelsParDp(W, densiteCommandes()); }
inline bool petitEcran(float W, float H, float densite) { return H / pixelsParDp(W, densite) <= HAUTEUR_PETIT_ECRAN; }
inline bool petitEcran(float W, float H) { return petitEcran(W, H, densiteCommandes()); }
inline float unitePetitEcran(float W, float H, float densite) { return std::min({pixelsParDp(W, densite), H / HAUTEUR_MISE_EN_PAGE, W / LARGEUR_MISE_EN_PAGE}); }
inline float unitePetitEcran(float W, float H) { return unitePetitEcran(W, H, densiteCommandes()); }

} // namespace wfs::ui
