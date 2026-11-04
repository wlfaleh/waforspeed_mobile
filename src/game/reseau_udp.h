// WaForSpeed — duel entre deux appareils sur le même Wi-Fi (WFS-83, épic E3) : transport UDP du réseau local.
// Implémente duel::Transport (WFS-21) sans serveur : chaque joueur diffuse sa présence sur le port 47321 ; message texte
// « WFS1|<code>|<id>|<horlogeMs>|<encode(champs)> » de 512 octets au plus (duel::MSG_MAX). Principe de js/net/p2p.js
// (le créateur est l'hôte, les présences circulent en texte), sans PeerJS ni WebRTC : la diffusion sur le Wi-Fi suffit.
//   - prise non bloquante, lue à chaque image (pompe) : aucun thread, aucun blocage de la boucle de jeu ;
//   - présence renvoyée toutes les 0,1 s en course, 0,5 s hors course ; joueur retiré 3 s après son dernier message,
//     tout de suite s'il envoie « bye=1 » (leave) ;
//   - horloge commune = celle de l'hôte : l'invité ajoute la moyenne des 10 derniers décalages (hôte − local) ;
//   - tout message invalide (version, code, identifiant, horloge, longueur, champs refusés par duel::decode) est ignoré.
#pragma once
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "duel.h"

namespace wfs::reseau {

constexpr int PORT = 47321;
constexpr double ABSENCE_MS = 3000;          // RG-06 : joueur retiré après 3 s sans message
constexpr double CADENCE_COURSE = 0.1, CADENCE_SALON = 0.5;   // RG-05 (s)

struct Config {
    int port = PORT;                                       // port d'écoute
    std::vector<std::pair<std::string, int>> destinations; // vide : diffusion (255.255.255.255 + chaque interface) sur `port`
    std::function<double()> horloge;                       // horloge locale (ms) ; vide : horloge monotone de l'appareil
};

// dernier message reçu d'un joueur (tous salons confondus)
struct Recu { std::string code, id; double horloge = 0, recuLocal = 0; duel::Fields f; bool bye = false; uint32_t n = 0; };

// prise UDP partagée : une par appareil (le salon et le transport du duel lisent la même)
class Prise {
public:
    explicit Prise(Config c = {});
    ~Prise();
    Prise(const Prise&) = delete; Prise& operator=(const Prise&) = delete;
    bool ouverte() const { return fd_ >= 0; }
    // réseau local utilisable : prise ouverte et au moins une interface IPv4 avec diffusion (Wi-Fi), ou destinations données
    bool reseau() const { return fd_ >= 0 && reseau_; }
    void pomper();                                         // lit tout ce qui est arrivé, sans bloquer
    void envoyer(const std::string& msg);
    double horlogeMs() const;                              // horloge locale (ms)
    const std::string& monId() const { return id_; }
    const std::map<std::string, Recu>& recus() const { return recus_; }
    int ignores() const { return ignores_; }               // messages refusés (banc, tests)
    // analyse d'un message reçu (public pour les tests) : faux si le message est invalide
    static bool lire(const std::string& msg, Recu& r);
private:
    Config c_;
    int fd_ = -1;
    bool reseau_ = false;
    std::string id_;
    std::map<std::string, Recu> recus_;
    int ignores_ = 0;
};
// prise de l'application (port 47321, diffusion), à la demande ; neuve : nouvel essai (Wi-Fi revenu)
std::shared_ptr<Prise> priseCommune(bool neuve = false);

// salons ouverts annoncés sur le Wi-Fi (RG-02, RG-08) : hôte seul dans son salon, vu depuis moins de 3 s, 4 au plus
struct SalonOuvert { std::string code, hote, carte; };
std::vector<SalonOuvert> salonsOuverts(const Prise& p);

class TransportUdp : public duel::Transport {
public:
    TransportUdp(std::shared_ptr<Prise> p, std::string code);
    void presence(const duel::Fields& f) override;
    std::vector<duel::PeerView> peers() const override;
    double nowMs() const override;
    void leave() override;
    double decalage() const { return decalage_; }          // horloge de l'hôte − horloge locale (ms), 0 pour l'hôte
private:
    void envoyerMoi() const;
    void suivre() const;                                    // pompe la prise, renvoie la présence à la cadence, décalage
    std::shared_ptr<Prise> p_;
    std::string code_;
    duel::Fields mine_;
    mutable double dernierEnvoi_ = -1e18, decalage_ = 0;
    mutable std::deque<double> echantillons_;
    mutable uint32_t vuHote_ = 0;
    bool parti_ = false;
};

// verrou de diffusion du Wi-Fi (Android : WifiManager.MulticastLock, reseau_android.cpp) : pris à l'ouverture du salon,
// rendu quand on quitte le duel ; sans plateforme branchée, rien
void installerVerrou(void (*f)(bool prendre));
void verrou(bool prendre);

} // namespace wfs::reseau
