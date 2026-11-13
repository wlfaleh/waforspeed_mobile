// WaForSpeed — cadeau du jour : tirage, garantie, replis, jour local, protection de l'heure, sauvegarde (WFS-500, épic E19, zone E3).
// Logique seulement, sans écran (l'écran et l'animation : WFS-501 ; la voiture prêtée : WFS-502 ; le gain donné pour de vrai : WFS-503).
//
// Règles (regles-de-gestion/WFS-500.md, règles communes RC-E19-01 à 07 dans regles-de-gestion/E19-cadeau-du-jour.md) :
//   - un cadeau gratuit par jour : disponible si le jour local (minuit, heure du téléphone, décalage horaire du moment) de « maintenant »
//     est après celui du dernier cadeau ouvert ; les cadeaux ne s'accumulent pas ; premier lancement : disponible (RG-06) ;
//   - heure reculée (maintenant avant l'heure UTC du dernier cadeau) : aucun cadeau tant qu'elle n'est pas repassée (RG-07) ;
//   - chances dans configgame.json, section « cadeau » (RG-01) ; étoiles : entier uniforme dans la tranche, jamais moins de
//     cadeau.etoiles_min (RG-02) ; catégorie vide → étoiles, tranches renormalisées (RG-04) ; garantie au 30e cadeau sans voiture ni
//     skin (RG-05) ;
//   - ouvrir() est la SEULE action : elle tire, met à jour l'état et écrit la sauvegarde AVANT de rendre le résultat (RG-08). Aucune
//     fonction ne relance, double ou rachète un cadeau (RC-E19-01, gratuit pour toujours).
// Le hasard vient d'une graine (Mulberry32) : même graine, même cadeau ; aucune dépendance au nombre d'images (RG-10).
// Thread principal (comme la sauvegarde).
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "../core/configgame.h"
#include "../core/math.h"
#include "sauvegarde.h"

namespace wfs::cadeau {

// ---- heure (RC-E19-04) : secondes UTC et décalage local du moment (secondes à l'est de Greenwich)
struct Instant {
    long long epoch = 0;
    int decalage = 0;
};
Instant maintenant();                                   // heure du téléphone (std::time et fuseau local)
long long jourLocal(const Instant& t);                  // n° du jour local (jours depuis le 1970-01-01, minuit local)
std::string texteJour(long long jour);                  // 20405 → « 2025-11-14 »
bool lireJour(const std::string& texte, long long& jour);   // « AAAA-MM-JJ » → n° du jour ; faux si mal écrit

// ---- disponibilité (RG-06, RG-07)
enum class Etat { Disponible, DejaOuvert, HeureReculee };
const char* texteEtat(Etat e);
Etat etat(const EtatCadeau& c, const Instant& t);
inline bool disponible(const EtatCadeau& c, const Instant& t) { return etat(c, t) == Etat::Disponible; }

// ---- entrée du tirage (RG-03) : ce que le joueur peut encore gagner ; le garage n'est pas lu par le tirage
struct VoitureJoueur {
    std::string id;                    // voiture possédée par le joueur
    std::vector<std::string> coloris;  // couleurs et finitions pas encore possédées pour elle
    std::vector<std::string> skins;    // skins pas encore possédées pour elle
};
struct Entree {
    std::vector<VoitureJoueur> voitures;
    std::vector<std::string> voituresPretables;   // voitures non possédées, vendues (jamais retirées NC), pas déjà prêtées
};
// entrée faite depuis la sauvegarde (voitures possédées de WFS-498, catalogue de WFS-487)
Entree entreeDepuis(const Sauvegarde& s);

// ---- résultat (RG-08)
enum class Gain { Etoiles, Coloris, Skin, Voiture };
const char* idGain(Gain g);   // « etoiles », « coloris », « skin », « voiture » (clé cadeau-dernier)
struct Resultat {
    Gain gain = Gain::Etoiles;
    int etoiles = 0;          // Etoiles : nombre (≥ cadeau.etoiles_min)
    std::string voiture;      // Coloris, Skin : voiture du joueur ; Voiture : voiture prêtée
    std::string objet;        // Coloris, Skin : identifiant du catalogue (personnalisation.h)
    bool garantie = false;    // donné (ou tenté) par la garantie
    bool repli = false;       // catégorie tirée vide : étoiles à la place
    std::string texte() const;   // « etoiles;23 », « coloris;golf:rose », « skin;golf:rallye », « voiture;m4 »
};
bool lireResultat(const std::string& texte, Resultat& r);   // inverse de texte() ; faux si mal écrit

// ---- tirage pur (testé par tests/test_cadeau.cpp) : sansRare est le compteur de la garantie, mis à jour (RG-05)
Resultat tirer(const ConfigJeu::Cadeau& c, const Entree& e, int& sansRare, Mulberry32& hasard);

// ---- ouvrir (RG-08) : rien (nullopt) si aucun cadeau n'est disponible ; sinon tire, met à jour s.cadeau (heure, décalage, jour,
// garantie, total, dernier), écrit la sauvegarde puis rend le gain ; écriture impossible : rien (nullopt), état remis, le cadeau reste
// disponible. Sauvegarde inactive (tests, bancs) : rien n'est écrit. Le gain n'est PAS donné au joueur ici.
std::optional<Resultat> ouvrir(Sauvegarde& s, const Instant& t, const Entree& e, uint32_t graine);
std::optional<Resultat> ouvrir(Sauvegarde& s);   // jeu : maintenant(), entreeDepuis(s), graine tirée à l'ouverture

// ---- journal (banc WFS_BANC=sauvegarde)
std::vector<std::string> bilan(const Sauvegarde& s, const Instant& t);

} // namespace wfs::cadeau
