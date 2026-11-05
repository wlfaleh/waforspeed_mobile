// WaForSpeed — duel en ligne (WFS-21, épic E3) : salon, présence, adversaires (fantômes), contacts, classement.
// Port de js/net/duel.js (joinRoom, setReady, checkStart, startDuelRace, sendRacePresence, updateGhosts, finishDuel,
// renderDuelResult), de js/game/update.js (hitGhosts) et de js/ui/hud.js (renderBoard) du jeu web.
// Le réseau passe par une interface Transport (RG-07) : ce ticket fournit un transport en mémoire (deux joueurs dans
// le même processus, pour les tests et le banc) ; un transport réseau (sockets, permission INTERNET) viendra par E7.
// Toutes les données reçues sont bornées et nettoyées (RG-04, RG-07) : un message hostile ne fait jamais planter.
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "cars.h"
#include "track.h"

namespace wfs {

struct Player; struct Course;

namespace duel {

constexpr int KMS[4] = {2, 4, 6, 10};                        // DUEL_KMS
constexpr double KM = 1000.0 / M;                            // 1 km en unités du jeu
constexpr const char* CODE_CHARS = "abcdefghjkmnpqrstuvwxyz23456789";
constexpr size_t NICK_MAX = 14;                              // pseudo ≤ 14 caractères
constexpr size_t MSG_MAX = 512;                              // message plus long : refusé en entier
constexpr double PRESENCE_DT = 0.1;                          // présence envoyée toutes les 0,1 s (RG-03)

// hashStr du jeu web (FNV-1a 32 bits sur charCodeAt(0) de chaque caractère)
uint32_t hashStr(std::string_view s);
// graine de la manche (RG-02) : hashStr(code) + round × 977, ramenée à 32 bits comme mulberry32
inline uint32_t raceSeed(std::string_view code, int round) { return hashStr(code) + (uint32_t)round * 977u; }
bool validCode(std::string_view code);                       // 4 caractères [a-z0-9] (après passage en minuscules)
std::string makeCode(uint32_t (*rnd)());                     // createRoom : 4 caractères de CODE_CHARS
std::string cleanName(std::string_view n);                   // sans caractère de contrôle, espaces retirés, ≤ 14 caractères

// ---------- présence (champs envoyés par chaque joueur, comme sub.presence du jeu web) ----------
using Fields = std::map<std::string, std::string>;
// encodage texte « clé=valeur;… » (valeurs échappées) ; decode renvoie false (et rien) si le message est invalide
std::string encode(const Fields& f);
bool decode(std::string_view msg, Fields& out);
// lectures bornées (num, peerName, peerCar du jeu web)
double num(const Fields& f, const char* k, double def = 0);
std::string str(const Fields& f, const char* k);
std::string peerName(const Fields& f);                       // « Pilote » par défaut
const char* peerCar(const Fields& f);                        // voiture connue, sinon « golf »

struct PeerView { std::string id; Fields f; double updatedAt = 0; };   // updatedAt : ms (horloge du transport)

// ---------- transport (RG-07) ----------
class Transport {
public:
    virtual ~Transport() = default;
    virtual void presence(const Fields& f) = 0;               // fusionne ces champs dans ma présence
    virtual std::vector<PeerView> peers() const = 0;          // les autres joueurs du salon
    virtual double nowMs() const = 0;                         // horloge commune (ms)
    virtual void leave() = 0;
};

// salon en mémoire : chaque client y écrit sa présence sous forme de texte (encode), lue et décodée par les autres
struct SalonMemoire {
    struct Entree { std::string msg; double updatedAt = 0; bool present = true; };
    std::map<std::string, Entree> joueurs;
    double now = 0;                                           // horloge commune (ms), avancée par le test ou le banc
    void injecter(const std::string& id, const std::string& msgBrut);   // essai : message hostile tel quel
};
class TransportMemoire : public Transport {
public:
    TransportMemoire(std::shared_ptr<SalonMemoire> salon, std::string id);
    void presence(const Fields& f) override;
    std::vector<PeerView> peers() const override;
    double nowMs() const override { return salon_->now; }
    void leave() override;
private:
    std::shared_ptr<SalonMemoire> salon_; std::string id_; Fields mine_;
};

// ---------- adversaire vu de chez moi (ghosts du jeu web) ----------
struct Ghost {
    double rd = 0, x = 0, h = 0, v = 0, ft = 0, hitCd = 0;    // distance lissée, écart latéral, cap, vitesse, arrivée (ms)
    int b = 0; bool done = false;
    std::string name, car, paint;
};

enum class Status { Lobby, Ready, Race, Done };
const char* statusText(Status s);                             // « lobby », « ready », « race », « done »

struct BoardRow { std::string name; double d = 0; bool me = false, done = false; double ft = 0; };

// effets d'un contact pour le son, les étincelles et les règles de course (bump / accident)
struct ContactEvents { int frottements = 0, chocs = 0; double impact = 0; bool accident = false; int side = 0; };

// contact entre ma voiture et une autre (fantôme du duel, ordinateur de la course à 5 : WFS-110) ; effet sur ma voiture
ContactEvents contacter(double dt, Player& pl, const Track& t, const CarDef& me, Ghost& g);

class Session {
public:
    // joinRoom / createRoom : entre dans le salon `code` (host = créateur : il choisit carte et distance)
    bool join(std::unique_ptr<Transport> t, std::string_view code, bool host, std::string_view nick,
              const char* car, const char* paint, std::string_view map);
    void leave();
    bool inRoom() const { return t_ != nullptr; }
    void setKm(int km);                                        // créateur seulement, hors course ; 2, 4, 6 ou 10
    void setReady(bool on);
    // à chaque image hors course : manche suivante quand tous sont prêts → true (la course démarre, voir seed())
    bool checkStart();
    // pendant la course : horloge commune, présence toutes les 0,1 s, fantômes, arrivée (true quand j'arrive)
    bool raceTick(double dt, const Player& pl, double raceTime);
    void updateGhosts(double dt);
    // hitGhosts : contacts avec les adversaires (chaque joueur gère sa propre voiture ; effet symétrique)
    ContactEvents hitGhosts(double dt, Player& pl, const Track& t, const CarDef& me);

    double trafficClock() const;                               // (maintenant − départ)/1000 − 3
    uint32_t seed() const { return raceSeed(code_, round_); }
    double finish() const { return km_ * KM; }                 // FINISH
    int km() const { return km_; }
    int round() const { return round_; }
    Status status() const { return st_; }
    const std::string& code() const { return code_; }
    const std::string& nick() const { return nick_; }
    bool host() const { return host_; }
    const std::map<std::string, Ghost>& ghosts() const { return ghosts_; }
    std::vector<PeerView> others() const;
    std::vector<BoardRow> board(double myDist) const;          // renderBoard : classement en direct
    std::vector<BoardRow> results() const;                     // renderDuelResult : temps d'arrivée puis distance
    int position(double myDist) const;                         // « Position n/N »
    double myFinish() const { return myFinish_; }              // s

private:
    void startRace();
    void followHost();
    std::unique_ptr<Transport> t_;
    std::string code_, nick_ = "Pilote", car_ = "golf", paint_, map_;
    bool host_ = false;
    int km_ = 4, round_ = 0;
    Status st_ = Status::Lobby;
    double startMs_ = 0, presenceTimer_ = 0, myFinish_ = 0;
    std::map<std::string, Ghost> ghosts_;
};

} // namespace duel
} // namespace wfs
