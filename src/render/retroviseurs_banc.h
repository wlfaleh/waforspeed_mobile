// WaForSpeed — WFS-324 : banc « retroviseurs » (WFS_BANC=retroviseurs ; WFS_VOITURES=golf,m4 : liste, golf par défaut).
// Vue Cockpit, Ville, une voiture placée derrière (même voie, puis voie de gauche), de jour et de nuit : images complètes et zooms ×3
// de chaque glace (retro-<voiture>-<scène>[-nuit].png, -zoom-interieur / -gauche / -droite) ; pixels de la voiture vue dans chaque
// glace (image avec / sans la voiture) ; coût en Basse et en Haute (avec / sans rétroviseurs, ms par image et passe « rétroviseurs »).
// Renvoie le nombre d'écarts (voiture derrière invisible dans la glace intérieure ou dans la glace gauche, image illisible).
#pragma once
#include <string>
#include <vector>

namespace wfs {
class Game;
class Renderer;
int benchRetroviseurs(Game& g, Renderer& r, const std::string& outDir, std::vector<std::string> voitures);
}
