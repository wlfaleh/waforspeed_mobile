// WaForSpeed — tenue sur téléphone (WFS-123, épic E7) : chauffe, batterie et 60 images/s stables.
//   - RG-01 : état thermique d'Android (AThermal, API 30+, chargé par dlsym dans chauffe_android.cpp) ramené à un
//     niveau 0 (aucune), 1 (légère / modérée) ou 2 (sévère ou plus), lu par la boucle de jeu (chauffeNiveau) ;
//   - RG-02 : chauffe sévère en course → un niveau de qualité en moins, une fois par course, même si le joueur a choisi
//     sa qualité ; bandeau « Graphismes rapides » du jeu web ; qualité choisie rétablie au menu après 60 s au niveau 0,
//     jamais enregistrée (Regulateur ci-dessous, appliqué par chauffe_systeme.cpp) ;
//   - RG-03 : chauffe modérée → la résolution dynamique ne remonte plus (qualite::Auto::imageChauffe) ;
//   - RG-06 : 30 images/s hors course, 60 en course (plafondIps, attente de la boucle android_main).
// Le jeu web n'a pas d'état thermique (le navigateur ne l'expose pas) : seule la rétrogradation et le bandeau de
// watchPerf / setQuality (js/ui/screens.js) sont repris, par qualite::Auto et qualite_systeme.cpp.
// Calcul seul ici (aucun appel Vulkan ni Android) : testé par tests/test_chauffe.cpp.
#pragma once
#include <string>
#include "../core/ecran.h"

namespace wfs {
class Game; class Renderer;

namespace chauffe {

// statut thermique brut (valeurs d'ATHERMAL_STATUS_* : 0 aucun, 1 léger, 2 modéré, 3 sévère, 4 critique, 5 urgence,
// 6 arrêt ; −1 inconnu) → niveau du jeu (RG-01)
inline int niveauDepuisStatut(int statut) { return statut >= 3 ? 2 : statut >= 1 ? 1 : 0; }

// source du statut, installée par la plate-forme : Android (capteur AThermal) ou banc PC (WFS_CHAUFFE, desktop_main.cpp).
// Sans source (iOS, banc sans WFS_CHAUFFE) : niveau 0 et module de chauffe inactif (captures des autres scènes inchangées).
using Source = int (*)();
void definirSource(Source s);
bool sourceActive();
int statut();   // statut brut lu à la source (−1 sans source)

// temps de course cumulé depuis le lancement (s), avancé par le module de chauffe : WFS_CHAUFFE_APRES du banc (RG-08)
double tempsCourse();
void avancerTempsCourse(double dt);

// RG-06 : plafond d'images/s selon l'écran (Course = décompte, course et écran de fin ; le reste : menus, garage, terrain,
// pause, salon, chargement) ; plafondCourant() est lu par la boucle android_main après chaque image
inline int plafondIps(Ecran e) { return e == Ecran::Course ? 60 : 30; }
int plafondCourant();
void definirPlafond(int ips);

// RG-02 : rétrogradation de chauffe (une par course) et retour à la qualité choisie
struct Regulateur {
    int choisi = -1;            // qualité à rétablir (−1 : aucune rétrogradation de chauffe en cours)
    int applique = -1;          // qualité posée par la chauffe (le joueur qui la change dans les réglages reprend la main)
    bool faitCourse = false;    // rétrogradation déjà faite dans cette course
    double froid = 0;           // temps passé au niveau 0 (s)
    double dernierTemps = -1;   // raceTime vu au dernier pas (il redescend à chaque nouvelle course)
    enum Effet { RIEN = 0, RETRO = 1, RETABLI = 2 };
    // un pas de `dt` s : niveau de chauffe, en course (décompte fini), hors course (menus : RG-02 « au menu »), raceTime de
    // la course active (−1 sans course) ; `qualite` (reglages().qualite) est modifiée ; renvoie l'effet
    int pas(double dt, int niveau, bool enCourse, bool horsCourse, double raceTime, int& qualite);
};
constexpr double FROID_RETABLIR = 60;   // RG-02 : 60 s au niveau 0 avant de rétablir la qualité choisie

const char* nomQualite(int q);   // « Basse », « Moyenne », « Haute » (journal)

// banc d'essai (WFS_BANC=chauffe, RG-10) : parcours complet accueil → course → fin → accueil avec chauffe simulée,
// captures chauffe-*.png ; `source` = simulation du banc. Rend le nombre d'écarts (0 = conforme).
int benchChauffe(Game& g, Renderer& r, const std::string& out, Source source);

} // namespace chauffe

// RG-01 : niveau de chauffe 0 / 1 / 2, lu par la boucle de jeu (jamais par le thread du capteur)
int chauffeNiveau();
// RG-02 : bandeau « Graphismes rapides — pour plus de fluidité » du jeu web (défini dans src/render/qualite_systeme.cpp)
void bandeauGraphismesRapides();

} // namespace wfs
