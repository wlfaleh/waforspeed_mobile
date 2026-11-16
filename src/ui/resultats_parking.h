// WaForSpeed — WFS-561 (épic E23, R19 maquette 11) : scène 3D de l'écran des résultats. Règles : regles-de-gestion/WFS-561.md.
//   - RG-04 : course à 5, duel et Poursuite (les modes à classement) ; le Checkpoint garde son écran (RG-07) ;
//   - RG-05 : parking souterrain du menu (WFS-552, réutilisé tel quel : ecrans::parking::Parking), tableau d'affichage à LED ambre
//     au mur du fond (POS · PILOTE · TEMPS · ÉCART), son reflet sur le sol, la voiture du joueur (peinture, finition, skin :
//     dessinée par Game), le rang et le temps en haut à gauche ;
//   - RG-02 : vrai panneau LED : caisson en aluminium extrudé thermolaqué, masque noir à visières pare-soleil au-dessus de
//     chaque rangée de points, points ambre 590 nm au pas de 16 mm (tableau_led.h) ; ≈ 3,10 × 1,52 m ;
//   - RG-06 / RG-08 : panneau du score, boutons et fenêtres 2D des maquettes 31 à 33 restent dans les modes ; la fenêtre 2D est
//     le repli quand la scène n'est pas prête (mémoire graphique refusée, RG-C5) ;
//   - RG-C5 : tout est construit au chargement (init) ; pendant l'écran, seuls les points allumés et leur reflet sont réécrits,
//     dans des tampons réservés d'avance (deux tampons tour à tour : jamais celui de l'image encore en vol).
//
// Accroche des modes (quelques lignes dans leur `resultat()` et leur `update`, RG-09) :
//   - à chaque pas tant que leurs résultats sont à l'écran : resultats3d::demander(tableau) ;
//   - dans drawUi : if (resultats3d::affiche()) { panneau du score et boutons seulement } else { fenêtre 2D d'avant (repli) }.
#pragma once
#include <string>
#include <vector>
#include "../core/math.h"
#include "tableau_led.h"

namespace wfs {
class Game; class Renderer;
namespace resultats3d {

// ce que l'écran montre : titre en haut à gauche (« 1er · 2:05,4 », déjà traduit) et lignes du tableau (5 au plus)
struct Tableau { std::string titre; std::vector<led::Ligne> lignes; };

// un mode montre ses résultats à ce pas (copie seulement si le contenu a changé : aucune allocation sinon)
void demander(const Tableau& t);
// course quittée, rejouée, retour au menu : plus de scène des résultats
void effacer();
// vrai si la scène 3D des résultats a pris l'image en cours (à lire dans drawUi) : le mode dessine alors seulement le panneau
// du score et ses boutons ; faux : le mode dessine sa fenêtre 2D (repli, RG-08)
bool affiche();
// scène préparée au chargement (faux : mémoire graphique refusée, repli 2D)
bool prete();
// titre du haut à gauche (« 1er · 2:05,4 ») : dessiné par la scène quand elle est affichée
const std::string& titre();

// ---- placement (repère du parking de WFS-552 : celui de la voiture, mètres, y vers le haut, avant de la voiture vers −Z,
// voile du fond en z = +2,75) ; calcul seul, testé par tests/test_tableau_led.cpp
inline constexpr float CAISSON_L = 3.10f, CAISSON_H = 1.52f, CAISSON_P = 0.14f;   // caisson (m)
inline constexpr float ENTRAXE_MUR = 0.06f;                                       // pattes de fixation (dos du caisson au voile)
inline constexpr float Z_FOND = 5.00f / 2 + 0.25f;    // nu du voile du fond (menu_parking.cpp : PLACE_P / 2 + 0,25 m de bordure)
inline constexpr float Z_DOS = Z_FOND - ENTRAXE_MUR;   // dos du caisson
inline constexpr float Z_AVANT = Z_DOS - CAISSON_P;    // nez du caisson (vers l'allée, −z)
inline constexpr float Z_MASQUE = Z_AVANT + 0.002f;    // masque en retrait de 2 mm derrière le nez du caisson
inline constexpr vec3 CENTRE_TABLEAU{1.57f, 1.06f, 0.f};                            // centre de la face (x, y) ; z : face (voir faceZ)
float faceZ();                                                                    // plan des points (devant le masque)
// voiture du joueur arrêtée en biais dans l'allée, devant le tableau (maquette 11 : vue de trois quarts avant) : position de son
// centre et lacet (rad, rotateY) dans le repère du parking ; le parking est dessiné autour de la voiture, là où elle s'est arrêtée
inline constexpr vec3 POSE_VOITURE{-1.23f, 0.f, -2.86f};
inline constexpr float LACET_VOITURE = -50.f * 3.14159265f / 180.f;   // avant tourné de 50° vers la gauche de l'image
mat4 parkingDepuisVoiture();   // repère de la voiture → repère du parking (inverse de translate(POSE) × rotateY(LACET))
// caméra fixe de l'écran : œil, point regardé, angle vertical (rad), décentrement vertical de l'objectif (NDC)
struct Camera { vec3 oeil, cible; float fovY, decalageY; };
Camera camera(float aspect);
namespace banc { inline bool panneau = false; }   // banc seulement : gros plan du panneau (avis de l'agent 3D)
// centre (monde du parking) du point de la colonne c, rangée r de la matrice
vec3 positionPoint(int c, int r);
// reflet d'un point P sur le sol mouillé (plan y = 0) vu de l'œil E : point du sol où se voit l'image miroir de P (P.y > 0)
vec3 refletSol(const vec3& E, const vec3& P);

// banc WFS_BANC=resultats3d (resultats_parking_banc.cpp) : scène avec des résultats posés à la main (course à 5 1er et 3e,
// duel, Poursuite ; FR et EN ; nuit ; gros plan du panneau ; repli) ; rend le nombre d'écarts
int benchResultats(Game& g, Renderer& r, const std::string& out);

} // namespace resultats3d
} // namespace wfs
