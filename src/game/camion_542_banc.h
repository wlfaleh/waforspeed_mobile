// WaForSpeed — banc d'essai de WFS-542 (ANO-273 : un vrai camion en travers au lieu du Range Rover Sport agrandi) :
// WFS_BANC=camion-542. Golf du joueur, Corniche, programme fixe de la Poursuite (camion à 1,95 km, file de droite libre),
// légende des mesures sur chaque image (camion542-<scène>-NN) :
//   - nuit     : vue Poursuite de nuit, le joueur arrive dans la file libre (30, 20 puis 12 m du camion), police derrière lui ;
//   - jour     : même scène de jour ;
//   - cockpit  : vue Cockpit de jour, voiture arrêtée à 10 m du flanc du camion, file fermée puis file du milieu ;
//   - vitrine  : de jour, vues de côté et de 3/4 : la Golf (4,28 m) arrêtée à 0,5 m du flanc du camion, pour juger la taille ;
//   - retrait  : joueur arrêté à 30 m du camion : encore là à 55 s, retiré à 61 s après son apparition (RG-04) ; le retrait à
//                50 m derrière le joueur est vérifié par test_camion_542 (le camion n'est déjà plus dessiné à 40 m derrière).
// Le banc n'utilise que des accroches qui existent aussi sur main (poursuiteBancPolice, poursuiteBancFixe) : le même fichier,
// compilé sur main, donne les images « avant » (Range Rover Sport agrandi 1,45 fois). Rend 0 (images ; mesures : test_camion_542).
#pragma once
#include <string>
namespace wfs {
class Renderer;
class Game;
int benchCamion542(Game& g, Renderer& r, const std::string& out);
}
