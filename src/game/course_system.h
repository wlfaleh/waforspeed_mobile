// WaForSpeed — module « course » (WFS-12) branché sur le moteur par les systèmes enfichables (WFS-30).
// Règles : course.h. Ce module tient la course solo de la partie : compte à rebours affiché, chrono, checkpoints,
// score, fin de partie (écran de fin avec « Rejouer » : même carte, même graine de trafic).
// Le HUD de course (vitesse, chrono, score en continu) viendra avec WFS-27 : il lira activeCourse().
#pragma once
#include <string>
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
// WFS-420 (accroche) : banc du téléphone : la course repart du départ, comme « Rejouer » (même graine de trafic)
void courseRejouer();
// WFS-506 (banc « textes ») : centre d'un bouton de l'écran de fin Checkpoint (« rejouer », « menu ») ; faux s'il n'est pas dessiné
bool courseBouton(const std::string& nom, float& x, float& y);
// scènes du banc d'essai de la course (captures course-decompte-*.png, course-fin.png), appelées par desktop_main.cpp
void benchCourseScene(Game& g, Renderer& r, const std::string& outDir);
// WFS-125 (RG-04) : badge « NOUVEAU RECORD » (haut yBadge, si nouveau) et ligne « RECORD » (bas `bas`) centrés sur cx,
// mêmes styles pour les écrans de fin des 3 modes (Checkpoint, Course à 5, Poursuite)
void dessinerRecord(Renderer& r, float cx, float yBadge, float bas, float u, const std::string& valeur, bool nouveau);
// WFS-141 (RG-03, ANO-59) : bouton parallélogramme de l'écran de fin (clip-path du bouton .btn, biais de 14 px en 720 p),
// celui de la fin Checkpoint, commun aux 3 modes (Course à 5 et Poursuite : mêmes boutons, tailles inchangées)
void biseauFin(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c);
// WFS-557 (maquettes R19 31, 32, 33) : mise en page commune des fins Course à 5, Poursuite et Duel, en unités de la maquette
// (1600 × 720, k = H / 720, centrée en largeur) : fenêtre de verre 640 × 556 (score à sa droite, score_fin::poser), en-tête
// (sur-titre gris, titre en italique 84, sous-titre 36), lignes en tuiles de verre (Inter), puis « Retour » (autocollant,
// « Menu principal ») et le bouton rond (Rejouer, Revanche) sous le score. Rectangles écrits dans fen[4], retour[4], action[4].
struct FinR19 { float k = 1, x = 0, y = 0, w = 0, h = 0, ox = 0; };
FinR19 panneauFinR19(Renderer& r, float W, float H);
void enTeteFinR19(Renderer& r, const FinR19& f, const std::string& surTitre, const std::string& titre, const vec4& couleurTitre, const std::string& sousTitre);
// ligne n (0 = première) : texte à gauche, valeur à droite ; moi : liseré et texte orange (toi)
void ligneFinR19(Renderer& r, const FinR19& f, int n, const std::string& gauche, const std::string& droite, bool moi, bool nomPropre = true);
float basLignesFinR19(const FinR19& f, int n);   // y sous la ligne n − 1
void boutonsFinR19(Renderer& r, const FinR19& f, const std::string& action, bool actionAppuyee, float retour[4], float bouton[4]);
// WFS-125 (RG-09) : banc des records et du fantôme (WFS_BANC=records, lancé aussi à la fin du groupe vitrines) :
// captures records-fantome, records-poursuite, records-course5 ; rend le nombre d'écarts du parcours
int benchRecords(Game& g, Renderer& r, const std::string& outDir);
// WFS-318 (accroche) : fantôme du record de la course en cours : distance (unités), latéral (demi-largeurs), durée de la trace
// du record (s) ; faux sans fantôme (pas de record, décompte, trace finie)
bool fantomeCourant(double& d, double& x, double& duree);

} // namespace wfs
