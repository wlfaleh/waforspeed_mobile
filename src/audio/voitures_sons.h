// WaForSpeed — son des autres voitures (WFS-272, épic E6) : adversaires de la Course à 5 et du Duel, police de la Poursuite et
// trafic entendus À LEUR PLACE, comme les lieux de WFS-261 (ambiance_lieux.*, dont les lois sont réutilisées) :
//   - RG-01 : au plus VOIX voix ; les plus proches gagnent ; une voiture garde sa voix ; entrée et sortie en fondu de FONDU s ;
//     portée 120 m (police : 200 m) ; rien de retiré (rumeur, passage, klaxon, sirène de WFS-121 / WFS-131) ;
//   - RG-02 : moteur (fréquence de combustion d'un 4 temps : régime / 60 × cylindres / 2, régime tiré des rapports de la fiche)
//     et pneus, niveaux selon la vitesse d'après CNOSSOS-EU ; une voiture du trafic à 50 km/h à 7,5 m reste sous le moteur du
//     joueur en pleine accélération ;
//   - RG-03 / RG-04 : distance, passe-bas et gauche / droite de WFS-261 (lieux::spatialiser) ;
//   - RG-05 : effet Doppler physique f' = f · (c + v_auditeur) / (c − v_source), c = 343 m/s, lissé ;
//   - RG-06 : passage et klaxon du trafic joués ici avec le gauche / droite de leur voiture (même tampon, même gain) ; la
//     sirène reçoit le gauche / droite de la police la plus proche (accroche de sfx.*) ; au centre : exactement comme avant ;
//   - RG-07 : les voix passent dans l'écho des tunnels (bus mono, avec le moteur et le trafic) ;
//   - RG-08 : thread audio sans allocation ni verrou (tables calculées une fois, cibles atomiques, file sans verrou).
// Thread du jeu : Attribution (choix des voix) et calcul des cibles ; thread audio : SonsVoitures::mix.
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"
#include "ambiance_lieux.h"

namespace wfs {
struct CarDef;
namespace course5 { struct Ordinateur; }
namespace poursuite { struct Police; }
// WFS-272 (accroches de lecture) : voitures des modes de jeu, lecture seule ; nullptr hors du mode
const std::vector<course5::Ordinateur>* course5Ordinateurs();                       // course5_system.cpp
const std::vector<poursuite::Police>* poursuiteVoitures(const char** voiture);     // poursuite_system.cpp (voiture repeinte)

namespace voitures {

constexpr int VOIX = 8;
constexpr float FONDU = 0.25f;                     // s : entrée et sortie d'une voix (règle : ≥ 0,2 s)
constexpr float PORTEE = 120.f, PORTEE_POLICE = 200.f;
constexpr float D_PLEIN = 3.75f, D_REF = 7.5f;     // m : niveau plein en deçà (voiture collée) ; distance de référence (R51)
constexpr float VITESSE_SON = 343.f;               // m/s, air à 20 °C
constexpr int CYLINDRES = 4;                       // la fiche (cars.h) ne donne pas le nombre de cylindres : 4 par défaut
// RG-02 : niveau (RMS, master 1) d'une voiture du trafic à 50 km/h à 7,5 m : le moteur du joueur en pleine accélération
// mesure 0,072 à 0,082 (5 voix de moteur, master 0,5 ; journal WFS-272) : la voiture du trafic est 6 dB plus bas
constexpr float NIVEAU_7_5 = 0.072f;
constexpr float PLAFOND = 2 * NIVEAU_7_5;          // plafond doux : une voiture voisine ne dépasse jamais le moteur du joueur
constexpr float CHARGE_DB = 6.f;                   // moteur en charge (adversaires, police) : propulsion + 6 dB (estimation)
constexpr float LISSAGE_DOPPLER = 0.012f;          // s : lissage du facteur Doppler (≈ un pas) ; le mélangeur glisse sur chaque bloc

// ---------- RG-02 : CNOSSOS-EU (directive (UE) 2015/996, annexe, appendice F, tableau F-1, catégorie 1 : voitures) ----------
// puissance acoustique par bande d'octave (63 Hz à 8 kHz) : roulement L_R = A_R + B_R · lg(v / 70) ; propulsion
// L_P = A_P + B_P · (v − 70) / 70 (v en km/h) ; pondération A ajoutée pour l'oreille
inline constexpr float A_R[8] = {79.7f, 85.7f, 84.5f, 90.2f, 97.3f, 93.9f, 84.1f, 74.3f};
inline constexpr float B_R[8] = {30.0f, 41.5f, 38.9f, 25.7f, 32.5f, 37.2f, 39.0f, 40.0f};
inline constexpr float A_P[8] = {94.5f, 89.2f, 88.0f, 85.9f, 84.2f, 86.9f, 83.3f, 76.1f};
inline constexpr float B_P[8] = {-1.3f, 7.2f, 7.7f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f};
inline constexpr float POND_A[8] = {-26.2f, -16.1f, -8.6f, -3.2f, 0.f, 1.2f, 1.0f, -1.1f};
float roulementDbA(float kmh);     // L_WA du roulement (pneus), v ≥ 20 km/h (domaine de la méthode), dB(A)
float propulsionDbA(float kmh);    // L_WA de la propulsion (moteur), dB(A)
// amplitudes relatives (1 = niveau total d'une voiture en croisière à 50 km/h) du moteur et des pneus
struct Parts { float moteur = 0, pneus = 0; };
Parts parts(float kmh, bool charge, bool electrique);

// ---------- moteur ----------
// RG-02 : fréquence de combustion (Hz) d'un moteur 4 temps
inline float frequenceCombustion(float trMin, int cylindres = CYLINDRES) { return trMin / 60.f * (float)cylindres / 2.f; }
// régime (tr/min) à la vitesse v (m/s) d'après les rapports de la fiche (physique::params) ; rapport : tenu d'un appel à
// l'autre (hystérésis) ; charge : adversaire ou police (haut dans les tours), sinon conduite tranquille du trafic
float regime(const CarDef& car, float vMs, int& rapport, bool charge);
bool electrique(const CarDef& car);   // même règle que le son du moteur du joueur (model3)

// ---------- RG-05 ----------
// facteur Doppler de la source (position ps, vitesse vs, m/s) entendue par l'auditeur (pl, vl) ; borné à [0,5 ; 2]
float doppler(vec3 ps, vec3 vs, vec3 pl, vec3 vl);
// RG-06 : gauche / droite d'un son existant (sirène, passage, klaxon) : loi de WFS-261 (puissance constante, largeur 0,8),
// normalisée pour que la source droit devant (ou droit derrière) garde exactement son niveau d'avant dans chaque canal
struct Pan { float g = 1, d = 1; };
Pan panExistant(float az);

// ---------- RG-01 : choix des voix (thread du jeu) ----------
enum Genre : uint8_t { TRAFIC, ADVERSAIRE, POLICE };
struct Candidat { uint64_t id = 0; vec3 p{}, v{}; float kmh = 0; const CarDef* car = nullptr; uint8_t genre = TRAFIC; };
struct EtatVoix {
    uint64_t id = 0; bool occupee = false; int sens = 0;   // +1 : entrée / tenue ; −1 : sortie
    float fondu = 0;                                      // 0..1
    Candidat c;                                           // dernière vue de la voiture
    int rapport = 1; float dop = 1;
    bool neuve = true;                                    // pas encore de Doppler lissé
};
class Attribution {
public:
    // candidats du pas (toutes les voitures des modes et du trafic) ; ecoute : tête du conducteur ; dt en s
    void update(float dt, const std::vector<Candidat>& c, vec3 ecoute);
    const EtatVoix& voix(int i) const { return v_[i]; }
    EtatVoix& voix(int i) { return v_[i]; }
    void reset() { for (auto& x : v_) x = EtatVoix{}; }
    int actives() const { int n = 0; for (const auto& x : v_) n += x.occupee; return n; }
private:
    EtatVoix v_[VOIX];
    std::vector<int> ordre_;      // tampon réutilisé (thread du jeu)
    std::vector<float> d2_;
};

// cibles d'une voix (thread du jeu → thread audio)
struct Cible {
    float gG = 0, gD = 0;          // gains gauche / droite (distance, côté, fondu, niveau)
    float moteur = 0, pneus = 0;   // parts (somme quadratique ≤ 1 avant les gains)
    float fCycle = 30;             // Hz : un cycle moteur (2 tours) ; la combustion est à CYLINDRES × fCycle
    float doppler = 1, coupure = lieux::COUPURE_PROCHE;
    uint8_t timbre = 0;            // 0 : trafic (tranquille), 1 : en charge, 2 : électrique
};
// toutes les cibles d'une voix à partir de son état (dist, côté, Doppler lissé, régime) ; ecoute, vEcoute : tête et vitesse
// du joueur ; avant, droite : axes de sa voiture
Cible calculerCible(EtatVoix& e, float dt, vec3 ecoute, vec3 vEcoute, vec3 avant, vec3 droite);

// ---------- mélange (thread audio) ----------
class SonsVoitures {
public:
    void build(float rate);                         // thread du jeu (tables, une fois par fréquence)
    void cible(int voix, const Cible& c);           // thread du jeu
    // RG-06 : son existant (tampon de SfxMixer, déjà calculé) joué une fois avec son gain et son gauche / droite
    void jouer(const std::vector<float>* tampon, float gain, Pan pan);
    // thread audio : ajoute les voix et les sons placés dans G et D, et leur part mono dans bus (écho du tunnel) ;
    // actif = le jeu donne des nouvelles (sinon fondu vers le silence en 30 ms, comme SfxMixer)
    void mix(float* G, float* D, float* bus, int n, float master, bool actif);
    float frequenceJouee(int voix) const { return fJouee_[voix]; }   // banc : Hz de la combustion entendue
private:
    struct Voix {
        std::atomic<float> cG{0}, cD{0}, cMoteur{0}, cPneus{0}, cF{30}, cDop{1}, cFc{lieux::COUPURE_PROCHE};
        std::atomic<int> cTimbre{0};
        float gG = 0, gD = 0, aM = 0, aP = 0, inc = 0, incPneu = 1, lp = 0;
        double phase = 0, posPneu = 0;
    };
    Voix voix_[VOIX];
    struct Demande { const std::vector<float>* buf; float gain, g, d; };
    static constexpr int FILE = 16, PLACES = 6;
    Demande file_[FILE];
    std::atomic<uint32_t> tete_{0}, queue_{0};
    struct Place { const std::vector<float>* buf = nullptr; size_t pos = 0; float gain = 0, g = 1, d = 1; };
    Place places_[PLACES];
    const std::vector<float>* tables_[3] = {};   // tables d'un cycle moteur : trafic, en charge, électrique (RMS 1)
    const std::vector<float>* pneus_ = nullptr;  // boucle de bruit de roulement (RMS 1)
    float rate_ = 48000, fade_ = 0, kFade_ = 0.001f;
    float fJouee_[VOIX] = {};
    std::atomic<bool> pret_{false};
};

// banc d'essai PC (RG-09) : voitures-course5, voitures-trafic, voitures-police (.wav stéréo + .png : gauche, droite, hauteur)
void benchVoitures(const std::string& out);

} // namespace voitures
} // namespace wfs
