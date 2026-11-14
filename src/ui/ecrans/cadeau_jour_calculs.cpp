// WaForSpeed — écran du cadeau du jour (WFS-501) : calculs purs, sans rendu (testés par tests/test_cadeau_ecran.cpp). Voir cadeau_jour.h.
#include <algorithm>
#include <cstdio>
#include "cadeau_jour.h"

namespace wfs {
namespace ecrans::cadeau_jour {

// ---------------------------------------------------------------- calculs purs
long long secondesAvantProchain(const EtatCadeau& c, const cadeau::Instant& t) {
    if (cadeau::etat(c, t) != cadeau::Etat::DejaOuvert) return -1;
    const long long minuit = (cadeau::jourLocal(t) + 1) * 86400LL - t.decalage;   // prochain minuit local, en secondes UTC
    return std::max(0LL, minuit - t.epoch);
}

std::string texteHeures(long long s) {
    s = std::clamp(s, 0LL, 86400LL);
    const long long minutes = (s + 59) / 60;   // arrondi au-dessus : « 00:01 » jusqu'à la dernière seconde
    char b[16];
    std::snprintf(b, sizeof b, "%02lld:%02lld", std::min(minutes / 60, 24LL), minutes % 60);
    return b;
}

std::vector<LigneChance> lignesChances(const ConfigJeu::Cadeau& c) {
    std::vector<LigneChance> v;
    for (const auto& t : c.tranche)
        v.push_back({"cadeau.chances.etoiles", std::to_string(t.min), std::to_string(t.max), t.chance});
    char h[16];
    std::snprintf(h, sizeof h, "%g", c.pretHeures);
    v.push_back({"cadeau.chances.coloris", "", "", c.chanceColoris});
    v.push_back({"cadeau.chances.skin", "", "", c.chanceSkin});
    v.push_back({"cadeau.chances.voiture", h, "", c.chanceVoiture});
    return v;
}

int restantsGarantie(const ConfigJeu::Cadeau& c, const EtatCadeau& e) { return std::max(1, c.garantie - e.sansRare); }

} // namespace ecrans::cadeau_jour
} // namespace wfs
