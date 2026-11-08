// WaForSpeed — son de la pluie (WFS-288, épic E6) : la pluie de la Ville de nuit (meteo::pluieActive) s'entend, sans rien retirer de
// WFS-174 (chuintement des pneus), WFS-261, WFS-272, WFS-275 ni WFS-278 :
//   - RG-01 : pluie de référence sourcée (Marshall et Palmer 1948, Atlas et coll. 1973 ; references/reel/son/pluie.md) : nombre
//     d'impacts par m² et par seconde, part des grosses gouttes, flux sur le pare-brise selon la vitesse ;
//   - RG-02 : écran Course seulement (comme l'ambiance), fondus 1 s / 0,4 s ; « Son : Coupé » : silence (audio_system) ;
//   - RG-03 : vue extérieure : averse stéréo décorrélée (bruit calculé en direct : aucun motif qui revient), gouttes isolées,
//     clapotis des flaques (bulles de Minnaert) ;
//   - RG-04 : vue Cockpit : averse étouffée par la carrosserie, tambourinage du toit et du pare-brise (impacts tirés au hasard à la
//     cadence de RG-01, plus nombreux sur le pare-brise quand on roule) ;
//   - RG-05 : essuie-glaces (vue Cockpit) : même modèle que l'image (cockpit::EtatEssuie, mêmes entrées que camera_vue.cpp) ;
//   - RG-06 : tunnel : pluie coupée (0,3 s) puis reprise (0,5 s), essuie-glaces continus ;
//   - RG-09 : thread audio sans allocation ni verrou (tables de build, cibles atomiques) ; aucun fichier son.
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "../render/vehicule_cockpit.h"   // cockpit::EtatEssuie, cadenceEssuie (lecture seule)

namespace wfs::pluie {

// pente dy / dz du pare-brise de la Golf, vue de l'œil : du pied (−0,22 ; −1,10) au haut (+0,16 ; −0,52), soit 33,2° sur l'horizontale
// (géométrie lue dans vehicule_cockpit.h de WFS-235, constante retirée par WFS-282 ; gardée ici pour le flux du pare-brise)
constexpr float PENTE_PARE_BRISE = 0.655f;

// ---------- RG-01 : pluie de référence (calcul pur) ----------
constexpr float PLUIE_MMH = 5.f;                  // pluie modérée (décision du PO : 2,5 à 7,6 mm/h [à vérifier])
constexpr float MP_N0 = 8000.f;                   // m⁻³·mm⁻¹ (Marshall et Palmer 1948)
constexpr float D_MIN = 0.5f, D_GROSSE = 2.f, D_MAX = 8.f;   // mm : plus petite goutte comptée, « grosse » goutte, borne du calcul
constexpr float D_BULLE_MIN = 0.8f, D_BULLE_MAX = 1.1f;     // mm : gouttes qui laissent une bulle dans l'eau (Pumphrey et Crum 1989 [à vérifier])
// pente de la répartition des tailles (mm⁻¹) : Λ = 4,1 · R^−0,21 (Marshall et Palmer 1948)
float lambdaMP(float R);
// vitesse de chute (m/s) d'une goutte de D mm : v = 9,65 − 10,3 · e^(−0,6 · D) (Atlas et coll. 1973, ajustée sur Gunn et Kinzer 1949)
float vitesseChute(float D);
struct Reference {
    float lambda = 0;          // mm⁻¹
    float concentration = 0;   // gouttes de D ≥ 0,5 mm par m³
    float flux = 0;            // impacts par m² et par seconde sur une surface horizontale (D ≥ 0,5 mm)
    float fluxGrosses = 0;     // dont D ≥ 2 mm
    float partGrosses = 0;     // fluxGrosses / flux
    float fluxBulles = 0;      // dont 0,8 ≤ D ≤ 1,1 mm (bulles des flaques)
    float tauxMmh = 0;         // contrôle : pluie recalculée depuis la répartition (mm/h)
};
Reference reference(float R = PLUIE_MMH);         // intégration numérique (D de 0,5 à 8 mm)
// RG-04 : impacts par m² et par seconde sur un pare-brise de pente dy / dz (incliné de atan(pente) sur l'horizontale), voiture à
// kmh : concentration × vitesse relative (chute + vitesse de la voiture) projetée sur la normale = flux · cos α + V · sin α · concentration
float fluxPareBrise(const Reference& r, float kmh, float pente = PENTE_PARE_BRISE);
// surfaces de la voiture (m²), estimées depuis ses dimensions (WFS-158, L × W × H, empattement) : toit 0,60 × empattement de long
// et 0,70 × largeur de large ; pare-brise : 0,26 × hauteur de haut (Golf : du pied à −0,22 m au haut à +0,16 m de l'œil, ci-dessus)
// sur 0,80 × largeur, pente de la Golf [proportions de la Golf 8, à vérifier pour les autres]
struct Surfaces { float toit = 0, pareBrise = 0, pente = PENTE_PARE_BRISE; };
Surfaces surfaces(float L, float W, float H, float empattement);

// ---------- niveaux (RG-07 : décision du PO, niveau absolu non mesurable : le volume dépend du téléphone) ----------
// RMS par canal, avant `master` (0,5) ; vérifiés par test_audio_pluie : crête de la pluie ≥ 6 dB sous celle du moteur à mi-régime,
// chuintement des pneus de WFS-174 plus fort que l'averse dans sa bande (3-6 kHz) dès 50 km/h
constexpr float AVERSE_EXT = 0.006f;              // averse autour de la voiture (vue extérieure)
constexpr float ISOLATION_DB = -20.f;             // tôle et vitrage entre la pluie et l'oreille [à vérifier]
constexpr float COUPURE_HABITACLE = 1000.f;       // Hz : passe-bas de l'averse entendue de l'habitacle [à vérifier]
constexpr float GOUTTE = 0.04f;                   // crête d'une goutte isolée (vue extérieure)
constexpr float BULLE = 0.012f;                   // crête d'une bulle de flaque (petite part du mélange)
constexpr float TAMBOUR = 0.014f;                 // crête d'un impact de goutte de 2 mm à sa vitesse de chute (vue Cockpit)
constexpr float TAMBOUR_MAX = 4.f;                // impact le plus fort, en multiples d'une goutte de 2 mm (crête bornée)
constexpr float FROTTEMENT = 0.012f, CHOC = 0.06f, BOURDON = 0.004f;   // essuie-glaces (vue Cockpit) [à vérifier]
constexpr float A_ECOUTE = 1.f;                   // m² : sol autour du micro dont les grosses gouttes s'entendent une à une [à vérifier]
constexpr float A_FLAQUE = 0.1f;                  // m² : flaque proche dont les bulles s'entendent [à vérifier]
constexpr float FONDU_ENTREE = 1.f, FONDU_SORTIE = 0.4f;               // s : RG-02
constexpr float FONDU_TUNNEL_ENTREE = 0.3f, FONDU_TUNNEL_SORTIE = 0.5f; // s : RG-06
constexpr float FONDU_VUE = 0.15f;                // s : passage vue extérieure ↔ Cockpit

// ---------- son (thread du jeu → thread audio) ----------
class SonsPluie {
public:
    // thread du jeu : tables (tailles de gouttes tirées selon le flux), réglage des filtres ; aucune allocation ensuite
    void build(float rate);
    // thread du jeu, écran Course (comme l'ambiance de la carte, WFS-261 ; coupée en pause, dans les menus, scènes dédiées)
    void active(bool on) { actif_.store(on, std::memory_order_relaxed); }
    // thread du jeu, à chaque pas : modèle des essuie-glaces avancé avec les mêmes entrées que l'image (camera_vue.cpp) ;
    // pluie : meteo::pluieActive ; cockpit : vue Cockpit (essuie-glaces entendus si `balais`) ; tunnel : sous un tunnel
    void pas(float dt, double kmh, bool pluie, bool cockpit, bool balais, bool tunnel, const Surfaces& s);
    // thread audio : ajoute la pluie dans G et D (n trames) ; rien n'est touché tant qu'elle est muette
    void mix(float* G, float* D, int n, float master);
    // lecture (tests, banc)
    const cockpit::EtatEssuie& essuie() const { return essuie_; }
    float phaseAudio() const { return phaseA_; }           // phase des essuie-glaces du thread audio (fin du dernier bloc)
    float phaseDebutBloc() const { return phaseDebut_; }   // … au début du dernier bloc (après recalage sur le modèle)
    uint64_t impactsToit() const { return nToit_; }
    uint64_t impactsPareBrise() const { return nPb_; }
    uint64_t gouttes() const { return nGouttes_; }
    uint64_t bulles() const { return nBulles_; }
    uint64_t chocs() const { return nChocs_; }
    float presence() const { return gPresence_; }          // fondu de RG-02 (0..1)
    float horsTunnel() const { return gTunnel_; }          // fondu de RG-06 (1 dehors, 0 sous le tunnel)
    const Reference& ref() const { return ref_; }

private:
    struct Biquad {
        float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        float run(float x) { const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
        void raz() { x1 = x2 = y1 = y2 = 0; }
    };
    static Biquad passeBas(float fc, float q, float rate);
    static Biquad passeHaut(float fc, float q, float rate);
    static Biquad passeBande(float fc, float q, float rate);   // gain 1 au centre
    struct Grain { float t = 1e9f, duree = 0, gG = 0, gD = 0, amp = 0, f = 0, ph = 0, df = 0; };
    static constexpr int GRAINS = 4, TABLE = 256;
    uint32_t hasard();
    float aleatoire() { return (float)(hasard() >> 8) * (1.f / 16777216.f); }
    float bruit() { return aleatoire() * 2 - 1; }

    Reference ref_;
    cockpit::EtatEssuie essuie_;   // thread du jeu : même modèle que l'image
    std::array<float, TABLE> tailleChute_{}, tailleConcentration_{};   // quantiles (mm) : tirage selon le flux N·v ou la concentration N
    // cibles (thread du jeu → thread audio)
    std::atomic<bool> actif_{false}, pluie_{false}, cockpit_{false}, balais_{false}, tunnel_{false}, marche_{false}, pret_{false};
    std::atomic<float> pToit_{0}, pPb_{0}, partChute_{1}, vnPb_{0}, cosPb_{1}, phase_{0}, cadence_{0};
    std::atomic<uint32_t> seq_{0};
    // thread audio
    float rate_ = 48000;
    uint32_t etat_ = 0x9E3779B9u, seqVu_ = 0;
    Biquad extG_[2], extD_[2], intG_[2], intD_[2], goutteBp_, toitG_, toitD_, pbG_, pbD_, tambG_, tambD_, frot_;
    float kExt_ = 1, kInt_ = 1, kGoutte_ = 1, kFrot_ = 1;   // normalisations (RMS 1 du bruit filtré), mesurées dans build
    float kToit_ = 1, kPb_ = 1;                             // crête 1 de la réponse d'un impact (toit, pare-brise)
    float gPresence_ = 0, gTunnel_ = 1, gVue_ = 0, gBalais_ = 0;
    float pGoutte_ = 0, pBulle_ = 0;
    Grain gouttes_[GRAINS], bulles_[GRAINS];
    float phaseA_ = 0, phaseDebut_ = 0, depuisChoc_ = 1, chocT_ = 1e9f, chocAmp_ = 0, phBourdon_ = 0;
    bool marcheVue_ = false;
    uint64_t nToit_ = 0, nPb_ = 0, nGouttes_ = 0, nBulles_ = 0, nChocs_ = 0;
};

// banc d'essai PC (RG-10) : pluie-arriere, pluie-cockpit, pluie-tunnel (.wav stéréo, moteur du banc + pluie ; .png : niveaux
// gauche / droite et spectre de la pluie) ; coût au journal
void benchPluie(const std::string& out, const std::vector<float>& moteur);

} // namespace wfs::pluie
