// WaForSpeed — WFS-535 (épic E22) : score d'une course terminée et conversion en étoiles.
// Fonction pure (RC-E22-02, RG-06) : entrée = résultat de la course, sortie = lignes du score {clé de texte, points}, total, étoiles,
// « nouveau record ». Aucun accès au rendu, au réseau ni à la sauvegarde ; le barème est lu dans config().scoreEtoiles
// (configgame.json, section « score_etoiles », RC-E22-01). Le branchement en fin de course est WFS-536, l'écran WFS-537.
// Le score existant de chaque mode (points de distance, score de la Poursuite, records) n'est pas touché (RC-E22-03).
#pragma once
#include <vector>
#include "../core/configgame.h"
#include "profil.h"   // ModeJoue : checkpoint (course solo), course à 5, Poursuite, duel

namespace wfs::score {

// compteur d'un défi : inconnu (le mode n'a pas ce compteur) → le défi vaut 0, sans erreur (RG-04)
enum class Defi { Inconnu = -1, Rate = 0, Reussi = 1 };

struct ResultatCourse {
    ModeJoue mode = ModeJoue::Checkpoint;
    bool terminee = false;        // faux : course quittée → aucun score, 0 ★ (RG-05)
    // RG-01 : place ou issue
    int place = 0;                // course à 5 : 1 à 5 (hors bornes : 0 point de place)
    bool victoire = false;        // duel, Poursuite
    int scorePoursuite = -1;      // Poursuite : score restant à l'arrivée (-1 : inconnu)
    int scorePoursuiteDepart = -1;   // Poursuite : score au départ (poursuite.score_depart) ; -1 : inconnu
    long long pointsDistance = -1;   // course solo : points de distance de la course (score existant) ; -1 : inconnu
    double adversaireParti = -1;  // duel : secondes de course quand l'adversaire est parti (-1 : il n'est pas parti) (RC-E22-04)
    // RG-02 : temps (secondes) ; record ≤ 0 : pas encore de record sur cette carte dans ce mode (1re course)
    double temps = 0, record = 0;
    // RG-03 : chocs de la voiture du joueur (-1 : inconnu → 0 point)
    int chocs = -1;
    // RG-04 : défis
    int depassements = -1;        // course à 5 : adversaires dépassés (-1 : inconnu)
    Defi enTeteDernierTour = Defi::Inconnu;     // course à 5
    int contactsPolice = -1;      // Poursuite (-1 : inconnu)
    Defi jamaisDerriere = Defi::Inconnu;        // duel : jamais derrière après la mi-course
    double ecartArrivee = -1;     // duel : avance à l'arrivée sur l'adversaire (s ; -1 : inconnue)
    Defi checkpointsSansRetard = Defi::Inconnu; // course solo
    Defi nitroJamaisVide = Defi::Inconnu;       // course solo
};

// une ligne du détail du score (écran WFS-537) : clé de texte (traduite par ui::tr dans WFS-537) et points
struct Ligne { const char* cle; int points; };

struct Score {
    std::vector<Ligne> lignes;    // dans l'ordre : place / issue, temps, conduite propre, défis (lignes à 0 point non mises)
    int total = 0;                // somme des lignes
    int etoiles = 0;              // RG-05 : arrondi(total ÷ points_par_etoile), borné [min, max] ; quittée ou duel arrangé : 0
    bool nouveauRecord = false;   // temps meilleur que le record existant
};

Score calculer(const ResultatCourse& r, const ConfigJeu::ScoreEtoiles& bareme);
inline Score calculer(const ResultatCourse& r) { return calculer(r, config().scoreEtoiles); }

} // namespace wfs::score
