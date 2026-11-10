// WaForSpeed — écran « Crédits » (WFS-406, épic E5 ; accroche E8 : bouton « Crédits » du menu principal).
// Montre le contenu de licences/CREDITS.txt de l'APK (une seule source, fabriquée par tools/qa/verifier_licences.py --generer) :
// chaque actif sous licence (voitures, deux-roues, personnes, animaux, routes, sons, police, code tiers) avec son auteur, sa
// licence et le lien de la licence, puis les textes MIT et Apache 2.0 exigés par ces licences. Sans aucun nom de marque.
// RG-06 : « Politique de confidentialité : <adresse> » en tête si configgame.json donne confidentialite_url (vide : ligne cachée).
// Style design-street-racing des Réglages et des CGU : fond --bg, panneau --surface à liseré cyan, sur-titre cyan, titre en
// capitales, texte défilant au doigt avec ascenseur, bouton « Retour » orange en biseau qui ramène au menu.
//
// Utilisation (menu.cpp) : credits::dessinerBouton(...) dans drawUi ; dans onTouch, credits::ouvert() → credits::toucher(...),
// sinon un appui relâché sur credits::dansBouton(...) → credits::ouvrir(). Le panneau est dessiné par le système « Credits »
// (ordre 470, par-dessus le menu, le bandeau des paquets et le bouton des Réglages).
#pragma once
#include <string>
#include <vector>
#include "../core/system.h"   // Marges

namespace wfs {
class Game;
class Renderer;
namespace credits {

// texte de CREDITS.txt → paragraphes affichables (fonction pure, testée par tests/test_credits.cpp) :
//   1re ligne non vide → TITRE ; ligne sans retrait finie par « : » → SECTION (sans les « : ») ; ligne en retrait → ENTREE ;
//   autre ligne → TEXTE ; ligne vide → VIDE. Sous une section « Texte de la licence… », les lignes d'un même paragraphe
//   (coupées à 80 colonnes dans le fichier) sont recollées pour suivre la largeur de l'écran.
//   urlConfidentialite non vide → paragraphe « Politique de confidentialité : <adresse> » juste après le titre (RG-06).
struct Paragraphe {
    enum Style { TITRE, SECTION, ENTREE, TEXTE, VIDE } style = TEXTE;
    std::string texte;
};
std::vector<Paragraphe> paragraphes(const std::string& texte, const std::string& urlConfidentialite);

// écran (un seul) : ouvrir lit licences/CREDITS.txt (APK, ou assets/ du banc) et l'adresse de configgame.json
void ouvrir();
void fermer();
bool ouvert();
// panneau plein écran (appelé par le système Credits) ; dp : pixels par dp (cibles de 48 dp)
void dessiner(Renderer& r, float W, float H, const Marges& m);
// toucher quand l'écran est ouvert (glisser dans le texte = défilement, « Retour » = menu) ; toujours gardé (vrai)
bool toucher(int id, float x, float y, bool down);

// bouton « CRÉDITS » du menu (même style que « Réglages », .cam-btn : vitré, biseau, 44 px de haut), en bas à droite de la zone sûre
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse);
bool dansBouton(float x, float y);

// banc d'essai et tests
float defilement();
float defilementMax();
bool centreRetour(float& x, float& y);
bool centreBouton(float& x, float& y);
bool centreTexte(float& x, float& y);
// défile jusqu'à la 1re ligne qui contient `mot` (en haut du cadre) ; faux si absent
bool defilerJusqua(const std::string& mot);

// banc d'essai (WFS_BANC=credits) : menu avec le bouton, écran Crédits début / milieu / deux-roues / native_app_glue / fin,
// retour au menu, garage des voitures renommées ; renvoie le nombre d'erreurs
int benchCredits(Game& g, Renderer& r, const std::string& out);

} // namespace credits
} // namespace wfs
