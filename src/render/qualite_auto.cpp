// WaForSpeed — qualité automatique (WFS-15) : voir qualite_auto.h.
#include "qualite_auto.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace wfs::qualite {

// watchPerf(raw) du jeu web
int Auto::image(double dt, bool enCourse) {
    if (!enCourse) { t = 0; n = 0; return RIEN; }
    t += dt; n++;
    if (t < 2) return RIEN;
    const double fps = n / t; t = 0; n = 0;
    fpsMesure = (float)fps; mesures++;
    int effet = RIEN;
    const float prev = echelle;
    // +(x).toFixed(2) : arrondi au centième (pas de dérive 0,6000001)
    if (fps < 40 && echelle > 0.6f) echelle = std::max(0.6f, std::round((echelle - 0.1f) * 100) / 100);
    else if (fps > 57 && echelle < 1.f) echelle = std::min(1.f, std::round((echelle + 0.1f) * 100) / 100);
    if (echelle != prev) effet |= ECHELLE;
    if (!fait && !choixJoueur && niveau > BASSE && echelle <= 0.6f && fps < 30) { fait = true; niveau--; effet |= RETRO; }
    return effet;
}

// WFS-150 (RG-05) : mode diagnostic, lu une fois
bool modeDiagnostic() {
    if (diagnosticForce) return true;
    static const bool actif = [] {
#if defined(__ANDROID__)
        char v[PROP_VALUE_MAX] = {};
        if (__system_property_get("debug.waforspeed.diagnostic", v) > 0) return std::strcmp(v, "1") == 0;
        return false;
#else
        const char* e = std::getenv("WFS_DIAGNOSTIC");
        return e && !std::strcmp(e, "1");
#endif
    }();
    return actif;
}

// WFS-134 (RG-01) : « Rendu : Haute · 100 % · MSAA ×4 · 58 img/s · chauffe 0 »
std::string texteRendu(int niveau, float echelle, int msaa, float ips, int chauffe) {
    static const char* NOMS[4] = {"Basse", "Moyenne", "Haute", "Ultra"};   // WFS-150 : Ultra
    const char* nom = NOMS[niveau < BASSE ? BASSE : niveau > ULTRA ? ULTRA : niveau];
    char ip[16];
    if (ips >= 0) std::snprintf(ip, sizeof ip, "%.0f", ips); else std::snprintf(ip, sizeof ip, "--");
    char b[128];
    std::snprintf(b, sizeof b, "Rendu : %s · %.0f %% · MSAA ×%d · %s img/s · chauffe %d", nom, echelle * 100, msaa, ip, chauffe);
    return b;
}

} // namespace wfs::qualite
