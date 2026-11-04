// WaForSpeed — qualité automatique (WFS-15) : voir qualite_auto.h.
#include "qualite_auto.h"
#include <algorithm>
#include <cmath>

namespace wfs::qualite {

// watchPerf(raw) du jeu web
int Auto::image(double dt, bool enCourse) {
    if (!enCourse) { t = 0; n = 0; return RIEN; }
    t += dt; n++;
    if (t < 2) return RIEN;
    const double fps = n / t; t = 0; n = 0;
    fpsMesure = (float)fps;
    int effet = RIEN;
    const float prev = echelle;
    // +(x).toFixed(2) : arrondi au centième (pas de dérive 0,6000001)
    if (fps < 40 && echelle > 0.6f) echelle = std::max(0.6f, std::round((echelle - 0.1f) * 100) / 100);
    else if (fps > 57 && echelle < 1.f) echelle = std::min(1.f, std::round((echelle + 0.1f) * 100) / 100);
    if (echelle != prev) effet |= ECHELLE;
    if (!fait && !choixJoueur && niveau > BASSE && echelle <= 0.6f && fps < 30) { fait = true; niveau--; effet |= RETRO; }
    return effet;
}

} // namespace wfs::qualite
