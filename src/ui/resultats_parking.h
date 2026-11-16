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

// ---- aides des accroches des modes (textes traduits, ui::tr) :
// titre « 1er · 2:05,4 » / « 1st · 2:05.4 » (rang ≥ 1, temps en secondes) ; tete : mot du résultat à la place du rang (« VICTOIRE »)
std::string titreRang(int rang, double secondes);
std::string titreMot(const std::string& mot, double secondes);
// ligne d'un classement (course à 5, duel) : le joueur « TOI · <pseudo> » ; arrivé : temps (le 1er) ou écart au 1er ; pas arrivé :
// sa distance (« 1,76 KM », comme la maquette 31) ; texte (parti…) : à la place du temps
led::Ligne ligneClassement(int rang, const std::string& nom, bool moi, bool fini, double secondes, double teteSecondes, double distanceKm,
                           const std::string& texte = "");
// maquette 11 : panneau du score (score_fin) en haut à droite, à la place de la fenêtre 2D des modes
void poserScore(float W, float H, float margeDroite);
// maquette 11 : « MENU » en plaque en bas à gauche, bouton rond (Rejouer, Revanche) en bas à droite ; boutons de WFS-557 (taille
// des maquettes 10 et 11 : plaque 200 × 44, rond de 110, unités 1600 × 720) ; rectangles touchables écrits dans menu[4], bouton[4]
void boutons(Renderer& r, float W, float H, float margeGauche, float margeDroite, const std::string& action, bool actionAppuyee, float menu[4], float bouton[4]);

// ---- placement (repère du parking de WFS-552 : celui de la voiture, mètres, y vers le haut, avant de la voiture vers −Z,
// voile du fond en z = +2,75) ; calcul seul, testé par tests/test_tableau_led.cpp
inline constexpr float CAISSON_L = 3.10f, CAISSON_H = 1.52f, CAISSON_P = 0.14f;   // caisson (m)
inline constexpr float ENTRAXE_MUR = 0.06f;                                       // pattes de fixation (dos du caisson au voile)
inline constexpr float Z_FOND = 5.00f / 2 + 0.25f;    // nu du voile du fond (menu_parking.cpp : PLACE_P / 2 + 0,25 m de bordure)
inline constexpr float Z_DOS = Z_FOND - ENTRAXE_MUR;   // dos du caisson
inline constexpr float Z_AVANT = Z_DOS - CAISSON_P;    // nez du caisson (vers l'allée, −z)
inline constexpr float Z_MASQUE = Z_AVANT + 0.002f;    // masque en retrait de 2 mm derrière le nez du caisson
inline constexpr vec3 CENTRE_TABLEAU{-0.12f, 1.06f, 0.f};                            // centre de la face (x, y) ; z : face (voir faceZ)
float faceZ();                                                                    // plan des points (devant le masque)
// voiture du joueur garée en biais à droite du tableau, l'arrière vers le mur (maquette 11 : vue de trois quarts avant) : position de son
// centre et lacet (rad, rotateY) dans le repère du parking ; le parking est dessiné autour de la voiture, là où elle s'est arrêtée
inline constexpr vec3 POSE_VOITURE{-3.09f, 0.f, -0.35f};   // flanc arrière à 0,1 m du poteau (x = −4, z = 1,6)
inline constexpr float LACET_VOITURE = -78.f * 3.14159265f / 180.f;   // avant tourné de 78° vers la gauche de l'image (40° de plus que la direction de l'œil : trois quarts)
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
// banc WFS_BANC=resultats3d-modes : vraies courses des 3 modes, FR et EN, et repli 2D (resultats_parking_banc.cpp)
int benchResultatsModes(Game& g, Renderer& r, const std::string& out);

} // namespace resultats3d
} // namespace wfs
