// WaForSpeed — profil du joueur (WFS-480) : voir profil.h.
#include "profil.h"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <sstream>

namespace wfs {

namespace profil {

const char* motif(Refus r) {
    switch (r) {
        case Refus::TropCourt: return "trop court";
        case Refus::TropLong: return "trop long";
        case Refus::CaractereInterdit: return "caractère interdit";
        case Refus::MotInterdit: return "mot interdit";
        default: return "";
    }
}

// nombre de caractères UTF-8 (octets de suite 10xxxxxx non comptés)
static int nbCaracteres(const std::string& s) {
    int n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
    return n;
}

// espaces (seulement « ») retirés au début et à la fin
static std::string sansEspacesDeBord(const std::string& s) {
    const size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

Refus verifierPseudo(const std::string& brut, std::string* propre) {
    const std::string p = sansEspacesDeBord(brut);
    if (propre) *propre = p;
    const int n = nbCaracteres(p);
    if (n < PSEUDO_MIN) return Refus::TropCourt;
    if (n > PSEUDO_MAX) return Refus::TropLong;
    for (size_t i = 0; i < p.size(); i++) {
        const char c = p[i];
        const bool permis = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_';
        if (!permis || (c == ' ' && i > 0 && p[i - 1] == ' ')) return Refus::CaractereInterdit;   // RG-01 : pas deux espaces de suite
    }
    if (contientMotInterdit(p)) return Refus::MotInterdit;
    return Refus::Aucun;
}

Refus verifierPlaque(const std::string& brut, std::string* propre) {
    std::string p;
    for (char c : brut) p += (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;   // RG-02 : minuscules → majuscules
    if (propre) *propre = p;
    const int n = nbCaracteres(p);
    if (n < PLAQUE_MIN) return Refus::TropCourt;
    if (n > PLAQUE_MAX) return Refus::TropLong;
    for (size_t i = 0; i < p.size(); i++) {
        const char c = p[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) return Refus::CaractereInterdit;
        if (c == '-' && (i == 0 || i + 1 == p.size() || p[i - 1] == '-')) return Refus::CaractereInterdit;   // tiret au bord ou double
    }
    if (contientMotInterdit(p)) return Refus::MotInterdit;
    return Refus::Aucun;
}

std::string plaqueProposee(const std::string& pseudo, uint32_t (*hasard)()) {
    std::string p;
    for (char c : pseudo) {
        if ((int)p.size() >= PLAQUE_MAX) break;
        if (c >= 'a' && c <= 'z') p += (char)(c - 'a' + 'A');
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) p += c;
    }
    if (!p.empty() && verifierPlaque(p) == Refus::Aucun) return p;
    const unsigned n = hasard ? hasard() % 1000u : 0u;   // RG-03 : WFS- + 3 chiffres
    char buf[16];
    std::snprintf(buf, sizeof buf, "WFS-%03u", n);
    return buf;
}

// ---- RG-04 : mots interdits

// lettre sans accent d'un caractère UTF-8 de deux octets C3 xx (Latin-1) ; nullptr = caractère retiré
static const char* sansAccentC3(unsigned char b) {
    static const char* const T[64] = {
        "a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",   // C3 80..8F : À..Ï
        "d", "n", "o", "o", "o", "o", "o", nullptr, "o", "u", "u", "u", "u", "y", nullptr, "ss",   // C3 90..9F : Ð..ß
        "a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",   // C3 A0..AF : à..ï
        "d", "n", "o", "o", "o", "o", "o", nullptr, "o", "u", "u", "u", "u", "y", nullptr, "y",   // C3 B0..BF : ð..ÿ
    };
    return b >= 0x80 && b <= 0xBF ? T[b - 0x80] : nullptr;
}

std::string normaliser(const std::string& texte) {
    std::string o;
    for (size_t i = 0; i < texte.size(); i++) {
        const unsigned char c = (unsigned char)texte[i];
        if (c >= 0x80) {   // UTF-8 : accents du français ramenés à la lettre, Œ / œ → oe, le reste retiré
            if (c == 0xC3 && i + 1 < texte.size()) { if (const char* l = sansAccentC3((unsigned char)texte[i + 1])) o += l; i++; }
            else if (c == 0xC5 && i + 1 < texte.size() && ((unsigned char)texte[i + 1] == 0x92 || (unsigned char)texte[i + 1] == 0x93)) { o += "oe"; i++; }
            else while (i + 1 < texte.size() && ((unsigned char)texte[i + 1] & 0xC0) == 0x80) i++;   // octets de suite sautés
            continue;
        }
        if (c >= 'A' && c <= 'Z') { o += (char)(c - 'A' + 'a'); continue; }
        if (c >= 'a' && c <= 'z') { o += (char)c; continue; }
        switch (c) {   // chiffres « leet » ; les autres chiffres restent
            case '0': o += 'o'; break;
            case '1': o += 'i'; break;
            case '3': o += 'e'; break;
            case '4': o += 'a'; break;
            case '5': o += 's'; break;
            case '7': o += 't'; break;
            case '@': o += 'a'; break;
            case '$': o += 's'; break;
            default: if (c >= '0' && c <= '9') o += (char)c; break;   // espaces, tirets, tirets bas, ponctuation : retirés
        }
    }
    return o;
}

static std::vector<std::string>& interdits() { static std::vector<std::string> v; return v; }
static std::vector<std::string>& blancs() { static std::vector<std::string> v; return v; }

int chargerMotsInterdits(const std::string& contenu) {
    interdits().clear(); blancs().clear();
    std::istringstream in(contenu);
    std::string ligne;
    while (std::getline(in, ligne)) {
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        if (ligne.empty() || ligne[0] == '#') continue;
        const bool blanc = ligne[0] == '+';
        const std::string m = normaliser(blanc ? ligne.substr(1) : ligne);
        if (m.empty() || m.size() > 64) continue;
        auto& liste = blanc ? blancs() : interdits();
        if (std::find(liste.begin(), liste.end(), m) == liste.end()) liste.push_back(m);
    }
    return (int)interdits().size();
}

int nombreMotsInterdits() { return (int)interdits().size(); }

bool contientMotInterdit(const std::string& texte) {
    const std::string t = normaliser(texte);
    if (t.empty() || interdits().empty()) return false;
    // positions couvertes par un mot de la liste blanche (ex. « scunthorpe », « classe ») : un mot interdit entièrement
    // dedans ne compte pas
    std::vector<char> couvert(t.size(), 0);
    for (const std::string& b : blancs())
        for (size_t p = t.find(b); p != std::string::npos; p = t.find(b, p + 1))
            std::fill(couvert.begin() + (long)p, couvert.begin() + (long)(p + b.size()), 1);
    for (const std::string& m : interdits())
        for (size_t p = t.find(m); p != std::string::npos; p = t.find(m, p + 1))
            if (std::find(couvert.begin() + (long)p, couvert.begin() + (long)(p + m.size()), 0) != couvert.begin() + (long)(p + m.size())) return true;
    return false;
}

} // namespace profil

// ---- courses jouées

const char* idModeJoue(ModeJoue m) {
    switch (m) {
        case ModeJoue::Course5: return "course5";
        case ModeJoue::Poursuite: return "poursuite";
        case ModeJoue::Duel: return "duel";
        default: return "checkpoint";
    }
}
const char* nomModeJoue(ModeJoue m) {
    switch (m) {
        case ModeJoue::Course5: return "Course à 5";
        case ModeJoue::Poursuite: return "Poursuite";
        case ModeJoue::Duel: return "Duel";
        default: return "Checkpoint";
    }
}
static bool modeParId(const std::string& id, ModeJoue& m) {
    for (ModeJoue x : {ModeJoue::Checkpoint, ModeJoue::Course5, ModeJoue::Poursuite, ModeJoue::Duel})
        if (id == idModeJoue(x)) { m = x; return true; }
    return false;
}

std::string CourseJouee::resultat() const {
    char buf[64];
    if (quittee) return "Quittée";
    switch (mode) {
        case ModeJoue::Course5: {   // RG-10 : place et temps
            const long long cs = tempsMs / 10;
            std::snprintf(buf, sizeof buf, "%d%s en %lld:%02lld,%02lld", place, place == 1 ? "re" : "e", cs / 6000, cs / 100 % 60, cs % 100);
            return buf;
        }
        case ModeJoue::Poursuite:   // RG-10 : victoire / arrêté et score
            std::snprintf(buf, sizeof buf, "%s · %lld pts", victoire ? "Échappé" : "Arrêté", score);
            return buf;
        case ModeJoue::Duel: return victoire ? "Gagné" : "Perdu";
        default:   // Checkpoint : score
            std::snprintf(buf, sizeof buf, "%lld pts", score);
            return buf;
    }
}

std::string dateUtc(long long secondes) {
    const std::time_t t = (std::time_t)secondes;
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

// ---- sauvegarde (RG-11)

static bool entierPositif(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 18) return false;
    v = 0;
    for (char c : s) { if (c < '0' || c > '9') return false; v = v * 10 + (c - '0'); }
    return true;
}
// « a;b;c » → champs (au plus n)
static std::vector<std::string> champs(const std::string& s, size_t n) {
    std::vector<std::string> o;
    size_t a = 0;
    while (o.size() < n) {
        const size_t b = s.find(';', a);
        o.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a));
        if (b == std::string::npos) break;
        a = b + 1;
    }
    return o;
}
// identifiant de voiture lisible dans la sauvegarde (CARS[] : minuscules, chiffres, tirets bas) ; rien d'autre n'est gardé
static bool idVoitureOk(const std::string& id) {
    if (id.empty() || id.size() > 32) return false;
    for (char c : id) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    return true;
}

void Profil::ecrire(std::ostream& o) const {
    o << "profil=" << (cree ? 1 : 0) << "\n";
    if (!plaque.empty()) o << "plaque=" << plaque << "\n";
    if (arrivee > 0) o << "arrivee=" << arrivee << "\n";
    o << "stats=" << courses << ";" << metres << ";" << vmax << ";" << frolements << ";" << victoires << "\n";
    for (const auto& [id, v] : parVoiture) o << "voiture-" << id << "=" << v.metres << ";" << v.courses << "\n";
    for (size_t i = 0; i < dernieres.size() && i < (size_t)profil::HISTORIQUE_MAX; i++) {
        const CourseJouee& c = dernieres[i];
        char cle[24];
        std::snprintf(cle, sizeof cle, "historique-%02d", (int)i + 1);
        o << cle << "=" << idModeJoue(c.mode) << ";" << c.voiture << ";" << c.date << ";" << (c.quittee ? 1 : 0) << ";" << (c.victoire ? 1 : 0)
          << ";" << c.place << ";" << c.tempsMs << ";" << c.score << "\n";
    }
}

bool Profil::lireCle(const std::string& k, const std::string& v) {
    long long n = 0;
    if (k == "profil") { cree = v == "1"; return true; }
    if (k == "plaque") { std::string p; if (profil::verifierPlaque(v, &p) == profil::Refus::Aucun) plaque = p; return true; }
    if (k == "arrivee") { if (entierPositif(v, n) && n > 0) arrivee = n; return true; }
    if (k == "stats") {
        const auto f = champs(v, 5);
        long long w[5];
        bool ok = f.size() == 5;
        for (size_t i = 0; ok && i < 5; i++) ok = entierPositif(f[i], w[i]);
        if (ok) { courses = w[0]; metres = w[1]; vmax = (int)std::min(w[2], 9999LL); frolements = w[3]; victoires = w[4]; }
        return true;
    }
    if (k.rfind("voiture-", 0) == 0) {
        const std::string id = k.substr(8);
        const auto f = champs(v, 2);
        long long m = 0, c = 0;
        if (idVoitureOk(id) && f.size() == 2 && entierPositif(f[0], m) && entierPositif(f[1], c)) parVoiture[id] = {m, c};
        return true;
    }
    if (k.rfind("historique-", 0) == 0) {
        long long idx = 0;
        if (!entierPositif(k.substr(11), idx) || idx < 1 || idx > profil::HISTORIQUE_MAX) return true;
        const auto f = champs(v, 8);
        CourseJouee c;
        long long w[6];
        bool ok = f.size() == 8 && modeParId(f[0], c.mode) && idVoitureOk(f[1]);
        for (size_t i = 0; ok && i < 6; i++) ok = entierPositif(f[2 + i], w[i]);
        if (!ok) return true;   // ligne mal formée : ignorée (RG-11 : jamais d'erreur)
        c.voiture = f[1]; c.date = w[0]; c.quittee = w[1] == 1; c.victoire = w[2] == 1; c.place = (int)std::min(w[3], 99LL);
        c.tempsMs = w[4]; c.score = w[5];
        if ((long long)dernieres.size() < idx) dernieres.resize((size_t)idx);
        dernieres[(size_t)idx - 1] = c;
        return true;
    }
    return false;
}

void Profil::noterCourse(const CourseJouee& c) {
    // les cases vides (historique lu avec des trous) sont retirées avant d'ajouter
    dernieres.erase(std::remove_if(dernieres.begin(), dernieres.end(), [](const CourseJouee& x) { return x.voiture.empty(); }), dernieres.end());
    courses++;
    metres += std::max(0LL, c.metres);
    vmax = std::max(vmax, c.vmax);
    frolements += std::max(0, c.frolements);
    if (c.victoire && !c.quittee && c.mode != ModeJoue::Checkpoint) victoires++;   // RG-07 : un record de Checkpoint n'est pas une victoire
    if (idVoitureOk(c.voiture)) { ParVoiture& p = parVoiture[c.voiture]; p.metres += std::max(0LL, c.metres); p.courses++; }
    dernieres.insert(dernieres.begin(), c);
    if (dernieres.size() > (size_t)profil::HISTORIQUE_MAX) dernieres.resize(profil::HISTORIQUE_MAX);   // RG-10 : la 21e efface la plus ancienne
}

std::string Profil::voiturePreferee() const {
    const std::pair<const std::string, ParVoiture>* best = nullptr;
    for (const auto& e : parVoiture) {
        if (e.second.courses <= 0) continue;
        if (!best || e.second.metres > best->second.metres || (e.second.metres == best->second.metres && e.second.courses > best->second.courses)) best = &e;
    }
    return best ? best->first : "—";
}

void Profil::apresChargement(long long maintenant) {
    if (arrivee <= 0) arrivee = maintenant;
    dernieres.erase(std::remove_if(dernieres.begin(), dernieres.end(), [](const CourseJouee& x) { return x.voiture.empty(); }), dernieres.end());
    if (cree && plaque.empty()) cree = false;
}

std::string Profil::pseudoPropose(const std::string& nick) { return nick == "Pilote" ? std::string() : nick; }

bool Profil::creer(const std::string& pseudo, const std::string& plaq, std::string& nickSortie) {
    std::string ps, pl;
    if (profil::verifierPseudo(pseudo, &ps) != profil::Refus::Aucun || profil::verifierPlaque(plaq, &pl) != profil::Refus::Aucun) return false;
    nickSortie = ps; plaque = pl; cree = true;
    return true;
}

std::string Profil::texte(const std::string& nick) const {
    std::ostringstream o;
    char km1[32];
    std::snprintf(km1, sizeof km1, "%.1f", km());
    o << "pseudo=" << nick << " plaque=" << (plaque.empty() ? "—" : plaque) << " profil=" << (cree ? 1 : 0)
      << " arrivée=" << (arrivee > 0 ? dateUtc(arrivee) : "—") << " | courses=" << courses << " km=" << km1 << " vmax=" << vmax
      << " km/h frôlements=" << frolements << " victoires=" << victoires << " voiture préférée=" << voiturePreferee() << " | "
      << dernieres.size() << " dernière(s) course(s)";
    for (size_t i = 0; i < dernieres.size(); i++) {
        const CourseJouee& c = dernieres[i];
        o << (i ? " ; " : " : ") << nomModeJoue(c.mode) << ", " << c.voiture << ", " << c.resultat() << ", " << dateUtc(c.date);
    }
    return o.str();
}

} // namespace wfs
