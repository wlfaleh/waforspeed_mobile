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
    cm_ = nullptr; mondeOk_ = cibleOk_ = false; monde = {}; braquageRoues = aideRoues = latCible_ = 0;   // WFS-241
    position = dist = playerX = speed = steer = heading = odo = 0;
    latAcc = lonAcc = 0; nitro = config().nitro.depart; nitroOn = braking = reversing = false;
    playerSpin = spinYaw = controlLock = 0;
}

void Player::update(double dt, const DriveInput& in, Track& track, bool playing) {
    if (modeleMonde()) { updateMonde(dt, in, track, playing); return; }   // WFS-241 : voiture du joueur, conduite réaliste
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

// ================================================================ WFS-241 : conduite en coordonnées du monde
// Cause de fond d'ANO-113 (banc « conduite », mesures « avant ») : dans l'ancien modèle, la voiture n'a pas de position dans
// le monde : son cap est compté par rapport à la route (player.cpp, guider() : « heading −= k × avance », la route tourne
// sous la voiture) et des aides cachées la ramènent vers l'axe (réalignement, cap borné, renvoi de la bordure, force
// centrifuge forfaitaire). Activée ou désactivée, la route « porte » donc toujours en partie la voiture, et les deux réglages
// se ressemblent. Ici, la voiture roule dans le monde (conduite_monde.h) : sans volant ni aide, elle va tout droit ; l'aide à
// la direction est un vrai angle de roues ajouté, calculé pour suivre la voie, qui cède dès que le joueur braque.
namespace {
constexpr double AIDE_VOLANT_PLEIN = 0.35;   // volant au-delà duquel l'aide laisse tout au joueur (comme WFS-217)
constexpr double AIDE_VOLANT_LIBRE = 0.05;   // volant sous lequel le joueur est « lâché » (l'aide garde la voie où il l'a laissée)
constexpr double AIDE_VMIN = 2.0;            // m/s : manœuvres (demi-tour, marche arrière) sans aide
constexpr double BORDURE_RENVOI = 2.5;       // rad/s : cap redressé le long de la glissière au plus (pas de saut)
}

double Player::braquagePour(double k, double v) const {
    if (!modeleMonde()) {   // ancien modèle : angle des roues de Player::update
        const double roue = std::atan(k * perf.wb / perf.handling), roueMax = Phys::maxSteer / (1 + v / Phys::steerV);
        return std::clamp(roue / roueMax, -1.0, 1.0);
    }
    const physique::Params& P = cm_ ? *cm_ : physique::params(*car);
    return std::clamp(std::atan(k * P.L) / physique::braquageMax(P, v), -1.0, 1.0);
}

// position et cap réels → projection sur la route (tracé lissé, celui du rendu : la voiture est dessinée exactement où elle est)
void Player::projeter(Track& t) {
    double s = dist;
    TrackPoint C{};
    for (int i = 0; i < 6; i++) {   // point de la ligne de la route le plus proche (Newton, converge en 2 ou 3 tours)
        C = t.pointLisse(s, 0);
        const double e = (monde.x - C.x) * std::sin(C.th) - (monde.z - C.z) * std::cos(C.th);
        s += e / M;
        if (std::abs(e) < 1e-6) break;
    }
    C = t.pointLisse(s, 0);
    dist = s; position = t.wrap(s);
    playerX = ((monde.x - C.x) * std::cos(C.th) + (monde.z - C.z) * std::sin(C.th)) / RW;
    heading = std::remainder(monde.cap - C.th, 2 * PI_D);
}

// un système a déplacé la voiture par la route (choc du trafic, remise en piste, départ, reset) : la voiture est replacée
// dans le monde à cet endroit, avec ce cap ; sa vitesse garde sa direction dans la voiture (dérive comprise)
void Player::synchroniser(Track& t) {
    if (mondeOk_ && dist == sDist_ && playerX == sX_ && heading == sHead_ && speed == sSpeed_) return;
    double u = speed * M, v = 0, r = 0;
    if (mondeOk_) {
        const double u0 = monde.u(), k = std::abs(u0) > 0.1 ? std::clamp(u / u0, 0.0, 1.5) : 0.0;
        v = monde.v() * k; r = monde.r * k;
    }
    const TrackPoint C = t.pointLisse(dist, playerX * RW);
    monde.x = C.x; monde.z = C.z; monde.cap = C.th + heading; monde.r = r;
    const double sn = std::sin(monde.cap), cs = std::cos(monde.cap);
    monde.vx = u * sn + v * cs; monde.vz = -u * cs + v * sn;
    if (!mondeOk_) monde.rapport = 1;
    mondeOk_ = true;
}

// WFS-256 : test continu tous les 10 cm du déplacement du pas (rien ne se traverse, même à 300 km/h), puis réaction
// d'un corps rigide (chocs_decor.h) au premier contact ; le reste du déplacement du pas est abandonné
void Player::heurter(double x0, double z0, double cap0) {
    const chocs::Registre& reg = chocs::registre();
    if (reg.taille() == 0 && reg.mobilesVide()) return;
    const double hl = car->d.L / 2, hw = car->d.W / 2, portee = std::hypot(hl, hw) + 0.2;
    const double x1 = monde.x, z1 = monde.z, cap1 = monde.cap;
    const int n = std::max(1, (int)std::ceil(std::hypot(x1 - x0, z1 - z0) / 0.10));
    for (int k = 1; k <= n; k++) {
        const double t = (double)k / n, cx = x0 + (x1 - x0) * t, cz = z0 + (z1 - z0) * t, cap = cap0 + (cap1 - cap0) * t;
        bool touche = false;
        double px = cx, pz = cz;
        reg.autour(cx, cz, portee, [&](const chocs::Obstacle& o) {
            const chocs::Contact c = chocs::contact(px, pz, cap, hl, hw, o);
            if (!c.touche) return;
            if (!touche) { monde.x = px; monde.z = pz; monde.cap = cap; touche = true; }   // la voiture s'arrête au premier contact du pas
            const chocs::Reponse r = chocs::reagir(monde.x, monde.z, monde.vx, monde.vz, monde.r, cm_->masse, cm_->Iz, c, o);
            px = monde.x; pz = monde.z;
            if (r.choc) { chocsDecor.n++; if (r.impact > chocsDecor.impact) { chocsDecor.impact = r.impact; chocsDecor.genre = r.genre; } }
        });
        if (touche) return;
    }
}

// aide à la direction (activée) : angle de roues ajouté pour suivre la voie où le joueur a laissé la voiture (poursuite
// pure : point de cette voie visé 0,6 s devant, comme un conducteur qui regarde où va la route). Part de l'aide : pleine
// volant lâché, nulle au-delà de 0,35 (elle ne lutte jamais contre le joueur). Pas d'aide à contresens ni en manœuvre.
double Player::aider(double v, Track& t) {
    const double a = std::abs(steer), edgeM = (t.map() ? t.map()->edge : 1.6) * RW;
    if (a > AIDE_VOLANT_LIBRE || !cibleOk_) { latCible_ = playerX * RW; cibleOk_ = true; }
    latCible_ = std::clamp(latCible_, -std::min(RW - 1.2, edgeM), std::min(RW - 1.2, edgeM));   // la voie suivie reste sur la chaussée
    if (std::abs(heading) > PI_D / 2 || v < AIDE_VMIN || monde.u() < 0) return 0;
    const double w = 1 - std::clamp(a / AIDE_VOLANT_PLEIN, 0.0, 1.0);
    if (w <= 0) return 0;
    const physique::Params& P = *cm_;
    const double ld = std::clamp(0.6 * v, 6.0, 30.0);
    const TrackPoint T = t.pointLisse(dist + ld / M, latCible_);
    const double dx = T.x - monde.x, dz = T.z - monde.z, sn = std::sin(monde.cap), cs = std::cos(monde.cap);
    const double av = dx * sn - dz * cs, dr = dx * cs + dz * sn;
    const double kappa = 2 * dr / std::max(1.0, av * av + dr * dr);   // courbure du cercle qui passe par le point visé (1/m)
    const double roue = std::clamp(std::atan(kappa * P.L), -physique::braquageMax(P, v), physique::braquageMax(P, v));
    return w * roue;
}

void Player::updateMonde(double dt, const DriveInput& in, Track& track, bool playing) {
    if (!cm_) cm_ = &physique::params(*car);
    const physique::Params& P = *cm_;
    // tête-à-queue après un accident : comme l'ancien modèle (rotation montrée, dérive), commandes bloquées un moment
    controlLock = std::max(0.0, controlLock - dt);
    if (std::abs(playerSpin) > 0.25) {
        spinYaw += playerSpin * dt; playerSpin *= std::max(0.0, 1 - 2.4 * dt);
        playerX += playerSpin * 0.012 * dt * std::min(1.0, speed / MAX_SPEED * 3);
    } else { playerSpin = 0; spinYaw += (0 - spinYaw) * std::min(1.0, dt * 3); if (std::abs(spinYaw) < 0.002) spinYaw = 0; }
    spinYaw = std::atan2(std::sin(spinYaw), std::cos(spinYaw));
    const bool locked = controlLock > 0;
    synchroniser(track);

    // volant progressif (mêmes vitesses que l'ancien modèle : configgame.json, steerIn / steerOut)
    if (playing && !locked) {
        const double target = std::clamp((double)in.steer, -1.0, 1.0);
        const double rate = (std::abs(target) < 1e-3 || signf((float)target) != signf((float)steer)) ? Phys::steerOut : Phys::steerIn;
        steer += std::clamp(target - steer, -dt * rate, dt * rate);
    } else steer += -steer * std::min(1.0, dt * 4);
    const double u = monde.u(), v = std::abs(u);
    const double roueMax = physique::braquageMax(P, v);
    aideRoues = aide == Aide::Activee && playing && !locked ? aider(v, track) : 0;
    braquageRoues = locked ? 0 : std::clamp(steer * roueMax + aideRoues, -roueMax, roueMax);

    // commandes : mêmes règles que l'ancien modèle (frein puis marche arrière enchaînée sous vRecul, nitro) ; rien d'automatique
    nitroOn = playing && in.nitro && nitro > 0.01;
    if (nitroOn) nitro = std::max(0.0, nitro - dt * perf.nitroBurn);
    const bool accelerating = playing && !locked && in.gas && !in.brake;
    reversing = playing && !locked && in.brake && !in.gas && u < Phys::vRecul;
    braking = playing && !locked && in.brake && u > 0 && !reversing;
    physique::Commande c;
    c.braquage = braquageRoues;
    if (!playing) {}
    else if (locked) c.frein = 0.5;
    else if (reversing) { c.recul = true; c.gaz = 1; }
    else if (braking || (accelerating && u < -0.05)) c.frein = 1;
    else if (accelerating) c.gaz = 1;
    c.nitro = nitroOn ? config().nitro.poussee : 1;
    c.abs = g_aidesVoiture.abs; c.antipatinage = g_aidesVoiture.antipatinage; c.esp = g_aidesVoiture.esp;   // WFS-253
    physique::Sol sol;
    if (std::abs(playerX) > 1) { sol.mu = physique::MU_HERBE; sol.crr = physique::CRR_HERBE; }   // bas-côté
    sol.pente = track.pointLisse(dist, playerX * RW).slope * std::cos(heading);
    const double x0 = monde.x, z0 = monde.z, cap0 = monde.cap;
    physique::pas(P, monde, c, sol, dt);
    heurter(x0, z0, cap0);   // WFS-256 : chocs avec le décor, sans rien traverser entre deux pas
    projeter(track);

    // glissière (bord de la carte) : la voiture glisse le long, sa vitesse vers l'extérieur est annulée, son cap redressé
    const double edge = track.map() ? track.map()->edge : 1.6;
    if (std::abs(playerX) > edge) {
        const double cote = playerX > 0 ? 1.0 : -1.0;
        playerX = cote * edge;
        const TrackPoint C = track.pointLisse(dist, playerX * RW);
        monde.x = C.x; monde.z = C.z;
        const double nx = std::cos(C.th) * cote, nz = std::sin(C.th) * cote, vn = monde.vx * nx + monde.vz * nz;
        if (vn > 0) { monde.vx -= nx * vn * 1.1; monde.vz -= nz * vn * 1.1; }
        const double frot = std::max(0.0, 1 - dt * 1.5);   // frottement contre la glissière
        monde.vx *= frot; monde.vz *= frot;
        if (std::sin(heading) * cote > 0) {
            const double ax = std::abs(heading) > PI_D / 2 ? (heading > 0 ? PI_D : -PI_D) : 0.0, dev = heading - ax;
            heading = ax + (dev > 0 ? 1 : -1) * std::max(0.0, std::abs(dev) - BORDURE_RENVOI * dt);
            monde.cap = C.th + heading; monde.r *= 0.5;
        }
    }
    const double prevSpeed = speed;
    speed = std::clamp(monde.u() / M, REV_MAX, MAX_SPEED * 1.6);
    odo += dt * speed;
    latAcc = monde.ay; lonAcc = (speed - prevSpeed) * M / std::max(dt, 1e-3);
    sDist_ = dist; sX_ = playerX; sHead_ = heading; sSpeed_ = speed;
}

} // namespace wfs
