// WaForSpeed — banc d'essai du changement de carte (WFS-74) : voir bench_changement_carte.cpp.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
void benchChangementCarte(Game& g, Renderer& r, const std::string& out);
} // namespace wfs
