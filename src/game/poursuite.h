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
#include "track.h"

namespace wfs {

class Game; class Renderer;
// RG-04 : nombre de voitures du trafic voulu pendant une poursuite (0 hors poursuite) : accroche de traffic_system.cpp
int poursuiteTrafic(int trafficN, bool enCourse);
// banc d'essai (WFS_BANC=course5) : poursuite-police, poursuite-bouchon, poursuite-surprise, poursuite-hud, poursuite-fin
void benchPoursuite(Game& g, Renderer& r, const std::string& outDir);

namespace poursuite {

// ---- RG-01 / RG-02 : score
constexpr int SCORE_DEPART = 100;
constexpr double KM_ARRIVEE = 10;
constexpr int CHOC_TRAFIC = 5, CHOC_GLISSIERE = 3, CHOC_POLICE = 10;
constexpr double COLLE_M = 12, COLLE_S = 1;          // police à moins de 12 m pendant 1 s : −1 par seconde
constexpr double DELAI_CHOC = 1.0;                   // un même type de choc compte au plus une fois par seconde
// ---- RG-03 : police
constexpr int POLICE_DEPART = 2, POLICE_MAX = 6;
constexpr double RENFORT_KM = 2;
// ---- RG-04 : trafic 2 à 3 fois plus dense, embouteillages de 300 à 600 m à 20-40 km/h
constexpr double DENSITE = 2.5;
constexpr double FILES[3] = {-2.0 / 3.0, 0.0, 2.0 / 3.0};   // = LANE_X du trafic

struct Etat {
    int score = SCORE_DEPART;
    double colle = 0;                 // temps passé avec une police à moins de 12 m (s)
    double cdTrafic = 0, cdGlissiere = 0;
    bool arrete = false, gagne = false;
    int chocsTrafic = 0, chocsGlissiere = 0, chocsPolice = 0, quasi = 0;
};
// un choc : −points (score jamais sous 0) ; 0 = arrêté (défaite immédiate)
void perdre(Etat& e, int points);
// RG-02 : choc avec le trafic / la glissière (au plus un par seconde chacun) ; contact police (délai porté par la police)
void chocTrafic(Etat& e);
void chocGlissiere(Etat& e);
void chocPolice(Etat& e);
// RG-01 : quasi-contact réussi : +1 (jamais au-dessus de 100)
void quasiContact(Etat& e);
// RG-02 : un pas de dt secondes ; dPoliceM = distance (m) de la police la plus proche
void pas(Etat& e, double dt, double dPoliceM);
// RG-01 : arrivée à 10 km avec un score ≥ 1 = victoire
void arrivee(Etat& e, double distKm);
// RG-03 : voitures de police voulues à cette distance : 2 au départ, +1 tous les 2 km, 6 au plus
int policeVoulue(double distKm);

// ---- RG-03 : une voiture de police (distance rd en unités comme Player::dist, x en demi-largeurs de route, v en unités/s)
struct Police {
    double rd = 0, x = 0, v = 0, hitCd = 0;
    double cibleX = 0;
    int role = 0;                     // 0 : derrière (pousse), 1 : à gauche, 2 : à droite
    bool fixe = false;                // barrage (RG-05) : immobile
    unsigned peinture = 0xf2f4f8;     // blanc / bleu marine (voitures existantes repeintes)
};
struct Obstacle { double s = 0, x = 0, v = 0; };   // voiture du trafic (distance, latéral, vitesse)
// un pas de conduite de la police (dt en s, t : temps de course) : rattrape le joueur (sJ, xJ, vJ), se place derrière
// (15 m) ou à côté (18 / 21 m), contourne le trafic ; à tour de rôle, une voiture tente de pousser (ATTAQUE_S secondes
// toutes les CYCLE_S secondes, décalées) ; trop loin derrière (> 250 m) : revient à 120 m (la poursuite ne se perd pas)
constexpr double CYCLE_S = 12, ATTAQUE_S = 3;
bool attaque(const Police& p, int indice, double t);
void avancer(std::vector<Police>& ps, double dt, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs);
// renfort : une voiture de plus, 90 m derrière le joueur
Police renfort(int indice, double sJ, double vJ, uint32_t graine);
// distance (m) de la police mobile la plus proche du joueur (1e9 sans police)
double plusProcheM(const std::vector<Police>& ps, double sJ, double xJ);

// ---- RG-04 / RG-05 : programme de la course tiré par la graine
enum class Surprise { Barrage, Deboite, Travaux, Accident, Nombre };
const char* bandeau(Surprise s);   // texte court du bandeau (« BARRAGE DE POLICE »…)
struct Bouchon { double km = 0, longueurM = 0; int passage = 0; double kmhMin = 20, kmhMax = 40; bool place = false; };
struct Evenement { double km = 0; Surprise type = Surprise::Barrage; int file = 0; bool declenche = false; };
struct Programme { std::vector<Bouchon> bouchons; std::vector<Evenement> surprises; };
// 3 embouteillages (300 à 600 m, une file libre = le passage étroit) et 6 surprises (les 4 types au moins une fois)
// banc : premier embouteillage et première surprise tout près du départ (captures)
Programme programme(uint32_t graine, bool banc = false);

} // namespace poursuite
} // namespace wfs
