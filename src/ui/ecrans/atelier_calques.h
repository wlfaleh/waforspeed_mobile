// WaForSpeed — WFS-496 (épic E18, zone E8) : logique de l'atelier d'autocollants, sans écran ni Vulkan (testée par
// tests/test_atelier.cpp). L'écran (atelier.h) ne fait que dessiner cet état et lui passer les gestes du joueur.
//
// Ce qui est réutilisé (rien n'est recréé) :
//   - les calques, leur texte, la bibliothèque des 50 motifs et la cuisson : vehicule_autocollants.h (WFS-495) ;
//   - les 28 couleurs : PEINTURES (sauvegarde.h, WFS-487) ; les packs et leurs prix : perso::catalogue() (WFS-487, WFS-498).
//
// Règles (regles-de-gestion/WFS-496.md) :
//   RG-02 : 10 motifs gratuits (pack « essentiels ») : 2 bandes, 3 numéros, 1 flamme, 1 éclair, 1 étoile, 2 lettres ; chaque autre
//           motif appartient à un pack payant (numeros, bandes, flammes, etoiles : BOUTIQUE_DESSINS_IDS) et se montre avec son cadenas.
//           Un motif d'un pack non possédé ne se pose pas (les packs se vendent dans WFS-497).
//   RG-03 : rotation ± 5°, échelle ± 5 % (bornes de WFS-495 : 10 % à 300 %), miroir, couleur (28), supprimer, annuler / refaire
//           (20 pas : 20 états précédents gardés).
//   RG-04 : 20 calques au plus ; monter, descendre, masquer, choisir.
//   RG-05 : « Valider » : texte gardé dans la sauvegarde (clé autocollants-<voiture>) puis cuisson.
//
// Texte gardé (sauvegarde, clé autocollants-<voiture>) : le texte des calques de WFS-495 ; un calque masqué commence par « ! »
// (il reste dans l'atelier, il n'est pas cuit). Seuls des caractères [a-z0-9,;!-] : rien ne casse le fichier clé=valeur.
#pragma once
#include <string>
#include <vector>
#include "../../render/vehicule_autocollants.h"

namespace wfs::ecrans::atelier {

namespace va = vehicule_autocollants;

// ---- motifs et packs (RG-02)
const char* packDuMotif(const std::string& motif);   // « essentiels » (gratuit), « numeros », « bandes », « flammes », « etoiles » ; nullptr si inconnu
bool motifGratuit(const std::string& motif);         // pack « essentiels »
int motifsGratuits();                                // 10
// le joueur peut-il poser ce motif ? (pack gratuit ou possédé dans la sauvegarde active ; sans sauvegarde active : gratuits seulement)
bool motifPermis(const std::string& motif);
constexpr int NB_FAMILLES = 6;                       // Bandes, Numéros, Flammes, Éclairs, Étoiles, Lettres (ordre de va::Famille)
std::vector<const va::Motif*> motifsDe(va::Famille f);   // motifs d'une famille, gratuits d'abord, puis l'ordre de la bibliothèque

// ---- état de l'atelier
struct CalqueAtelier {
    va::Calque c;
    bool masque = false;
    bool operator==(const CalqueAtelier& o) const;
};
constexpr int MAX_CALQUES = va::MAX_CALQUES;   // 20 (RG-04)
constexpr int MAX_HISTORIQUE = 20;             // RG-03 : 20 pas
constexpr int PAS_ROTATION = 5, PAS_ECHELLE = 5;   // RG-03 : boutons fins

class Etat {
public:
    // texte gardé → calques (calques illisibles ignorés, comptés dans *rejetes) ; historique vidé ; aucun calque choisi si vide,
    // sinon le dernier (celui du dessus)
    void charger(const std::string& texte, int* rejetes = nullptr);
    std::string texteGarde() const;          // tous les calques (« ! » devant les masqués)
    std::string texteCuisson() const;        // calques visibles seulement (texte de WFS-495), dans l'ordre
    const std::vector<CalqueAtelier>& calques() const { return calques_; }
    int choisi() const { return choisi_; }   // −1 : aucun
    const CalqueAtelier* calqueChoisi() const { return choisi_ >= 0 && choisi_ < (int)calques_.size() ? &calques_[choisi_] : nullptr; }
    bool modifie() const { return modifie_; }   // changé depuis charger() (RG-05 : confirmation avant de quitter)

    // ---- actions (chacune garde l'état d'avant dans l'historique ; faux si rien n'a changé ou si refusé)
    bool ajouter(const std::string& motif, va::Direction d);   // faux si 20 calques (RG-04) ou motif inconnu ; nouveau calque choisi, au-dessus
    bool supprimer();                        // calque choisi ; le suivant (ou le précédent) devient choisi
    bool monter();                           // calque choisi d'un cran vers le dessus (dessiné plus tard)
    bool descendre();
    bool masquer();                          // bascule masqué / visible
    bool tourner(int degres);                // rotation ramenée dans [−180, 180[
    bool agrandir(int pourcent);             // échelle + pourcent, bornée 10 à 300 %
    bool miroir();
    bool colorer(const std::string& couleur);   // identifiant de PEINTURES ; faux si inconnu
    void choisir(int i);                     // pas d'historique (sélection seulement)
    // geste continu (glisser, deux doigts) : debutGeste garde l'état d'avant une seule fois, les appels suivants ne
    // remplissent pas l'historique ; finGeste ferme le geste (rien gardé si rien n'a changé)
    void debutGeste();
    void placer(int xCm, int yCm);           // position du calque choisi (cm, bornes de WFS-495 : x ± 600, y ± 300)
    void transformer(int rotation, int echelle);   // valeurs absolues (rotation ramenée, échelle bornée)
    void finGeste();
    bool annuler();
    bool refaire();
    int pasAnnuler() const { return (int)avant_.size(); }
    int pasRefaire() const { return (int)apres_.size(); }

private:
    struct Image { std::vector<CalqueAtelier> calques; int choisi; };
    void garder();                           // état actuel → historique (refaire vidé)
    std::vector<CalqueAtelier> calques_;
    int choisi_ = -1;
    std::vector<Image> avant_, apres_;
    bool geste_ = false;
    Image debut_{};                          // état au début du geste en cours
    bool modifie_ = false;
};

// rotation ramenée dans [−180, 180[ (même règle que vehicule_autocollants::lire)
int ramener(int degres);

// ---- sauvegarde (RC-E18-04) : texte gardé pour une voiture (sauvegarde active seulement ; vide sinon)
std::string texteGarde(const std::string& carId);
void garder(const std::string& carId, const std::string& texte);   // écrit la sauvegarde (vide : clé retirée)
// texte à cuire pour la course et le garage : calques visibles, motifs permis seulement (un texte modifié à la main ne donne
// rien qui ne soit possédé) ; vide si aucun
std::string calquesCourse(const std::string& carId);

} // namespace wfs::ecrans::atelier
