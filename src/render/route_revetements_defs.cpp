// WaForSpeed — WFS-205 : bibliothèque des revêtements de route : données et calculs sans GPU (tests/fidelite_routes.cpp).
#include <algorithm>
#include <cmath>
#include <cstring>
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

Carte carteDe(const char* id) {
    if (!id) return Carte::Ville;
    if (!std::strcmp(id, "campagne")) return Carte::Campagne;
    if (!std::strcmp(id, "plage")) return Carte::Corniche;
    if (!std::strcmp(id, "circuit")) return Carte::Circuit;
    return Carte::Ville;
}

Revetement chaussee(Carte c) {
    switch (c) {
    case Carte::Campagne: return Revetement::EnrobeCampagne;
    case Carte::Circuit: return Revetement::BetonCircuit;
    default: return Revetement::EnrobeUse;   // Ville, Corniche (enrobé usé : le plus clair des enrobés, 0,10-0,18)
    }
}

float codeMateriau(int qualite, Carte c, float neige, int tourSeg) {
    const float mode = qualite >= 2 ? 2.f : qualite == 1 ? 1.f : 0.f;
    const float k = std::round(std::clamp(neige, 0.f, 1.f) * (float)NEIGE_PAS);   // entier : la valeur reste exacte en flottant
    return mode + 4.f * (float)(int)c + 16.f * k + 256.f * (float)std::clamp(tourSeg, 0, TOUR_SEG_MAX);   // ≤ 2^24 − 1 : exact
}

// WFS-392 : abris (tunnels) de la carte rangés dans les cases du matériau (voir route_revetements.h)
Abris coderAbris(const std::vector<std::pair<int64_t, int64_t>>& t, int64_t tourSeg) {
    Abris a;
    const int nombre = (int)t.size();
    if (tourSeg <= 0 || tourSeg > TOUR_SEG_MAX) { a.tunnels = a.perdus = nombre; return a; }
    int cases = 0;
    for (int i = 0; i < nombre; i++) {
        int64_t debut = t[(size_t)i].first, fin = t[(size_t)i].second;
        while (i + 1 < nombre && t[(size_t)i + 1].first <= fin) fin = std::max(fin, t[(size_t)++i].second);   // abris qui se touchent : un seul
        a.tunnels++;
        bool entier = debut >= 0 && fin > debut && fin <= tourSeg;
        // cases de l'abri : morceaux de ABRI_LONG_MAX segments au plus, chacun recouvrant le précédent
        int besoin = 0;
        for (int64_t d = debut; entier && d < fin; d = d + ABRI_LONG_MAX >= fin ? fin : d + ABRI_LONG_MAX - ABRI_RECOUVRE) besoin++;
        entier = entier && cases + besoin <= ABRIS_MAX && fin <= ABRI_DEBUT_MAX;   // tout début de morceau < fin
        if (!entier) { a.perdus++; continue; }
        for (int64_t d = debut; d < fin; d = d + ABRI_LONG_MAX >= fin ? fin : d + ABRI_LONG_MAX - ABRI_RECOUVRE)
            a.code[cases++] = (float)(d * 1024 + std::min<int64_t>(ABRI_LONG_MAX, fin - d));
    }
    if (cases > 0) a.tourSeg = (int)tourSeg;
    return a;
}

float abriRoute(const Abris& a, double yM) {
    const double tour = a.tourSeg * SEG_ROUTE_M;
    if (tour <= 0) return 1.f;
    const double d = yM - std::floor(yM / tour) * tour;
    double sec = 0;
    for (const float c : a.code) {
        const double lon = std::fmod((double)c, 1024.0);
        if (lon < 0.5) continue;
        const double debut = std::floor((double)c / 1024.0) * SEG_ROUTE_M;
        const double x = std::clamp((std::min(d - debut, debut + lon * SEG_ROUTE_M - d) + 1.0) / 9.0, 0.0, 1.0);   // smoothstep(−1, 8)
        sec = std::max(sec, x * x * (3 - 2 * x));
    }
    return (float)(1.0 - sec);
}

int niveauxSautes(uint32_t cote, uint32_t maxi) {
    int n = 0;
    while (cote > maxi && cote / 2 >= 4) { cote /= 2; n++; }
    return n;
}

} // namespace wfs::route
