// WaForSpeed — son des chocs selon ce qu'on touche (WFS-298, épic E6) : au coup sourd actuel de la carrosserie (SfxId::Choc,
// Accident, gardés tels quels dans sfx.cpp) s'ajoute le son de l'obstacle, synthétisé en direct dans le thread audio (décision du
// PO, option A : la clarté suit exactement la vitesse) :
//   - RG-01 : famille d'après chocs::Genre (poteau, arbre, mobilier, béton, voiture ; « autre » : rien) ;
//   - RG-02 : synthèse modale (somme de sinusoïdes amorties excitées par le contact, van den Doel et coll. 2001) ;
//     décroissance de chaque mode T60 = 2,2 / (f · η), η de la matière montée (Bies, Engineering Noise Control, 5e éd., tableau C.1,
//     relu dans le dépôt jmrplens/phonometry ; references/reel/son/chocs.md) ;
//   - RG-03 : niveau ∝ vitesse d'impact (+6 dB par doublement, de 7 à 80 km/h), contact de Hertz : durée ∝ v^(−1/5), donc coupure de
//     l'excitation ∝ v^(1/5) (référence 30 km/h) ; 4 variantes par famille, jamais deux fois la même de suite ;
//   - RG-04 : côté : voitures::panExistant(azimut) (WFS-272), écho du tunnel comme les voix des autres voitures ;
//   - RG-07 : variantes et voix préparées dans build ; thread audio sans allocation ni verrou ; aucun fichier son.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "../game/chocs_decor.h"   // chocs::Genre (lecture seule)

namespace wfs::sonchocs {

// ---------- RG-01 ----------
enum Famille : int { POTEAU = 0, ARBRE, MOBILIER, BETON, VOITURE, NB_FAMILLES, AUCUNE = -1 };
int famillePour(chocs::Genre g);   // Poteau → POTEAU, Arbre → ARBRE, Mobilier → MOBILIER, Mur / Bâtiment / Îlot → BETON, Voiture → VOITURE
const char* nomFamille(int f);

// ---------- RG-02 : matières (facteur de perte de la matière montée, Bies 5e éd. tableau C.1, colonne « in situ ») ----------
constexpr float ETA_ACIER_MAT = 0.01f;     // acier boulonné (mât d'éclairage, poteau) : borne basse de 0,01-0,02
constexpr float ETA_ACIER_TOLE = 0.02f;    // tôle d'acier (mobilier, carrosserie) : borne haute de 0,01-0,02
constexpr float ETA_BOIS = 0.05f;          // bois (chêne, pin, sapin…) : 0,05
constexpr float ETA_PLASTIQUE = 0.10f;     // polypropylène, PVC, polycarbonate : 0,1
constexpr float ETA_VERRE = 0.02f;         // verre (éclats des feux) : 0,02
inline float t60Mode(float f, float eta) { return 2.2f / (f * eta); }   // s
constexpr float T60_BETON = 0.08f;         // masse immobile tenue par le sol : coup sans résonance (≤ 0,1 s, valeur du PO [à vérifier])
constexpr float T60_FEUILLES = 0.8f;       // feuillage qui tremble (0,6 à 1 s, valeur du PO [à vérifier]), bruit de 2 à 8 kHz

struct Mode { float f = 0, a = 0, t60 = 0; };
constexpr int MODES_MAX = 8;
struct Matiere {   // famille : modes de référence (avant variante), décroissance retenue (mode le plus long), durée du son
    const char* nom;
    Mode modes[MODES_MAX]; int n;
    float contact30;   // s : durée du contact à 30 km/h (coupure de l'excitation 1 / durée) [à vérifier]
    float duree;       // s : son entendu au plus
};
const Matiere& matiere(int famille);
float t60Retenu(int famille);   // s : décroissance du mode le plus long (béton : T60_BETON)

// ---------- RG-03 : vitesse d'impact ----------
constexpr float V_MIN = 2.f;               // m/s : 7 km/h, seuil du choc (chocs_cartes.cpp : en dessous, frottement)
constexpr float V_MAX = 80.f / 3.6f;       // m/s : 80 km/h, niveau plafonné au-delà
constexpr float V_REF = 30.f / 3.6f;       // m/s : 30 km/h, référence du niveau et de la clarté
inline float gainVitesse(float v) { return std::min(std::max(v, V_MIN), V_MAX) / V_REF; }   // amplitude ∝ vitesse (+6 dB par doublement)
float dureeContact(int famille, float v);  // s : contact30 · (V_REF / v)^(1/5) (Hertz, Johnson 1985 § 11.4 : t ∝ v^(−1/5))
inline float coupureExcitation(int famille, float v) { return 1.f / dureeContact(famille, v); }   // Hz : ∝ v^(1/5)

// ---------- RG-05 : niveaux (décision du PO ; le volume absolu dépend du téléphone : non mesurable) ----------
// crête du son de l'obstacle à 30 km/h, par rapport à la crête du coup sourd du même choc (SfxId::Choc de sfx.cpp : noiseHit 0,3 s,
// 200 Hz, gain 0,4 + impact) : −4,5 dB au centre, −1,6 dB du côté de l'obstacle (panoramique de WFS-272 : × 1,4) (règle : −6 à 0 dB) ; éclats de l'accident : crête 0,6 × celle de la tôle
constexpr float ECART_CRETE_DB = -4.5f;
constexpr float ECLATS = 0.6f;
constexpr int VARIANTES = 4, VOIX = 4, ECLATS_N = 24;

// ---------- RG-04 : ce qui est touché, et de quel côté (calcul pur) ----------
// rectangle orienté au sens du jeu : centre (x, z), avant = (sin cap, −cos cap), droite = (cos cap, sin cap), demi-longueur, demi-largeur
struct Rect { double x = 0, z = 0, cap = 0, hl = 0, hw = 0; };
// distance (m, < 0 : enfoncé) entre la voiture et un obstacle du registre ; (qx, qz) : point de l'obstacle le plus proche de la voiture
double distance(const Rect& voiture, const chocs::Obstacle& o, double& qx, double& qz);
// distance entre deux rectangles (sommet contre côté) ; (qx, qz) : point de b le plus proche de a
double distance(const Rect& a, const Rect& b, double& qx, double& qz);
float azimut(const Rect& voiture, double qx, double qz);   // rad : 0 devant, + à droite, ± π derrière
constexpr double PORTEE_OBSTACLE = 0.5, PORTEE_VOITURE = 1.0;   // m : RG-04

// ---------- son ----------
class SonsChocs {
public:
    void build(float rate);   // thread du jeu : variantes (modes ± 5 %, graines), normalisation des crêtes, voix (aucune allocation ensuite)
    // thread du jeu : un choc de la famille f à v m/s, venu de gauche (gainG > gainD) ou de droite (panoramique de WFS-272) ;
    // accident : éclats (voiture) ; rend la variante jouée (−1 : aucune)
    int choc(int f, float v, float panG, float panD, bool accident);
    // thread audio : ajoute les chocs en cours dans G, D et le bus de l'écho (n trames) ; rien n'est touché sans choc
    void mix(float* G, float* D, float* bus, int n, float master);
    // lecture (tests, banc)
    float cible30() const { return cible30_; }          // crête visée à 30 km/h (avant master)
    float crete30(int f, int v) const { return crete30_[f][v]; }   // crête mesurée dans build, avant normalisation
    int actives() const { int k = 0; for (const Voix& x : voix_) k += x.active; return k; }
    int derniere(int f) const { return derniere_[f]; }
    const Mode& mode(int f, int variante, int k) const { return var_[f][variante].modes[k]; }

private:
    struct Resonateur { float c1 = 0, c2 = 0, b = 0, y1 = 0, y2 = 0; };
    struct Eclat { int debut = 0, fin = 0; float c1 = 0, c2 = 0, b = 0; };   // résonateur d'un éclat (verre ou plastique), de debut à fin
    struct Variante { Mode modes[MODES_MAX]; int n = 0; uint32_t graine = 1; float norm = 1; Eclat eclats[ECLATS_N]; };
    struct Voix {
        bool active = false, accident = false; int f = 0, v = 0; int t = 0, fin = 0, nc = 1;
        float amp = 0, gG = 0, gD = 0, aire = 0;
        Resonateur r[MODES_MAX]; int n = 0;
        uint32_t bruit = 1; float bpY1 = 0, bpY2 = 0, bpX1 = 0, bpX2 = 0, flutter = 0, envF = 1;
        float ey1[ECLATS_N] = {}, ey2[ECLATS_N] = {};
    };
    struct Demande { int f = 0, variante = 0; float v = 0, gG = 0, gD = 0; bool accident = false; };
    static constexpr uint32_t FILE = 8;
    void demarrer(Voix& x, const Demande& d);
    static constexpr int BLOC_SYNTH = 256;
    void bloc(Voix& x, float* y, int n);   // n trames (≤ BLOC_SYNTH) de la voix, avant amp et master

    float rate_ = 48000, cible30_ = 0.05f;
    Variante var_[NB_FAMILLES][VARIANTES];
    float crete30_[NB_FAMILLES][VARIANTES] = {};
    int derniere_[NB_FAMILLES] = {-1, -1, -1, -1, -1};
    uint32_t hasard_ = 0x2545F491u;
    Voix voix_[VOIX];
    float bpB0_ = 0, bpB2_ = 0, bpA1_ = 0, bpA2_ = 0;   // feuillage : passe-bande 2-8 kHz (centre 4 kHz)
    float decFeuilles_ = 1, attaqueFeuilles_ = 1;      // feuillage : décroissance par trame (T60 0,8 s), montée (trames)
    Demande file_[FILE];
    std::atomic<uint32_t> tete_{0}, queue_{0};
    std::atomic<bool> pret_{false};
};

// mesures (banc, tests) : T60 (s) de x dans la bande d'un tiers d'octave autour de fBande (0 : large bande), d'après la pente de
// la courbe de décroissance de Schroeder entre −5 et −35 dB ; niveau RMS (dB) de la bande d'un tiers d'octave autour de f
float mesurerT60(const std::vector<float>& x, float rate, float fBande);
float niveauTiers(const std::vector<float>& x, size_t a, size_t n, float rate, float f);
// coup sourd actuel de la carrosserie (mêmes réglages que SfxMixer::build de sfx.cpp) : petit choc (Choc) et accident (Accident)
std::vector<float> coupSourd(float rate, bool accident);

// banc d'essai PC (RG-08) : chocs-<famille>.wav (15, 30, 50, 80 km/h, obstacle à droite puis à gauche) et chocs-<famille>-avant.wav (coup
// sourd actuel seul), chocs-familles.wav (les 5 familles à 30 km/h) ; .png : niveaux gauche / droite et spectre ; tableau au journal, coût
void benchChocs(const std::string& out);

} // namespace wfs::sonchocs
