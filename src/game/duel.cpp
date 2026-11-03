// WaForSpeed — duel en ligne (WFS-21) : voir duel.h. Les noms des fonctions du jeu web sont rappelés en commentaire.
#include "duel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "player.h"

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
    for (const CarDef& d : CARS) if (c == d.id) return d.id;
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
    t_->presence({{"n", nick_}, {"st", "lobby"}, {"r", "0"}, {"d", "0"}, {"x", "0"}, {"v", "0"}, {"car", car_}, {"paint", paint_},
                  {"host", host ? "1" : "0"}, {"map", host ? map_ : ""}, {"km", host ? std::to_string(km_) : ""}});
    return true;
}
void Session::leave() {
    if (t_) t_->leave();
    t_.reset(); code_.clear(); st_ = Status::Lobby; ghosts_.clear();
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
    startMs_ = t_->nowMs();
    t_->presence({{"st", "race"}, {"r", std::to_string(round_)}, {"d", "0"}, {"x", "0"}, {"v", "0"}, {"ft", ""}, {"car", car_}, {"paint", paint_}});
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
        const double est = clampd(num(p.f, "d"), 0, fin) + v * std::min(0.5, std::max(0.0, now - p.updatedAt) / 1000);
        const double x = clampd(num(p.f, "x"), -2, 2);
        auto it = ghosts_.find(p.id);
        Ghost g; if (it != ghosts_.end()) g = it->second; else { g.rd = est; g.x = x; }
        g.rd = std::abs(est - g.rd) > 4000 ? est : g.rd + (est - g.rd) * std::min(1.0, dt * 8);
        g.x += (x - g.x) * std::min(1.0, dt * 10);
        g.h = clampd(num(p.f, "h"), -3.2, 3.2); g.b = (int)clampd(num(p.f, "b"), 0, 2); g.v = v;
        g.name = peerName(p.f); g.car = peerCar(p.f);
        g.paint = cleanName(str(p.f, "paint"));
        g.done = st == "done"; g.ft = clampd(num(p.f, "ft", 0), 0, 9e9);
        next[p.id] = g;
    }
    ghosts_.swap(next);
}

// hitGhosts (js/game/update.js) : côte à côte = frottement et séparation progressive ; arrière = recul ou poussée,
// selon la masse relative (kg / 1450). Le hasard du jeu web (cap ±0,06 quand on est percuté) est remplacé par une
// valeur déterministe tirée de l'état (même effet moyen, rejouable dans les tests).
ContactEvents Session::hitGhosts(double dt, Player& pl, const Track& t, const CarDef& me) {
    ContactEvents ev;
    if (st_ != Status::Race) return ev;
    const double myLen = me.d.L / M, myW = me.d.W / (2 * RW);
    for (auto& [id, g] : ghosts_) {
        g.hitCd = std::max(0.0, g.hitCd - dt);
        if (g.done) continue;
        const CarDef& oc = carById(g.car.c_str());
        const double oLen = oc.d.L / M, oW = oc.d.W / (2 * RW);
        const double dz = g.rd - pl.dist, lat = std::abs(pl.playerX - g.x);
        const double touchZ = (myLen + oLen) / 2, touchLat = myW + oW;
        if (std::abs(dz) >= touchZ || lat >= touchLat) continue;
        const double sx = pl.playerX - g.x;
        const int side = sx > 0 ? 1 : sx < 0 ? -1 : (g.x > 0 ? -1 : 1);
        const double overlapZ = touchZ - std::abs(dz), overlapX = touchLat - lat;
        const double oMass = oc.spec.kg / 1450.0, share = oMass / (oMass + pl.perf.mass);   // la plus lourde bouge moins
        ev.side = side;
        if (overlapX * RW < overlapZ * M * 0.7) {   // côte à côte : les carrosseries frottent et se repoussent
            pl.playerX += side * std::min(overlapX * share * 2 + 0.003, dt * 2.5 + 0.004);
            pl.heading += side * 0.06 * share; pl.speed *= 1 - 0.8 * dt;
            if (g.hitCd <= 0) { g.hitCd = 0.2; ev.frottements++; }
            continue;
        }
        const double rel = std::abs(pl.speed - g.v), impact = rel / MAX_SPEED;
        if (dz > 0) {   // je percute l'arrière de l'adversaire : je recule derrière lui et perds ma vitesse
            pl.position = t.wrap(pl.position - overlapZ - 0.15 / M); pl.dist = std::max(0.0, pl.dist - overlapZ - 0.15 / M);
            pl.speed = std::min(pl.speed, g.v + rel * (0.15 + 0.3 * (1 - share)));
        } else {        // il me percute par l'arrière : je suis poussé vers l'avant
            pl.speed = std::max(pl.speed, g.v * (0.6 + 0.4 * share));
            const double r = std::sin(pl.dist * 12.9898 + g.rd * 78.233) * 0.5;   // ∈ [−0,5 ; 0,5] (Math.random() − 0,5)
            pl.heading += r * 0.12 * std::min(1.0, impact * 4);
        }
        if (g.hitCd <= 0) {
            g.hitCd = 0.35; ev.chocs++; ev.impact = std::max(ev.impact, impact);
            if (impact * share > 0.22) { ev.accident = true; pl.accident(std::min(1.0, impact * share * 1.5), side); }
        }
    }
    return ev;
}

// renderBoard : moi + les adversaires, triés par distance ; un adversaire arrivé compte pour FINISH
std::vector<BoardRow> Session::board(double myDist) const {
    std::vector<BoardRow> rows{{nick_ + " (toi)", myDist, true, st_ == Status::Done, myFinish_ * 1000}};
    for (const auto& [id, g] : ghosts_) rows.push_back({g.name, g.done ? finish() : g.rd, false, g.done, g.ft});
    std::stable_sort(rows.begin(), rows.end(), [](const BoardRow& a, const BoardRow& b) { return a.d > b.d; });
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
    for (const PeerView& p : others()) {
        const std::string st = str(p.f, "st");
        if ((int)num(p.f, "r", -1) != round_) continue;
        if (st == "race") rows.push_back({peerName(p.f), clampd(num(p.f, "d"), 0, finish()), false, false, 0});
        else rows.push_back({peerName(p.f), finish(), false, true, clampd(num(p.f, "ft", 9e9), 0, 9e9)});
    }
    std::stable_sort(rows.begin(), rows.end(), [](const BoardRow& a, const BoardRow& b) {
        if (a.done != b.done) return a.done;
        return a.done ? a.ft < b.ft : a.d > b.d;
    });
    return rows;
}

} // namespace wfs::duel
