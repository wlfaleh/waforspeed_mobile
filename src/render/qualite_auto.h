// WaForSpeed — qualité automatique (WFS-15, épic E4) : résolution dynamique et rétrogradation unique.
// Port de watchPerf / setQuality (js/ui/screens.js) du jeu web : toutes les 2 s de course, images/s < 40 → échelle
// de la scène −0,1 (plancher 0,6) ; > 57 → +0,1 (plafond 1) ; à 0,6 et toujours sous 30 images/s, passage au niveau
// inférieur une seule fois (« Graphismes rapides pour plus de fluidité »), sauf si le joueur a choisi lui-même.
// Calcul seul (aucun appel Vulkan) : testé par tests/test_qualite.cpp.
#pragma once
#include <string>
#include "../core/configgame.h"   // WFS-128

namespace wfs::qualite {

enum Niveau { BASSE = 0, MOYENNE = 1, HAUTE = 2, ULTRA = 3 };
// WFS-150 (RG-04, P-01) : Ultra = une valeur de plus dans le choix « Graphismes » des Réglages. Les coûts de la partie
// (reglages().qualite : ombres, bloom, décor, flaques, distance) restent ceux de la Haute (valeur 2) : Basse, Moyenne et
// Haute gardent leur coût exact ; Ultra ajoute seulement les passes de qualiteMin ULTRA (src/render/passes.h) et la
// capture d'environnement en 256 px. ultraChoisi = choix du joueur (clé « ultra=1 » de reglages.txt) ; il ne vaut que si
// la qualité des coûts est Haute (une rétrogradation de chauffe ou automatique l'efface tant qu'elle dure).
inline bool ultraChoisi = false;
inline int niveauAffiche(int q) { return q == HAUTE && ultraChoisi ? ULTRA : q; }
// RG-04 : Ultra proposé seulement si le téléphone tient la Haute (« banc de chauffe » en jeu) : en course, en Haute, sans
// chauffe (niveau 0), au moins IPS_TENUE images/s sur MESURES_TENUE mesures de 2 s de suite (10 s). Une mesure sous le seuil
// ou une chauffe remet la série à zéro. Une fois tenu (ou Ultra déjà choisi), Ultra reste proposé pour la session.
constexpr float IPS_TENUE = 57.f;    // seuil de remontée de la résolution dynamique (watchPerf du jeu web)
constexpr int MESURES_TENUE = 5;
struct TenueUltra {
    int suite = 0; bool tenue = false;
    void mesure(float ips, int niveau, int chauffe) {
        if (niveau == HAUTE && chauffe == 0 && ips >= IPS_TENUE) { if (++suite >= MESURES_TENUE) tenue = true; }
        else suite = 0;
    }
};
inline TenueUltra tenueUltra;
inline bool ultraPropose() { return ultraChoisi || tenueUltra.tenue; }
// RG-03 : côté de la cubemap d'environnement selon le niveau affiché (Moyenne : comme la Haute)
inline int tailleCubemap(int niveau) { return niveau >= ULTRA ? 256 : niveau <= BASSE ? 64 : 128; }
// RG-05 : mode diagnostic (coût GPU de chaque passe dans la ligne « Rendu » des Réglages) : WFS_DIAGNOSTIC=1 sur PC,
// propriété Android debug.waforspeed.diagnostic=1 (adb shell setprop) ; coupé par défaut (captures inchangées)
bool modeDiagnostic();
inline bool diagnosticForce = false;   // banc d'essai : capture reglages-diagnostic
// échelle de la scène 3D de chaque niveau (appliquerQualite de WFS-41) et force du bloom (coupé en Basse)
// WFS-104 : la voiture en course a la netteté du garage : 100 % en Moyenne et Haute (Moyenne : 75 % avant)
// WFS-134 (RG-03) : Basse à 75 % (50 % avant : voiture en escalier sur le téléphone)
inline float echelleNiveau(int n) { return n == BASSE ? 0.75f : 1.f; }
inline float bloomNiveau(int n) { return n == BASSE ? 0.f : 1.f; }
// WFS-86 : échelle réelle de la scène = échelle du niveau × résolution dynamique, avec un plancher. Le jeu web applique
// resScale à un nombre de pixels déjà plafonné (DPR ≤ 1,5 sur téléphone) : il ne descend jamais sous ≈ 0,9 pixel CSS.
// Ici l'échelle porte sur les pixels physiques : sans plancher, Moyenne × 0,6 = 45 % (voiture floue en course).
// WFS-104 (RG-02) : la résolution ne descend plus sous la netteté du garage : plancher 100 % en Haute, 90 % en Moyenne
// (≈ 0,9 pixel CSS du jeu web) ; Basse fixe à 50 % (choix du joueur). Un téléphone trop lent perd d'autres coûts
// (allegement : bloom puis ombres), jamais la netteté de la voiture.
inline const float& PLANCHER = g_configJeu.qualite.plancherMoyenne;   // WFS-128 : 0,9 ;   // plancher de la Moyenne (WFS-86 : 0,6)
inline float plancher(int n) { return n == HAUTE ? 1.f : n == MOYENNE ? PLANCHER : echelleNiveau(BASSE); }   // WFS-134 : Basse 75 %   // Moyenne : qualite.plancher_moyenne
inline float echelleRendu(int n, float dynamique) {
    if (n == BASSE) return echelleNiveau(BASSE);
    const float e = echelleNiveau(n) * dynamique, p = plancher(n);
    return e < p ? p : (e > 1.f ? 1.f : e);
}
// WFS-104 : coûts baissés à la place de la résolution quand la résolution dynamique voudrait descendre
// (facteur dynamique ≤ 0,8 : bloom coupé ; ≤ 0,6 : ombres du soleil coupées) ; rétablis quand les images/s remontent
// WFS-222 (RG-03) : au plancher (≤ 0,6, sous 40 images/s), à tous les niveaux, Basse comprise : reflets de la route mouillée
// coupés et captures d'environnement suspendues (les sondes en place restent : aucun saut de lumière) ; netteté intacte
struct Allegement { bool bloom = true, ombres = true, reflets = true, captures = true; };
inline Allegement allegement(int n, float dynamique) {
    Allegement a;
    a.reflets = a.captures = dynamique > 0.6f + 1e-4f;
    if (n == BASSE) { a.bloom = false; a.ombres = false; return a; }   // réglages de la Basse (WFS-41)
    a.bloom = dynamique > 0.8f + 1e-4f; a.ombres = dynamique > 0.6f + 1e-4f;
    return a;
}

// WFS-134 (RG-02) : pas de rétrogradation cachée. La rétrogradation automatique et la chauffe (WFS-123) baissent le
// niveau des COÛTS (reglages().qualite : ombres, bloom, décor lointain, piétons, distance d'affichage), jamais la
// netteté : résolution et MSAA de la scène restent ceux du niveau d'où l'on est descendu (niveau retenu ci-dessous).
// retenuAuto : niveau d'avant la rétrogradation automatique (qualite_systeme.cpp) ; retenuChauffe : qualité choisie à
// rétablir après la chauffe (Regulateur::choisi de WFS-123) ; −1 : aucune baisse en cours. Un choix du joueur dans les
// réglages les remet à −1 (le niveau choisi devient la netteté).
inline int retenuAuto = -1, retenuChauffe = -1;
inline void oublierRetenus() { retenuAuto = retenuChauffe = -1; }
// niveau dont la scène prend la résolution : jamais sous un niveau retenu
inline int niveauNettete(int q) {
    if (q < retenuAuto) q = retenuAuto;
    if (q < retenuChauffe) q = retenuChauffe;
    return q > HAUTE ? HAUTE : q;
}
inline float echelleScene(int q, float dynamique) { return echelleRendu(niveauNettete(q), dynamique); }
// WFS-144 (RG-04, WFS-134 RG-03 / RG-04) : MSAA et anisotrope de la scène selon le niveau de netteté (q : qualité des coûts) ;
// Basse : ×2 et ×4 ; Moyenne / Haute : ceux du GPU (msaaGpu ×4 ou ×1, anisoGpu ×8 au plus). x2 : le GPU accepte le MSAA ×2
// (sinon ×4 gardé : lavapipe du banc). Netteté retenue (rétrogradation, chauffe) : ceux du niveau quitté.
inline bool netteteBasse(int q) { return q == BASSE && niveauNettete(q) == BASSE; }
inline int msaaScene(int q, int msaaGpu, bool x2) { return netteteBasse(q) && msaaGpu > 2 && x2 ? 2 : msaaGpu; }
inline float anisoScene(int q, float anisoGpu) { return netteteBasse(q) && anisoGpu > 4.f ? 4.f : anisoGpu; }

// WFS-134 (RG-01) : ligne « Rendu : Haute · 100 % · MSAA ×4 · 58 img/s · chauffe 0 » (ips < 0 : pas encore mesuré)
std::string texteRendu(int niveau, float echelle, int msaa, float ips, int chauffe);
// ligne du rendu réellement utilisé, lue en direct (qualite_systeme.cpp) : Réglages, journal, rapport partagé
std::string ligneRendu();

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
    int mesures = 0;              // WFS-150 : nombre de mesures de 2 s faites (tenue de la Haute pour proposer Ultra)
};

} // namespace wfs::qualite
