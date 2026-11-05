// WaForSpeed — conduite du joueur (port de la partie « conduite » de js/game/update.js).
// Unités du jeu web : position/dist en unités (6 mm), speed en unités/s, playerX en demi-largeurs de route.
#pragma once
#include "cars.h"
#include "track.h"

namespace wfs {

struct DriveInput { float steer = 0; bool gas = false, brake = false, nitro = false; };

// paramètres physiques du jeu web (PHYS)
struct Phys {
    // WFS-128 : valeurs de configgame.json (section conduite) ; défauts : brake 12,5, roll 0,35, drag 0,00042, engineBrake 0,9,
    // grip 18, maxSteer 0,6, steerV 14, steerIn 4, steerOut 6, align 3, maxHeading 0,5, offRoad 7, vLibre 10, vRecul 3
    // WFS-90 : conduite libre sous vLibre m/s (36 km/h) ; marche arrière enchaînée sous vRecul m/s
    static inline const double &brake = g_configJeu.conduite.brake, &roll = g_configJeu.conduite.roll, &drag = g_configJeu.conduite.drag,
        &engineBrake = g_configJeu.conduite.engineBrake, &grip = g_configJeu.conduite.grip, &maxSteer = g_configJeu.conduite.maxSteer,
        &steerV = g_configJeu.conduite.steerV, &steerIn = g_configJeu.conduite.steerIn, &steerOut = g_configJeu.conduite.steerOut,
        &align = g_configJeu.conduite.align, &maxHeading = g_configJeu.conduite.maxHeading, &offRoad = g_configJeu.conduite.offRoad,
        &vLibre = g_configJeu.conduite.vLibre, &vRecul = g_configJeu.conduite.vRecul;
};
constexpr double CENTRIFUGAL = 0.2;
constexpr double REV_MAX = -MAX_SPEED * 0.12, REV_ACC = MAX_SPEED * 0.1;

struct Player {
    const CarDef* car = nullptr; CarPerf perf{};
    double position = 0, dist = 0, playerX = 0, speed = 0, steer = 0, heading = 0;
    double odo = 0;   // WFS-90 : distance parcourue dans le sens de la voiture (unités, signée) : roulage des roues, même à contresens
    double latAcc = 0, lonAcc = 0, nitro = 0.4;
    bool nitroOn = false, braking = false, reversing = false;
    // accident (playerAccident du jeu web) : tête-à-queue puis remise dans l'axe, commandes bloquées un moment
    double playerSpin = 0, spinYaw = 0, controlLock = 0;
    void reset(const CarDef& c);
    void accident(double impact, int side) { playerSpin = side * (3 + impact * 9); controlLock = 0.6 + impact * 1.2; }
    // une étape de conduite (dt en secondes) ; analog : braquage continu (-1..1) du tactile
    void update(double dt, const DriveInput& in, Track& track, bool playing);
    double kmh() const { return speed / MAX_SPEED * 260.0; }
};

} // namespace wfs
