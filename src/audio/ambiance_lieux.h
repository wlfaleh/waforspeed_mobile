// WaForSpeed — son des lieux (WFS-261, épic E6) : sources sonores placées dans le monde, écho des tunnels, sortie stéréo.
//
// Avant WFS-261, l'ambiance de la carte était une seule boucle mono (musique.*, classe Ambiance). Ici, chaque lieu sonne
// LÀ OÙ IL EST, sans rien retirer de l'existant :
//   - RG-02 : sources déclarées par les cartes (registre `lieux::poser`, accroches de lecture dans ville.cpp, plage.cpp,
//     circuit.cpp, campagne.cpp ; terrasses lues dans vie::clientsTerrasses) : fontaines des ronds-points, rivage de la
//     Corniche, tribunes du Grand Prix, troupeaux de la Campagne, terrasses de la Ville. Sons fabriqués par calcul.
//   - RG-03 : −6 dB par doublement de la distance (source ponctuelle, 20·log10(d/d0)), coupé au-delà de la portée ;
//     passe-bas de 8 kHz à 1 m vers 2 kHz à la portée.
//   - RG-04 : panoramique à puissance constante (gauche / droite selon l'angle par rapport à l'axe de la voiture) ;
//     source de dos : −3 dB et aigus coupés au-dessus de 4 kHz.
//   - RG-05 : tunnel : réflexions précoces des parois (délai = largeur / 343 m/s) et queue de réverbération (TR60 2,2 s)
//     sur le moteur, le vent et le trafic ; fondu d'entrée et de sortie de 0,4 s ; réverbération ≥ 6 dB sous le son direct.
//   - RG-06 : la nuit, moitié du volume (comme l'ambiance).
//   - RG-07 : thread audio sans allocation ni verrou : tampons calculés une fois (thread du jeu), cibles atomiques.
// Repère des sources : « repère de la route » (segment du tour, avancée, latéral, hauteur), comme le décor : la route ne
// reboucle jamais en 3D (track.h), chaque tour est une copie déplacée du premier ; la source est placée sur la copie du tour
// le plus proche du joueur.
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"
#include "sfx_synth.h"

namespace wfs {
class Track;

namespace lieux {

enum Type : uint8_t { FONTAINE, VAGUES, TRIBUNE, FERME, TERRASSE, NB_TYPES };
const char* nomType(int t);

// RG-03 : portée (m) imposée par la règle ; d0 : distance de référence (niveau plein en deçà, plus proche approche possible
// de la voiture) ; volume : niveau efficace (RMS) du son à d0, part du master (boucles) ou crête (appels de la ferme)
struct Fiche { float portee, d0, volume; };
inline constexpr Fiche FICHES[NB_TYPES] = {
    {40.f, 8.f, 0.045f},     // fontaine : îlot de 8 m (giratoire.h, R_ILOT) ; au passage dans l'anneau (14 m) ≈ 0,026
    {120.f, 15.f, 0.09f},    // vagues : rivage à ≈ 53 m de l'axe sur la plage (RW + 46) ; au passage ≈ 0,025
    {300.f, 15.f, 0.08f},    // tribunes : 6 rangs à RW + 14 m ; au passage (≈ 22 m) ≈ 0,055
    {80.f, 10.f, 0.25f},     // ferme : troupeau à ≈ 30 m de l'axe ; crête d'un appel au passage ≈ 0,08
    {50.f, 4.f, 0.06f},      // terrasses : tables à RW + 4,25 m ; au passage (≈ 12 m) ≈ 0,02
};
constexpr int ESPECES = 3;   // FERME : variante 0 vache, 1 cheval, 2 mouton (ordre de campagne.cpp, WFS-186)

// une source dans le repère de la route : segment du tour [0, longueur), avancée (m, vers l'avant), latéral (m, + à droite),
// hauteur au-dessus de la route (m) ; variante : espèce des animaux de la ferme
struct Lieu { int64_t seg = 0; float avant = 0, lat = 0, dy = 0; uint8_t variante = 0; };

// ---------- registre (thread principal) : chaque carte déclare ses sources à son init, le son les lit à chaque pas ----------
// remplace toutes les sources (carte, type) : une partie suivante sur la même carte ne double rien
void poser(const char* carte, Type t, std::vector<Lieu> l);
const std::vector<Lieu>& lire(const std::string& carte, Type t);
// point p du monde (tour 0) exprimé dans le repère du segment seg (accroche des cartes qui calculent des positions du monde)
Lieu depuisMonde(Track& t, int64_t seg, vec3 p);
// position dans le monde de la copie de la source la plus proche du segment absolu aP (tour du joueur)
vec3 positionMonde(Track& t, const Lieu& l, int64_t aP);
// RG-02 (terrasses) : clients attablés (abscisse en segments, latéral) regroupés par terrasse (même côté, < 6 m d'écart)
struct Attable { double s; float lat; };
std::vector<Lieu> regrouperTerrasses(const std::vector<Attable>& clients, int64_t longueur);

// ---------- calculs purs (testés par tests/test_audio_lieux.cpp) ----------
// RG-03 : gain (amplitude) à la distance d : 1 jusqu'à d0, puis d0 / d (−6 dB par doublement), fondu linéaire sur les
// 10 derniers % de la portée, 0 au-delà
float gainDistance(float d, float d0, float portee);
// RG-03 : coupure du passe-bas (Hz) : 8 kHz à 1 m, 2 kHz à la portée, géométrique en log(d) entre les deux
float coupureDistance(float d, float portee);
constexpr float COUPURE_PROCHE = 8000.f, COUPURE_PORTEE = 2000.f, COUPURE_DOS = 4000.f, DOS_DB = -3.f, PAN_LARGEUR = 0.8f;
// RG-03 et RG-04 : gains gauche / droite et coupure d'une source à la distance d et à l'azimut az (rad : 0 devant, +π/2 à
// droite, ±π derrière) ; panoramique à puissance constante (gG² + gD² = g²)
struct Gains { float g = 0, gG = 0, gD = 0, coupure = COUPURE_PROCHE; };
Gains spatialiser(float d, float az, const Fiche& f);
// coefficient du passe-bas à un pôle (y += a · (x − y)) pour la coupure fc
float coefPasseBas(float fc, float rate);
// RG-01 : mélange final, n trames : canaux = 2 → gauche = mono + G, droite = mono + D (le son existant recopié à l'égal dans
// les deux canaux) ; canaux = 1 (iOS, gel ; banc) → mono + (G + D) · √½ ; écrêté à ±1
void entrelacer(const float* mono, const float* G, const float* D, float* out, int n, int canaux);

// ---------- RG-05 : réverbération d'un tunnel ----------
constexpr float VITESSE_SON = 343.f;                 // m/s, air à 20 °C
constexpr float LARGEUR_TUNNEL = 2 * (7.5f + 4.f);   // m : piédroits à RW + 4 m de l'axe (ville.cpp, plage.cpp TUNNEL_W)
constexpr float TR60 = 2.2f;                          // s : béton, tube routier (règle : 1,5 à 3 s)
constexpr float FONDU_TUNNEL = 0.4f;                  // s : entrée et sortie (règle : ≥ 0,3 s)
class Tunnel {
public:
    void build(float rate, float largeur = LARGEUR_TUNNEL, float tr60 = TR60);   // thread du jeu (allocation)
    void dedans(bool oui) { cible_ = oui; }                                       // thread du jeu
    bool dedansCible() const { return cible_.load(); }
    // thread audio : ajoute la réverbération de `in` (mono : moteur, vent, trafic) dans G et D
    void process(const float* in, float* G, float* D, int n);
    float delai() const { return delai_; }   // s : première réflexion
    float niveau() const { return mix_; }    // fondu en cours (0 dehors, 1 dedans)
    float gainHumide = 0.2f;                  // étalonné : réverbération ≈ −10 dB sous le son direct (test RG-05 : ≥ 6 dB)
private:
    struct Ligne { std::vector<float> b; size_t i = 0; };
    struct Peigne { Ligne l; float fb = 0, amort = 0.25f, filtre = 0; };
    struct PasseTout { Ligne l; float g = 0.5f; };
    Ligne avant_;                  // pré-délai : réflexions précoces (1 et 2 aller-retour d'une paroi à l'autre)
    Peigne peignes_[2][4];         // [gauche, droite][4] : délais décorrélés
    PasseTout passeTout_[2][2];
    size_t r1_ = 0, r2_ = 0;       // réflexions (échantillons)
    float rate_ = 48000, delai_ = 0, mix_ = 0, pasFondu_ = 0;
    bool vide_ = true;             // tampons à zéro (aucune queue fantôme à la prochaine entrée)
    std::atomic<bool> cible_{false}, pret_{false};
};

// ---------- sources placées ----------
class SonsDesLieux {
public:
    // thread du jeu : tampons des 5 sources (synthèse, allocation) ; nuit : moitié du volume (RG-06)
    void build(float rate, bool nuit);
    void active(bool on) { actif_ = on; }
    // thread du jeu, à chaque pas : cibles d'une source (g = 0 : muette) ; variante : espèce (ferme)
    void cible(int type, const Gains& g, int variante = 0);
    // thread audio : ajoute les sources dans G et D (n trames)
    void mix(float* G, float* D, int n, float master);
    // banc d'essai : premier appel de la ferme avancé à `secondes`
    void premierAppel(float secondes) { attenteAppel_ = (int64_t)(secondes * rate_); }
    const std::vector<float>* boucle(int type) const { return voix_[type].boucle; }   // nullptr pour la ferme
    const std::vector<float>* appel(int espece) const { return appels_[espece]; }
private:
    struct Voix {
        const std::vector<float>* boucle = nullptr;   // tampon partagé (calculé une fois) ; nullptr : ferme (appels seulement)
        sfx::LecteurAleatoire lecteur;
        std::atomic<float> cG{0}, cD{0}, cFc{COUPURE_PROCHE};
        float gG = 0, gD = 0, lp = 0;         // thread audio : gains en cours, état du passe-bas
    };
    Voix voix_[NB_TYPES];
    const std::vector<float>* appels_[ESPECES] = {};
    const std::vector<float>* fondu_ = nullptr;
    std::atomic<int> espece_{0};
    const std::vector<float>* appel_ = nullptr; size_t appelPos_ = 0;
    int64_t attenteAppel_ = 0;
    uint32_t hasard_ = 0x6C8E9CF5u;
    float rate_ = 48000, gainNuit_ = 1;
    std::atomic<bool> actif_{false}, pret_{false};
};

// banc d'essai PC (RG-08) : lieu-<type>.wav (10 s, stéréo, passage à 50 km/h devant la source) et lieu-<type>.png
// (niveaux gauche / droite) ; lieu-tunnel.wav / .png (passage d'un tunnel, son du moteur `moteur` en entrée) ; coût
void benchLieux(const std::string& out, const std::vector<float>& moteur);

} // namespace lieux
} // namespace wfs
