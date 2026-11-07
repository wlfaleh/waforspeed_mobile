#pragma once
// WaForSpeed — WFS-161 (ST-5) : bâtiments du kit d'architecture posés dans les cartes à la place des boîtes.
// Une carte (ville.cpp, plage.cpp, campagne.cpp) déclare ses bâtiments à son init (poser) ; le système « archi-carte »
// (archi_carte.cpp) les construit par morceaux de route autour du joueur, au niveau de détail de leur distance
// (RG-05 : < 120 m complet, < 300 m allégé, au-delà volume seul), et les dessine en un appel par matériau et par morceau.
// Repli (RG-05) : WFS_ARCHI=0 (ou aucun bâtiment posé) → actif() rend false et la carte garde ses anciennes boîtes.
#include <cstdint>
#include "../core/math.h"
#include "archi_kit.h"

namespace wfs::archi::carte {

// oublie les bâtiments posés (appelé par la carte à son init, avant poser)
void vider();
// pose un bâtiment : seg = segment de la route où il commence (choix du morceau), origine = coin bas gauche de la façade
// (au sol), x = sens de la façade (unitaire, horizontal), z = normale de la façade vers la rue (unitaire, horizontale)
void poser(int64_t seg, vec3 origine, vec3 x, vec3 z, float largeur, float profondeur, float hauteur, Style style, uint32_t graine);
// vrai si le kit remplace les boîtes de la carte (bâtiments posés et kit non désactivé)
bool actif();

} // namespace wfs::archi::carte
