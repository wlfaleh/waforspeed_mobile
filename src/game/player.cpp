#include "player.h"
#include <algorithm>

namespace wfs {

static double clampd(double v, double a, double b) { return v < a ? a : (v > b ? b : v); }
static constexpr double PI_D = 3.14159265358979323846;
// WFS-217 : aide à la direction du joueur (constantes du code, pas de configgame.json : RG-03)
static constexpr double RENVOI_MAX = 2.5;      // rad/s : cap ramené vers la borne ou renvoyé par la bordure (≈ 0,04 rad par image)
static constexpr double LISSAGE_M = 3.0;       // m : courbure lissée sur ±3 m autour de la voiture (pas de marche d'un segment à l'autre)
static constexpr double ANTICIPE_S = 0.23;     // s : la direction visée est celle de la route 0,23 × v plus loin (≤ 8 m)
static constexpr double ANTICIPE_MAX = 8.0;    // m
static constexpr double VOLANT_PLEIN = 0.35;   // braquage au-delà duquel l'aide laisse tout au joueur
static constexpr double ELARGI_M = 8.0;        // m : borne du cap élargie de 8 m × courbure (rayon 14 m : +0,57 rad)
static constexpr double ELARGI_MAX = 0.6;      // rad

// WFS-217 (RG-01, RG-02) : le cap de la voiture est compté par rapport à la route sous elle. Dans le jeu web, la route
// « porte » la voiture : quand elle tourne, la voiture tourne avec elle sans volant, et le joueur qui braque pour suivre le
// virage qu'il voit tourne deux fois (cap qui part vers l'intérieur, borné puis réaligné de force : la lutte des épingles et
// des ronds-points). Ici, la route tourne sous la voiture (cap − courbure × chemin parcouru, comme une vraie voiture) et
// l'aide n'en compense que la part que le joueur ne prend pas au volant (pleine sans volant, nulle au-delà de 0,35) :
//  - réalignement vers la direction réelle de la route, lissée et anticipée de 0,23 × v (pas l'axe du segment) ;
//  - cap borné autour de cette direction, élargi vers l'intérieur selon la courbure locale, ramené à 2,5 rad/s au plus ;
//  - braqué dans le virage au-delà de l'adhérence (vitesse trop haute), l'aide porte la part du virage qui manque.
// Désactivée : ni compensation, ni réalignement, ni borne ; la voiture va où le joueur la dirige (adhérence et force
// centrifuge gardées). Tout est continu : courbure et direction viennent de Track::point (cap interpolé).
void Player::guider(double dt, double yawRate, double v, bool locked, Track& track) {
    const double th0 = track.point(dist, 0).th;
    const double k = (track.point(dist + LISSAGE_M / M, 0).th - track.point(dist - LISSAGE_M / M, 0).th) / (2 * LISSAGE_M);   // 1/m
    courbeLisse_ = k * SEG_M / CURVE_K;   // même unité que Segment::curve (force centrifuge)
    const double avance = speed * M * std::cos(heading);   // m/s le long de la route (négatif à contresens)
    const bool active = aide == Aide::Activee;
    double w = !active ? 0.0 : locked ? 1.0 : 1.0 - clampd(std::abs(steer) / VOLANT_PLEIN, 0, 1);   // part de l'aide
    if (active && !locked && steer * k > 0) {   // braqué dans le virage : l'aide porte ce que l'adhérence ne permet pas
        const double besoin = std::abs(k * avance), lim = Phys::grip * perf.handling / std::max(std::abs(v), 1.0);
        if (besoin > lim) w = std::max(w, 1 - lim / besoin);
    }
    if (!locked) heading += yawRate * dt;
    heading -= k * avance * (1 - w) * dt;   // la route tourne sous la voiture ; l'aide en porte la part w
    if (std::abs(heading) > PI_D) heading = std::atan2(std::sin(heading), std::cos(heading));
    if (!active) return;
    // direction visée : celle de la route un peu plus loin dans le sens de marche (± π à contresens, WFS-90)
    const double axe = std::abs(heading) > PI_D / 2 ? (heading > 0 ? PI_D : -PI_D) : 0.0, sens = axe == 0 ? 1.0 : -1.0;
    const double d = std::min(ANTICIPE_MAX, ANTICIPE_S * std::abs(v));
    const double cible = axe + track.point(dist + sens * d / M, 0).th - th0;
    double dev = heading - cible;
    // borne élargie du côté intérieur du virage seulement (trajectoire serrée permise ; vers l'extérieur, borne du jeu web)
    const double elargi = dev * k * sens > 0 ? std::min(ELARGI_MAX, std::abs(k) * ELARGI_M) : 0.0;
    const double maxH = clampd(Phys::maxHeading - std::abs(v) * 0.0045, 0.16, Phys::maxHeading) + elargi;
    // réalignement doux (jeu web : 3 /s, double après un accident), seulement près de la direction (demi-tour libre, WFS-90)
    if (std::abs(dev) <= Phys::maxHeading + elargi) dev *= std::exp(-dt * Phys::align * w * (locked ? 2 : 1));
    if (std::abs(v) >= Phys::vLibre && std::abs(dev) > maxH) {   // cap borné : ramené vers la borne, jamais d'un coup
        const double exces = std::abs(dev) - maxH;
        dev = (dev > 0 ? 1 : -1) * (maxH + std::max(0.0, exces - RENVOI_MAX * dt));
    }
    heading = cible + dev;
}

void Player::reset(const CarDef& c) {
    car = &c; perf = carPerf(c);
    position = dist = playerX = speed = steer = heading = odo = 0;
    latAcc = lonAcc = 0; nitro = config().nitro.depart; nitroOn = braking = reversing = false;
    playerSpin = spinYaw = controlLock = 0;
}

void Player::update(double dt, const DriveInput& in, Track& track, bool playing) {
    const Segment& seg = track.findSegment(position);
    const double speedPct = speed / MAX_SPEED;

    // tête-à-queue après un accident : la voiture tourne puis se remet droite (update() du jeu web)
    controlLock = std::max(0.0, controlLock - dt);
    if (std::abs(playerSpin) > 0.25) {
        spinYaw += playerSpin * dt; playerSpin *= std::max(0.0, 1 - 2.4 * dt);
        playerX += playerSpin * 0.012 * dt * std::min(1.0, speedPct * 3);
    } else { playerSpin = 0; spinYaw += (0 - spinYaw) * std::min(1.0, dt * 3); if (std::abs(spinYaw) < 0.002) spinYaw = 0; }
    spinYaw = std::atan2(std::sin(spinYaw), std::cos(spinYaw));   // ramené entre -π et π
    const bool locked = controlLock > 0;

    // ---------- braquage progressif : la voiture tourne, son cap la déplace ----------
    const double v = speed * M;   // m/s
    if (playing && !locked) {
        double target = clampd(in.steer, -1, 1);
        double rate = (std::abs(target) < 1e-3 || signf((float)target) != signf((float)steer)) ? Phys::steerOut : Phys::steerIn;
        steer += clampd(target - steer, -dt * rate, dt * rate);
    } else steer += -steer * std::min(1.0, dt * 4);
    // angle des roues plus petit à haute vitesse
    const double wheelAngle = steer * Phys::maxSteer / (1 + std::abs(v) / Phys::steerV);
    double yawRate = v / perf.wb * std::tan(wheelAngle) * perf.handling;
    const double yawLim = Phys::grip * perf.handling / std::max(std::abs(v), 1.0);   // limite d'adhérence
    yawRate = clampd(yawRate, -yawLim, yawLim);
    double courbe = seg.curve;   // courbure de la force centrifuge (unités de Segment::curve)
    if (aide != Aide::Web) {   // WFS-217 : voiture du joueur (aide à la direction activée ou désactivée)
        guider(dt, yawRate, v, locked, track);
        courbe = courbeLisse_;
    } else {
    if (!locked) heading += yawRate * dt;
    // WFS-90 : conduite libre. Le cap est compté par rapport au sens de la route le plus proche (axe 0, ou ±π à
    // contresens). Dans le sens normal et près de l'axe, tout est identique au jeu web (réalignement, cap borné).
    if (std::abs(heading) > PI_D) heading = std::atan2(std::sin(heading), std::cos(heading));
    const double axe = std::abs(heading) > PI_D / 2 ? (heading > 0 ? PI_D : -PI_D) : 0.0;
    double dev = heading - axe;
    // réalignement doux sur la route, seulement près d'un axe : plus de retour automatique quand l'angle est grand (RG-02)
    if ((std::abs(steer) < 0.05 || locked) && std::abs(dev) <= Phys::maxHeading) dev *= std::exp(-dt * Phys::align * (locked ? 2 : 1));
    const double maxH = clampd(Phys::maxHeading - std::abs(v) * 0.0045, 0.16, Phys::maxHeading);
    // cap borné autour de l'axe (jeu web) ; à basse vitesse (< V_LIBRE) la voiture tourne librement jusqu'à 180° (RG-01) ;
    // au-delà, un cap trop grand revient vers la borne à 2,5 rad/s au plus (pas de saut) — dépassement d'un pas : borne exacte
    if (std::abs(v) >= Phys::vLibre && std::abs(dev) > maxH) {
        const double exces = std::abs(dev) - maxH;
        dev = (dev > 0 ? 1 : -1) * (maxH + std::max(0.0, exces - 2.5 * dt));
    }
    heading = axe + dev;
    }
    latAcc = v * yawRate;
    const double prevSpeed = speed;
    position = track.wrap(position + dt * speed * std::cos(heading));
    dist += dt * speed * std::cos(heading);
    odo += dt * speed;   // WFS-90
    playerX += v * std::sin(heading) * dt / RW;
    // force centrifuge dans les virages
    const double sp = std::min(speedPct, 1.1);
    playerX -= dt * 2 * sp * sp * courbe * CENTRIFUGAL;

    // ---------- moteur, freins, frottements (m/s²) ----------
    nitroOn = playing && in.nitro && nitro > 0.01;
    if (nitroOn) nitro = std::max(0.0, nitro - dt * perf.nitroBurn);
    const double top = MAX_SPEED * perf.top * (nitroOn ? perf.nitroMult : 1);
    const bool accelerating = playing && !locked && in.gas && !in.brake;
    // WFS-90 (RG-03) : marche arrière sans attendre l'arrêt complet (sous 3 m/s, le frein enchaîne sur la marche arrière)
    reversing = playing && !locked && in.brake && !in.gas && speed * M < Phys::vRecul;
    braking = playing && !locked && in.brake && speed > 0 && !reversing;
    const double av = std::abs(v);
    const double resist = (Phys::roll + Phys::drag * av * av + (accelerating ? 0 : Phys::engineBrake)) / M;
    // WFS-188 (RG-06) : résistance du bas-côté proportionnelle à la vitesse (pleine dès 15 m/s) : à l'arrêt dans l'herbe,
    // une voiture dont la poussée recalée est sous Phys::offRoad (Golf, Compass…) peut toujours repartir
    const double off = std::abs(playerX) > 1 ? Phys::offRoad / M * (perf.referenceWeb ? 1.0 : std::min(1.0, av / 15.0)) : 0;
    if (!playing) speed = std::max(0.0, speed - resist * dt);
    else if (locked) speed = speed > 0 ? std::max(0.0, speed - Phys::brake * 0.5 / M * dt) : std::min(0.0, speed + Phys::brake * 0.5 / M * dt);
    else if (reversing) speed = std::max(REV_MAX, speed - REV_ACC * dt);
    else if (braking) speed = std::max(0.0, speed - (Phys::brake * perf.freinage / M + resist) * dt);   // WFS-188 (RG-04) : selon la voiture
    else if (accelerating && speed < 0) speed = std::min(0.0, speed + Phys::brake / M * dt);
    else if (accelerating) {
        const double push = perf.a0 / M * (nitroOn ? config().nitro.poussee : 1) * std::max(0.0, 1 - std::pow(std::max(0.0, speed) / top, 2.2));
        speed += (push - resist * 0.35 - off) * dt;
    } else speed = speed < 0 ? std::min(0.0, speed + resist * dt) : std::max(0.0, speed - (resist + off) * dt);
    if (speed > top) speed = std::max(top, speed - MAX_SPEED * 0.4 * dt);
    lonAcc = (speed - prevSpeed) * M / std::max(dt, 1e-3);

    // bordure : on glisse le long, la voiture est redressée
    const double edge = track.map() ? track.map()->edge : 1.6;
    if (std::abs(playerX) > edge) {
        playerX = signf((float)playerX) * edge;
        // vers la bordure : le cap est renvoyé (× −0,25) autour de son axe (WFS-90 : aussi à contresens)
        if (signf((float)std::sin(heading)) == signf((float)playerX)) {
            const double ax = std::abs(heading) > PI_D / 2 ? (heading > 0 ? PI_D : -PI_D) : 0.0, renvoi = ax + (heading - ax) * -0.25;
            // WFS-217 (RG-01) : voiture du joueur renvoyée progressivement (2,5 rad/s au plus), sans saut de cap d'une image à l'autre
            heading = aide == Aide::Web ? renvoi : heading + clampd(renvoi - heading, -RENVOI_MAX * dt, RENVOI_MAX * dt);
        }
        speed *= 1 - dt * 2;
    }
    speed = clampd(speed, REV_MAX, MAX_SPEED * 1.6);
}

} // namespace wfs
