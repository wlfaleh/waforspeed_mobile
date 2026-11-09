// WaForSpeed — cartes 100 % dynamiques (WFS-343) : lecture des blocs texte, registre, découverte des paquets. Voir carte_donnees.h.
#include "carte_donnees.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include "../../core/log.h"
#include "../../game/chocs_decor.h"

namespace wfs {
namespace cartes_v2 {
namespace {

constexpr int64_t SEGMENTS_MIN = 300, SEGMENTS_MAX = 200000;   // 360 m à 240 km
constexpr size_t OBJETS_MAX = 50000;
constexpr float LAT_MAX = 400, TAILLE_MAX = 300;

using Mots = std::vector<std::string>;

// lignes non vides sans commentaire, découpées en mots (guillemets non gérés : un texte libre est la fin de la ligne)
std::vector<std::pair<int, Mots>> lignes(const std::string& texte, std::vector<std::string>* brutes = nullptr) {
    std::vector<std::pair<int, Mots>> v;
    std::istringstream in(texte);
    std::string l; int n = 0;
    while (std::getline(in, l)) {
        n++;
        if (!l.empty() && l.back() == '\r') l.pop_back();
        const size_t h = l.find('#');
        if (h != std::string::npos) l.resize(h);
        std::istringstream m(l); Mots w; std::string x;
        while (m >> x) w.push_back(x);
        if (w.empty()) continue;
        v.push_back({n, w});
        if (brutes) brutes->push_back(l);
    }
    return v;
}
bool nombre(const std::string& s, double& v) {
    char* f = nullptr;
    v = std::strtod(s.c_str(), &f);
    return f && *f == 0 && !s.empty() && std::isfinite(v);
}
bool entier(const std::string& s, int64_t& v) { double d; if (!nombre(s, d) || d != std::floor(d)) return false; v = (int64_t)d; return true; }
bool couleur(const std::string& s, uint32_t& c) {
    std::string h = s;
    if (h.rfind("0x", 0) == 0 || h.rfind("0X", 0) == 0) h = h.substr(2); else if (!h.empty() && h[0] == '#') h = h.substr(1);
    if (h.size() != 6) return false;
    char* f = nullptr; const unsigned long v = std::strtoul(h.c_str(), &f, 16);
    if (!f || *f) return false;
    c = (uint32_t)v; return true;
}
// texte libre après le mot-clé (nom, description) : reprend la ligne brute
std::string reste(const std::string& brute, const std::string& cle) {
    size_t p = brute.find(cle);
    if (p == std::string::npos) return {};
    p += cle.size();
    while (p < brute.size() && (brute[p] == ' ' || brute[p] == '\t')) p++;
    std::string r = brute.substr(p);
    while (!r.empty() && (r.back() == ' ' || r.back() == '\t')) r.pop_back();
    return r;
}
std::string ligneErr(int n, const std::string& quoi) { return "ligne " + std::to_string(n) + " : " + quoi; }

bool format(const std::vector<std::pair<int, Mots>>& L, std::string& e, const char* bloc) {
    if (L.empty() || L[0].second[0] != "format" || L[0].second.size() != 2) { e = std::string("bloc ") + bloc + " : « format N » attendu en 1re ligne"; return false; }
    int64_t f = 0;
    if (!entier(L[0].second[1], f) || f < 1) { e = std::string("bloc ") + bloc + " : format illisible"; return false; }
    if (f > FORMAT_DONNEES) { e = std::string("bloc ") + bloc + " : format " + std::to_string(f) + " plus récent que le jeu (" + std::to_string(FORMAT_DONNEES) + ")"; return false; }
    return true;
}

bool typeSection(const std::string& s, trace::Type& t) {
    static const std::pair<const char*, trace::Type> T[] = {{"ligne", trace::Type::Ligne}, {"virage", trace::Type::Virage}, {"epingle", trace::Type::Epingle},
        {"lacets", trace::Type::Lacets}, {"montee", trace::Type::Montee}, {"descente", trace::Type::Descente}, {"autoroute", trace::Type::Autoroute},
        {"tunnel", trace::Type::Tunnel}, {"pont", trace::Type::Pont}, {"chicane", trace::Type::Chicane}};   // pas de rond-point (WFS-283)
    for (const auto& [n, v] : T) if (s == n) { t = v; return true; }
    return false;
}
bool genre(const std::string& s, int& g) {
    static const std::pair<const char*, chocs::Genre> G[] = {{"poteau", chocs::Genre::Poteau}, {"arbre", chocs::Genre::Arbre}, {"mobilier", chocs::Genre::Mobilier},
        {"mur", chocs::Genre::Mur}, {"batiment", chocs::Genre::Batiment}, {"autre", chocs::Genre::Autre}};
    for (const auto& [n, v] : G) if (s == n) { g = (int)v; return true; }
    return false;
}

// « repete pas de a commande … » : la commande est répétée, son 1er argument (segment) remplacé par de, de + pas, … (< a)
bool deplier(const std::vector<std::pair<int, Mots>>& L, std::vector<std::pair<int, Mots>>& out, std::string& e) {
    for (const auto& [n, w] : L) {
        if (w[0] != "repete") { out.push_back({n, w}); continue; }
        int64_t pas = 0, de = 0, a = 0;
        if (w.size() < 6 || !entier(w[1], pas) || !entier(w[2], de) || !entier(w[3], a) || pas < 1 || a < de) { e = ligneErr(n, "« repete pas de a commande segment … » attendu"); return false; }
        if ((a - de) / pas > (int64_t)OBJETS_MAX) { e = ligneErr(n, "trop de répétitions"); return false; }
        for (int64_t s = de; s < a; s += pas) { Mots m(w.begin() + 4, w.end()); m[1] = std::to_string(s); out.push_back({n, m}); }
    }
    return true;
}

} // namespace

int64_t DonneesCarte::segments() const {
    int64_t n = 0;
    for (const Route& r : routes) n += r.entree + r.tenue + r.sortie;
    return n;
}

bool lireTrace(const std::string& texte, DonneesCarte& d, std::string& e) {
    std::vector<std::string> brutes;
    const auto L = lignes(texte, &brutes);
    if (!format(L, e, "trace")) return false;
    d.routes.clear(); d.sections.clear(); d.quartiers.clear();
    for (size_t i = 1; i < L.size(); i++) {
        const auto& [n, w] = L[i];
        const std::string& k = w[0];
        double x[6] = {};
        auto nums = [&](size_t a, size_t b) { if (w.size() < a + 1 || w.size() > b + 1) return false; for (size_t j = 1; j < w.size(); j++) if (!nombre(w[j], x[j - 1])) return false; return true; };
        if (k == "nom") { d.nom = reste(brutes[i], "nom"); if (d.nom.empty()) { e = ligneErr(n, "nom vide"); return false; } }
        else if (k == "description") d.description = reste(brutes[i], "description");
        else if (k == "quartiers") {
            std::string q = reste(brutes[i], "quartiers"), x0; std::istringstream s(q);
            while (std::getline(s, x0, ';')) { while (!x0.empty() && x0[0] == ' ') x0.erase(0, 1); while (!x0.empty() && x0.back() == ' ') x0.pop_back(); if (!x0.empty()) d.quartiers.push_back(x0); }
        }
        else if (k == "cache") { if (!nums(1, 1)) { e = ligneErr(n, "« cache 0|1 » attendu"); return false; } d.cache = x[0] != 0; }
        else if (k == "graine") { if (!nums(1, 1) || x[0] < 0 || x[0] > 4294967295.0) { e = ligneErr(n, "graine illisible"); return false; } d.graine = (uint32_t)x[0]; }
        else if (k == "bord") { if (!nums(1, 1) || x[0] < 0.5 || x[0] > 4) { e = ligneErr(n, "bord hors de [0,5 ; 4]"); return false; } d.bord = x[0]; }
        else if (k == "checkpoint_m") { if (!nums(1, 1) || x[0] < 100 || x[0] > 5000) { e = ligneErr(n, "checkpoint_m hors de [100 ; 5000]"); return false; } d.checkpointM = x[0]; }
        else if (k == "trafic") {
            if (!nums(3, 3) || x[0] < 0 || x[0] > 200 || x[1] < 0.05 || x[2] > 1.2 || x[1] > x[2]) { e = ligneErr(n, "« trafic nombre vitesse_min vitesse_max » attendu (0 à 200, 0,05 ≤ min ≤ max ≤ 1,2)"); return false; }
            d.traficN = (int)x[0]; d.traficMin = x[1]; d.traficMax = x[2];
        }
        else if (k == "vignette") {
            if (w.size() != 4 || !couleur(w[1], d.vignetteNuit) || !couleur(w[2], d.vignetteJour) || !couleur(w[3], d.accent)) { e = ligneErr(n, "« vignette sol_nuit sol_jour accent » attendu (0xRRGGBB)"); return false; }
        }
        else if (k == "route") {
            if (!nums(5, 5)) { e = ligneErr(n, "« route entree tenue sortie courbe montee » attendu"); return false; }
            for (int j = 0; j < 3; j++) if (x[j] < 0 || x[j] > 2000 || x[j] != std::floor(x[j])) { e = ligneErr(n, "longueurs de route entières de 0 à 2000 segments"); return false; }
            if (std::fabs(x[3]) > 8 || std::fabs(x[4]) > 400) { e = ligneErr(n, "courbe (|c| ≤ 8) ou montée (|m| ≤ 400) hors limites"); return false; }
            if (x[0] + x[1] + x[2] < 1) { e = ligneErr(n, "route vide"); return false; }
            d.routes.push_back({(int)x[0], (int)x[1], (int)x[2], x[3], x[4]});
        }
        else if (k == "section") {
            trace::Section s;
            if (w.size() < 4 || !typeSection(w[1], s.type)) { e = ligneErr(n, "« section type debut fin [cote rayon angle] » attendu (types : ligne virage epingle lacets montee descente autoroute tunnel pont chicane)"); return false; }
            int64_t a = 0, b = 0; double c = 0, r = 0, an = 0;
            if (!entier(w[2], a) || !entier(w[3], b) || (w.size() > 4 && !nombre(w[4], c)) || (w.size() > 5 && !nombre(w[5], r)) || (w.size() > 6 && !nombre(w[6], an)) || w.size() > 7) { e = ligneErr(n, "section illisible"); return false; }
            s.debut = a; s.fin = b; s.cote = c < 0 ? -1 : c > 0 ? 1 : 0; s.rayon = r; s.angle = an;
            s.autoroute = s.type == trace::Type::Autoroute;
            d.sections.push_back(s);
        }
        else { e = ligneErr(n, "instruction inconnue « " + k + " »"); return false; }
    }
    if (d.nom.empty()) { e = "bloc trace : nom manquant"; return false; }
    const int64_t S = d.segments();
    if (d.routes.empty() || S < SEGMENTS_MIN || S > SEGMENTS_MAX) { e = "bloc trace : tour de " + std::to_string(S) + " segments (attendu " + std::to_string(SEGMENTS_MIN) + " à " + std::to_string(SEGMENTS_MAX) + ")"; return false; }
    std::sort(d.sections.begin(), d.sections.end(), [](const trace::Section& a, const trace::Section& b) { return a.debut < b.debut; });
    for (size_t i = 0; i < d.sections.size(); i++) {
        const trace::Section& s = d.sections[i];
        if (s.debut < 0 || s.fin > S || s.debut >= s.fin) { e = "bloc trace : section " + std::string(trace::nomType(s.type)) + " [" + std::to_string(s.debut) + ", " + std::to_string(s.fin) + ") hors du tour"; return false; }
        if (i && s.debut < d.sections[i - 1].fin) { e = "bloc trace : sections qui se chevauchent au segment " + std::to_string(s.debut); return false; }
    }
    return true;
}

bool lireChocs(const std::string& texte, DonneesCarte& d, std::string& e) {
    const auto L0 = lignes(texte);
    if (!format(L0, e, "chocs")) return false;
    std::vector<std::pair<int, Mots>> L;
    if (!deplier({L0.begin() + 1, L0.end()}, L, e)) { e = "bloc chocs : " + e; return false; }
    const int64_t S = d.segments();
    d.obstacles.clear();
    for (const auto& [n, w] : L) {
        Objet o; double x[10] = {}; int64_t seg = 0;
        const bool boite = w[0] == "boite", cyl = w[0] == "cylindre";
        // boite seg along lat demi_travers demi_long angle couleur hauteur genre ; cylindre seg along lat rayon couleur hauteur genre
        const size_t N = boite ? 10 : 8;
        if ((!boite && !cyl) || w.size() != N) { e = "bloc chocs : " + ligneErr(n, "« boite seg along lat demi_travers demi_long angle couleur hauteur genre » ou « cylindre seg along lat rayon couleur hauteur genre » attendu"); return false; }
        const size_t iC = boite ? 7 : 5;
        bool ok = entier(w[1], seg) && couleur(w[iC], o.couleur) && genre(w[N - 1], o.genre);
        for (size_t j = 2; ok && j < N - 1; j++) if (j != iC) ok = nombre(w[j], x[j]);
        if (!ok) { e = "bloc chocs : " + ligneErr(n, "valeur illisible"); return false; }
        o.type = boite ? Objet::Boite : Objet::Cylindre; o.seg = seg; o.along = (float)x[2]; o.lat = (float)x[3];
        if (boite) { o.largeur = (float)x[4] * 2; o.profondeur = (float)x[5] * 2; o.angle = (float)x[6]; o.hauteur = (float)x[8]; }
        else { o.rayon = (float)x[4]; o.hauteur = (float)x[6]; }
        const float t = boite ? std::min(o.largeur, o.profondeur) : o.rayon;
        if (seg < 0 || seg >= S || std::fabs(o.lat) > LAT_MAX || std::fabs(o.along) > 50 || t <= 0 || std::max({o.largeur, o.profondeur, o.rayon, o.hauteur}) > TAILLE_MAX || o.hauteur <= 0) {
            e = "bloc chocs : " + ligneErr(n, "obstacle hors du tour ou de taille impossible"); return false; }
        const double an = o.angle * 3.14159265358979 / 180;   // demi-étendue en travers de la route (boîte tournée de son angle)
        const double travers = boite ? std::fabs(o.largeur / 2 * std::cos(an)) + std::fabs(o.profondeur / 2 * std::sin(an)) : o.rayon;
        if (std::fabs(o.lat) - travers < RW) { e = "bloc chocs : " + ligneErr(n, "obstacle sur la chaussée (|lat| − taille < 7,5 m)"); return false; }
        d.obstacles.push_back(o);
        if (d.obstacles.size() > OBJETS_MAX) { e = "bloc chocs : trop d'obstacles"; return false; }
    }
    return true;
}

bool lireDecor(const std::string& texte, DonneesCarte& d, std::string& e) {
    const auto L0 = lignes(texte);
    if (!format(L0, e, "decor")) return false;
    std::vector<std::pair<int, Mots>> L;
    if (!deplier({L0.begin() + 1, L0.end()}, L, e)) { e = "bloc decor : " + e; return false; }
    const int64_t S = d.segments();
    d.objets.clear(); d.sol.clear(); d.herbe = true;
    auto err = [&](int n, const std::string& q) { e = "bloc decor : " + ligneErr(n, q); return false; };
    for (const auto& [n, w] : L) {
        const std::string& k = w[0];
        if (k == "ciel") {   // ciel brouillard_jour lointain_jour horizon_nuit brouillard_nuit lointain_nuit
            double a = 0, b = 0;
            if (w.size() != 6 || !couleur(w[1], d.brouillardJour) || !nombre(w[2], a) || !couleur(w[3], d.horizonNuit) || !couleur(w[4], d.brouillardNuit) || !nombre(w[5], b) || a < 50 || b < 50 || a > 5000 || b > 5000)
                return err(n, "« ciel brouillard_jour lointain_jour horizon_nuit brouillard_nuit lointain_nuit » attendu (50 à 5000 m)");
            d.lointainJour = (float)a; d.lointainNuit = (float)b;
        } else if (k == "herbe") {
            if (w.size() != 2 || (w[1] != "0" && w[1] != "1")) return err(n, "« herbe 0|1 » attendu");
            d.herbe = w[1] == "1";
        } else if (k == "bande") {   // bande a b ya yb couleur : sol de chaque côté, m depuis l'axe
            Bande bd; double x[4] = {};
            if (w.size() != 6 || !couleur(w[5], bd.couleur)) return err(n, "« bande a b ya yb couleur » attendu");
            for (int j = 0; j < 4; j++) if (!nombre(w[1 + j], x[j])) return err(n, "bande illisible");
            bd.a = (float)x[0]; bd.b = (float)x[1]; bd.ya = (float)x[2]; bd.yb = (float)x[3];
            if (bd.a < RW - 0.01 || bd.b <= bd.a || bd.b > 2000 || std::fabs(bd.ya) > 100 || std::fabs(bd.yb) > 100) return err(n, "bande hors limites (a ≥ 7,5 m, a < b ≤ 2000)");
            if (!d.sol.empty() && bd.a < d.sol.back().b - 0.01f) return err(n, "bandes non rangées de l'axe vers l'extérieur");
            if (d.sol.size() >= 6) return err(n, "6 bandes au plus");
            d.sol.push_back(bd);
        } else {
            Objet o; int64_t seg = 0; double x[8] = {};
            auto lire = [&](size_t N, std::initializer_list<size_t> couleurs) {
                if (w.size() != N || !entier(w[1], seg)) return false;
                for (size_t j = 2; j < N; j++) {
                    if (std::find(couleurs.begin(), couleurs.end(), j) != couleurs.end()) { if (!couleur(w[j], j == *couleurs.begin() ? o.couleur : o.couleur2)) return false; }
                    else if (!nombre(w[j], x[j])) return false;
                }
                return true;
            };
            if (k == "maison") {   // maison seg lat angle largeur profondeur hauteur couleur_murs couleur_toit
                if (!lire(9, {7, 8})) return err(n, "« maison seg lat angle largeur profondeur hauteur couleur_murs couleur_toit » attendu");
                o.type = Objet::Maison; o.lat = (float)x[2]; o.angle = (float)x[3]; o.largeur = (float)x[4]; o.profondeur = (float)x[5]; o.hauteur = (float)x[6];
                if (std::fabs(o.lat) - o.profondeur / 2 < RW + 1) return err(n, "maison sur la chaussée ou le trottoir");
            } else if (k == "sapin" || k == "feuillu" || k == "cypres") {   // sapin seg lat hauteur
                if (!lire(4, {})) return err(n, "« " + k + " seg lat hauteur » attendu");
                o.type = k == "sapin" ? Objet::Sapin : k == "feuillu" ? Objet::Feuillu : Objet::Cypres; o.lat = (float)x[2]; o.hauteur = (float)x[3];
                if (std::fabs(o.lat) < RW + 1.5f) return err(n, "arbre sur la chaussée");
            } else if (k == "lampe") {   // lampe seg cote (lampadaire de rue)
                if (!lire(3, {}) || (x[2] != 1 && x[2] != -1)) return err(n, "« lampe seg cote(±1) » attendu");
                o.type = Objet::Lampe; o.lat = (float)x[2] * (float)(RW + 0.8); o.hauteur = 7.6f;
            } else if (k == "lumiere") {   // lumiere seg lat hauteur rayon couleur (émise)
                if (!lire(6, {5})) return err(n, "« lumiere seg lat hauteur rayon couleur » attendu");
                o.type = Objet::Lumiere; o.lat = (float)x[2]; o.hauteur = (float)x[3]; o.rayon = (float)x[4];
                if (o.rayon <= 0 || o.rayon > 2 || o.hauteur < 0) return err(n, "lumière de rayon 0 à 2 m");
            } else if (k == "guirlande") {   // guirlande seg_debut seg_fin lat hauteur pas_seg couleur : une lumière tous les pas segments
                int64_t fin = 0;
                if (w.size() != 7 || !entier(w[1], seg) || !entier(w[2], fin) || !nombre(w[3], x[3]) || !nombre(w[4], x[4]) || !nombre(w[5], x[5]) || !couleur(w[6], o.couleur) || x[5] < 1 || fin <= seg || fin > S)
                    return err(n, "« guirlande seg_debut seg_fin lat hauteur pas_seg couleur » attendu");
                if (std::fabs(x[3]) < RW + 0.5) return err(n, "guirlande sur la chaussée");
                for (int64_t s = seg; s < fin; s += (int64_t)x[5]) {
                    Objet l; l.type = Objet::Lumiere; l.seg = s; l.lat = (float)x[3]; l.hauteur = (float)x[4]; l.rayon = 0.09f; l.couleur = o.couleur;
                    d.objets.push_back(l);
                }
                if (d.objets.size() > OBJETS_MAX) return err(n, "trop d'objets");
                continue;
            } else return err(n, "instruction inconnue « " + k + " »");
            o.seg = seg;
            if (seg < 0 || seg >= S) return err(n, "segment hors du tour (0 à " + std::to_string(S - 1) + ")");
            if (std::fabs(o.lat) > LAT_MAX || o.hauteur < 0 || std::max({o.largeur, o.profondeur, o.hauteur}) > TAILLE_MAX || (o.type == Objet::Maison && (o.largeur <= 0 || o.profondeur <= 0 || o.hauteur <= 0)))
                return err(n, "objet hors limites");
            d.objets.push_back(o);
            if (d.objets.size() > OBJETS_MAX) return err(n, "trop d'objets");
        }
    }
    return true;
}

// ---------------------------------------------------------------- registre
namespace {
struct Entree { DonneesCarte d; MapDef def{}; };
std::mutex g_m;
std::deque<Entree>& registre() { static std::deque<Entree> r; return r; }
Entree* trouver(const std::string& id) { for (Entree& e : registre()) if (e.d.id == id) return &e; return nullptr; }

// Track::build d'une carte de données : routes du jeu web (addRoad) puis sections typées
void construire(Track& t, const MapDef& m) {
    std::vector<Route> routes; std::vector<trace::Section> sections;
    { std::lock_guard<std::mutex> v(g_m); if (const Entree* e = trouver(m.id)) { routes = e->d.routes; sections = e->d.sections; } }
    for (const Route& r : routes) t.ajouterRoute(r.entree, r.tenue, r.sortie, r.courbe, r.montee);
    t.poserSections(std::move(sections));
}
const MapDef* chercher(const char* id) {
    if (!id) return nullptr;
    std::lock_guard<std::mutex> v(g_m);
    const Entree* e = trouver(id);
    return e ? &e->def : nullptr;
}
bool carteDuCode(const std::string& id) {
    const MapDef* (*f)(const char*) = g_cartesDonnees;
    g_cartesDonnees = nullptr;   // MAPS seules
    const bool code = id == mapById(id.c_str()).id;
    g_cartesDonnees = f;
    return code;
}
} // namespace

bool enregistrer(DonneesCarte d, std::string& e) {
    if (d.id.empty() || d.id.size() > 40 || d.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") != std::string::npos) { e = "identifiant de carte invalide « " + d.id + " » (a-z, 0-9, -)"; return false; }
    if (carteDuCode(d.id)) { e = "« " + d.id + " » est une carte du code : jamais remplacée par un paquet"; return false; }
    std::lock_guard<std::mutex> v(g_m);
    Entree* x = trouver(d.id);
    if (!x) { registre().push_back({}); x = &registre().back(); }
    x->d = std::move(d);
    MapDef& m = x->def;
    m.id = x->d.id.c_str(); m.name = x->d.nom.c_str();
    m.seed = x->d.graine; m.curves = {1}; m.hills = 0; m.sections = 0; m.edge = x->d.bord;
    m.trafficN = x->d.traficN; m.trafficMin = x->d.traficMin; m.trafficMax = x->d.traficMax;
    m.construire = &construire;
    g_cartesDonnees = &chercher;
    WFS_LOG("cartes de données (WFS-343) : %s « %s » enregistrée (%lld segments, %zu sections, %zu obstacles, %zu objets)%s", m.id, m.name,
            (long long)x->d.segments(), x->d.sections.size(), x->d.obstacles.size(), x->d.objets.size(), x->d.cache ? ", carte de test cachée" : "");
    return true;
}
const DonneesCarte* donnees(const std::string& id) { std::lock_guard<std::mutex> v(g_m); const Entree* e = trouver(id); return e ? &e->d : nullptr; }
std::vector<std::string> cartesDonnees(bool cachees) {
    std::lock_guard<std::mutex> v(g_m);
    std::vector<std::string> r;
    for (const Entree& e : registre()) if (cachees || !e.d.cache) r.push_back(e.d.id);
    return r;
}
void oublier() { std::lock_guard<std::mutex> v(g_m); registre().clear(); g_cartesDonnees = nullptr; }

uint64_t empreinteDonnees(const std::string& id) { return carte::empreinte(id, 0, 100 + FORMAT_DONNEES, {"donnees"}); }

namespace {
std::string texteBloc(const carte::Paquet& p, const char* nom) { const carte::Bloc* b = p.bloc(nom); return b ? std::string(b->donnees.begin(), b->donnees.end()) : std::string(); }
bool lireBlocs(const std::string& id, const std::string& tr, const std::string& ch, const std::string& de, DonneesCarte& d, std::string& e) {
    d = {}; d.id = id;
    return lireTrace(tr, d, e) && lireChocs(ch, d, e) && lireDecor(de, d, e);
}
} // namespace

int decouvrir() {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::vector<std::string> ids;
    for (const fs::directory_entry& f : fs::directory_iterator(carte::dossierPaquets(), ec)) {
        const std::string n = f.path().filename().string();
        const std::string suf = ".wfcarte";
        if (n.size() <= suf.size() || n.compare(n.size() - suf.size(), suf.size(), suf) != 0) continue;   // « .part » exclu
        const size_t u = n.rfind('_');
        if (u == std::string::npos) continue;
        const std::string id = n.substr(0, u);
        if (!carteDuCode(id) && std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    int nb = 0;
    for (const std::string& id : ids) {
        std::string raison;
        bool ok = false;
        for (carte::FormatTexture f : carte::FORMATS) {   // format préféré d'abord (les blocs de données sont les mêmes)
            const std::string chemin = carte::dossierPaquets() + "/" + carte::nomFichier(id, f);
            carte::Paquet p;
            const carte::Verdict v = carte::verifier_carte(chemin, {id, f, empreinteDonnees(id)}, &p);
            if (v == carte::Verdict::Absent) continue;
            if (v != carte::Verdict::Bon) { raison = carte::nomFichier(id, f) + " : " + carte::texte(v); continue; }
            DonneesCarte d;
            if (!lireBlocs(id, texteBloc(p, "trace"), texteBloc(p, "chocs"), texteBloc(p, "decor"), d, raison)) { raison = carte::nomFichier(id, f) + " : " + raison; continue; }
            ok = enregistrer(std::move(d), raison);
            break;
        }
        if (ok) nb++;
        else WFS_ERR("cartes de données (WFS-343) : %s refusée, absente du choix du terrain — %s", id.c_str(), raison.c_str());
    }
    return nb;
}

bool sourcesCarte(const std::string& racine, const std::string& id, carte::FormatTexture f, carte::Paquet& p, std::string& e) {
    const std::string dir = racine + "/../paquets/sources/" + id + "/";
    std::string t[3];
    const char* N[3] = {"trace", "chocs", "decor"};
    for (int i = 0; i < 3; i++) {
        std::ifstream in(dir + N[i] + ".txt", std::ios::binary);
        if (!in) { e = "source absente : " + dir + N[i] + ".txt"; return false; }
        t[i].assign(std::istreambuf_iterator<char>(in), {});
    }
    DonneesCarte d;
    if (!lireBlocs(id, t[0], t[1], t[2], d, e)) return false;
    p = {}; p.carte = id; p.format = f; p.empreinte = empreinteDonnees(id);
    p.blocs.push_back({"decor_regroupe", {}});
    p.blocs.push_back({"maillages", carte::archive({})});
    p.blocs.push_back({"textures", carte::archive({})});
    for (int i = 0; i < 3; i++) p.blocs.push_back({N[i], std::vector<uint8_t>(t[i].begin(), t[i].end())});
    return true;
}

} // namespace cartes_v2
} // namespace wfs
