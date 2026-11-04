// WaForSpeed — planche de contrôle des 20 voitures (WFS-77, épic E5).
// Banc d'essai : chaque voiture du catalogue est chargée, repeinte en Turquoise (peinture du garage), photographiée en
// vitrine (3/4 avant) avec son nom en bas, puis réduite dans une grille 5 × 4 : vitrines-planche.png. Le journal donne,
// pour chaque voiture, les roues séparées (4/4) et le nombre de matériaux de peinture du .wfm.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
void benchPlancheVoitures(Game& g, Renderer& r, const std::string& out);
} // namespace wfs
