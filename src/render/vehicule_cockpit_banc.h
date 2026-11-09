// WaForSpeed — WFS-349 (RG-03) : banc « cockpit-voisine » : voiture voisine toute proche en vue Cockpit.
#pragma once
#include <string>
#include <vector>

namespace wfs {
class Game; class Renderer;
// Voisine à 1,8 m puis 1,9 m de centre à centre, à gauche puis à droite, devant puis à côté du joueur, regards Bas / Normal /
// Haut, Ville de jour : compte les pixels de la voisine dessinés sur l'habitacle ou vus sous la ligne de ceinture (0 visé) et
// ceux vus par la vitre latérale (> 0 visé : elle reste visible). `voitures` vide : les 20 intérieurs. `captures` : voitures
// dont les images (jour et nuit) sont gardées dans outDir. Rend le nombre d'écarts.
int benchCockpitVoisine(Game& g, Renderer& r, const std::string& outDir, std::vector<std::string> voitures,
                        const std::vector<std::string>& captures);
} // namespace wfs
