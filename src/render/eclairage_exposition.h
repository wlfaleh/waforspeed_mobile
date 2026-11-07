// WaForSpeed — WFS-213 (épic E13) : état de l'exposition automatique de WFS-151 rendu sûr d'une scène à l'autre.
// En-tête seul : utilisé par eclairage_physique.cpp et vérifié par tests/fidelite_lumiere.cpp (RG-03), sans Vulkan.
//   RG-01 : à chaque nouvelle scène (init, changement de carte ou de voiture), l'EV100 repart de la valeur du moment
//           (EV_REF_JOUR / EV_REF_NUIT) et se recale sur la première mesure valide ;
//   RG-02 : aucune valeur non finie ni absurde n'entre dans l'adaptation : mesure de luminance ou sonde invalide → la
//           dernière valeur juste est gardée ; une capture dont une face n'a pas d'exposition connue est abandonnée.
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
