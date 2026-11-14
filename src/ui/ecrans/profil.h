// WaForSpeed — écran « Profil » (WFS-482, épic E17, zone E8 ; accroche E8 : bouton « Profil » du menu principal).
// Le joueur voit son identité et son historique (données de WFS-480, src/game/profil.h), et peut changer son pseudo et sa
// plaque avec le formulaire et le clavier de WFS-481 (creation_profil.h, ouvrirModificationProfil).
//   RG-01 : bouton « Profil » du menu ; « Retour » (et la touche Retour d'Android, zone retour de la manette) ramène au menu.
//   RG-02 : en-tête : pseudo, aperçu de la plaque (celui de WFS-481), « Pilote depuis le 12 nov. 2026 » (date d'arrivée).
//   RG-03 : courses, kilomètres (1 décimale), vitesse maximale, frôlements, victoires.
//   RG-04 : records Checkpoint, Course à 5, Poursuite (mêmes textes qu'à l'écran de fin) ; vide : « — ».
//   RG-05 : voiture préférée (nom du garage, sans marque) et ses km.
//   RG-06 : 20 dernières courses dans une liste qu'on fait défiler au doigt (mode, voiture, résultat, date) ; vide : « Aucune
//           course pour l'instant ».
//   RG-07 : « Modifier » ouvre le formulaire de WFS-481 par-dessus ; « Valider » écrit tout de suite, « Annuler » ne change rien.
//   RG-08 : texte de 12 dp au moins ; pseudo de 16 « W » jamais coupé.
// Textes par ui::tr (clés profil.*, français / anglais, WFS-504). Style design-street-racing des Crédits et des Réglages.
//
// Utilisation (menu.cpp) : profil_ecran::dessinerBouton(...) dans drawUi ; dans onTouch, profil_ecran::ouvert() →
// profil_ecran::toucher(...), sinon un appui relâché sur profil_ecran::dansBouton(...) → profil_ecran::ouvrir(). Le panneau
// est dessiné par le système « ProfilEcran » (ordre 468 : par-dessus le menu, le bandeau des paquets, les Réglages et le
// cadeau du jour, sous les Crédits 470 ; jamais ouverts ensemble).
#pragma once
#include <string>
#include "../../core/system.h"   // Marges
#include "../../game/profil.h"

namespace wfs {
class Game;
class Renderer;
namespace ecrans::profil_ecran {

// ---- textes (langue active ; testés au banc « profil » dans les deux langues)
std::string date(long long secondes);           // RG-02 / RG-06 : « 12 nov. 2026 » / « Nov 12, 2026 » (heure locale) ; 0 : « — »
std::string resultat(const CourseJouee& c);     // RG-06 : « 2e en 1:23.4 », « Échappé · 900 pts », « Quittée »…
std::string nomMode(ModeJoue m);                // Checkpoint, Course à 5, Poursuite, Duel
std::string nomVoiture(const std::string& id);  // RG-05 : nom du garage (configgame.json), sans marque ; id inconnu : l'id

// ---- écran (un seul, par-dessus le menu)
void ouvrir();
void fermer();
bool ouvert();
void dessiner(Renderer& r, float W, float H, const Marges& m, double temps);
bool toucher(int id, float x, float y, bool down);   // toujours gardé (vrai) quand l'écran est ouvert

// RG-01 : bouton « PROFIL » du menu (même style et même taille que « Cadeau du jour » et « Crédits »), au-dessus du cadeau
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse);
bool dansBouton(float x, float y);

// ---- banc d'essai (WFS_BANC=profil, WFS_LANGUE=fr|en) : profil neuf, parcours menu → Profil → Modifier → Valider → Retour →
// course → fin → Profil, profil plein (21 courses → 20 lignes), pseudo de 16 « W ». La vraie sauvegarde n'est jamais touchée
// (fichier du banc, effacé à la fin). Rend le nombre d'écarts.
int benchProfil(Game& g, Renderer& r, const std::string& out);
// banc : centre d'une zone (« bouton », « retour », « modifier », « liste »), défilement de la liste
bool centre(const char* nom, float& x, float& y);
float defilement();
float defilementMax();
int lignesHistorique();   // lignes de la liste à la dernière image (20 au plus)
bool pseudoEntier();      // RG-08 : pseudo dessiné en entier à la dernière image

} // namespace ecrans::profil_ecran
} // namespace wfs
