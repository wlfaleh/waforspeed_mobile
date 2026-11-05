// WaForSpeed — duel par Internet (WFS-89) : voir reseau_peerjs.h. Port de joinP2P (js/net/p2p.js du jeu web).
#include "reseau_peerjs.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../core/log.h"
#include "binarypack.h"

namespace wfs::reseau {

namespace {
FabriquePair g_fabrique;
double horlogeMurale() {
    using namespace std::chrono;
    return (double)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
bool cleOk(const std::string& k) {
    if (k.empty() || k.size() > 8) return false;
    for (char c : k) if (c < 'a' || c > 'z') return false;
    return true;
}
// champs numériques des présences du jeu web (num(pres(p, k)) : un texte n'y est pas un nombre)
bool champNombre(const std::string& k) {
    static const char* const N[] = {"r", "d", "x", "v", "h", "b", "km", "ft"};
    for (const char* n : N) if (k == n) return true;
    return false;
}
std::string texteNombre(double v) {
    char b[32];
    if (std::floor(v) == v && std::fabs(v) < 1e15) std::snprintf(b, sizeof b, "%lld", (long long)v);
    else std::snprintf(b, sizeof b, "%.9g", v);
    return b;
}
// présence native (duel::Fields) → objet du jeu web : nombres, host booléen, map « carte/moment » → map + tod
bp::Valeur versWeb(const duel::Fields& f) {
    bp::Valeur o = bp::Valeur::objet();
    for (const auto& [k, v] : f) {
        if (k == "host") { o.ajoute("host", bp::Valeur::booleen(v == "1")); continue; }
        if (k == "map") {
            const size_t sl = v.find('/');
            o.ajoute("map", v.empty() ? bp::Valeur::nul() : bp::Valeur::texte(v.substr(0, sl)));
            if (sl != std::string::npos) o.ajoute("tod", bp::Valeur::texte(v.substr(sl + 1)));
            continue;
        }
        if (champNombre(k)) {
            char* fin = nullptr;
            const double x = v.empty() ? 0 : std::strtod(v.c_str(), &fin);
            o.ajoute(k, v.empty() || !fin || *fin || !std::isfinite(x) ? bp::Valeur::nul() : bp::Valeur::nombre(x));
            continue;
        }
        o.ajoute(k, bp::Valeur::texte(v));
    }
    return o;
}
std::vector<uint8_t> messageListe(const std::string& monId, const duel::Fields& moi, const std::vector<std::pair<std::string, const duel::Fields*>>& autres) {
    bp::Valeur m = bp::Valeur::objet(), l = bp::Valeur::tableau();
    auto ligne = [&](const std::string& id, const duel::Fields& f) {
        bp::Valeur e = bp::Valeur::tableau(); e.tab.push_back(bp::Valeur::texte(id)); e.tab.push_back(versWeb(f)); l.tab.push_back(std::move(e));
    };
    ligne(monId, moi);
    for (const auto& [id, f] : autres) ligne(id, *f);
    m.ajoute("t", bp::Valeur::texte("all")); m.ajoute("list", std::move(l));
    return bp::pack(m);
}
enum { E_OUVERT, E_IDPRIS, E_ERREUR, E_CONNEXION, E_DONNEES, E_FERMETURE };
} // namespace

#ifndef WFS_INTERNET
std::unique_ptr<Pair> pairPeerJsReel() { return nullptr; }   // version sans libdatachannel (banc PC, iOS)
#endif

void installerFabriquePair(FabriquePair f) { g_fabrique = std::move(f); }
FabriquePair fabriquePair() {
    if (g_fabrique) return g_fabrique;
    return [] { return pairPeerJsReel(); };
}

std::vector<uint8_t> messagePresence(const duel::Fields& f) {
    bp::Valeur m = bp::Valeur::objet();
    m.ajoute("t", bp::Valeur::texte("p")); m.ajoute("presence", versWeb(f));
    return bp::pack(m);
}

// objet de présence (jeu web ou natif) → Fields bornés ; faux si ce n'est pas un objet
bool lirePresenceWeb(const void* valeurBp, duel::Fields& out) {
    const bp::Valeur& p = *(const bp::Valeur*)valeurBp;
    out.clear();
    if (p.type != bp::Valeur::Objet) return false;
    std::string carte, moment;
    for (const auto& [k, v] : p.obj) {
        if (!cleOk(k) || out.size() >= 32) continue;
        switch (v.type) {
            case bp::Valeur::Texte: if (v.s.size() <= 64) out[k] = v.s; break;
            case bp::Valeur::Nombre: if (std::isfinite(v.n)) out[k] = texteNombre(v.n); break;
            case bp::Valeur::Booleen: out[k] = v.b ? "1" : "0"; break;
            case bp::Valeur::Nul: out[k] = ""; break;
            default: break;   // tableaux et objets imbriqués : ignorés
        }
        if (k == "map" && v.type == bp::Valeur::Texte) carte = v.s;
        if (k == "tod" && v.type == bp::Valeur::Texte) moment = v.s;
    }
    if (!carte.empty() && !moment.empty() && carte.find('/') == std::string::npos) out["map"] = carte + "/" + moment;
    return true;
}

// ---------------------------------------------------------------- transport
TransportPeerJs::TransportPeerJs(FabriquePair fabrique, std::string code)
    : fabrique_(std::move(fabrique)), code_(std::move(code)), vivant_(std::make_shared<bool>(true)) {
    for (char& c : code_) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    debut_ = horlogeMurale();
    pair_ = fabrique_ ? fabrique_() : nullptr;
    if (!pair_) { etat_ = Etat::Echec; WFS_LOG("duel Internet : indisponible (pas de réseau PeerJS dans cette version)"); return; }
    WFS_LOG("duel Internet : salon %s, essai comme relais (%s)", code_.c_str(), idRelais(code_).c_str());
    pair_->ouvrir(idRelais(code_), rappels());
}
TransportPeerJs::~TransportPeerJs() { *vivant_ = false; if (pair_) pair_->fermer(); }

EvenementsPair TransportPeerJs::rappels() const {
    std::weak_ptr<bool> v = vivant_;
    auto pousser = [this, v](Evt e) { auto s = v.lock(); if (!s || !*s) return; std::lock_guard<std::mutex> l(verrou_); file_.push_back(std::move(e)); };
    EvenementsPair ev;
    ev.ouvert = [pousser](const std::string& id) { pousser({E_OUVERT, id, {}}); };
    ev.idPris = [pousser] { pousser({E_IDPRIS, "", {}}); };
    ev.erreur = [pousser](const std::string& e) { pousser({E_ERREUR, e, {}}); };
    ev.connexion = [pousser](const std::string& p) { pousser({E_CONNEXION, p, {}}); };
    ev.donnees = [pousser](const std::string& p, std::vector<uint8_t> o) { if (o.size() <= 65536) pousser({E_DONNEES, p, std::move(o)}); };
    ev.fermeture = [pousser](const std::string& p) { pousser({E_FERMETURE, p, {}}); };
    return ev;
}

void TransportPeerJs::pomper() const {
    for (int tour = 0; tour < 16; tour++) {   // un événement peut en produire d'autres (ID-TAKEN → OPEN → connexion)
        std::deque<Evt> f;
        { std::lock_guard<std::mutex> l(verrou_); f.swap(file_); }
        if (f.empty()) break;
        for (const Evt& e : f) traiter(e);
    }
    const double now = horlogeMurale();
    if (etat_ == Etat::Connexion && now - debut_ > DELAI_CONNEXION_MS) {   // 12 s : échec (upstream_error du jeu web)
        etat_ = Etat::Echec; WFS_LOG("duel Internet : salon %s injoignable après 12 s", code_.c_str());
        if (pair_) { pair_->fermer(); pair_.reset(); }
    }
    if (envoiEnAttente_ && now - dernierEnvoi_ >= GROUPAGE_MS) envoyerMoi();
}

void TransportPeerJs::traiter(const Evt& e) const {
    if (parti_ || !pair_) return;
    switch (e.type) {
        case E_OUVERT:
            monId_ = e.a;
            if (e.a == idRelais(code_)) { relais_ = true; etat_ = Etat::Pret; WFS_LOG("duel Internet : relais du salon %s", code_.c_str()); }
            else pair_->connecter(idRelais(code_));   // 2) invité : connexion au relais
            break;
        case E_IDPRIS: {   // le salon existe : nouveau pair à identifiant aléatoire (startClient)
            pair_->fermer();
            pair_ = fabrique_();
            if (!pair_) { etat_ = Etat::Echec; break; }
            WFS_LOG("duel Internet : salon %s déjà ouvert, connexion au relais", code_.c_str());
            pair_->ouvrir("", rappels());
            break;
        }
        case E_ERREUR:
            WFS_LOG("duel Internet : erreur « %s »", e.a.c_str());
            if (etat_ == Etat::Connexion) { etat_ = Etat::Echec; pair_->fermer(); pair_.reset(); }
            else if (!relais_ && e.a != "peer-unavailable") etat_ = Etat::Perdu;
            break;
        case E_CONNEXION:
            if (relais_) {
                if (std::find(connectes_.begin(), connectes_.end(), e.a) == connectes_.end()) connectes_.push_back(e.a);
                diffuser();
            } else if (e.a == idRelais(code_)) {
                etat_ = Etat::Pret;
                WFS_LOG("duel Internet : connecté au relais du salon %s", code_.c_str());
                dernierEnvoi_ = -1e18; envoyerMoi();
            }
            break;
        case E_DONNEES: {
            bp::Valeur m;
            if (!bp::unpack(e.o, m) || m.type != bp::Valeur::Objet) break;   // message invalide : ignoré
            const bp::Valeur* t = m.champ("t");
            if (!t || t->type != bp::Valeur::Texte) break;
            const double now = horlogeMurale();
            if (relais_ && t->s == "p") {
                const bp::Valeur* p = m.champ("presence");
                duel::Fields f;
                if (!p || !lirePresenceWeb(p, f)) break;
                table_[e.a] = {std::move(f), now};
                diffuser();
            } else if (!relais_ && t->s == "all" && e.a == idRelais(code_)) {
                const bp::Valeur* l = m.champ("list");
                if (!l || l->type != bp::Valeur::Tableau || l->tab.size() > 16) break;
                std::map<std::string, Joueur> avant; avant.swap(table_);
                for (const bp::Valeur& ligne : l->tab) {
                    if (ligne.type != bp::Valeur::Tableau || ligne.tab.size() != 2 || ligne.tab[0].type != bp::Valeur::Texte) continue;
                    const std::string& id = ligne.tab[0].s;
                    duel::Fields f;
                    if (id == monId_ || id.empty() || id.size() > 64 || !lirePresenceWeb(&ligne.tab[1], f)) continue;
                    const auto it = avant.find(id);
                    table_[id] = {f, it != avant.end() && it->second.f == f ? it->second.maj : now};   // updatedAt inchangé si même présence
                }
            }
            break;
        }
        case E_FERMETURE:
            if (relais_) {
                connectes_.erase(std::remove(connectes_.begin(), connectes_.end(), e.a), connectes_.end());
                table_.erase(e.a);
                diffuser();
            } else if (e.a == idRelais(code_)) {   // relais parti : connexion perdue
                table_.clear(); etat_ = Etat::Perdu;
                WFS_LOG("duel Internet : connexion au relais du salon %s perdue", code_.c_str());
            }
            break;
        default: break;
    }
}

void TransportPeerJs::diffuser() const {
    if (!pair_ || connectes_.empty()) return;
    std::vector<std::pair<std::string, const duel::Fields*>> autres;
    for (const auto& [id, j] : table_) autres.emplace_back(id, &j.f);
    const std::vector<uint8_t> m = messageListe(monId_, moi_, autres);
    for (const std::string& c : connectes_) pair_->envoyer(c, m);
}

void TransportPeerJs::envoyerMoi() const {
    if (!pair_ || parti_) return;
    if (relais_) { diffuser(); return; }
    const double now = horlogeMurale();
    if (etat_ != Etat::Pret || now - dernierEnvoi_ < GROUPAGE_MS) { envoiEnAttente_ = true; return; }   // groupé (30 ms)
    pair_->envoyer(idRelais(code_), messagePresence(moi_));
    dernierEnvoi_ = now; envoiEnAttente_ = false;
}

void TransportPeerJs::presence(const duel::Fields& f) {
    for (const auto& [k, v] : f) moi_[k] = v;
    pomper();
    envoyerMoi();
}

std::vector<duel::PeerView> TransportPeerJs::peers() const {
    pomper();
    std::vector<duel::PeerView> out;
    for (const auto& [id, j] : table_) { duel::PeerView p; p.id = id; p.f = j.f; p.updatedAt = j.maj; out.push_back(std::move(p)); }
    return out;
}

double TransportPeerJs::nowMs() const { pomper(); return horlogeMurale(); }

void TransportPeerJs::leave() {
    if (parti_) return;
    parti_ = true; *vivant_ = false;
    if (pair_) { pair_->fermer(); pair_.reset(); }   // LEAVE (fermeture des canaux)
    table_.clear();
}

// ---------------------------------------------------------------- deux transports à la fois
TransportDouble::TransportDouble(std::unique_ptr<duel::Transport> local, std::unique_ptr<duel::Transport> internet)
    : local_(std::move(local)), internet_(std::move(internet)) {}
void TransportDouble::presence(const duel::Fields& f) {
    if (local_) local_->presence(f);
    if (internet_) internet_->presence(f);
}
std::vector<duel::PeerView> TransportDouble::peers() const {
    if (choisi_ == 1) return local_->peers();
    if (choisi_ == 2) return internet_->peers();
    auto a = local_ ? local_->peers() : std::vector<duel::PeerView>{};
    if (!a.empty()) { choisi_ = 1; if (internet_) { internet_->leave(); internet_.reset(); } WFS_LOG("duel : adversaire trouvé sur le Wi-Fi local"); return a; }
    auto b = internet_ ? internet_->peers() : std::vector<duel::PeerView>{};
    if (!b.empty()) { choisi_ = 2; if (local_) { local_->leave(); local_.reset(); } WFS_LOG("duel : adversaire trouvé par Internet"); return b; }
    return {};
}
double TransportDouble::nowMs() const {
    if (choisi_ == 2 || !local_) return internet_ ? internet_->nowMs() : horlogeMurale();
    if (internet_) internet_->lien();   // la file d'Internet continue d'être lue
    return local_->nowMs();
}
void TransportDouble::leave() {
    if (local_) local_->leave();
    if (internet_) internet_->leave();
}

// ---------------------------------------------------------------- réseau PeerJS simulé (banc, tests)
namespace {
class PairMemoire : public Pair {
public:
    explicit PairMemoire(std::shared_ptr<ReseauMemoire> r) : r_(std::move(r)) {}
    ~PairMemoire() override { fermer(); }
    void ouvrir(const std::string& id, EvenementsPair ev) override {
        ev_ = std::move(ev);
        if (r_->horsLigne) { if (ev_.erreur) ev_.erreur("network"); return; }
        const std::string i = id.empty() ? "pair" + std::to_string(++r_->compteur) + "x9" : id;
        if (r_->pairs.count(i)) { if (ev_.idPris) ev_.idPris(); return; }
        id_ = i; r_->pairs[id_] = &ev_;
        if (ev_.ouvert) ev_.ouvert(id_);
    }
    void connecter(const std::string& dst) override {
        auto it = r_->pairs.find(dst);
        if (id_.empty() || it == r_->pairs.end()) { if (ev_.erreur) ev_.erreur("peer-unavailable"); return; }
        r_->liens[id_].push_back(dst); r_->liens[dst].push_back(id_);
        if (it->second->connexion) it->second->connexion(id_);
        if (ev_.connexion) ev_.connexion(dst);
    }
    void envoyer(const std::string& p, const std::vector<uint8_t>& o) override {
        const auto& l = r_->liens[id_];
        if (std::find(l.begin(), l.end(), p) == l.end()) return;
        auto it = r_->pairs.find(p);
        if (it != r_->pairs.end() && it->second->donnees) it->second->donnees(id_, o);
    }
    void fermer() override {
        if (id_.empty()) return;
        const std::vector<std::string> l = r_->liens[id_];
        r_->liens.erase(id_); r_->pairs.erase(id_);
        for (const std::string& p : l) {
            auto& lp = r_->liens[p]; lp.erase(std::remove(lp.begin(), lp.end(), id_), lp.end());
            auto it = r_->pairs.find(p);
            if (it != r_->pairs.end() && it->second->fermeture) it->second->fermeture(id_);
        }
        id_.clear();
    }
private:
    std::shared_ptr<ReseauMemoire> r_;
    EvenementsPair ev_;
    std::string id_;
};
} // namespace

FabriquePair ReseauMemoire::fabrique() {
    std::shared_ptr<ReseauMemoire> moi = shared_from_this();
    return [moi]() -> std::unique_ptr<Pair> { return std::make_unique<PairMemoire>(moi); };
}

} // namespace wfs::reseau
