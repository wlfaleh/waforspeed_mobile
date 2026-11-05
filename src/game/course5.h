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
class Game; class Renderer;
// banc d'essai (WFS_BANC=course5) : modes-choix, course5-grille, course5-course, course5-fin
void benchCourse5(Game& g, Renderer& r, const std::string& outDir);

namespace course5 {

constexpr int ORDINATEURS = 4;
enum Difficulte { FACILE = 0, NORMALE = 1, DIFFICILE = 2 };
constexpr double FACTEUR_VITESSE[3] = {0.80, 0.90, 0.97};     // part de la vitesse de pointe visée (RG-04)
constexpr Difficulte DIFFICULTE = NORMALE;                    // RG-04 : normale par défaut
constexpr double FILES[3] = {-0.62, 0.0, 0.62};               // files (demi-largeurs de route), comme le trafic
constexpr double ECART_MIN_M = 1.2;                           // jamais plus près que cela de la voiture de devant (m)

struct Ordinateur {
    std::string car; unsigned peinture = 0;   // voiture du catalogue, peinture (0xRRGGBB)
    std::string nom;                          // « Golf 8 GTI »… (classement)
    duel::Ghost g;                            // rd (distance, unités), x (latéral), v (vitesse), hitCd, done, ft (ms)
    double cible = 0, cibleX = 0, varVitesse = 1;
    int file = 1;
};

// RG-02 : 4 ordinateurs tirés au hasard parmi les 20 voitures (modèles différents entre eux et de celui du joueur),
// peinture aléatoire ; même graine = même tirage
std::vector<Ordinateur> tirer(uint32_t graine, const char* carJoueur, Difficulte d = DIFFICULTE);
// RG-05 : grille de départ de 5 : le joueur derrière, les ordinateurs sur deux rangées devant lui
void grille(std::vector<Ordinateur>& os, double sJoueur);
// RG-04 : un pas de conduite des ordinateurs (dt en s) ; joueur : distance et écart latéral (les ordinateurs l'évitent) ;
// arrivée à `finish` (unités) avec le temps de course (s)
void avancer(std::vector<Ordinateur>& os, double dt, double finish, double raceTime, double sJoueur, double xJoueur, double vJoueur);
// RG-06 : classement (moi + les ordinateurs), comme renderBoard du duel : arrivés d'abord (par temps), puis par distance
struct Ligne { std::string nom; double d = 0, ft = 0; bool moi = false, fini = false; };
std::vector<Ligne> classement(const std::vector<Ordinateur>& os, const std::string& nomJoueur, double dJoueur, double ftJoueur, bool joueurFini, double finish);
int position(const std::vector<Ligne>& lignes);   // « Position n/5 »

} // namespace course5
} // namespace wfs
