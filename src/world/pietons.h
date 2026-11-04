// WaForSpeed — piétons animés (WFS-19, épic E2). Port de js/world/life.js (populatePeople, addPerson) et de
// js/world/people.js (pedLook, updatePeopleReal) du jeu web : vrais personnages `homme` et `femme` (Quaternius, CC0,
// assets/people/*.wfp) animés par squelette (WFS-63), habillés par l'apparence de WFS-72. Aucune silhouette géométrique.
// Système enfichable « Pietons » (ordre 106, après les décors ville 100 et plage 105).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
// banc d'essai : captures pietons-ville-0 / -1 (deux instants, poses de marche différentes) et pietons-plage, puis g.shutdown()
void benchPietons(Game& g, Renderer& r, const std::string& outDir);
} // namespace wfs
