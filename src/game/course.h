// WaForSpeed — règles de la course solo (ticket WFS-12, épic E3) : compte à rebours, chrono, checkpoints, score,
// combos de quasi-contacts, accidents, fin de partie. Port de js/game/update.js (fin de update(), playerAccident,
// bump, crash) et de js/game/state.js (resetRace, score, kmh) du jeu web.
// La conduite (nitro comprise) reste dans Player, les chocs avec le trafic dans Traffic : ici, uniquement les règles.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "player.h"
#include "traffic.h"

namespace wfs {

enum class RaceState { Count, Play, Over };   // state du jeu web : 'count', 'play', 'over'

struct Course {
    RaceState state = RaceState::Count;
    double clock = -3;                      // trafficClock : < 0 pendant le compte à rebours
    double timeLeft = 30, nextCp = CHECK_DIST, raceTime = 0, comboTimer = 0, overTimer = 0;
    int level = 0, combo = 0, nearCount = 0, maxKmh = 0, accidents = 0;
    int bumps = 0, scrapes = 0; double lastImpact = 0;   // WFS-28 : compteurs lus par les bruitages (src/audio/sfx.cpp)
    long long bonus = 0;
    int lastCount = -1;                     // dernier chiffre affiché (3, 2, 1, puis 0 = « GO »)
    double goShown = 0;                     // « GO » reste affiché 0,7 s après le départ (setTimeout du jeu web)
    double timeScale = 1, slowmoT = 0;      // ralenti après un accident (main.js du jeu web)
    double duelFinish = 0;                  // WFS-21 : > 0 = duel (mode 'duel') : arrivée à FINISH, pas de chrono ni checkpoint

    // resetRace('play') + startRace() : 30 s, compte à rebours de 3 s (la nitro est remise à 0,4 par Player::reset)
    void reset();
    bool playing() const { return state == RaceState::Play; }

    // événements du jeu (mêmes effets que le jeu web)
    void nearMiss();                        // quasi-contact : combo, 250×combo points (la nitro est donnée par Traffic)
    void bump(double impact);               // petit choc : combo perdu au-delà de 0,08
    void accident();                        // playerAccident : combo perdu, ralenti ×0,35 pendant 0,5 s
    void crash(Player& pl, Track& t);       // décor percuté : recul de 300, vitesse MAX_SPEED/8, tête-à-queue
    void applyHits(const HitEvents& e);     // événements d'un pas de collision, dans l'ordre où ils sont arrivés

    // fin de update() du jeu web, après la conduite et les chocs : combo, compte à rebours, chrono, checkpoints, fin.
    // traffic (facultatif) : +6 voitures à chaque checkpoint tant qu'il y en a moins de 110
    void step(double dt, const Player& pl, Traffic* traffic, double playerPos);
    // pendant le compte à rebours la voiture reste immobile (speed = 0 du jeu web)
    void hold(Player& pl) const;   // et la nitro ne bouge pas (nitroOn exige playing)
    // durée réelle → durée de jeu (ralenti après un accident, boucle de main.js)
    double scaleDt(double raw);

    long long score(const Player& pl) const { return std::max(0LL, (long long)std::floor(pl.dist * 0.006)) * 5 + bonus; }
    static int kmh(double speed) { return (int)std::floor(speed / MAX_SPEED * 260.0 + 0.5); }   // Math.round du jeu web
    // secondes ajoutées au checkpoint n (1 = premier) : add = max(5, 11 − level×0,4) avec level = n − 1
    static double checkpointBonus(int n) { return std::max(5.0, 11.0 - (n - 1) * 0.4); }
    // texte du compte à rebours à afficher (« 3 », « 2 », « 1 », « GO », ou vide)
    std::string countText() const;
};

// course automatique de référence (même scénario que tests/course_web.cjs) : pas de 1/120 s, plein gaz,
// braquage tout-ou-rien vers le centre, nitro au-delà de 0,5. Sert au test unitaire et au journal du banc d'essai.
std::vector<std::string> simulateCourse(const char* mapId, const char* carId, uint32_t seed);

} // namespace wfs
