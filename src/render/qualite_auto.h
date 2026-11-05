// WaForSpeed — qualité automatique (WFS-15, épic E4) : résolution dynamique et rétrogradation unique.
// Port de watchPerf / setQuality (js/ui/screens.js) du jeu web : toutes les 2 s de course, images/s < 40 → échelle
// de la scène −0,1 (plancher 0,6) ; > 57 → +0,1 (plafond 1) ; à 0,6 et toujours sous 30 images/s, passage au niveau
// inférieur une seule fois (« Graphismes rapides pour plus de fluidité »), sauf si le joueur a choisi lui-même.
// Calcul seul (aucun appel Vulkan) : testé par tests/test_qualite.cpp.
#pragma once

namespace wfs::qualite {

enum Niveau { BASSE = 0, MOYENNE = 1, HAUTE = 2 };
// échelle de la scène 3D de chaque niveau (appliquerQualite de WFS-41) et force du bloom (coupé en Basse)
// WFS-104 : la voiture en course a la netteté du garage : 100 % en Moyenne et Haute (Moyenne : 75 % avant)
inline float echelleNiveau(int n) { return n == BASSE ? 0.5f : 1.f; }
inline float bloomNiveau(int n) { return n == BASSE ? 0.f : 1.f; }
// WFS-86 : échelle réelle de la scène = échelle du niveau × résolution dynamique, avec un plancher. Le jeu web applique
// resScale à un nombre de pixels déjà plafonné (DPR ≤ 1,5 sur téléphone) : il ne descend jamais sous ≈ 0,9 pixel CSS.
// Ici l'échelle porte sur les pixels physiques : sans plancher, Moyenne × 0,6 = 45 % (voiture floue en course).
// WFS-104 (RG-02) : la résolution ne descend plus sous la netteté du garage : plancher 100 % en Haute, 90 % en Moyenne
// (≈ 0,9 pixel CSS du jeu web) ; Basse fixe à 50 % (choix du joueur). Un téléphone trop lent perd d'autres coûts
// (allegement : bloom puis ombres), jamais la netteté de la voiture.
inline float plancher(int n) { return n == HAUTE ? 1.f : n == MOYENNE ? 0.9f : 0.5f; }
constexpr float PLANCHER = 0.9f;   // plancher de la Moyenne (WFS-86 : 0,6)
inline float echelleRendu(int n, float dynamique) {
    if (n == BASSE) return echelleNiveau(BASSE);
    const float e = echelleNiveau(n) * dynamique, p = plancher(n);
    return e < p ? p : (e > 1.f ? 1.f : e);
}
// WFS-104 : coûts baissés à la place de la résolution quand la résolution dynamique voudrait descendre
// (facteur dynamique ≤ 0,8 : bloom coupé ; ≤ 0,6 : ombres du soleil coupées) ; rétablis quand les images/s remontent
struct Allegement { bool bloom = true, ombres = true; };
inline Allegement allegement(int n, float dynamique) {
    Allegement a;
    if (n == BASSE) { a.bloom = false; a.ombres = false; return a; }   // réglages de la Basse (WFS-41)
    a.bloom = dynamique > 0.8f + 1e-4f; a.ombres = dynamique > 0.6f + 1e-4f;
    return a;
}

// premier lancement (aucun réglage enregistré) : Haute comme le jeu web, Basse sur un GPU logiciel
inline int niveauInitial(bool gpuLogiciel) { return gpuLogiciel ? BASSE : HAUTE; }

struct Auto {
    float echelle = 1.f;          // résolution dynamique (resScale), multipliée par l'échelle du niveau
    int niveau = HAUTE;
    bool choixJoueur = false;     // qualityChosen : niveau choisi par le joueur → pas de rétrogradation automatique
    bool fait = false;            // perfDone : rétrogradation déjà faite
    double t = 0; int n = 0;      // dynT, dynN

    enum Effet { RIEN = 0, ECHELLE = 1, RETRO = 2 };
    // une image de durée réelle `dt` (s) ; enCourse = état 'play' du jeu web ; renvoie ce qui a changé
    int image(double dt, bool enCourse);
    // WFS-123 (RG-03) : chauffe modérée ou plus (chauffe ≥ 1) : la résolution dynamique ne remonte plus (aucun +0,1)
    int imageChauffe(double dt, bool enCourse, int chauffe) {
        const float e0 = echelle; int ef = image(dt, enCourse);
        if (chauffe >= 1 && echelle > e0) { echelle = e0; ef &= ~ECHELLE; }
        return ef;
    }
    float fpsMesure = 0;          // dernière mesure sur 2 s (journal)
};

} // namespace wfs::qualite
