// WaForSpeed — « Duel entre amis » par Internet (WFS-112, épic E3) : client du relais WebSocket (serveur/, Cloudflare
// Workers). Marche en 4G et derrière toutes les box : chaque joueur ouvre une WebSocket sécurisée vers le relais, qui
// renvoie les présences aux autres joueurs du même code (protocole : serveur/src/salon.js).
//   - aucune bibliothèque tierce : WebSocket maison (RFC 6455, texte seulement) sur un flux d'octets ; flux TCP
//     (POSIX) pour le relais local des tests, flux TLS de la plateforme sur Android (javax.net.ssl, reseau_android.cpp) ;
//   - le réseau tourne sur le fil du flux ; le jeu ne fait que lire des files sous verrou à chaque image (jamais d'attente) ;
//   - reconnexion automatique (RG-05) : nouvel essai toutes les 1,5 s pendant 30 s, même identifiant (le relais garde la
//     place) ; ping toutes les 2 s (affiché), horloge commune = horloge du relais (décalage du ping le plus court) ;
//   - données reçues jamais crues (RG-06) : tailles, identifiants, nombres et présences bornés ; message invalide ignoré.
#pragma once
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "duel.h"

namespace wfs::reseau {

// ---------------------------------------------------------------- adresse du relais (RG-12 : une constante du jeu)
// Adresse donnée par Cloudflare au déploiement (serveur/LISEZMOI.md) : https://waforspeed-relais.<sous-domaine>.workers.dev
constexpr const char* RELAIS_HOTE = "waforspeed-relais.wlfaleh.workers.dev";
constexpr int RELAIS_PORT = 443;
struct AdresseRelais { std::string hote = RELAIS_HOTE; int port = RELAIS_PORT; bool tls = true; };
AdresseRelais& adresseRelais();   // celle du jeu ; le banc d'essai et les tests y mettent le relais local (ws://127.0.0.1)

constexpr double RECONNEXION_MS = 30000;      // RG-05 : 30 s pour revenir après une coupure
constexpr double ESSAI_MS = 1500;             // délai entre deux essais de connexion
constexpr double CONNEXION_MS = 10000;        // première connexion : « relais injoignable » au-delà
constexpr double PING_MS = 2000, SILENCE_MS = 7000;   // ping ; sans rien reçu depuis 7 s : connexion jugée morte
constexpr double GROUPAGE_RELAIS_MS = 30;     // au plus une présence envoyée toutes les 30 ms
constexpr size_t MESSAGE_MAX = 1024;          // message du relais plus long : ignoré
constexpr size_t JOUEURS_MAX = 8;             // au plus 8 autres joueurs retenus (le relais en accepte 4)

// lien d'invitation (RG-02) : https://<relais>/d/<code> (page qui ouvre le jeu par waforspeed://duel/<code>)
std::string lienInvitation(const std::string& code, const AdresseRelais& a = adresseRelais());
std::string texteInvitation(const std::string& code);   // message partagé (WhatsApp, SMS…) : code + lien
// code d'un lien reçu (waforspeed://duel/k7qx, https://…/d/K7QX, intent://duel/…) ; faux si le lien est invalide
bool lireLienInvitation(std::string_view lien, std::string& code);

// ---------------------------------------------------------------- flux d'octets (TCP ou TLS), lu sans attendre
class Flux {
public:
    enum class Etat { Connexion, Ouvert, Ferme };
    virtual ~Flux() = default;
    virtual Etat etat() const = 0;
    virtual void envoyer(const std::string& octets) = 0;   // mis en file (envoyé dès que la connexion est ouverte)
    virtual std::string lire() = 0;                        // tout ce qui est arrivé depuis la dernière lecture
    virtual void fermer() = 0;                             // ferme après avoir envoyé ce qui est en file
    virtual void couper() { fermer(); }                    // essai : coupure nette (perte de réseau)
};
using FabriqueFlux = std::function<std::unique_ptr<Flux>(const std::string& hote, int port, bool tls)>;
std::unique_ptr<Flux> fluxTcp(const std::string& hote, int port);   // POSIX, sur son propre fil
// TLS : fourni par la plateforme (Android : reseau_android.cpp) ; sans plateforme, le flux TLS est fermé d'emblée
void installerFluxTls(std::function<std::unique_ptr<Flux>(const std::string& hote, int port)> f);
std::unique_ptr<Flux> fluxDefaut(const std::string& hote, int port, bool tls);

// ---------------------------------------------------------------- client WebSocket (texte seulement)
class ClientWs {
public:
    enum class Etat { Poignee, Ouvert, Ferme };
    ClientWs(std::unique_ptr<Flux> f, const std::string& hote, const std::string& chemin);
    Etat etat();
    void envoyer(const std::string& texte);
    std::vector<std::string> recevoir();       // messages texte complets arrivés
    void fermer();
    void couper() { if (f_) f_->couper(); }
    int erreurs() const { return erreurs_; }   // trames refusées (tests)
    // essai : octets bruts reçus du serveur, tels quels (trames hostiles)
    void injecter(const std::string& octets) { pomper(octets); }
    static constexpr size_t TRAME_MAX = 4096;
private:
    void pomper(const std::string& nouveaux);
    void echec(const char* raison);
    std::unique_ptr<Flux> f_;
    std::string cle_, tampon_, morceaux_;
    Etat etat_ = Etat::Poignee;
    std::deque<std::string> recus_;
    int erreurs_ = 0;
};
std::string sha1(std::string_view donnees);   // 20 octets (poignée de main WebSocket)
std::string base64(std::string_view donnees);

// ---------------------------------------------------------------- transport du duel par le relais
class TransportRelais : public duel::Transport {
public:
    // horloge locale en ms (vide : horloge monotone de l'appareil) ; id vide : identifiant aléatoire de 12 caractères
    TransportRelais(FabriqueFlux f, AdresseRelais a, std::string code, bool hote, std::function<double()> horloge = {}, std::string id = "");
    ~TransportRelais() override;
    void presence(const duel::Fields& f) override;
    std::vector<duel::PeerView> peers() const override;
    double nowMs() const override;             // horloge du relais (horloge locale + décalage)
    void leave() override;
    duel::Lien lien() const override;
    int pingMs() const override { pomper(); return ping_; }
    std::string raison() const override { return raison_; }
    double expireDansMs() const override;      // validité restante du code (RG-06), −1 si inconnue
    double reconnexionRestanteMs() const override;
    int reconnexions() const { return reconnexions_; }
    int ignores() const { return ignores_; }
    const std::string& monId() const { return id_; }
    void couper();                             // essai : coupure du réseau (la reconnexion doit suivre)
private:
    void pomper() const;
    void connecter() const;
    void traiter(const std::string& m) const;
    void envoyerPresence() const;
    double local() const;
    FabriqueFlux fabrique_;
    AdresseRelais a_;
    std::string code_, id_;
    bool hote_;
    std::function<double()> horloge_;
    duel::Fields moi_;
    mutable std::unique_ptr<ClientWs> ws_;
    mutable duel::Lien etat_ = duel::Lien::Connexion;
    mutable std::string raison_;
    mutable bool ouvert_ = false, attente_ = false, parti_ = false;
    mutable double debut_ = 0, perte_ = 0, prochainEssai_ = 0, dernierEnvoi_ = -1e18, dernierPing_ = -1e18, dernierRecu_ = 0;
    mutable double decalage_ = 0, expire_ = -1;
    mutable int ping_ = -1, reconnexions_ = 0, ignores_ = 0;
    mutable bool aEuDecalage_ = false, accepte_ = false;   // accepte_ : le relais nous a déjà accueillis (OK)
    mutable std::deque<std::pair<double, double>> echantillons_;   // (aller-retour, décalage)
    struct Joueur { duel::Fields f; double maj = 0; };
    mutable std::map<std::string, Joueur> table_;
};

// ---------------------------------------------------------------- plateforme : partage et lien profond (Android)
// « Inviter » (RG-02) : feuille de partage d'Android (WhatsApp, SMS…) ; sans plateforme : rien (faux), le texte est journalisé
void installerPartage(void (*f)(const std::string& texte));
bool partager(const std::string& texte);
const std::string& dernierPartage();   // banc, tests
// lien profond reçu (le jeu ouvert par le lien d'invitation) : lu une fois ; vide s'il n'y en a pas
void installerLienRecu(std::string (*f)());
std::string prendreLienRecu();
void lienRecuEssai(const std::string& lien);   // banc, tests : comme si le jeu avait été ouvert par ce lien

// ---------------------------------------------------------------- relais local (tests, banc d'essai ; pas sur Android)
// lance « node <script> 0 --parent [options] » et lit le port choisi ; arrêté à la destruction
class RelaisLocal {
public:
    ~RelaisLocal();
    bool demarrer(const std::string& script, const std::vector<std::string>& options = {});
    int port() const { return port_; }
    void arreter();
private:
    int pid_ = -1, port_ = 0, entree_ = -1;
};

} // namespace wfs::reseau
