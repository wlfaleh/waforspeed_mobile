// WaForSpeed — duel par Internet (WFS-89, épic E3) : salon par code, compatible avec le jeu web (PeerJS 1.5.4).
// Port de joinP2P (js/net/p2p.js) : le salon de code « abcd » a pour relais le pair PeerJS « waforspeed-wfs-abcd-v1 ».
//   1) on essaie de devenir le relais (le premier arrivé l'est) ; 2) si l'identifiant est pris (ID-TAKEN), on ouvre un pair
//   à identifiant aléatoire et on se connecte au relais (canal fiable) ; 12 s sans succès : échec.
// Messages (BinaryPack, src/game/binarypack.*) : l'invité envoie {t:'p', presence} au relais (groupé : au plus un envoi
// toutes les 30 ms) ; le relais renvoie {t:'all', list:[[id, presence], …]} à tous. Les présences du jeu web (nombres,
// host: true, map + tod) sont converties en duel::Fields et inversement ; un message invalide est ignoré.
// Le réseau (signalisation WebSocket + WebRTC, libdatachannel) tourne sur ses propres fils : les événements sont mis en
// file sous verrou et traités sur le fil du jeu à chaque lecture (peers, nowMs) — jamais d'attente dans la boucle de jeu.
#pragma once
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "duel.h"

namespace wfs::reseau {

// ---------------------------------------------------------------- pair PeerJS (réseau réel ou simulé)
// Les rappels peuvent venir d'un autre fil (libdatachannel) : celui qui les reçoit les met en file.
struct EvenementsPair {
    std::function<void(const std::string& monId)> ouvert;           // OPEN : identifiant accepté par le serveur
    std::function<void()> idPris;                                    // ID-TAKEN : le relais de ce salon existe déjà
    std::function<void(const std::string& erreur)> erreur;           // serveur injoignable, pair introuvable…
    std::function<void(const std::string& pair)> connexion;          // canal de données ouvert avec ce pair
    std::function<void(const std::string& pair, std::vector<uint8_t> octets)> donnees;
    std::function<void(const std::string& pair)> fermeture;          // canal fermé (départ, réseau perdu)
};
class Pair {
public:
    virtual ~Pair() = default;
    virtual void ouvrir(const std::string& id, EvenementsPair ev) = 0;   // id vide : identifiant aléatoire
    virtual void connecter(const std::string& dst) = 0;                  // canal fiable vers dst (PeerJS : connect)
    virtual void envoyer(const std::string& pair, const std::vector<uint8_t>& octets) = 0;
    virtual void fermer() = 0;                                           // LEAVE, canaux fermés ; plus aucun rappel ensuite
};
using FabriquePair = std::function<std::unique_ptr<Pair>()>;
// fabrique utilisée par le salon : PeerJS réel (si compilé avec libdatachannel), sinon nulle (« Internet indisponible ») ;
// le banc et les tests y mettent un réseau simulé
void installerFabriquePair(FabriquePair f);
FabriquePair fabriquePair();
std::unique_ptr<Pair> pairPeerJsReel();   // nullptr si le jeu est compilé sans libdatachannel (WFS_INTERNET)

// réseau PeerJS simulé en mémoire (RG-11) : serveur de signalisation (identifiants, ID-TAKEN) et canaux, mêmes octets
// BinaryPack que le vrai réseau ; un seul fil. hors_ligne : chaque ouverture échoue (pas d'Internet).
struct ReseauMemoire : std::enable_shared_from_this<ReseauMemoire> {
    bool horsLigne = false;
    std::map<std::string, EvenementsPair*> pairs;   // identifiant → rappels du pair ouvert
    std::map<std::string, std::vector<std::string>> liens;
    int compteur = 0;
    FabriquePair fabrique();
};

// ---------------------------------------------------------------- transport du duel par Internet
const char* const PEERJS_SERVEUR = "0.peerjs.com";
inline std::string idRelais(const std::string& code) { return "waforspeed-wfs-" + code + "-v1"; }   // room.join('wfs-' + code)
constexpr double DELAI_CONNEXION_MS = 12000, GROUPAGE_MS = 30;

class TransportPeerJs : public duel::Transport {
public:
    enum class Etat { Connexion, Pret, Echec, Perdu };
    TransportPeerJs(FabriquePair fabrique, std::string code);
    ~TransportPeerJs() override;
    void presence(const duel::Fields& f) override;
    std::vector<duel::PeerView> peers() const override;
    double nowMs() const override;            // horloge murale de l'appareil (Date.now() du jeu web)
    void leave() override;
    Etat etat() const { pomper(); return etat_; }
    bool relais() const { return relais_; }
    const std::string& monId() const { return monId_; }
private:
    struct Evt { int type; std::string a; std::vector<uint8_t> o; };
    EvenementsPair rappels() const;
    void pomper() const;                      // traite la file d'événements (fil du jeu)
    void traiter(const Evt& e) const;
    void diffuser() const;                    // relais : {t:'all', list} à tous
    void envoyerMoi() const;                  // invité : {t:'p', presence} (groupé à 30 ms)
    FabriquePair fabrique_;
    std::string code_;
    mutable std::unique_ptr<Pair> pair_;
    mutable std::mutex verrou_;
    mutable std::deque<Evt> file_;
    std::shared_ptr<bool> vivant_;            // les rappels tardifs d'un pair fermé sont ignorés
    duel::Fields moi_;
    mutable std::string monId_;
    mutable Etat etat_ = Etat::Connexion;
    mutable bool relais_ = false, envoiEnAttente_ = false, parti_ = false;
    mutable double debut_ = 0, dernierEnvoi_ = -1e18;
    struct Joueur { duel::Fields f; double maj = 0; };
    mutable std::map<std::string, Joueur> table_;    // autres joueurs (relais : présences reçues ; invité : liste du relais)
    mutable std::vector<std::string> connectes_;      // relais : canaux ouverts
};

// conversions des présences (compatibilité avec le jeu web) — publiques pour les tests
std::vector<uint8_t> messagePresence(const duel::Fields& f);   // {t:'p', presence}
bool lirePresenceWeb(const void* valeurBp, duel::Fields& out);  // objet BinaryPack → Fields (valeurBp : const bp::Valeur*)

// ---------------------------------------------------------------- deux transports à la fois (RG-07 du ticket)
// « Créer un salon » / « Rejoindre » ouvrent le Wi-Fi local (UDP) et Internet avec le même code : le premier qui voit un
// second joueur est gardé, l'autre est quitté. Avant : présences envoyées aux deux, horloge du Wi-Fi local.
class TransportDouble : public duel::Transport {
public:
    TransportDouble(std::unique_ptr<duel::Transport> local, std::unique_ptr<TransportPeerJs> internet);
    void presence(const duel::Fields& f) override;
    std::vector<duel::PeerView> peers() const override;
    double nowMs() const override;
    void leave() override;
    const TransportPeerJs* internet() const { return internet_.get(); }
    int choisi() const { return choisi_; }   // 0 : pas encore, 1 : Wi-Fi local, 2 : Internet
private:
    mutable std::unique_ptr<duel::Transport> local_;
    mutable std::unique_ptr<TransportPeerJs> internet_;
    mutable int choisi_ = 0;
};

} // namespace wfs::reseau
