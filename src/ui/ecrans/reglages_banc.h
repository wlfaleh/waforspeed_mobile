// WaForSpeed — WFS-420 : banc de performance automatique sur le téléphone, côté écrans (zone src/ui/ecrans/reglages*).
// Lancé depuis le menu caché de WFS-419 (bouton « Banc auto ») : écran du protocole (RG-05), puis le banc roule seul (RG-01,
// RG-02 : Ville de nuit puis de jour, pilote automatique, un interrupteur à la fois), puis tableau des résultats et fichier
// diagnostic-AAAA-MM-JJ-hhmm.txt avec « Partager » (RG-04). Calculs et texte du fichier : src/render/banc_telephone.h.
// Le protocole et les résultats sont dessinés par-dessus le panneau des Réglages (reglages.cpp les appelle) ; le bandeau du
// banc en course et son bouton « Arrêter » sont dessinés par le système du banc (reglages_banc.cpp).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans::banc_telephone {

void ouvrirProtocole();          // menu caché → écran du protocole
bool protocoleOuvert();
bool resultatsOuverts();
bool enCours();                  // chargement, chauffe ou mesure
void commencer();                // « Commencer » du protocole
// dessin par-dessus le panneau des Réglages (W × H pixels de l'interface, k = pixels par dp)
void dessinerProtocole(Renderer& r, float W, float H, float k);
void dessinerResultats(Renderer& r, float W, float H, float k);
// touchers (appui) quand l'écran est ouvert : toujours gardés
bool toucherProtocole(float x, float y);
bool toucherResultats(float x, float y);
// dossier du fichier de résultats (banc PC) ; vide : qualite::dossierMesure (Android : externalDataPath)
inline std::string dossierBanc;

// banc PC (WFS_BANC=banc-telephone) : menu caché, protocole, banc court (WFS_BANC_TEL_DUREES, défaut 0,6 s + 1,2 s par essai),
// nuit puis jour, résultats ; captures banc-tel-*.png ; rend le nombre d'écarts
int benchBancTelephone(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans::banc_telephone
} // namespace wfs
