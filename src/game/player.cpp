#include "player.h"
#include <algorithm>
#include "../world/cartes_v2/carte_donnees.h"   // WFS-346 (accroche) : μ des surfaces de la carte

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
static constexpr double VOLANT_TENU = 0.05;    // WFS-316 (RG-02) : au-delà, le joueur tient le volant
// WFS-316 (RG-05) : la correction ne vaut qu'aux vitesses des virages serrés : pleine jusqu'à 50 km/h, nulle à 70 km/h et au-delà
// (à 150-230 km/h, sans borne, l'aide activée donnait un lacet 1,6 à 3,5 fois plus vif : test_direction_anneau)
static constexpr double TENU_PLEIN = 50 / 3.6, TENU_NUL = 70 / 3.6;   // m/s
// part de la correction WFS-316 : 1 volant tenu à basse vitesse, 0 volant lâché, au-dessus de 70 km/h, ou au banc « avant »
static double partTenu(double steer, double v) {
    if (std::abs(steer) < VOLANT_TENU || g_avantWfs316) return 0;
    return clampd((TENU_NUL - std::abs(v)) / (TENU_NUL - TENU_PLEIN), 0, 1);
}
static constexpr double FILE_GAIN = 0.6;       // WFS-258 : maintien de voie, rad de cap par demi-largeur d'écart à la file
static constexpr double FILE_CORR_MAX = 0.3;   // WFS-258 : correction de cap du maintien de voie, au plus (rad)

// WFS-258 (point 3 de RG-00b, ANO-138) : direction moins vive à haute vitesse, voiture du joueur. Au-dessus de 100 km/h, avec
// l'angle de roues d'avant (maxSteer / (1 + v / steerV)), un quart de volant demandait déjà toute l'adhérence (2,1 g dès
// 150 km/h, comme le volant à fond) : la moindre touche faisait virer au maximum. L'angle est ramené pour que le volant à fond
// demande 1,2 × l'adhérence du jeu (Phys::grip, RG-06), un quart de volant à peu près un quart ; réduction progressive de
// 100 à 130 km/h (aucun à-coup), rien sous 100 km/h (zone de WFS-316). maxSteer, steerV et grip ne changent pas.
static double reductionBraquageHauteVitesse(double v, double wb) {
    constexpr double V0 = 100 / 3.6, V1 = 130 / 3.6, MARGE = 1.2;
    if (v <= V0 || g_avantWfs258Vif) return 1.0;
    const double roueAvant = Phys::maxSteer / (1 + v / Phys::steerV);                // angle des roues à fond, sans réduction
    const double roueAdherence = std::atan(MARGE * Phys::grip * wb / (v * v));         // angle qui demande 1,2 × l'adhérence
    const double f = std::min(1.0, roueAdherence / roueAvant), part = std::min(1.0, (v - V0) / (V1 - V0));
    return 1.0 - part * (1.0 - f);
}

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
    // WFS-295 (RG-01) : à l'écart n de l'axe (n > 0 à droite, k > 0 virage à droite), le rayon est 1/k − n : un mètre parcouru
    // par la voiture fait avancer l'axe de 1 / (1 − k·n) m et tourner la route d'autant (repère de Frenet). Sans ce facteur,
    // la voiture à l'extérieur du virage tournait comme sur un rayon (1 + n/R) fois trop grand (épingles : ×1,34 au bord)
    facRoute_ = g_avantWfs295 ? 1.0 : std::max(0.2, 1 - k * playerX * RW);
    const double avance = speed * M * std::cos(heading) / facRoute_;   // m/s le long de l'axe de la route (négatif à contresens)
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
    // WFS-258 (RG-00b, point 1) : maintien de voie : volant lâché, l'aide ramène la voiture vers la file où le joueur l'a
    // laissée (sans cela, la dérive vers l'extérieur des virages n'était jamais corrigée) ; seulement la part de l'aide (w)
    // la file est reprise à la position de la voiture quand le joueur braque, sous 5 m/s (grille de départ, arrêt) et quand un
    // autre système a déplacé la voiture depuis le pas précédent (placement sur la grille, remise en piste, choc)
    const bool deplacee = playerX != finPasX_ || dist != finPasD_;
    if (std::abs(steer) > 0.05 || !active || g_avantWfs258 || std::abs(v) < 5 || deplacee) fileAide_ = playerX;   // banc « avant » : sans maintien de voie
    const double corrFile = clampd(-(playerX - fileAide_) * FILE_GAIN, -FILE_CORR_MAX, FILE_CORR_MAX) * sens * w;
    const double cible = axe + track.point(dist + sens * d / M, 0).th - th0 + corrFile;
    double dev = heading - cible;
    // borne élargie du côté intérieur du virage seulement (trajectoire serrée permise ; vers l'extérieur, borne du jeu web)
    const double elargi = dev * k * sens > 0 ? std::min(ELARGI_MAX, std::abs(k) * ELARGI_M) : 0.0;
    const double maxH = clampd(Phys::maxHeading - std::abs(v) * 0.0045, 0.16, Phys::maxHeading) + elargi;
    // réalignement doux (jeu web : 3 /s, double après un accident), seulement près de la direction (demi-tour libre, WFS-90)
    if (std::abs(dev) <= Phys::maxHeading + elargi) dev *= std::exp(-dt * Phys::align * w * (locked ? 2 : 1));
    // WFS-316 (RG-02) : volant tenu (|volant| ≥ 0,05) sous 60 km/h, plus de borne : elle ramenait le cap vers l'axe, donc la voiture
    // vers l'extérieur d'un virage serré (l'axe y tourne vite) ; lâché, ou au-dessus de 60 km/h (RG-05), borne gardée
    const bool tenu = partTenu(steer, v) >= 0.5;   // jusqu'à 60 km/h
    if (!tenu && std::abs(v) >= Phys::vLibre && std::abs(dev) > maxH) {   // cap borné : ramené vers la borne, jamais d'un coup
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
    fileAide_ = playerX; finPasX_ = 1e9; cedeAuDoigt_ = true; freinAide = false;   // WFS-258 : maintien de voie repris à la position de la voiture
}

void Player::update(double dt, const DriveInput& in, Track& track, bool playing) {
    if (aide != Aide::Web) { const auto& sz = cartes_v2::saisonPartie(); const bool bc = std::abs(playerX) > 1; muSol_ = bc ? sz.muBasCote : sz.muChaussee; kSol_ = muSol_ / (bc ? physique::MU_HERBE : 1.0); }   // WFS-346 (accroche) : voiture du joueur
    if (modeleMonde()) { updateMonde(dt, in, track, playing); return; }   // WFS-241 : voiture du joueur, conduite réaliste
    // WFS-256 : voiture du joueur en conduite d'avant (g_conduiteMonde faux) : pose de départ pour les chocs avec le décor
    const bool chocsDecorActifs = aide != Aide::Web && chocs::registre().taille() > 0;
    const double dAvant = dist, xAvant = playerX, capAvant = heading;
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
    } else if (!playing || aide == Aide::Web || g_avantWfs295) steer += -steer * std::min(1.0, dt * 4);   // WFS-295 (RG-03) : joueur, braquage gardé après un choc
    // angle des roues plus petit à haute vitesse
    const double wheelAngle = steer * Phys::maxSteer / (1 + std::abs(v) / Phys::steerV) * (aide == Aide::Web ? 1.0 : reductionBraquageHauteVitesse(std::abs(v), perf.wb));
    double yawRate = v / perf.wb * std::tan(wheelAngle) * perf.handling;
    const double yawLim = Phys::grip * kSol_ * perf.handling / std::max(std::abs(v), 1.0);   // limite d'adhérence (WFS-346 : × μ du sol)
    yawRate = clampd(yawRate, -yawLim, yawLim);
    double courbe = seg.curve;   // courbure de la force centrifuge (unités de Segment::curve)
    facRoute_ = 1;   // ordinateur (jeu web) : la route porte la voiture, avance inchangée
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
    const double avanceAxe = dt * speed * std::cos(heading) / facRoute_;   // WFS-295 (RG-01) : avance de l'axe de la route
    position = track.wrap(position + avanceAxe);
    dist += avanceAxe;
    odo += dt * speed;   // WFS-90
    playerX += v * std::sin(heading) * dt / RW;
    // force centrifuge dans les virages (jeu web : la route y « porte » la voiture). WFS-316 (RG-02) : voiture du joueur, volant
    // tenu : plus de force forfaitaire jusqu'à 50 km/h (effacée de 50 à 70 km/h, RG-05), la route tourne déjà sous la voiture (guider,
    // repère de Frenet) : l'ajouter faisait compter le virage deux fois (3,2 m/s vers l'extérieur à 50 km/h dans l'anneau de 14 m) ;
    // lâché, au-dessus de 70 km/h, ou ordinateur : inchangé
    const double sp = std::min(speedPct, 1.1);
    const double fTenu = aide == Aide::Web ? 0.0 : partTenu(steer, v);   // RG-05 : effacée de 50 à 70 km/h
    playerX -= (1 - fTenu) * dt * 2 * sp * sp * courbe * CENTRIFUGAL;

    // ---------- moteur, freins, frottements (m/s²) ----------
    // WFS-258 (RG-00b, point 1) : aide activée, la voiture ralentit à temps pour le virage qui vient (épingle du Grand Prix prise
    // à 228 km/h dans la vidéo de l'utilisateur) : au-dessus de la vitesse permise, gaz et nitro coupés, puis frein. Vitesse
    // permise : adhérence et freinage de cette conduite (Phys::grip, Phys::brake) sur les 600 m qui viennent, et au plus 90 % de
    // celle des ordinateurs de la Course à 5 (vitesseVirage, course5.cpp : la plus haute où la voiture reste sur la chaussée ;
    // trop optimiste dans les chicanes pour un joueur qui braque). Le joueur peut toujours freiner plus tôt ; aide désactivée,
    // rien n'est fait à sa place (RG-07 de WFS-241).
    DriveInput cmd = in;
    // Décision du PO (option A) : la limite CÈDE quand le joueur conduit lui-même : il braque fort (doigt ou flèche à 0,35 et plus,
    // VOLANT_PLEIN : l'aide lui laisse déjà tout le volant) ou il freine ; elle ne ralentit que volant lâché ou presque
    // WFS-258 (point 3, revue du lead dev, PR #271) : au-dessus de 100 km/h, la direction est réduite (un doigt à fond veut dire
    // « tourner au maximum », pas « aller plus vite ») : la limite ne cède plus qu'au frein ; sinon le joueur, qui braque plus pour
    // le même virage, passait le seuil et arrivait trop vite (et le seuil sur le braquage réduit oscillait autour de 126 km/h)
    // WFS-258 (remarque du lead dev, PR #271) : écarts contre la bascule (gaz coupé puis remis, feux stop qui clignotent) :
    //  - doigt relâché (sous 0,35) : la limite cédera au doigt sous 100 km/h, pas au-dessus (règle du point 3) ;
    //  - doigt tenu : la décision prise au moment où le joueur braque est gardée ; elle ne change qu'au-delà d'un écart (la
    //    limite reprend au-dessus de 110 km/h, recède sous 80 km/h) : avec 95 / 100 km/h, la voiture faisait encore le va-et-vient
    //    (freinée au-dessus de 100, relâchée sous 95 : 76 clignotements par tour à la Corniche) ;
    //  - le frein de l'aide se serre au-dessus de 103 % de la vitesse permise et reste serré jusqu'à revenir à la vitesse permise
    const double va = std::abs(v);
    if (std::abs(in.steer) < VOLANT_PLEIN) cedeAuDoigt_ = va <= 100 / 3.6;
    else if (cedeAuDoigt_ && va > 110 / 3.6) cedeAuDoigt_ = false;
    else if (!cedeAuDoigt_ && va < 80 / 3.6) cedeAuDoigt_ = true;
    const bool joueurConduit = (std::abs(in.steer) >= VOLANT_PLEIN && (cedeAuDoigt_ || g_avantWfs258Vif)) || in.brake;   // banc « avant » : règle de main
    bool freinAideAvant = freinAide;
    freinAide = false;
    if (aide == Aide::Activee && playing && !locked && speed > 0 && !g_avantWfs258 && !joueurConduit) {
        double vp = vitesseVirageAide(track, dist, Phys::grip * kSol_ / 9.81, Phys::brake * kSol_ / 9.81);   // WFS-346 : aide qui tient compte de μ
        if (g_vitesseVirageCourse5) vp = std::min(vp, g_vitesseVirageCourse5(track, dist) * M * 0.9);
        if (v > vp) {
            cmd.gas = false; cmd.nitro = false;
            if ((v > vp * 1.03 || freinAideAvant) && v > Phys::vRecul * 2) { cmd.brake = true; freinAide = true; }
        }
    }
    nitroOn = playing && cmd.nitro && nitro > 0.01;
    if (nitroOn) nitro = std::max(0.0, nitro - dt * perf.nitroBurn);
    const double top = MAX_SPEED * perf.top * (nitroOn ? perf.nitroMult : 1);
    const bool accelerating = playing && !locked && cmd.gas && !cmd.brake;
    // WFS-90 (RG-03) : marche arrière sans attendre l'arrêt complet (sous 3 m/s, le frein enchaîne sur la marche arrière)
    reversing = playing && !locked && cmd.brake && !cmd.gas && speed * M < Phys::vRecul;
    braking = playing && !locked && cmd.brake && speed > 0 && !reversing;
    const double av = std::abs(v);
    const double resist = (Phys::roll + Phys::drag * av * av + (accelerating ? 0 : Phys::engineBrake)) / M;
    // WFS-188 (RG-06) : résistance du bas-côté proportionnelle à la vitesse (pleine dès 15 m/s) : à l'arrêt dans l'herbe,
    // une voiture dont la poussée recalée est sous Phys::offRoad (Golf, Compass…) peut toujours repartir
    const double off = std::abs(playerX) > 1 ? Phys::offRoad / M * (perf.referenceWeb ? 1.0 : std::min(1.0, av / 15.0)) : 0;
    if (!playing) speed = std::max(0.0, speed - resist * dt);
    else if (locked) speed = speed > 0 ? std::max(0.0, speed - Phys::brake * 0.5 / M * dt) : std::min(0.0, speed + Phys::brake * 0.5 / M * dt);
    else if (reversing) speed = std::max(REV_MAX, speed - REV_ACC * dt);
    else if (braking) speed = std::max(0.0, speed - (Phys::brake * perf.freinage / M + resist) * kSol_ * dt);   // WFS-188 (RG-04) : selon la voiture ; WFS-346 : ABS, tout le freinage × μ du sol
    else if (accelerating && speed < 0) speed = std::min(0.0, speed + Phys::brake / M * dt);
    else if (accelerating) {
        const double push = perf.a0 / M * (nitroOn ? config().nitro.poussee : 1) * std::max(0.0, 1 - std::pow(std::max(0.0, speed) / top, 2.2));
        speed += ((kSol_ < 1 ? std::min(push, kSol_ * 0.55 * 9.81 / M) : push) - resist * 0.35 - off) * dt;   // WFS-346 : antipatinage, ≤ μ g × part motrice (≈ 55 %)
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
    if (chocsDecorActifs) {   // WFS-256 : même test continu et même réaction que la conduite réelle, sur la pose dans le monde
        const TrackPoint A = track.pointLisse(dAvant, xAvant * RW), B = track.pointLisse(dist, playerX * RW);
        double x = B.x, z = B.z, cap = B.th + heading, r = 0;
        const double u = speed * M, sn = std::sin(cap), cs = std::cos(cap);
        double vx = u * sn, vz = -u * cs;
        const physique::Params& P = physique::params(*car);   // masse et inertie de la fiche (la conduite d'avant n'en change pas)
        if (heurter(track, A.x, A.z, A.th + capAvant, x, z, cap, vx, vz, r, P.masse, P.Iz)) {
            // la conduite d'avant n'a pas de vitesse de travers : la voiture prend la direction de sa vitesse après le choc
            // (le long du mur), ou garde son cap si elle est renvoyée en arrière (poteau pris de face)
            const double avance = vx * std::sin(cap) - vz * std::cos(cap), v = std::hypot(vx, vz);
            const double capAligne = std::atan2(vx, -vz);
            if (avance > 0 && v > 1.0 && !enfonceParPivot(x, z, cap, capAligne)) { cap = capAligne; speed = std::min(v / M, MAX_SPEED * 1.6); }
            else speed = clampd(avance / M, REV_MAX, MAX_SPEED * 1.6);
            projeterPose(track, x, z, cap);
        }
    }
    finPasX_ = playerX; finPasD_ = dist;   // WFS-258 : pose laissée par ce pas (déplacement par un autre système détecté au suivant)
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
void Player::projeter(Track& t) { projeterPose(t, monde.x, monde.z, monde.cap); }

void Player::projeterPose(Track& t, double x, double z, double cap) {
    double s = dist;
    TrackPoint C{};
    for (int i = 0; i < 6; i++) {   // point de la ligne de la route le plus proche (Newton, converge en 2 ou 3 tours)
        C = t.pointLisse(s, 0);
        const double e = (x - C.x) * std::sin(C.th) - (z - C.z) * std::cos(C.th);
        s += e / M;
        if (std::abs(e) < 1e-6) break;
    }
    C = t.pointLisse(s, 0);
    dist = s; position = t.wrap(s);
    playerX = ((x - C.x) * std::cos(C.th) + (z - C.z) * std::sin(C.th)) / RW;
    heading = std::remainder(cap - C.th, 2 * PI_D);
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
    const double xAvant = sX_;
    monde.x = C.x; monde.z = C.z; monde.cap = C.th + heading; monde.r = r;
    const double sn = std::sin(monde.cap), cs = std::cos(monde.cap);
    monde.vx = u * sn + v * cs; monde.vz = -u * cs + v * sn;
    // WFS-256 (RG-04) : un système a écarté la voiture sur le côté (contact avec une voiture du trafic, un adversaire, la
    // police) : la vitesse qui la ramène vers ce contact est annulée (contrainte de contact), sinon elle y rentre au pas suivant
    if (mondeOk_ && playerX != xAvant) {
        const double sens = playerX > xAvant ? 1.0 : -1.0, nx = std::cos(C.th) * sens, nz = std::sin(C.th) * sens;
        const double vn = monde.vx * nx + monde.vz * nz;
        if (vn < 0) { monde.vx -= vn * nx; monde.vz -= vn * nz; }
    }
    if (!mondeOk_) monde.rapport = 1;
    mondeOk_ = true;
}

// WFS-256 : test continu tous les 10 cm du déplacement du pas (rien ne se traverse, même à 300 km/h), puis réaction
// d'un corps rigide (chocs_decor.h) au premier contact ; le reste du déplacement du pas est abandonné
bool Player::heurter(Track& track, double x0, double z0, double cap0, double& x, double& z, double& cap, double& vx, double& vz, double& r,
                     double m, double Iz) {
    const chocs::Registre& reg = chocs::registre();
    if (reg.taille() == 0) return false;
    const double hl = car->d.L / 2, hw = car->d.W / 2, pas = std::hypot(x - x0, z - z0);
    // obstacles du voisinage (une fois par pas) : à moins de la demi-diagonale de la voiture + le déplacement du pas
    obstaclesProches_.clear();
    reg.autour(track, dist, std::hypot(hl, hw) + pas + 1.0, [&](const chocs::Obstacle& o) { obstaclesProches_.push_back(o); });
    if (obstaclesProches_.empty()) return false;
    const double x1 = x, z1 = z, cap1 = cap;
    const int n = std::max(1, (int)std::ceil(pas / 0.10));
    for (int k = 1; k <= n; k++) {
        const double t = (double)k / n, ck = cap0 + std::remainder(cap1 - cap0, 2 * PI_D) * t;
        double px = x0 + (x1 - x0) * t, pz = z0 + (z1 - z0) * t;
        bool touche = false;
        for (const chocs::Obstacle& o : obstaclesProches_) {
            const chocs::Contact c = chocs::contact(px, pz, ck, hl, hw, o);
            if (!c.touche) continue;
            if (!touche) { x = px; z = pz; cap = ck; touche = true; }   // la voiture s'arrête au premier contact du pas
            const chocs::Reponse rp = chocs::reagir(x, z, vx, vz, r, m, Iz, c, o);
            px = x; pz = z;
            if (rp.choc) { chocsDecor.n++; if (rp.impact > chocsDecor.impact) { chocsDecor.impact = rp.impact; chocsDecor.genre = rp.genre; } }
        }
        if (touche) return true;
    }
    return false;
}

// WFS-256 (conduite d'avant) : pivoter la voiture sur place de cap0 à cap1 l'enfoncerait-il dans un obstacle voisin ?
bool Player::enfonceParPivot(double x, double z, double cap0, double cap1) const {
    const double hl = car->d.L / 2, hw = car->d.W / 2;
    for (const chocs::Obstacle& o : obstaclesProches_) {
        const chocs::Contact a = chocs::contact(x, z, cap0, hl, hw, o), b = chocs::contact(x, z, cap1, hl, hw, o);
        if (b.touche && b.prof > (a.touche ? a.prof : 0) + 0.005) return true;
    }
    return false;
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
    sol.mu = muSol_; if (std::abs(playerX) > 1) sol.crr = physique::CRR_HERBE;   // bas-côté ; WFS-346 (accroche) : μ de la carte (défaut : route 1, bas-côté MU_HERBE)
    sol.pente = track.pointLisse(dist, playerX * RW).slope * std::cos(heading);
    const double x0 = monde.x, z0 = monde.z, cap0 = monde.cap;
    physique::pas(P, monde, c, sol, dt);
    heurter(track, x0, z0, cap0, monde.x, monde.z, monde.cap, monde.vx, monde.vz, monde.r, P.masse, P.Iz);   // WFS-256 : chocs avec le décor
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
