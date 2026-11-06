// WaForSpeed — sirène de la police (WFS-131, épic E6) : vraie sirène continue (« wail » qui monte et descend, « yelp »
// rapide quand la police colle), volume selon la distance, effet Doppler, jingles de fin de poursuite (ARRÊTÉ / ÉCHAPPÉ).
// Le jeu web n'a pas de sirène : valeurs fixées par le PO (regles-de-gestion/WFS-131.md), ajustables dans le bloc de
// constantes ci-dessous. En-tête seul, sans dépendance au jeu : décisions (thread du jeu, SireneSon), lecture des boucles
// (thread audio, SfxMixer::mix), fabrication des sons (démarrage, SfxMixer::build) ; utilisé par sfx.cpp,
// audio_system.cpp (banc : sirene.wav) et tests/test_audio.cpp.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
#include "sfx_synth.h"

namespace wfs {

namespace sirene {
// RG-01 / RG-02 : gamme et cycles des deux boucles
constexpr double F_BAS = 650, F_HAUT = 1450;          // Hz
constexpr float CYCLE_WAIL = 3.2f;                    // montée de 1,6 s puis descente de 1,6 s
constexpr float CYCLE_YELP = 0.30f;                   // même gamme, cycle rapide
constexpr double H2 = 0.35, H3 = 0.15;                // harmoniques 2 et 3 (timbre de haut-parleur)
constexpr float CRETE = 0.9f;                         // crête des boucles (comme boucleRumeur)
constexpr float RACCORD_S = 0.05f;                    // fin fondue dans le début (raccord sans clic)
constexpr double YELP_ENTREE_M = 20, YELP_SORTIE_M = 25;   // hystérésis : yelp sous 20 m, wail au-delà de 25 m
constexpr float FONDU_MODE_S = 0.15f;                 // passage wail ↔ yelp
// RG-03 : volume selon la distance de la police mobile la plus proche
constexpr double PRES_M = 15, LOIN_M = 150;
constexpr float GAIN_PRES = 0.45f;                    // gain à 15 m ou moins (0 à 150 m ou plus, linéaire entre les deux)
constexpr float LISSAGE_S = 0.3f;                     // montée / descente à 95 % en 0,3 s (même convention que trafic::RUMEUR_TAU)
constexpr float FIN_S = 0.8f;                         // RG-05 : extinction linéaire après la fin de la course
// RG-04 : effet Doppler
constexpr double C_SON = 343;                         // vitesse du son dans l'air sec à 20 °C (m/s)
constexpr double DOPPLER_MAX = 0.12;                  // facteur borné à 1 ± 0,12
constexpr double AUCUNE = 1e8;                        // distance ≥ AUCUNE : aucune police
// RG-09 : journal
constexpr float JOURNAL_ECART = 0.05f;
} // namespace sirene

// ---------------------------------------------------------------- décisions (fonctions pures, testées)
// RG-04 : facteur de hauteur c / (c − v), v = vitesse de la police vers le joueur (m/s, > 0 si elle se rapproche)
inline float dopplerSirene(double vRapprochementMs) {
    using namespace sirene;
    if (!std::isfinite(vRapprochementMs)) return 1;
    const double f = vRapprochementMs >= C_SON ? 1 + DOPPLER_MAX : C_SON / (C_SON - vRapprochementMs);
    return (float)std::clamp(f, 1 - DOPPLER_MAX, 1 + DOPPLER_MAX);
}
// RG-03 : 0,45 à 15 m ou moins, 0 à 150 m ou plus (et sans police), linéaire entre les deux
inline float gainSirene(double distanceM) {
    using namespace sirene;
    if (!(distanceM < LOIN_M)) return 0;   // aussi : distance inconnue (NaN)
    if (distanceM <= PRES_M) return GAIN_PRES;
    return (float)(GAIN_PRES * (LOIN_M - distanceM) / (LOIN_M - PRES_M));
}
// RG-02 : « yelp » sous 20 m ; on y reste jusqu'à 25 m (pas de va-et-vient entre 20 et 25 m)
inline bool yelpSirene(bool yelpAvant, double distanceM) {
    return yelpAvant ? distanceM <= sirene::YELP_SORTIE_M : distanceM < sirene::YELP_ENTREE_M;
}

// ---------------------------------------------------------------- thread audio (aucune allocation)
// gain joué vers le gain visé : lissage exponentiel (coefficient k par échantillon) ; en fin de course (RG-05), descente
// linéaire de pasFin par échantillon : de GAIN_PRES à 0 en FIN_S au plus, jamais coupé net
// pas de la descente de fin, par échantillon : 1 % de marge pour que les arrondis du float (pas de ≈ 1e-5 retirés d'un gain
// de 0,45) ne fassent jamais dépasser FIN_S
inline float pasFinSirene(float rate) { return sirene::GAIN_PRES / (sirene::FIN_S * rate) * 1.01f; }
inline float suivreGainSirene(float g, float cible, bool fin, float k, float pasFin) {
    if (fin && cible < g) return std::max(cible, g - pasFin);
    return g + (cible - g) * k;
}
// lecture fractionnaire d'une boucle (pas de lecture variable = Doppler), interpolation linéaire, raccord fin → début
inline float lireBoucle(const std::vector<float>& b, double pos) {
    const size_t i = (size_t)pos, j = i + 1 < b.size() ? i + 1 : 0;
    const float f = (float)(pos - (double)i);
    return b[i] + (b[j] - b[i]) * f;
}
inline double avancerBoucle(double pos, double pas, size_t n) {
    pos += pas;
    while (pos >= (double)n) pos -= (double)n;
    return pos;
}

// ---------------------------------------------------------------- fabrication (une fois, au démarrage)
// RG-01 / RG-02 : un cycle de sirène. Fréquence f(t) = fm − a·cos(2πt/T) : 650 Hz au début, 1 450 Hz à mi-cycle, retour à
// 650 Hz, sans saut ; la phase est l'intégrale exacte de f. Onde = fondamentale + 35 % d'harmonique 2 + 15 % d'harmonique 3.
// Avec les valeurs du PO, fm·T est entier (1 050 Hz × 3,2 s = 3 360 ; × 0,30 s = 315) : le cycle se raccorde de lui-même ;
// la fin est quand même fondue dans le début (comme boucleRumeur), pour rester sans clic si les constantes changent.
inline std::vector<float> boucleSirene(float rate, float cycle) {
    using namespace sirene;
    constexpr double PI = 3.14159265358979323846;
    const size_t n = std::max<size_t>(16, (size_t)std::lround(cycle * rate));
    const size_t x = std::min(n / 4, (size_t)(RACCORD_S * rate));
    const double T = (double)n / rate, fm = (F_BAS + F_HAUT) / 2, a = (F_HAUT - F_BAS) / 2;
    std::vector<float> tmp(n + x);
    for (size_t i = 0; i < tmp.size(); i++) {
        const double t = (double)i / rate, ph = 2 * PI * (fm * t - a * T / (2 * PI) * std::sin(2 * PI * t / T));
        tmp[i] = (float)(std::sin(ph) + H2 * std::sin(2 * ph) + H3 * std::sin(3 * ph));
    }
    std::vector<float> out(tmp.begin(), tmp.begin() + (long)n);
    for (size_t i = 0; i < x; i++) { const float k = (float)i / (float)x; out[i] = out[i] * k + tmp[n + i] * (1 - k); }
    float p = 0; for (float s : out) p = std::max(p, std::abs(s));
    if (p > 1e-6f) for (float& s : out) s *= CRETE / p;
    return out;
}
// RG-06 : « ARRÊTÉ » : deux notes graves descendantes, 330 puis 220 Hz, 0,25 s chacune (0,50 s)
inline std::vector<float> jingleArrete(float rate) {
    std::vector<float> b;
    sfx::beep(b, rate, 330, 0, 0.25f); sfx::beep(b, rate, 220, 0.25f, 0.25f);
    return b;
}
// RG-06 : « ÉCHAPPÉ » : arpège montant 523 / 659 / 784 Hz (do, mi, sol), 0,12 s chacune, la dernière tenue 0,35 s (0,59 s)
inline std::vector<float> jingleEchappe(float rate) {
    std::vector<float> b;
    sfx::beep(b, rate, 523, 0, 0.12f); sfx::beep(b, rate, 659, 0.12f, 0.12f); sfx::beep(b, rate, 784, 0.24f, 0.35f);
    return b;
}

// ---------------------------------------------------------------- décision à chaque pas du jeu (thread du jeu)
struct CiblesSirene {
    float gain = 0, doppler = 1;
    bool yelp = false, fin = false;
    bool journal = false;                  // RG-09 : écrire « sirene <wail|yelp> gain … doppler … police … m »
    double distanceM = sirene::AUCUNE;     // police la plus proche (pour le journal)
};

class SireneSon {
public:
    // appel : la poursuite a donné, à ce pas, la police mobile la plus proche (distance en m, ≥ AUCUNE : aucune) et sa
    // vitesse de rapprochement (m/s) ; sans appel, il n'y a pas de poursuite en cours (autre mode, menus, décompte).
    // enJeu : écran Course, course en jeu, son actif ; fin : course finie (extinction en FIN_S, RG-05).
    CiblesSirene update(double dt, bool appel, double distanceM, double vRapprochementMs, bool enJeu, bool fin) {
        CiblesSirene c;
        const bool police = appel && distanceM < sirene::AUCUNE;
        if (police) c.distanceM = distanceM;
        if (police && enJeu) {
            yelp_ = yelpSirene(yelp_, distanceM);
            c.gain = gainSirene(distanceM); c.yelp = yelp_; c.doppler = dopplerSirene(vRapprochementMs);
        } else if (fin) {   // la sirène s'éteint dans le mode où elle était, la hauteur suit encore la police
            c.fin = true; c.yelp = yelp_; c.doppler = police ? dopplerSirene(vRapprochementMs) : 1.f;
        } else {
            yelp_ = false;   // la prochaine poursuite repart en wail
        }
        // RG-09 : une ligne par seconde au plus, si le gain a changé de plus de 0,05 ou si le mode a changé ; le retour à 0
        // est toujours écrit (preuve « aucune sirène coincée »)
        horloge_ += dt;
        const bool retourZero = c.gain == 0 && gainLog_ > 0;
        const bool change = std::abs(c.gain - gainLog_) > sirene::JOURNAL_ECART || (c.gain > 0 && c.yelp != yelpLog_);
        if ((horloge_ >= 1 && change) || retourZero) { c.journal = true; gainLog_ = c.gain; yelpLog_ = c.yelp; }
        if (horloge_ >= 1) horloge_ = 0;
        return c;
    }
private:
    bool yelp_ = false, yelpLog_ = false;
    float gainLog_ = 0;
    double horloge_ = 0;
};

} // namespace wfs
