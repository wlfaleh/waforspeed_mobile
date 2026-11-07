// WaForSpeed — trafic simulé (port de js/game/traffic.js) et chocs avec le joueur (port de la partie
// « collisions » de js/game/update.js : hitCar, quasi-contacts).
// Les voitures se suivent sans se chevaucher, freinent derrière plus lent qu'elles, ne changent de file que si
// la place est libre (clignotant ~1 s avant), tiennent compte du joueur et contournent les voitures accidentées.
// La simulation avance par pas fixes de 1/60 s à partir d'une graine : même graine → même trafic.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include "player.h"
#include "track.h"

namespace wfs {

constexpr double T_STEP = 1.0 / 60.0;
constexpr int LANES = 3;
constexpr double LANE_X[LANES] = {-2.0 / 3.0, 0.0, 2.0 / 3.0};
constexpr double PLAYER_Z = 0;
constexpr double LANE_HALF = 0.36;          // écart latéral (demi-largeurs de route) à partir duquel deux voitures se gênent
constexpr double SAFE_TIME = 1.1;           // temps de sécurité derrière la voiture de devant (s)
constexpr double SAFE_MIN = 5.0 / M;        // distance minimale à l'arrêt (unités)
constexpr double NEAR_MISS = 0.24;          // marge latérale d'un quasi-contact (demi-largeurs de route)

// catalogue du jeu web (js/cars/catalog.js, même ordre) : seules la longueur et la largeur servent au trafic.
// Le tirage du type utilise ces 20 voitures, comme le jeu web ; l'affichage prend le modèle natif le plus proche.
struct WebCarDims { const char* id; double L, W; };   // double : mêmes calculs que le JavaScript
inline const WebCarDims WEB_CARS[] = {
    {"golf", 4.29, 1.79}, {"p208", 4.06, 1.75}, {"abarth", 3.66, 1.63}, {"tcross", 4.11, 1.76}, {"compass", 4.40, 1.82},
    {"s3", 4.47, 1.80}, {"x5", 4.92, 2.00}, {"giulia", 4.64, 1.87}, {"m4", 4.79, 1.89}, {"p911", 4.52, 1.85},
    {"a45", 4.36, 1.78}, {"model3", 4.69, 1.85}, {"corsa", 4.06, 1.77}, {"gt86", 4.24, 1.78}, {"elantra", 4.68, 1.83},
    {"v60", 4.63, 1.87}, {"rrsport", 4.95, 2.05}, {"mustang", 4.82, 1.92}, {"gallardo", 4.35, 1.90}, {"sf90", 4.85, 1.97},
};
constexpr int WEB_CAR_COUNT = sizeof(WEB_CARS) / sizeof(WEB_CARS[0]);
// longueur exacte (double) d'une voiture du jeu web, en unités ; repli : valeur du catalogue natif
inline double webCarLength(const char* id, double fallbackM) {
    for (const auto& c : WEB_CARS) if (std::strcmp(c.id, id) == 0) return c.L / M;
    return fallbackM / M;
}
// couleurs du trafic (TRAFFIC_PAINTS du jeu web)
inline const unsigned TRAFFIC_PAINTS[] = {0xc9ccd2, 0x0f1014, 0x7b808a, 0xa7acb5, 0x1d3f8f, 0x7a1426, 0x2f4b3e, 0x3b3f48, 0xc9b28a};
constexpr int TRAFFIC_PAINT_COUNT = sizeof(TRAFFIC_PAINTS) / sizeof(TRAFFIC_PAINTS[0]);

struct TrafficCar {
    int type = 0, color = 0;
    double len = 0, w = 0;                  // longueur (unités), demi-largeur (demi-largeurs de route)
    int lane = 0, target = 0;
    double offset = 0, prevOff = 0, z = 0, speed = 0, want = 0;
    uint32_t rng = 0;
    double think = 0;
    // accident : glisse, tourne sur elle-même, s'arrête puis repart
    bool crashed = false; double crashT = 0, crashVx = 0, crashYaw = 0, crashSpin = 0;
    int signal = 0;                         // clignotant : -1 gauche, 1 droite
    bool pending = false; int pendLane = 0; double pendT = 0;
    double prevDz = 0;
    double laneYaw = 0;                     // cap dû au changement de file (rad), pour l'affichage
    double zPrec = 0, offPrec = 0, yawPrec = 0;   // WFS-221 : état avant le dernier pas (rendu interpolé, GameView::alpha)
    // WFS-136 (RG-03) : voiture qui traverse le carrefour à feux n° traverse (−1 : voiture normale), au rouge, de gauche
    // à droite ; cachée (ni dessinée, ni touchée) le reste du temps
    int traverse = -1, rangTraverse = 0;
    bool cache = false;
};

// ---------- trafic vivant (WFS-67) : état d'affichage, calculé à partir de la simulation sans la modifier ----------
// clignotants et feux de détresse (scene.js) : chaque voiture a son rythme, allumé 0,38 s sur 0,72 s
inline bool clignotantAllume(double t, uint32_t rng) { return std::fmod(t + (rng % 997) / 997.0, 0.72) < 0.38; }
// feux stop : décélération de plus de 2 m/s², gardés 0,4 s (pas de clignotement des feux à chaque pas)
struct FeuxStop {
    double prev = -1, tenue = 0;
    bool freine() const { return tenue > 0; }
    void maj(double speedUnits, double dt) {
        if (prev >= 0 && dt > 0 && (prev - speedUnits) * M / dt > 2.0) tenue = 0.4; else tenue = std::max(0.0, tenue - dt);
        prev = speedUnits;
    }
};
// hauteurs des feux des 20 voitures du jeu web (js/cars/catalog.js : d.H, d.nose, d.tail), même ordre que WEB_CARS
struct WebCarFeux { const char* id; float H, nose, tail; };
inline const WebCarFeux WEB_FEUX[] = {
    {"golf", 1.46f, 0.72f, 1.0f}, {"p208", 1.43f, 0.7f, 0.98f}, {"abarth", 1.49f, 0.72f, 1.0f}, {"tcross", 1.58f, 0.82f, 1.04f},
    {"compass", 1.64f, 0.85f, 1.08f}, {"s3", 1.4f, 0.72f, 0.98f}, {"x5", 1.75f, 0.86f, 1.1f}, {"giulia", 1.43f, 0.7f, 0.97f},
    {"m4", 1.39f, 0.68f, 0.96f}, {"p911", 1.3f, 0.66f, 0.95f}, {"a45", 1.42f, 0.72f, 1.0f}, {"model3", 1.44f, 0.72f, 0.98f},
    {"corsa", 1.43f, 0.7f, 0.98f}, {"gt86", 1.29f, 0.66f, 0.95f}, {"elantra", 1.42f, 0.7f, 0.97f}, {"v60", 1.48f, 0.72f, 1.02f},
    {"rrsport", 1.82f, 0.86f, 1.12f}, {"mustang", 1.38f, 0.68f, 0.96f}, {"gallardo", 1.18f, 0.62f, 0.92f}, {"sf90", 1.19f, 0.62f, 0.92f},
};
constexpr double LOD_CAR_M = 60;   // au-delà : silhouette légère (scene.js : 50 m) ; WFS-134 (RG-04) : modèle complet jusqu'à 60 m

// ce que le trafic doit savoir du joueur
struct TrafficView { double position = 0, playerX = 0, speed = 0, myLen = 0; bool garage = false; };

// événements d'un pas de collision (pour le score, le son et les secousses : WFS-10 / WFS-12)
// seq : les événements qui comptent pour les règles de course (WFS-12), dans l'ordre de la boucle du jeu web
struct HitEvent { char kind; double impact; };   // 'n' quasi-contact, 'b' petit choc (bump), 'a' accident
struct HitEvents {
    int nearMiss = 0; int scrapes = 0; int bumps = 0; int accidents = 0; float shake = 0;
    HitEvent seq[16]; int count = 0;
    void push(char kind, double impact) { if (count < 16) seq[count++] = {kind, impact}; }
};

class Traffic {
public:
    // resetRace() du jeu web : nouvelle graine, n voitures devant le joueur
    void reset(Track& track, uint32_t seed, int n, double playerPos, bool menu = false);
    void addCars(int n, double playerPos);
    // avance la simulation jusqu'à l'horloge (advanceTraffic) ; parallel : recherche de la voiture de devant
    // répartie sur les threads de jobs() (sinon : ordre exact du jeu web, pour les tests de fidélité)
    void advance(double dt, const TrafficView& pv);
    void step(double h, const TrafficView& pv);
    bool laneFree(const TrafficCar& car, int lane, const TrafficView& pv) const;
    static void crash(TrafficCar& car, double impact, int side);   // crashTraffic
    // chocs et quasi-contacts avec le joueur (boucle « for (const car of cars) » de update())
    HitEvents collide(Player& pl, double dt);
    // pendant le compte à rebours (pas de chocs) : suit seulement la position relative des voitures (car.prevDz)
    void watch(const Player& pl);

    std::vector<TrafficCar> cars;
    uint32_t seed = 1;
    double clock = 0, sim = 0;
    bool parallel = true;
private:
    void findLeaders(const TrafficView& pv, int begin, int end);
    void ajouterTrace2(int n, double playerPos);   // WFS-136 : voitures en plus sur l'autoroute, voitures qui traversent
    void placerTraverse(TrafficCar& car) const;     // WFS-136 : position d'une voiture qui traverse (horloge des feux)
    Track* track_ = nullptr;
    std::vector<double> gap_, lead_;        // voiture de devant de chaque voiture (calculées avant le pas)
    double scrapeCd_ = 0;
};

} // namespace wfs
