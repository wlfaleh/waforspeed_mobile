// WaForSpeed — duel en ligne (WFS-21) : voir duel.h. Les noms des fonctions du jeu web sont rappelés en commentaire.
#include "duel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>   // WFS-242 : jeton de la grille
#include "course5.h"   // WFS-242 (RG-05) : places de la grille de départ
#include "player.h"
#include "trace_cartes.h"   // WFS-136 (accroche A6) : version du tracé dans la présence

namespace wfs::duel {

static double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

// hashStr : FNV-1a sur charCodeAt(0) de chaque caractère (la chaîne UTF-8 est décodée en caractères)
uint32_t hashStr(std::string_view s) {
    uint32_t h = 2166136261u;
    auto mix = [&](uint32_t u) { h ^= u; h *= 16777619u; };
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = (unsigned char)s[i];
        uint32_t cp; int n;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 6) { cp = c & 31; n = 2; }
        else if ((c >> 4) == 14) { cp = c & 15; n = 3; }
        else if ((c >> 3) == 30) { cp = c & 7; n = 4; }
        else { cp = 0xFFFD; n = 1; }
        if (i + n > s.size()) { cp = 0xFFFD; n = 1; }
        for (int k = 1; k < n; k++) cp = (cp << 6) | ((unsigned char)s[i + k] & 63);
        i += n;
        // for (const ch of str) parcourt les caractères ; ch.charCodeAt(0) ne garde que la 1re demi-unité hors BMP
        mix(cp >= 0x10000 ? 0xD800 + ((cp - 0x10000) >> 10) : cp);
    }
    return h;
}

bool validCode(std::string_view code) {
    if (code.size() != 4) return false;
    for (char c : code) if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return false;
    return true;
}

std::string makeCode(uint32_t (*rnd)()) {
    const std::string_view chars = CODE_CHARS;
    std::string c;
    for (int i = 0; i < 4; i++) c += chars[rnd() % chars.size()];
    return c;
}

// cleanName : caractères de contrôle retirés, espaces de bord retirés, 14 caractères au plus (UTF-8 non coupé)
std::string cleanName(std::string_view n) {
    std::string s;
    for (unsigned char c : n) if (c >= 0x20 && c != 0x7f) s += (char)c;
    const size_t a = s.find_first_not_of(' '), b = s.find_last_not_of(' ');
    s = a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
    std::string out; size_t chars = 0;
    for (size_t i = 0; i < s.size() && chars < NICK_MAX; chars++) {
        const unsigned char c = (unsigned char)s[i];
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + n > s.size()) break;
        out.append(s, i, n); i += n;
    }
    return out;
}

// ---------- encodage des messages ----------
static bool keyOk(std::string_view k) {
    if (k.empty() || k.size() > 8) return false;
    for (char c : k) if (c < 'a' || c > 'z') return false;
    return true;
}
std::string encode(const Fields& f) {
    std::string out;
    for (const auto& [k, v] : f) {
        if (!keyOk(k)) continue;
        if (!out.empty()) out += ';';
        out += k; out += '=';
        for (unsigned char c : v) {
            if (c == ';' || c == '=' || c == '%' || c < 0x20) { char b[4]; std::snprintf(b, sizeof b, "%%%02X", c); out += b; }
            else out += (char)c;
        }
    }
    return out;
}
bool decode(std::string_view msg, Fields& out) {
    out.clear();
    if (msg.size() > MSG_MAX) return false;
    auto hex = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
    size_t i = 0;
    while (i < msg.size()) {
        size_t e = msg.find(';', i); if (e == std::string_view::npos) e = msg.size();
        const std::string_view kv = msg.substr(i, e - i);
        const size_t eq = kv.find('=');
        if (eq == std::string_view::npos || !keyOk(kv.substr(0, eq)) || out.size() >= 32) { out.clear(); return false; }
        std::string v;
        const std::string_view raw = kv.substr(eq + 1);
        for (size_t k = 0; k < raw.size(); k++) {
            if (raw[k] == '%') {
                if (k + 2 >= raw.size()) { out.clear(); return false; }
                const int a = hex(raw[k + 1]), b = hex(raw[k + 2]);
                if (a < 0 || b < 0) { out.clear(); return false; }
                v += (char)(a * 16 + b); k += 2;
            } else v += raw[k];
        }
        out[std::string(kv.substr(0, eq))] = v;
        i = e + 1;
    }
    return true;
}

// num du jeu web : nombre fini, sinon la valeur par défaut
double num(const Fields& f, const char* k, double def) {
    const auto it = f.find(k);
    if (it == f.end() || it->second.empty() || it->second.size() > 24) return def;
    char* end = nullptr;
    const double v = std::strtod(it->second.c_str(), &end);
    return (end && *end == 0 && std::isfinite(v)) ? v : def;
}
std::string str(const Fields& f, const char* k) { const auto it = f.find(k); return it == f.end() ? std::string() : it->second; }
std::string peerName(const Fields& f) { const std::string n = cleanName(str(f, "n")); return n.empty() ? "Pilote" : n; }
const char* peerCar(const Fields& f) {
    const std::string c = str(f, "car");
    for (const CarDef& d : CARS) if (c == d.id && !voitureRetiree(d.id)) return d.id;   // WFS-404 : voiture retirée → golf
    return "golf";
}

// ---------- transport en mémoire ----------
void SalonMemoire::injecter(const std::string& id, const std::string& msgBrut) { joueurs[id] = {msgBrut, now, true}; }

TransportMemoire::TransportMemoire(std::shared_ptr<SalonMemoire> salon, std::string id) : salon_(std::move(salon)), id_(std::move(id)) {}
void TransportMemoire::presence(const Fields& f) {
    for (const auto& [k, v] : f) mine_[k] = v;
    salon_->joueurs[id_] = {encode(mine_), salon_->now, true};
}
std::vector<PeerView> TransportMemoire::peers() const {
    std::vector<PeerView> out;
    for (const auto& [id, e] : salon_->joueurs) {
        if (id == id_ || !e.present) continue;
        PeerView p; p.id = id; p.updatedAt = e.updatedAt;
        if (!decode(e.msg, p.f)) continue;   // message invalide : joueur ignoré
        out.push_back(std::move(p));
    }
    return out;
}
void TransportMemoire::leave() { salon_->joueurs.erase(id_); }

// ---------- session ----------
const char* statusText(Status s) {
    switch (s) { case Status::Ready: return "ready"; case Status::Race: return "race"; case Status::Done: return "done"; default: return "lobby"; }
}
static std::string fmt(double v, int dec) { char b[32]; std::snprintf(b, sizeof b, "%.*f", dec, v); return b; }

std::vector<PeerView> Session::others() const { return t_ ? t_->peers() : std::vector<PeerView>{}; }

bool Session::join(std::unique_ptr<Transport> t, std::string_view code, bool host, std::string_view nick,
                   const char* car, const char* paint, std::string_view map) {
    std::string c(code);
    for (char& ch : c) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
    if (!t || !validCode(c)) return false;
    leave();
    t_ = std::move(t); code_ = c; host_ = host; round_ = 0; st_ = Status::Lobby; ghosts_.clear();
    nick_ = cleanName(nick); if (nick_.empty()) nick_ = "Pilote";
    car_ = car ? car : "golf"; paint_ = paint ? paint : ""; map_ = map;
    jeton_ = std::random_device{}() & 0x7fffffffu;   // WFS-242 (RG-05) : ordre des places de la grille (une fois par salon)
    t_->presence({{"n", nick_}, {"st", "lobby"}, {"r", "0"}, {"d", "0"}, {"x", "0"}, {"v", "0"}, {"car", car_}, {"paint", paint_},
                  {"host", host ? "1" : "0"}, {"map", host ? map_ : ""}, {"km", host ? std::to_string(km_) : ""},
                  {"trace", trace::signature()},   // WFS-136 (RG-07, A7.5) : cartes au tracé 2
                  {"j", std::to_string(jeton_)}});
    return true;
}
void Session::leave() {
    if (t_) t_->leave();
    t_.reset(); code_.clear(); st_ = Status::Lobby; ghosts_.clear(); partants_.clear();
}
void Session::setKm(int km) {
    if (st_ == Status::Race || (t_ && !host_)) return;
    if (std::find(std::begin(KMS), std::end(KMS), km) == std::end(KMS)) return;
    km_ = km;
    if (t_) t_->presence({{"km", std::to_string(km)}});
}
// followHostMap : la distance (et la carte) sont celles du créateur du salon
void Session::followHost() {
    if (!t_ || host_ || st_ == Status::Race) return;
    for (const PeerView& p : others()) {
        if (str(p.f, "host") != "1") continue;
        const int km = (int)num(p.f, "km", 0);
        if (std::find(std::begin(KMS), std::end(KMS), km) != std::end(KMS)) km_ = km;
        const std::string m = str(p.f, "map");
        if (!m.empty() && m.size() <= 16) map_ = m;
    }
}
void Session::setReady(bool on) {
    if (!t_ || st_ == Status::Race) return;
    st_ = on ? Status::Ready : Status::Lobby;
    if (on) for (const PeerView& p : others()) round_ = std::max(round_, (int)clampd(num(p.f, "r"), 0, 1e6));
    t_->presence({{"st", statusText(st_)}, {"r", std::to_string(round_)}, {"n", nick_}, {"car", car_}, {"paint", paint_}});
}
// onRoomPeers + checkStart : on rattrape la manche des autres, puis départ quand tous sont prêts (ou déjà partis)
bool Session::checkStart() {
    if (!t_) return false;
    followHost();
    if (st_ != Status::Ready) return false;
    const auto os = others();
    if (os.empty()) return false;
    // WFS-136 (RG-07) : adversaire sur un autre tracé (ou ancienne version sans champ « trace ») : pas de course
    for (const PeerView& p : os) if (str(p.f, "trace") != trace::signature()) return false;
    int mx = round_;
    for (const PeerView& p : os) if (str(p.f, "st") == "ready") mx = std::max(mx, (int)clampd(num(p.f, "r"), 0, 1e6));
    if (mx > round_) { round_ = mx; t_->presence({{"r", std::to_string(round_)}}); }
    for (const PeerView& p : os) {
        const std::string st = str(p.f, "st"); const int r = (int)num(p.f, "r", -1);
        if (!((st == "ready" && r == round_) || (st == "race" && r == round_ + 1))) return false;
    }
    startRace();
    return true;
}
// startDuelRace : manche suivante, graine commune, horloge commune (départ = maintenant, compte à rebours de 3 s)
void Session::startRace() {
    round_ += 1; st_ = Status::Race; myFinish_ = 0; presenceTimer_ = 0; ghosts_.clear();
    partants_.clear();   // WFS-122 (RG-02) : qui prend le départ (un partant qui quitte en course reste au classement : « parti »)
    for (const PeerView& p : others()) partants_[p.id] = {peerName(p.f), 0};
    // WFS-242 (RG-05) : place sur la grille : créateur devant, puis par jeton (et pseudo) croissant ; chaque téléphone ne place
    // que sa voiture, les autres la voient à sa place par la présence (d, x)
    place_ = 0;
    if (!host_)
        for (const PeerView& p : others()) {
            if (str(p.f, "host") == "1") { place_++; continue; }
            const uint32_t j = (uint32_t)clampd(num(p.f, "j", 0), 0, 2147483647.0);
            place_ += j < jeton_ || (j == jeton_ && peerName(p.f) < nick_);
        }
    startMs_ = t_->nowMs();
    const course5::Place pg = course5::placeGrille(place_);   // WFS-242 : les autres me voient tout de suite à ma place (pas de glissement)
    t_->presence({{"st", "race"}, {"r", std::to_string(round_)}, {"d", std::to_string((long long)std::llround(pg.s))},
                  {"x", fmt(std::round(pg.x * 1000) / 1000, 3)}, {"v", "0"}, {"ft", ""}, {"car", car_}, {"paint", paint_}});
}
double Session::trafficClock() const { return t_ ? (t_->nowMs() - startMs_) / 1000.0 - 3.0 : -3.0; }

// sendRacePresence + finishDuel (fin de update() en mode duel)
bool Session::raceTick(double dt, const Player& pl, double raceTime) {
    if (!t_ || st_ != Status::Race) return false;
    if (pl.dist >= finish()) {
        myFinish_ = raceTime; st_ = Status::Done;
        t_->presence({{"st", "done"}, {"ft", std::to_string((long long)std::llround(myFinish_ * 1000))},
                      {"d", std::to_string((long long)std::llround(finish()))}, {"v", "0"}});
        return true;
    }
    presenceTimer_ -= dt;
    if (presenceTimer_ <= 0) {
        presenceTimer_ = PRESENCE_DT;
        t_->presence({{"d", std::to_string((long long)std::llround(std::min(pl.dist, finish())))},
                      {"x", fmt(std::round(pl.playerX * 1000) / 1000, 3)},
                      {"v", std::to_string((long long)std::llround(pl.speed))},
                      {"h", fmt(std::round((pl.heading + pl.spinYaw) * 1000) / 1000, 3)},
                      {"b", pl.braking ? "1" : pl.reversing ? "2" : "0"}});
    }
    return false;
}

// updateGhosts : position estimée d + v × min(0,5 s, âge), lissage, téléportation au-delà de 4000 unités, bornes
void Session::updateGhosts(double dt) {
    if (!t_ || (st_ != Status::Race && st_ != Status::Done)) { ghosts_.clear(); return; }
    const double now = t_->nowMs(), fin = finish();
    std::map<std::string, Ghost> next;
    for (const PeerView& p : others()) {
        const std::string st = str(p.f, "st");
        if ((st != "race" && st != "done") || (int)num(p.f, "r", -1) != round_) continue;
        const double v = st == "race" ? clampd(num(p.f, "v"), 0, MAX_SPEED * 1.6) : 0;
        // WFS-242 (RG-05) : distance négative permise jusqu'à D_GRILLE (grille de départ derrière la ligne)
        const double est = clampd(num(p.f, "d"), D_GRILLE, fin) + v * std::min(0.5, std::max(0.0, now - p.updatedAt) / 1000);
        const double x = clampd(num(p.f, "x"), -2, 2);
        auto it = ghosts_.find(p.id);
        Ghost g; if (it != ghosts_.end()) g = it->second; else { g.rd = est; g.x = x; }
        g.noterPas();   // WFS-221 (accroche)
        g.rd = std::abs(est - g.rd) > 4000 ? est : g.rd + (est - g.rd) * std::min(1.0, dt * 8);
        g.x += (x - g.x) * std::min(1.0, dt * 10);
        g.h = clampd(num(p.f, "h"), -3.2, 3.2); g.b = (int)clampd(num(p.f, "b"), 0, 2); g.v = v;
        g.name = peerName(p.f); g.car = peerCar(p.f);
        g.paint = cleanName(str(p.f, "paint"));
        g.done = st == "done"; g.ft = clampd(num(p.f, "ft", 0), 0, 9e9);
        next[p.id] = g;
        if (auto pt = partants_.find(p.id); pt != partants_.end()) pt->second.d = g.done ? fin : g.rd;   // dernière distance vue
    }
    ghosts_.swap(next);
}

// hitGhosts (js/game/update.js) : côte à côte = frottement et séparation progressive ; arrière = recul ou poussée,
// selon la masse relative (kg / 1450). Le hasard du jeu web (cap ±0,06 quand on est percuté) est remplacé par une
// valeur déterministe tirée de l'état (même effet moyen, rejouable dans les tests).
// hitGhosts (une voiture) : contact entre ma voiture et une autre voiture (fantôme du duel ou ordinateur de la course à 5,
// WFS-110 RG-05) ; effet seulement sur ma voiture, la plus lourde bouge moins
// WFS-461 (RG-01, ANO-261) : contact réaliste. L'effort entre deux voitures passe par leurs pneus : une voiture n'est poussée de
// côté que si la poussée dépasse l'adhérence de ses pneus (μ m g) ; la poussée d'une autre voiture est elle-même bornée par
// l'adhérence de SES pneus (μ m' g) : à masses voisines, rien ne glisse. Avant (jeu web) : côte à côte, ma voiture était
// déplacée de 2 fois le recouvrement + 3 mm à chaque pas (jusqu'à 22 m/s de côté) et tournée de 0,06 rad par pas tant que
// le contact durait (7 rad/s à 120 pas par seconde) : la voiture « glissait sur la glace » contre la police (vidéo 12:04).
// Sources : μ pneu-route sèche ≈ 0,9-1,0 ; frottement tôle / plastique sur tôle ≈ 0,3-0,5 (on prend 0,4) ; coefficient de
// restitution d'un choc de voitures à basse vitesse ≈ 0,1-0,3 (on prend 0,2) ; part motrice ≈ 55 % du poids (traction avant).
namespace {
constexpr double G = 9.81;
constexpr double MU_CARROSSERIE = 0.4;   // frottement carrosserie contre carrosserie
constexpr double RESTITUTION = 0.2;      // choc mou : les voitures se séparent à 20 % de leur vitesse d'approche
constexpr double PART_MOTRICE = 0.55;    // part du poids sur les roues motrices (poussée maxi d'une voiture qui pousse)
constexpr double DEMI_TOUR = 3.14159265358979323846;
}

ContactEvents contacter(double dt, Player& pl, const Track& t, const CarDef& me, Ghost& g) {
    ContactEvents ev;
    g.hitCd = std::max(0.0, g.hitCd - dt);
    if (g.done) return ev;
    const double myLen = me.d.L / M, myW = me.d.W / (2 * RW);
    const CarDef& oc = carById(g.car.c_str());
    const double oLen = oc.d.L / M, oW = oc.d.W / (2 * RW);
    const double dz = g.rd - pl.dist, lat = std::abs(pl.playerX - g.x);
    const double touchZ = (myLen + oLen) / 2, touchLat = myW + oW;
    if (std::abs(dz) >= touchZ || lat >= touchLat) return ev;
    const double sx = pl.playerX - g.x;
    const int side = sx > 0 ? 1 : sx < 0 ? -1 : (g.x > 0 ? -1 : 1);
    const double overlapZ = touchZ - std::abs(dz), overlapX = touchLat - lat;
    const double oMass = oc.spec.kg / 1450.0, share = oMass / (oMass + pl.perf.mass);   // la plus lourde bouge moins
    const double mu = std::max(0.05, pl.muSol()), aPneus = mu * G;                        // adhérence de mes pneus (m/s²)
    ev.side = side;
    if (overlapX * RW < overlapZ * M * 0.7) {   // côte à côte : les carrosseries frottent
        // contrainte de contact : ma voiture sort juste du recouvrement, rien de plus (pas de poussée sans effort) ; un
        // recouvrement profond (voiture posée dans l'autre) se défait progressivement, sans téléportation
        pl.playerX += side * std::min(overlapX, dt * 2.5 + 0.004);
        // le flanc de l'autre voiture bloque la rotation vers elle (choc mou : la vitesse vers elle est annulée) : un cap qui
        // pointe vers elle est ramené parallèle, 2,5 rad/s au plus (comme le long de la glissière), jamais au-delà ; sans cela, la
        // voiture qui braque contre l'autre tournait jusqu'à 50° et passait derrière elle
        if (side * pl.speed * std::sin(pl.heading) < 0) {
            const double axe = std::abs(pl.heading) > DEMI_TOUR / 2 ? (pl.heading > 0 ? DEMI_TOUR : -DEMI_TOUR) : 0.0;
            pl.heading += std::clamp(axe - pl.heading, -2.5 * dt, 2.5 * dt);
        }
        // frottement des carrosseries : effort normal borné par l'adhérence des pneus, donc ≤ μc × μ g ; il rapproche ma vitesse
        // de celle de l'autre voiture (jamais au-delà : un contact ne fait pas passer la marche arrière)
        const double dv = (g.v - pl.speed) * M, aFrot = MU_CARROSSERIE * aPneus;
        pl.speed += std::clamp(dv, -aFrot * dt, aFrot * dt) / M;
        if (g.hitCd <= 0) { g.hitCd = 0.2; ev.frottements++; }
        return ev;
    }
    const double rel = std::abs(pl.speed - g.v), impact = rel / MAX_SPEED;
    const bool nouveau = g.hitCd <= 0;   // premier pas du choc (les pas suivants : contact tenu)
    if (dz > 0) {   // je percute l'arrière de l'adversaire : je reviens au contact et perds ma vitesse (choc mou)
        pl.position = t.wrap(pl.position - overlapZ - 0.01 / M); pl.dist = std::max(0.0, pl.dist - overlapZ - 0.01 / M);
        if (pl.speed > g.v) pl.speed -= (1 + RESTITUTION) * share * (pl.speed - g.v);
    } else if (g.v > pl.speed) {   // il me percute par l'arrière : quantité de mouvement échangée (choc), puis poussée tenue
        if (nouveau) pl.speed += (1 + RESTITUTION) * share * (g.v - pl.speed);
        else {
            // poussée tenue : la force de ses roues motrices (≤ μ g × part motrice × sa masse) moins mon freinage (≤ μ g × ma
            // masse) ou ma marche arrière (≤ μ g × part motrice × ma masse), sur les deux masses ; une voiture qui freine ne se
            // laisse pas pousser par une voiture de même poids
            const double retenue = pl.braking ? pl.perf.mass : pl.reversing ? PART_MOTRICE * pl.perf.mass : 0.0;
            const double f = aPneus * (PART_MOTRICE * oMass - retenue);
            pl.speed += std::min(g.v - pl.speed, std::max(0.0, f / (oMass + pl.perf.mass)) * dt / M);
        }
        // un choc décentré fait tourner un peu la voiture : une fois par vrai choc (vitesse d'approche > 3 m/s, 11 km/h), jamais pendant une
        // poussée tenue (avant : à chaque pas du contact, cap tiré au hasard 120 fois par seconde : la voiture « dansait »)
        if (nouveau && rel * M > 3.0) {
            const double r = std::sin(pl.dist * 12.9898 + g.rd * 78.233) * 0.5;   // ∈ [−0,5 ; 0,5] (Math.random() − 0,5)
            pl.heading += r * 0.12 * std::min(1.0, impact * 4);
        }
    }
    if (nouveau) {
        g.hitCd = 0.35; ev.chocs++; ev.impact = std::max(ev.impact, impact);
        if (impact * share > 0.22) { ev.accident = true; pl.accident(std::min(1.0, impact * share * 1.5), side); }
    }
    return ev;
}

ContactEvents Session::hitGhosts(double dt, Player& pl, const Track& t, const CarDef& me) {
    ContactEvents ev;
    if (st_ != Status::Race) return ev;
    for (auto& [id, g] : ghosts_) {
        const ContactEvents e = contacter(dt, pl, t, me, g);
        ev.frottements += e.frottements; ev.chocs += e.chocs; ev.impact = std::max(ev.impact, e.impact);
        ev.accident = ev.accident || e.accident; if (e.side) ev.side = e.side;
    }
    return ev;
}

// renderBoard : moi + les adversaires, triés par distance ; un adversaire arrivé compte pour FINISH
std::vector<BoardRow> Session::board(double myDist) const {
    std::vector<BoardRow> rows{{nick_ + " (toi)", myDist, true, st_ == Status::Done, myFinish_ * 1000}};
    for (const auto& [id, g] : ghosts_) rows.push_back({g.name, g.done ? finish() : g.rd, false, g.done, g.ft});
    // arrivés d'abord, dans l'ordre d'arrivée (temps), puis ceux qui roulent, par distance
    std::stable_sort(rows.begin(), rows.end(), [](const BoardRow& a, const BoardRow& b) {
        if (a.done != b.done) return a.done;
        return a.done ? a.ft < b.ft : a.d > b.d;
    });
    return rows;
}
int Session::position(double myDist) const {
    const auto rows = board(myDist);
    for (size_t i = 0; i < rows.size(); i++) if (rows[i].me) return (int)i + 1;
    return 1;
}
// renderDuelResult : arrivés d'abord (par temps), puis ceux qui roulent (par distance)
std::vector<BoardRow> Session::results() const {
    std::vector<BoardRow> rows{{nick_ + " (toi)", finish(), true, true, myFinish_ * 1000}};
    std::map<std::string, bool> vus;
    for (const PeerView& p : others()) {
        const std::string st = str(p.f, "st");
        if ((int)num(p.f, "r", -1) != round_) continue;
        vus[p.id] = true;
        if (st == "race") rows.push_back({peerName(p.f), clampd(num(p.f, "d"), 0, finish()), false, false, 0});
        else rows.push_back({peerName(p.f), finish(), false, true, clampd(num(p.f, "ft", 9e9), 0, 9e9)});
    }
    // WFS-122 (RG-02) : partant qui a quitté le salon pendant la manche : « parti », à sa dernière distance vue
    for (const auto& [id, pt] : partants_) if (!vus.count(id)) rows.push_back({pt.name, pt.d, false, false, 0, true});
    std::stable_sort(rows.begin(), rows.end(), [](const BoardRow& a, const BoardRow& b) {
        if (a.done != b.done) return a.done;
        if (a.parti != b.parti) return b.parti;   // ceux qui roulent avant ceux qui sont partis
        return a.done ? a.ft < b.ft : a.d > b.d;
    });
    return rows;
}
// WFS-183 (RG-01, ANO-93) : dans l'ordre du classement de la manche (mieux classé d'abord, « Parti » en dernier),
// jamais dans l'ordre de l'identifiant du relais ; seuls les partants de la manche comptent
std::vector<std::string> Session::adversaires() const {
    std::vector<std::string> n;
    std::map<std::string, bool> partant;
    for (const auto& [id, pt] : partants_) partant[pt.name] = true;
    for (const BoardRow& row : results()) if (!row.me && partant.count(row.name)) n.push_back(row.name);
    return n;
}

} // namespace wfs::duel
