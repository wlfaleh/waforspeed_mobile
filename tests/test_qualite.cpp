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
    VERIFIE(echelleNiveau(BASSE) == 0.75f && echelleNiveau(MOYENNE) == 1.f && echelleNiveau(HAUTE) == 1.f, "échelles des niveaux (WFS-104)");
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

    // WFS-104 (RG-02) : netteté du garage en course : Haute toujours 100 %, Moyenne jamais sous 90 %, Basse 50 % fixe
    bool hauteFixe = true, moyenneOk = true;
    for (float d = 0.6f; d <= 1.0001f; d += 0.1f) {
        hauteFixe = hauteFixe && echelleRendu(HAUTE, d) == 1.f;
        moyenneOk = moyenneOk && echelleRendu(MOYENNE, d) >= 0.9f - 1e-6f && echelleRendu(MOYENNE, d) <= 1.f;
    }
    VERIFIE(hauteFixe, "Haute : toujours 100 %%");
    VERIFIE(moyenneOk && echelleRendu(MOYENNE, 1) == 1.f && std::abs(echelleRendu(MOYENNE, 0.6f) - 0.9f) < 1e-6f, "Moyenne : 90-100 %%");
    VERIFIE(echelleRendu(BASSE, 1) == 0.75f && echelleRendu(BASSE, 0.6f) == 0.75f, "Basse : 75 %% fixe (WFS-134 RG-03)");
    std::printf("échelle réelle (WFS-104, WFS-134) : Haute 100 %% fixe, Moyenne 90-100 %%, Basse 75 %% : OK\n");
    // WFS-134 (RG-02) : rétrogradation automatique et chauffe : la résolution reste celle du niveau quitté
    VERIFIE(niveauNettete(BASSE) == BASSE && echelleScene(BASSE, 1) == 0.75f, "sans baisse en cours : niveau du joueur");
    retenuChauffe = HAUTE;   // chauffe sévère : Haute → Moyenne puis → Basse à la course suivante
    VERIFIE(echelleScene(MOYENNE, 1) == 1.f && echelleScene(BASSE, 1) == 1.f && echelleScene(BASSE, 0.6f) == 1.f, "chauffe : Haute gardée à 100 %%");
    oublierRetenus(); retenuAuto = MOYENNE;   // rétrogradation automatique Moyenne → Basse
    VERIFIE(std::abs(echelleScene(BASSE, 0.6f) - 0.9f) < 1e-6f && echelleScene(BASSE, 1) == 1.f, "rétrogradation : plancher de la Moyenne gardé");
    retenuChauffe = HAUTE; VERIFIE(niveauNettete(BASSE) == HAUTE, "deux baisses : la plus haute netteté");
    oublierRetenus(); VERIFIE(echelleScene(BASSE, 1) == 0.75f, "choix du joueur : retenus oubliés");
    // la chauffe simulée de bout en bout (Regulateur de WFS-123 non lié ici) : aucune échelle sous le plancher du joueur
    bool jamaisSous = true;
    for (int choisi = BASSE; choisi <= HAUTE; choisi++) for (int q = BASSE; q <= choisi; q++) for (float d = 0.6f; d <= 1.0001f; d += 0.1f) {
        retenuChauffe = q < choisi ? choisi : -1;
        jamaisSous = jamaisSous && echelleScene(q, d) >= plancher(choisi) - 1e-6f;
    }
    oublierRetenus();
    VERIFIE(jamaisSous, "aucune baisse de résolution par la chauffe (WFS-134 RG-02)");
    std::printf("netteté retenue (WFS-134) : chauffe et rétrogradation ne baissent jamais la résolution : OK\n");
    // WFS-134 (RG-01) : ligne des Réglages
    const std::string l = texteRendu(HAUTE, 1.f, 4, 58.4f, 0), l2 = texteRendu(BASSE, 0.75f, 4, -1, 2);
    VERIFIE(l == "Rendu : Haute · 100 % · MSAA ×4 · 58 img/s · chauffe 0", "ligne « Rendu » (Haute)");
    VERIFIE(l2 == "Rendu : Basse · 75 % · MSAA ×4 · -- img/s · chauffe 2", "ligne « Rendu » (Basse, non mesuré)");
    std::printf("%s / %s : OK\n", l.c_str(), l2.c_str());
    // téléphone lent : bloom coupé à 0,8, ombres à 0,6, rien à 0,9 ; Basse : ni bloom ni ombres
    const Allegement a1 = allegement(HAUTE, 0.9f), a8 = allegement(HAUTE, 0.8f), a6 = allegement(MOYENNE, 0.6f), aB = allegement(BASSE, 1);
    VERIFIE(a1.bloom && a1.ombres && !a8.bloom && a8.ombres && !a6.bloom && !a6.ombres && !aB.bloom && !aB.ombres, "allègement");
    std::printf("allègement : bloom coupé à 0,8, ombres à 0,6, résolution intacte : OK\n");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("qualité : tout est OK\n");
    return 0;
}
