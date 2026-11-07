// WaForSpeed — WFS-213 (épic E13) : état de l'exposition automatique de WFS-151 rendu sûr d'une scène à l'autre.
// En-tête seul : utilisé par eclairage_physique.cpp et vérifié par tests/fidelite_lumiere.cpp (RG-03), sans Vulkan.
//   RG-01 : à chaque nouvelle scène (init, changement de carte ou de voiture), l'EV100 repart de la valeur du moment
//           (EV_REF_JOUR / EV_REF_NUIT) et se recale sur la première mesure valide ;
//   RG-02 : aucune valeur non finie ni absurde n'entre dans l'adaptation : mesure de luminance ou sonde invalide → la
//           dernière valeur juste est gardée ; une capture dont une face n'a pas d'exposition connue est abandonnée.
// WFS-230 : borne basse de la nuit selon l'éclairement reçu (evMinNuit).
#pragma once
#include <algorithm>
#include <cmath>
#include "eclairage_unites.h"

namespace wfs::lumiere {

// luminance moyenne plausible d'une scène ou d'une sonde (cd/m²) : de la nuit sans lune (≈ 10⁻⁴) au ciel le plus clair
// (≈ 10⁴), avec une marge ; le disque du soleil n'est pas dans les captures (WFS-151)
constexpr double L_MIN_PLAUSIBLE = 1e-7, L_MAX_PLAUSIBLE = 1e5;
inline bool luminanceValide(double L) { return std::isfinite(L) && L >= L_MIN_PLAUSIBLE && L <= L_MAX_PLAUSIBLE; }
inline bool kFacesValides(const double k[6]) {
    for (int f = 0; f < 6; f++) if (!std::isfinite(k[f]) || !(k[f] > 0)) return false;
    return true;
}

// ---- WFS-230 (RG-01) : borne basse de l'exposition de nuit selon l'éclairement reçu (lampes + lune + ciel), pas de rayon fixe.
// Posemètre en lumière incidente à capteur plan : EV100 = log2(E × S / C), S = 100, C = 250 (ISO 2720:1974) : 20 lx (rue
// éclairée) → EV 3,0 ; 1 lx → −1,3 ; borne −2 atteinte sous 0,63 lx (rase campagne au clair de lune). La borne −2 laisse le
// paysage au clair de lune (EV mesurée −4,4 à −2,6) environ 1,5 IL sous le gris moyen : sombre mais lisible, pas de « nuit
// américaine » (avis expert-realisme-3d du 2026-11-07 ; Thompson, Shirley et Ferwerda 2002, « A Spatial Post-Processing
// Algorithm for Images of Night Scenes », J. Graphics Tools 7(1)) ; DA-realisme.md § 3.
constexpr double EV_LUNE_MIN = -2.0, C_INCIDENTE = 250.0;
// lampadaire routier : éclairement horizontal sous la lampe 15 lx (classe P1 de l'EN 13201-2:2015, valeur moyenne [à vérifier])
constexpr double E_LAMPE_SOL = 15.0;
// éclairement horizontal à d mètres du pied d'une lampe ponctuelle de hauteur h (cosinus d'incidence : décroissance en 1/d³)
inline double eclairementLampe(double E0, double d, double h) {
    h = std::max(h, 1.0);
    const double q = 1.0 + (d / h) * (d / h);
    return E0 / (q * std::sqrt(q));
}
inline double evMinNuit(double eclairement) {
    if (!std::isfinite(eclairement)) return EV_NUIT_MIN;
    return std::clamp(std::log2(std::max(eclairement, 1e-9) * 100.0 / C_INCIDENTE), EV_LUNE_MIN, EV_NUIT_MIN);
}

struct Exposition {
    double ev = EV_REF_JOUR; bool init = false, recaler = true;
    // RG-01 : nouvelle scène : valeur du moment, recalée sur la première mesure valide
    void nouvelleScene(bool nuit) { ev = nuit ? EV_REF_NUIT : EV_REF_JOUR; init = false; recaler = true; }
    // cible bornée ; mesure invalide (RG-02) : la valeur actuelle (dernière juste)
    double cible(double Lmes, double evMin, double evMax) const {
        const double c = luminanceValide(Lmes) ? ev100Luminance(Lmes) : ev;
        return std::clamp(c, evMin, evMax);
    }
    // une image : recalage direct (première mesure valide d'une scène, exposition forcée), sinon adaptation de l'œil
    void image(double Lmes, double evMin, double evMax, double dt, bool force = false, double evForce = 0) {
        const double c = force ? evForce : cible(Lmes, evMin, evMax);
        if (force || !init || (recaler && luminanceValide(Lmes))) { ev = c; init = true; }
        else ev = adapter(ev, c, dt);
        if (!std::isfinite(ev)) ev = std::clamp(EV_REF_JOUR, evMin, evMax);   // jamais de valeur non finie
    }
};

} // namespace wfs::lumiere
