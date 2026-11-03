// WaForSpeed — menu principal (ticket WFS-37, épic E8) : affichage seulement (les appuis viennent avec WFS-38).
// Port de la section #menu du jeu web (index.html) et de son style (css/style.css : .screen, .mitem, .mi-t, .mi-d,
// .menu-foot, .record, .mycar, .intro, @keyframes screen-in / slide-in) ; logo et sur-titre communs avec l'écran de
// chargement (dessinerBlocLogo). Système enfichable « Menu » (ordre 450), dessiné seulement en Ecran::Menu.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
// banc d'essai : captures menu-anim (100 ms après l'entrée) et menu-0 (menu posé), puis g.shutdown()
void benchMenu(Game& g, Renderer& r, const std::string& outDir);
} // namespace ecrans
} // namespace wfs
