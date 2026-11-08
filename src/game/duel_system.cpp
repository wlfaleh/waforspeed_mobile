// WaForSpeed — module « duel » (WFS-21) : voir duel_system.h.
#include "duel_system.h"
#include "../core/memoire.h"   // WFS-297 (RG-06)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "course_system.h"
#include "course5.h"   // WFS-112 : ouvrirChoixMode (« Retour » de la fin du duel) ; WFS-189 : rechargerNitro
#include "commandes_tactiles.h"   // WFS-146 (accroche, ANO-35) : dessin2d::biseau des boutons du résultat
#include "game.h"
#include "player.h"
#include "sauvegarde.h"   // WFS-122 (RG-01) : derniers adversaires
#include "traffic_system.h"
#include "../world/conducteurs.h"   // WFS-73 (accroche)

namespace wfs {

// couleurs du skill design-street-racing (tokens.md), codes sRGB comme ui::CYAN
static const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f};      // --bg #0A0B10
static const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};   // --surface #14161F
static const vec4 ETIQUETTE{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.78f}; // nameSprite : rgba(10,11,16,.78)
static const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1};      // --text #F2F4F8
static const vec4 PISTE{1, 1, 1, 0.12f};                                  // fond des barres du classement

static duel::Session g_session;
duel::Session& duelSession() { return g_session; }

// banc d'essai : salon en mémoire et adversaire simulé (rejoue des messages de présence)
struct BancDuel {
    std::shared_ptr<duel::SalonMemoire> salon;
    duel::Session rival; Player rivalPl;
    double avance = 18 / M, lat = -0.45;   // l'adversaire roule 18 m devant, file de gauche
    bool arrivee = false;                  // fin forcée : le joueur passe la ligne
    double rivalFt = 0;                    // temps d'arrivée de l'adversaire (s), 0 = il roule encore
};
static BancDuel* g_banc = nullptr;
static bool g_arriveeForcee = false;   // WFS-112 : banc du duel entre amis (le joueur passe la ligne tout de suite)
static float g_zones[4] = {0, 0, 0, 0}, g_zonesMenu[4] = {0, 0, 0, 0};   // « Revanche », « Retour » (banc d'essai)

static vec3 hexLin(unsigned h) {
    auto lin = [](float c) { return std::pow(c, 2.2f); };
    return {lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
}

class DuelSystem : public System {
public:
    const char* name() const override { return "duel"; }

    void init(Renderer& r, const GameView&) override { r_ = &r; overT_ = 0; }
    void shutdown(Renderer& r) override {
        for (auto& [id, m] : models_) r.destroy(m);
        models_.clear();
    }

    // modèles des adversaires chargés hors de l'image (pas de chargement entre beginFrame et endFrame)
    void update(double dt, const GameView&) override {
        if (g_banc) g_banc->salon->now += dt * 1000;
        if (!r_) return;
        for (const auto& [id, gh] : g_session.ghosts()) {
            if (models_.count(gh.car)) continue;
            Model m; loadCarModel(*r_, std::string(carById(gh.car.c_str()).file), m);
            models_[gh.car] = std::move(m);
        }
    }

    // après la course (ordre 220 > 210) : départ de la manche, horloge commune, contacts, présence, arrivée
    void collide(double dt, Player& pl, const GameView& g) override {
        duel::Session& s = g_session;
        if (g_banc) simulerRival(dt, pl);
        if (!s.inRoom() || g.ecran != Ecran::Course) return;
        Course* c = activeCourseMut();
        if (s.status() == duel::Status::Lobby || s.status() == duel::Status::Ready) {
            if (!s.checkStart()) return;
            // startDuelRace : resetRace('play', hashStr(code) + round × 977), compte à rebours de 3 s
            pl.reset(*pl.car);
            // WFS-242 (RG-05) : chaque joueur à sa place de la grille, derrière la ligne (avant : tous sur la ligne, l'un dans l'autre)
            course5::placerSurGrille(pl, *g.track, course5::placeGrille(s.place()));
            WFS_LOG("duel : place %d sur la grille", s.place() + 1);
            trafficRestartSeed(s.seed(), pl.position);
            if (c) { c->reset(); c->duelFinish = s.finish(); }
            memoire::nouvellePartie();   // WFS-297 (RG-06, accroche) : chaque manche est une partie
            overT_ = 0;
            // WFS-122 (RG-01) : adversaires de la manche en tête des « Derniers adversaires » du salon
            sauvegarde().noterAdversaires(s.adversaires()); sauvegarde().enregistrer();
            WFS_LOG("duel : manche %d, salon %s, graine %u, %d km", s.round(), s.code().c_str(), s.seed(), s.km());
            return;
        }
        if (c && s.status() == duel::Status::Race) {
            c->clock = s.trafficClock();   // horloge commune (raceStartWall du jeu web)
            if ((g_banc && g_banc->arrivee) || (g_arriveeForcee && c->playing())) pl.dist = std::max(pl.dist, s.finish());
            if (c->playing()) {
                const duel::ContactEvents e = s.hitGhosts(dt, pl, *g.track, *g.car);
                if (e.accident) c->accident(); else if (e.chocs) c->bump(e.impact);
                // WFS-189 (RG-04, RG-05) : sans trafic, la nitro se recharge au temps ; pas de bonus de dépassement en Duel
                // (équité réseau) ; chaque téléphone le calcule pour son joueur, rien de plus n'est envoyé
                if (pl.dist < s.finish()) pl.nitro = course5::rechargerNitro(pl.nitro, pl.nitroOn, dt, 0);
            }
            if (s.raceTick(dt, pl, c->raceTime))
                WFS_LOG("duel : arrivée en %.2f s, position %d/%d", s.myFinish(), s.position(pl.dist), (int)s.board(pl.dist).size());
        }
        s.updateGhosts(dt);
        if (s.status() == duel::Status::Done) overT_ += dt;
        if (s.status() != duel::Status::Done) { arrives_ = -1; return; }
        // WFS-183 (RG-01, ANO-93) : à chaque arrivée, derniers adversaires repris dans l'ordre du classement de la manche
        int n = 0; for (const duel::BoardRow& row : s.results()) n += row.done || row.parti;
        if (n != arrives_) { arrives_ = n; sauvegarde().noterAdversaires(s.adversaires()); sauvegarde().enregistrer(); }
    }

    // adversaires : vraie voiture à la place reçue, cap envoyé (lissé), sur la piste (scene.js : rivaux)
    void render(Renderer& r, const GameView& g) override {
        if (g.sceneDediee || !g.player || !g.track) return;
        const double sP = g.player->dist;
        for (const auto& [id, gh] : g_session.ghosts()) {
            const double rel = gh.rd - sP;
            if (gh.done || rel < -40 / M || rel > 600 / M) continue;   // visible de 40 m derrière à 600 m devant
            auto it = models_.find(gh.car);
            if (it == models_.end() || !it->second.ok) continue;
            double& yaw = yaw_[id];
            yaw += (gh.h - yaw) * 0.15;
            const TrackPoint p = g.track->pointLisse(gh.rdRendu(g.alpha), gh.xRendu(g.alpha) * RW);   // WFS-221 (accroche)
            const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
            const CarDef& cd = carById(gh.car.c_str());
            const vec3 paint = hexLin(cd.paint);
            r.adversaireProchaineVoiture();   // WFS-243 (ANO-117, accroche) : visible jusqu'à 600 m, pas coupé à 150 / 200 m
            r.draw(it->second, m, &paint);
            conducteurs::ajouter(it->second, m, cd.id, (uint32_t)std::hash<std::string>{}(id) | 1u);   // WFS-73 (accroche) : adversaire
        }
    }

    void drawUi(Renderer& r, const GameView& g) override {
        const duel::Session& s = g_session;
        g_zones[2] = g_zonesMenu[2] = 0;   // posées de nouveau par resultat() s'il est dessiné
        if (!ui::fontReady() || g.ecran != Ecran::Course || !s.inRoom() || !g.player) return;
        // WFS-112 : après « Revanche » (prêt), l'écran de fin reste affiché (« PRÊT… ») jusqu'au départ de la manche
        const bool attente = s.status() == duel::Status::Ready && overT_ >= 0.9;
        if (s.status() != duel::Status::Race && s.status() != duel::Status::Done && !attente) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        noms(r, g, u);
        board(r, g, u);
        if ((s.status() == duel::Status::Done || attente) && overT_ >= 0.9) resultat(r, W, H, u);
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        duel::Session& s = g_session;
        // WFS-112 : comme WFS-115, les boutons agissent au relâcher du doigt qui les a appuyés (un doigt resté posé
        // n'appuie pas sur l'écran suivant)
        if (!t.down && t.id == doigt_) {
            const int b = bouton_; doigt_ = -1; bouton_ = 0;
            if (g.ecran != Ecran::Course || !s.inRoom()) return true;
            if (b == 1 && revanche_.contient(t.x, t.y) && s.status() == duel::Status::Done) { WFS_LOG("duel : « Revanche »"); s.setReady(true); }   // prêt pour la manche suivante
            // RG-11 : « Retour » quitte le duel et ramène au menu des modes (jamais à l'accueil seul)
            if (b == 2 && menu_.contient(t.x, t.y)) { WFS_LOG("duel : « Retour » → menu des modes"); s.leave(); g.allerA(Ecran::Menu); ouvrirChoixMode(); }
            return true;
        }
        if (g.ecran != Ecran::Course || (s.status() != duel::Status::Done && s.status() != duel::Status::Ready) || overT_ < 0.9) return false;
        if (t.down && doigt_ < 0) {
            if (revanche_.contient(t.x, t.y)) { doigt_ = t.id; bouton_ = 1; }
            else if (menu_.contient(t.x, t.y)) { doigt_ = t.id; bouton_ = 2; }
        }
        return true;
    }

private:
    struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };

    // adversaire simulé du banc : il roule `avance` devant le joueur et envoie sa présence comme un vrai joueur
    void simulerRival(double dt, const Player& pl) {
        BancDuel& b = *g_banc;
        duel::Session& rv = b.rival;
        if (rv.status() == duel::Status::Lobby || rv.status() == duel::Status::Ready) { rv.checkStart(); return; }
        if (rv.status() != duel::Status::Race) return;
        if (rv.trafficClock() < 0) return;
        if (b.rivalFt > 0) { b.rivalPl.dist = rv.finish(); rv.raceTick(dt, b.rivalPl, b.rivalFt); return; }
        b.rivalPl.dist = pl.dist + b.avance; b.rivalPl.position = b.rivalPl.dist;
        b.rivalPl.playerX = b.lat; b.rivalPl.speed = pl.speed; b.rivalPl.heading = 0;
        rv.raceTick(dt, b.rivalPl, rv.trafficClock());
    }

    // nameSprite : étiquette sombre avec liseré cyan, 2,6 m au-dessus de la voiture adverse
    void noms(Renderer& r, const GameView& g, float u) {
        const mat4 vp = g.proj * g.view;
        for (const auto& [id, gh] : g_session.ghosts()) {
            if (gh.done) continue;
            const TrackPoint p = g.track->point(gh.rd, gh.x * RW);
            const float x = (float)p.x, y = (float)p.y + 2.6f, z = (float)p.z;
            const float cx = vp.at(0, 0) * x + vp.at(0, 1) * y + vp.at(0, 2) * z + vp.at(0, 3);
            const float cy = vp.at(1, 0) * x + vp.at(1, 1) * y + vp.at(1, 2) * z + vp.at(1, 3);
            const float cw = vp.at(3, 0) * x + vp.at(3, 1) * y + vp.at(3, 2) * z + vp.at(3, 3);
            if (cw < 0.5f) continue;   // derrière la caméra ou trop près
            const float sx = (cx / cw * 0.5f + 0.5f) * g.uiWidth, sy = (cy / cw * 0.5f + 0.5f) * g.uiHeight;
            const float px = std::clamp(3.2f * u * 10.f / cw, 1.6f * u, 3.2f * u);   // plus petite au loin
            const float w = std::max(ui::width(px, gh.name) + px * 1.2f, px * 4.f), h = px * 1.5f;
            r.uiRect(sx - w / 2, sy - h / 2, w, h, ETIQUETTE);
            r.uiRect(sx - w / 2, sy - h / 2, px * 0.16f, h, ui::CYAN);
            ui::text(r, sx + px * 0.08f, sy - px * 0.5f, px, gh.name, TEXTE, ui::CENTRE, 0.f, false);
        }
    }

    // renderBoard + « Position n/N » : à gauche, sous le haut de l'écran (zone sûre)
    void board(Renderer& r, const GameView& g, float u) {
        const duel::Session& s = g_session;
        const auto rows = s.board(g.player->dist);
        const float x = g.marges.gauche + 2.5f * u, w = 34 * u, rh = 4.6f * u, px = 2.7f * u;
        float y = g.marges.haut + 33 * u;   // sous le bouton « Menu » (WFS-39) et le bouton de vue (WFS-66)
        char pos[48]; std::snprintf(pos, sizeof pos, "POSITION %d/%d", s.position(g.player->dist), (int)rows.size());
        // WFS-112 (RG-05) : ping du relais, reconnexion en cours
        const duel::Transport* tr = s.transport();
        if (tr && tr->lien() == duel::Lien::Reconnexion) std::snprintf(pos, sizeof pos, "POSITION %d/%d · RECONNEXION…", s.position(g.player->dist), (int)rows.size());
        else if (tr && tr->pingMs() >= 0) std::snprintf(pos, sizeof pos, "POSITION %d/%d · %d MS", s.position(g.player->dist), (int)rows.size(), tr->pingMs());
        r.uiRect(x, y, w, rh * (rows.size() + 1) + 1.2f * u, SURFACE);
        ui::text(r, x + 1.2f * u, y + 0.9f * u, px, pos, ui::CYAN, ui::GAUCHE, 0.12f, false, true);
        y += rh;
        for (const auto& row : rows) {
            const float k = (float)std::clamp(row.d / s.finish(), 0.0, 1.0);
            const std::string km = row.d >= s.finish() ? "Arrivé" : ui::decimal(std::clamp(row.d, 0.0, s.finish()) * 0.006 / 1000, 2) + " km";   // WFS-242 : grille derrière la ligne : 0,00
            ui::text(r, x + 1.2f * u, y + 0.6f * u, px, row.name, row.me ? ui::ORANGE : TEXTE, ui::GAUCHE, 0.f, false);
            ui::text(r, x + w - 1.2f * u, y + 0.6f * u, px, km, TEXTE, ui::DROITE, 0.f, false);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, w - 2.4f * u, 0.5f * u, PISTE);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, (w - 2.4f * u) * k, 0.5f * u, row.me ? ui::ORANGE : ui::CYAN);
            y += rh;
        }
    }

    // renderDuelResult : « Victoire » / « ne place », temps, classement, « Revanche » et « Menu »
    void resultat(Renderer& r, float W, float H, float u) {
        const duel::Session& s = g_session;
        const auto rows = s.results();
        int rank = 1; bool roule = false; size_t presents = 0;   // WFS-122 : les adversaires partis ne roulent plus
        for (size_t i = 0; i < rows.size(); i++) { if (rows[i].me) rank = (int)i + 1; if (!rows[i].done && !rows[i].parti) roule = true; if (!rows[i].parti) presents++; }
        r.uiRect(0, 0, W, H, FOND);
        const float pw = 64 * u, ph = (34 + 6.5f * rows.size()) * u, px0 = (W - pw) / 2, py0 = (H - ph) / 2;
        r.uiRect(px0, py0, pw, ph, SURFACE);
        const std::string sur = presents == 1 ? (rows.size() > 2 ? "TES ADVERSAIRES ONT QUITTÉ LE DUEL" : "TON ADVERSAIRE A QUITTÉ LE DUEL")
                              : roule ? (presents > 2 ? "ARRIVÉ · LA COURSE CONTINUE" : "ARRIVÉ · TON ADVERSAIRE ROULE ENCORE") : "DUEL TERMINÉ";
        ui::text(r, W / 2, py0 + 3 * u, 2.6f * u, sur, ui::CYAN, ui::CENTRE, 0.16f, false);
        const std::string titre = presents == 1 ? "ARRIVÉ" : rank == 1 ? "VICTOIRE" : std::to_string(rank) + "e PLACE";   // WFS-146 (RG-06) : « 2e PLACE », jamais « 2E »
        ui::text(r, W / 2, py0 + 7 * u, 9 * u, titre, rank == 1 ? ui::ORANGE : TEXTE, ui::CENTRE, 0.04f, false, true);
        ui::text(r, W / 2, py0 + 17.5f * u, 4.2f * u, ui::chrono(s.myFinish()), TEXTE, ui::CENTRE, 0.f, false, true);
        float y = py0 + 24 * u;
        const double lead = rows[0].done ? rows[0].ft : -1;
        for (size_t i = 0; i < rows.size(); i++) {
            const auto& row = rows[i];
            std::string tm;
            if (row.parti) tm = "Parti";   // WFS-122 (RG-02) : a quitté le salon pendant la manche
            else if (!row.done) tm = ui::decimal(std::max(0.0, row.d) * 0.006 / 1000, 2) + " km";   // WFS-242
            else if (row.ft >= 9e9) tm = "Arrivé";
            else if (i == 0 || lead < 0) tm = ui::chrono(row.ft / 1000);
            else tm = "+" + ui::decimal((row.ft - lead) / 1000, 1) + " s";
            const vec4 c = row.me ? ui::ORANGE : TEXTE;
            ui::text(r, px0 + 5 * u, y, 3.4f * u, std::to_string(i + 1) + ". " + row.name, c, ui::GAUCHE, 0.f, false);
            ui::text(r, px0 + pw - 5 * u, y, 3.4f * u, tm, c, ui::DROITE, 0.f, false);
            y += 6.5f * u;
        }
        // boutons : la paire est centrée (WFS-57) ; zones de toucher = rectangles dessinés
        const float bw = 24 * u, bh = 7 * u, gap = 3 * u, by = py0 + ph - bh - 3 * u, bx = (W - (2 * bw + gap)) / 2;
        revanche_ = {bx, by, bw, bh}; menu_ = {bx + bw + gap, by, bw, bh};
        g_zones[0] = bx; g_zones[1] = by; g_zones[2] = bw; g_zones[3] = bh;
        g_zonesMenu[0] = bx + bw + gap; g_zonesMenu[1] = by; g_zonesMenu[2] = bw; g_zonesMenu[3] = bh;
        const bool attend = s.status() == duel::Status::Ready;
        dessin2d::biseau(r, revanche_.x, revanche_.y, bw, bh, 14 * H / 720, dessin2d::uni(ui::ORANGE));   // WFS-146 (RG-06) : biseau 14 px de la fin solo
        ui::text(r, revanche_.x + bw / 2, by + 1.9f * u, 3.4f * u, attend ? "PRÊT…" : "REVANCHE", vec4{0.04f, 0.04f, 0.06f, 1}, ui::CENTRE, 0.08f, false);
        dessin2d::biseau(r, menu_.x, menu_.y, bw, bh, 14 * H / 720, dessin2d::uni({30 / 255.f, 33 / 255.f, 48 / 255.f, 1}));   // .btn--ghost (#1E2130), comme la fin solo
        ui::text(r, menu_.x + bw / 2, by + 1.9f * u, 3.4f * u, "RETOUR", TEXTE, ui::CENTRE, 0.08f, false);
    }

    Renderer* r_ = nullptr;
    std::map<std::string, Model> models_;
    std::map<std::string, double> yaw_;
    double overT_ = 0;
    int arrives_ = -1;   // WFS-183 : arrivés déjà notés dans « Derniers adversaires »
    Zone revanche_, menu_;
    int doigt_ = -1, bouton_ = 0;   // WFS-112 : bouton appuyé (1 Revanche, 2 Retour) et son doigt
};
WFS_SYSTEM(DuelSystem, 220)

// ---------- banc d'essai ----------
void duelArriveeForcee(bool oui) { g_arriveeForcee = oui; }
bool duelBouton(const char* nom, float& x, float& y) {
    const float* z = std::string(nom) == "revanche" ? g_zones : g_zonesMenu;
    if (z[2] <= 0) return false;
    x = z[0] + z[2] / 2; y = z[1] + z[3] / 2;
    return true;
}

void benchDuelScene(Game& g, Renderer& r, const std::string& out) {
    BancDuel banc; banc.salon = std::make_shared<duel::SalonMemoire>();
    g_banc = &banc;
    duel::Session& s = g_session;
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { g_banc = nullptr; return; }
    s.join(std::make_unique<duel::TransportMemoire>(banc.salon, "moi"), "K7QX", true, "Alex", "golf", "", "ville");
    s.setKm(2);
    banc.rival.join(std::make_unique<duel::TransportMemoire>(banc.salon, "rival"), "k7qx", false, "Nadia", "m4", "", "ville");
    banc.rivalPl.reset(carById("m4"));
    banc.rival.setReady(true); s.setReady(true);
    g.setAutopilot(true); g.camMode = 0;
    // départ (3 s de compte à rebours) puis 4 s de course : l'adversaire 18 m devant, dans la file de gauche
    double t = 600;
    for (int i = 0; i < 7 * 60; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(t + i / 60.0); }
    g.render(t + 7.0); g.render(t + 7.02);
    r.capturePng(out + "/duel-course.png");
    WFS_LOG("capture duel-course : manche %d, graine %u, %zu adversaire(s), position %d/%d, %.0f km/h", s.round(), s.seed(),
            s.ghosts().size(), s.position(g.player().dist), (int)s.board(g.player().dist).size(), g.player().kmh());
    // l'adversaire passe la ligne le premier, puis le joueur 0,5 s après : écran de résultat (« 2e place »)
    banc.rivalFt = std::max(0.0, s.trafficClock());
    for (int i = 0; i < 30; i++) { g.update(1.0 / 60); }
    banc.arrivee = true;
    for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t + 8 + i / 60.0); }
    g.render(t + 10); g.render(t + 10.02);
    r.capturePng(out + "/duel-resultat.png");
    WFS_LOG("capture duel-resultat : %s, arrivée en %.2f s, %zu ligne(s) au résultat", duel::statusText(s.status()), s.myFinish(), s.results().size());
    s.leave(); banc.rival.leave();
    g_banc = nullptr;
    courseBenchRules(false);
    g.shutdown();
}

} // namespace wfs
