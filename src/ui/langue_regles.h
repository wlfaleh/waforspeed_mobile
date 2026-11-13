// WaForSpeed — règles du système de langue (WFS-504, épic E20) : calculs seuls, sans rendu ni fichier, testés par
// tests/test_langue.cpp.
//   - RG-01 : langue du téléphone « fr » (quel que soit le pays : fr-FR, fr-CA, fr_MA…) → français ; toute autre → anglais ;
//   - RG-02 : réglage « Langue » Automatique / Français / English (clé langue=auto|fr|en de la sauvegarde, RG-04) ;
//   - RG-05 : tables « cle=texte » (UTF-8, « \n » = retour à la ligne, {1} {2} = valeurs) ;
//   - RG-07 : garde-fou « phrase française » (lettre accentuée française ou mot français courant dans un texte anglais).
#pragma once
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace wfs::ui::langue {

enum class Langue { Francais = 0, Anglais = 1 };
enum class Choix { Automatique = 0, Francais = 1, Anglais = 2 };   // ordre des boutons du réglage

// RG-01 : code de langue d'Android (Locale.getLanguage(), ou « fr-CA », « fr_MA ») → langue du jeu
inline Langue langueDuCode(const std::string& code) {
    std::string l;
    for (char c : code) { if (c == '-' || c == '_') break; l += (char)std::tolower((unsigned char)c); }
    return l == "fr" ? Langue::Francais : Langue::Anglais;   // illisible ou vide → anglais
}
inline Langue langueActive(Choix choix, const std::string& codeTelephone) {
    if (choix == Choix::Francais) return Langue::Francais;
    if (choix == Choix::Anglais) return Langue::Anglais;
    return langueDuCode(codeTelephone);
}

// RG-04 : clé langue de la sauvegarde ; valeur inconnue ou absente → auto
inline Choix choixDeCle(const std::string& v) { return v == "fr" ? Choix::Francais : v == "en" ? Choix::Anglais : Choix::Automatique; }
inline const char* cleDeChoix(Choix c) { return c == Choix::Francais ? "fr" : c == Choix::Anglais ? "en" : "auto"; }

// RG-05 : lecture d'une table « cle=texte ». Lignes vides et lignes commençant par « # » ignorées ; « \n » écrit dans le
// fichier devient un vrai retour à la ligne. Rend le nombre de lignes illisibles (sans « = » ou clé vide) ; une clé en double
// garde la dernière valeur et compte comme illisible (le test l'interdit).
template <class Table>   // std::map<std::string, std::string> (avec ou sans std::less<> : recherche sans copie)
inline int analyserTable(const std::string& contenu, Table& table) {
    int illisibles = 0;
    size_t a = 0;
    if (contenu.compare(0, 3, "\xEF\xBB\xBF") == 0) a = 3;   // BOM UTF-8 éventuel
    while (a < contenu.size()) {
        size_t b = contenu.find('\n', a);
        if (b == std::string::npos) b = contenu.size();
        std::string ligne = contenu.substr(a, b - a);
        a = b + 1;
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        if (ligne.empty() || ligne[0] == '#') continue;
        const size_t eq = ligne.find('=');
        if (eq == std::string::npos || eq == 0) { illisibles++; continue; }
        std::string cle = ligne.substr(0, eq), texte;
        while (!cle.empty() && cle.back() == ' ') cle.pop_back();
        for (size_t i = eq + 1; i < ligne.size(); i++) {
            if (ligne[i] == '\\' && i + 1 < ligne.size() && ligne[i + 1] == 'n') { texte += '\n'; i++; }
            else texte += ligne[i];
        }
        if (table.count(cle)) illisibles++;
        table[cle] = texte;
    }
    return illisibles;
}

// marqueurs {1} à {9} d'un texte (ensemble trié, pour comparer les deux langues : RG-06)
inline std::set<int> marqueurs(const std::string& t) {
    std::set<int> m;
    for (size_t i = 0; i + 2 < t.size(); i++)
        if (t[i] == '{' && t[i + 1] >= '1' && t[i + 1] <= '9' && t[i + 2] == '}') m.insert(t[i + 1] - '0');
    return m;
}
// RG-05 : remplace {1}, {2}… par les valeurs (marqueur sans valeur : laissé vide, jamais « {3} » à l'écran)
inline std::string remplir(const std::string& t, const std::vector<std::string>& valeurs) {
    std::string out;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '{' && i + 2 < t.size() && t[i + 1] >= '1' && t[i + 1] <= '9' && t[i + 2] == '}') {
            const size_t n = (size_t)(t[i + 1] - '1');
            if (n < valeurs.size()) out += valeurs[n];
            i += 2;
        } else out += t[i];
    }
    return out;
}

// ---------------------------------------------------------------- RG-07 : garde-fou « phrase française »
// lettres accentuées françaises, minuscules et capitales (UTF-8) : é è ê à ç ù î ô û ë ï, â œ
inline const char* const ACCENTS_FR[] = {"é", "è", "ê", "à", "ç", "ù", "î", "ô", "û", "ë", "ï", "â", "œ",
                                         "É", "È", "Ê", "À", "Ç", "Ù", "Î", "Ô", "Û", "Ë", "Ï", "Â", "Œ"};
// mots français courants (comparés en minuscules, mot entier) ; « course », « vitesse », « réglages » : vocabulaire du jeu
inline const char* const MOTS_FR[] = {"le", "la", "les", "des", "du", "une", "et", "pour", "avec", "vitesse", "course", "réglages",
                                      "un", "au", "aux", "sur", "dans", "est", "pas", "ou", "voiture", "partie", "carte", "fermer",
                                      "activé", "activée", "coupé", "coupée", "oui", "non", "basse", "moyenne", "haute",
                                      "musique", "droitier", "gaucher", "glisser", "flèches", "boutons", "grands", "petite", "grande",
                                      "affichage", "conduite", "caméra", "graphismes", "commandes", "menu principal"};
// noms propres inventés et mots identiques dans les deux langues, acceptés dans un texte anglais (« Français » : RG-02)
inline const char* const LISTE_BLANCHE[] = {"FRANÇAIS", "Français", "français"};

// minuscules (ASCII et capitales accentuées Latin-1 : À..Þ → à..þ)
inline std::string minuscules(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        const unsigned char c = (unsigned char)s[i];
        if (c >= 'A' && c <= 'Z') out += (char)(c + 32);
        else if (c == 0xC3 && i + 1 < s.size() && (unsigned char)s[i + 1] >= 0x80 && (unsigned char)s[i + 1] <= 0x9E &&
                 (unsigned char)s[i + 1] != 0x97) { out += (char)0xC3; out += (char)((unsigned char)s[i + 1] + 0x20); i++; }
        else if (c == 0xC5 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0x92) { out += (char)0xC5; out += (char)0x93; i++; }   // Œ → œ
        else out += (char)c;
    }
    return out;
}

// rend la raison (« lettre é », « mot « pour » ») si le texte, affiché en anglais, contient du français ; vide sinon
inline std::string phraseFrancaise(const std::string& texte) {
    std::string t = texte;
    for (const char* b : LISTE_BLANCHE) for (size_t p; (p = t.find(b)) != std::string::npos;) t.replace(p, std::string(b).size(), " ");
    for (const char* a : ACCENTS_FR) if (t.find(a) != std::string::npos) return std::string("lettre ") + a;
    const std::string m = " " + minuscules(t) + " ";
    // découpe en mots : tout ce qui n'est ni lettre ASCII ni octet UTF-8 sépare deux mots
    std::string mots = " ";
    for (unsigned char c : m) mots += (std::isalpha(c) || c >= 0x80) ? (char)c : ' ';
    mots += " ";
    for (const char* w : MOTS_FR) if (mots.find(std::string(" ") + w + " ") != std::string::npos) return std::string("mot « ") + w + " »";
    return "";
}

} // namespace wfs::ui::langue
