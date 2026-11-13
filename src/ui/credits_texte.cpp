// WaForSpeed — lecture du texte des crédits (WFS-406) : voir credits.h. Sans dépendance (testée par tests/test_credits.cpp).
#include "credits.h"
#include <algorithm>
#include <utility>

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

// WFS-505 (RG-02) : voir credits.h
std::string traduireCredits(const std::string& texte, const std::string& table) {
    std::vector<std::pair<std::string, std::string>> paires;
    size_t a = 0;
    while (a < table.size()) {
        size_t b = table.find('\n', a);
        if (b == std::string::npos) b = table.size();
        std::string l = table.substr(a, b - a);
        a = b + 1;
        if (!l.empty() && l.back() == '\r') l.pop_back();
        if (l.empty() || l[0] == '#') continue;
        const size_t s = l.find('|');
        if (s == std::string::npos || s == 0) continue;
        paires.emplace_back(l.substr(0, s), l.substr(s + 1));
    }
    std::stable_sort(paires.begin(), paires.end(), [](const auto& x, const auto& y) { return x.first.size() > y.first.size(); });
    std::string t;
    for (size_t i = 0; i < texte.size();) {
        bool fait = false;
        for (const auto& [fr, en] : paires)
            if (texte.compare(i, fr.size(), fr) == 0) { t += en; i += fr.size(); fait = true; break; }
        if (!fait) t += texte[i++];
    }
    // typographie anglaise
    std::string out;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == ' ' && i + 1 < t.size() && (t[i + 1] == ':' || t[i + 1] == ';')) continue;   // « mot : » → « mot: »
        if (t.compare(i, 3, "\xC2\xAB ") == 0) { out += "\xE2\x80\x9C"; i += 2; continue; }   // « + espace → “
        if (t.compare(i, 3, " \xC2\xBB") == 0) { out += "\xE2\x80\x9D"; i += 2; continue; }   // espace + » → ”
        if (t.compare(i, 2, "\xC2\xAB") == 0) { out += "\xE2\x80\x9C"; i += 1; continue; }
        if (t.compare(i, 2, "\xC2\xBB") == 0) { out += "\xE2\x80\x9D"; i += 1; continue; }
        out += t[i];
    }
    return out;
}

void traduireParagraphes(std::vector<Paragraphe>& paras, const std::string& table) {
    bool licence = false;
    for (Paragraphe& p : paras) {
        if (p.style == Paragraphe::VIDE) continue;
        if (p.style == Paragraphe::SECTION) licence = p.texte.rfind(LICENCE_TEXTE, 0) == 0;
        else if (licence && p.style == Paragraphe::TEXTE) continue;   // texte d'une licence : déjà en anglais
        p.texte = traduireCredits(p.texte, table);
    }
}

} // namespace credits
} // namespace wfs
