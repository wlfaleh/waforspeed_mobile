// WaForSpeed — écran du cadeau du jour (WFS-501, épic E19, zone E8 ; accroche E8 : bouton « Cadeau du jour » du menu principal).
// Montre le cadeau tiré par la logique de WFS-500 (src/game/cadeau.h) : bouton du menu avec pastille quand un cadeau attend,
// animation d'ouverture d'une boîte cadeau dessinée en 2D (néons de design-street-racing, ≈ 1,5 s : elle tremble, s'ouvre,
// lumière, puis le gain), gain selon son type, tableau « Chances » lu dans configgame.json, compte à rebours jusqu'au prochain.
//
// Règles (regles-de-gestion/WFS-501.md, règles communes RC-E19-01 à 07 de regles-de-gestion/E19-cadeau-du-jour.md) :
//   - le tirage est fait AVANT l'animation (cadeau::ouvrir de WFS-500 : sauvegarde écrite d'abord) : quitter pendant l'animation
//     ne perd ni ne double rien ; au retour, le dernier gain (cadeau-dernier) est réaffiché ;
//   - un appui pendant l'animation la passe, sans redonner de cadeau ; aucune 2e ouverture le même jour depuis l'écran ;
//   - AUCUNE offre payante (RC-E19-01) : ni « rouvrir », ni « doubler », ni pub, ni prix ;
//   - textes par ui::tr (français / anglais, WFS-504) ; noms des voitures : noms affichés de configgame.json (jamais une marque).
//   - le gain n'est pas donné ici (WFS-503) ; « Activer » de la voiture prêtée commence le prêt et ferme l'écran (WFS-502).
//
// Utilisation (menu.cpp) : cadeau_jour::dessinerBouton(...) dans drawUi ; dans onTouch, cadeau_jour::ouvert() →
// cadeau_jour::toucher(...), sinon un appui relâché sur cadeau_jour::dansBouton(...) → cadeau_jour::ouvrir(). Le panneau est
// dessiné par le système « CadeauJour » (ordre 466 : par-dessus le menu, le bandeau des paquets et les Réglages).
#pragma once
#include <string>
#include <vector>
#include "../../core/system.h"   // Marges
#include "../../game/cadeau.h"

namespace wfs {
class Game;
class Renderer;
namespace ecrans::cadeau_jour {

// ---- calculs purs (testés par tests/test_cadeau_ecran.cpp)
// secondes jusqu'au prochain minuit local (RC-E19-04) ; heure reculée (WFS-500 RG-07) ou cadeau disponible : -1
long long secondesAvantProchain(const EtatCadeau& c, const cadeau::Instant& t);
// « hh:mm » (minutes arrondies au-dessus : jamais « 00:00 » avant minuit) ; 24 h au plus
std::string texteHeures(long long secondes);
// tableau « Chances » (RG-04) : 7 lignes lues dans configgame.json (4 tranches d'étoiles, coloris, skin, voiture prêtée)
struct LigneChance { std::string cle, v1, v2; double chance; };   // texte = ui::tr(cle, v1, v2)
std::vector<LigneChance> lignesChances(const ConfigJeu::Cadeau& c);
// cadeaux restants avant la garantie (RG-04) : au moins 1
int restantsGarantie(const ConfigJeu::Cadeau& c, const EtatCadeau& e);

// ---- écran (un seul, par-dessus le menu)
void ouvrir();     // cadeau disponible : tirage (WFS-500) puis animation ; sinon : écran d'attente (dernier gain, compte à rebours)
void fermer();
bool ouvert();
void dessiner(Renderer& r, float W, float H, const Marges& m, double temps);
bool toucher(int id, float x, float y, bool down);   // toujours gardé (vrai) quand l'écran est ouvert

// bouton « CADEAU DU JOUR » du menu (même style et même taille que « Crédits » et « Réglages »), au-dessus de « Crédits », en bas à
// droite de la zone sûre ; pastille orange quand un cadeau attend, sinon « Prochain cadeau dans hh:mm » à sa gauche
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse);
bool dansBouton(float x, float y);

// ---- banc d'essai (WFS_BANC=cadeau, RG-07) : WFS_CADEAU=etoiles|coloris|skin|voiture|chances|attente ne garde que ces captures ;
// la vraie sauvegarde n'est jamais touchée (sauvegarde inactive du banc). Rend le nombre d'erreurs du parcours RG-08.
int benchCadeau(Game& g, Renderer& r, const std::string& out);
// WFS-502 (RG-09, RG-10) : banc de la voiture prêtée (pret_bandeau_banc.cpp), lancé par benchCadeau : WFS_CADEAU=pret-attente|pret-actif|
// pret-fin ne garde que ces captures (sans WFS_CADEAU : toutes, après celles de WFS-501). Rend le nombre d'erreurs du parcours RG-10.
int benchPret(Game& g, Renderer& r, const std::string& out, const std::string& filtre);

} // namespace ecrans::cadeau_jour
} // namespace wfs
