// WaForSpeed — WFS-537 (épic E22) : bloc « Score » de l'écran de fin des 4 modes (course solo, course à 5, Poursuite, duel).
// Règles : regles-de-gestion/WFS-537.md et E22-score.md.
//   - RG-01 : une ligne par élément du score (place ou issue, temps, conduite propre, défis) avec ses points, puis le total ;
//   - RG-02 : « +N ★ » qui compte de 0 à N en moins de 1 s (son court existant à la fin du compte), le nouveau solde,
//     « Nouveau record ! » si le record est battu ;
//   - RG-03 : course quittée : pas de bloc ;
//   - RG-04 : le bloc est une carte à côté du panneau de l'écran de fin : il ne cache jamais « Rejouer » / « Menu »
//     (place et taille des boutons inchangées, 2400 × 1080 et 1280 × 720) ;
//   - RG-05 : textes par ui::tr (assets/textes/fr.txt et en.txt, clés score.* et fin.score.*).
// Données : le score est calculé une seule fois en fin de course par profil_system.cpp (WFS-536), qui publie ici le détail,
// les étoiles vraiment reçues et le nouveau solde du portefeuille (publier). Les écrans de fin des modes posent seulement
// leur panneau (poser, une ligne par mode) ; le système ScoreFin (405, interface) dessine le bloc après eux.
#pragma once
#include <string>
#include <vector>
#include "../core/math.h"
#include "score_course.h"

namespace wfs {
class Game; class Renderer;
namespace score_fin {

struct Etat {
    bool pret = false;                   // une course terminée a son score (vrai jusqu'au départ suivant ou au retour au menu)
    bool detail = false;                 // lignes du score connues (faux : résultat du mode jamais reçu, secours gain_course)
    std::vector<score::Ligne> lignes;    // clés de texte score.* et points
    int total = 0;
    int etoiles = 0;                     // ★ reçues par le portefeuille (sans portefeuille, banc : ★ calculées)
    bool nouveauRecord = false;
    long long solde = -1;                // nouveau solde (−1 : pas de portefeuille, banc d'essai sans sauvegarde)
};

// profil_system.cpp (WFS-536) : fin d'une course terminée. sc : score calculé (nullptr : pas de détail) ; etoiles : ★ reçues
// (< 0 : aucune ligne « +N ★ ») ; solde : nouveau solde (< 0 : sans portefeuille)
void publier(const score::Score* sc, int etoiles, long long solde);
void effacer();          // départ d'une course, course quittée (RG-03), retour au menu
const Etat& etat();
// écran de fin d'un mode : rectangle du panneau dessiné à cette image (pixels de l'interface) ; le bloc se pose à côté
void poser(float x, float y, float w, float h);
// course solo (accroche de course_system.cpp) : vrai si le bloc sera dessiné (le panneau, plus étroit, lui laisse la place)
inline bool affiche() { return etat().pret; }
// ★ pleine à 5 branches (polygones convexes ; la police n'a pas le glyphe), aussi pour le compteur du menu
void dessinerEtoile(Renderer& r, float cx, float cy, float rayon, const vec4& couleur);
// banc : rectangle du bloc à la dernière image où il a été dessiné ; faux s'il ne l'a pas été depuis effacer()
bool rectBloc(float& x, float& y, float& w, float& h);

// banc WFS_BANC=score-fin (score_fin_banc.cpp) : vraies courses des 4 modes en français puis en anglais, sauvegarde active
// (portefeuille), écrans de fin ; menu avec 0, 250 et 12 450 ★ ; parcours accueil → course → fin → « Menu » (compteur à
// jour) ; appui sur le compteur → garage. Captures score-fin-<fr|en>-<mode>-<L>x<H>, score-menu-<fr|en>-<n>-<L>x<H>.
// Rend le nombre d'écarts (0 : bon).
int benchScoreFin(Game& g, Renderer& r, const std::string& out);

} // namespace score_fin
} // namespace wfs
