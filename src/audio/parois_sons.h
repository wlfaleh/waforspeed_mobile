// WaForSpeed — acoustique des lieux (WFS-275, épic E6) : écho des façades, des murs et de la falaise ; sons masqués derrière les
// immeubles de la Ville. Complète WFS-261 (ambiance_lieux.*) et WFS-272 (voitures_sons.*) sans rien leur retirer :
//   - RG-02 : parois lues dans les cartes (registre `parois::declarer`, accroches de lecture de plage.cpp, campagne.cpp, circuit.cpp ;
//     Ville : façades des immeubles et entrepôts du plan, lues ici) ; une paroi par côté et par segment (la plus proche), ≤ 40 m ;
//   - RG-03 : écho par sources images (géométrie) : délai 2·d / c, niveau 20·lg(r0 / chemin), perte à la réflexion des parois dures ;
//     rue bordée des deux côtés : ordres 2 et 3 ; aucune queue de réverbération (non sourcée) ;
//   - RG-04 : l'écho de la paroi de gauche vient de gauche (panoramique de WFS-261 à ±90°) ;
//   - RG-05 : trouées en fondu de 75 ms ; délai qui change : fondu entre deux prises (60 ms), sans glissement de hauteur ;
//     tunnel : seul l'écho du tunnel de WFS-261 (fondu croisé de 0,4 s) ;
//   - RG-06 : source cachée derrière un immeuble de la Ville : diffraction D = 10·lg(3 + 20·N), N = 2·δ / λ (1 kHz), ≤ 20 dB ;
//   - RG-07 : sans paroi ni masque, le son sort exactement comme avant ;
//   - RG-08 : thread audio sans allocation ni verrou (ligne à retard allouée à build, cibles atomiques).
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"
#include "ambiance_lieux.h"

namespace wfs {
class Track;
namespace ville { struct Plan; }

namespace parois {

constexpr float PORTEE = 40.f;               // m : paroi plus loin ignorée (écho ≤ −34 dB, RG-02)
constexpr float R0 = 1.5f;                   // m : moteur → oreille du conducteur (son direct de référence, RG-03)
// WFS-278 (RG-01) : coefficients d'absorption à 1 kHz, relus dans la base de matériaux de pyroomacoustics (annexe de M. Vorländer,
// « Auralization », Springer 2008 ; references/reel/son/absorption-parois.md). Amplitude réfléchie √(1 − α) : −0,13 dB à 0,03,
// −0,18 dB à 0,04 (règle : ≤ 0,3 dB). WFS-275 avait 0,05 (valeur non sourcée).
constexpr float ALPHA_DUR = 0.03f;      // « hard surface average » (murs de brique, enduit, sols durs) : façades enduites, garages
constexpr float ALPHA_BETON = 0.03f;    // « rough concrete » : mur des stands, tribunes
constexpr float ALPHA_VERRE = 0.03f;    // « glass window » : vitrages, mur-rideau du centre commercial
constexpr float ALPHA_PIERRE = 0.04f;   // « limestone walls » : roche calcaire de la Corniche, casino, église
constexpr float ALPHA = ALPHA_DUR;      // par défaut
constexpr float FONDU_TROUEE = 0.075f;       // s : un côté qui s'ouvre ou se ferme (règle : 50 à 100 ms)
constexpr float FONDU_PRISE = 0.06f;         // s : fondu entre l'ancien et le nouveau délai
constexpr float LAMBDA_1K = lieux::VITESSE_SON / 1000.f;   // m : longueur d'onde à 1 kHz (0,343 m)
constexpr float MASQUE_MAX = 20.f;           // dB : plafond de la diffraction (RG-06)
constexpr float MASQUE_TAU = 0.05f;          // s : entrée et sortie du masque (90 % en 0,115 s ; règle : ≥ 0,1 s)
constexpr float MASQUE_COUPURE = 1000.f;     // Hz : son plus sourd derrière un immeuble

// ---------- registre (thread principal) ----------
struct Paroi { float d = 0, h = 0, alpha = ALPHA; };   // distance de la paroi à l'axe (m), hauteur (m), absorption ; d = 0 : aucune
using Cotes = std::array<Paroi, 2>;          // [0] gauche, [1] droite
void effacer(const char* carte);
// paroi du côté cote (−1 gauche, +1 droite) à d m de l'axe, haute de h m, longue de longueurM m, centrée sur le segment centre
// (tour de longueurTour segments) ; la plus proche de l'axe gagne. WFS-278 : profondeur > 0 → c'est aussi un obstacle pour les
// masques (boîte de la façade à d + profondeur, haute de h) ; alpha : absorption du matériau (RG-01)
void declarer(const char* carte, int64_t longueurTour, int64_t centre, int cote, float d, float h, float longueurM,
              float profondeur = 0, float alpha = ALPHA);
const std::vector<Cotes>& lire(const std::string& carte);
// Ville : façades des immeubles (RW + 6,5 m, SET_BACK de ville.cpp) et des entrepôts du plan (lecture seule) ; WFS-278 : et les
// lieux du tracé 2 (casino, centre commercial, garages de WFS-176), aux distances et hauteurs de leur dessin (ville.cpp)
void depuisVille(const ville::Plan& p, int64_t longueurTour);

// ---------- RG-03 : réflexions (calcul pur) ----------
struct Reflexion { float delai = 0, gain = 0; int cote = 0; int ordre = 1; };   // s ; amplitude / son direct ; −1 gauche, +1 droite
// a, b : distances de la tête du conducteur aux parois de gauche et de droite (m, 0 : absente) ; rend le nombre de réflexions (≤ 6)
int reflexions(float a, float b, Reflexion out[6], float alphaG = ALPHA, float alphaD = ALPHA);

// ---------- RG-05 : suivi d'un pas à l'autre (thread du jeu) ----------
// a, b : distances de la tête aux parois (m, 0 : aucune) ; tunnel : la voiture est dans un tunnel (seul l'écho de WFS-261) ;
// rend les réflexions du pas et leurs gains gauche / droite (présence des côtés en fondu de FONDU_TROUEE, tunnel en fondu croisé)
class Suivi {
public:
    int pas(float dt, float a, float b, bool tunnel, Reflexion r[6], float gG[6], float gD[6], float alphaG = ALPHA, float alphaD = ALPHA);
    void reset() { *this = Suivi{}; }
    float presenceG() const { return pG_; }
    float presenceD() const { return pD_; }
private:
    float pG_ = 0, pD_ = 0, aG_ = 10, aD_ = 10, fT_ = 1, alG_ = ALPHA, alD_ = ALPHA;
};

// ---------- écho (thread audio) ----------
constexpr int PRISES = 6;
class Echos {
public:
    void build(float rate);                                    // thread du jeu : ligne à retard de 0,5 s
    void cible(int prise, float delaiS, float gG, float gD);   // thread du jeu
    // thread audio : ajoute dans G et D l'écho du bus (moteur, vent, bruitages, voitures)
    void process(const float* bus, float* G, float* D, int n);
private:
    struct Prise {
        std::atomic<float> cDelai{0}, cG{0}, cD{0};
        float tete = 0, suivante = 0, xf = -1, gG = 0, gD = 0;   // xf < 0 : aucun fondu entre prises en cours
    };
    Prise p_[PRISES];
    std::vector<float> ligne_;
    size_t ecr_ = 0;
    float rate_ = 48000, pasXf_ = 0.001f;
    std::atomic<bool> pret_{false};
    float lire(float delai) const;
};

// ---------- RG-06 : sons masqués (calcul pur) ----------
float diffractionDb(float detour);   // 10·lg(3 + 20·N), N = 2·δ / λ (λ à 1 kHz), plafonné à MASQUE_MAX ; 0 si δ < 0
struct Immeuble { lieux::Lieu centre; float demiProf = 0, demiLong = 0, h = 0; };   // centre : repère de la route ; profondeur ⟂ route
std::vector<Immeuble> immeublesVille(const ville::Plan& p, int64_t longueurTour);   // WFS-278 : lieux de WFS-176 compris
// WFS-278 (RG-03) : obstacles déclarés avec les parois (profondeur > 0) d'une carte ; les morceaux voisins d'un même mur sont réunis
const std::vector<Immeuble>& obstacles(const std::string& carte);
// détour le plus court (m) de a vers b autour (côtés) ou par-dessus (toit) d'une boîte de centre c (sol), d'axes r (profondeur)
// et f (longueur), haute de h ; −1 si le segment a–b ne la traverse pas
float detour(vec3 a, vec3 b, vec3 c, vec3 r, vec3 f, float demiProf, float demiLong, float h);
// gains et coupure d'une source masquée : masque lissé (etat → cible, constante MASQUE_TAU, pas dt), gain × 10^(−m/20), coupure
// abaissée vers MASQUE_COUPURE ; masque nul : rien ne change (RG-07)
void appliquerMasque(float& gG, float& gD, float& coupure, float& etat, float cible, float dt);
// masque (dB) de la source s entendue en a, d'après les immeubles proches. Un seul obstacle sur la ligne : diffraction simple
// (plafond 20 dB). WFS-278 (RG-04) : plusieurs : diffraction multiple d'ISO 9613-2 (éq. 14, terme C3) : chemin tendu par-dessus
// ou autour de tous les obstacles, D = 10·lg(3 + 20·N·C3), C3 = (1 + (5λ/e)²) / (1/3 + (5λ/e)²), e : distance entre la première et
// la dernière arête, plafond 25 dB (source : implémentation de référence CNOSSOS-EU, references/reel/son/absorption-parois.md)
// Obstacles accolés (à moins de 1 m : morceaux d'une même roche, garages côte à côte) : un seul groupe, dont le chemin « autour » fait le
// tour, traité comme plusieurs arêtes (éq. 14). Un obstacle seul et isolé garde la diffraction simple de WFS-275.
struct ImmeubleMonde { vec3 c, r, f; float demiProf, demiLong, h; };
float masqueDb(vec3 a, vec3 s, const std::vector<ImmeubleMonde>& proches);
constexpr float MASQUE_MAX_MULTIPLE = 25.f;
// RG-04 : D de plusieurs arêtes, détour z (m) et distance e (m) entre la première et la dernière arête (e = 0 : une seule)
float diffractionMultipleDb(float z, float e);

// banc d'essai PC (RG-09) : parois-ville, parois-falaise, parois-stands, masque-ville (.wav stéréo + .png) ; coût
void benchParois(const std::string& out, const std::vector<float>& moteur);

} // namespace parois
} // namespace wfs
