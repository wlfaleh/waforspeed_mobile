// WaForSpeed — WFS-325 : banc « cockpit-vitrine » (WFS_BANC=cockpit-vitrine ; WFS_VOITURES=golf,m4 : liste, sinon les 4 du lot 1).
// L'intérieur de chaque voiture est dessiné seul, dans une scène dédiée (WFS-44) à l'éclairage d'atelier, vu de 4 places tournées
// vers ce que la vue Cockpit ne montre pas en entier : console, portière du conducteur, centre et gauche de la planche.
// Captures vitrine-<voiture>-<vue>.png : aide à la revue des pièces modelées, ce n'est pas l'image du jeu (vue Cockpit : banc « cockpit »).
#pragma once
#include <string>
#include <vector>

namespace wfs {
class Game;
class Renderer;
void benchCockpitVitrine(Game& g, Renderer& r, const std::string& out, std::vector<std::string> voitures);
}
