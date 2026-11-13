// WaForSpeed — portefeuille d'étoiles, voitures à acheter, essai gratuit, équivalents et équilibrage (WFS-498, épic E18, zone E3).
// Données et calculs seulement, sans écran (le garage les affichera : WFS-499, 489, 491, 494).
//
// Règles (regles-de-gestion/WFS-498.md) :
//   - une seule monnaie, les étoiles ★ : solde dans la sauvegarde (clé etoiles), jamais négatif ; un débit refusé ne change rien ;
//   - médailles = packs du market (bronze, argent, or : config().boutique.medaille), pas une monnaie : on donne seulement
//     l'équivalent d'un prix en médailles et en euros ;
//   - voitures : prix dans configgame.json (boutique.voitures.<id>.prix) ; prix 0 = gratuite, possédée dès le départ ;
//     voitures retirées (VOITURES_RETIREES de cars.h) jamais vendues ; achat : débit une seule fois (clé voitures-possedees) ;
//   - essai gratuit : une course solo par voiture bloquée, une seule fois (clé essai-<voiture>=1, écrite au départ) ;
//   - banc d'équilibrage : courses pour la 1re voiture et heures pour toutes les voitures (cibles ≈ 25 courses et ≈ 120 h).
// Prix lus dans config() à chaque appel : un prix changé dans configgame.json compte à la partie suivante, sans recompiler.
// Thread principal (comme la sauvegarde). La sauvegarde n'est pas écrite ici : l'appelant appelle enregistrer().
#pragma once
#include <string>
#include <vector>
#include "personnalisation.h"
#include "sauvegarde.h"

namespace wfs::boutique {

constexpr long long SOLDE_MAX = 1000000000000LL;   // 10^12 ★ : borne de lecture et de crédit (aucun débordement possible)

// ---- portefeuille (RG-01)
long long solde(const Sauvegarde& s);
// ajoute n ★ (n ≥ 0, borné à SOLDE_MAX) ; raison : texte court pour le journal
void crediter(Sauvegarde& s, long long n, const char* raison);
// retire n ★ si le solde suffit ; faux (rien de changé) sinon ou si n < 0
bool debiter(Sauvegarde& s, long long n, const char* raison);
// RG-01 : fin d'une course terminée (pas quittée) : boutique.gain_course ★ ; rend les étoiles créditées
int crediterCourse(Sauvegarde& s);

// ---- équivalents d'un prix (RG-02)
// prix arrondi à la centaine supérieure, puis or, argent, bronze dans cet ordre : 2 200 ★ → « 3 or + 1 argent + 1 bronze » ;
// 0 ★ → « gratuit »
std::string equivalentMedailles(long long prix);
// prix × (euros du pack de bronze / étoiles du pack de bronze), au centime : 2 200 ★ → 2 178 centimes
long long equivalentCentimes(long long prix);
std::string texteEuros(long long centimes);   // 2178 → « 21,78 € »

// ---- voitures (RG-03, RG-04, RG-07)
bool voitureVendue(const std::string& carId);        // connue de configgame (VOITURES_IDS), pas retirée
int prixVoiture(const std::string& carId);           // ★ ; −1 si pas vendue
bool voitureGratuite(const std::string& carId);      // vendue et prix 0
bool possedeVoiture(const Sauvegarde& s, const std::string& carId);   // gratuite ou achetée
enum class AchatVoiture { Achetee, DejaPossedee, SoldeInsuffisant, NonVendue };
const char* texteAchat(AchatVoiture a);
AchatVoiture acheterVoiture(Sauvegarde& s, const std::string& carId);
// RG-07 : achat d'une couleur, finition, skin ou d'un pack de dessins avec le portefeuille (catalogue de WFS-487)
perso::Achat acheterObjet(Sauvegarde& s, const perso::Objet& o, const std::string& carId);

// ---- essai gratuit (RG-08)
bool essaiDisponible(const Sauvegarde& s, const std::string& carId);   // vendue, pas possédée, essai jamais fait
// note l'essai (au départ d'une course solo avec une voiture bloquée) ; faux si pas disponible (rien de noté)
bool marquerEssai(Sauvegarde& s, const std::string& carId);

// ---- sauvegardes d'avant ce ticket (RG-09) : la voiture choisie devient possédée (ses couleurs : perso::migrer, WFS-487)
void migrer(Sauvegarde& s);

// ---- équilibrage (RG-11)
struct Equilibrage {
    std::string premiereVoiture;        // la voiture payante la moins chère
    int prixPremiere = 0;
    long long coursesPremiere = 0;      // courses pour l'acheter (gain par course)
    int nbVoitures = 0;                 // voitures payantes vendues
    long long totalVoitures = 0;        // ★ pour toutes les acheter
    long long coursesVoitures = 0;
    double heuresVoitures = 0;          // × minutes par course
    long long totalPersonnalisation = 0;   // pour information : toutes les peintures, finitions, skins (par voiture) et packs de dessins
    long long coursesTout = 0;
    double heuresTout = 0;
    int gainCourse = 0;
    double minutesParCourse = 0;
};
Equilibrage equilibrage();   // avec config() (configgame.json lu)
std::vector<std::string> bilanEquilibrage(const Equilibrage& e);   // lignes du journal
// bornes de RG-11 : 1re voiture 22 à 28 courses, tout débloquer 108 à 132 h ; vide si bon, sinon une ligne par écart
std::vector<std::string> verifierEquilibrage(const Equilibrage& e);

// ---- journal (bancs)
std::vector<std::string> bilanPortefeuille(const Sauvegarde& s);   // solde, voitures possédées, essais faits

} // namespace wfs::boutique
