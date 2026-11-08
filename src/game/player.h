// WaForSpeed — conduite du joueur (port de la partie « conduite » de js/game/update.js).
// Unités du jeu web : position/dist en unités (6 mm), speed en unités/s, playerX en demi-largeurs de route.
#pragma once
#include <algorithm>
#include <cmath>
#include "cars.h"
#include "conduite_monde.h"   // WFS-241 : conduite en coordonnées du monde
#include "chocs_decor.h"      // WFS-256 : chocs avec le décor
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

// WFS-217 : aide à la direction (« guide auto »). Web : aides du jeu web, gardées telles quelles pour l'ordinateur et les
// références (simulateCourse, tests) ; Activee / Desactivee : voiture du joueur, selon la ligne « Aide à la direction » des
// Réglages (posée à chaque pas par le système aide_direction.cpp).
enum class Aide : uint8_t { Web = 0, Activee = 1, Desactivee = 2 };
struct ReglagesConduite { bool aideDirection = true; };   // Réglages (WFS-217 RG-02) : Activée par défaut, rien dans configgame.json
inline ReglagesConduite& reglagesConduite() { static ReglagesConduite r; return r; }

// WFS-241 : modèle de conduite de la voiture du joueur (aides Activee / Desactivee). true : conduite réaliste en coordonnées
// du monde (conduite_monde.h) ; false : ancien modèle « par rapport à l'axe de la route » (WFS-217), gardé comme repli interne
// et pour les mesures « avant » du banc de conduite. L'ordinateur (Aide::Web) garde toujours la conduite du jeu web.
inline bool g_conduiteMonde = false;   // WFS-258 (RG-00) : repli sur l'ancien modèle, refus de WFS-241 par l'utilisateur (le nouveau reste disponible, rien n'est supprimé)
// WFS-253 : ABS, antipatinage, ESP de la voiture du joueur : toujours activés dans le jeu ; le banc les coupe un par un pour les mesurer
struct AidesVoiture { bool abs = true, antipatinage = true, esp = true; };
inline AidesVoiture g_aidesVoiture;
// WFS-295 : banc seulement (WFS_BANC=virages) : rejoue le comportement d'avant la correction (avance sans 1/(1 − k·n), blocage de
// 0,6 + 1,2 × impact après un accident, braquage remis à zéro) pour les captures « avant » ; toujours faux dans le jeu
inline bool g_avantWfs295 = false;
// WFS-258 : banc seulement (WFS_BANC=virages) : l'aide activée ne ralentit pas avant les virages (comportement de main) ; faux dans le jeu
inline bool g_avantWfs258 = false;

// WFS-258 : vitesse permise (m/s) à `dist` : accélération latérale `gLat` au plus dans le rayon de chaque virage des 600 m qui
// viennent, freinage de `gFrein` pour l'atteindre avant le virage. Aide à la direction activée de la voiture du joueur : adhérence
// d'un pneu sur route sèche (0,85 g en virage : épingle de 20 m à 46 km/h, courbe de 250 m à 165 km/h ; 0,8 g au freinage) ;
// pilote automatique (vitessePilote, aide_direction_banc.h, WFS-241) : 0,6 g et 0,7 g, plus prudent
inline double vitesseVirageAide(const Track& t, double dist, double gLat = 0.85, double gFrein = 0.8) {
    double v = 1e9;
    for (double m = 0; m <= 600; m += 4) {
        const double k = std::abs(t.findSegment(t.wrap(dist + m / M)).curve) * CURVE_K / SEG_M;
        if (k > 1e-4) v = std::min(v, std::sqrt(gLat * 9.81 / k + 2 * gFrein * 9.81 * m));
    }
    return v;
}

struct Player {
    const CarDef* car = nullptr; CarPerf perf{};
    double position = 0, dist = 0, playerX = 0, speed = 0, steer = 0, heading = 0;
    double odo = 0;   // WFS-90 : distance parcourue dans le sens de la voiture (unités, signée) : roulage des roues, même à contresens
    double latAcc = 0, lonAcc = 0, nitro = 0.4;
    bool nitroOn = false, braking = false, reversing = false;
    // accident (playerAccident du jeu web) : tête-à-queue puis remise dans l'axe, commandes bloquées un moment
    double playerSpin = 0, spinYaw = 0, controlLock = 0;
    Aide aide = Aide::Web;   // WFS-217 : gardée par reset()
    void reset(const CarDef& c);
    // WFS-295 (RG-03) : commandes bloquées 0,5 s au plus après un accident (avant : 0,6 + 1,2 × impact, jusqu'à 1,8 s)
    // (voiture du joueur ; l'ordinateur, Aide::Web, garde les règles du jeu web, référence de test_course)
    void accident(double impact, int side) {
        playerSpin = side * (3 + impact * 9);
        controlLock = aide == Aide::Web || g_avantWfs295 ? 0.6 + impact * 1.2 : 0.2 + 0.3 * std::clamp(impact, 0.0, 1.0);
    }
    // une étape de conduite (dt en secondes) ; analog : braquage continu (-1..1) du tactile
    void update(double dt, const DriveInput& in, Track& track, bool playing);
    // WFS-217 : cap et position latérale de la voiture du joueur (aide Activee / Desactivee), voir player.cpp
    void guider(double dt, double yawRate, double v, bool locked, Track& track);
    double kmh() const { return speed / MAX_SPEED * 260.0; }
    // WFS-241 : conduite en coordonnées du monde (voiture du joueur). La position réelle est dans `monde` ; dist, position,
    // playerX, heading et speed en sont la projection sur la route (tracé lissé, Track::pointLisse), écrite à chaque pas :
    // le rendu, la caméra, le trafic et la course les lisent comme avant. Un système qui les modifie (choc, remise en piste,
    // départ) est pris en compte au pas suivant (la voiture est replacée dans le monde, sa vitesse gardée en direction).
    bool modeleMonde() const { return aide != Aide::Web && g_conduiteMonde; }
    physique::Etat monde{};
    double braquageRoues = 0;   // angle des roues avant appliqué (rad, joueur + aide), modèle du monde
    double aideRoues = 0;       // part de cet angle ajoutée par l'aide à la direction (rad)
    // volant (−1..1) qui donne la courbure k (1/m) à la vitesse v (m/s), selon le modèle en cours (joueur simulé des bancs)
    double braquagePour(double k, double v) const;
    // WFS-256 : chocs avec le décor depuis la dernière lecture (lus et remis à zéro par le système chocs_decor.cpp) :
    // nombre, vitesse d'impact la plus forte (m/s, vers l'obstacle) et genre de l'objet touché
    struct ChocsDuPas { int n = 0; double impact = 0; chocs::Genre genre = chocs::Genre::Autre; } chocsDecor;
    bool freinAide = false;   // WFS-258 : l'aide à la direction freine ou coupe le gaz avant un virage (ce pas)
private:
    // WFS-256 : test continu du pas (pose de départ → pose (x, z, cap) d'arrivée) et réaction de corps rigide ; vrai si contact
    // (pose et vitesse corrigées). Commun aux deux modèles de conduite (conduite réelle et conduite d'avant)
    bool heurter(Track& track, double x0, double z0, double cap0, double& x, double& z, double& cap, double& vx, double& vz, double& r,
                 double m, double Iz);
    void projeterPose(Track& track, double x, double z, double cap);   // position du monde → dist, playerX, heading
    bool enfonceParPivot(double x, double z, double cap0, double cap1) const;   // conduite d'avant : pivot sur place sans s'enfoncer
    std::vector<chocs::Obstacle> obstaclesProches_;   // WFS-256 : obstacles du voisinage (tampon gardé : pas d'allocation par pas)
    double facRoute_ = 1;   // WFS-295 : 1 − k·n (rayon à l'écart n de l'axe), au moins 0,2
    double courbeLisse_ = 0;   // WFS-217 : courbure lissée sous la voiture (force centrifuge du joueur)
    void updateMonde(double dt, const DriveInput& in, Track& track, bool playing);
    void synchroniser(Track& track);
    void projeter(Track& track);
    double aider(double v, Track& track);
    const physique::Params* cm_ = nullptr;
    bool mondeOk_ = false, cibleOk_ = false;
    double sDist_ = 0, sX_ = 0, sHead_ = 0, sSpeed_ = 0;   // dernière projection écrite (détection d'un changement extérieur)
    double latCible_ = 0;      // aide : écart latéral (m) de la voie suivie, celui où le joueur a lâché le volant
};

} // namespace wfs
