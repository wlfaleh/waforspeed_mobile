// WaForSpeed — WFS-496 (épic E18, zone E8) : atelier d'autocollants, ouvert depuis le garage.
// Règles : regles-de-gestion/WFS-496.md ; logique sans écran (calques, annuler / refaire, motifs gratuits, sauvegarde) :
// atelier_calques.h ; pose et cuisson des autocollants sur la carrosserie : vehicule_autocollants.h et vehicule_skin.h (WFS-495).
//   RG-01 : bouton « Atelier » au garage, sous « Market » (même taille que lui : les boutons du garage ne changent pas) → écran
//           Atelier : la voiture du garage au centre, 5 vues (gauche, droite, avant, arrière, dessus), rotation au doigt (glisser
//           hors de l'autocollant choisi : tourner autour de la voiture).
//   RG-02 : bibliothèque par famille (6 onglets), 10 motifs gratuits, les autres avec le cadenas et le nom de leur pack.
//   RG-03 : glisser sur l'autocollant choisi = le placer (sur le plan de son côté de la voiture) ; deux doigts = tourner et agrandir ;
//           boutons fins ± 5° et ± 5 % ; miroir ; couleur (28) ; supprimer ; annuler / refaire (20 pas).
//   RG-04 : liste des calques (20 au plus, « 20 calques au maximum ») : choisir, monter, descendre, masquer.
//   RG-05 : « Valider » = habillage gardé (sauvegarde) et cuit (WFS-495), retour au garage ; « Quitter » sans valider : confirmation.
//   RG-06 : interface design-street-racing (boutons R19 de WFS-549, verre de WFS-548), français et anglais (ui::tr, clés atelier.*).
// Aperçu : la voiture du garage porte la cuisson des calques visibles ; pendant un geste, seul le contour de l'autocollant choisi
// suit le doigt (interface 2D), la cuisson est refaite quand le doigt se lève (48 à 76 ms sur PC, jamais en course).
// Accroches (garage.cpp, ≤ 10 lignes) : dessinerBoutonGarage / dansBoutonGarage / ouvrir ; ouvert() → toucher() ; camera() après
// le décor ; apercu() au lieu de l'aperçu de la skin. Le panneau est dessiné par le système « atelier » (ordre 467).
#pragma once
#include <string>
#include "../../core/system.h"   // Marges, FrameParams, SceneDediee, GameView

namespace wfs {
class Game;
class Renderer;
struct Model;
struct Touch;
namespace ecrans::atelier {

void ouvrir(const char* carId, int skin);   // skin : indice de BOUTIQUE_SKINS_IDS montrée au garage (−1 : aucune)
void fermer();                              // sans rien garder
bool ouvert();
bool toucher(const Touch& t, const GameView& g);   // tous les touchers du garage tant que l'atelier est ouvert
// scène dédiée du garage : caméra de l'atelier (vue choisie, rotation au doigt) ; rien si fermé
void camera(FrameParams& fp, SceneDediee& s, const GameView& g);
// aperçu de la voiture du garage m (id carId, skin montrée) : atelier ouvert : calques visibles de l'atelier ; fermé : habillage
// gardé (calquesCourse). Faux si aucun autocollant n'est à montrer (le garage montre alors sa skin, comme avant).
bool apercu(Renderer& r, const Model& m, const char* carId, int skin);

// RG-01 : bouton « ATELIER » du garage, sous « MARKET » ; op : fondu d'entrée du garage
void dessinerBoutonGarage(Renderer& r, float W, float H, const Marges& m, float op);
bool dansBoutonGarage(float x, float y);

// ---- banc d'essai (WFS_BANC=atelier, atelier_banc.cpp) : parcours RC-E18-09 et captures ; rend le nombre d'écarts
int benchAtelier(Game& g, Renderer& r, const std::string& out);
bool centre(const char* nom, float& x, float& y);   // centre d'une zone à la dernière image (« bouton », « valider », « motif-3 »…)
const std::string& message();                       // message montré à la dernière image (vide : aucun)
std::string texteAtelier();                         // calques de l'atelier ouvert (texte gardé)
void choisirVue(int v);                              // 0 gauche, 1 droite, 2 avant, 3 arrière, 4 dessus
bool versEcran(float a, float b, float& x, float& y);   // point (cm) du plan du calque choisi → écran (banc : gestes simulés)

} // namespace ecrans::atelier
} // namespace wfs
