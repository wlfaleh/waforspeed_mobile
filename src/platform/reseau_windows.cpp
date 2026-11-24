// WaForSpeed — version PC Windows (WFS-604, épic E24) : duel en ligne PC ↔ téléphone par le relais (WFS-112).
// Le client WebSocket du jeu (ClientWs, reseau_relais.cpp) parle à un « flux » d'octets TLS, fourni par la plateforme
// (Android : javax.net.ssl). Sous Windows, le flux passe par WinHTTP (bibliothèque du système : TLS, certificats et
// proxy de Windows, aucune bibliothèque tierce) et son API WebSocket (Windows 8 et plus) :
//   - la poignée de main HTTP écrite par ClientWs est lue ici (chemin, clé), WinHTTP fait la vraie poignée de main
//     avec le relais, puis le flux rend à ClientWs la réponse « 101 » attendue (Sec-WebSocket-Accept calculé sur sa clé) ;
//   - les trames du client (masquées) sont décodées et leur contenu envoyé par WinHttpWebSocketSend ;
//   - chaque message reçu (WinHttpWebSocketReceive) est rendu à ClientWs comme une trame de serveur (non masquée).
// Le protocole du relais et le jeu ne changent pas : un PC et un téléphone se retrouvent dans le même salon par le code.
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "../core/log.h"
#include "../game/reseau_relais.h"

namespace wfs::reseau {
namespace {

std::wstring large(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

// état partagé avec les fils (gardé vivant après la destruction du flux)
struct EtatWs {
    std::mutex m;
    std::condition_variable cv;
    std::string brut;                     // octets écrits par ClientWs, pas encore décodés
    std::deque<std::pair<bool, std::string>> aEnvoyer;   // (binaire, contenu)
    std::string recu;                     // octets rendus à ClientWs
    std::atomic<int> etat{(int)Flux::Etat::Connexion};
    std::atomic<bool> fin{false}, fermerApres{false}, poigneeLue{false};
    std::string chemin, cle;
    HINTERNET session = nullptr, connexion = nullptr, ws = nullptr;
    ~EtatWs() {
        if (ws) WinHttpCloseHandle(ws);
        if (connexion) WinHttpCloseHandle(connexion);
        if (session) WinHttpCloseHandle(session);
    }
};

// trame de serveur (non masquée) pour ClientWs
std::string trameServeur(bool binaire, const std::string& p) {
    std::string t;
    t += (char)(0x80 | (binaire ? 2 : 1));
    const uint64_t n = p.size();
    if (n < 126) t += (char)n;
    else if (n < 65536) { t += (char)126; t += (char)(n >> 8); t += (char)(n & 255); }
    else { t += (char)127; for (int k = 7; k >= 0; k--) t += (char)((n >> (8 * k)) & 255); }
    return t + p;
}

// décode les trames complètes du client (masquées) ; renvoie faux sur une trame de fermeture
bool decoder(EtatWs& e) {   // e.m tenu
    std::string& b = e.brut;
    for (;;) {
        if (b.size() < 2) return true;
        const uint8_t b0 = (uint8_t)b[0], b1 = (uint8_t)b[1];
        const int op = b0 & 15;
        uint64_t n = b1 & 127; size_t i = 2;
        if (n == 126) { if (b.size() < 4) return true; n = (uint64_t)(uint8_t)b[2] << 8 | (uint8_t)b[3]; i = 4; }
        else if (n == 127) { if (b.size() < 10) return true; n = 0; for (int k = 0; k < 8; k++) n = n << 8 | (uint8_t)b[2 + k]; i = 10; }
        const bool masque = b1 & 0x80;
        const size_t debut = i + (masque ? 4 : 0);
        if (b.size() < debut + n) return true;
        std::string p = b.substr(debut, (size_t)n);
        if (masque) for (size_t k = 0; k < p.size(); k++) p[k] = (char)(p[k] ^ b[i + (k & 3)]);
        b.erase(0, debut + (size_t)n);
        if (op == 8) return false;                                   // fermeture demandée par le jeu
        if (op == 1 || op == 2 || op == 0) e.aEnvoyer.emplace_back(op == 2, std::move(p));
        // ping / pong du client : WinHTTP gère lui-même les siens avec le relais
    }
}

void boucleReception(std::shared_ptr<EtatWs> e) {
    std::string message; std::string tampon(16384, '\0');
    while (!e->fin) {
        DWORD lu = 0; WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        const DWORD r = WinHttpWebSocketReceive(e->ws, tampon.data(), (DWORD)tampon.size(), &lu, &type);
        if (r != NO_ERROR) { if (!e->fin) WFS_WARN("duel PC : réception coupée (%lu)", (unsigned long)r); break; }
        if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) break;
        message.append(tampon.data(), lu);
        if (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE || type == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) continue;
        if (message.size() > 262144) { message.clear(); continue; }
        { std::lock_guard<std::mutex> l(e->m); if (e->recu.size() < 262144) e->recu += trameServeur(type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, message); }
        message.clear();
    }
    e->etat = (int)Flux::Etat::Ferme;
    e->fin = true; e->cv.notify_all();
}

void boucle(std::shared_ptr<EtatWs> e, std::string hote, int port) {
    // 1. attendre la poignée de main écrite par ClientWs (chemin et clé)
    {
        std::unique_lock<std::mutex> l(e->m);
        e->cv.wait_for(l, std::chrono::seconds(10), [&] { return e->fin.load() || e->brut.find("\r\n\r\n") != std::string::npos; });
        const size_t f = e->brut.find("\r\n\r\n");
        if (f == std::string::npos) { e->etat = (int)Flux::Etat::Ferme; return; }
        const std::string tete = e->brut.substr(0, f);
        e->brut.erase(0, f + 4);
        const size_t g = tete.find(' '), h = g == std::string::npos ? g : tete.find(' ', g + 1);
        e->chemin = (g != std::string::npos && h != std::string::npos) ? tete.substr(g + 1, h - g - 1) : "/";
        const size_t k = tete.find("Sec-WebSocket-Key:");
        if (k != std::string::npos) {
            size_t d = k + 18; while (d < tete.size() && tete[d] == ' ') d++;
            size_t z = tete.find("\r\n", d); if (z == std::string::npos) z = tete.size();
            e->cle = tete.substr(d, z - d);
        }
    }
    // 2. vraie poignée de main avec le relais (TLS de Windows)
    e->session = WinHttpOpen(L"WaForSpeed", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!e->session) e->session = WinHttpOpen(L"WaForSpeed", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (e->session) WinHttpSetTimeouts(e->session, 10000, 10000, 10000, 0);   // réception sans limite (le relais envoie sa présence)
    e->connexion = e->session ? WinHttpConnect(e->session, large(hote).c_str(), (INTERNET_PORT)port, 0) : nullptr;
    HINTERNET req = e->connexion ? WinHttpOpenRequest(e->connexion, L"GET", large(e->chemin).c_str(), nullptr, WINHTTP_NO_REFERER,
                                                      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    bool ok = req && WinHttpSetOption(req, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0)
                  && WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0)
                  && WinHttpReceiveResponse(req, nullptr);
    DWORD statut = 0, taille = sizeof statut;
    if (ok) ok = WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statut, &taille, WINHTTP_NO_HEADER_INDEX) && statut == 101;
    if (ok) e->ws = WinHttpWebSocketCompleteUpgrade(req, 0);
    if (req) WinHttpCloseHandle(req);
    if (!e->ws) {
        WFS_WARN("duel PC : relais %s injoignable (statut %lu, erreur %lu)", hote.c_str(), (unsigned long)statut, (unsigned long)GetLastError());
        e->etat = (int)Flux::Etat::Ferme; return;
    }
    WFS_LOG("duel PC : relais %s ouvert (%s)", hote.c_str(), e->chemin.c_str());
    // 3. réponse « 101 » rendue à ClientWs (Sec-WebSocket-Accept calculé sur sa propre clé)
    {
        std::lock_guard<std::mutex> l(e->m);
        e->recu = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
                  base64(sha1(e->cle + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) + "\r\n\r\n" + e->recu;
        e->poigneeLue = true;
    }
    e->etat = (int)Flux::Etat::Ouvert;
    std::thread(boucleReception, e).detach();
    // 4. envoi : trames du jeu décodées puis envoyées une à une
    for (;;) {
        std::pair<bool, std::string> m; bool fermer = false;
        {
            std::unique_lock<std::mutex> l(e->m);
            if (!decoder(*e)) fermer = true;
            e->cv.wait_for(l, std::chrono::milliseconds(20), [&] { return e->fin.load() || !e->aEnvoyer.empty() || !e->brut.empty() || e->fermerApres.load(); });
            if (!decoder(*e)) fermer = true;
            if (e->aEnvoyer.empty()) { if (e->fin || fermer || e->fermerApres) break; continue; }
            m = std::move(e->aEnvoyer.front()); e->aEnvoyer.pop_front();
        }
        const DWORD r = WinHttpWebSocketSend(e->ws, m.first ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE : WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                             m.second.empty() ? nullptr : (PVOID)m.second.data(), (DWORD)m.second.size());
        if (r != NO_ERROR) { WFS_WARN("duel PC : envoi coupé (%lu)", (unsigned long)r); break; }
    }
    WinHttpWebSocketClose(e->ws, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
    e->fin = true;
    e->etat = (int)Flux::Etat::Ferme;
}

class FluxWsWindows : public Flux {
public:
    FluxWsWindows(const std::string& hote, int port) : e_(std::make_shared<EtatWs>()) {
        std::thread([e = e_, hote, port] { boucle(e, hote, port); }).detach();
    }
    ~FluxWsWindows() override { if (!e_->fermerApres) { e_->fin = true; } e_->cv.notify_all(); }
    Etat etat() const override { return (Etat)e_->etat.load(); }
    void envoyer(const std::string& o) override {
        { std::lock_guard<std::mutex> l(e_->m); if (e_->brut.size() < 65536) e_->brut += o; }
        e_->cv.notify_all();
    }
    std::string lire() override {
        std::lock_guard<std::mutex> l(e_->m);
        if (!e_->poigneeLue) return {};   // rien avant la réponse « 101 »
        std::string r; r.swap(e_->recu); return r;
    }
    void fermer() override { e_->fermerApres = true; e_->cv.notify_all(); }
    void couper() override { e_->fin = true; e_->cv.notify_all(); }
private:
    std::shared_ptr<EtatWs> e_;
};

} // namespace

void installerReseauWindows() {
    installerFluxTls([](const std::string& hote, int port) -> std::unique_ptr<Flux> { return std::make_unique<FluxWsWindows>(hote, port); });
}

} // namespace wfs::reseau
#endif
