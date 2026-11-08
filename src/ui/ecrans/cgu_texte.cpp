// WaForSpeed — CGU (WFS-308) : lecture du Markdown simple de store/cgu.md et store/confidentialite.md (voir cgu.h).
// Sans dépendance au rendu : testé seul par tests/test_cgu.cpp.
#include "cgu.h"
#include <cctype>

namespace wfs {
namespace ecrans {
namespace {

std::string sansEspaces(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) b--;
    return s.substr(a, b - a);
}

// mise en forme retirée : **gras**, *italique*, `code`, [texte](adresse) → « texte (adresse) »
std::string texteSimple(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); i++) {
        const char c = s[i];
        if (c == '*' || c == '`') continue;
        if (c == '[') {
            const size_t f = s.find("](", i), p = f == std::string::npos ? f : s.find(')', f);
            if (p != std::string::npos) {
                const std::string t = s.substr(i + 1, f - i - 1), u = s.substr(f + 2, p - f - 2);
                o += texteSimple(t) + (u.empty() || u == t ? "" : " (" + u + ")");
                i = p;
                continue;
            }
        }
        o += c;
    }
    return o;
}

} // namespace

std::vector<ParagrapheCgu> paragraphesMarkdown(const std::string& md) {
    std::vector<ParagrapheCgu> v;
    size_t debut = 0;
    auto ajouter = [&](ParagrapheCgu::Style st, const std::string& t) {
        if (st == ParagrapheCgu::VIDE) { if (!v.empty() && v.back().style != ParagrapheCgu::VIDE) v.push_back({st, ""}); return; }
        // ligne de texte qui suit une ligne de texte (ou la suite d'une puce, indentée) : même paragraphe
        if (st == ParagrapheCgu::TEXTE && !v.empty() && (v.back().style == ParagrapheCgu::TEXTE || v.back().style == ParagrapheCgu::PUCE)) {
            v.back().texte += " " + t;
            return;
        }
        v.push_back({st, t});
    };
    while (debut <= md.size()) {
        size_t fin = md.find('\n', debut);
        if (fin == std::string::npos) fin = md.size();
        const std::string l = sansEspaces(md.substr(debut, fin - debut));
        debut = fin + 1;
        if (l.empty()) ajouter(ParagrapheCgu::VIDE, "");
        else if (l.rfind("## ", 0) == 0) ajouter(ParagrapheCgu::SOUS_TITRE, texteSimple(sansEspaces(l.substr(3))));
        else if (l.rfind("# ", 0) == 0) ajouter(ParagrapheCgu::TITRE, texteSimple(sansEspaces(l.substr(2))));
        else if (l.rfind("- ", 0) == 0 || l.rfind("* ", 0) == 0) ajouter(ParagrapheCgu::PUCE, texteSimple(sansEspaces(l.substr(2))));
        else ajouter(ParagrapheCgu::TEXTE, texteSimple(l));
        if (fin == md.size()) break;
    }
    while (!v.empty() && v.back().style == ParagrapheCgu::VIDE) v.pop_back();
    return v;
}

int versionDansTexte(const std::string& md) {
    const size_t p = md.find("\nVersion ");
    if (p == std::string::npos) return 0;
    int n = 0;
    for (size_t i = p + 9; i < md.size() && std::isdigit((unsigned char)md[i]) && n < 100000; i++) n = n * 10 + (md[i] - '0');
    return n;
}

} // namespace ecrans
} // namespace wfs
