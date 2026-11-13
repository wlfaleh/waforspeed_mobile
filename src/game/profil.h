// WaForSpeed — profil du joueur (WFS-480, épic E17, zone E3) : pseudo, plaque, date d'arrivée, statistiques,
// voiture préférée et 20 dernières courses. Données et règles seulement (les écrans sont WFS-481 / 482, la plaque 3D WFS-483).
//
// Les données sont gardées dans le fichier de la sauvegarde (sauvegarde.cpp, format « clé=valeur ») sous ces clés (RG-11) :
//   profil=1                          drapeau : pseudo et plaque validés (RG-06) ; 0 ou absent = écran de création à montrer
//   plaque=AB-123                     plaque du joueur (RG-02)
//   arrivee=1794530000                date d'arrivée (UTC, secondes depuis 1970), écrite une seule fois (RG-05)
//   stats=courses;metres;vmax;frolements;victoires                                  (RG-07)
//   voiture-<id>=metres;courses       km et courses par voiture (voiture préférée, RG-09)
//   historique-01=mode;voiture;date;quittee;victoire;place;temps_ms;score   … jusqu'à historique-20, 01 = la plus récente (RG-10)
//                 WFS-536 (RG-03) : + « ;etoiles » à la fin (★ gagnées) ; ligne d'avant sans ce champ : lue, étoiles inconnues (-1)
// Une ancienne sauvegarde (sans ces clés) se lit sans erreur : profil=0, records gardés (ils restent dans sauvegarde.*, RG-08).
//
// Le pseudo reste la clé « nick » de la sauvegarde (RC-E17-03) ; ce module ne fait que le vérifier (RG-01).
#pragma once
#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace wfs {

namespace profil {

// RG-01, RG-02 : limites
constexpr int PSEUDO_MIN = 3, PSEUDO_MAX = 16;
constexpr int PLAQUE_MIN = 1, PLAQUE_MAX = 8;
constexpr int HISTORIQUE_MAX = 20;   // RG-10

// motif du refus d'un pseudo ou d'une plaque (RG-01) ; Aucun = accepté
enum class Refus { Aucun, TropCourt, TropLong, CaractereInterdit, MotInterdit };
const char* motif(Refus r);   // « trop court », « trop long », « caractère interdit », « mot interdit » ; "" si accepté

// RG-01 : pseudo de 3 à 16 caractères après retrait des espaces au début et à la fin ; lettres A-Z / a-z, chiffres,
// espace (pas deux de suite), « - » et « _ » ; aucun mot interdit. propre (facultatif) : le pseudo sans espaces de bord.
Refus verifierPseudo(const std::string& brut, std::string* propre = nullptr);
// RG-02 : plaque de 1 à 8 caractères A-Z, 0-9 et « - » (minuscules mises en majuscules), pas de tiret au début, à la fin
// ni deux de suite ; aucun mot interdit (RG-04). propre (facultatif) : la plaque en majuscules.
Refus verifierPlaque(const std::string& brut, std::string* propre = nullptr);
// RG-03 : plaque proposée d'après le pseudo (majuscules, lettres et chiffres, 8 premiers) ; vide ou interdite :
// « WFS- » + 3 chiffres tirés par hasard() (fonction de hasard du jeu, Mulberry32 par exemple)
std::string plaqueProposee(const std::string& pseudo, uint32_t (*hasard)());

// RG-04 : texte ramené à sa forme de comparaison : minuscules, sans accents, chiffres « leet » ramenés aux lettres
// (0→o, 1→i, 3→e, 4→a, 5→s, 7→t, @→a, $→s), espaces, tirets et tirets bas retirés
std::string normaliser(const std::string& texte);
// RG-04 : liste des mots interdits (assets/profil/mots_interdits.txt : un mot par ligne, lignes vides et « # » ignorées)
// et liste blanche (lignes « + mot » du même fichier : mots courants qui contiennent un mot interdit). Renvoie le nombre de
// mots interdits lus. Sans liste chargée, aucun mot n'est refusé (le jeu la charge au démarrage : ProfilSystem::init).
int chargerMotsInterdits(const std::string& contenu);
int nombreMotsInterdits();
// vrai si un mot interdit est contenu dans le texte (après normaliser), hors des mots de la liste blanche
bool contientMotInterdit(const std::string& texte);

} // namespace profil

// mode d'une course jouée (mêmes identifiants que la clé « mode » de la sauvegarde, + duel)
enum class ModeJoue { Checkpoint, Course5, Poursuite, Duel };
const char* idModeJoue(ModeJoue m);    // checkpoint, course5, poursuite, duel
const char* nomModeJoue(ModeJoue m);   // Checkpoint, Course à 5, Poursuite, Duel

// une course jouée (RG-07, RG-10)
struct CourseJouee {
    ModeJoue mode = ModeJoue::Checkpoint;
    std::string voiture;          // id de CARS[]
    long long date = 0;           // fin de la course (UTC, secondes)
    long long metres = 0;         // distance parcourue par la voiture du joueur (m)
    int vmax = 0;                 // vitesse maximale du compteur (km/h)
    int frolements = 0;           // frôlements (Course::nearCount, messages « frôlement » du HUD)
    bool quittee = false;         // quittée après le départ (pas de résultat)
    bool victoire = false;        // Course à 5 : 1re place ; Poursuite : échappé ; Duel : gagné (Checkpoint : jamais)
    int place = 0;                // Course à 5 : place (1..5)
    long long tempsMs = 0;        // Course à 5 : temps
    long long score = 0;          // Checkpoint, Poursuite : score
    int etoiles = -1;             // WFS-536 (RG-03) : ★ gagnées (quittée : 0) ; -1 = inconnues (course d'avant WFS-536)
    std::string resultat() const; // « 12 345 pts », « 2e en 1:23,45 », « Échappé · 900 pts », « Gagné », « Quittée »
};

struct Profil {
    bool cree = false;            // RG-06 : profil=1
    std::string plaque;           // RG-02 (vide tant que le profil n'est pas créé)
    long long arrivee = 0;        // RG-05 : date d'arrivée (UTC, secondes) ; 0 = pas encore écrite
    // RG-07 : statistiques
    long long courses = 0, metres = 0, frolements = 0, victoires = 0;
    int vmax = 0;
    struct ParVoiture { long long metres = 0, courses = 0; };
    std::map<std::string, ParVoiture> parVoiture;   // RG-09
    std::vector<CourseJouee> dernieres;              // RG-10 : la plus récente en premier, 20 au plus

    // ---- sauvegarde (appelé par sauvegarde.cpp)
    void ecrire(std::ostream& o) const;                          // clés du profil (profil, plaque, arrivee, stats…)
    bool lireCle(const std::string& cle, const std::string& valeur);   // vrai si la clé est une clé du profil (lue ou ignorée)
    // après la lecture (ou sans fichier) : RG-05 date d'arrivée écrite si elle manque (1er lancement de cette version) ;
    // historique sans trous ; profil=1 sans plaque valide → 0 (l'écran de création revient, rien n'est perdu)
    void apresChargement(long long maintenant);

    // ---- règles
    void noterCourse(const CourseJouee& c);   // RG-07, RG-09, RG-10 : statistiques, km par voiture, 20 dernières
    std::string voiturePreferee() const;      // RG-09 : id de la voiture (plus de km, puis plus de courses) ; « — » sans course
    double km() const { return metres / 1000.0; }   // RG-07 : affiché en km à 1 décimale
    // RG-05 : pseudo proposé à l'écran de création pour un joueur déjà installé (vide si c'est le pseudo par défaut)
    static std::string pseudoPropose(const std::string& nick);
    // RG-06 : crée le profil (pseudo et plaque vérifiés) ; faux (rien ne change) si l'un des deux est refusé
    bool creer(const std::string& pseudo, const std::string& plaque, std::string& nickSortie);
    // texte lisible du profil (journal du banc d'essai, RG des critères d'acceptation)
    std::string texte(const std::string& nick) const;
};

// date UTC lisible « AAAA-MM-JJ hh:mm:ss » (journal, écran Profil)
std::string dateUtc(long long secondes);

} // namespace wfs
