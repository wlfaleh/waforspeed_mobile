// WaForSpeed — banc d'essai de WFS-404 (voitures retirées) : voir voitures_retirees_banc.cpp
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
void benchVoituresFace(Game& g, Renderer& r, const std::string& outDir);       // face-<id> : chaque voiture permise de face
void benchSensInverseVille(Game& g, Renderer& r, const std::string& outDir);   // ville-sens-inverse-<moment>-<n>
} // namespace wfs
