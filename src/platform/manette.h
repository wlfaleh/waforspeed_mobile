// WaForSpeed — manettes de jeu (WFS-22 / WFS-23, épic E7) : partie pure, sans Android ni Vulkan.
//
// Le jeu web n'a pas de manette : on reprend ses actions clavier (KEYMAP de js/ui/screens.js : up, down, left, right,
// nitro ; C = caméra ; P / Échap = pause) et on les donne au même `DriveInput` que le tactile (RG-06 : noyau de conduite
// inchangé ; le braquage reste progressif dans Player::update, taux PHYS.steerIn 4 / steerOut 6 de js/core/config.js).
//
//   gâchette droite (ou R2) = accélérer      gâchette gauche (ou L2) = freiner / marche arrière
//   stick gauche (axe X) ou croix = braquer  A / Croix = nitro         Y / Triangle = changer de vue   Start = pause
//   menus : croix ou stick = déplacer le focus, A = valider, B = retour ; zone morte 0,12 sur les axes
//
// Focus des menus : chaque écran déclare ses boutons à chaque image (manette::focus(zone), dans drawUi) ; la manette
// déplace le focus vers le bouton le plus proche dans la direction demandée et « appuie » sur le bouton choisi par un
// toucher simulé en son centre (les écrans gardent leur propre logique de toucher, rien n'est dupliqué).
#pragma once
#include <vector>

namespace wfs {
struct DriveInput;

namespace manette {

constexpr float ZONE_MORTE = 0.12f;      // RG-02 (WFS-23)
constexpr float SEUIL_MENU = 0.5f;       // stick : direction de menu au-delà de 0,5, réarmée sous 0,3
constexpr float REARME_MENU = 0.3f;

enum Bouton { A, B, X, Y, START, SELECT, L1, R1, L2, R2, HAUT, BAS, GAUCHE, DROITE, NB_BOUTONS };

struct Etat {
    float stickX = 0, stickY = 0;          // −1..1 (Y vers le bas positif, comme Android)
    float gachetteG = 0, gachetteD = 0;    // 0..1
    float croixX = 0, croixY = 0;          // chapeau (AXIS_HAT_X / Y) : −1, 0, 1
    bool bouton[NB_BOUTONS] = {};
};

// zone morte : |v| < 0,12 → 0, puis remise à l'échelle 0..1 (pas de saut à la sortie de la zone morte)
float zoneMorte(float v);
// commandes de conduite d'un état de manette (fonction pure, vérifiée par tests/test_manette.cpp)
void conduite(const Etat& e, DriveInput& out);
// direction de menu (dx, dy dans {−1, 0, 1}) : un pas par appui de la croix (front par rapport à l'état précédent),
// un pas par poussée du stick au-delà de 0,5 (arme : réarmé quand le stick revient sous 0,3)
void directionMenu(const Etat& e, const Etat& precedent, bool& arme, int& dx, int& dy);

// ---------- focus des menus ----------
struct Zone { float x, y, w, h; bool retour; };
// déclare un bouton de l'écran affiché (pixels de l'interface) ; retour = bouton « Retour » (touche B)
void zoneFocus(float x, float y, float w, float h, bool retour = false);
template <class Z> inline void focus(const Z& z, bool retour = false) { if (z.w > 0 && z.h > 0) zoneFocus(z.x, z.y, z.w, z.h, retour); }
// bouton voisin dans la direction (dx, dy) : centre du côté demandé, le plus proche (écart de côté compté double) ;
// renvoie i s'il n'y en a pas
int voisin(const std::vector<Zone>& zones, int i, int dx, int dy);
// zones de l'image en cours / de l'image précédente (échangées par finImage, à la fin de l'interface)
std::vector<Zone>& zonesImage();
const std::vector<Zone>& zonesPrecedentes();
void finImage();

// ---------- état de la manette (thread de la boucle de jeu) ----------
// événements de la plateforme : boutons, axes ; toucherEcran = retour au tactile (RG-03) ; deconnexion = manette partie
void bouton(Bouton b, bool appui);
void axes(float stickX, float stickY, float gachetteG, float gachetteD, float croixX, float croixY);
void toucherEcran();
void deconnexion();
bool deconnexionRecue();       // vrai une fois après deconnexion() (pause automatique en course, RG-05)
bool actif();                  // mode manette : dernière entrée reçue d'une manette (commandes tactiles masquées)
const Etat& etat();

} // namespace manette
} // namespace wfs
