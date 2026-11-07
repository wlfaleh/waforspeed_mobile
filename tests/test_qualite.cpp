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

    // 25 images/s : WFS-222 (RG-03) : −0,2 toutes les 2 s sous 30 images/s jusqu'à 0,6, puis rétrogradation unique
    Auto a;
    VERIFIE(course(a, 25, 2) & Auto::ECHELLE && std::abs(a.echelle - 0.8f) < 1e-6f, "1re baisse (%.2f)", a.echelle);
    // watchPerf : la rétrogradation est testée juste après la baisse, dans la même mesure que l'arrivée à 0,6 (4 s)
    VERIFIE(course(a, 25, 2) & Auto::RETRO && std::abs(a.echelle - 0.6f) < 1e-6f && a.niveau == MOYENNE && a.fait,
            "à 0,6 sous 30 images/s : rétrogradation (%.2f, niveau %d)", a.echelle, a.niveau);
    course(a, 25, 10);
    VERIFIE(a.niveau == MOYENNE, "une seule rétrogradation (niveau %d)", a.niveau);
    std::printf("25 images/s : échelle 1 → 0,6 par pas de 0,2 toutes les 2 s (WFS-222), Haute → Moyenne à 0,6, une seule fois : OK\n");

    // 35 images/s : l'échelle baisse mais pas de rétrogradation (≥ 30)
    Auto b; course(b, 35, 20);
    VERIFIE(std::abs(b.echelle - 0.6f) < 1e-6f && b.niveau == HAUTE && !b.fait, "35 images/s : pas de rétrogradation");
    // 60 images/s : WFS-246 : les effets reviennent d'abord (2 paliers, 4 s), puis l'échelle remonte jusqu'à 1 (8 s)
    VERIFIE(b.palier == 2, "35 images/s au plancher : effets coupés après la résolution (palier %d)", b.palier);
    course(b, 60, 14);
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

    // WFS-246 (levier 3) : résolution adaptative à tous les niveaux, de 100 % (Basse 75 %) jusqu'à 60 %, jamais en dessous
    bool dansBornes = true;
    for (int q = BASSE; q <= HAUTE; q++) for (float d = 0.3f; d <= 1.0001f; d += 0.05f)
        dansBornes = dansBornes && echelleRendu(q, d) >= ECHELLE_MIN - 1e-6f && echelleRendu(q, d) <= echelleNiveau(q) + 1e-6f;
    VERIFIE(dansBornes, "échelle toujours entre 60 %% et celle du niveau");
    VERIFIE(echelleRendu(HAUTE, 1) == 1.f && std::abs(echelleRendu(HAUTE, 0.6f) - 0.6f) < 1e-6f && std::abs(echelleRendu(MOYENNE, 0.8f) - 0.8f) < 1e-6f, "Haute / Moyenne : 100 → 60 %%");
    VERIFIE(echelleRendu(BASSE, 1) == 0.75f && std::abs(echelleRendu(BASSE, 0.8f) - 0.6f) < 1e-6f && std::abs(echelleRendu(BASSE, 0.6f) - 0.6f) < 1e-6f, "Basse : 75 → 60 %%");
    VERIFIE(budgetMs(BASSE) < 16.8f && budgetMs(HAUTE) == 22.f && budgetMs(MOYENNE) == 22.f, "budgets 16,7 / 22 ms");
    std::printf("échelle réelle (WFS-246) : Haute et Moyenne 100 → 60 %%, Basse 75 → 60 %% : OK\n");
    // WFS-246 : Basse à 50 images/s (20 ms > 16,7) : la résolution descend à 60 % (facteur 0,8), puis les effets
    { Auto f; f.niveau = BASSE; course(f, 50, 2);
      VERIFIE(std::abs(f.echelle - 0.9f) < 1e-6f && f.palier == 0, "Basse 50 img/s : 1re baisse (%.2f)", f.echelle);
      course(f, 50, 2); VERIFIE(std::abs(f.echelle - 0.8f) < 1e-6f && f.palier == 0, "Basse : scène à 60 %% (%.2f)", f.echelle);
      course(f, 50, 2); VERIFIE(f.palier == 1 && std::abs(f.echelle - 0.8f) < 1e-6f, "Basse : effets après la résolution (palier %d)", f.palier);
      // image calée sur 60 Hz : sans horodatages, aucune remontée en Basse ; GPU à 9 ms (< 65 % de 16,7) : remontée
      course(f, 60, 4); VERIFIE(f.palier == 1 && std::abs(f.echelle - 0.8f) < 1e-6f, "Basse 60 Hz sans mesure GPU : rien ne bouge");
      int e = 0; for (int i = 0; i < 60 * 8; i++) e |= f.image(1.0 / 60, true, 9.0);
      VERIFIE(f.palier == 0 && std::abs(f.echelle - 1.f) < 1e-6f && (e & Auto::ECHELLE), "Basse GPU 9 ms : effets puis 75 %% (%.2f, palier %d)", f.echelle, f.palier);
      // Haute 60 Hz, GPU 18 ms (> 65 % de 22) : pas de remontée (pas d'aller-retour)
      Auto h; h.echelle = 0.7f; for (int i = 0; i < 60 * 6; i++) h.image(1.0 / 60, true, 18.0);
      VERIFIE(std::abs(h.echelle - 0.7f) < 1e-6f, "Haute GPU 18 ms : échelle gardée (%.2f)", h.echelle);
      std::printf("Basse 50 img/s : 75 → 60 %% puis effets ; remontée au temps GPU, sans aller-retour : OK\n"); }
    // WFS-134 (RG-02) : rétrogradation automatique et chauffe : la résolution reste celle du niveau quitté
    VERIFIE(niveauNettete(BASSE) == BASSE && echelleScene(BASSE, 1) == 0.75f, "sans baisse en cours : niveau du joueur");
    retenuChauffe = HAUTE;   // chauffe sévère : Haute → Moyenne puis → Basse à la course suivante
    VERIFIE(echelleScene(MOYENNE, 1) == 1.f && echelleScene(BASSE, 1) == 1.f && echelleScene(BASSE, 0.6f) == echelleScene(HAUTE, 0.6f), "chauffe : Haute gardée à 100 %%");
    oublierRetenus(); retenuAuto = MOYENNE;   // rétrogradation automatique Moyenne → Basse
    VERIFIE(echelleScene(BASSE, 0.8f) == echelleScene(MOYENNE, 0.8f) && echelleScene(BASSE, 1) == 1.f, "rétrogradation : échelle de la Moyenne gardée");
    retenuChauffe = HAUTE; VERIFIE(niveauNettete(BASSE) == HAUTE, "deux baisses : la plus haute netteté");
    oublierRetenus(); VERIFIE(echelleScene(BASSE, 1) == 0.75f, "choix du joueur : retenus oubliés");
    // la chauffe simulée de bout en bout (Regulateur de WFS-123 non lié ici) : aucune échelle sous le plancher du joueur
    bool jamaisSous = true;
    for (int choisi = BASSE; choisi <= HAUTE; choisi++) for (int q = BASSE; q <= choisi; q++) for (float d = 0.6f; d <= 1.0001f; d += 0.1f) {
        retenuChauffe = q < choisi ? choisi : -1;
        jamaisSous = jamaisSous && std::abs(echelleScene(q, d) - echelleScene(choisi, d)) < 1e-6f;   // WFS-246 : même échelle qu'au niveau choisi
    }
    oublierRetenus();
    VERIFIE(jamaisSous, "aucune baisse de résolution par la chauffe (WFS-134 RG-02)");
    std::printf("netteté retenue (WFS-134) : chauffe et rétrogradation ne baissent jamais la résolution : OK\n");
    // WFS-134 (RG-01) : ligne des Réglages
    const std::string l = texteRendu(HAUTE, 1.f, 4, 58.4f, 0), l2 = texteRendu(BASSE, 0.75f, 4, -1, 2);
    VERIFIE(l == "Rendu : Haute · 100 % · MSAA ×4 · 58 img/s · chauffe 0", "ligne « Rendu » (Haute)");
    VERIFIE(l2 == "Rendu : Basse · 75 % · MSAA ×4 · -- img/s · chauffe 2", "ligne « Rendu » (Basse, non mesuré)");
    std::printf("%s / %s : OK\n", l.c_str(), l2.c_str());
    // WFS-246 : allègement par paliers, après la résolution : 1 = bloom, reflets, captures ; 2 = + ombres ; Basse : ni bloom ni ombres
    const Allegement a0 = allegement(HAUTE, 0), a1 = allegement(HAUTE, 1), a2 = allegement(MOYENNE, 2), aB = allegement(BASSE, 0), b1 = allegement(BASSE, 1);
    VERIFIE(a0.bloom && a0.ombres && a0.reflets && a0.captures && !a1.bloom && a1.ombres && !a1.reflets && !a1.captures, "allègement palier 1");
    VERIFIE(!a2.bloom && !a2.ombres && !a2.reflets && !a2.captures && !aB.bloom && !aB.ombres && aB.reflets && aB.captures && !b1.reflets && !b1.captures, "allègement palier 2, Basse");
    std::printf("allègement (WFS-246) : palier 1 bloom / reflets / captures, palier 2 ombres, Basse sans bloom ni ombres : OK\n");

    // WFS-144 (RG-04) : Basse à MSAA ×2 et anisotrope ×4 ; Moyenne / Haute inchangées ; netteté retenue ; GPU sans ×2 ou sans MSAA
    oublierRetenus();
    VERIFIE(msaaScene(BASSE, 4, true) == 2 && anisoScene(BASSE, 8) == 4.f, "Basse : MSAA ×2, aniso ×4");
    VERIFIE(msaaScene(MOYENNE, 4, true) == 4 && msaaScene(HAUTE, 4, true) == 4 && anisoScene(MOYENNE, 8) == 8.f && anisoScene(HAUTE, 8) == 8.f, "Moyenne / Haute : ×4 / ×8");
    VERIFIE(msaaScene(BASSE, 4, false) == 4 && msaaScene(BASSE, 1, true) == 1 && anisoScene(BASSE, 2) == 2.f, "GPU sans ×2, sans MSAA, aniso ×2");
    retenuAuto = HAUTE; VERIFIE(msaaScene(BASSE, 4, true) == 4 && anisoScene(BASSE, 8) == 8.f, "Basse par rétrogradation : netteté de la Haute gardée");
    oublierRetenus();
    std::printf("netteté de la Basse (WFS-144) : MSAA ×2, aniso ×4, retenus respectés : OK\n");

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("qualité : tout est OK\n");
    return 0;
}
