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
float Auto::echelleMin() const { return ECHELLE_MIN / echelleNiveau(niveauNettete(niveau)); }

int Auto::image(double dt, bool enCourse, double gpuMs) {
    if (!enCourse) { t = 0; n = 0; return RIEN; }
    t += dt; n++;
    if (t < 2) return RIEN;
    const double fps = n / t; t = 0; n = 0;
    fpsMesure = (float)fps; mesures++;
    int effet = RIEN;
    const float prev = echelle; const int palierPrev = palier;
    // WFS-246 (levier 3) : budget de l'image du niveau (16,7 ms en Basse, 22 ms en Moyenne / Haute) ; 5 % de tolérance
    // (image calée sur 60 Hz : 57-60 img/s mesurées). Marge : temps GPU sous 65 % du budget (une remontée de 0,1 coûte
    // jusqu'à 35 % de pixels en plus) ; sans horodatages, la règle de watchPerf (> 57 img/s) hors Basse.
    const double budget = budgetMs(niveau), ms = 1000.0 / fps;
    const bool trop = ms > budget * 1.05;
    const bool marge = gpuMs > 0 ? gpuMs < budget * 0.65 && ms < budget : (fps > 57 && niveau != BASSE);
    const float bas = echelleMin();
    // +(x).toFixed(2) : arrondi au centième (pas de dérive 0,6000001)
    // WFS-222 (RG-03) : sous 30 images/s, pas de 0,2 (allègement complet en 4 s au lieu de 8)
    const float pas = fps < 30 ? 0.2f : 0.1f;
    if (trop) {
        if (echelle > bas + 1e-4f) echelle = std::max(bas, std::round((echelle - pas) * 100) / 100);
        else if (palier < 2) palier++;   // déjà à 60 % : alors seulement, les effets
    } else if (marge) {
        if (palier > 0) palier--;
        else if (echelle < 1.f) echelle = std::min(1.f, std::round((echelle + 0.1f) * 100) / 100);
    }
    if (echelle != prev || palier != palierPrev) effet |= ECHELLE;
    if (!fait && !choixJoueur && niveau > BASSE && echelle <= bas + 1e-4f && fps < 30) { fait = true; niveau--; effet |= RETRO; }
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
