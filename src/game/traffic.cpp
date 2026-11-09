// WaForSpeed — trafic simulé, port de js/game/traffic.js (et des chocs de js/game/update.js).
#include "traffic.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include "../core/jobs.h"
#include "trace_cartes.h"

namespace wfs {

static double clampd(double v, double a, double b) { return v < a ? a : (v > b ? b : v); }
static int signi(double v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// nextRand(car) du jeu web
static double nextRand(TrafficCar& car) {
    car.rng = (car.rng ^ (car.rng >> 15)) * 2246822507u + 0x9e3779b9u;
    return car.rng / 4294967296.0;
}

void Traffic::reset(Track& track, uint32_t s, int n, double playerPos, bool menu) {
    track_ = &track;
    cars.clear();
    plafond = SANS_PLAFOND;   // WFS-281 : fixé à la fin, une fois toutes les voitures du départ et la réserve posées
    seed = s ? s : 1;
    clock = sim = menu ? 0 : -3;   // compte à rebours de 3 s avant le départ (trafficClock du jeu web)
    scrapeCd_ = 0;
    // WFS-243 (RG-ANO-144) : anneaux des ronds-points du tracé 2 : la courbure y change de sens (comme giratoire::calculer)
    anneaux_.clear();
    for (const trace::Section& s : track.sections()) {
        if (s.type != trace::Type::RondPoint || s.fin - s.debut < 20) continue;
        const double c0 = track.segAt(s.debut + 2).curve; int64_t e0 = -1, e1 = -1;
        for (int64_t a = s.debut + 1; a < s.fin; a++) if (track.segAt(a).curve * c0 < 0) { if (e0 < 0) e0 = a; e1 = a; }
        if (e0 >= 0) anneaux_.push_back({e0, e1});
    }
    reserve_.clear(); regulT_ = clock;
    addCars(n, playerPos);
    if (track.version() == 2) ajouterTrace2(n, playerPos);   // WFS-136 ; tracé 1 (jeu web) : rien ne change
    if (totalFixe) {   // WFS-281 (RG-02) : réserve de la montée de difficulté, mêmes tirages que les addCars(6) de la course
        const int depart = (int)cars.size(), k = (int)std::lround(depart * MONTEE_MAX);
        cars.reserve((size_t)(depart + k));   // aucune allocation quand la réserve s'active
        addCars(k, playerPos);
        reserve_.assign(cars.end() - k, cars.end());
        cars.resize((size_t)depart);
        plafond = depart + k;
    }
}

// WFS-136 (RG-04) : +30 % de voitures sur l'autoroute (au prorata de sa longueur) ; (RG-03) 1 ou 2 voitures par carrefour
// à feux, qui traversent au rouge. Tirages par graine (même graine → même trafic).
void Traffic::ajouterTrace2(int n, double playerPos) {
    const double L = track_->length(), La = trace::longueurAutoroute(*track_);
    Mulberry32 R(seed * 2654435761u + 0x5bd1e995u);
    const int extra = (int)std::lround(0.3 * n * La / std::max(L, 1.0));
    for (int i = 0; i < extra; i++) {
        addCars(1, playerPos);
        // replacée sur un tronçon d'autoroute tiré au hasard (le reste de son tirage est gardé)
        double u = R.next() * La;
        for (const trace::Section& s : track_->sections()) {
            if (!s.autoroute) continue;
            const double l = s.finU() - s.debutU();
            if (u < l) { cars.back().z = track_->wrap(s.debutU() + u); break; }
            u -= l;
        }
        cars.back().prevDz = track_->relZ(cars.back().z - (playerPos + PLAYER_Z));
    }
    for (const trace::Section& s : track_->sections()) {
        if (s.feu < 0 || s.depart) continue;
        const int k = R.next() < 0.5 ? 1 : 2;
        for (int j = 0; j < k; j++) {
            TrafficCar c;
            c.type = (int)std::floor(R.next() * WEB_CAR_COUNT);
            c.len = WEB_CARS[c.type].L / M; c.w = WEB_CARS[c.type].W / (2 * RW);
            c.color = (int)std::floor(R.next() * TRAFFIC_PAINT_COUNT);
            c.rng = (uint32_t)(R.next() * 4294967296.0);
            c.traverse = s.feu; c.rangTraverse = j;
            c.z = track_->wrap(s.ligneArret * SEG + trace::CARREFOUR_M / M);
            c.lane = c.target = 0; c.speed = c.want = 0; c.think = 1e9;
            placerTraverse(c);
            c.prevDz = track_->relZ(c.z - (playerPos + PLAYER_Z));
            cars.push_back(c);
        }
    }
}

// position d'une voiture qui traverse, déduite de l'horloge du trafic (aucun état caché : toujours la même à la même heure)
// rouge + 1 s (+ 1,6 s pour la 2e) : de 24 m à gauche du centre à 24 m à droite, à 36 km/h ; cachée hors du passage
void Traffic::placerTraverse(TrafficCar& car) const {
    constexpr double X0 = 3.2, VX = 10.0 / RW;   // demi-largeurs de route ; demi-largeurs par seconde (10 m/s)
    double tp = 0;
    const bool rouge = trace::feuCarrefour(car.traverse, sim, &tp) == trace::Feu::Rouge;
    const double x = -X0 + VX * (tp - 1.0 - 1.6 * car.rangTraverse);
    car.cache = !rouge || x < -X0 || x > X0;
    car.offset = car.prevOff = car.cache ? -X0 : x;
    car.laneYaw = std::numbers::pi / 2;   // de profil : nez vers la droite de la route
    car.speed = 0; car.signal = 0;
}

// addCars(n) du jeu web
void Traffic::addCars(int n, double playerPos) {
    const MapDef& map = *track_->map();
    if (totalFixe) n = std::min(n, plafond - (int)cars.size());   // WFS-281 (RG-02) : pas au-delà du plafond
    // WFS-281 : d'abord les voitures de la réserve (préparées au chargement), posées devant le joueur comme une voiture neuve
    for (; n > 0 && !reserve_.empty(); n--) {
        TrafficCar c = reserve_.front(); reserve_.erase(reserve_.begin());   // au plus 15 % des voitures : petit tableau
        Mulberry32 R(c.rng ^ (uint32_t)(cars.size() * 2654435761u));
        c.z = tirerZ(R, c.lane, c.len, playerPos);
        c.offset = c.prevOff = (LANE_X[c.lane] + (c.deuxRoues ? DROITE_VOIE : 0)) * facteurVoie(c.z);
        c.zPrec = c.z; c.offPrec = c.offset; c.speed = c.want;
        c.prevDz = track_->relZ(c.z - (playerPos + PLAYER_Z));
        cars.push_back(c);
    }
    for (int i = 0; i < n; i++) {
        Mulberry32 R(seed * 7919u + (uint32_t)(cars.size() + 1) * 104729u);
        TrafficCar c;
        c.type = (int)std::floor(R.next() * WEB_CAR_COUNT);
        c.len = WEB_CARS[c.type].L / M;
        c.want = MAX_SPEED * (map.trafficMin + R.next() * (map.trafficMax - map.trafficMin));
        c.lane = (int)std::floor(R.next() * LANES);
        const double z = tirerZ(R, c.lane, c.len, playerPos);
        c.color = (int)std::floor(R.next() * TRAFFIC_PAINT_COUNT);
        c.w = WEB_CARS[c.type].W / (2 * RW);
        c.target = c.lane; c.offset = c.prevOff = LANE_X[c.lane] * facteurVoie(z);   // WFS-243 : déjà resserrée si posée dans un anneau
        if (track_->version() == 2) {   // WFS-232 (RG-03) : deux-roues, tirage à part (celui du jeu web reste le même)
            // quota exact (et non un tirage par véhicule : 29 véhicules en Campagne) : le véhicule i est un deux-roues quand
            // floor((i + 1) × part + phase) dépasse floor(i × part + phase), phase et modèle tirés par la graine
            const double part = deuxRouesPermis ? partDeuxRoues(map.id) : 0, phase = Mulberry32(seed * 2246822519u + 0x27d4eb2fu).next(), i0 = (double)cars.size();
            Mulberry32 D(seed * 2246822519u + (uint32_t)(cars.size() + 1) * 3266489917u);
            if (std::floor((i0 + 1) * part + phase) > std::floor(i0 * part + phase)) {
                const int k = std::min(DEUX_ROUES_COUNT - 1, (int)(D.next() * DEUX_ROUES_COUNT));
                c.deuxRoues = k + 1; c.len = DEUX_ROUES[k].L / M; c.w = DEUX_ROUES[k].W / (2 * RW);
                c.offset = c.prevOff = (LANE_X[c.lane] + DROITE_VOIE) * facteurVoie(z);
            }
        }
        c.z = z; c.speed = c.want;
        c.rng = (uint32_t)(R.next() * 4294967296.0);
        c.think = 1 + R.next() * 4;
        c.prevDz = track_->relZ(z - (playerPos + PLAYER_Z));
        cars.push_back(c);
    }
}

// place d'une voiture neuve (addCars du jeu web) : 36 m devant le joueur à 36 m derrière, sans chevaucher une voiture de sa file
double Traffic::tirerZ(Mulberry32& R, int lane, double len, double playerPos) const {
    const double L = track_->length();
    double z = 0;
    for (int k = 0; k < 24; k++) {
        z = track_->wrap(playerPos + PLAYER_Z + 6000 + R.next() * (L - 12000));
        bool clash = false;
        for (const auto& o : cars)
            if (o.lane == lane && std::abs(track_->relZ(o.z - z)) < (o.len + len) / 2 + 30 / M) { clash = true; break; }
        if (!clash) break;
    }
    return z;
}

// laneFree(car, lane) : la file est-elle libre autour de la voiture ?
bool Traffic::laneFree(const TrafficCar& car, int lane, const TrafficView& pv) const {
    const double lx = LANE_X[lane];
    for (const auto& o : cars) {
        if (&o == &car || o.cache || (std::abs(o.offset - lx) > LANE_HALF && o.target != lane)) continue;
        const double dz = track_->relZ(o.z - car.z);
        if (dz >= 0 && dz < (o.len + car.len) / 2 + std::max(8 / M, (car.speed - o.speed) * 1.6)) return false;
        if (dz < 0 && -dz < (o.len + car.len) / 2 + std::max(10 / M, (o.speed - car.speed) * 2.2)) return false;
    }
    if (std::abs(pv.playerX - lx) < LANE_HALF + 0.1 && std::abs(track_->relZ(pv.position - car.z)) < 22 / M) return false;
    return true;
}

// voiture de devant (dans la même file ou en train d'y entrer, + le joueur) pour les voitures [begin, end).
// Ne fait que lire `cars` : peut tourner sur plusieurs threads.
void Traffic::findLeaders(const TrafficView& pv, int begin, int end) {
    for (int i = begin; i < end; i++) {
        const TrafficCar& car = cars[(size_t)i];
        double gap = std::numeric_limits<double>::infinity(), leadSpeed = car.want;
        const double tx = LANE_X[car.target];
        for (const auto& o : cars) {
            if (&o == &car || o.cache) continue;
            if (std::abs(o.offset - car.offset) > LANE_HALF && std::abs(o.offset - tx) > LANE_HALF) continue;
            const double dz = track_->relZ(o.z - car.z);
            if (dz <= 0) continue;
            const double g = dz - (o.len + car.len) / 2;
            if (g < gap) { gap = g; leadSpeed = o.speed; }
        }
        if (!pv.garage) {
            const double dz = track_->relZ(pv.position - car.z);
            if (dz > 0 && (std::abs(pv.playerX - car.offset) < LANE_HALF || std::abs(pv.playerX - tx) < LANE_HALF)) {
                const double g = dz - (pv.myLen + car.len) / 2;
                if (g < gap) { gap = g; leadSpeed = pv.speed; }
            }
        }
        // WFS-136 (RG-03) : feu rouge (ou orange, s'il reste plus d'1 s de route avant la ligne) : la ligne d'arrêt est un
        // obstacle arrêté ; la file se forme derrière la première voiture avec les règles habituelles (embouteillages WFS-111)
        if (car.traverse < 0) for (const trace::Section& s : track_->sections()) {
            if (s.feu < 0 || s.depart) continue;
            const double dz = track_->relZ(s.ligneArret * SEG - car.z);
            if (dz <= 0 || dz > 150 / M) continue;
            const trace::Feu f = trace::feuCarrefour(s.feu, sim);
            const double g = dz - car.len / 2;
            if ((f == trace::Feu::Rouge || (f == trace::Feu::Orange && g > car.speed * 1.0)) && g < gap) { gap = g; leadSpeed = 0; }
        }
        gap_[(size_t)i] = gap; lead_[(size_t)i] = leadSpeed;
    }
}

// WFS-243 (RG-ANO-144) : resserrement des files dans l'anneau des ronds-points, continu (rampes) ; 1 ailleurs (tracé 1 : toujours)
double Traffic::facteurVoie(double z) const {
    if (anneaux_.empty()) return 1;
    const double a = track_->wrap(z) / SEG, rampe = RAMPE_ANNEAU_M / SEG_M;
    for (const auto& [e0, e1] : anneaux_) {
        if (a < e0 - rampe || a > e1 + 1 + rampe) continue;
        const double t = a < e0 ? (e0 - a) / rampe : a > e1 + 1 ? (a - e1 - 1) / rampe : 0;   // 0 dans l'anneau, 1 au bout de la rampe
        return FACTEUR_ANNEAU + (1 - FACTEUR_ANNEAU) * t;
    }
    return 1;
}

// stepTraffic(h) du jeu web.
// Mode exact (parallel = false) : chaque voiture voit les voitures déjà avancées dans ce pas, comme en JS.
// Mode parallèle : toutes les voitures de devant sont cherchées d'abord, sur tous les cœurs, à partir de l'état
// du début du pas ; le reste du pas (vitesses, changements de file) reste dans l'ordre du jeu web.
void Traffic::step(double h, const TrafficView& pv) {
    const size_t n = cars.size();
    gap_.resize(n); lead_.resize(n);
    if (parallel) jobs().parallelFor((int)n, 16, [&](int b, int e) { findLeaders(pv, b, e); });
    for (size_t i = 0; i < n; i++) {
        TrafficCar& car = cars[i];
        if (car.crashed) {
            car.crashT += h;
            car.speed = std::max(0.0, car.speed - MAX_SPEED * 0.55 * h);
            car.offset = clampd(car.offset + car.crashVx * h, -1.12, 1.12);
            car.crashVx *= std::max(0.0, 1 - 2.5 * h);
            car.crashYaw += car.crashSpin * h; car.crashSpin *= std::max(0.0, 1 - 1.8 * h);
            if (car.deuxRoues) {   // WFS-232 (RG-05) : chute sur le flanc du côté où il penchait (sinon du côté où il est poussé)
                car.inclPrec = car.inclinaison;
                const double sens = std::abs(car.inclinaison) > 1e-3 ? (car.inclinaison > 0 ? 1 : -1) : (car.crashVx >= 0 ? 1 : -1);
                double a = std::max(std::abs(car.inclinaison), AMORCE_CHUTE);
                if (a < TOMBE) {
                    car.chute += 9.81 / HAUTEUR_CHUTE * std::sin(a) * h;
                    a = std::min(TOMBE, a + car.chute * h);
                    if (a >= TOMBE) car.chute = 0;   // posé au sol
                }
                car.inclinaison = sens * a;
            }
            if (car.crashT > 7 && std::abs(track_->relZ(pv.position - car.z)) > 60 / M) {
                car.crashed = false; car.speed = 0; car.signal = 0; car.pending = false;
                car.inclinaison = car.inclPrec = car.chute = 0;   // WFS-232 : relevé (hors de la vue)
                int best = 0; double bd = 9;
                for (int l = 0; l < LANES; l++) { const double d = std::abs(LANE_X[l] - car.offset); if (d < bd) { bd = d; best = l; } }
                car.lane = car.target = best;
            }
            car.laneYaw = 0;
            car.z = track_->wrap(car.z + car.speed * h);
            continue;
        }
        if (car.traverse >= 0) { placerTraverse(car); continue; }   // WFS-136 : traversée du carrefour au rouge
        if (!parallel) findLeaders(pv, (int)i, (int)i + 1);
        const double gap = gap_[i], leadSpeed = lead_[i];
        // WFS-136 (RG-02, RG-04) : 40 km/h dans l'anneau, 60 km/h dans les épingles (aussi 30 m avant), +15 % sur l'autoroute ;
        // tracé 1 : want = car.want (facteur 1, limite infinie), le calcul du jeu web est inchangé
        double want = track_->sections().empty() ? car.want
            : std::min({car.want * trace::facteurTrafic(*track_, car.z), trace::vitesseTrafic(*track_, car.z), trace::vitesseTrafic(*track_, car.z + 30 / M)});
        if (car.deuxRoues) {   // WFS-232 : vitesse de courbe d'un deux-roues : inclinaison de confort au point le plus serré devant lui,
            // atteinte par un freinage régulier (v² = vc² + 2 · a · d, a = FREIN_VIRAGE) : il ralentit à l'approche, sans à-coup
            const double v = car.speed * M, vue = VUE_VIRAGE_M + v * v / (2 * FREIN_VIRAGE);
            double vmax = 1e9;
            for (double d = 0; d <= vue; d += SEG_M) {
                const double k = std::abs(track_->findSegment(car.z + d / M).curve * CURVE_K / SEG_M);
                if (k > 1e-6) vmax = std::min(vmax, std::sqrt(9.81 / k * std::tan(INCLINAISON_CONFORT) + 2 * FREIN_VIRAGE * std::max(0.0, d - 2)));
            }
            if (vmax < 1e8) want = std::min(want, vmax / M);
        }
        // vitesse visée : sa vitesse de croisière, ou celle qui garde la distance de sécurité
        const double safe = SAFE_MIN + car.speed * SAFE_TIME;
        double target = want;
        if (gap < safe) target = std::min(target, std::max(0.0, leadSpeed - (safe - gap) * 1.2));
        const double acc = target > car.speed ? MAX_SPEED * 0.1 : MAX_SPEED * 0.7;
        car.speed = std::max(0.0, car.speed + clampd(target - car.speed, -acc * h, acc * h));
        if (gap < 0.8 / M) car.speed = std::min(car.speed, leadSpeed * 0.95);
        // envie de changer de file (bloqué derrière plus lent, ou de temps en temps)
        car.think -= h;
        const double kv = facteurVoie(car.z);   // WFS-243 (RG-ANO-144) : files resserrées dans l'anneau, pas de changement de file
        if (car.think <= 0 && car.target == car.lane && !car.pending && kv >= 1) {
            const double r = nextRand(car);
            car.think = 1.5 + r * 4;
            const bool blocked = gap < safe * 1.6 && leadSpeed < want * 0.85;
            if (blocked || r < 0.12) {
                const int dir = nextRand(car) < 0.5 ? -1 : 1;
                // d'abord le clignotant, le changement de file vient ~1 s plus tard
                for (int nl : {car.lane + dir, car.lane - dir})
                    if (nl >= 0 && nl < LANES && laneFree(car, nl, pv)) {
                        car.pending = true; car.pendLane = nl; car.pendT = 0.9 + nextRand(car) * 0.5;
                        car.signal = signi(nl - car.lane); break;
                    }
            }
        }
        if (car.pending) {
            car.pendT -= h;
            if (car.pendT <= 0) {
                if (laneFree(car, car.pendLane, pv)) car.target = car.pendLane; else car.signal = 0;
                car.pending = false;
            }
        }
        // abandonne le changement si la file se remplit, sinon glisse vers elle
        const double droite = car.deuxRoues ? DROITE_VOIE : 0;   // WFS-232 : deux-roues du côté droit de sa voie
        if (car.target != car.lane && !laneFree(car, car.target, pv) && std::abs(car.offset - (LANE_X[car.lane] + droite) * kv) < 0.2) { car.target = car.lane; car.signal = 0; }
        const double d = (LANE_X[car.target] + droite) * kv - car.offset;
        const double move = clampd(d, -0.5 * h, 0.5 * h);
        car.offset += move;
        car.laneYaw = std::atan2(move * RW, std::max(car.speed * M * h, 1e-3));
        if (std::abs(d) < 0.01) { car.lane = car.target; if (!car.pending) car.signal = 0; }
        car.z = track_->wrap(car.z + car.speed * h);
        if (car.deuxRoues) {   // WFS-232 (RG-04) : inclinaison d'équilibre dans la courbe, sans à-coup (filtre du premier ordre)
            const double kappa = track_->findSegment(car.z).curve * CURVE_K / SEG_M;   // 1/m, + à droite
            car.inclPrec = car.inclinaison;
            const double pas = (inclinaisonEquilibre(car.speed * M, kappa) - car.inclinaison) * std::min(1.0, h * LISSAGE_INCLINAISON);
            car.inclinaison += std::clamp(pas, -ROULIS_MAX * h, ROULIS_MAX * h);   // roulis au plus 60°/s (pas d'à-coup)
        }
    }
}

// advanceTraffic() : pas fixes jusqu'à l'horloge (au plus 600 pas d'un coup)
void Traffic::advance(double dt, const TrafficView& pv) {
    clock += dt;
    int steps = (int)std::floor((clock - sim) / T_STEP);
    if (steps > 600) { sim = clock - 600 * T_STEP; steps = 600; }
    for (int i = 0; i < steps; i++) { step(T_STEP, pv); sim += T_STEP; }
    // WFS-281 : régulation de la densité une fois par seconde, dans le jeu seulement (les tests du jeu web n'en ont pas)
    if (totalFixe && sim - regulT_ >= 1.0) { regulT_ = sim; regulerDensite(pv.position); }
}

// WFS-281 (ANO-151, RG-02, RG-03) : sur un tracé fermé, les voitures rapides rattrapent les lentes et forment des pelotons
// qui grossissent avec le temps ; le joueur qui en rattrape un en voit jusqu'à 8 ou 14 d'un coup (10 min de course :
// médiane de 8 véhicules dessinés pendant 30 s en Ville, 4 au départ). Hors de la vue (devant au-delà de 300 m, derrière
// au-delà de 40 m), une zone qui dépasse REGUL_MAX × la densité moyenne du tracé (arrondie) envoie ses voitures les plus lentes (celles
// qui retiennent le peloton) à plus de REGUL_LOIN_M du joueur, à une place libre de leur file : rien ne change à l'écran, le
// total ne change pas, et le trafic qui arrive dans la vue garde la densité du départ (« un trafic urbain réel qui circule »).
void Traffic::regulerDensite(double playerPos) {
    const double Lm = track_->length() * M;
    if (Lm < 2 * REGUL_LOIN_M + 400) return;
    int actifs = 0;
    for (const auto& c : cars) actifs += !c.cache && c.traverse < 0;
    const double densite = actifs / Lm;   // voitures par mètre
    for (const auto& zone : REGUL_ZONES) {
        const int max = std::max(1, (int)std::lround(REGUL_MAX * densite * (zone[1] - zone[0])));
        int idx[64], n = 0;   // 64 voitures au plus par zone : bien au-delà des 4 à 6 attendues (total fixe, 120 voitures au plus)
        for (size_t i = 0; i < cars.size() && n < 64; i++) {
            const TrafficCar& c = cars[i];
            if (c.cache || c.traverse >= 0 || c.crashed) continue;
            const double dz = track_->relZ(c.z - playerPos) * M;
            if (dz >= zone[0] && dz <= zone[1]) idx[n++] = (int)i;
        }
        if (n <= max) continue;
        std::sort(idx, idx + n, [&](int a, int b) { return cars[(size_t)a].want < cars[(size_t)b].want || (cars[(size_t)a].want == cars[(size_t)b].want && a < b); });
        for (int k = 0; k < n - max; k++) {
            TrafficCar& c = cars[(size_t)idx[k]];
            Mulberry32 R(seed * 40503u + c.rng + (uint32_t)std::lround(sim * 60));
            double z = c.z;
            for (int essai = 0; essai < 24; essai++) {
                z = track_->wrap(playerPos + (REGUL_LOIN_M + R.next() * (Lm - 2 * REGUL_LOIN_M)) / M);
                bool libre = true;
                for (const auto& o : cars)
                    if (&o != &c && o.lane == c.lane && std::abs(track_->relZ(o.z - z)) < (o.len + c.len) / 2 + 30 / M) { libre = false; break; }
                if (libre) break;
            }
            c.z = c.zPrec = z;
            c.target = c.lane; c.pending = false; c.signal = 0; c.laneYaw = c.yawPrec = 0;
            c.offset = c.prevOff = c.offPrec = (LANE_X[c.lane] + (c.deuxRoues ? DROITE_VOIE : 0)) * facteurVoie(z);
            c.speed = c.want;
            c.prevDz = track_->relZ(z - (playerPos + PLAYER_Z));
        }
    }
}

int Traffic::dessines(double position, bool basse, bool dense, const TrafficCar** out) const {
    struct Vis { double dz; const TrafficCar* c; };
    Vis vis[64]; int n = 0;
    for (const auto& c : cars) {
        if (c.cache) continue;   // WFS-136 (accroche) : voiture qui traverse, hors de son passage
        const double dz = track_->relZ(c.z - position);
        if (dz < -30 / M || dz > 300 / M || n >= 64) continue;
        vis[n++] = {std::abs(dz), &c};
    }
    std::sort(vis, vis + n, [](const Vis& a, const Vis& b) { return a.dz < b.dz; });
    n = std::min(n, dense ? 3 : basse ? 6 : TRAFIC_DESSINES_MAX);   // WFS-134 (RG-11) : Basse : moins de voitures, jamais de cubes
    if (basse) while (n > 0 && vis[n - 1].dz * M > 120) n--;        // Basse : trafic dessiné jusqu'à 120 m
    for (int k = 0; k < n; k++) out[k] = vis[k].c;
    return n;
}

// crashTraffic(car, impact, side) : la voiture percutée part en glissade
void Traffic::crash(TrafficCar& car, double impact, int side) {
    car.crashed = true; car.crashT = 0;
    car.crashVx = side * (0.4 + impact * 1.6); car.crashYaw = 0; car.crashSpin = side * (1.5 + impact * 7);
    car.target = car.lane;
    // WFS-232 (RG-05) : un deux-roues percuté bascule tout de suite (vitesse de chute donnée par le choc) et tourne peu sur
    // lui-même : il tombe et glisse au lieu de pivoter debout
    if (car.deuxRoues) { car.crashSpin *= 0.25; car.chute = 1.5 + impact * 2; }
}

bool g_avantWfs333 = false;

// WFS-333 : recouvrement de deux rectangles orientés par les 4 axes séparateurs (axes propres de chaque rectangle)
double recouvrement(const Emprise& a, const Emprise& b, double* nx, double* nz) {
    const double ax[4][2] = {{std::sin(a.cap), std::cos(a.cap)}, {std::cos(a.cap), -std::sin(a.cap)},
                             {std::sin(b.cap), std::cos(b.cap)}, {std::cos(b.cap), -std::sin(b.cap)}};
    auto demi = [](const Emprise& e, double ux, double uz) {
        return e.demiL * std::abs(std::sin(e.cap) * ux + std::cos(e.cap) * uz) + e.demiW * std::abs(std::cos(e.cap) * ux - std::sin(e.cap) * uz);
    };
    double mini = 1e18, bx = 1, bz = 0;
    for (const auto& u : ax) {
        const double d = (b.x - a.x) * u[0] + (b.z - a.z) * u[1];
        const double o = demi(a, u[0], u[1]) + demi(b, u[0], u[1]) - std::abs(d);
        if (o < mini) { mini = o; const double sg = d >= 0 ? 1 : -1; bx = u[0] * sg; bz = u[1] * sg; }
    }
    if (nx) *nx = bx;
    if (nz) *nz = bz;
    return mini;
}

Emprise empriseJoueur(const Player& pl) {
    return {pl.playerX * RW, 0, pl.heading + pl.spinYaw, pl.car->d.L / 2, pl.car->d.W / 2};
}

Emprise empriseTrafic(const TrafficCar& c, double dz) {
    // WFS-136 : une voiture qui traverse est déjà de profil par son cap (laneYaw = π/2)
    return {c.offset * RW, dz * M, c.laneYaw + (c.crashed ? c.crashYaw : 0), c.len * M / 2, c.w * RW};
}

bool separerDuJoueur(const Player& pl, double& rd, double& x, double longueurM, double largeurM) {
    if (!pl.car) return false;
    const Emprise j = empriseJoueur(pl), v{x * RW, (rd - pl.dist) * M, 0, longueurM / 2, largeurM / 2};
    double nx = 0, nz = 0;
    const double o = recouvrement(j, v, &nx, &nz);
    if (o <= -JEU_CONTACT_M) return false;
    rd += nz * (o + JEU_CONTACT_M) / M; x += nx * (o + JEU_CONTACT_M) / RW;
    return true;
}

// chocs avec le joueur (hitCar) et quasi-contacts, comme la fin de update() du jeu web.
// WFS-333 (RG-02, RG-03) : emprises tournées comme le dessin (tête-à-queue du joueur, rotation d'une voiture accidentée), et
// à la fin, aucune voiture dessinée dans celle du joueur : la voiture du trafic encore en recouvrement est écartée.
// Voiture du joueur seulement (Aide::Activee / Desactivee, posée à chaque pas par aide_direction.cpp) : l'ordinateur et les
// références du jeu web (Aide::Web, test_course) gardent exactement les chocs du jeu web.
HitEvents Traffic::collide(Player& pl, double dt) {
    HitEvents ev;
    const bool dessine = pl.aide != Aide::Web && !g_avantWfs333;   // WFS-333 : emprises dessinées
    scrapeCd_ -= dt;
    const CarDef& me = *pl.car;
    // emprise réelle de la voiture tournée de « heading » (rectangle orienté) ; WFS-333 : + tête-à-queue (spinYaw), tel que dessiné
    const double spin = dessine ? pl.spinYaw : 0.0, capJ = pl.heading + spin;
    const double ch = spin == 0 ? std::cos(pl.heading) : std::abs(std::cos(capJ)), sh = std::abs(std::sin(capJ));
    const double pz = pl.position + PLAYER_Z;
    const double myLen = (me.d.L * ch + me.d.W * sh) / M, myW = (me.d.W * ch + me.d.L * sh) / (2 * RW);
    for (auto& car : cars) {
        if (car.cache) continue;   // WFS-136 : voiture qui traverse, hors de son passage
        const double dz = track_->relZ(car.z - pz);
        const double lat = std::abs(pl.playerX - car.offset);
        // WFS-136 : une voiture qui traverse est de profil (longueur en travers de la route)
        const bool travers = car.traverse >= 0 && !car.crashed;
        double cLen = travers ? car.w * 2 * RW / M : car.len, cW = travers ? car.len * M / (2 * RW) : car.w;
        if (dessine && car.crashed) {   // WFS-333 : voiture accidentée, tournée sur elle-même : emprise de son rectangle tourné
            const double a = car.laneYaw + car.crashYaw, cc = std::abs(std::cos(a)), ss = std::abs(std::sin(a));
            cLen = (car.len * M * cc + car.w * 2 * RW * ss) / M; cW = (car.w * 2 * RW * cc + car.len * M * ss) / (2 * RW);
        }
        const double touchLat = myW + cW, touchZ = (myLen + cLen) / 2;
        // WFS-333 : rectangle englobant atteint ; voitures tournées (tête-à-queue, accident) : contact seulement si les
        // rectangles orientés se touchent (sinon choc à distance, sans contact visible)
        const bool tourne = dessine && (spin != 0 || car.crashed);
        const bool contact = lat < touchLat && std::abs(dz) < touchZ &&
                             (!tourne || recouvrement(empriseJoueur(pl), empriseTrafic(car, dz)) > 0);
        if (contact) {
            // ---------- hitCar ----------
            const int side = signi(pl.playerX - car.offset) ? signi(pl.playerX - car.offset) : 1;
            const double overlapZ = touchZ - std::abs(dz), overlapX = touchLat - lat;
            if (overlapX * RW < overlapZ * M * 0.7) {
                // contact latéral : les carrosseries frottent, on s'écarte l'un de l'autre
                pl.playerX += side * (overlapX + 0.004); car.offset -= side * overlapX * 0.35 * pl.perf.mass;
                pl.heading *= 0.5;
                pl.speed *= 0.94; car.speed *= 0.97;
                if (scrapeCd_ <= 0) { scrapeCd_ = 0.18; ev.scrapes++; ev.shake = std::max(ev.shake, 0.3f); }   // WFS-70 (RG-03, RG-11) : frottement 0,3
            } else if (dz > 0) {
                // on percute l'arrière : la voiture est poussée, on perd notre vitesse
                const double rel = std::max(0.0, pl.speed - car.speed), impact = rel / MAX_SPEED, v0 = car.speed;
                const double back = overlapZ + 0.25 / M;
                pl.position = track_->wrap(pl.position - back); pl.dist = std::max(0.0, pl.dist - back);
                car.speed = v0 + rel * 0.55;
                pl.speed = v0 + rel * clampd(0.04 + 0.08 * pl.perf.mass, 0.06, 0.2);   // une voiture lourde garde plus de vitesse
                if (impact / pl.perf.mass > 0.28) { crash(car, impact, -side); pl.accident(impact, side); ev.accidents++; ev.shake = 0.9f; ev.push('a', impact); }
                else {
                    ev.bumps++; ev.shake = std::max(ev.shake, (float)(0.25 + impact * 1.5)); ev.push('b', impact);
                    if (impact > 0.15) crash(car, impact * 0.5, -side);
                }
            } else {
                // une voiture nous rentre dedans par l'arrière (on a pilé devant elle)
                const double rel = std::max(0.0, car.speed - pl.speed), impact = rel / MAX_SPEED;
                car.z = track_->wrap(pl.position + PLAYER_Z - touchZ - 0.25 / M);
                pl.speed += rel * 0.5;
                // WFS-333 (RG-02, ANO-180) : une voiture accidentée (en glissade, de travers) ne prend pas la vitesse du joueur :
                // elle n'est plus conduite, elle garde la sienne (bornée à celle du joueur) et reste derrière ; avant, chaque
                // contact la recalait juste derrière lui à sa vitesse : elle le suivait de travers, collée, jusqu'au trottoir
                car.speed = dessine && car.crashed ? std::min(car.speed, pl.speed * 0.9) : pl.speed - rel * 0.1;
                ev.bumps++; ev.shake = std::max(ev.shake, (float)(0.25 + impact * 1.5)); ev.push('b', impact);
            }
            if (dessine) separer(pl, car);
            car.prevDz = track_->relZ(car.z - (pl.position + PLAYER_Z));
            continue;
        }
        // quasi-contact : on double une voiture en la frôlant à plus de la moitié de la vitesse max
        if (car.prevDz > 0 && dz <= 0 && dz > -2000 && lat >= touchLat && lat < touchLat + NEAR_MISS && pl.speed > MAX_SPEED * 0.5 && !car.crashed) {
            ev.nearMiss++; ev.push('n', 0);
            pl.nitro = std::min(1.0, pl.nitro + pl.perf.nitroGain);
        }
        car.prevDz = dz;
    }
    // WFS-333 (RG-03) : dernière vérification, sur les emprises dessinées : une voiture repoussée contre une autre ou entrée
    // pendant le pas (glissade, tête-à-queue) ne reste jamais dans celle du joueur
    if (dessine)
        for (auto& car : cars)
            if (!car.cache && std::abs(track_->relZ(car.z - pz)) * M < 12) separer(pl, car);
    return ev;
}

// WFS-333 (RG-03) : écarte la voiture du trafic de celle du joueur si leurs emprises dessinées se recouvrent (direction de
// moindre recouvrement, JEU_CONTACT_M de jeu) ; le joueur n'est pas déplacé (sa conduite garde la main)
void Traffic::separer(const Player& pl, TrafficCar& car) {
    const double dz = track_->relZ(car.z - (pl.position + PLAYER_Z));
    if (std::abs(dz) * M > 12) return;
    double nx = 0, nz = 0;
    const double o = recouvrement(empriseJoueur(pl), empriseTrafic(car, dz), &nx, &nz);
    if (o <= -JEU_CONTACT_M) return;
    car.offset += nx * (o + JEU_CONTACT_M) / RW;
    car.z = track_->wrap(car.z + nz * (o + JEU_CONTACT_M) / M);
    car.prevDz = track_->relZ(car.z - (pl.position + PLAYER_Z));
}

void Traffic::watch(const Player& pl) {
    for (auto& car : cars) car.prevDz = track_->relZ(car.z - (pl.position + PLAYER_Z));
}

} // namespace wfs
