// WaForSpeed — « Duel entre amis » par Internet (WFS-112) : voir reseau_relais.h. Protocole : serveur/src/salon.js.
#include "reseau_relais.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <random>
#include <thread>
#include "../core/log.h"

#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#if !defined(__ANDROID__)
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

namespace wfs::reseau {

AdresseRelais& adresseRelais() { static AdresseRelais a; return a; }

static double horlogeMonotone() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}
static uint32_t alea() {
    static std::mt19937 g(std::random_device{}() ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
    static std::mutex m; std::lock_guard<std::mutex> l(m);
    return g();
}
static std::string minuscules(std::string_view s) {
    std::string o(s);
    for (char& c : o) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return o;
}

// ---------------------------------------------------------------- liens d'invitation
std::string lienInvitation(const std::string& code, const AdresseRelais& a) {
    const bool portStd = (a.tls && a.port == 443) || (!a.tls && a.port == 80);
    return std::string(a.tls ? "https://" : "http://") + a.hote + (portStd ? "" : ":" + std::to_string(a.port)) + "/d/" + minuscules(code);
}
std::string texteInvitation(const std::string& code) {
    std::string c = code;
    for (char& ch : c) if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    return "Viens me défier sur WaForSpeed ! Duel entre amis, code " + c + " : " + lienInvitation(code);
}
bool lireLienInvitation(std::string_view lien, std::string& code) {
    if (lien.size() > 512) return false;
    const std::string l = minuscules(lien);
    for (const char* marque : {"duel/", "/d/"}) {
        const size_t p = l.find(marque);
        if (p == std::string::npos) continue;
        const size_t d = p + std::strlen(marque);
        if (d + 4 > l.size()) continue;
        const std::string c = l.substr(d, 4);
        const bool fin = d + 4 == l.size() || !std::isalnum((unsigned char)l[d + 4]);   // exactement 4 caractères
        if (fin && duel::validCode(c)) { code = c; return true; }
    }
    return false;
}

// ---------------------------------------------------------------- SHA-1, base64 (poignée de main)
std::string sha1(std::string_view d) {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    std::string m(d);
    const uint64_t bits = (uint64_t)d.size() * 8;
    m += (char)0x80;
    while (m.size() % 64 != 56) m += (char)0;
    for (int i = 7; i >= 0; i--) m += (char)((bits >> (i * 8)) & 255);
    auto rol = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
    for (size_t b = 0; b < m.size(); b += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = (uint32_t)(uint8_t)m[b + 4 * i] << 24 | (uint32_t)(uint8_t)m[b + 4 * i + 1] << 16 | (uint32_t)(uint8_t)m[b + 4 * i + 2] << 8 | (uint8_t)m[b + 4 * i + 3];
        for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], bb = h[1], c = h[2], dd = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) { f = (bb & c) | (~bb & dd); k = 0x5A827999; }
            else if (i < 40) { f = bb ^ c ^ dd; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (bb & c) | (bb & dd) | (c & dd); k = 0x8F1BBCDC; }
            else { f = bb ^ c ^ dd; k = 0xCA62C1D6; }
            const uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = dd; dd = c; c = rol(bb, 30); bb = a; a = t;
        }
        h[0] += a; h[1] += bb; h[2] += c; h[3] += dd; h[4] += e;
    }
    std::string o;
    for (uint32_t v : h) for (int i = 3; i >= 0; i--) o += (char)((v >> (i * 8)) & 255);
    return o;
}
std::string base64(std::string_view d) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o;
    for (size_t i = 0; i < d.size(); i += 3) {
        const uint32_t n = (uint32_t)(uint8_t)d[i] << 16 | (i + 1 < d.size() ? (uint32_t)(uint8_t)d[i + 1] << 8 : 0) | (i + 2 < d.size() ? (uint8_t)d[i + 2] : 0);
        o += T[(n >> 18) & 63]; o += T[(n >> 12) & 63];
        o += i + 1 < d.size() ? T[(n >> 6) & 63] : '=';
        o += i + 2 < d.size() ? T[n & 63] : '=';
    }
    return o;
}

// ---------------------------------------------------------------- flux TCP (POSIX), sur son propre fil
namespace {
// état partagé avec le fil du flux : le fil le garde vivant après la destruction du flux (envoi de ce qui reste en file,
// puis fermeture), le jeu n'attend jamais
struct EtatTcp {
    std::mutex m;
    std::string aEnvoyer, recu;
    std::atomic<int> etat{(int)Flux::Etat::Connexion};
    std::atomic<bool> fin{false}, fermerApres{false};
};
void boucleTcp(std::shared_ptr<EtatTcp> e, const std::string& hote, int port) {
    int fd = -1;
    addrinfo indices{}, *res = nullptr;
    indices.ai_family = AF_UNSPEC; indices.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hote.c_str(), std::to_string(port).c_str(), &indices, &res) == 0 && res) {
        for (addrinfo* a = res; a && fd < 0 && !e->fin; a = a->ai_next) {
            fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            if (fd < 0) continue;
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            if (connect(fd, a->ai_addr, a->ai_addrlen) != 0 && errno != EINPROGRESS) { close(fd); fd = -1; continue; }
            bool ok = false;
            for (int t = 0; t < 250 && !e->fin; t++) {   // 5 s au plus
                pollfd p{fd, POLLOUT, 0};
                if (poll(&p, 1, 20) > 0) { int err = 0; socklen_t n = sizeof err; getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &n); ok = err == 0; break; }
            }
            if (!ok) { close(fd); fd = -1; }
        }
        freeaddrinfo(res);
    }
    if (fd < 0) { e->etat = (int)Flux::Etat::Ferme; return; }
    int un = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &un, sizeof un);
    e->etat = (int)Flux::Etat::Ouvert;
    char buf[4096];
    while (!e->fin) {
        std::string sortie;
        { std::lock_guard<std::mutex> l(e->m); sortie.swap(e->aEnvoyer); }
        size_t envoye = 0;
        for (int essais = 0; envoye < sortie.size() && !e->fin && essais < 250; essais++) {
            const ssize_t n = send(fd, sortie.data() + envoye, sortie.size() - envoye, MSG_NOSIGNAL);
            if (n > 0) envoye += (size_t)n;
            else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { pollfd p{fd, POLLOUT, 0}; poll(&p, 1, 20); }
            else e->fin = true;
        }
        if (e->fermerApres) { std::lock_guard<std::mutex> l(e->m); if (e->aEnvoyer.empty()) { shutdown(fd, SHUT_WR); break; } }
        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, 10) > 0) {
            const ssize_t n = recv(fd, buf, sizeof buf, 0);
            if (n > 0) { std::lock_guard<std::mutex> l(e->m); if (e->recu.size() < 262144) e->recu.append(buf, (size_t)n); }
            else if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) break;
        }
    }
    close(fd);
    e->etat = (int)Flux::Etat::Ferme;
}
class FluxTcp : public Flux {
public:
    FluxTcp(const std::string& hote, int port) : e_(std::make_shared<EtatTcp>()) {
        std::thread([e = e_, hote, port] { boucleTcp(e, hote, port); }).detach();
    }
    ~FluxTcp() override { if (!e_->fermerApres) e_->fin = true; }   // fermé : le fil finit d'envoyer seul
    Etat etat() const override { return (Etat)e_->etat.load(); }
    void envoyer(const std::string& o) override { std::lock_guard<std::mutex> l(e_->m); if (e_->aEnvoyer.size() < 65536) e_->aEnvoyer += o; }
    std::string lire() override { std::lock_guard<std::mutex> l(e_->m); std::string r; r.swap(e_->recu); return r; }
    void fermer() override { e_->fermerApres = true; }
    void couper() override { e_->fin = true; }
private:
    std::shared_ptr<EtatTcp> e_;
};
class FluxFerme : public Flux {
public:
    Etat etat() const override { return Etat::Ferme; }
    void envoyer(const std::string&) override {}
    std::string lire() override { return {}; }
    void fermer() override {}
};
std::function<std::unique_ptr<Flux>(const std::string&, int)> g_fluxTls;
} // namespace

std::unique_ptr<Flux> fluxTcp(const std::string& hote, int port) { return std::make_unique<FluxTcp>(hote, port); }
void installerFluxTls(std::function<std::unique_ptr<Flux>(const std::string&, int)> f) { g_fluxTls = std::move(f); }
std::unique_ptr<Flux> fluxDefaut(const std::string& hote, int port, bool tls) {
    if (!tls) return fluxTcp(hote, port);
    if (g_fluxTls) if (auto f = g_fluxTls(hote, port)) return f;
    return std::make_unique<FluxFerme>();
}

// ---------------------------------------------------------------- client WebSocket
ClientWs::ClientWs(std::unique_ptr<Flux> f, const std::string& hote, const std::string& chemin) : f_(std::move(f)) {
    std::string brut;
    for (int i = 0; i < 16; i++) brut += (char)(alea() & 255);
    cle_ = base64(brut);
    if (f_) f_->envoyer("GET " + chemin + " HTTP/1.1\r\nHost: " + hote + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + cle_ +
                        "\r\nSec-WebSocket-Version: 13\r\nUser-Agent: WaForSpeed\r\n\r\n");
}
ClientWs::Etat ClientWs::etat() { pomper(f_ ? f_->lire() : std::string()); return etat_; }
void ClientWs::echec(const char* raison) {
    if (etat_ != Etat::Ferme) WFS_WARN("relais : connexion fermée (%s)", raison);
    erreurs_++; etat_ = Etat::Ferme; tampon_.clear(); morceaux_.clear();
    if (f_) f_->couper();
}
static std::string trameClient(int opcode, std::string_view p) {
    std::string t; t += (char)(0x80 | opcode);
    const size_t n = p.size();
    if (n < 126) t += (char)(0x80 | n);
    else { t += (char)(0x80 | 126); t += (char)((n >> 8) & 255); t += (char)(n & 255); }
    char k[4]; for (char& c : k) c = (char)(alea() & 255);
    t.append(k, 4);
    for (size_t i = 0; i < n; i++) t += (char)(p[i] ^ k[i & 3]);
    return t;
}
void ClientWs::envoyer(const std::string& texte) {
    if (etat_ != Etat::Ouvert || !f_ || texte.size() > TRAME_MAX) return;
    f_->envoyer(trameClient(1, texte));
}
std::vector<std::string> ClientWs::recevoir() {
    pomper(f_ ? f_->lire() : std::string());
    std::vector<std::string> r(recus_.begin(), recus_.end());
    recus_.clear();
    return r;
}
void ClientWs::fermer() {
    if (f_ && etat_ == Etat::Ouvert) f_->envoyer(trameClient(8, std::string("\x03\xe8", 2)));
    if (f_) f_->fermer();
    etat_ = Etat::Ferme;
}
void ClientWs::pomper(const std::string& nouveaux) {
    if (etat_ == Etat::Ferme) return;
    tampon_ += nouveaux;
    if (etat_ == Etat::Poignee) {
        const size_t fin = tampon_.find("\r\n\r\n");
        if (fin == std::string::npos) {
            if (tampon_.size() > 4096) echec("réponse trop longue");
            else if (f_ && f_->etat() == Flux::Etat::Ferme) echec("relais injoignable");
            return;
        }
        const std::string tete = minuscules(tampon_.substr(0, fin));
        const std::string attendu = minuscules(base64(sha1(cle_ + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")));
        const size_t a = tete.find("\r\nsec-websocket-accept:");
        std::string recu;
        if (a != std::string::npos) {
            size_t d = a + 23; while (d < tete.size() && tete[d] == ' ') d++;
            size_t e = tete.find("\r\n", d); if (e == std::string::npos) e = tete.size();
            recu = tete.substr(d, e - d); while (!recu.empty() && recu.back() == ' ') recu.pop_back();
        }
        if (tete.compare(0, 12, "http/1.1 101") != 0 || recu != attendu) { echec("poignée de main refusée"); return; }
        tampon_.erase(0, fin + 4);
        etat_ = Etat::Ouvert;
    }
    for (;;) {
        if (tampon_.size() < 2) break;
        const uint8_t b0 = (uint8_t)tampon_[0], b1 = (uint8_t)tampon_[1];
        const int op = b0 & 15; const bool fin = b0 & 0x80;
        if (b0 & 0x70) { echec("bits réservés"); return; }
        if (b1 & 0x80) { echec("trame masquée du serveur"); return; }
        uint64_t n = b1 & 127; size_t i = 2;
        if (n == 126) { if (tampon_.size() < 4) break; n = (uint64_t)(uint8_t)tampon_[2] << 8 | (uint8_t)tampon_[3]; i = 4; }
        else if (n == 127) {
            if (tampon_.size() < 10) break;
            n = 0; for (int k = 0; k < 8; k++) n = n << 8 | (uint8_t)tampon_[2 + k];
            i = 10;
        }
        if (n > TRAME_MAX) { echec("trame trop longue"); return; }
        if (tampon_.size() < i + n) break;
        const std::string p = tampon_.substr(i, (size_t)n);
        tampon_.erase(0, i + (size_t)n);
        if (op == 8) { fermer(); return; }
        if (op == 9) { if (f_) { std::string t; t += (char)0x8A; t += (char)(0x80 | p.size()); char k[4] = {0, 0, 0, 0}; t.append(k, 4); t += p; if (p.size() < 126) f_->envoyer(t); } continue; }
        if (op == 10) continue;
        if (op != 0 && op != 1) { echec("type de trame refusé"); return; }
        if (op == 1 && !morceaux_.empty()) { echec("message coupé"); return; }
        morceaux_ += p;
        if (morceaux_.size() > TRAME_MAX) { echec("message trop long"); return; }
        if (fin) { if (recus_.size() < 256) recus_.push_back(morceaux_); morceaux_.clear(); }
    }
    if (f_ && f_->etat() == Flux::Etat::Ferme && etat_ == Etat::Ouvert) { etat_ = Etat::Ferme; }
}

// ---------------------------------------------------------------- transport du duel par le relais
static bool idValide(std::string_view s) {
    if (s.empty() || s.size() > 20) return false;
    for (char c : s) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    return true;
}
// nombre décimal positif fini (horloges du relais, en ms) ; faux sinon
static bool nombre(std::string_view s, double& v) {
    if (s.empty() || s.size() > 24) return false;
    for (char c : s) if (!((c >= '0' && c <= '9') || c == '.')) return false;
    const std::string t(s);
    char* fin = nullptr; v = std::strtod(t.c_str(), &fin);
    return fin && *fin == 0 && std::isfinite(v) && v >= 0 && v < 1e15;
}

TransportRelais::TransportRelais(FabriqueFlux f, AdresseRelais a, std::string code, bool hote, std::function<double()> horloge, std::string id)
    : fabrique_(std::move(f)), a_(std::move(a)), code_(minuscules(code)), id_(std::move(id)), hote_(hote), horloge_(std::move(horloge)) {
    if (!idValide(id_) || id_.size() < 6) {
        const char* C = "abcdefghijkmnpqrstuvwxyz23456789";
        id_.clear(); for (int i = 0; i < 12; i++) id_ += C[alea() % 32];
    }
    debut_ = local();
    connecter();
}
TransportRelais::~TransportRelais() { if (!parti_) leave(); }

double TransportRelais::local() const { return horloge_ ? horloge_() : horlogeMonotone(); }
double TransportRelais::nowMs() const { pomper(); return local() + decalage_; }

void TransportRelais::connecter() const {
    prochainEssai_ = local() + ESSAI_MS;
    ouvert_ = false;
    if (!fabrique_ || !duel::validCode(code_)) { ws_.reset(); return; }
    auto f = fabrique_(a_.hote, a_.port, a_.tls);
    const std::string chemin = "/salon/" + code_ + "?id=" + id_ + "&hote=" + (hote_ ? "1" : "0") + "&v=1";
    const bool portStd = (a_.tls && a_.port == 443) || (!a_.tls && a_.port == 80);
    ws_ = std::make_unique<ClientWs>(std::move(f), a_.hote + (portStd ? "" : ":" + std::to_string(a_.port)), chemin);
}

void TransportRelais::envoyerPresence() const {
    if (!ws_ || !ouvert_) return;
    const std::string p = duel::encode(moi_);
    if (p.size() > duel::MSG_MAX) { WFS_WARN("relais : présence trop longue (%zu octets), pas envoyée", p.size()); return; }
    ws_->envoyer("P|" + p);
    attente_ = false; dernierEnvoi_ = local();
}

void TransportRelais::traiter(const std::string& m) const {
    dernierRecu_ = local();
    if (m.size() > MESSAGE_MAX) { ignores_++; return; }
    auto champ = [&](size_t& i) -> std::string_view {
        if (i > m.size()) return {};
        const size_t e = m.find('|', i); std::string_view v = std::string_view(m).substr(i, e == std::string::npos ? std::string::npos : e - i);
        i = e == std::string::npos ? m.size() + 1 : e + 1;
        return v;
    };
    size_t i = 0;
    const std::string_view type = champ(i);
    if (type == "P") {
        const std::string_view id = champ(i), s = champ(i);
        double maj;
        if (i > m.size() || !idValide(id) || !nombre(s, maj) || id == id_) { ignores_++; return; }
        duel::Fields f;
        if (!duel::decode(std::string_view(m).substr(i), f)) { ignores_++; return; }
        const std::string cle(id);
        if (!table_.count(cle) && table_.size() >= JOUEURS_MAX) { ignores_++; return; }
        table_[cle] = {std::move(f), maj};
    } else if (type == "B") {
        const std::string_view id = champ(i);
        if (!idValide(id)) { ignores_++; return; }
        table_.erase(std::string(id));
    } else if (type == "pong") {
        const std::string_view c = champ(i), s = champ(i);
        double envoi, serveur;
        if (!nombre(c, envoi) || !nombre(s, serveur)) { ignores_++; return; }
        const double t = local(), rtt = t - envoi;
        if (rtt < 0 || rtt > 10000) { ignores_++; return; }
        echantillons_.push_back({rtt, serveur + rtt / 2 - t});
        if (echantillons_.size() > 8) echantillons_.pop_front();
        const auto best = std::min_element(echantillons_.begin(), echantillons_.end());
        decalage_ = best->second; aEuDecalage_ = true;
        ping_ = (int)std::lround(ping_ < 0 ? rtt : ping_ * 0.6 + rtt * 0.4);
    } else if (type == "OK") {
        const std::string_view s = champ(i), e = champ(i);
        double serveur, fin;
        if (!nombre(s, serveur) || !nombre(e, fin)) { ignores_++; return; }
        if (!aEuDecalage_) decalage_ = serveur - local();   // première estimation, affinée par les pings
        expire_ = fin; accepte_ = true;
    } else if (type == "E") {
        const std::string_view r = champ(i);
        static const char* RAISONS[] = {"inconnu", "expire", "plein", "pris", "invalide"};
        raison_ = "invalide";
        for (const char* x : RAISONS) if (r == x) raison_ = x;
        // reconnexion refusée (place perdue) : perdu ; sinon échec (code inconnu, expiré, plein)
        etat_ = accepte_ ? duel::Lien::Perdu : duel::Lien::Echec;
        WFS_WARN("relais : salon %s refusé (%s)", code_.c_str(), raison_.c_str());
        if (ws_) ws_->fermer();
    } else ignores_++;
}

void TransportRelais::pomper() const {
    if (parti_) return;
    const double t = local();
    if (ws_) {
        const auto ms = ws_->recevoir();
        if (ws_->etat() == ClientWs::Etat::Ouvert && !ouvert_ && etat_ != duel::Lien::Echec && etat_ != duel::Lien::Perdu) {
            ouvert_ = true; dernierRecu_ = t;
            if (etat_ == duel::Lien::Reconnexion) { reconnexions_++; WFS_LOG("relais : reconnecté au salon %s", code_.c_str()); }
            etat_ = duel::Lien::Pret;
            envoyerPresence();
            ws_->envoyer("ping|" + std::to_string((long long)t));
            dernierPing_ = t;
        }
        for (const std::string& m : ms) traiter(m);
        if (!ws_) return;
        if (ouvert_ && t - dernierRecu_ > SILENCE_MS) { WFS_WARN("relais : plus de nouvelles du relais depuis %.0f s", SILENCE_MS / 1000); ws_->couper(); ws_->fermer(); }
        if (ws_->etat() == ClientWs::Etat::Ferme) {
            ws_.reset(); ouvert_ = false;
            if (etat_ == duel::Lien::Pret) { etat_ = duel::Lien::Reconnexion; perte_ = t; WFS_WARN("relais : connexion perdue, reconnexion (%.0f s au plus)", RECONNEXION_MS / 1000); }
        }
    }
    if (etat_ == duel::Lien::Echec || etat_ == duel::Lien::Perdu) return;
    if (!ws_) {
        if (etat_ == duel::Lien::Connexion && t - debut_ > CONNEXION_MS) { etat_ = duel::Lien::Echec; raison_ = "injoignable"; WFS_WARN("relais : %s injoignable", a_.hote.c_str()); return; }
        if (etat_ == duel::Lien::Reconnexion && t - perte_ > RECONNEXION_MS) { etat_ = duel::Lien::Perdu; raison_ = "perdu"; table_.clear(); WFS_WARN("relais : connexion perdue (30 s)"); return; }
        if (t >= prochainEssai_) connecter();
        return;
    }
    if (!ouvert_) return;
    if (t - dernierPing_ >= PING_MS) { ws_->envoyer("ping|" + std::to_string((long long)t)); dernierPing_ = t; }
    if (attente_ && t - dernierEnvoi_ >= GROUPAGE_RELAIS_MS) envoyerPresence();
}

void TransportRelais::presence(const duel::Fields& f) {
    for (const auto& [k, v] : f) moi_[k] = v;
    attente_ = true;
    pomper();
}
std::vector<duel::PeerView> TransportRelais::peers() const {
    pomper();
    std::vector<duel::PeerView> out;
    for (const auto& [id, j] : table_) { duel::PeerView p; p.id = id; p.f = j.f; p.updatedAt = j.maj; out.push_back(std::move(p)); }
    return out;
}
void TransportRelais::leave() {
    if (parti_) return;
    if (ws_ && ouvert_) ws_->envoyer("bye");
    if (ws_) ws_->fermer();
    ws_.reset(); table_.clear(); parti_ = true;
}
duel::Lien TransportRelais::lien() const { pomper(); return etat_; }
double TransportRelais::expireDansMs() const { return expire_ < 0 ? -1 : std::max(0.0, expire_ - nowMs()); }
double TransportRelais::reconnexionRestanteMs() const {
    pomper();
    return etat_ == duel::Lien::Reconnexion ? std::max(0.0, RECONNEXION_MS - (local() - perte_)) : -1;
}
void TransportRelais::couper() { if (ws_) ws_->couper(); }

// ---------------------------------------------------------------- partage et lien profond
namespace {
void (*g_partage)(const std::string&) = nullptr;
std::string (*g_lienRecu)() = nullptr;
std::string g_dernierPartage, g_lienEssai;
}
void installerPartage(void (*f)(const std::string&)) { g_partage = f; }
bool partager(const std::string& texte) {
    g_dernierPartage = texte;
    WFS_LOG("duel : partage de l'invitation « %s »", texte.c_str());
    if (!g_partage) return false;
    g_partage(texte);
    return true;
}
const std::string& dernierPartage() { return g_dernierPartage; }
void installerLienRecu(std::string (*f)()) { g_lienRecu = f; }
std::string prendreLienRecu() {
    if (!g_lienEssai.empty()) { std::string l; l.swap(g_lienEssai); return l; }
    return g_lienRecu ? g_lienRecu() : std::string();
}
void lienRecuEssai(const std::string& lien) { g_lienEssai = lien; }

// ---------------------------------------------------------------- relais local
RelaisLocal::~RelaisLocal() { arreter(); }
void RelaisLocal::arreter() {
#if !defined(__ANDROID__)
    if (entree_ >= 0) { close(entree_); entree_ = -1; }
    if (pid_ > 0) { kill(pid_, SIGTERM); int st = 0; waitpid(pid_, &st, 0); pid_ = -1; }
#endif
    port_ = 0;
}
bool RelaisLocal::demarrer(const std::string& script, const std::vector<std::string>& options) {
#if defined(__ANDROID__)
    (void)script; (void)options; return false;
#else
    arreter();
    if (access(script.c_str(), R_OK) != 0) { WFS_WARN("relais local : %s introuvable", script.c_str()); return false; }
    int sortie[2], entree[2];
    if (pipe(sortie) != 0) return false;
    if (pipe(entree) != 0) { close(sortie[0]); close(sortie[1]); return false; }
    posix_spawn_file_actions_t fa; posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, entree[0], 0);
    posix_spawn_file_actions_adddup2(&fa, sortie[1], 1);
    posix_spawn_file_actions_addclose(&fa, entree[1]);
    posix_spawn_file_actions_addclose(&fa, sortie[0]);
    std::vector<std::string> args = {"node", script, "0", "--parent"};
    args.insert(args.end(), options.begin(), options.end());
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    pid_t pid = -1;
    const int e = posix_spawnp(&pid, "node", &fa, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&fa);
    close(entree[0]); close(sortie[1]);
    if (e != 0) { close(entree[1]); close(sortie[0]); WFS_WARN("relais local : node introuvable"); return false; }
    pid_ = pid; entree_ = entree[1];
    std::string lu; char b[256];
    for (int t = 0; t < 500 && port_ == 0; t++) {   // 10 s au plus
        pollfd p{sortie[0], POLLIN, 0};
        if (poll(&p, 1, 20) <= 0) continue;
        const ssize_t n = read(sortie[0], b, sizeof b);
        if (n <= 0) break;
        lu.append(b, (size_t)n);
        const size_t k = lu.find("RELAIS PORT ");
        if (k != std::string::npos && lu.find('\n', k) != std::string::npos) port_ = std::atoi(lu.c_str() + k + 12);
    }
    close(sortie[0]);
    if (port_ <= 0) { WFS_WARN("relais local : pas de port annoncé"); arreter(); return false; }
    WFS_LOG("relais local : node %s, port %d", script.c_str(), port_);
    return true;
#endif
}

} // namespace wfs::reseau
