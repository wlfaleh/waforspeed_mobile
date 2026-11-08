// WaForSpeed — modes de course (WFS-110, épic E3) : Checkpoint (60 s, +30 s par checkpoint, voir course.h), Course à
// 5 voitures (1 joueur contre 4 ordinateurs, sans trafic), Duel (sans trafic). Le jeu web n'a pas de course contre
// l'ordinateur : la conduite des ordinateurs est un suiveur de route simple (RG-04). La ligne d'arrivée, le classement
// et les contacts reprennent le duel (js/net/duel.js : DUEL_KMS, FINISH = km × 1000 / 0,006, hitGhosts ; WFS-21).
// Calcul seul (aucun rendu) : testé par tests/test_course5.cpp.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "cars.h"
#include "duel.h"
#include "../core/configgame.h"   // WFS-128
#include "track.h"   // WFS-189 : virages (RG-02 bis)

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
class Game; class Renderer; struct Player;
// banc d'essai (WFS_BANC=course5) : modes-choix, course5-grille, course5-course, course5-fin
void benchCourse5(Game& g, Renderer& r, const std::string& outDir);

namespace course5 {

inline const int& ORDINATEURS = g_configJeu.course5.ordinateurs;   // WFS-128 : 4
enum Difficulte { FACILE = 0, NORMALE = 1, DIFFICILE = 2 };
inline const double (&FACTEUR_VITESSE)[3] = g_configJeu.course5.facteur;   // WFS-128 : 0,80 / 0,90 / 0,97 ; WFS-327 : 0,80 / 0,95 / 1,00 ;     // part de la vitesse de pointe visée (RG-04)
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
    double gardeFile = 0;                     // WFS-243 (ANO-150) : s restantes avant de pouvoir changer encore de voie
    int fileQuittee = -1;                     // WFS-243 (ANO-150) : file quittée au dernier changement (−1 : aucun)
    int changements = 0;                      // WFS-243 (ANO-150) : changements de voie depuis la grille (banc, tests)
};

// WFS-189 (RG-01) : vitesse max de fiche des adversaires à ± 20 % de celle du joueur
constexpr double CLASSE_VMAX = 0.20;
// WFS-189 (RG-01) : indices (CARS) des adversaires possibles du joueur : ceux de sa classe, complétés par les plus proches
std::vector<int> candidats(const char* carJoueur);
// RG-02 : 4 ordinateurs tirés au hasard (modèles différents entre eux et de celui du joueur), peinture aléatoire ; même
// graine = même tirage. WFS-189 (RG-01) : parmi les candidats() de la voiture du joueur
std::vector<Ordinateur> tirer(uint32_t graine, const char* carJoueur, Difficulte d = NORMALE);
// WFS-242 (RG-01) : grille de départ réelle, derrière la ligne de départ (damier de decor_carte.cpp, en s = 0) : PLACES places
// en quinconce, 2 par rang, dans la voie de gauche et la voie de droite (FILES[0] et FILES[2], axes des voies de la chaussée à
// 3 voies : 9,3 m entre les deux axes, ≥ 7 m entre deux carrosseries côte à côte). Quinconce de la FIA : chaque place est
// DECALAGE_M (8 m) derrière la précédente, en changeant de voie ; un rang (2 places) fait donc RANG_M = 16 m. Ainsi deux voitures
// d'une même voie ont plus de 11 m de pare-chocs à pare-chocs (WFS-189 RG-03 : plus de 8 m au départ ; avec des rangs de 8 m,
// des ordinateurs restaient 3 à 4 s collés à moins de 8 m). La 1re place a son centre RECUL_M derrière la ligne : même la voiture
// la plus longue du catalogue (4,9 m) a son avant à plus de 1 m derrière la ligne.
constexpr int PLACES = 5;
constexpr double DECALAGE_M = 8, RANG_M = 2 * DECALAGE_M, RECUL_M = 3.5;
struct Place { double s = 0, x = 0; int file = 0; };   // distance (unités ; < 0 : derrière la ligne), écart latéral (demi-largeurs), file
// k = 0 (devant) … PLACES − 1 (derrière) ; hors bornes : ramené dans [0, PLACES − 1]. Rang k / 2 ; place paire à gauche, impaire
// à droite et DECALAGE_M plus loin derrière (quinconce). En ligne : le Duel (duel.cpp) l'envoie dans la présence du départ
inline Place placeGrille(int k) {
    k = std::clamp(k, 0, PLACES - 1);
    Place p;
    p.file = k % 2 ? 2 : 0;
    p.x = FILES[p.file];
    p.s = -(RECUL_M + RANG_M * (k / 2) + DECALAGE_M * (k % 2)) / M;
    return p;
}
// WFS-242 (RG-02) : place du joueur tirée au sort parmi les 5, d'après la graine de la course (même graine = même place)
int tirerPlaceJoueur(uint32_t graine);
// RG-05, WFS-242 (RG-01) : les ordinateurs prennent les places de la grille laissées par le joueur (placeJoueur ; < 0 : toutes
// les places), dans l'ordre WFS-189 (RG-03) : 0-100 de fiche croissant (à égalité, la plus forte vitesse max devant)
void grille(std::vector<Ordinateur>& os, int placeJoueur);
// WFS-242 (RG-01, RG-05) : pose la voiture du joueur à l'arrêt sur une place de la grille (Course à 5 et Duel) : distance,
// position sur la piste, écart latéral, cap droit. La conduite la prend telle quelle au pas suivant (Player::modeleMonde)
void placerSurGrille(Player& pl, const Track& t, const Place& p);
// WFS-242 (RG-07) : vrai quand la Course à 5 est prête à lancer le compte à rebours (modèles des ordinateurs chargés, ou pas
// de Course à 5 en cours) : le « 3 » n'apparaît pas pendant le chargement, figé, sans les commandes
bool course5PreteAuDepart();
// WFS-242 (RG-03) : pendant les GARDE_VOIE_S premières secondes de course, les ordinateurs gardent leur voie (aucun changement
// de file au départ : pas de choc dans la mêlée) ; ils suivent la voiture de devant
constexpr double GARDE_VOIE_S = 3;
// WFS-243 (RG-ANO-150) : un changement de voie décidé est gardé jusqu'à l'arrivée dans la nouvelle file, et au moins
// GARDE_CHANGEMENT_S ; pas de retour vers la file quittée tant qu'une voiture y est à moins de RETOUR_LIBRE_M devant
// (avant : la voiture « devant » était cherchée sur la position x, qui rejoint la nouvelle file lentement : cibleX faisait
// des allers-retours entre deux files à chaque pas quand les deux étaient libres)
constexpr double GARDE_CHANGEMENT_S = 2, RETOUR_LIBRE_M = 30;
// RG-04 : un pas de conduite des ordinateurs (dt en s) ; joueur : distance et écart latéral (les ordinateurs l'évitent) ;
// arrivée à `finish` (unités) avec le temps de course (s). WFS-133 : rattrapage (RG-04), aspiration (RG-05) et nitro
// (RG-06 : nitrosPrises compte les nitros utilisées)
// WFS-189 (RG-02 bis) : `piste` donnée : les ordinateurs ralentissent avant les virages serrés (rond-point, épingle…) à la
// vitesse où la voiture du joueur reste sur la chaussée (vitesseVirage), avec le freinage du joueur ; nullptr : sans virages
void avancer(std::vector<Ordinateur>& os, double dt, double finish, double raceTime, double sJoueur, double xJoueur, double vJoueur, const Track* piste = nullptr);
constexpr double MARGE_VIRAGE = 0.9;      // WFS-189 (RG-02 bis) : part du cap max utilisée en courbe
constexpr double DERIVE_MAX = 0.8;        // WFS-189 (RG-02 bis) : dérive latérale tolérée (largeurs de route) dans un virage
constexpr double COURBURE_VIRAGE = 4;     // WFS-189 (RG-02 bis) : courbure au-delà de laquelle un segment compte comme virage
constexpr double PLAFOND_JOUEUR = 1.10;   // WFS-189 (RG-02 ter) : vitesse visée ≤ facteur × 1,10 × pointe du joueur
double vitesseVirage(const Track& t, double s);   // WFS-189 (RG-02 bis) : vitesse permise (unités/s) en s
double vitesseVirageOrdinateur(const Track& t, double s, const CarDef& c);   // WFS-327 (RG-01) : vitesse permise à un ordinateur (unités/s)
double vitesseVirageOrdinateurDevant(const Track& t, double s, const CarDef& c);   // WFS-327 : la même, en regardant 4 et 8 m plus loin (ordinateurs)
// RG-05 : une voiture en (s, x) est dans l'aspiration d'une des voitures de `os` (même file, moins de aspirationM devant)
bool dansAspiration(const std::vector<Ordinateur>& os, double s, double x);
// RG-06 : classement (moi + les ordinateurs), comme renderBoard du duel : arrivés d'abord (par temps), puis par distance
struct Ligne { std::string nom; double d = 0, ft = 0; bool moi = false, fini = false; };
std::vector<Ligne> classement(const std::vector<Ordinateur>& os, const std::string& nomJoueur, double dJoueur, double ftJoueur, bool joueurFini, double finish);
int position(const std::vector<Ligne>& lignes);   // « Position n/5 »

// WFS-189 (RG-04, RG-05, ANO-100) : en Course à 5 et en Duel (sans trafic, donc sans la recharge de traffic.cpp), la jauge de
// nitro du joueur se recharge au temps (+ config().nitro.rechargeTemps par seconde de course) et de
// config().nitro.rechargeDepassement à chaque adversaire dépassé (Course à 5 seulement : l'appelant donne 0 en Duel).
// Rien pendant que la nitro brûle ; plafond 1. Checkpoint et Poursuite n'appellent pas cette fonction.
inline double rechargerNitro(double nitro, bool brule, double dt, int depasses) {
    if (brule) return nitro;
    const auto& n = g_configJeu.nitro;
    return std::min(1.0, nitro + n.rechargeTemps * dt + n.rechargeDepassement * depasses);
}
// adversaires dépassés par le joueur : un ordinateur qui passe derrière lui compte une fois ; il ne compte de nouveau que
// s'il est d'abord repassé à plus de REARMEMENT_M devant (deux voitures roue dans roue ne remplissent pas la jauge)
constexpr double REARMEMENT_M = 10;
struct SuiviDepassements {
    std::vector<bool> arme;                                       // vrai : l'ordinateur k peut être dépassé (compté)
    void reset() { arme.clear(); }                                // nouvelle grille (préparé au premier compter())
    int compter(const std::vector<Ordinateur>& os, double sJoueur);   // dépassements à ce pas
};

} // namespace course5
} // namespace wfs
