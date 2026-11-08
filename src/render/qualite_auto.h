// WaForSpeed — qualité automatique (WFS-15, épic E4) : résolution dynamique et rétrogradation unique.
// Port de watchPerf / setQuality (js/ui/screens.js) du jeu web : toutes les 2 s de course, images/s < 40 → échelle
// de la scène −0,1 (plancher 0,6) ; > 57 → +0,1 (plafond 1) ; à 0,6 et toujours sous 30 images/s, passage au niveau
// inférieur une seule fois (« Graphismes rapides pour plus de fluidité »), sauf si le joueur a choisi lui-même.
// Calcul seul (aucun appel Vulkan) : testé par tests/test_qualite.cpp.
#pragma once
#include <cstdint>
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
// WFS-246 (RG-02, levier 3 ; ANO-125 : 10 img/s en Haute sur le téléphone) : RÉSOLUTION ADAPTATIVE à tous les niveaux. Le
// téléphone rend la scène à la définition de son écran (2400 × 1080 et plus : 3 fois les pixels du banc). Au-delà du budget
// de l'image (budgetMs : 16,7 ms en Basse, 22 ms en Moyenne / Haute), la scène descend jusqu'à 60 % AVANT toute coupe
// d'effet ; l'interface reste à la pleine définition ; remontée dès que le GPU a de la marge. Remplace les planchers fixes
// de WFS-104 / WFS-134 (Haute 100 %, Moyenne 90 %, Basse 75 % fixe), qui laissaient la Haute à 10 img/s.
constexpr float ECHELLE_MIN = 0.6f;
inline float budgetMs(int n) { return n == BASSE ? 16.7f : 22.f; }
inline const float& PLANCHER = g_configJeu.qualite.plancherMoyenne;   // WFS-128 (clé qualite.plancher_moyenne) : plus lue depuis WFS-246
inline float plancher(int) { return ECHELLE_MIN; }
inline float echelleRendu(int n, float dynamique) {
    const float m = echelleNiveau(n), e = m * dynamique;
    return e < ECHELLE_MIN ? ECHELLE_MIN : (e > m ? m : e);
}
// WFS-246 (levier 3) : les effets ne sont coupés qu'APRÈS la résolution, quand la scène est déjà à 60 % et l'image toujours
// hors budget : palier 1 (une mesure de 2 s de plus) : bloom, reflets de la route mouillée et captures d'environnement
// (WFS-222 : les sondes en place restent, aucun saut de lumière) ; palier 2 : ombres du soleil. Rétablis dans l'ordre
// inverse, avant toute remontée de la résolution.
struct Allegement { bool bloom = true, ombres = true, reflets = true, captures = true; };
inline Allegement allegement(int n, int palier) {
    Allegement a;
    a.reflets = a.captures = a.bloom = palier < 1;
    a.ombres = palier < 2;
    if (n == BASSE) { a.bloom = false; a.ombres = false; }   // réglages de la Basse (WFS-41)
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
// WFS-317 (I9) : budget de dessin de la dernière image (toutes passes) : « 420 appels · 744 k triangles (décor 63 %, voitures 13 %,
// personnes 24 %) » ; detail : appels et triangles de chaque famille (fichier de mesure)
std::string texteDessin(bool detail);
// mise en forme seule (testée par tests/test_qualite.cpp) : appels et triangles des 3 familles (décor, voitures, personnes) ;
// "" si aucun appel ; parts arrondies au pour cent ; familles approchées : conducteurs et pilotes comptés en personnes,
// poussettes et objets non animés en décor
std::string texteComptes(const uint32_t appels[3], const uint32_t triangles[3], bool detail);

// WFS-246 (RG-06) : mesure sur le téléphone : durées des images sur une fenêtre glissante de 20 s de course (médiane, 95 %,
// 99 %, images > 33 ms et > 50 ms). Calcul seul (testé par tests/test_qualite.cpp) ; le système de qualité l'alimente.
struct MesureImages {
    static constexpr double FENETRE_S = 20;
    static constexpr int CAP = 20 * 144;     // 144 Hz au plus
    float ms[CAP] = {}; int n = 0, tete = 0; double duree = 0;   // tete : prochaine place ; duree : somme des n gardées (ms)
    void ajouter(float msImage);
    void vider() { n = 0; tete = 0; duree = 0; }
    struct Stats { float mediane = 0, p95 = 0, p99 = 0, ecartType = 0; int plus33 = 0, plus50 = 0, images = 0; float secondes = 0; };   // WFS-262 (RG-05) : écart type
    Stats stats() const;
};
// WFS-262 (RG-04) : cadence de la course, choisie automatiquement : 60 img/s ; 30 quand le téléphone ne tient pas 60 alors
// que la scène est déjà à 60 % et les effets allégés (95 % des images > 20 ms pendant 6 s) : 30 régulières se sentent mieux
// que 40 à 55 irrégulières. Retour à 60 quand le GPU a de la marge (< 11 ms pendant 10 s) ; sans horodatages GPU, nouvel
// essai à 60 toutes les 30 s (jamais bloquée à 30). Remise à 60 au début de chaque course. Lu par chauffe::plafondIps.
// DÉSACTIVÉE par défaut (cadenceAuto = faux : 60 toujours) : l'utilisateur n'a pas encore accepté 30 img/s comme étape
// (question du PO, tache utilisateur.md, 2026-11-07 17:14) ; s'active par un réglage (décision du PO, WFS-262).
struct Cadence {
    int ips = 60; double tSous = 0, tMarge = 0;
    // dt : durée de l'image (s) ; p95 : 95 % des images de la fenêtre (ms) ; gpuMs ≤ 0 : inconnu ; renvoie vrai si changée
    bool image(double dt, bool enCourse, bool auPlancher, float p95, double gpuMs) {
        if (!enCourse) { tSous = tMarge = 0; return false; }
        const int avant = ips;
        if (ips == 60) { tSous = auPlancher && p95 > 20.f ? tSous + dt : 0; if (tSous >= 6) { ips = 30; tSous = 0; } }
        else {
            if (gpuMs > 0) tMarge = gpuMs < 11 ? tMarge + dt : 0;
            else tMarge += dt / 3;   // sans mesure du GPU : 30 s à 30 img/s, puis nouvel essai à 60
            if (tMarge >= 10) { ips = 60; tMarge = 0; }
        }
        return ips != avant;
    }
};
inline int cadenceCourse = 60;   // plafond des images/s en course (chauffe::plafondIps)
inline bool cadenceAuto = false;  // WFS-262 : passage automatique à 30 permis (faux tant que l'utilisateur n'a pas dit oui)

// texte du fichier rendu-mesure.txt (et de la feuille de partage), ligne courte des Réglages ; vides avant toute mesure
std::string texteMesure();
std::string ligneMesure();
void mesureEssai(const float* ms, int n);   // banc : remplit la fenêtre (capture des Réglages)
inline std::string dossierMesure;   // Android : externalDataPath (android_main.cpp) ; vide : aucun fichier écrit

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
    // WFS-246 : gpuMs = temps GPU de l'image 3D (horodatages, moyenne lissée ; ≤ 0 : inconnu) : seule mesure de la marge
    // quand l'image est calée sur 60 Hz (images/s plafonnées par l'écran)
    int image(double dt, bool enCourse, double gpuMs = -1);
    // WFS-123 (RG-03) : chauffe modérée ou plus (chauffe ≥ 1) : la résolution dynamique ne remonte plus (aucun +0,1)
    int imageChauffe(double dt, bool enCourse, int chauffe, double gpuMs = -1) {
        const float e0 = echelle; const int p0 = palier; int ef = image(dt, enCourse, gpuMs);
        if (chauffe >= 1 && echelle > e0) { echelle = e0; ef &= ~ECHELLE; }
        if (chauffe >= 1 && palier < p0) palier = p0;   // WFS-246 : les effets coupés ne reviennent pas non plus
        return ef;
    }
    int palier = 0;               // WFS-246 : allègement des effets (0 aucun, 1 bloom / reflets / captures, 2 + ombres)
    float echelleMin() const;     // WFS-246 : facteur qui met la scène à 60 % (Basse : 0,8 × 75 %)
    float fpsMesure = 0;          // dernière mesure sur 2 s (journal)
    int mesures = 0;              // WFS-150 : nombre de mesures de 2 s faites (tenue de la Haute pour proposer Ultra)
};

} // namespace wfs::qualite
