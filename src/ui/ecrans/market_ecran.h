// WaForSpeed — écran « Market » (WFS-513, épic E21, zone E8) : le joueur voit son solde d'étoiles, chaque pack (étoiles,
// bonus offert, prix de Google), le « Sans pub », achète et restaure ses achats. Logique et sécurité des achats : WFS-510
// (src/game/market.h : catalogue, livre des achats, boutique) ; Google Play : WFS-511 (market::boutiqueActive()).
//   RG-01 : bouton « Market » au menu principal (et compteur d'étoiles du menu, WFS-537 RG-09) et au garage ; « Retour » (et la
//           touche Retour d'Android : zone retour de la manette) referme l'écran : on retrouve l'écran d'où l'on vient.
//   RG-02 : solde en haut ; une carte par pack : nom, étoiles totales en gros, bonus offert en badge (« +20 % offerts »), base
//           en petit (« 600 ★ + 120 offertes »), prix de Google sur le bouton ; pack débutant en tête et mis en avant tant
//           qu'il est proposé (market::packDebutantPropose) ; « Sans pub » à part, « Acheté » une fois possédé.
//   RG-03 : appui → Boutique::acheter ; bouton désactivé pendant l'attente ; acheté → solde qui monte (1,2 s) et « +720 ★ » ;
//           en attente → « Achat en attente de paiement » ; annulé → rien.
//   RG-04 : « Restaurer mes achats » → Boutique::restaurer → « Achats restaurés » (ou « Aucun achat à restaurer » après 2 s).
//   RG-05 : sans boutique ou sans prix de Google : packs visibles, boutons grisés, « Connexion à Google Play nécessaire ».
//   RG-06 : mentions en petit ; aucun compte à rebours, aucune « offre limitée ».
//   RG-07 : textes par ui::tr (clés market.*, français / anglais, WFS-504) ; noms des packs : market::Pack::nomFr / nomEn.
//   RG-08 : PC et bancs : market::creerBoutique() (WFS_BOUTIQUE=simulee : boutique simulée de WFS-510, prix de secours).
// Le crédit des étoiles reste fait par market::brancher (sauvegarde écrite, puis confirmation à Google) : l'écran ne crédite
// jamais rien lui-même ; il rebranche la boutique avec un rappel « après » pour montrer le résultat.
//
// Utilisation : menu.cpp et garage.cpp (accroches) : dessinerBouton(...) / dessinerBoutonGarage(...) dans drawUi ; dans
// onTouch, ouvert() → toucher(...), sinon un appui sur dansBouton(...) → ouvrir(...). Le panneau est dessiné par le système
// « MarketEcran » (ordre 469 : par-dessus le menu, les Réglages, le Profil et le garage, sous les Crédits 470).
#pragma once
#include <string>
#include "../../core/ecran.h"
#include "../../core/system.h"   // Marges

namespace wfs {
class Game;
class Renderer;
namespace market { class Boutique; }
namespace ecrans::market_ecran {

// ---- écran (un seul, par-dessus le menu ou le garage)
void ouvrir(Ecran origine);
void fermer();
bool ouvert();
void dessiner(Renderer& r, float W, float H, const Marges& m, double temps);
bool toucher(int id, float x, float y, bool down);   // toujours gardé (vrai) quand l'écran est ouvert

// RG-01 : bouton « MARKET » du menu (même style que « Profil », « Cadeau du jour », « Crédits »), au-dessus de « Profil »
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse);
bool dansBouton(float x, float y);
// RG-01 : bouton « MARKET » du garage, sous le solde d'étoiles (en haut à droite) ; op : fondu d'entrée du garage
void dessinerBoutonGarage(Renderer& r, float W, float H, const Marges& m, float op);
bool dansBoutonGarage(float x, float y);

// ---- textes (langue active ; testés au banc « market » dans les deux langues)
std::string nomPack(int i);                    // « Médaille d'or » / « Gold medal »
std::string texteBase(int i);                  // « 600 ★ + 120 offertes » ; vide sans bonus
std::string texteOffert(int i);                // « +20 % offerts » ; vide sans bonus

// ---- boutique utilisée par l'écran : market::boutiqueActive() (Android), sinon market::creerBoutique() (PC, bancs) ;
// nullptr : hors ligne (RG-05)
market::Boutique* boutique();

// ---- banc d'essai (WFS_BANC=market, après le banc logique de WFS-510) : parcours menu → Market → achat → retour, garage →
// Market → retour, compteur d'étoiles, touche Retour, achat en attente, annulé, restauration, hors ligne ; FR puis EN.
// La vraie sauvegarde n'est jamais touchée (fichier du banc, effacé à la fin). Rend le nombre d'écarts.
int benchMarketEcran(Game& g, Renderer& r, const std::string& out);
void horsLigneBanc(bool oui);   // banc : aucune boutique (RG-05)
void nouvelleBoutiqueBanc();    // banc : boutique simulée neuve (chaque langue part d'un téléphone neuf)
// banc : centre d'une zone (« bouton », « garage », « retour », « restaurer », « sans_pub », « pack-0 » … « pack-4 »)
bool centre(const char* nom, float& x, float& y);
const std::string& message();   // message montré à la dernière image (vide : aucun)
long long soldeAffiche();        // solde dessiné à la dernière image (animation de RG-03)

} // namespace ecrans::market_ecran
} // namespace wfs
