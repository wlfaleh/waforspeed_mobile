// WaForSpeed — scène du banc d'essai pour les projecteurs, la lumière hémisphère et le brouillard linéaire (WFS-45)
#pragma once
#include <string>
namespace wfs {
class Renderer;
// spots-sol.png : éclairage du garage du jeu web (js/world/garage.js → buildGarage) sur un sol sombre, une voiture au centre ;
// le journal donne le coût de la passe de scène sans projecteur puis avec les 3 projecteurs
void benchSpots(Renderer& r, const std::string& out);
}
