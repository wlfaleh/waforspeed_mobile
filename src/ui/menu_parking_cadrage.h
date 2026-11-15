// WaForSpeed — WFS-552 (RG-04) : cadrage de la caméra rapprochée du menu, calcul seul (aucun Vulkan : testé par
// tests/test_menu_parking.cpp). Voir menu_parking.h.
#pragma once
#include "../core/math.h"

namespace wfs::ecrans::parking {

// cadrage de la caméra rapprochée (RG-04), calcul seul (testé par tests/test_menu_parking.cpp) : position de l'œil et point
// regardé dans le repère de la voiture, décalage horizontal de l'image (NDC) pour mettre la voiture dans la partie droite
// decalage, decalageY : décentrement de l'objectif (NDC, y vers le haut) ; projection = translation (decalage, −decalageY) × perspective
struct Cadrage { vec3 oeil, cible; float fovY = 0, decalage = 0, decalageY = 0; };
Cadrage cadrage(float longueur, float largeur, float hauteur, float aspect, float cap = 0.42f);

// banc d'essai seulement (menu_parking_banc.cpp) : angle de vue imposé (rad depuis l'axe de la voiture ; < 0 : celui du menu) et
// vue du décor seul (poteau, plafond, sol) pour l'avis de l'agent 3D
namespace banc { inline float cap = -1; inline bool decor = false; }
// demi-largeur et demi-hauteur (NDC) de la boîte de la voiture vue par ce cadrage, et son centre (NDC) : contrôle « voiture non coupée »
struct Emprise { float xmin, xmax, ymin, ymax; };
Emprise emprise(const Cadrage& c, float longueur, float largeur, float hauteur, float aspect);

} // namespace wfs::ecrans::parking
