// WaForSpeed — cadeau du jour (WFS-500) : voir cadeau.h.
#include "cadeau.h"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <random>
#include "../core/log.h"
#include "boutique.h"
#include "personnalisation.h"

namespace wfs::cadeau {

namespace {
constexpr long long JOUR_S = 86400;

// calendrier grégorien proleptique (algorithmes « days_from_civil » / « civil_from_days » de H. Hinnant, domaine public)
long long joursDepuisCivil(long long y, int m, int d) {
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const long long yoe = y - era * 400;
    const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
void civilDepuisJours(long long z, long long& y, int& m, int& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const long long doe = z - era * 146097;
    const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long long mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    y = yoe + era * 400 + (m <= 2);
}
long long divBas(long long a, long long b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }   // division arrondie vers −∞

int uniforme(Mulberry32& h, int n) { return n <= 1 ? 0 : std::min((int)(h.next() * n), n - 1); }   // entier dans [0, n)

// RG-04 : tranche d'étoiles tirée avec ses seuls poids (renormalisés : on tire dans leur somme) ; tous nuls : la première
int tirerTranche(const ConfigJeu::Cadeau& c, Mulberry32& h) {
    double total = 0;
    for (const auto& t : c.tranche) total += std::max(0.0, t.chance);
    if (total <= 0) return 0;
    double r = h.next() * total;
    for (int i = 0; i < CADEAU_TRANCHES; i++) {
        r -= std::max(0.0, c.tranche[i].chance);
        if (r < 0) return i;
    }
    return CADEAU_TRANCHES - 1;   // arrondi du double
}
// RG-02 : entier uniforme dans la tranche, jamais moins de etoiles_min
Resultat etoilesDe(const ConfigJeu::Cadeau& c, int tranche, Mulberry32& h) {
    const auto& t = c.tranche[tranche];
    const int lo = std::min(t.min, t.max), hi = std::max(t.min, t.max);
    Resultat r;
    r.gain = Gain::Etoiles;
    r.etoiles = std::max(c.etoilesMin, lo + uniforme(h, hi - lo + 1));
    return r;
}
Resultat repli(const ConfigJeu::Cadeau& c, Mulberry32& h) {
    Resultat r = etoilesDe(c, tirerTranche(c, h), h);
    r.repli = true;
    return r;
}
// RG-03 : voiture du joueur au hasard parmi celles qui ont encore un objet à gagner (coloris ou skin), puis l'objet au hasard
Resultat objetDe(const Entree& e, bool skin, Mulberry32& h) {
    std::vector<const VoitureJoueur*> v;
    for (const VoitureJoueur& j : e.voitures) if (!(skin ? j.skins : j.coloris).empty()) v.push_back(&j);
    const VoitureJoueur& j = *v[(size_t)uniforme(h, (int)v.size())];
    const auto& liste = skin ? j.skins : j.coloris;
    Resultat r;
    r.gain = skin ? Gain::Skin : Gain::Coloris;
    r.voiture = j.id;
    r.objet = liste[(size_t)uniforme(h, (int)liste.size())];
    return r;
}
Resultat voitureDe(const Entree& e, Mulberry32& h) {
    Resultat r;
    r.gain = Gain::Voiture;
    r.voiture = e.voituresPretables[(size_t)uniforme(h, (int)e.voituresPretables.size())];
    return r;
}
bool aColoris(const Entree& e) { return std::any_of(e.voitures.begin(), e.voitures.end(), [](const VoitureJoueur& j) { return !j.coloris.empty(); }); }
bool aSkin(const Entree& e) { return std::any_of(e.voitures.begin(), e.voitures.end(), [](const VoitureJoueur& j) { return !j.skins.empty(); }); }
bool idSur(const std::string& s) {   // identifiant de voiture ou d'objet : a-z, 0-9, « - »
    if (s.empty() || s.size() > 32) return false;
    for (char ch : s) if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-')) return false;
    return true;
}
} // namespace

// ---------------------------------------------------------------- heure
Instant maintenant() {
    Instant t;
    const std::time_t e = std::time(nullptr);
    t.epoch = (long long)e;
    std::tm l{};
#if defined(_WIN32)
    localtime_s(&l, &e);
#else
    localtime_r(&e, &l);
#endif
    // décalage = heure locale lue comme si elle était UTC − heure UTC (portable, sans tm_gmtoff)
    const long long local = joursDepuisCivil(l.tm_year + 1900LL, l.tm_mon + 1, l.tm_mday) * JOUR_S + l.tm_hour * 3600LL + l.tm_min * 60LL + l.tm_sec;
    t.decalage = (int)std::clamp(local - t.epoch, -18 * 3600LL, 18 * 3600LL);
    return t;
}

long long jourLocal(const Instant& t) { return divBas(t.epoch + t.decalage, JOUR_S); }

std::string texteJour(long long jour) {
    long long y; int m, d;
    civilDepuisJours(jour, y, m, d);
    char b[32];
    std::snprintf(b, sizeof b, "%04lld-%02d-%02d", y, m, d);
    return b;
}

bool lireJour(const std::string& s, long long& jour) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) if (s[(size_t)i] < '0' || s[(size_t)i] > '9') return false;
    const int y = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    jour = joursDepuisCivil(y, m, d);
    return texteJour(jour) == s;   // refuse le 31 avril, le 29 février d'une année non bissextile…
}

// ---------------------------------------------------------------- disponibilité (RG-06, RG-07)
const char* texteEtat(Etat e) {
    switch (e) {
        case Etat::Disponible: return "disponible";
        case Etat::DejaOuvert: return "déjà ouvert aujourd'hui";
        default: return "heure du téléphone reculée : attendre l'heure du dernier cadeau";
    }
}

Etat etat(const EtatCadeau& c, const Instant& t) {
    if (c.heure <= 0 && c.jour.empty()) return Etat::Disponible;      // jamais ouvert (1er lancement, ancienne sauvegarde)
    if (t.epoch < c.heure) return Etat::HeureReculee;                 // RG-07 : bloquant quel que soit le fuseau
    long long dernier = 0;
    if (!lireJour(c.jour, dernier)) dernier = jourLocal({c.heure, c.decalage});
    return jourLocal(t) > dernier ? Etat::Disponible : Etat::DejaOuvert;   // RG-06 : un seul, sans accumulation
}

// ---------------------------------------------------------------- entrée (RG-03)
Entree entreeDepuis(const Sauvegarde& s) {
    Entree e;
    for (const char* idc : VOITURES_IDS) {
        const std::string id = idc;
        if (!boutique::voitureVendue(id)) continue;   // voitures retirées (licence NC) : jamais dans un cadeau
        if (boutique::possedeVoiture(s, id)) {
            VoitureJoueur j;
            j.id = id;
            for (const perso::Objet& o : perso::catalogue()) {
                if (!perso::disponible(o, id) || perso::possede(s, o, id)) continue;
                if (o.type == perso::Type::Couleur || o.type == perso::Type::Finition) j.coloris.push_back(o.id);
                else if (o.type == perso::Type::Skin) j.skins.push_back(o.id);
            }
            e.voitures.push_back(std::move(j));
        } else if (id != s.cadeau.pretVoiture) {
            e.voituresPretables.push_back(id);   // jamais une voiture déjà possédée ni déjà prêtée
        }
    }
    return e;
}

// ---------------------------------------------------------------- résultat
const char* idGain(Gain g) {
    switch (g) {
        case Gain::Etoiles: return "etoiles";
        case Gain::Coloris: return "coloris";
        case Gain::Skin: return "skin";
        default: return "voiture";
    }
}

std::string Resultat::texte() const {
    const std::string t = idGain(gain);
    if (gain == Gain::Etoiles) return t + ";" + std::to_string(etoiles);
    if (gain == Gain::Voiture) return t + ";" + voiture;
    return t + ";" + voiture + ":" + objet;
}

bool lireResultat(const std::string& s, Resultat& r) {
    const size_t p = s.find(';');
    if (p == std::string::npos) return false;
    const std::string t = s.substr(0, p), v = s.substr(p + 1);
    Resultat x;
    if (t == "etoiles") {
        if (v.empty() || v.size() > 9 || v.find_first_not_of("0123456789") != std::string::npos) return false;
        x.gain = Gain::Etoiles; x.etoiles = std::stoi(v);
    } else if (t == "voiture") {
        if (!idSur(v)) return false;
        x.gain = Gain::Voiture; x.voiture = v;
    } else if (t == "coloris" || t == "skin") {
        const size_t q = v.find(':');
        if (q == std::string::npos || !idSur(v.substr(0, q)) || !idSur(v.substr(q + 1))) return false;
        x.gain = t == "skin" ? Gain::Skin : Gain::Coloris; x.voiture = v.substr(0, q); x.objet = v.substr(q + 1);
    } else return false;
    r = x;
    return true;
}

// ---------------------------------------------------------------- tirage (RG-01 à RG-05)
Resultat tirer(const ConfigJeu::Cadeau& c, const Entree& e, int& sansRare, Mulberry32& h) {
    const bool skinPossible = aSkin(e), voiturePossible = !e.voituresPretables.empty();
    Resultat r;
    if (sansRare + 1 >= c.garantie) {   // RG-05 : ce cadeau est le 30e de suite sans voiture ni skin
        if (!skinPossible && !voiturePossible) {   // tout possédé : étoiles, compteur remis à 0
            r = repli(c, h); r.garantie = true;
            sansRare = 0;
            return r;
        }
        bool skin = skinPossible;
        if (skinPossible && voiturePossible) {   // poids 4 (skin) contre 3 (voiture) ; tous deux nuls : une chance sur deux
            const double ws = std::max(0.0, c.chanceSkin), wv = std::max(0.0, c.chanceVoiture);
            skin = ws + wv > 0 ? h.next() * (ws + wv) < ws : h.next() < 0.5;
        }
        r = skin ? objetDe(e, true, h) : voitureDe(e, h);
        r.garantie = true;
        sansRare = 0;
        return r;
    }
    // RG-01 : 4 tranches d'étoiles, coloris, skin, voiture
    double poids[CADEAU_TRANCHES + 3], total = 0;
    for (int i = 0; i < CADEAU_TRANCHES; i++) poids[i] = std::max(0.0, c.tranche[i].chance);
    poids[CADEAU_TRANCHES] = std::max(0.0, c.chanceColoris);
    poids[CADEAU_TRANCHES + 1] = std::max(0.0, c.chanceSkin);
    poids[CADEAU_TRANCHES + 2] = std::max(0.0, c.chanceVoiture);
    for (double p : poids) total += p;
    int k = 0;
    if (total > 0) {
        double x = h.next() * total;
        for (k = 0; k < CADEAU_TRANCHES + 2; k++) { x -= poids[k]; if (x < 0) break; }   // k = CADEAU_TRANCHES + 2 si rien avant
    }
    if (k < CADEAU_TRANCHES) r = etoilesDe(c, k, h);
    else if (k == CADEAU_TRANCHES) r = aColoris(e) ? objetDe(e, false, h) : repli(c, h);           // RG-04 : catégorie vide
    else if (k == CADEAU_TRANCHES + 1) r = skinPossible ? objetDe(e, true, h) : repli(c, h);
    else r = voiturePossible ? voitureDe(e, h) : repli(c, h);
    if (r.gain == Gain::Voiture || r.gain == Gain::Skin) sansRare = 0;
    else sansRare = std::min(sansRare + 1, 100000);
    return r;
}

// ---------------------------------------------------------------- ouvrir (RG-08)
std::optional<Resultat> ouvrir(Sauvegarde& s, const Instant& t, const Entree& e, uint32_t graine) {
    const Etat et = etat(s.cadeau, t);
    if (et != Etat::Disponible) {
        WFS_LOG("cadeau du jour : pas de cadeau (%s)", texteEtat(et));
        return std::nullopt;
    }
    const EtatCadeau avant = s.cadeau;   // remis tel quel si la sauvegarde ne peut pas être écrite
    Mulberry32 h(graine);
    const Resultat r = tirer(config().cadeau, e, s.cadeau.sansRare, h);
    EtatCadeau& c = s.cadeau;
    c.heure = t.epoch;
    c.decalage = t.decalage;
    c.jour = texteJour(jourLocal(t));
    c.total = std::min(c.total + 1, 1000000000LL);
    c.dernier = r.texte();
    // RG-08 : écrit AVANT de rendre le gain : pas de 2e cadeau si le jeu est tué pendant l'animation. Écriture faite ici, sur le
    // thread principal, après les écritures en attente (attendre) : on sait si elle a réussi. Échec → rien n'est donné, état remis.
    if (s.actif() && !s.chemin().empty()) {
        s.attendre();
        if (!ecrireAtomique(s.chemin(), s.texte())) {
            s.cadeau = avant;
            WFS_ERR("cadeau du jour : sauvegarde impossible (%s) : cadeau non ouvert, il reste disponible", s.chemin().c_str());
            return std::nullopt;
        }
    }
    WFS_LOG("cadeau du jour ouvert (%s) : %s%s%s ; garantie %d/%d ; %lld cadeau(x) ouvert(s)", c.jour.c_str(), c.dernier.c_str(),
            r.garantie ? " (garantie)" : "", r.repli ? " (repli : catégorie toute possédée)" : "", c.sansRare, config().cadeau.garantie, c.total);
    return r;
}

std::optional<Resultat> ouvrir(Sauvegarde& s) {
    const Instant t = maintenant();
    std::random_device rd;   // RG-10 : graine tirée à l'ouverture, indépendante du nombre d'images
    const uint64_t e = (uint64_t)t.epoch * 2654435761ull;   // non signé sur 64 bits : aucun débordement (remarque du lead dev, PR #392)
    const uint32_t graine = rd() ^ (uint32_t)(e ^ (e >> 32));
    return ouvrir(s, t, entreeDepuis(s), graine);
}

// ---------------------------------------------------------------- journal
std::vector<std::string> bilan(const Sauvegarde& s, const Instant& t) {
    const EtatCadeau& c = s.cadeau;
    char b[512];
    std::vector<std::string> v;
    std::snprintf(b, sizeof b, "CADEAU : cadeau-heure=%lld cadeau-decalage=%d cadeau-jour=%s cadeau-sans-rare=%d cadeau-total=%lld cadeau-dernier=%s",
                  c.heure, c.decalage, c.jour.empty() ? "—" : c.jour.c_str(), c.sansRare, c.total, c.dernier.empty() ? "—" : c.dernier.c_str());
    v.push_back(b);
    std::snprintf(b, sizeof b, "CADEAU : pret-voiture=%s pret-recu=%lld pret-fin=%lld", c.pretVoiture.empty() ? "—" : c.pretVoiture.c_str(), c.pretRecu, c.pretFin);
    v.push_back(b);
    const Entree e = entreeDepuis(s);
    size_t coloris = 0, skins = 0;
    for (const VoitureJoueur& j : e.voitures) { coloris += j.coloris.size(); skins += j.skins.size(); }
    std::snprintf(b, sizeof b, "CADEAU : %s le %s ; garantie %d/%d ; à gagner : %zu coloris, %zu skin(s) sur %zu voiture(s), %zu voiture(s) à prêter",
                  texteEtat(etat(c, t)), texteJour(jourLocal(t)).c_str(), c.sansRare, config().cadeau.garantie, coloris, skins, e.voitures.size(),
                  e.voituresPretables.size());
    v.push_back(b);
    return v;
}

} // namespace wfs::cadeau
