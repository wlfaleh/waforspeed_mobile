// WaForSpeed — modes de course (WFS-110, épic E3) : Checkpoint (60 s, +30 s par checkpoint, voir course.h), Course à
// 5 voitures (1 joueur contre 4 ordinateurs, sans trafic), Duel (sans trafic). Le jeu web n'a pas de course contre
// l'ordinateur : la conduite des ordinateurs est un suiveur de route simple (RG-04). La ligne d'arrivée, le classement
// et les contacts reprennent le duel (js/net/duel.js : DUEL_KMS, FINISH = km × 1000 / 0,006, hitGhosts ; WFS-21).
// Calcul seul (aucun rendu) : testé par tests/test_course5.cpp.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "cars.h"
#include "duel.h"
#include "../core/configgame.h"   // WFS-128

namespace wfs {

// ---- mode choisi (RG-09), mémorisé par la sauvegarde (WFS-24 : clés mode, km5)
enum class ModeCourse { Checkpoint, Course5, Poursuite, Duel };
ModeCourse& modeCourse();          // mode de la prochaine course (Checkpoint par défaut)
int& kmCourse5();                  // 2, 4, 6 ou 10 km
const char* nomMode(ModeCourse m);
// pas de trafic : course à 5 voitures (RG-03) et duel (RG-07), seulement pendant la course
bool sansTrafic(bool enCourse);
// RG-01 : panneau de choix du mode, ouvert par « Course solo » au menu (modes_ecran.cpp) ; faux sans sauvegarde
// (banc d'essai : le menu va au garage comme avant), sauf choixModeBanc(true)
bool ouvrirChoixMode();
bool choixModeOuvert();
void choixModeBanc(bool force);
// WFS-125 (RG-09) : banc d'essai : centre d'une zone du panneau des modes (« carte0 »…, « km0 »…, « continuer »), boutons
// de l'écran de fin de la course à 5 (« rejouer », « menu ») et record battu à la dernière course à 5
bool choixModeCentre(const std::string& zone, float& x, float& y);
bool course5Bouton(const std::string& nom, float& x, float& y);
int course5Positions(double* rd, int max);   // WFS-173 (RG-08) : distance des ordinateurs (unités), lecture seule ; rend leur nombre
bool course5NouveauRecord();
class Game; class Renderer;
// banc d'essai (WFS_BANC=course5) : modes-choix, course5-grille, course5-course, course5-fin
void benchCourse5(Game& g, Renderer& r, const std::string& outDir);

namespace course5 {

inline const int& ORDINATEURS = g_configJeu.course5.ordinateurs;   // WFS-128 : 4
enum Difficulte { FACILE = 0, NORMALE = 1, DIFFICILE = 2 };
inline const double (&FACTEUR_VITESSE)[3] = g_configJeu.course5.facteur;   // WFS-128 : 0,80 / 0,90 / 0,97 ;     // part de la vitesse de pointe visée (RG-04)
// WFS-133 (RG-01 à RG-03) : difficulté choisie au panneau des modes (normale par défaut), clé diff5 de la sauvegarde
Difficulte& difficulte();
const char* cleDifficulte(Difficulte d);            // « facile », « normale », « difficile »
Difficulte difficulteDeCle(const std::string& v);   // valeur inconnue → NORMALE
constexpr int NITROS[3] = {0, 1, 2};                // RG-06 : nitros par ordinateur (facile, normale, difficile)
constexpr double DERNIER_KM_M = 1000;               // RG-04 : pas de rattrapage dans le dernier kilomètre
constexpr double FILES[3] = {-0.62, 0.0, 0.62};               // files (demi-largeurs de route), comme le trafic
inline const double& ECART_MIN_M = g_configJeu.course5.ecartMinM;   // WFS-128 : 1,2 ;                           // jamais plus près que cela de la voiture de devant (m)

struct Ordinateur {
    std::string car; unsigned peinture = 0;   // voiture du catalogue, peinture (0xRRGGBB)
    std::string nom;                          // « Golf 8 GTI »… (classement)
    duel::Ghost g;                            // rd (distance, unités), x (latéral), v (vitesse), hitCd, done, ft (ms)
    double cible = 0, cibleX = 0, varVitesse = 1;
    int file = 1;
    Difficulte diff = NORMALE;                // WFS-133 : difficulté du tirage
    int nitros = 1, nitrosPrises = 0;         // RG-06 : nitros restantes, nitros utilisées
    double nitroT = 0;                        // s de nitro restantes (0 : pas de nitro)
    bool aspiration = false;                  // RG-05 : dans l'aspiration à ce pas
};

// WFS-189 (RG-01) : vitesse max de fiche des adversaires à ± 20 % de celle du joueur
constexpr double CLASSE_VMAX = 0.20;
// WFS-189 (RG-01) : indices (CARS) des adversaires possibles du joueur : ceux de sa classe, complétés par les plus proches
std::vector<int> candidats(const char* carJoueur);
// RG-02 : 4 ordinateurs tirés au hasard (modèles différents entre eux et de celui du joueur), peinture aléatoire ; même
// graine = même tirage. WFS-189 (RG-01) : parmi les candidats() de la voiture du joueur
std::vector<Ordinateur> tirer(uint32_t graine, const char* carJoueur, Difficulte d = NORMALE);
// RG-05 : grille de départ de 5 : le joueur derrière, les ordinateurs sur deux rangées devant lui ; WFS-189 (RG-03) :
// rangés par vitesse max décroissante (les plus rapides devant)
void grille(std::vector<Ordinateur>& os, double sJoueur);
// RG-04 : un pas de conduite des ordinateurs (dt en s) ; joueur : distance et écart latéral (les ordinateurs l'évitent) ;
// arrivée à `finish` (unités) avec le temps de course (s). WFS-133 : rattrapage (RG-04), aspiration (RG-05) et nitro
// (RG-06 : nitrosPrises compte les nitros utilisées)
void avancer(std::vector<Ordinateur>& os, double dt, double finish, double raceTime, double sJoueur, double xJoueur, double vJoueur);
// RG-05 : une voiture en (s, x) est dans l'aspiration d'une des voitures de `os` (même file, moins de aspirationM devant)
bool dansAspiration(const std::vector<Ordinateur>& os, double s, double x);
// RG-06 : classement (moi + les ordinateurs), comme renderBoard du duel : arrivés d'abord (par temps), puis par distance
struct Ligne { std::string nom; double d = 0, ft = 0; bool moi = false, fini = false; };
std::vector<Ligne> classement(const std::vector<Ordinateur>& os, const std::string& nomJoueur, double dJoueur, double ftJoueur, bool joueurFini, double finish);
int position(const std::vector<Ligne>& lignes);   // « Position n/5 »

} // namespace course5
} // namespace wfs
