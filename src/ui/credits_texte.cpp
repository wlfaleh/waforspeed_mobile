// WaForSpeed — lecture du texte des crédits (WFS-406) : voir credits.h. Sans dépendance (testée par tests/test_credits.cpp).
#include "credits.h"

namespace wfs {
namespace credits {
namespace {
const char* LICENCE_TEXTE = "Texte de la licence";
std::string sansEspaces(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
} // namespace

std::vector<Paragraphe> paragraphes(const std::string& texte, const std::string& urlConfidentialite) {
    std::vector<Paragraphe> v;
    bool titre = false, licence = false, recoller = false;
    size_t debut = 0;
    while (debut <= texte.size()) {
        size_t fin = texte.find('\n', debut);
        if (fin == std::string::npos) fin = texte.size();
        const std::string brute = texte.substr(debut, fin - debut);
        debut = fin + 1;
        const std::string l = sansEspaces(brute);
        if (l.empty()) {
            if (titre && (v.empty() || v.back().style != Paragraphe::VIDE)) v.push_back({Paragraphe::VIDE, ""});
            recoller = false;
            if (fin == texte.size()) break;
            continue;
        }
        if (!titre) {   // 1re ligne : titre, puis l'adresse de la politique de confidentialité (RG-06)
            v.push_back({Paragraphe::TITRE, l});
            if (!urlConfidentialite.empty()) v.push_back({Paragraphe::TEXTE, "Politique de confidentialité : " + urlConfidentialite});
            titre = true;
            continue;
        }
        const bool retrait = brute[0] == ' ' || brute[0] == '\t';
        if (!retrait && l.size() > 1 && l.back() == ':') {   // « Section : »
            std::string s = sansEspaces(l.substr(0, l.size() - 1));
            licence = s.rfind(LICENCE_TEXTE, 0) == 0;
            v.push_back({Paragraphe::SECTION, s});
            recoller = false;
            continue;
        }
        if (licence) {   // texte d'une licence : lignes d'un même paragraphe recollées
            if (recoller) v.back().texte += " " + l;
            else v.push_back({Paragraphe::TEXTE, l});
            recoller = true;
            continue;
        }
        v.push_back({retrait ? Paragraphe::ENTREE : Paragraphe::TEXTE, l});
    }
    while (!v.empty() && v.back().style == Paragraphe::VIDE) v.pop_back();
    return v;
}

} // namespace credits
} // namespace wfs
