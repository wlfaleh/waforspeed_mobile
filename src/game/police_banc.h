// WaForSpeed — banc d'essai de la police de la Poursuite (WFS-461, ANO-261, ANO-262) : WFS_BANC=police.
// Corniche, de jour, Golf du joueur, 208 de police. Suites d'images (Release du banc, dossier de sortie) :
//   - police-flanc-NN       : vers 30 km/h, voiture de police tenue contre le flanc gauche du joueur, le joueur braque vers
//                              elle 0,5 s puis lâche le volant (vue Poursuite) ; au journal et sur l'image : déplacement de
//                              côté et rotation de la voiture du joueur depuis le contact ;
//   - police-flanc-cockpit-NN : même scène, vue Cockpit (comme la vidéo de l'utilisateur du 2026-11-12, 12:04) ;
//   - police-rattrapage-NN  : joueur vers 150 km/h, la police rattrape (vue de devant, la police derrière le joueur) ;
//                              vitesse de la police écrite sur l'image et au journal (fiche de la 208 : 208 km/h).
// Le banc n'utilise que des accroches qui existent aussi sur main (poursuiteBancPolice, poursuitePolice) : le même fichier,
// compilé sur main, donne les images « avant ». Rend le nombre d'écarts (police au-delà de sa fiche + 1 %).
#pragma once
#include <string>
namespace wfs {
class Renderer;
class Game;
int benchPolice(Game& g, Renderer& r, const std::string& out);
}
