// WaForSpeed — transport UDP du duel sur le réseau local (WFS-83) : voir reseau_udp.h.
#include "reseau_udp.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include "../core/log.h"

#if defined(_WIN32)
// pas de cible Windows : la prise reste fermée (le duel affiche « Aucun réseau »)
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define WFS_SOCKETS 1
#endif

namespace wfs::reseau {

namespace {
void (*g_verrou)(bool) = nullptr;

bool idOk(const std::string& s) {
    if (s.size() != 8) return false;
    for (char c : s) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    return true;
}
std::string idAleatoire() {
    std::random_device rd; std::mt19937 g(rd() ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
    const char* cs = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string s(8, 'a');
    for (char& c : s) c = cs[g() % 36];
    return s;
}
double horlogeAppareil() {
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return duration<double, std::milli>(steady_clock::now() - t0).count();
}

#ifdef WFS_SOCKETS
// adresses de destination : celles de la configuration, sinon diffusion générale + diffusion de chaque interface IPv4 active
std::vector<sockaddr_in> destinations(const Config& c) {
    std::vector<sockaddr_in> out;
    auto ajoute = [&](uint32_t ip, int port) {
        for (const auto& a : out) if (a.sin_addr.s_addr == ip && a.sin_port == htons((uint16_t)port)) return;
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons((uint16_t)port); a.sin_addr.s_addr = ip; out.push_back(a);
    };
    if (!c.destinations.empty()) {
        for (const auto& [ip, port] : c.destinations) { in_addr a{}; if (inet_pton(AF_INET, ip.c_str(), &a) == 1) ajoute(a.s_addr, port); }
        return out;
    }
    ajoute(htonl(INADDR_BROADCAST), c.port);
    ifaddrs* l = nullptr;
    if (getifaddrs(&l) == 0) {
        for (ifaddrs* i = l; i; i = i->ifa_next)
            if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET && (i->ifa_flags & IFF_UP) && (i->ifa_flags & IFF_BROADCAST) &&
                !(i->ifa_flags & IFF_LOOPBACK) && i->ifa_broadaddr)
                ajoute(((sockaddr_in*)i->ifa_broadaddr)->sin_addr.s_addr, c.port);
        freeifaddrs(l);
    }
    return out;
}
#endif
} // namespace

void installerVerrou(void (*f)(bool)) { g_verrou = f; }
void verrou(bool prendre) {
    static bool pris = false;
    if (pris == prendre) return;
    pris = prendre;
    WFS_LOG("duel réseau : verrou de diffusion %s", prendre ? "pris" : "rendu");
    if (g_verrou) g_verrou(prendre);
}

// ---------------------------------------------------------------- prise
Prise::Prise(Config c) : c_(std::move(c)), id_(idAleatoire()) {
#ifdef WFS_SOCKETS
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) { WFS_WARN("duel réseau : prise UDP impossible"); return; }
    const int un = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &un, sizeof un);
    setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &un, sizeof un);
    const int fl = fcntl(fd_, F_GETFL, 0);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons((uint16_t)c_.port); a.sin_addr.s_addr = htonl(INADDR_ANY);
    if (fl < 0 || fcntl(fd_, F_SETFL, fl | O_NONBLOCK) < 0 || bind(fd_, (sockaddr*)&a, sizeof a) < 0) {
        WFS_WARN("duel réseau : port UDP %d indisponible", c_.port);
        close(fd_); fd_ = -1; return;
    }
    reseau_ = !c_.destinations.empty() || destinations(c_).size() > 1;   // au-delà de 255.255.255.255 : une vraie interface
    WFS_LOG("duel réseau : prise UDP ouverte sur le port %d (joueur %s), réseau local %s", c_.port, id_.c_str(), reseau_ ? "présent" : "absent");
#endif
}
Prise::~Prise() {
#ifdef WFS_SOCKETS
    if (fd_ >= 0) close(fd_);
#endif
}
double Prise::horlogeMs() const { return c_.horloge ? c_.horloge() : horlogeAppareil(); }

bool Prise::lire(const std::string& msg, Recu& r) {
    if (msg.size() > duel::MSG_MAX || msg.compare(0, 5, "WFS1|") != 0) return false;
    // WFS1|code|id|horloge|charge : trois séparateurs après la version ; la charge peut contenir des « | » (pseudo)
    const size_t b = msg.find('|', 5), c = b == std::string::npos ? b : msg.find('|', b + 1), d = c == std::string::npos ? c : msg.find('|', c + 1);
    if (d == std::string::npos) return false;
    std::string code = msg.substr(5, b - 5), id = msg.substr(b + 1, c - b - 1), h = msg.substr(c + 1, d - c - 1);
    for (char& ch : code) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
    if (!duel::validCode(code) || !idOk(id) || h.empty() || h.size() > 15) return false;
    for (char ch : h) if (ch < '0' || ch > '9') return false;
    const double horloge = std::strtod(h.c_str(), nullptr);
    if (!std::isfinite(horloge)) return false;
    duel::Fields f;
    if (!duel::decode(std::string_view(msg).substr(d + 1), f)) return false;
    r.code = code; r.id = id; r.horloge = horloge; r.f = std::move(f); r.bye = duel::str(r.f, "bye") == "1";
    return true;
}

void Prise::pomper() {
#ifdef WFS_SOCKETS
    if (fd_ < 0) return;
    char buf[1024];
    for (int k = 0; k < 256; k++) {   // au plus 256 messages par image
        const ssize_t n = recv(fd_, buf, sizeof buf, 0);
        if (n < 0) break;
        Recu r;
        if (!lire(std::string(buf, (size_t)n), r)) { ignores_++; continue; }   // message invalide : ignoré
        if (r.id == id_) continue;                                               // mon propre message (diffusion)
        r.recuLocal = horlogeMs();
        r.n = recus_.count(r.id) ? recus_[r.id].n + 1 : 1;
        recus_[r.id] = std::move(r);
    }
    const double now = horlogeMs();
    for (auto it = recus_.begin(); it != recus_.end();) { if (now - it->second.recuLocal > 10000) it = recus_.erase(it); else ++it; }
#endif
}

void Prise::envoyer(const std::string& msg) {
#ifdef WFS_SOCKETS
    if (fd_ < 0 || msg.size() > duel::MSG_MAX) return;
    static thread_local std::vector<sockaddr_in> dest; static thread_local const Prise* pour = nullptr;
    if (pour != this || dest.empty()) { dest = destinations(c_); pour = this; }
    for (const sockaddr_in& a : dest) sendto(fd_, msg.data(), msg.size(), 0, (const sockaddr*)&a, sizeof a);   // erreurs ignorées
#else
    (void)msg;
#endif
}

std::shared_ptr<Prise> priseCommune(bool neuve) {
    static std::shared_ptr<Prise> p;
    if (!p || !p->ouverte() || neuve) p = std::make_shared<Prise>();   // nouvel essai si le réseau n'était pas là
    return p;
}

std::vector<SalonOuvert> salonsOuverts(const Prise& p) {
    std::vector<std::pair<double, SalonOuvert>> v;
    const double now = p.horlogeMs();
    for (const auto& [id, r] : p.recus()) {
        if (r.bye || now - r.recuLocal >= ABSENCE_MS || duel::str(r.f, "host") != "1" || duel::str(r.f, "open") != "1") continue;
        const std::string carte = duel::str(r.f, "map");
        v.push_back({r.recuLocal, {r.code, duel::peerName(r.f), carte.size() <= 16 ? carte : ""}});
    }
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    std::vector<SalonOuvert> out;
    for (size_t i = 0; i < v.size() && out.size() < 4; i++) out.push_back(v[i].second);
    return out;
}

// ---------------------------------------------------------------- transport
TransportUdp::TransportUdp(std::shared_ptr<Prise> p, std::string code) : p_(std::move(p)), code_(std::move(code)) {
    for (char& ch : code_) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
}

void TransportUdp::envoyerMoi() const {
    duel::Fields f = mine_;
    if (duel::str(f, "host") == "1") {   // RG-03 / RG-08 : salon annoncé tant que l'hôte est seul et hors course
        bool seul = true;
        const double now = p_->horlogeMs();
        for (const auto& [id, r] : p_->recus()) if (r.code == code_ && !r.bye && now - r.recuLocal < ABSENCE_MS) seul = false;
        f["open"] = seul && duel::str(f, "st") != "race" && duel::str(f, "bye") != "1" ? "1" : "0";
    }
    char h[32]; std::snprintf(h, sizeof h, "%.0f", std::max(0.0, p_->horlogeMs()));
    std::string msg = "WFS1|" + code_ + "|" + p_->monId() + "|" + h + "|" + duel::encode(f);
    if (msg.size() > duel::MSG_MAX) { WFS_WARN("duel réseau : présence trop longue (%zu octets), non envoyée", msg.size()); return; }
    p_->envoyer(msg);
    dernierEnvoi_ = p_->horlogeMs();
}

void TransportUdp::presence(const duel::Fields& f) {
    if (parti_) return;
    for (const auto& [k, v] : f) mine_[k] = v;
    envoyerMoi();
}

void TransportUdp::suivre() const {
    p_->pomper();
    const double now = p_->horlogeMs();
    const double cadence = (duel::str(mine_, "st") == "race" ? CADENCE_COURSE : CADENCE_SALON) * 1000;
    if (!parti_ && now - dernierEnvoi_ >= cadence) envoyerMoi();
    if (duel::str(mine_, "host") == "1") { decalage_ = 0; return; }   // l'hôte donne l'heure
    // RG-07 : décalage = horloge de l'hôte − horloge locale à la réception, moyenne des 10 derniers messages de l'hôte
    for (const auto& [id, r] : p_->recus()) {
        if (r.code != code_ || duel::str(r.f, "host") != "1" || r.bye || r.n == vuHote_) continue;
        vuHote_ = r.n;
        echantillons_.push_back(r.horloge - r.recuLocal);
        while (echantillons_.size() > 10) echantillons_.pop_front();
        double s = 0; for (double e : echantillons_) s += e;
        decalage_ = s / (double)echantillons_.size();
    }
}

std::vector<duel::PeerView> TransportUdp::peers() const {
    suivre();
    std::vector<duel::PeerView> out;
    const double now = p_->horlogeMs();
    for (const auto& [id, r] : p_->recus()) {
        if (r.code != code_ || r.bye || now - r.recuLocal >= ABSENCE_MS) continue;   // RG-06
        duel::PeerView v; v.id = id; v.f = r.f; v.updatedAt = r.recuLocal + decalage_;
        out.push_back(std::move(v));
    }
    return out;
}

double TransportUdp::nowMs() const { suivre(); return p_->horlogeMs() + decalage_; }

void TransportUdp::leave() {
    if (parti_) return;
    mine_["bye"] = "1";   // RG-06 : retrait immédiat chez les autres
    envoyerMoi();
    parti_ = true;
}

} // namespace wfs::reseau
