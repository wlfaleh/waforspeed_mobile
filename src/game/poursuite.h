// WaForSpeed — mode « Poursuite police » (WFS-111, épic E3) : la police te poursuit, parcours 10 km en gardant au moins
// 1 point sur un score de départ de 100, dans un trafic dense, avec des embouteillages et des surprises.
// Le jeu web n'a pas ce mode : police et surprises sont créées ici, sur le trafic existant (traffic.*, port de
// js/game/traffic.js) et les contacts du duel (duel::contacter, port de hitGhosts de js/net/duel.js).
// Calcul seul (aucun rendu) : testé par tests/test_poursuite.cpp. Branchement sur le moteur : poursuite_system.cpp.
// Valeurs validées par l'utilisateur (2026-11-04), ajustables ici après le test sur téléphone.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "course5.h"   // WFS-119 : ModeCourse (clé de sauvegarde)
#include "track.h"
#include "../core/configgame.h"   // WFS-128

namespace wfs {

class Game; class Renderer;
// RG-04 : nombre de voitures du trafic voulu pendant une poursuite (0 hors poursuite) : accroche de traffic_system.cpp
int poursuiteTrafic(int trafficN, bool enCourse);
// banc d'essai (WFS_BANC=course5) : poursuite-police, poursuite-bouchon, poursuite-surprise, poursuite-hud, poursuite-fin
void benchPoursuite(Game& g, Renderer& r, const std::string& outDir);
// WFS-119 RG-08 (ANO-53) : parcours au doigt de la course à 5 : fin → « Rejouer » → nouvelle course → gaz pris → fin →
// « Menu » (même doigt) → accueil ; capture course5-rejouer ; rend le nombre d'écarts
int benchCourse5Rejouer(Game& g, Renderer& r, const std::string& outDir);
// WFS-125 (RG-09) : banc d'essai : boutons de l'écran de fin (« rejouer », « menu »), record battu, fin forcée
// (1 = arrêté tout de suite, 2 = saut à 150 m de l'arrivée)
bool poursuiteBouton(const std::string& nom, float& x, float& y);
int poursuitePolice(double* rd, bool* fixe, int max);   // WFS-173 (RG-08) : police (distance en unités, barrage), lecture seule
bool poursuiteNouveauRecord();
void poursuiteBancFin(int mode);
void poursuiteBancFixe(bool oui);   // programme et graine fixes (captures stables)

namespace poursuite {

// ---- RG-01 / RG-02 : score
// WFS-128 : valeurs de configgame.json (section poursuite) ; défauts : score 100, 10 km, chocs 5 / 3 / 10, police à 12 m pendant 1 s
inline const int& SCORE_DEPART = g_configJeu.poursuite.scoreDepart;
inline const double& KM_ARRIVEE = g_configJeu.poursuite.kmArrivee;
inline const int &CHOC_TRAFIC = g_configJeu.poursuite.chocTrafic, &CHOC_GLISSIERE = g_configJeu.poursuite.chocGlissiere, &CHOC_POLICE = g_configJeu.poursuite.chocPolice;
inline const double &COLLE_M = g_configJeu.poursuite.colleM, &COLLE_S = g_configJeu.poursuite.colleS;          // police à moins de 12 m pendant 1 s : −1 par seconde
constexpr double DELAI_CHOC = 1.0;                   // un même type de choc compte au plus une fois par seconde
// ---- RG-03 : police
inline const int &POLICE_DEPART = g_configJeu.poursuite.policeDepart, &POLICE_MAX = g_configJeu.poursuite.policeMax;   // WFS-128 : 2, 6
inline const double& RENFORT_KM = g_configJeu.poursuite.renfortKm;   // WFS-128 : 2
// ---- RG-04 : trafic 2 à 3 fois plus dense, embouteillages de 300 à 600 m à 20-40 km/h
inline const double& DENSITE = g_configJeu.poursuite.densite;   // WFS-128 : 2,5
constexpr double FILES[3] = {-2.0 / 3.0, 0.0, 2.0 / 3.0};   // = LANE_X du trafic

struct Etat {
    int score = SCORE_DEPART;
    double colle = 0;                 // temps passé avec une police à moins de 12 m (s)
    double cdTrafic = 0, cdGlissiere = 0;
    bool arrete = false, gagne = false;
    int chocsTrafic = 0, chocsGlissiere = 0, chocsPolice = 0, quasi = 0;
    int ralentisseurs = 0;            // WFS-119 : ralentisseurs passés trop vite
    // WFS-338 (RG-03, ANO-193) : chaque perte de points garde sa raison, écrite à l'écran (« −5 CHOC AVEC LE TRAFIC »)
    int pertes = 0;                   // nombre de pertes depuis le départ (le HUD voit ainsi chaque nouvelle perte)
    int dernierePerte = 0;            // points de la dernière perte
    const char* raison = "";          // raison de la dernière perte
};
// WFS-338 (RG-03) : raisons d'une perte de points, telles qu'elles s'écrivent à l'écran
inline constexpr const char* RAISON_TRAFIC = "CHOC AVEC LE TRAFIC";
inline constexpr const char* RAISON_GLISSIERE = "CHOC CONTRE LA GLISSIÈRE";
inline constexpr const char* RAISON_POLICE = "CHOC AVEC LA POLICE";
inline constexpr const char* RAISON_COLLE = "POLICE AU CONTACT";
inline constexpr const char* RAISON_RALENTISSEUR = "RALENTISSEUR TROP VITE";
inline constexpr const char* RAISON_PLOT = "CÔNE DE CHANTIER";
inline constexpr const char* RAISON_CAMION = "CHOC CONTRE LE CAMION";
// un choc : −points (score jamais sous 0) ; 0 = arrêté (défaite immédiate) ; WFS-338 : raison obligatoire (RG-03)
void perdre(Etat& e, int points, const char* raison);
// RG-02 : choc avec le trafic / la glissière (au plus un par seconde chacun) ; contact police (délai porté par la police)
void chocTrafic(Etat& e, const char* raison = RAISON_TRAFIC);
void chocGlissiere(Etat& e, const char* raison = RAISON_GLISSIERE);
void chocPolice(Etat& e);
// WFS-338 (RG-03) : texte de la perte affiché sous le score : « −5 CHOC AVEC LE TRAFIC »
std::string textePerte(int points, const char* raison);
constexpr double PERTE_S = 2.0;   // durée d'affichage de la raison d'une perte (s)
// RG-01 : quasi-contact réussi : +1 (jamais au-dessus de 100)
void quasiContact(Etat& e);
// RG-02 : un pas de dt secondes ; dPoliceM = distance (m) de la police la plus proche
void pas(Etat& e, double dt, double dPoliceM);
// WFS-119 RG-01 : ralentisseur passé à kmh : −3 au-dessus de 90 km/h (comme un plot), rien dessous ; rend les points perdus
constexpr double RALENTISSEUR_KMH = 90;
constexpr int CHOC_RALENTISSEUR = 3;
int ralentisseur(Etat& e, double kmh);
// RG-01 : arrivée à 10 km avec un score ≥ 1 = victoire
void arrivee(Etat& e, double distKm);
// RG-03 : voitures de police voulues à cette distance : 2 au départ, +1 tous les 2 km, 6 au plus
int policeVoulue(double distKm);

// ---- RG-03 : une voiture de police (distance rd en unités comme Player::dist, x en demi-largeurs de route, v en unités/s)
struct Police {
    double rd = 0, x = 0, v = 0, hitCd = 0;
    double rdPrec = 0, xPrec = 0;     // WFS-221 (accroche) : état avant le dernier pas (rendu interpolé)
    double cibleX = 0;
    int role = 0;                     // 0 : derrière (pousse), 1 : à gauche, 2 : à droite
    bool fixe = false;                // barrage (RG-05) : immobile
    unsigned peinture = 0xf2f4f8;     // blanc / bleu marine (voitures existantes repeintes)
};
struct Obstacle { double s = 0, x = 0, v = 0; };   // voiture du trafic (distance, latéral, vitesse)
// un pas de conduite de la police (dt en s, t : temps de course) : rattrape le joueur (sJ, xJ, vJ), se place derrière
// (15 m) ou à côté (18 / 21 m), contourne le trafic ; à tour de rôle, une voiture tente de pousser (ATTAQUE_S secondes
// toutes les CYCLE_S secondes, décalées) ; trop loin derrière (> PERDUE_M) : revient à APPARITION_M (la poursuite ne se perd pas)
constexpr double CYCLE_S = 12, ATTAQUE_S = 3;
// WFS-338 (RG-05, ANO-195) : la police n'apparaît jamais d'un coup ni collée au joueur. Elle est dessinée jusqu'à
// DESSIN_DERRIERE_M derrière lui (rétroviseur, vue arrière) ; elle naît (renfort) et revient (trop loin) à APPARITION_M
// derrière, hors de ce rayon, donc hors de toute vue ; elle se rapproche ensuite à vitesse relative bornée (APPROCHE_*),
// d'autant plus lente qu'elle est près ; à côté du joueur, elle garde toujours une file d'écart (COTE_X), jamais par-dessus.
constexpr double DESSIN_DERRIERE_M = 160, DESSIN_DEVANT_M = 300;
constexpr double APPARITION_M = 220, PERDUE_M = 300;
constexpr double APPROCHE_MIN = 4, APPROCHE_PENTE = 0.3, APPROCHE_MAX = 45;   // m/s : 4 + 0,3 × distance, 45 au plus
double approcheMax(double relM);   // vitesse de rapprochement permise (m/s) à relM mètres derrière le joueur
constexpr double CONTACT_L_M = 4.1, CONTACT_W_M = 1.9;   // contact entre deux voitures (m) : longueur et largeur moyennes
constexpr double COTE_X = 0.62, BORD_X = 0.92;   // écart latéral à côté du joueur, bord de la chaussée (demi-largeurs)
// file visée par une voiture « à côté » (role 1 : à gauche, 2 : à droite) ; côté impossible (bord) : l'autre côté ;
// aucun des deux : derrière le joueur (xJ) ; *cote = false dans ce dernier cas
double placeCote(double xJ, int role, bool* cote = nullptr);
bool attaque(const Police& p, int indice, double t);
void avancer(std::vector<Police>& ps, double dt, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs);
// renfort : une voiture de plus, APPARITION_M derrière le joueur (WFS-338 : avant 90 m, en vue dans le rétroviseur)
Police renfort(int indice, double sJ, double vJ, uint32_t graine);
// distance (m) de la police mobile la plus proche du joueur (1e9 sans police)
double plusProcheM(const std::vector<Police>& ps, double sJ, double xJ);
// WFS-338 (RG-02, ANO-192) : chiffre de l'encart POLICE : distance à la police la plus proche (« 45 M », « 1,20 KM », « — »)
std::string texteDistancePolice(double m);

// ---- RG-04 / RG-05 : programme de la course tiré par la graine
enum class Surprise { Barrage, Deboite, Travaux, Accident, Camion, Ralentisseur, Nombre };   // WFS-119 : Camion, Ralentisseur
constexpr int TYPES_SURPRISE = (int)Surprise::Nombre;   // 6
const char* bandeau(Surprise s);   // texte court du bandeau (« BARRAGE DE POLICE »…)
struct Bouchon { double km = 0, longueurM = 0; int passage = 0; double kmhMin = 20, kmhMax = 40; bool place = false; };
struct Evenement { double km = 0; Surprise type = Surprise::Barrage; int file = 0; bool declenche = false, rappel = false, place = false; };   // rappel : WFS-119 ; place : WFS-338
// WFS-338 (RG-01, RG-05) : les obstacles d'une surprise (barrage, chantier, accident, camion, ralentisseur) sont posés
// PLACE_M avant, au-delà de la distance de dessin (DESSIN_DEVANT_M) : ils ne surgissent jamais à l'écran ; le bandeau
// vient ensuite, ANNONCE_M avant (chantier : ANNONCE_TRAVAUX_M, quand les cônes se voient)
constexpr double PLACE_M = 450, ANNONCE_M = 260, ANNONCE_TRAVAUX_M = 120;   // WFS-338 : chantier : bandeau à 120 m, tenu jusqu'au biseau
bool sePlaceAvant(Surprise s);   // vrai : obstacles posés PLACE_M avant (tout sauf la voiture qui déboîte)
// WFS-338 (RG-01, ANO-191) : chantier réel : biseau de cônes en travers de la file fermée, puis cônes le long de la file
// voisine tous les 10 m ; panneau « travaux » (AK5) devant le biseau. Positions (distance en m depuis le début, x en
// demi-largeurs) ; la file fermée est entièrement barrée (aucun passage entre deux cônes du biseau)
struct Cone { double dM = 0, x = 0; };
std::vector<Cone> chantier(int file);
constexpr double PANNEAU_AVANT_M = 30;   // panneau AK5, posé avant le biseau, sur le côté de la file fermée
struct Programme { std::vector<Bouchon> bouchons; std::vector<Evenement> surprises; };
// 3 embouteillages (300 à 600 m, une file libre = le passage étroit) et 6 surprises (les 6 types, une fois chacun)
// banc : premier embouteillage et première surprise tout près du départ (captures)
Programme programme(uint32_t graine, bool banc = false);
// banc : ralentisseur et camion en travers après l'embouteillage du banc (captures poursuite-ralentisseur, poursuite-camion)
constexpr double BANC_RALENTISSEUR_KM = 1.55, BANC_CAMION_KM = 1.95;
constexpr double BANC_TRAVAUX_KM = 2.20;   // WFS-338 : banc : chantier sur la file de gauche (capture poursuite-travaux)

// ---- WFS-119 RG-01 : « CAMION EN TRAVERS » : un gros véhicule existant (x5, rrsport si le joueur roule en x5), agrandi,
// posé en travers à cheval sur 2 files ; la file `fileLibre` (0 ou 2) reste libre
constexpr float CAMION_ECHELLE = 1.45f;
constexpr double RAPPEL_M = 110;   // camion et ralentisseur : bandeau rappelé 110 m avant (obstacle à éviter / à passer lentement)
const char* camionCar(const char* carJoueur);
int fileLibreCamion(int file);   // 0 ou 2 (la file du milieu ne peut pas rester seule libre)
struct Travers { double xM = 0, demiX = 0, demiZ = 0; };   // centre latéral (m), demi-largeur couverte (m), demi-épaisseur (m)
Travers camionTravers(int fileLibre, double longueurM, double largeurM);

// ---- WFS-119 RG-03 : mode gardé dans la sauvegarde (clé mode) : « checkpoint », « course5 », « poursuite » ;
// inconnu ou absent (ancienne sauvegarde) : Checkpoint
std::string cleMode(ModeCourse m);
ModeCourse modeDeCle(const std::string& cle);

// ---- WFS-141 : rendu de la poursuite et des écrans de fin (calcul seul, testé par tests/test_poursuite.cpp)
// RG-01 (ANO-56) : gyrophares : rouge #FF2D55 et bleu #2D7CFF allumés l'un après l'autre à 2 Hz (0,25 s chacun), jamais
// ensemble (plus de rose) ; une voiture sur deux en opposition de phase ; halo de 1,5 × la largeur de la lampe
constexpr unsigned GYRO_ROUGE = 0xFF2D55, GYRO_BLEU = 0x2D7CFF;
constexpr float HALO_GYRO = 1.5f;   // diamètre du halo / largeur de la lampe
struct Gyrophare { bool rouge = false, bleu = false; };
Gyrophare gyrophare(double t, int voiture);
// RG-02 (ANO-57) : bandeau d'annonce effacé 1,5 s après l'annonce (taille inchangée : ordre permanent du 2026-11-06)
constexpr double BANDEAU_S = 1.5;
// RG-03 (ANO-59) : titre de l'écran de fin de la course à 5 : « VICTOIRE », puis « 2e PLACE », « 3e PLACE »…
std::string titrePlace(int rang);

} // namespace poursuite
} // namespace wfs
