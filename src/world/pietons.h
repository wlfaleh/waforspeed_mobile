// WaForSpeed — piétons animés (WFS-19, épic E2). Port de js/world/life.js (populatePeople, addPerson) et de
// js/world/people.js (pedLook, updatePeopleReal) du jeu web : vrais personnages `homme` et `femme` (Quaternius, CC0,
// assets/people/*.wfp) animés par squelette (WFS-63), habillés par l'apparence de WFS-72. Aucune silhouette géométrique.
// Système enfichable « Pietons » (ordre 106, après les décors ville 100 et plage 105).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"

namespace wfs {
class Game;
class Renderer;
// banc d'essai : captures pietons-ville-0 / -1 (deux instants, poses de marche différentes) et pietons-plage, puis g.shutdown()
void benchPietons(Game& g, Renderer& r, const std::string& outDir);

// WFS-199 : spectateurs des tribunes. Les mêmes personnages (homme.wfp, femme.wfp), allégés (sommets regroupés sur une grille,
// en pose de liaison) et posés une fois sur le processeur (Sitting_* / Idle_*, plusieurs instants du clip) : maillage fixe que
// le décor ajoute à ses morceaux (aucun appel de dessin en plus, aucune limite d'instances animées). Zone de chaque sommet,
// calculée comme le pedMaterial du jeu web (scene_skin.frag, WFS-72) : le décor y met ses couleurs.
namespace foule {
enum Zone : uint8_t { PEAU, CHEVEUX, HAUT, BAS, CHAUSSURES };
enum Pose { ASSIS_REPOS, ASSIS_PARLE, DEBOUT_REPOS, DEBOUT_PARLE, NB_POSES };
constexpr int COIFFURES = 3, VARIANTES = 3;   // variante : instant du clip (et manches courtes / longues)
struct Sommet { vec3 p, n; Zone zone; };
// repère local : face vers +z, centre en x = z = 0 ; assis : fesses en y = 0 (posées sur le siège) ; debout : pieds en y = 0
struct Silhouette { std::vector<Sommet> v; std::vector<uint32_t> i; float assise = 0.45f; };   // assise : fesses − pieds (m), assis seulement
// proche : grille de 10 cm, sinon 16 cm (de loin) ; nullptr si les personnages sont illisibles
const Silhouette* silhouette(bool femme, int coiffure, Pose pose, int variante, bool proche);
} // namespace foule
} // namespace wfs
