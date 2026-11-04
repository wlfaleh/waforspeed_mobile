// WaForSpeed — écran Salon du duel en ligne (WFS-83) : voir salon.h.
#include "salon.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <random>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course_system.h"
#include "../../game/duel_system.h"
#include "../../game/game.h"
#include "../../game/player.h"
#include "../../game/reseau_udp.h"
#include "../../game/sauvegarde.h"
#include "../../game/track.h"
#include "../../render/renderer.h"
#include "../text.h"

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
Zone g_creer, g_retour, g_salons[4], g_pret, g_quitter, g_km[4];
std::vector<reseau::SalonOuvert> g_liste;          // salons affichés à la dernière image (touchés dans le même ordre)
std::shared_ptr<reseau::Prise> g_priseEssai;      // banc d'essai : prise en boucle locale à la place de la prise commune
bool g_depart = false;

std::shared_ptr<reseau::Prise> prise() { return g_priseEssai ? g_priseEssai : reseau::priseCommune(); }

void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    const int N = 16;
    for (int i = 0; i < N; i++) {
        const float t = (i + 0.5f) / N;
        r.uiRect(x + biais * (1 - t), y + h * i / N, w - biais, h / N + 0.5f, c);
    }
}
// bouton .btn (orange) ou .btn--ghost (surface) avec son texte en capitales ; renvoie la zone touchable
Zone bouton(Renderer& r, float x, float y, float h, float k, const char* texte, bool plein, bool droite = false) {
    const float tp = 20 * k;
    const std::string t = ui::majuscules(texte);
    const float w = ui::width(tp, t, 0.12f) + 56 * k, x0 = droite ? x - w : x;
    biseau(r, x0, y, w, h, 14 * k, plein ? hex(ACCENT) : hex(SURFACE2));
    ui::text(r, x0 + w / 2, y + (h - tp) / 2, tp, t, plein ? hex(BG) : hex(TEXTE), ui::CENTRE, 0.12f, false);
    return {x0, y, w, h};
}

// carte et moment au format « carte/moment » (champ map de la présence de l'hôte)
std::string carteActuelle(const GameView& g) {
    const char* m = std::getenv("WFS_MOMENT");
    return std::string(g.map ? g.map->id : "ville") + "/" + (m && std::strcmp(m, "nuit") == 0 ? "nuit" : "jour");
}
std::string libelleCarte(const std::string& cm) {
    const size_t sl = cm.find('/');
    const std::string id = cm.substr(0, sl), mo = sl == std::string::npos ? "" : cm.substr(sl + 1);
    if (id.empty()) return "—";
    return std::string(mapById(id.c_str()).name) + (mo == "nuit" ? " · Nuit" : mo == "jour" ? " · Jour" : "");
}
std::string carteHote(const duel::Session& s, const GameView& g) {
    if (s.host()) return carteActuelle(g);
    for (const duel::PeerView& p : s.others()) if (duel::str(p.f, "host") == "1") return duel::str(p.f, "map");
    return "";
}
uint32_t hasard() {
    static std::mt19937 g(std::random_device{}() ^ (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
    return g();
}
// tous les pilotes du salon sont prêts pour la même manche (condition de checkStart, sans démarrer la manche)
bool tousPrets(const duel::Session& s) {
    if (!s.inRoom() || s.status() != duel::Status::Ready) return false;
    const auto os = s.others();
    if (os.empty()) return false;
    for (const duel::PeerView& p : os) if (duel::str(p.f, "st") != "ready") return false;
    return true;
}

class Salon : public System {
public:
    const char* name() const override { return "salon"; }

    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        g_depart = false; doigt_ = -1;
        if (nouveau == Ecran::Salon) {
            reseau::verrou(true);   // RG-09 : verrou de diffusion du Wi-Fi (Android)
            if (!duelSession().inRoom() && !g_priseEssai && !reseau::priseCommune()->reseau()) reseau::priseCommune(true);   // Wi-Fi revenu ?
            WFS_LOG("salon : %s", duelSession().inRoom() ? "dans le salon" : "accueil");
        }
        if (nouveau == Ecran::Menu && !duelSession().inRoom()) reseau::verrou(false);   // on a quitté le duel
    }

    void update(double, const GameView& g) override {
        if (g.ecran != Ecran::Salon) return;
        duel::Session& s = duelSession();
        if (!s.inRoom()) { prise()->pomper(); return; }
        // RG-04 : les deux prêts → course ; carte et moment de l'hôte (écran de chargement de WFS-74 s'ils ne sont pas chargés)
        if (!g_depart && tousPrets(s)) {
            g_depart = true;
            const std::string cm = s.host() ? carteActuelle(g) : carteHote(s, g);
            const size_t sl = cm.find('/');
            Game::Demandes& d = Game::demandes();
            d.carte = sl == std::string::npos || sl == 0 ? std::string(g.map ? g.map->id : "ville") : cm.substr(0, sl);
            d.nuit = sl != std::string::npos && cm.substr(sl + 1) == "nuit";
            WFS_LOG("salon : tous prêts, départ sur %s (%s)", d.carte.c_str(), d.nuit ? "nuit" : "jour");
        }
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Salon) return false;
        if (!t.down) { if (t.id == doigt_) doigt_ = -1; return true; }
        if (doigt_ >= 0) return true;
        doigt_ = t.id;
        duel::Session& s = duelSession();
        Sauvegarde& sv = sauvegarde();
        const char* car = g.car ? g.car->id : "golf";
        if (!s.inRoom()) {
            if (g_retour.contient(t.x, t.y)) { WFS_LOG("salon : « Retour »"); g.allerA(Ecran::Menu); return true; }
            if (g_creer.contient(t.x, t.y)) {   // createRoom : code de 4 caractères, l'hôte choisit carte et distance
                const std::string code = duel::makeCode(hasard);
                s.join(std::make_unique<reseau::TransportUdp>(prise(), code), code, true, sv.nick, car, sv.peinture(car).c_str(), carteActuelle(g));
                s.setKm(sv.km);
                WFS_LOG("salon : « Créer un salon » %s (%s, %d km)", code.c_str(), carteActuelle(g).c_str(), s.km());
                return true;
            }
            for (size_t i = 0; i < g_liste.size() && i < 4; i++)
                if (g_salons[i].contient(t.x, t.y)) {   // joinRoom
                    const std::string code = g_liste[i].code;
                    s.join(std::make_unique<reseau::TransportUdp>(prise(), code), code, false, sv.nick, car, sv.peinture(car).c_str(), "");
                    WFS_LOG("salon : rejoint %s (hôte %s)", code.c_str(), g_liste[i].hote.c_str());
                    return true;
                }
            return true;
        }
        if (g_quitter.contient(t.x, t.y)) { WFS_LOG("salon : « Quitter le salon »"); s.leave(); return true; }
        if (g_pret.contient(t.x, t.y)) {
            s.setReady(s.status() != duel::Status::Ready);
            WFS_LOG("salon : %s", s.status() == duel::Status::Ready ? "prêt" : "plus prêt");
            return true;
        }
        if (s.host()) for (int i = 0; i < 4; i++) if (g_km[i].contient(t.x, t.y)) {   // distance : l'hôte seul
            s.setKm(duel::KMS[i]); sv.km = duel::KMS[i]; sv.enregistrer();
            WFS_LOG("salon : distance %d km", duel::KMS[i]);
        }
        return true;
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Salon || g.sceneDediee || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        const Marges& mg = g.marges;
        const float zl = mg.gauche + 32 * k, zr = W - mg.droite - 32 * k;
        r.uiRect(0, 0, W, H, hex(BG, 0.94f));
        // en-tête (#lobby) : .eyebrow + .big
        const float hy = mg.haut + 26 * k;
        ui::text(r, zl, hy, 13 * k, ui::majuscules("Duel en ligne · 2 pilotes"), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
        ui::text(r, zl, hy + 20 * k, 44 * k, ui::majuscules("Duel"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        const float by = H - mg.bas - 24 * k - 56 * k, bh = 56 * k;
        if (duelSession().inRoom()) dessinerSalon(r, g, zl, zr, hy + 92 * k, by, bh, k);
        else dessinerAccueil(r, zl, zr, hy + 92 * k, by, bh, k);
    }

private:
    // #lobbyHome : pseudo, salons qui t'attendent, note, « Créer un salon », « Retour »
    void dessinerAccueil(Renderer& r, float zl, float zr, float y, float by, float bh, float k) {
        for (Zone& z : g_km) z = {};
        g_pret = g_quitter = {};
        ui::text(r, zl, y, 12 * k, ui::majuscules("Ton pseudo"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        ui::text(r, zl, y + 18 * k, 28 * k, sauvegarde().nick, hex(TEXTE), ui::GAUCHE, 0, false, true);
        float ly = y + 78 * k;
        ui::text(r, zl, ly, 12 * k, ui::majuscules("Salons qui t'attendent"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        ly += 22 * k;
        const auto p = prise();
        g_liste = reseau::salonsOuverts(*p);
        const float lw = std::min(zr - zl, 620 * k), lh = 54 * k;
        for (int i = 0; i < 4; i++) g_salons[i] = {};
        for (size_t i = 0; i < g_liste.size(); i++) {   // .btn--room : « Rejoindre <pseudo> », code, terrain
            const auto& sa = g_liste[i];
            const float yy = ly + i * (lh + 8 * k);
            g_salons[i] = {zl, yy, lw, lh};
            r.uiRect(zl, yy, lw, lh, GLASS);
            r.uiRect(zl, yy, 3 * k, lh, hex(ACCENT2));
            ui::text(r, zl + 18 * k, yy + 9 * k, 20 * k, ui::majuscules("Rejoindre " + sa.hote), hex(TEXTE), ui::GAUCHE, 0.06f, false);
            ui::text(r, zl + 18 * k, yy + 33 * k, 13 * k, libelleCarte(sa.carte), hex(MUTED), ui::GAUCHE, 0, false);
            ui::text(r, zl + lw - 18 * k, yy + 12 * k, 26 * k, ui::majuscules(sa.code), hex(ACCENT), ui::DROITE, 0.18f, false, true);
        }
        if (g_liste.empty()) { ui::text(r, zl, ly + 4 * k, 15 * k, "Aucun salon pour l'instant.", hex(0xC9CDDA), ui::GAUCHE, 0, false); ly += 30 * k; }
        else ly += g_liste.size() * (lh + 8 * k);
        // .note / .note.warn (RG-02)
        const bool reseau = p->reseau();
        ui::text(r, zl, ly + 14 * k, 15 * k, reseau ? "Recherche de salons sur le Wi-Fi…" : "Aucun réseau : le duel se joue entre deux appareils sur le même Wi-Fi",
                 reseau ? hex(0xC9CDDA) : hex(ACCENT), ui::GAUCHE, 0, false);
        g_creer = bouton(r, zl, by, bh, k, "Créer un salon", true);
        g_retour = bouton(r, g_creer.x + g_creer.w + 10 * k, by, bh, k, "Retour", false);
    }

    // #lobbyRoom : code, voiture, terrain, distance, joueurs, « Prêt », « Quitter le salon »
    void dessinerSalon(Renderer& r, const GameView& g, float zl, float zr, float y, float by, float bh, float k) {
        const duel::Session& s = duelSession();
        g_creer = g_retour = {}; for (Zone& z : g_salons) z = {};
        ui::text(r, zl, y, 12 * k, ui::majuscules("Code du salon"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        ui::text(r, zl, y + 16 * k, 72 * k, ui::majuscules(s.code()), hex(ACCENT), ui::GAUCHE, 0.18f, false, true);
        // lignes .car-line
        float ly = y + 112 * k;
        auto ligne = [&](const char* lib, const std::string& val) {
            ui::text(r, zl, ly + 6 * k, 12 * k, ui::majuscules(lib), hex(MUTED), ui::GAUCHE, 0.16f, false);
            ui::text(r, zl + 150 * k, ly, 22 * k, val, hex(TEXTE), ui::GAUCHE, 0, false);
            ly += 44 * k;
        };
        const CarDef& cd = carById(g.car ? g.car->id : "golf");
        ligne("Ta voiture", std::string(cd.brand) + " " + cd.model);
        ligne("Terrain", libelleCarte(carteHote(s, g)));
        // distance : 2 / 4 / 6 / 10 km (.km-row, l'hôte seul la change)
        ui::text(r, zl, ly + 14 * k, 12 * k, ui::majuscules("Distance"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        for (int i = 0; i < 4; i++) {
            const float bw = 84 * k, bx = zl + 150 * k + i * (bw + 6 * k), bhh = 40 * k;
            const bool on = s.km() == duel::KMS[i];
            g_km[i] = s.host() ? Zone{bx, ly, bw, bhh} : Zone{};
            r.uiRect(bx, ly, bw, bhh, on ? hex(ACCENT2, 0.16f) : hex(SURFACE2, s.host() ? 1.f : 0.5f));
            if (on) r.uiRect(bx, ly + bhh - 3 * k, bw, 3 * k, hex(ACCENT2));
            ui::text(r, bx + bw / 2, ly + 11 * k, 17 * k, std::to_string(duel::KMS[i]) + " KM", on ? hex(ACCENT2) : hex(TEXTE, s.host() ? 1.f : 0.5f), ui::CENTRE, 0.06f, false);
        }
        // joueurs (#players) : à droite
        const float px = std::max(zl + 620 * k, zr - 420 * k), pw = zr - px;
        float py = y;
        ui::text(r, px, py, 12 * k, ui::majuscules("Pilotes"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        py += 22 * k;
        auto joueur = [&](const std::string& nom, bool pret, bool moi) {
            r.uiRect(px, py, pw, 50 * k, GLASS);
            r.uiRect(px, py, 3 * k, 50 * k, moi ? hex(ACCENT) : hex(ACCENT2));
            ui::text(r, px + 16 * k, py + 14 * k, 20 * k, nom, moi ? hex(ACCENT) : hex(TEXTE), ui::GAUCHE, 0, false);
            ui::text(r, px + pw - 16 * k, py + 17 * k, 14 * k, ui::majuscules(pret ? "Prêt" : "En salon"), pret ? hex(0xB6FF00) : hex(MUTED), ui::DROITE, 0.12f, false);
            py += 58 * k;
        };
        joueur(s.nick() + " (toi)" + (s.host() ? " · hôte" : ""), s.status() == duel::Status::Ready, true);
        const auto os = s.others();
        for (const duel::PeerView& p : os) joueur(duel::peerName(p.f) + (duel::str(p.f, "host") == "1" ? " · hôte" : ""), duel::str(p.f, "st") == "ready", false);
        if (os.empty()) ui::text(r, px + 16 * k, py + 4 * k, 15 * k, "En attente d'un adversaire…", hex(0xC9CDDA), ui::GAUCHE, 0, false);
        // boutons
        const bool pret = s.status() == duel::Status::Ready;
        g_pret = bouton(r, zl, by, bh, k, "Prêt", !pret);   // prêt : bouton clair souligné de cyan (une touche l'annule)
        if (pret) { r.uiRect(g_pret.x + 14 * k, by + bh - 3 * k, g_pret.w - 14 * k, 3 * k, hex(ACCENT2)); }
        g_quitter = bouton(r, g_pret.x + g_pret.w + 10 * k, by, bh, k, "Quitter le salon", false);
    }

    int doigt_ = -1;
};

} // namespace

// ---------------------------------------------------------------- banc d'essai
void benchSalon(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    const std::string nickAvant = sauvegarde().nick;
    sauvegarde().nick = "Alex";
    // deux appareils en boucle locale : moi (A, prise du salon) et Nadia (B, session du banc)
    auto pa = std::make_shared<reseau::Prise>(reseau::Config{47411, {{"127.0.0.1", 47412}}, {}});
    auto pb = std::make_shared<reseau::Prise>(reseau::Config{47412, {{"127.0.0.1", 47411}}, {}});
    if (!pa->ouverte() || !pb->ouverte() || !g.init(r, "golf", "ville")) { WFS_ERR("salon : banc impossible (réseau local ou chargement)"); sauvegarde().nick = nickAvant; courseBenchRules(false); return; }
    g_priseEssai = pa;
    duel::Session nadia; Player nadiaPl; nadiaPl.reset(carById("m4"));
    g.setAutopilot(false); g.camMode = 0;
    double t = 1300;
    int n = 0;
    // une image : l'appareil de Nadia lit sa prise et renvoie sa présence, puis le jeu avance
    auto pas = [&](int nb) { for (int i = 0; i < nb; i++) { nadia.others(); g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); } };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({id, x, y, true}); pas(1); g.onTouch({id, x, y, false}); pas(1);
    };
    pas(20);
    g.allerA(Ecran::Menu); pas(60);
    capture("menu-duel", "« Duel en ligne » actif (plus grisé)");
    // salon-accueil : Nadia a créé un salon sur la Corniche de nuit ; il apparaît dans « Salons qui t'attendent »
    nadia.join(std::make_unique<reseau::TransportUdp>(pb, "n4d1"), "n4d1", true, "Nadia", "m4", "", "plage/nuit");
    g.allerA(Ecran::Salon); pas(30);
    capture("salon-accueil", "salon de Nadia listé");
    nadia.leave(); pas(10);
    // salon-hote : je crée un salon, Nadia le rejoint
    g.render(t); toucher(1, g_creer);
    const std::string code = duelSession().code();
    nadia.join(std::make_unique<reseau::TransportUdp>(pb, code), code, false, "Nadia", "m4", "", "");
    pas(40);
    capture("salon-hote", "code, voiture, terrain, distance, 2 pilotes");
    // salon-pret : Nadia prête, puis moi ; image prise avant le départ
    nadia.setReady(true); pas(40);
    g.render(t);
    const float px = g_pret.x + g_pret.w / 2, py = g_pret.y + g_pret.h / 2;
    g.onTouch({2, px, py, true});   // « Prêt » : appliqué au toucher, le départ se fait à la mise à jour suivante
    capture("salon-pret", "les deux pilotes prêts");
    g.onTouch({2, px, py, false});
    // départ : course sur la carte de l'hôte (moi), Nadia roule 18 m devant dans la file de gauche
    g.setAutopilot(true);
    bool nadiaPartie = false;
    for (int i = 0; i < 60 * 7; i++) {
        pas(1);   // mon départ (checkStart de DuelSystem), puis celui de Nadia dans la même image : même horloge de course
        if (!nadiaPartie && nadia.checkStart()) {
            nadiaPartie = true;
            WFS_LOG("duel : manche %d, salon %s, graine %u, %d km (second joueur, UDP)", nadia.round(), nadia.code().c_str(), nadia.seed(), nadia.km());
        }
        if (nadiaPartie && nadia.trafficClock() >= 0) {
            nadiaPl.dist = g.player().dist + 18 / M; nadiaPl.position = nadiaPl.dist; nadiaPl.playerX = -0.45; nadiaPl.speed = g.player().speed;
            nadia.raceTick(1.0 / 60, nadiaPl, nadia.trafficClock());
        }
    }
    const duel::Session& s = duelSession();
    capture("duel-reseau-course", "course démarrée, adversaire visible, classement à 2");
    WFS_LOG("salon : moi graine %u, Nadia graine %u, trafficClock %.3f / %.3f s, %zu adversaire(s) vu(s), %d message(s) ignoré(s)",
            s.seed(), nadia.seed(), s.trafficClock(), nadia.trafficClock(), s.ghosts().size(), pa->ignores());
    duelSession().leave(); nadia.leave();
    g_priseEssai = nullptr;
    sauvegarde().nick = nickAvant;
    g.shutdown();
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Salon, 453)   // interface (400-499) : après le terrain (452), avant la pause (455)

} // namespace wfs
