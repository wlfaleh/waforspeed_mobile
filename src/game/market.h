// WaForSpeed — market : catalogue des packs, livre des achats, « sans pub », ×2 de fin de course, boutique simulée (WFS-510, épic E21, zone E3).
// Logique seulement, sans écran ni Google (Google Play Billing : WFS-511 ; vidéo : WFS-512 ; écrans : WFS-513, 514).
//
// Règles (regles-de-gestion/WFS-510.md, E21-market.md) :
//   - une seule monnaie, les étoiles : l'argent achète des packs d'étoiles (et le « sans pub »), jamais un objet du jeu (RC-E21-01, 02) ;
//   - catalogue dans configgame.json, section market (RG-01) : total crédité = base + arrondi(base × offert / 100) ;
//   - livre des achats (RG-03, RC-E21-06) : un jeton déjà crédité ne crédite plus jamais ; crédit + jeton dans la MÊME écriture de la
//     sauvegarde, faite (et attendue) avant que l'appelant consomme ou confirme l'achat auprès de Google ;
//   - seul l'état « acheté » crédite ; « en attente » : rien, message ; « annulé » / erreur : rien, sans message (RG-04) ;
//   - ×2 : 5 par jour (date locale ; l'heure qui recule ne remet pas le compteur à zéro), une seule fois par course (RG-06) ;
//   - aucun tirage au hasard, aucun minuteur, aucune énergie (RC-E21-03).
// Thread principal (comme la sauvegarde). Valeurs lues dans config() à chaque appel : un pack changé dans configgame.json compte sans recompiler.
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "sauvegarde.h"

namespace wfs::market {

// RG-02 : le pack débutant donne un bon de skin ; il n'est proposé qu'une fois des skins en vente au garage.
// Passe à true avec WFS-494 (garage des skins, épic E18) : c'est la seule ligne à changer.
constexpr bool SKINS_EN_VENTE = false;

// ---- catalogue (RG-01)
struct Pack {
    std::string id;            // identifiant du produit de la Play Console (MARKET_PRODUITS_IDS)
    const char* nomFr;
    const char* nomEn;
    int etoilesBase = 0;
    int offert = 0;            // % offert (packs d'étoiles)
    long long total = 0;       // étoiles créditées et affichées
    int bonsSkin = 0;          // pack débutant
    bool consommable = false;  // packs d'étoiles : rachetables ; pack débutant, sans pub : une fois
    double prixSecours = 0;    // € : PC et bancs seulement (sur Android, prix de Google : Boutique::prix)
};
int indexProduit(const std::string& id);   // place dans MARKET_PRODUITS_IDS ; −1 si inconnu
Pack pack(int i);                          // i dans [0, MARKET_PRODUITS)
std::vector<Pack> catalogue();             // les 6 produits, dans l'ordre de RG-01
long long totalEtoiles(int base, int offert);   // base + arrondi(base × offert / 100)
std::string texteEurosSecours(double euros);    // 2.49 → « 2,49 € »

// ---- pack débutant (RG-02) : à partir de la fin de la course config().market.debutantApres, jamais après l'achat
bool packDebutantPropose(const Sauvegarde& s, bool skinsEnVente = SKINS_EN_VENTE);

// ---- livre des achats (RG-03, RG-04)
enum class Credit { Credite, DejaCredite, Inconnu };
const char* texte(Credit c);   // « crédité », « déjà crédité », « inconnu »
// crédite le produit si le jeton n'est pas dans le livre, puis note le jeton ; sauvegarde active : écrite et attendue ici (même écriture).
// Inconnu : produit inconnu ou jeton mal formé (rien de changé). Pack débutant ou « sans pub » déjà possédés : jeton noté, rien de plus.
Credit crediterAchat(Sauvegarde& s, const std::string& produit, const std::string& jeton);
bool dansLivre(const Sauvegarde& s, const std::string& jeton);

enum class Etat { Achete, EnAttente, Annule, Erreur };
const char* texte(Etat e);
struct Traitement {
    Credit credit = Credit::Inconnu;   // seulement pour Achete
    bool confirmer = false;            // l'appelant consomme (packs d'étoiles) ou confirme (pack débutant, sans pub) l'achat auprès de Google
    std::string message;               // à montrer au joueur ; vide : rien (annulé, erreur : pas de message alarmant)
};
Traitement traiterAchat(Sauvegarde& s, const std::string& produit, const std::string& jeton, Etat e);

// ---- sans pub (RG-05)
bool sansPub(const Sauvegarde& s);

// ---- ×2 de fin de course (RG-06) ; jour = AAAAMMJJ (jourLocal) ; course = n° de la course terminée (profil.courses, ≥ 1)
int jourLocal(long long secondesUtc);   // date locale du téléphone
int aujourdhui();
int x2Restants(const Sauvegarde& s, int jour);   // ×2 encore possibles ce jour-là
bool bonusX2Disponible(const Sauvegarde& s, long long course, int jour);
// crédite une 2e fois le gain de la course si disponible ; rend les étoiles créditées (0 : refusé, rien de changé).
// L'appelant enregistre la sauvegarde (comme boutique::crediter).
int appliquerX2(Sauvegarde& s, int gainCourse, long long course, int jour);
int x2SansPub(Sauvegarde& s, int gainCourse, long long course, int jour);        // fin de course avec « sans pub » : ×2 direct
int recompenseVideo(Sauvegarde& s, int gainCourse, long long course, int jour);  // vidéo vue en entier (WFS-512)

// ---- boutique (RG-07) : Google Play (WFS-511) ou simulée
class Boutique {
public:
    using Rappel = std::function<void(const std::string& produit, const std::string& jeton, Etat etat)>;
    virtual ~Boutique() = default;
    virtual const char* nom() const = 0;
    // liste des produits et de leurs prix ; les achats payés mais pas encore confirmés reviennent au rappel (rattrapage au lancement)
    virtual void chargerProduits() = 0;
    virtual std::string prix(const std::string& id) const = 0;   // texte du prix (Google) ; vide si inconnu
    virtual void acheter(const std::string& id) = 0;             // réponse par le rappel
    virtual void restaurer() = 0;   // achats possédés (pack débutant, sans pub) et achats pas encore confirmés, au rappel
    virtual void confirmer(const std::string& id, const std::string& jeton) = 0;   // consommer ou confirmer (acknowledge)
    void surAchat(Rappel r) { rappel_ = std::move(r); }
protected:
    void signaler(const std::string& id, const std::string& jeton, Etat e) { if (rappel_) rappel_(id, jeton, e); }
private:
    Rappel rappel_;
};

// relie une boutique à la sauvegarde : chaque achat passe par traiterAchat, puis confirmer si demandé ; apres (facultatif) : écran
void brancher(Boutique& b, Sauvegarde& s, std::function<void(const std::string& produit, const Traitement&)> apres = {});

// PC, bancs (WFS_BOUTIQUE=simulee) : répond sur commande, sans réseau ni argent
class BoutiqueSimulee : public Boutique {
public:
    enum class Reponse { Achete, EnAttente, Annule, Interrompu };   // Interrompu : crédit écrit, application coupée avant la confirmation
    const char* nom() const override { return "simulée"; }
    void chargerProduits() override;
    std::string prix(const std::string& id) const override;
    void acheter(const std::string& id) override;
    void restaurer() override;
    void confirmer(const std::string& id, const std::string& jeton) override;
    void repondre(Reponse r) { reponse_ = r; }   // réponse des achats suivants (Achete par défaut)
    void doubler(bool d) { doubler_ = d; }       // l'achat suivant renvoie le même jeton deux fois
    int nonConfirmes() const { return (int)enCours_.size(); }
    int confirmes() const { return confirmes_; }
private:
    Reponse reponse_ = Reponse::Achete;
    bool doubler_ = false, coupure_ = false;
    int numero_ = 0, confirmes_ = 0;
    std::vector<std::pair<std::string, std::string>> enCours_;   // achats payés, pas encore confirmés (produit, jeton)
    std::map<std::string, std::string> possedes_;                // non consommables confirmés : produit → jeton
};

// WFS_BOUTIQUE=simulee : boutique simulée ; sinon nullptr (Android : WFS-511)
std::unique_ptr<Boutique> creerBoutique();

// ---- banc WFS_BANC=market (RG-09, sans rendu, < 1 s) : configgame.json lu ; lignes au journal et dans <out>/market.txt ;
// rend le nombre de résultats faux (0 : tout est juste)
int benchMarket(const std::string& out);

} // namespace wfs::market
