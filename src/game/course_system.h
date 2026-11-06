// WaForSpeed — module « course » (WFS-12) branché sur le moteur par les systèmes enfichables (WFS-30).
// Règles : course.h. Ce module tient la course solo de la partie : compte à rebours affiché, chrono, checkpoints,
// score, fin de partie (écran de fin avec « Rejouer » : même carte, même graine de trafic).
// Le HUD de course (vitesse, chrono, score en continu) viendra avec WFS-27 : il lira activeCourse().
#pragma once
#include <string>
#include <vector>
#include "course.h"

namespace wfs {

class Game;
class Renderer;

// course de la partie active (nullptr si aucune) : lecture pour le trafic, le HUD (WFS-27) et le banc d'essai
const Course* activeCourse();
Course* activeCourseMut();   // module trafic seulement : immobilisation au départ et événements des chocs
// ralenti après un accident : durée réelle de l'image → durée de jeu (appelé par Game::update)
double courseScaleDt(double raw);
// banc d'essai : false = départ lancé, temps illimité, sans ralenti (scènes des autres tickets inchangées) ;
// la scène de la course (benchCourseScene) remet les règles. Vrai par défaut (jeu).
void courseBenchRules(bool on);
bool courseBenchRulesOn();   // WFS-70 : état à remettre après une scène
// banc d'essai : temps restant forcé (pour voir la fin de partie)
void courseBenchSetTime(double seconds);
// scènes du banc d'essai de la course (captures course-decompte-*.png, course-fin.png), appelées par desktop_main.cpp
void benchCourseScene(Game& g, Renderer& r, const std::string& outDir);
// WFS-125 (RG-04) : badge « NOUVEAU RECORD » (haut yBadge, si nouveau) et ligne « RECORD » (bas `bas`) centrés sur cx,
// mêmes styles pour les écrans de fin des 3 modes (Checkpoint, Course à 5, Poursuite)
void dessinerRecord(Renderer& r, float cx, float yBadge, float bas, float u, const std::string& valeur, bool nouveau);
// WFS-141 (RG-03, ANO-59) : gabarit commun des écrans de fin (Checkpoint, Course à 5, Poursuite), celui de la fin
// Checkpoint : voile #0A0B10, panneau de 120 u au plus (86 % de la largeur) avec liseré orange, sur-titre cyan 4,2 u,
// grand titre 18 u en italique avec halo, statistiques (libellé 3,4 u, valeur 6,5 u, rangées de `parRangee`), classement
// en deux colonnes (3,4 u), ligne « RECORD » et badge, boutons « Rejouer » / « Menu » en parallélogrammes (biseau 14 px).
// Le panneau grandit (et remonte) seulement si le contenu dépasse une rangée de statistiques : Checkpoint inchangé.
struct ZoneFin {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
struct StatFin { std::string libelle, valeur; };              // libellé en capitales grises, valeur blanche
struct LigneFin { std::string gauche, droite; vec4 cg, cd; };   // ligne du classement (course à 5)
struct EcranFin {
    std::string surTitre, titre;   // déjà en capitales (« 2e PLACE » : jamais « 2E »)
    vec4 couleurTitre;
    std::vector<StatFin> stats;
    int parRangee = 4;
    std::vector<LigneFin> lignes;
    bool record = false, nouveauRecord = false;   // record : sauvegarde active (ligne « RECORD »)
    std::string valeurRecord;
};
struct BoutonsFin { ZoneFin rejouer, menu; };
BoutonsFin dessinerEcranFin(Renderer& r, float W, float H, float u, const EcranFin& e);
// WFS-125 (RG-09) : banc des records et du fantôme (WFS_BANC=records, lancé aussi à la fin du groupe vitrines) :
// captures records-fantome, records-poursuite, records-course5 ; rend le nombre d'écarts du parcours
int benchRecords(Game& g, Renderer& r, const std::string& outDir);

} // namespace wfs
