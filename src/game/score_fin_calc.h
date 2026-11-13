// WaForSpeed — WFS-537 (épic E22) : calculs purs de l'écran de fin du score et du compteur d'étoiles du menu, sans rendu
// (testés par tests/test_score_fin.cpp) :
//   - etoilesAffichees : « +N ★ » qui compte de 0 à N en moins de 1 s (RG-02) ;
//   - nombreEtoiles : grand nombre lisible, « 12 450 » en français (espace fine insécable), « 12,450 » en anglais (RG-08).
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

namespace wfs::score_fin {

constexpr double DUREE_COMPTE = 0.8;   // RG-02 : le compte de 0 à N dure 0,8 s (< 1 s)

// étoiles montrées `ecoule` secondes après l'apparition du bloc : 0 au début, N à DUREE_COMPTE et après ; jamais au-delà de N
inline int etoilesAffichees(int n, double ecoule, double duree = DUREE_COMPTE) {
    if (n <= 0) return 0;
    if (!(ecoule > 0)) return 0;   // aussi NaN
    if (duree <= 0 || ecoule >= duree) return n;
    const double k = ecoule / duree, e = 1 - (1 - k) * (1 - k);   // départ rapide, fin douce (ease-out)
    return std::clamp((int)std::floor(e * n), 0, n);
}

// RG-08 : séparateur des milliers selon la langue ; nombres négatifs écrits avec « - »
inline std::string nombreEtoiles(long long v, bool anglais) {
    const bool neg = v < 0;
    const unsigned long long a = neg ? 0ULL - (unsigned long long)v : (unsigned long long)v;
    const std::string d = std::to_string(a), sep = anglais ? "," : "\xE2\x80\xAF";
    std::string out;
    for (size_t i = 0; i < d.size(); i++) {
        if (i && (d.size() - i) % 3 == 0) out += sep;
        out += d[i];
    }
    return neg ? "-" + out : out;
}

} // namespace wfs::score_fin
