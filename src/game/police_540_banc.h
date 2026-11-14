// WaForSpeed — banc d'essai de WFS-540 (police = vraie voiture, ANO-267, 268, 271, 272) : WFS_BANC=police-540.
// Golf du joueur, 208 de police, suites d'images (police540-<scène>-NN), légende des mesures écrite sur chaque image :
//   - contourne        : Corniche de jour, vue de dessus : le joueur change de file, la police derrière le suit (ANO-272 : avant,
//                        elle glissait de côté sans tourner ; après, elle braque et tourne comme une voiture) ;
//   - repart-nuit      : Corniche de nuit, vue Poursuite : joueur arrêté, la police collée devant lui, GAZ appuyé (ANO-267) ;
//   - repart-cockpit   : même scène, vue Cockpit ;
//   - arrestation      : Corniche de jour : joueur arrêté, la police arrive (ANO-268 : compte à rebours puis « ARRÊTÉ ») ;
//   - choc             : barrage de police, choc de face à 30 km/h sans frein, HUD visible (ANO-271 : pas de « R ») ;
//   - camion           : Corniche de jour, vue de haut : la police derrière le camion en travers (RG-09) ;
//   - recul            : Campagne de jour, vue de dessus : le joueur recule à 30 km/h à côté de la police (RG-10).
// Le banc n'utilise que des accroches qui existent aussi sur main (poursuiteBancPolice, poursuiteBancFixe, poursuiteVoitures) :
// le même fichier, compilé sur main, donne les images « avant ». Rend 0 (images seulement ; les mesures sont dans test_police_540).
#pragma once
#include <string>
namespace wfs {
class Renderer;
class Game;
int benchPolice540(Game& g, Renderer& r, const std::string& out);
}
