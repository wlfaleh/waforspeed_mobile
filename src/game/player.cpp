#include "player.h"
#include <algorithm>

namespace wfs {

static double clampd(double v, double a, double b) { return v < a ? a : (v > b ? b : v); }

void Player::reset(const CarDef& c) {
    car = &c; perf = carPerf(c);
    position = dist = playerX = speed = steer = heading = 0;
    latAcc = lonAcc = 0; nitro = 0.4; nitroOn = braking = reversing = false;
}

void Player::update(double dt, const DriveInput& in, Track& track, bool playing) {
    const Segment& seg = track.findSegment(position);
    const double speedPct = speed / MAX_SPEED;

    // ---------- braquage progressif : la voiture tourne, son cap la déplace ----------
    const double v = speed * M;   // m/s
    if (playing) {
        double target = clampd(in.steer, -1, 1);
        double rate = (std::abs(target) < 1e-3 || signf((float)target) != signf((float)steer)) ? Phys::steerOut : Phys::steerIn;
        steer += clampd(target - steer, -dt * rate, dt * rate);
    } else steer += -steer * std::min(1.0, dt * 4);
    // angle des roues plus petit à haute vitesse
    const double wheelAngle = steer * Phys::maxSteer / (1 + std::abs(v) / Phys::steerV);
    double yawRate = v / perf.wb * std::tan(wheelAngle) * perf.handling;
    const double yawLim = Phys::grip * perf.handling / std::max(std::abs(v), 1.0);   // limite d'adhérence
    yawRate = clampd(yawRate, -yawLim, yawLim);
    heading += yawRate * dt;
    if (std::abs(steer) < 0.05) heading *= std::exp(-dt * Phys::align);   // réalignement doux sur la route
    const double maxH = clampd(Phys::maxHeading - std::abs(v) * 0.0045, 0.16, Phys::maxHeading);
    heading = clampd(heading, -maxH, maxH);
    latAcc = v * yawRate;
    const double prevSpeed = speed;
    position = track.wrap(position + dt * speed * std::cos(heading));
    dist += dt * speed * std::cos(heading);
    playerX += v * std::sin(heading) * dt / RW;
    // force centrifuge dans les virages
    const double sp = std::min(speedPct, 1.1);
    playerX -= dt * 2 * sp * sp * seg.curve * CENTRIFUGAL;

    // ---------- moteur, freins, frottements (m/s²) ----------
    nitroOn = playing && in.nitro && nitro > 0.01;
    if (nitroOn) nitro = std::max(0.0, nitro - dt * perf.nitroBurn);
    const double top = MAX_SPEED * perf.top * (nitroOn ? perf.nitroMult : 1);
    const bool accelerating = playing && in.gas && !in.brake;
    reversing = playing && in.brake && !in.gas && speed <= 0;
    braking = playing && in.brake && speed > 0;
    const double av = std::abs(v);
    const double resist = (Phys::roll + Phys::drag * av * av + (accelerating ? 0 : Phys::engineBrake)) / M;
    const double off = std::abs(playerX) > 1 ? Phys::offRoad / M : 0;
    if (!playing) speed = std::max(0.0, speed - resist * dt);
    else if (reversing) speed = std::max(REV_MAX, speed - REV_ACC * dt);
    else if (braking) speed = std::max(0.0, speed - (Phys::brake / M + resist) * dt);
    else if (accelerating && speed < 0) speed = std::min(0.0, speed + Phys::brake / M * dt);
    else if (accelerating) {
        const double push = perf.a0 / M * (nitroOn ? 1.6 : 1) * std::max(0.0, 1 - std::pow(std::max(0.0, speed) / top, 2.2));
        speed += (push - resist * 0.35 - off) * dt;
    } else speed = speed < 0 ? std::min(0.0, speed + resist * dt) : std::max(0.0, speed - (resist + off) * dt);
    if (speed > top) speed = std::max(top, speed - MAX_SPEED * 0.4 * dt);
    lonAcc = (speed - prevSpeed) * M / std::max(dt, 1e-3);

    // bordure : on glisse le long, la voiture est redressée
    const double edge = track.map() ? track.map()->edge : 1.6;
    if (std::abs(playerX) > edge) {
        playerX = signf((float)playerX) * edge;
        if (signf((float)heading) == signf((float)playerX)) heading *= -0.25;
        speed *= 1 - dt * 2;
    }
    speed = clampd(speed, REV_MAX, MAX_SPEED * 1.6);
}

} // namespace wfs
