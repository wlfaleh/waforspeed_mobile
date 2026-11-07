// WaForSpeed — WFS-205 : bibliothèque des revêtements de route : données et calculs sans GPU (tests/fidelite_routes.cpp).
#include <algorithm>
#include "route_revetements.h"

namespace wfs::route {

// Plages du réel (M-01 albédo moyen sRGB linéaire, M-02 rugosité sèche) : DA-realisme.md § 3, references/reel/materiaux.json et
// expert-realisme-3d (2026-11-07, journal de WFS-205) ; sources, statuts et gains appliqués : references/reel/routes/revetements.json.
// Côté des scans Poly Haven : 2 m (E). Même table que CIBLES de references/reel/routes/outils/preparer.py (à garder identiques).
static const InfoRevetement INFOS[NB_REVETEMENTS] = {
    {"enrobe-neuf",     "enrobé neuf",                   0.04f, 0.07f, 0.75f, 0.90f, 2.f},
    {"enrobe-use",      "enrobé usé",                    0.10f, 0.18f, 0.75f, 0.90f, 2.f},
    {"enrobe-campagne", "enrobé grenu de campagne",      0.08f, 0.14f, 0.82f, 0.92f, 2.f},
    {"beton-circuit",   "béton bitumineux de circuit",   0.05f, 0.09f, 0.72f, 0.85f, 2.f},
    {"beton-trottoir",  "béton de trottoir",             0.25f, 0.35f, 0.80f, 0.95f, 2.f},
    {"paves-granit",    "pavés de granit",               0.20f, 0.35f, 0.60f, 0.80f, 2.f},
    {"gravier",         "gravier de bas-côté",           0.20f, 0.35f, 0.90f, 1.00f, 2.f},
};

const InfoRevetement& info(Revetement r) { return INFOS[std::clamp((int)r, 0, NB_REVETEMENTS - 1)]; }

uint32_t coteMax(int qualite, bool orm) {
    const uint32_t c = qualite >= 2 ? 1024u : qualite == 1 ? 512u : 256u;   // Ultra / Haute, Moyenne, Basse
    return orm ? c / 2 : c;
}

int niveauxSautes(uint32_t cote, uint32_t maxi) {
    int n = 0;
    while (cote > maxi && cote / 2 >= 4) { cote /= 2; n++; }
    return n;
}

} // namespace wfs::route
