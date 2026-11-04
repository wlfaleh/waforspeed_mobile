// Qualité automatique (WFS-15) : résolution dynamique et rétrogradation unique de watchPerf (js/ui/screens.js).
// usage : test_qualite
#include <cmath>
#include <cstdio>
#include "../src/render/qualite_auto.h"
using namespace wfs::qualite;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// `secondes` de course à `fps` images/s ; renvoie les effets cumulés
static int course(Auto& a, double fps, double secondes, bool enCourse = true) {
    int e = 0; for (int i = 0; i < (int)(fps * secondes + 0.5); i++) e |= a.image(1.0 / fps, enCourse); return e;
}

int main() {
    VERIFIE(niveauInitial(false) == HAUTE && niveauInitial(true) == BASSE, "niveau du premier lancement");
    VERIFIE(echelleNiveau(BASSE) == 0.5f && echelleNiveau(MOYENNE) == 0.75f && echelleNiveau(HAUTE) == 1.f, "échelles des niveaux");
    VERIFIE(bloomNiveau(BASSE) == 0.f && bloomNiveau(MOYENNE) > 0 && bloomNiveau(HAUTE) > 0, "bloom coupé en Basse seulement");

    // 25 images/s : −0,1 toutes les 2 s jusqu'à 0,6, puis rétrogradation unique
    Auto a;
    VERIFIE(course(a, 25, 2) & Auto::ECHELLE && std::abs(a.echelle - 0.9f) < 1e-6f, "1re baisse (%.2f)", a.echelle);
    // watchPerf : la rétrogradation est testée juste après la baisse, dans la même mesure que l'arrivée à 0,6 (8 s)
    VERIFIE(course(a, 25, 6) & Auto::RETRO && std::abs(a.echelle - 0.6f) < 1e-6f && a.niveau == MOYENNE && a.fait,
            "à 0,6 sous 30 images/s : rétrogradation (%.2f, niveau %d)", a.echelle, a.niveau);
    course(a, 25, 10);
    VERIFIE(a.niveau == MOYENNE, "une seule rétrogradation (niveau %d)", a.niveau);
    std::printf("25 images/s : échelle 1 → 0,6 par pas de 0,1 toutes les 2 s, Haute → Moyenne à 0,6, une seule fois : OK\n");

    // 35 images/s : l'échelle baisse mais pas de rétrogradation (≥ 30)
    Auto b; course(b, 35, 20);
    VERIFIE(std::abs(b.echelle - 0.6f) < 1e-6f && b.niveau == HAUTE && !b.fait, "35 images/s : pas de rétrogradation");
    // 60 images/s : l'échelle remonte jusqu'à 1
    course(b, 60, 10);
    VERIFIE(std::abs(b.echelle - 1.f) < 1e-6f, "remontée à 1 (%.2f)", b.echelle);
    // 50 images/s : zone morte, rien ne bouge
    const float e0 = b.echelle; VERIFIE(course(b, 50, 10) == Auto::RIEN && b.echelle == e0, "zone morte 40-57");
    std::printf("35 images/s : plancher sans rétrograder ; 60 : remontée à 1 ; 50 : rien ne bouge : OK\n");

    // choix du joueur : jamais de rétrogradation ; hors course : rien n'est mesuré
    Auto c; c.choixJoueur = true; course(c, 20, 30);
    VERIFIE(c.niveau == HAUTE && !c.fait && std::abs(c.echelle - 0.6f) < 1e-6f, "choix du joueur respecté");
    Auto d; VERIFIE(course(d, 10, 10, false) == Auto::RIEN && d.echelle == 1.f, "hors course : aucune mesure");
    // déjà en Basse : pas de niveau inférieur
    Auto e; e.niveau = BASSE; course(e, 20, 30); VERIFIE(e.niveau == BASSE, "pas de niveau sous Basse");
    std::printf("choix du joueur, hors course, niveau Basse : OK\n");

    // WFS-86 : plancher de l'échelle réelle (pas de 0,75 × 0,6 = 45 % en course)
    VERIFIE(echelleRendu(HAUTE, 1) == 1.f && std::abs(echelleRendu(HAUTE, 0.6f) - 0.6f) < 1e-6f, "Haute : 100 %% → 60 %%");
    VERIFIE(echelleRendu(MOYENNE, 1) == 0.75f && echelleRendu(MOYENNE, 0.6f) == PLANCHER && echelleRendu(MOYENNE, 0.7f) == PLANCHER, "Moyenne : jamais sous 60 %%");
    VERIFIE(echelleRendu(BASSE, 1) == 0.5f && echelleRendu(BASSE, 0.6f) == 0.5f, "Basse : 50 %% fixe");
    std::printf("plancher de l'échelle réelle : Haute 60-100 %%, Moyenne 60-75 %%, Basse 50 %% : OK\n");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("qualité : tout est OK\n");
    return 0;
}
