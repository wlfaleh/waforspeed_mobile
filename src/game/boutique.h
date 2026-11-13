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
//   - banc d'équilibrage : courses pour la 1re voiture et heures pour toutes les voitures (cibles ≈ 25 courses et ≈ 120 h) ;
//     WFS-536 : avec les étoiles du score (joueurs moyen, bon et débutant simulés), gain_course n'étant plus qu'un secours.
// Prix lus dans config() à chaque appel : un prix changé dans configgame.json compte à la partie suivante, sans recompiler.
// Thread principal (comme la sauvegarde). La sauvegarde n'est pas écrite ici : l'appelant appelle enregistrer().
#pragma once
#include <string>
#include <vector>
#include "personnalisation.h"
#include "profil.h"
#include "sauvegarde.h"
#include "score_course.h"   // WFS-536 : types seulement (le calcul est dans score_course.cpp, lié par le jeu et le banc)

namespace wfs { class Game; class Renderer; }

namespace wfs::boutique {

constexpr long long SOLDE_MAX = 1000000000000LL;   // 10^12 ★ : borne de lecture et de crédit (aucun débordement possible)

// ---- portefeuille (RG-01)
long long solde(const Sauvegarde& s);
// ajoute n ★ (n ≥ 0, borné à SOLDE_MAX) ; raison : texte court pour le journal
void crediter(Sauvegarde& s, long long n, const char* raison);
// retire n ★ si le solde suffit ; faux (rien de changé) sinon ou si n < 0
bool debiter(Sauvegarde& s, long long n, const char* raison);
// fin d'une course terminée (pas quittée) ; rend les étoiles créditées :
//   - WFS-536 (RG-01) : etoilesScore ≥ 0 = étoiles calculées par le score de la course (WFS-535, score_course.h), créditées telles quelles ;
//   - WFS-536 (RG-02) : etoilesScore < 0 = calcul impossible : secours boutique.gain_course ★, noté au journal du jeu.
int crediterCourse(Sauvegarde& s, int etoilesScore);

// WFS-536 : records du mode au départ de la course (lus avant l'arrivée : le mode écrit son nouveau record à l'arrivée)
struct RecordsDepart {
    long long pointsSolo = 0;       // course solo : meilleur score (clé waforspeed-best) ; 0 = pas encore de record
    long long tempsCourse5Ms = 0;   // course à 5 : temps du record (record-course5) ; 0 = pas encore de record
    bool poursuite = false;         // Poursuite : un record existe déjà
};
RecordsDepart recordsDepart(const Sauvegarde& s);
// WFS-536 : entrée du score (WFS-535) d'après la course notée au profil, ce qui est connu du jeu aujourd'hui :
//   - place ou issue, score du mode (points de distance de la course solo, score restant de la Poursuite) ;
//   - temps contre le record (RG-02 de WFS-535), records par mode (par carte : WFS-538) :
//       course à 5 : temps contre record-course5 ; course solo : pas de chrono, le score joue le rôle du temps
//       (temps = record ÷ score, record = 1 : score > record ⇔ « record battu », score ≥ record ÷ 1,05 ⇔ « proche ») ;
//       Poursuite : pas de temps, seulement « 1re course » sans record ; duel : aucun record, aucun point de temps ;
//   - chocs de la voiture du joueur (-1 : inconnus) ; duel : adversaireParti = secondes de course à son départ (-1 : resté) ;
//   - défis : compteurs pas encore donnés par les modes (0 point, RG-04 de WFS-535).
score::ResultatCourse resultatScore(const CourseJouee& c, const RecordsDepart& r, int chocs, double adversaireParti);

// WFS-536 (RG-05) : une course = un seul crédit. depart() ouvre la course ; finir() crédite une seule fois puis la ferme :
// un 2e appel (écran de fin rouvert, retour d'arrière-plan, quitter après l'arrivée) ne crédite plus rien.
class GainCourse {
public:
    void depart() { ouverte_ = true; }
    bool ouverte() const { return ouverte_; }
    // quittée : 0 ★ ; sinon crediterCourse(s, etoilesScore) ; course déjà fermée (ou jamais ouverte) : 0 ★, rien de changé
    int finir(Sauvegarde& s, bool quittee, int etoilesScore);
private:
    bool ouverte_ = false;
};

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
// banc WFS_BANC=equilibrage (sans rendu, < 1 s) : configgame.json lu, lignes au journal et dans <out>/equilibrage.txt ;
// rend le nombre d'écarts aux bornes de RG-11 (0 : équilibré)
int benchEquilibrage(const std::string& out);

// ---- WFS-536 (RG-04) : équilibrage avec le vrai calcul du score (boutique_banc.cpp, lie score_course.cpp)
// Joueurs simulés : place, chocs et temps tirés par un Mulberry32 à graine fixe (même résultat à chaque lancement), modes
// joués à tour de rôle (course solo, course à 5, Poursuite, duel), avec seulement ce que le jeu donne au score (voir
// profil_system.cpp : place ou issue, temps contre le record, chocs, score du mode ; défis pas encore comptés : 0 point).
enum class Joueur { Moyen, Bon, Debutant };
const char* nomJoueur(Joueur j);   // « moyen », « bon », « débutant »
struct SimulationJoueur {
    Joueur joueur = Joueur::Moyen;
    double etoilesMoyennes = 0;      // ★ par course, sur toutes les courses simulées
    int etoilesMin = 0, etoilesMax = 0;
    long long coursesPremiere = 0;   // courses pour acheter la 1re voiture (Equilibrage::prixPremiere)
    long long coursesVoitures = 0; double heuresVoitures = 0;   // toutes les voitures payantes (« tout débloquer », RG-11 de WFS-498)
    long long coursesTout = 0; double heuresTout = 0;           // pour information : voitures + personnalisation
};
SimulationJoueur simulerJoueur(Joueur j, const Equilibrage& e);
std::vector<std::string> bilanJoueur(const SimulationJoueur& sj);
// bornes de RG-04 : moyen : 1re voiture en 22 à 28 courses, tout débloquer en 108 à 132 h ; débutant : 1re voiture en 40 courses au plus ;
// vide si bon, sinon une ligne par écart (le bon joueur est seulement affiché)
std::vector<std::string> verifierJoueur(const SimulationJoueur& sj);

// WFS-536 : banc WFS_BANC=score-etoiles (score_banc.cpp, moteur et banc seulement) : vraie course à 5 terminée puis quittée,
// sauvegarde active dans un fichier du banc ; solde avant / après, aucun 2e crédit ; rend le nombre d'écarts (0 : bon)
int benchScoreEtoiles(Game& g, Renderer& r, const std::string& out);

} // namespace wfs::boutique
