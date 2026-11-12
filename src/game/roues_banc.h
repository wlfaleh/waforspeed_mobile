// WaForSpeed — scènes du banc d'essai des roues animées (WFS-60) : roues-arret-1.png, roues-arret-2.png (identiques),
// roues-roule.png (jantes tournées), roues-virage.png (roues avant braquées, arrière droites)
#pragma once
#include <string>
namespace wfs {
class Renderer;
class Game;
namespace fantome { struct Trace; }
void benchRoues(Renderer& r, const std::string& out);
// WFS-452 (RG-06, RG-08, RG-09) : banc « roues-suivies » (WFS_BANC=roues-suivies) : suites d'images à 30 img/s, rejouées
// « avant » (roues::g_avant452 : roues figées, braquage du trafic d'avant) puis « après », même trajet, Ville de jour :
//   - c5     : adversaire de la Course à 5 à côté du joueur, vers 30 km/h, roue arrière vue de côté (et c5-nuit, de nuit) ;
//   - police : voiture de police de la Poursuite qui glisse de côté vers le joueur, vue de 3/4 avant ;
//   - fantome : fantôme du record (Checkpoint) à côté du joueur, vers 30 km/h, vu de côté ;
//   - trafic : voiture du trafic devant le joueur, vue de face : ligne droite, puis changement de file forcé.
// Captures roues-<scène>-<avant|apres>-NN.png ; durée du rendu de chaque passe au journal (RG-05). Rend le nombre d'écarts.
int benchRouesSuivies(Game& g, Renderer& r, const std::string& out);
// accroches du banc (WFS-452), définies avec leur système : police tenue à côté du joueur (poursuite_system.cpp), fantôme
// donné par le banc (course_system.cpp)
void poursuiteBancPolice(bool actif, double ecartM, double x);
void courseBancFantome(const fantome::Trace& t);
}
