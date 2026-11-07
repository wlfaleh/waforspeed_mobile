// Tenue sur téléphone (WFS-123, RG-09) : niveau de chauffe, rétrogradation unique par course, retour à la qualité
// choisie après 60 s sans chauffe, échelle dynamique bloquée à chauffe modérée, plafond 30 / 60 images/s.
// usage : test_chauffe
#include <cmath>
#include <cstdio>
#include "../src/platform/chauffe.h"
#include "../src/render/qualite_auto.h"
using namespace wfs;
using namespace wfs::chauffe;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// `secondes` de jeu par pas de 1/120 s ; renvoie les effets cumulés
static int jouer(Regulateur& g, double secondes, int niveau, bool enCourse, bool horsCourse, double& raceTime, int& q) {
    int e = 0;
    for (int i = 0; i < (int)(secondes * 120 + 0.5); i++) {
        if (enCourse) raceTime += 1.0 / 120;
        e |= g.pas(1.0 / 120, niveau, enCourse, horsCourse, raceTime, q);
    }
    return e;
}

static int g_statut = 0;
static int statutTest() { return g_statut; }

int main() {
    // RG-01 : statut d'Android → niveau 0 / 1 / 2 ; sans source : niveau 0
    VERIFIE(niveauDepuisStatut(-1) == 0 && niveauDepuisStatut(0) == 0, "aucune chauffe");
    VERIFIE(niveauDepuisStatut(1) == 1 && niveauDepuisStatut(2) == 1, "légère / modérée → 1");
    for (int s = 3; s <= 6; s++) VERIFIE(niveauDepuisStatut(s) == 2, "statut %d → 2", s);
    VERIFIE(!sourceActive() && chauffeNiveau() == 0 && statut() == -1, "sans source : niveau 0");
    definirSource(statutTest); g_statut = 3;
    VERIFIE(sourceActive() && chauffeNiveau() == 2, "source : statut 3 → niveau 2");
    definirSource(nullptr);
    std::printf("niveaux de chauffe (statuts Android 0 à 6) : OK\n");

    // RG-02 : chauffe sévère en course → une seule rétrogradation par course, même qualité choisie (Haute → Moyenne)
    Regulateur g; int q = qualite::HAUTE; double rt = 0;
    VERIFIE(jouer(g, 5, 0, true, false, rt, q) == Regulateur::RIEN && q == qualite::HAUTE, "pas de chauffe : rien");
    VERIFIE(jouer(g, 0.01, 2, true, false, rt, q) == Regulateur::RETRO && q == qualite::MOYENNE, "sévère : Haute → Moyenne (%d)", q);
    VERIFIE(jouer(g, 30, 2, true, false, rt, q) == Regulateur::RIEN && q == qualite::MOYENNE, "une seule rétrogradation par course (%d)", q);
    // décompte (raceTime non encore lancé) : pas de rétrogradation
    VERIFIE(jouer(g, 3, 2, false, false, rt, q) == Regulateur::RIEN, "hors course : pas de rétrogradation");
    // nouvelle course (raceTime repart de 0), toujours sévère : Moyenne → Basse ; puis plus rien (Basse)
    rt = 0;
    VERIFIE(jouer(g, 1, 2, true, false, rt, q) == Regulateur::RETRO && q == qualite::BASSE, "course suivante : Moyenne → Basse (%d)", q);
    rt = 0;
    VERIFIE(jouer(g, 5, 2, true, false, rt, q) == Regulateur::RIEN && q == qualite::BASSE, "Basse : rien de plus");
    // retour au menu, chauffe retombée : 60 s au niveau 0 avant de rétablir la qualité choisie (Haute)
    VERIFIE(jouer(g, 59, 0, false, true, rt, q) == Regulateur::RIEN && q == qualite::BASSE, "59 s : pas encore");
    VERIFIE(jouer(g, 1.5, 0, false, true, rt, q) == Regulateur::RETABLI && q == qualite::HAUTE, "60 s : Haute rétablie (%d)", q);
    VERIFIE(g.choisi == -1, "plus rien à rétablir");
    std::printf("chauffe sévère : une rétrogradation par course, qualité choisie rétablie au menu après 60 s : OK\n");

    // 60 s sans chauffe passées en course : rétablie seulement au menu ; une chauffe entre-temps remet le compte à zéro
    Regulateur h; int q2 = qualite::HAUTE; double rt2 = 0;
    jouer(h, 1, 2, true, false, rt2, q2);
    VERIFIE(q2 == qualite::MOYENNE, "rétrogradée");
    VERIFIE(jouer(h, 70, 0, true, false, rt2, q2) == Regulateur::RIEN && q2 == qualite::MOYENNE, "en course : pas de retour");
    VERIFIE(jouer(h, 0.5, 0, false, true, rt2, q2) == Regulateur::RETABLI && q2 == qualite::HAUTE, "au menu : retour immédiat");
    jouer(h, 0.5, 2, true, false, rt2 = 0, q2);
    VERIFIE(q2 == qualite::MOYENNE, "rétrogradée de nouveau");
    jouer(h, 40, 0, false, true, rt2, q2); jouer(h, 1, 1, false, true, rt2, q2);
    VERIFIE(jouer(h, 40, 0, false, true, rt2, q2) == Regulateur::RIEN && q2 == qualite::MOYENNE, "chauffe 1 entre-temps : compte remis à zéro");
    VERIFIE(jouer(h, 21, 0, false, true, rt2, q2) == Regulateur::RETABLI && q2 == qualite::HAUTE, "60 s d'affilée : rétablie");
    // le joueur change la qualité lui-même pendant la chauffe : son choix est gardé (rien à rétablir)
    jouer(h, 0.5, 2, true, false, rt2 = 0, q2);
    VERIFIE(q2 == qualite::MOYENNE, "rétrogradée (3)");
    q2 = qualite::BASSE;
    VERIFIE(jouer(h, 70, 0, false, true, rt2, q2) == Regulateur::RIEN && q2 == qualite::BASSE && h.choisi == -1, "choix du joueur gardé");
    std::printf("retour au menu seulement, compte remis à zéro par la chauffe, choix du joueur gardé : OK\n");

    // RG-03 : chauffe modérée → la résolution dynamique ne remonte plus
    // WFS-222 : 35 images/s (pas de 0,1 ; sous 30 images/s le pas est de 0,2, test_qualite)
    qualite::Auto a; a.choixJoueur = true;
    for (int i = 0; i < 4 * 35 + 2; i++) a.imageChauffe(1.0 / 35, true, 0);   // + 2 : arrondi de la somme des 1/35
    const float bas = a.echelle;
    VERIFIE(std::fabs(bas - 0.8f) < 1e-6f, "35 images/s pendant 4 s : échelle 0,8 (%.2f)", bas);
    int ef = 0;
    for (int i = 0; i < 10 * 60; i++) ef |= a.imageChauffe(1.0 / 60, true, 1);
    VERIFIE(a.echelle == bas && !(ef & qualite::Auto::ECHELLE), "chauffe 1 : échelle bloquée (%.2f)", a.echelle);
    for (int i = 0; i < 3 * 35; i++) a.imageChauffe(1.0 / 35, true, 1);   // une mesure de 2 s (reste de la mesure précédente)
    VERIFIE(std::fabs(a.echelle - 0.7f) < 1e-6f, "chauffe 1 : l'échelle peut encore baisser (%.2f)", a.echelle);
    for (int i = 0; i < 6 * 60; i++) a.imageChauffe(1.0 / 60, true, 0);
    VERIFIE(a.echelle > 0.7f + 1e-6f, "sans chauffe : l'échelle remonte (%.2f)", a.echelle);
    std::printf("chauffe modérée : échelle dynamique bloquée (baisse toujours possible) : OK\n");

    // RG-06 : 30 images/s hors course, 60 en course
    VERIFIE(plafondIps(Ecran::Course) == 60, "course : 60");
    for (Ecran e : {Ecran::Chargement, Ecran::Menu, Ecran::Garage, Ecran::Terrain, Ecran::Pause, Ecran::Fin, Ecran::Salon})
        VERIFIE(plafondIps(e) == 30, "%s : 30", nomEcran(e));
    std::printf("plafond : 30 images/s aux menus, 60 en course : OK\n");

    if (echecs) { std::printf("test_chauffe : %d échec(s)\n", echecs); return 1; }
    std::printf("test_chauffe : tout est conforme\n");
    return 0;
}
