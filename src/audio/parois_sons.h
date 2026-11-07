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
// coefficient d'absorption des parois dures (béton, brique, pierre, verre, roche) à 1 kHz : 0,02 à 0,05 dans les tables usuelles
// d'acoustique du bâtiment [à vérifier : aucune source lisible avec le réseau du projet] ; 0,05 retenu (le plus absorbant) :
// amplitude réfléchie √(1 − α) = 0,975, soit −0,22 dB par réflexion (règle : ≤ 0,3 dB)
constexpr float ALPHA = 0.05f;
constexpr float FONDU_TROUEE = 0.075f;       // s : un côté qui s'ouvre ou se ferme (règle : 50 à 100 ms)
constexpr float FONDU_PRISE = 0.06f;         // s : fondu entre l'ancien et le nouveau délai
constexpr float LAMBDA_1K = lieux::VITESSE_SON / 1000.f;   // m : longueur d'onde à 1 kHz (0,343 m)
constexpr float MASQUE_MAX = 20.f;           // dB : plafond de la diffraction (RG-06)
constexpr float MASQUE_TAU = 0.05f;          // s : entrée et sortie du masque (90 % en 0,115 s ; règle : ≥ 0,1 s)
constexpr float MASQUE_COUPURE = 1000.f;     // Hz : son plus sourd derrière un immeuble

// ---------- registre (thread principal) ----------
struct Paroi { float d = 0, h = 0; };        // distance de la paroi à l'axe de la route (m) et hauteur (m) ; d = 0 : aucune
using Cotes = std::array<Paroi, 2>;          // [0] gauche, [1] droite
void effacer(const char* carte);
// paroi du côté cote (−1 gauche, +1 droite) à d m de l'axe, haute de h m, longue de longueurM m, centrée sur le segment centre
// (tour de longueurTour segments) ; la plus proche de l'axe gagne
void declarer(const char* carte, int64_t longueurTour, int64_t centre, int cote, float d, float h, float longueurM);
const std::vector<Cotes>& lire(const std::string& carte);
// Ville : façades des immeubles (RW + 6,5 m, SET_BACK de ville.cpp) et des entrepôts du plan (lecture seule)
void depuisVille(const ville::Plan& p, int64_t longueurTour);

// ---------- RG-03 : réflexions (calcul pur) ----------
struct Reflexion { float delai = 0, gain = 0; int cote = 0; int ordre = 1; };   // s ; amplitude / son direct ; −1 gauche, +1 droite
// a, b : distances de la tête du conducteur aux parois de gauche et de droite (m, 0 : absente) ; rend le nombre de réflexions (≤ 6)
int reflexions(float a, float b, Reflexion out[6]);

// ---------- RG-05 : suivi d'un pas à l'autre (thread du jeu) ----------
// a, b : distances de la tête aux parois (m, 0 : aucune) ; tunnel : la voiture est dans un tunnel (seul l'écho de WFS-261) ;
// rend les réflexions du pas et leurs gains gauche / droite (présence des côtés en fondu de FONDU_TROUEE, tunnel en fondu croisé)
class Suivi {
public:
    int pas(float dt, float a, float b, bool tunnel, Reflexion r[6], float gG[6], float gD[6]);
    void reset() { *this = Suivi{}; }
    float presenceG() const { return pG_; }
    float presenceD() const { return pD_; }
private:
    float pG_ = 0, pD_ = 0, aG_ = 10, aD_ = 10, fT_ = 1;
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
std::vector<Immeuble> immeublesVille(const ville::Plan& p, int64_t longueurTour);
// détour le plus court (m) de a vers b autour (côtés) ou par-dessus (toit) d'une boîte de centre c (sol), d'axes r (profondeur)
// et f (longueur), haute de h ; −1 si le segment a–b ne la traverse pas
float detour(vec3 a, vec3 b, vec3 c, vec3 r, vec3 f, float demiProf, float demiLong, float h);
// gains et coupure d'une source masquée : masque lissé (etat → cible, constante MASQUE_TAU, pas dt), gain × 10^(−m/20), coupure
// abaissée vers MASQUE_COUPURE ; masque nul : rien ne change (RG-07)
void appliquerMasque(float& gG, float& gD, float& coupure, float& etat, float cible, float dt);
// masque (dB) de la source s entendue en a, d'après les immeubles proches (le plus fort l'emporte)
struct ImmeubleMonde { vec3 c, r, f; float demiProf, demiLong, h; };
float masqueDb(vec3 a, vec3 s, const std::vector<ImmeubleMonde>& proches);

// banc d'essai PC (RG-09) : parois-ville, parois-falaise, parois-stands, masque-ville (.wav stéréo + .png) ; coût
void benchParois(const std::string& out, const std::vector<float>& moteur);

} // namespace parois
} // namespace wfs
