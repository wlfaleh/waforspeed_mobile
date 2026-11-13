// WaForSpeed — garage : prix en texte dans la langue active (WFS-499, RG-05 ; reprise des équivalents de WFS-489) : voir garage_achat.h.
// Calcul seul, sans rendu : testé par tests/test_garage_voitures.cpp.
#include <cstdio>
#include "garage_achat.h"
#include "../game/boutique.h"
#include "langue.h"

namespace wfs::ecrans::garage {

std::string nombreLangue(long long v) {
    // français : espace fine insécable, comme ui::nombre (« 2 200 ») ; anglais : virgule (« 2,200 »)
    const char* sep = ui::langue::active() == ui::langue::Langue::Francais ? "\xE2\x80\xAF" : ",";
    std::string d = std::to_string(v < 0 ? -v : v), out;
    for (size_t i = 0; i < d.size(); i++) { if (i && (d.size() - i) % 3 == 0) out += sep; out += d[i]; }
    return v < 0 ? "-" + out : out;
}

std::string texteMedailles(long long prix) {
    // reprise du texte de boutique::equivalentMedailles (« 3 or + 1 argent + 1 bronze »), chaque nom traduit
    std::string med, t = boutique::equivalentMedailles(prix) + " + ";
    for (size_t a = 0, b; (b = t.find(" + ", a)) != std::string::npos; a = b + 3) {
        const std::string part = t.substr(a, b - a);
        const size_t sp = part.find(' ');
        if (sp == std::string::npos) continue;
        med += (med.empty() ? "" : " + ") + part.substr(0, sp) + " " + ui::tr(("medaille." + part.substr(sp + 1)).c_str());
    }
    return med;
}

std::string texteEuros(long long prix) {
    const long long c = boutique::equivalentCentimes(prix);
    char eu[48];
    if (ui::langue::active() == ui::langue::Langue::Francais) std::snprintf(eu, sizeof eu, "%lld,%02lld €", c / 100, c % 100);
    else std::snprintf(eu, sizeof eu, "€%lld.%02lld", c / 100, c % 100);
    return eu;
}

std::string texteEquivalent(long long prix) { return nombreLangue(prix) + " ★ = " + texteMedailles(prix) + " = " + texteEuros(prix); }

std::string texteEquivalentVoiture(long long prix) { return "≈ " + texteMedailles(prix) + " · ≈ " + texteEuros(prix); }

} // namespace wfs::ecrans::garage
