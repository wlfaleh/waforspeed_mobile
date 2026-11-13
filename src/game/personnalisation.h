// WaForSpeed — catalogue de personnalisation de l'aspect des voitures (WFS-487, épic E18, zone E3).
// Données seulement, sans écran : couleurs (28), couleur secondaire, finitions (8), skins (4), packs d'autocollants (5),
// leurs prix en étoiles (configgame.json, section « boutique » : prix de base × coefficient de la classe de la voiture),
// ce que le joueur possède (sauvegarde : possede-<voiture>, possede-dessins), l'achat et l'aspect de chaque voiture.
//
// Règles (regles-de-gestion/WFS-487.md) :
//   - une couleur, une finition, une skin ou la couleur secondaire achetée vaut pour UNE voiture ; un pack d'autocollants pour toutes ;
//   - gratuit (8 couleurs, brillant, autocollants « essentiels ») : possédé partout ; couleur d'origine d'une voiture : gratuite pour elle ;
//   - aspectGarage peut montrer un objet non possédé (aperçu) ; aspectCourse ne rend que des objets possédés (le moteur de course
//     n'appelle que lui, RG-07) ;
//   - l'achat débite le solde une seule fois ; solde insuffisant ou objet non vendu : refus, rien de débité.
// Le portefeuille d'étoiles (WFS-498) n'existe pas encore : acheter() reçoit le solde à débiter ; WFS-498 y branche le sien.
// Thread principal (comme la sauvegarde).
#pragma once
#include <string>
#include <vector>
#include "sauvegarde.h"

namespace wfs::perso {

enum class Type { Couleur, Secondaire, Finition, Skin, Dessins };
const char* nomType(Type t);   // « couleur », « couleur secondaire », « finition », « skin », « autocollants »

// un objet du catalogue ; identifiants uniques tous types confondus (clés possede-*)
struct Objet {
    Type type;
    const char* id;
    const char* nom;
    int index;   // place dans sa liste (PEINTURES, BOUTIQUE_*_IDS de configgame.h) : prix lus dans config().boutique
};
constexpr const char* ID_SECONDAIRE = "secondaire";   // droit de poser une couleur secondaire sur une voiture (WFS-488, 489)

const std::vector<Objet>& catalogue();          // 28 couleurs, secondaire, 8 finitions, 4 skins, 5 packs
const Objet* objet(const std::string& id);      // nullptr si inconnu
const Objet* couleur(const std::string& id);    // nullptr si ce n'est pas une couleur

// ---- prix (config().boutique, lu à chaque appel : un prix changé dans configgame.json compte à la partie suivante)
int classeVoiture(const std::string& carId);    // 0 gratuite, 1 à 5 ; voiture inconnue : 0
double coefficient(const std::string& carId);   // coefficient de la classe
bool gratuit(const Objet& o, const std::string& carId);      // offert partout, ou couleur d'origine de cette voiture
bool cadeau(const Objet& o);                                 // seulement cadeau du jour / succès (jamais en vente)
bool disponible(const Objet& o, const std::string& carId);   // l'objet existe pour cette voiture (skins : liste de voitures)
int prix(const Objet& o, const std::string& carId);          // étoiles à payer : 0 si gratuit ; base × coefficient arrondi (dessins : base)

// ---- possession et achat
bool possede(const Sauvegarde& s, const Objet& o, const std::string& carId);   // gratuit, ou acheté (pour cette voiture ; dessins : partout)
enum class Achat { Achete, DejaPossede, SoldeInsuffisant, NonVendu };
const char* texteAchat(Achat a);
// débite solde (une seule fois) et note l'objet possédé ; la sauvegarde n'est pas écrite ici (l'appelant appelle enregistrer())
Achat acheter(Sauvegarde& s, long long& solde, const Objet& o, const std::string& carId);

// ---- aspect d'une voiture (RG-06, RG-07) ; peinture nullptr = couleur d'origine du modèle
struct Aspect {
    const PeintureDef* peinture = nullptr;
    const PeintureDef* peinture2 = nullptr;   // nullptr : même couleur que la principale
    const char* finition = "brillant";
    const char* skin = "";                    // vide : aucune
    std::string creation;                     // habillage de l'atelier (WFS-497) ; vide : aucun
};
Aspect aspectGarage(const Sauvegarde& s, const std::string& carId);   // choix du joueur, possédés ou non (aperçu)
Aspect aspectCourse(const Sauvegarde& s, const std::string& carId);   // seulement des objets possédés, sinon le défaut

// ---- sauvegardes d'avant ce ticket (RG-03) : chaque couleur déjà choisie devient possédée par sa voiture (rien de perdu)
void migrer(Sauvegarde& s);

// PROVISOIRE JUSQU'À WFS-489 (décision du PO, option A, 2026-11-12) : le garage actuel montre les 16 couleurs historiques
// sans cadenas ; « Choisir » donne la couleur choisie à cette voiture seulement (journal), pour que la course garde la couleur
// que le joueur voit. WFS-489 retire cet appel quand il pose le cadenas ; les couleurs déjà données restent au joueur.
// Vrai si la couleur vient d'être donnée.
bool donGarageProvisoire(Sauvegarde& s, const std::string& carId, const std::string& couleurId);

// ---- journal (banc WFS_BANC=sauvegarde) et contrôles
std::vector<std::string> bilanCatalogue();                 // une ligne par groupe : nombre et prix
std::vector<std::string> bilanPossession(const Sauvegarde& s);
std::vector<std::string> verifierCatalogue();              // problèmes du catalogue lu (voiture inconnue d'une skin…) ; vide si bon

} // namespace wfs::perso
