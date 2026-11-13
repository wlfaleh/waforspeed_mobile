// WaForSpeed — garage (WFS-11, avec WFS-40 : fiche et peintures ; épic E5).
// Port de js/world/garage.js (buildGarage, setGarageCar, renderGarage) et de js/ui/screens.js (renderGarageUI,
// selectPaint, cycleCar, leaveGarage) du jeu web, style css/style.css (.screen--show, .show-head, .show-panel, .specs,
// .statlist, .paint-row, .btn). Système enfichable « Garage » (ordre 440), actif seulement en Ecran::Garage :
//   - scène dédiée (WFS-44) : voiture sur plateau tournant, anneau néon, barres lumineuses, hémisphère + 3 projecteurs
//     (WFS-45), fond #07070c et brouillard 14-40 m ;
//   - fiche (marque, index, modèle, type, accroche, puissance, 0 à 100, vitesse max, 4 barres carStats, crédit) ;
//   - 16 peintures (PAINTS), mémorisées par voiture pendant la session ;
//   - « Choisir » : voiture + peinture retenues pour la course (GameView::choisirVoiture), puis course ; « Retour » : menu.
//
// WFS-534 — GUIDE : QUEL FICHIER TOUCHER POUR QUEL TICKET (le garage est découpé en panneaux pour que plusieurs sessions
// avancent en même temps sans se croiser ; chaque ticket ne touche que son fichier, garage.cpp seulement pour une accroche) :
//   - garage.cpp           écran (système « garage », 440), mise en page, touchers, enchaînement des panneaux ;
//   - garage_commun.*      jetons de couleur, fiches des voitures, zones d'appui, MiseEnPage, interface PanneauGarage ;
//   - garage_couleur.*     rangée des couleurs (principale, puis « toit et rétros »)       → WFS-489 ;
//   - garage_finition.*    finition (emplacement vide)                                     → WFS-491 ;
//   - garage_skin.*        skins (emplacement vide)                                        → WFS-494 ;
//   - garage_achat.*       Choisir / Retour, puis cadenas, prix, « Acheter », « Il te manque N ★ » → WFS-489, 499, 502 ;
//   - garage_fiche.*       en-tête, caractéristiques et barres de la voiture ; textes FR / EN du garage → WFS-507 ;
//   - garage_cadre.*       voiles, flèches, fond vitré du panneau, crédit, « Chargement… » ;
//   - garage_decor.*       décor 3D de la scène dédiée (sol, plateau, anneau, barres, lumières) ;
//   - garage_bancs.cpp     bancs d'essai benchGarage et benchGarageTour.
//   - garage_textes_banc.cpp  banc « textes » du garage et des paquets (WFS-507).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
// centre du bouton du garage nommé (« choisir », « retour », « suivant », « precedent ») à la dernière image : touchers
// simulés du banc d'essai (menu tactile, garage) ; false si le garage n'a pas encore été dessiné
bool garageBouton(const char* nom, float& x, float& y);
// WFS-96 : prochaine ouverture du garage en mode « regarder » (Showroom, Ta voiture) : « Choisir » ramène au menu
void garageRegarder();
const char* garageVoitureAffichee();   // WFS-127 : identifiant de la voiture affichée (son du showroom), nullptr sans garage
// banc d'essai : captures garage-0/1/2 (3 angles), garage-m4 (voiture suivante), garage-peinture-0/1, garage-peintures
// (planche des 16 peintures), garage-course (course avec la voiture et la peinture choisies), puis g.shutdown()
void benchGarage(Game& g, Renderer& r, const std::string& outDir, const std::string& suffixe = "");
void benchGarageTour(Game& g, Renderer& r, const std::string& outDir);   // WFS-404 : une capture par place du garage
int benchGarageCouleurs(Game& g, Renderer& r, const std::string& outDir);   // WFS-489 : garage-couleurs-* (garage_couleur_banc.cpp), rend les écarts
// WFS-507 : banc « textes » du garage (garage_textes_banc.cpp) : captures textes-<fr|en>-garage-*, parcours, garde-fou ; rend le nombre d'échecs
int benchGarageTextes(Game& g, Renderer& r, const std::string& outDir);
} // namespace ecrans
} // namespace wfs
