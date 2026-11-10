// WaForSpeed — WFS-414 : banc « cockpit-volant » : gros plans du volant en vue Cockpit (Ville, jour et nuit, braquage).
#pragma once
#include <string>
#include <vector>

namespace wfs {
class Game; class Renderer;
// Pour chaque voiture (`voitures` vide : les 6 voitures du volant de série, puis la M4 et la 911, témoins qui ne changent pas) :
// vue Cockpit entière en Ville de jour (volant-<voiture>-jour) et son gros plan du volant (…-gros-plan, ×2, recadré sur la jante
// projetée par la vraie caméra de l'image) ; Mustang : nuit et suite de 4 images d'un braquage ; Golf : suite de braquage.
void benchCockpitVolant(Game& g, Renderer& r, const std::string& outDir, std::vector<std::string> voitures);
} // namespace wfs
