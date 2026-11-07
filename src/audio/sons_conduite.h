// WaForSpeed — sons de la conduite liés aux vraies forces (WFS-255, étape 3a du chantier « Conduite réaliste ») : volume du
// crissement des pneus, du frottement des freins et de la pulsation de l'ABS, calculés depuis l'état de la voiture du joueur
// (conduite en coordonnées du monde, WFS-241 / WFS-253 : Player::monde). Partie pure (en-tête seul, sans hasard) : testée par
// tests/test_sons_conduite.cpp ; jouée par SfxEvenements (sfx.cpp) sur la boucle de crissement existante (WFS-28).
//  - RG-01 : crissement proportionnel au glissement total normalisé σ du pneu (σ = 1 au pic de la courbe de Pacejka) : rien
//    sous le pic, montée douce (courbe en S) de σ = 1 à 1,8 ; jamais déclenché par le seul angle du doigt.
//  - RG-02 : freins : frottement discret proportionnel à la décélération ; ABS en action : pulsation à 12,5 Hz (rythme
//    usuel de la régulation d'un ABS, 10 à 15 cycles par seconde) ; roues bloquées (sans ABS) : σ ≈ 5,6 → crissement plein.
#pragma once
#include <algorithm>
#include <cmath>

namespace wfs::sonsconduite {

constexpr float GAIN_MAX = 0.35f;                 // = crissement de WFS-28 (setBoucles 0,35)
constexpr float SIGMA_DEBUT = 1.0f, SIGMA_PLEIN = 1.8f;
constexpr float ABS_HZ = 12.5f, ABS_HAUT = 0.14f, ABS_BAS = 0.02f;
constexpr float FREIN_MAX = 0.06f;

struct Entree {
    double vms = 0;                 // vitesse (m/s)
    double sigmaAv = 0, sigmaAr = 0;
    bool abs = false;               // ABS en action ce pas
    bool freine = false;            // le joueur freine
    double decel = 0;               // décélération (m/s², > 0 en freinant)
};

class Calcul {
public:
    // volume visé de la boucle de crissement (0..GAIN_MAX), un appel par pas de conduite
    float crissement(double dt, const Entree& e) {
        const float vit = (float)std::clamp((e.vms - 3.0) / 7.0, 0.0, 1.0);   // rien à l'arrêt, plein dès 36 km/h
        const float sg = (float)std::max(e.sigmaAv, e.sigmaAr);
        float x = std::clamp((sg - SIGMA_DEBUT) / (SIGMA_PLEIN - SIGMA_DEBUT), 0.f, 1.f);
        x = x * x * (3 - 2 * x);   // montée douce
        // RG-01 : pas de saut : le crissement monte en 0,15 s au plus vite (pneu qui part d'un coup), redescend en 0,1 s
        const float cible = GAIN_MAX * x * vit, monte = GAIN_MAX / 0.15f * (float)dt, descend = GAIN_MAX / 0.10f * (float)dt;
        glisse_ = std::clamp(cible, glisse_ - descend, glisse_ + monte);
        float g = glisse_;
        if (e.freine && e.decel > 0) g = std::max(g, FREIN_MAX * (float)std::min(1.0, e.decel / 9.81) * vit);
        if (e.abs) {
            phase_ += (float)dt * ABS_HZ; phase_ -= std::floor(phase_);
            g = std::max(g, (phase_ < 0.45f ? ABS_HAUT : ABS_BAS) * vit);
        } else phase_ = 0;
        return g;
    }
private:
    float phase_ = 0, glisse_ = 0;
};

} // namespace wfs::sonsconduite
