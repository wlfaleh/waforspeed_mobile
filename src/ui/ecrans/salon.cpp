// WaForSpeed — écran Salon du duel en ligne (WFS-83) : voir salon.h.
#include "salon.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <random>
#include <thread>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course_system.h"
#include "../../game/course5.h"   // WFS-112 : mode Duel (lien d'invitation), panneau des modes (banc)
#include "../../game/traffic_system.h"   // WFS-112 : banc, course sans trafic
#include "../../game/duel_system.h"
#include "../../game/game.h"
#include "../../game/player.h"
#include "../../game/reseau_udp.h"
#include "../../game/reseau_peerjs.h"   // WFS-89 : duel par Internet
#include "../../game/reseau_relais.h"   // WFS-112 : duel entre amis par le relais
#include "../../game/sauvegarde.h"
#include "../../game/track.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "../../game/commandes_tactiles.h"   // WFS-146 : dessin2d::biseau (un seul polygone)

namespace wfs {
bool choixModeZone(const std::string& nom, float& x, float& y);   // modes_ecran.cpp (WFS-112, banc d'essai)
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
// WFS-89 : « Rejoindre avec un code » (4 cases, clavier des caractères de duel::CODE_CHARS, effacer, « Rejoindre »)
Zone g_touches[32], g_effacer, g_rejoindre;
Zone g_inviter;   // WFS-112 : « Inviter » (partage du lien)
std::string g_saisie;
const reseau::TransportDouble* g_double = nullptr;   // transport du salon en cours (Wi-Fi local + Internet), si ouvert ici
std::string g_doubleCode;
std::vector<reseau::SalonOuvert> g_liste;          // salons affichés à la dernière image (touchés dans le même ordre)
std::shared_ptr<reseau::Prise> g_priseEssai;      // banc d'essai : prise en boucle locale à la place de la prise commune
bool g_depart = false;

std::shared_ptr<reseau::Prise> prise() { return g_priseEssai ? g_priseEssai : reseau::priseCommune(); }

// WFS-89 : salon ouvert à la fois sur le Wi-Fi local et par Internet (même code) ; le premier qui voit un adversaire gagne
// WFS-112 : Internet passe par le relais WebSocket (serveur/) ; le Wi-Fi local reste en repli caché (relais injoignable)
std::unique_ptr<duel::Transport> transportDouble(const std::string& code, bool hote) {
    auto d = std::make_unique<reseau::TransportDouble>(std::make_unique<reseau::TransportUdp>(prise(), code),
                                                       std::make_unique<reseau::TransportRelais>(reseau::fluxDefaut, reseau::adresseRelais(), code, hote));
    g_double = d.get(); g_doubleCode = code;
    return d;
}
const reseau::TransportDouble* doubleActuel() {
    return duelSession().inRoom() && duelSession().code() == g_doubleCode ? g_double : nullptr;
}

void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    // WFS-146 (RG-04, ANO-20) : un seul polygone (avant : 16 bandes translucides qui se chevauchaient → fines lignes sombres)
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
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

    void update(double dt, const GameView& g) override {
        lienProfond(dt, g);   // WFS-112 : jeu ouvert par le lien d'invitation
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

    // WFS-112 (RG-02) : lien d'invitation touché (WhatsApp, SMS…) → le jeu s'ouvre dans le salon de l'ami, code rempli.
    // Lu toutes les 0,5 s hors course (en course, le lien attend la fin de la manche).
    void lienProfond(double dt, const GameView& g) {
        attenteLien_ -= dt;
        if (attenteLien_ > 0 || g.ecran == Ecran::Course || g.ecran == Ecran::Pause || g.ecran == Ecran::Chargement) return;
        attenteLien_ = 0.5;
        const std::string lien = reseau::prendreLienRecu();
        std::string code;
        if (lien.empty()) return;
        if (!reseau::lireLienInvitation(lien, code)) { WFS_WARN("salon : lien d'invitation invalide"); return; }
        duel::Session& s = duelSession();
        if (s.inRoom() && s.code() == code) { g.allerA(Ecran::Salon); return; }
        const char* car = g.car ? g.car->id : "golf";
        Sauvegarde& sv = sauvegarde();
        s.join(transportDouble(code, false), code, false, sv.nick, car, sv.peinture(car).c_str(), "");
        modeCourse() = ModeCourse::Duel;
        WFS_LOG("salon : lien d'invitation reçu, salon %s", code.c_str());
        g.allerA(Ecran::Salon);
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
                s.join(transportDouble(code, true), code, true, sv.nick, car, sv.peinture(car).c_str(), carteActuelle(g));
                s.setKm(sv.km);
                WFS_LOG("salon : « Créer un salon » %s (%s, %d km)", code.c_str(), carteActuelle(g).c_str(), s.km());
                return true;
            }
            for (int i = 0; i < 32; i++)   // WFS-89 : saisie du code
                if (g_touches[i].contient(t.x, t.y) && g_saisie.size() < 4) { g_saisie += duel::CODE_CHARS[i]; return true; }
            if (g_effacer.contient(t.x, t.y)) { if (!g_saisie.empty()) g_saisie.pop_back(); return true; }
            if (g_rejoindre.contient(t.x, t.y) && g_saisie.size() == 4) {
                const std::string code = g_saisie;
                s.join(transportDouble(code, false), code, false, sv.nick, car, sv.peinture(car).c_str(), "");
                WFS_LOG("salon : « Rejoindre » avec le code %s (Wi-Fi local et Internet)", code.c_str());
                g_saisie.clear();
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
        if (g_inviter.contient(t.x, t.y)) {   // WFS-112 (RG-02) : feuille de partage d'Android avec le lien
            const bool ok = reseau::partager(reseau::texteInvitation(s.code()));
            WFS_LOG("salon : « Inviter » (%s)", ok ? "partage ouvert" : "pas de partage sur cette plateforme");
            return true;
        }
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
        ui::text(r, zl, hy, 13 * k, ui::majuscules("Duel entre amis · 2 à 4 pilotes"), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
        ui::text(r, zl, hy + 20 * k, 44 * k, ui::majuscules("Duel"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        g_inviter = {};
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
        // WFS-112 (RG-03) : plus de mention du Wi-Fi ; les salons vus à côté (repli local) ne s'affichent que s'il y en a
        const auto p = prise();
        g_liste = reseau::salonsOuverts(*p);
        if (!g_liste.empty()) { ui::text(r, zl, ly, 12 * k, ui::majuscules("Salons à côté de toi"), hex(MUTED), ui::GAUCHE, 0.16f, false); ly += 22 * k; }
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
        ly += g_liste.size() * (lh + 8 * k);
        // WFS-112 : comment jouer avec un ami, où qu'il soit
        ui::text(r, zl, ly + 4 * k, 15 * k, "Crée un duel et invite un ami :", hex(0xC9CDDA), ui::GAUCHE, 0, false);
        ui::text(r, zl, ly + 26 * k, 15 * k, "il te rejoint où qu'il soit (4G, autre box)", hex(0xC9CDDA), ui::GAUCHE, 0, false);
        ui::text(r, zl, ly + 48 * k, 15 * k, "avec le lien ou le code.", hex(0xC9CDDA), ui::GAUCHE, 0, false);
        g_creer = bouton(r, zl, by, bh, k, "Créer un duel", true);
        g_retour = bouton(r, g_creer.x + g_creer.w + 10 * k, by, bh, k, "Retour", false);
        dessinerCode(r, zr, y, k);
    }

    // WFS-89 : « Rejoindre avec un code » (#codeIn / #joinBtn du jeu web) : 4 cases, clavier, effacer, « Rejoindre »
    void dessinerCode(Renderer& r, float zr, float y, float k) {
        const int N = (int)std::strlen(duel::CODE_CHARS), COLS = 8;
        const float tk = 46 * k, gap = 6 * k, largeur = COLS * tk + (COLS - 1) * gap, x0 = zr - largeur;
        ui::text(r, x0, y, 12 * k, ui::majuscules("Rejoindre avec un code"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        const float cw = 64 * k, ch = 72 * k, cy = y + 22 * k;
        for (int i = 0; i < 4; i++) {   // les 4 cases
            const float cx = x0 + i * (cw + 10 * k);
            const bool pleine = i < (int)g_saisie.size(), suivante = i == (int)g_saisie.size();
            r.uiRect(cx, cy, cw, ch, GLASS);
            r.uiRect(cx, cy + ch - 3 * k, cw, 3 * k, suivante ? hex(ACCENT2) : pleine ? hex(ACCENT) : hex(SURFACE2));
            if (pleine) ui::text(r, cx + cw / 2, cy + 12 * k, 44 * k, ui::majuscules(g_saisie.substr(i, 1)), hex(TEXTE), ui::CENTRE, 0, false, true);
        }
        const float ky = cy + ch + 18 * k;
        for (int i = 0; i < 32; i++) g_touches[i] = {};
        for (int i = 0; i <= N; i++) {   // clavier : 31 caractères puis « effacer »
            const float tx = x0 + (i % COLS) * (tk + gap), ty = ky + (i / COLS) * (tk + gap);
            r.uiRect(tx, ty, tk, tk, hex(SURFACE2));
            if (i < N) {
                g_touches[i] = {tx, ty, tk, tk};
                ui::text(r, tx + tk / 2, ty + 11 * k, 22 * k, ui::majuscules(std::string(1, duel::CODE_CHARS[i])), hex(TEXTE), ui::CENTRE, 0, false);
            } else {
                g_effacer = {tx, ty, tk, tk};
                // WFS-146 (RG-08, ANO-43) : icône « retour arrière » ⌫ lisible (pentagone pointé à gauche, croix), trait 2 px
                // blanc #F2F4F8 (le cyan est réservé à la sélection) ; même touche, même place, même taille qu'avant
                const float iw = 22 * k, ih = 16 * k, ix = tx + (tk - iw) / 2, iy = ty + (tk - ih) / 2, p = 6 * k, ep = 2 * k;
                const vec2 penta[5] = {{ix, iy + ih / 2}, {ix + p, iy}, {ix + iw, iy}, {ix + iw, iy + ih}, {ix + p, iy + ih}};
                dessin2d::contour(r, penta, 5, ep, hex(TEXTE));
                const float mx = ix + p + (iw - p) / 2, my = iy + ih / 2, c = 3 * k;   // croix de 6 px au centre du corps
                const vec2 d1[2] = {{mx - c, my - c}, {mx + c, my + c}}, d2[2] = {{mx - c, my + c}, {mx + c, my - c}};
                dessin2d::trait(r, d1, 2, ep, hex(TEXTE)); dessin2d::trait(r, d2, 2, ep, hex(TEXTE));
            }
        }
        const float ry = ky + 4 * (tk + gap) + 6 * k, rh = 50 * k;
        if (g_saisie.size() == 4) g_rejoindre = bouton(r, zr, ry, rh, k, "Rejoindre", true, true);
        else { g_rejoindre = {}; const Zone z = bouton(r, zr, ry, rh, k, "Rejoindre", false, true); r.uiRect(z.x, z.y, z.w, z.h, hex(BG, 0.5f)); }   // grisé
    }

    // #lobbyRoom : code, voiture, terrain, distance, joueurs, « Prêt », « Quitter le salon »
    void dessinerSalon(Renderer& r, const GameView& g, float zl, float zr, float y, float by, float bh, float k) {
        const duel::Session& s = duelSession();
        g_creer = g_retour = g_effacer = g_rejoindre = {}; for (Zone& z : g_salons) z = {}; for (Zone& z : g_touches) z = {};
        ui::text(r, zl, y, 12 * k, ui::majuscules("Code du salon"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        ui::text(r, zl, y + 16 * k, 72 * k, ui::majuscules(s.code()), hex(ACCENT), ui::GAUCHE, 0.18f, false, true);
        // WFS-112 (RG-02) : « Inviter » à côté du code (partage du lien), lien et validité du code (RG-06 : 15 min)
        const float cx = zl + ui::width(72 * k, ui::majuscules(s.code()), 0.18f) + 28 * k;
        g_inviter = bouton(r, cx, y + 24 * k, 52 * k, k, "Inviter", true);
        const duel::Transport* tr = s.transport();
        std::string sous = reseau::lienInvitation(s.code());
        if (tr && tr->expireDansMs() >= 0) {
            const int mn = (int)std::ceil(tr->expireDansMs() / 60000);
            sous += mn > 0 ? " · code valable " + std::to_string(mn) + " min" : " · code expiré pour les nouveaux";
        }
        ui::text(r, cx, y + 84 * k, 13 * k, sous, hex(MUTED), ui::GAUCHE, 0, false);
        // WFS-146 (RG-08, ANO-44) : aide de RG-07 de WFS-89 sous le code (12 px #8A90A6), dans l'espace libre avant les lignes
        ui::text(r, zl, y + 96 * k, 12 * k, "Donne ce code à ton adversaire : il le tape dans « Rejoindre avec un code ».", hex(MUTED), ui::GAUCHE, 0, false);
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
        // WFS-112 : état du lien (RG-05 : ping, reconnexion) ; échec : message clair (code inconnu, expiré, complet…)
        if (const reseau::TransportDouble* dbl = doubleActuel()) {
            std::string net; bool warn = false;
            const duel::Lien l = dbl->lien();
            if (dbl->choisi() == 1) net = "Adversaire trouvé à côté de toi";
            else if (l == duel::Lien::Pret) net = dbl->pingMs() >= 0 ? "Connecté · ping " + std::to_string(dbl->pingMs()) + " ms" : "Connecté";
            else if (l == duel::Lien::Connexion) net = "Connexion au serveur du duel…";
            else if (l == duel::Lien::Reconnexion) { net = "Reconnexion… " + std::to_string((int)std::ceil(dbl->reconnexionRestanteMs() / 1000)) + " s"; warn = true; }
            else {
                const std::string ra = dbl->raison();
                net = ra == "inconnu" ? "Ce code n'existe pas : vérifie-le avec ton ami"
                    : ra == "expire" ? "Ce code a expiré : demande un nouveau code"
                    : ra == "plein" ? "Ce salon est complet (4 pilotes)"
                    : ra == "pris" ? "Code déjà pris : crée un nouveau duel"
                    : ra == "perdu" ? "Connexion perdue : quitte le salon et recommence"
                    : "Serveur du duel injoignable : vérifie ta connexion Internet";
                warn = true;
            }
            ui::text(r, px + 16 * k, py + (os.empty() ? 34 : 4) * k, 14 * k, net, warn ? hex(ACCENT) : hex(0xC9CDDA), ui::GAUCHE, 0, false);
        }
        // boutons
        const bool pret = s.status() == duel::Status::Ready;
        g_pret = bouton(r, zl, by, bh, k, "Prêt", !pret);   // prêt : bouton clair souligné de cyan (une touche l'annule)
        if (pret) { r.uiRect(g_pret.x + 14 * k, by + bh - 3 * k, g_pret.w - 14 * k, 3 * k, hex(ACCENT2)); }
        g_quitter = bouton(r, g_pret.x + g_pret.w + 10 * k, by, bh, k, "Quitter le salon", false);
    }

    int doigt_ = -1;
    double attenteLien_ = 0;
};

} // namespace

// ---------------------------------------------------------------- banc d'essai
// WFS-112 (RG-11) : parcours complet du duel entre amis, au doigt, avec le relais local et un second client simulé :
// accueil → Course solo → « Duel entre amis » → Continuer → salon → « Créer un duel » (code) → « Inviter » (lien) →
// l'ami rejoint par le lien → les deux « Prêt » → course sans trafic → arrivée → écran de fin → « Revanche » → course →
// arrivée → « Retour » → menu des modes. Captures salon-amis (code, « Inviter », 2 pilotes, ping) et salon-amis-fin.
// Chaque étape est écrite au journal (« parcours duel : … (ok) ») ; rend le nombre d'écarts.
static int benchDuelAmis(Game& g, Renderer& r, const std::string& out, double& t) {
    int erreurs = 0, n = 0, doigt = 60;
    duel::Session ami; Player amiPl; amiPl.reset(carById("m4"));
    auto image = [&] {   // une image, en temps réel (le relais et l'horloge du duel sont réels)
        ami.others(); g.update(1.0 / 60); t += 1.0 / 60; if (++n % 6 == 0) g.render(t);
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    };
    auto images = [&](int k) { for (int i = 0; i < k; i++) image(); };
    auto verifier = [&](bool ok, const char* quoi) { if (ok) WFS_LOG("parcours duel : %s (ok)", quoi); else { erreurs++; WFS_ERR("parcours duel : %s (ÉCHEC)", quoi); } };
    auto appui = [&](float x, float y) {   // doigt Android : posé, deux mouvements, levé
        const int id = doigt++;
        g.render(t);
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x + 1, y, true}); image(); g.onTouch({id, x + 2, y, true}); image();
        g.onTouch({id, x + 2, y, false}); images(3);
    };
    auto zone = [&](const Zone& z) { g.render(t); const Zone c = z; if (c.w <= 0) { erreurs++; WFS_ERR("parcours duel : bouton absent"); return; } appui(c.x + c.w / 2, c.y + c.h / 2); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi);
    };
    // l'ami roule 25 m derrière moi et passe la ligne 1,5 s après moi (temps de course du jeu, même base que le mien)
    int apres = 0;
    auto rouler = [&](double monArrivee) {
        if (ami.status() == duel::Status::Ready) ami.checkStart();
        if (ami.status() != duel::Status::Race || ami.trafficClock() < 0) return;
        amiPl.dist = std::max(0.0, g.player().dist - 25 / M); amiPl.position = amiPl.dist; amiPl.playerX = 0.45; amiPl.speed = g.player().speed;
        if (monArrivee > 0 && ++apres > 20) amiPl.dist = ami.finish();
        ami.raceTick(1.0 / 60, amiPl, monArrivee > 0 ? monArrivee + 1.5 : 0);
    };
    auto manche = [&](int numero) {   // départ commun, course sans trafic, arrivée des deux pilotes, écran de fin
        bool course = false;
        for (int i = 0; i < 600 && !course; i++) { image(); rouler(-1); course = g.ecran() == Ecran::Course && duelSession().status() == duel::Status::Race; }
        verifier(course && duelSession().round() == numero && ami.round() == numero && duelSession().seed() == ami.seed(), numero == 1 ? "les deux « Prêt » → course, même manche et même graine" : "« Revanche » → nouvelle manche, même graine");
        const Traffic* tr = activeTraffic();
        images(30);
        verifier(!tr || tr->cars.empty(), "course sans trafic (WFS-110 RG-07)");
        double moi = -1; apres = 0;
        for (int i = 0; i < 900 && !(duelSession().status() == duel::Status::Done && ami.status() == duel::Status::Done); i++) {
            image(); if (moi < 0 && duelSession().status() == duel::Status::Done) moi = duelSession().myFinish(); rouler(moi);
        }
        verifier(duelSession().status() == duel::Status::Done && ami.status() == duel::Status::Done, "arrivée des deux pilotes");
        images(70);   // écran de fin après 0,9 s
        const auto res = duelSession().results();
        verifier(res.size() == 2 && res[0].me && res[1].done, "écran de fin : classement (1er moi, puis l'ami) et temps");
    };
    g.setAutopilot(false); g.camMode = 0;
    choixModeBanc(true); duelArriveeForcee(false);
    g.allerA(Ecran::Menu); images(20);
    verifier(g.ecran() == Ecran::Menu, "accueil");
    ouvrirChoixMode(); images(5);
    float x = 0, y = 0;
    if (choixModeZone("carte3", x, y)) appui(x, y); else erreurs++;
    if (choixModeZone("continuer", x, y)) appui(x, y); else erreurs++;
    images(5);
    verifier(g.ecran() == Ecran::Salon, "« Duel entre amis » puis « Continuer » → salon");
    zone(g_creer); images(5);
    const std::string code = duelSession().code();
    for (int i = 0; i < 200 && duelSession().transport() && duelSession().transport()->lien() != duel::Lien::Pret; i++) image();
    verifier(duelSession().inRoom() && duel::validCode(code) && duelSession().host(), "« Créer un duel » : code affiché");
    verifier(duelSession().transport() && duelSession().transport()->lien() == duel::Lien::Pret, "connecté au relais");
    zone(g_inviter);
    std::string codeLien;
    verifier(reseau::lireLienInvitation(reseau::dernierPartage(), codeLien) && codeLien == code, "« Inviter » : lien partagé avec le code");
    WFS_LOG("parcours duel : invitation « %s »", reseau::dernierPartage().c_str());
    ami.join(std::make_unique<reseau::TransportRelais>(reseau::fluxDefaut, reseau::adresseRelais(), codeLien, false), codeLien, false, "Nadia", "m4", "", "");
    for (int i = 0; i < 300 && (duelSession().others().empty() || ami.others().empty() || duelSession().transport()->pingMs() < 0); i++) image();
    verifier(duelSession().others().size() == 1 && ami.others().size() == 1, "l'ami rejoint par le lien : 2 pilotes au salon");
    images(20);
    capture("salon-amis", "code, « Inviter », lien, 2 pilotes, ping");
    ami.setReady(true); images(10);
    zone(g_pret);
    duelArriveeForcee(true);   // je passe la ligne dès le départ : la manche dure le compte à rebours
    manche(1);
    capture("salon-amis-fin", "fin du duel : victoire, temps, « Revanche » et « Retour »");
    float rx = 0, ry = 0;
    verifier(duelBouton("revanche", rx, ry), "bouton « Revanche » affiché");
    appui(rx, ry);
    verifier(duelSession().status() == duel::Status::Ready || duelSession().status() == duel::Status::Race, "« Revanche » : prêt pour la manche suivante");
    ami.setReady(true);
    manche(2);
    verifier(duelBouton("retour", rx, ry), "bouton « Retour » affiché");
    appui(rx, ry); images(10);
    verifier(g.ecran() == Ecran::Menu && choixModeOuvert() && !duelSession().inRoom(), "« Retour » → menu des modes (duel quitté)");
    WFS_LOG("parcours duel : %s (%d écart(s))", erreurs ? "ÉCHEC" : "conforme", erreurs);
    ami.leave(); duelSession().leave();
    duelArriveeForcee(false); choixModeBanc(false);
    return erreurs;
}

void benchSalon(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    const std::string nickAvant = sauvegarde().nick;
    sauvegarde().nick = "Alex";
    // deux appareils en boucle locale : moi (A, prise du salon) et Nadia (B, session du banc)
    auto pa = std::make_shared<reseau::Prise>(reseau::Config{47411, {{"127.0.0.1", 47412}}, {}});
    auto pb = std::make_shared<reseau::Prise>(reseau::Config{47412, {{"127.0.0.1", 47411}}, {}});
    if (!pa->ouverte() || !pb->ouverte() || !g.init(r, "golf", "ville")) { WFS_ERR("salon : banc impossible (réseau local ou chargement)"); sauvegarde().nick = nickAvant; courseBenchRules(false); return; }
    g_priseEssai = pa;
    // WFS-112 : relais du duel entre amis lancé en local (node serveur/local.js), à la place du relais Cloudflare
    reseau::RelaisLocal relais;
    const bool relaisOk = relais.demarrer("serveur/local.js");
    if (relaisOk) reseau::adresseRelais() = reseau::AdresseRelais{"127.0.0.1", relais.port(), false};
    else WFS_ERR("salon : relais local impossible (node absent ?) : captures du duel entre amis sans relais");
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
    // ---------- WFS-89 : « Rejoindre avec un code » et duel par Internet (réseau PeerJS simulé, mêmes messages BinaryPack) ----------
    g.setAutopilot(false);
    g.allerA(Ecran::Salon); pas(20);
    capture("salon-code", "bloc « Rejoindre avec un code » (4 cases vides, clavier)");
    for (char c : std::string("k7qx")) {
        const char* p = std::strchr(duel::CODE_CHARS, c);
        if (p) { g.render(t); toucher(30, g_touches[p - duel::CODE_CHARS]); }
    }
    capture("salon-code-saisi", "code K7QX saisi, « Rejoindre » actif");
    for (int i = 0; i < 4; i++) { g.render(t); toucher(31, g_effacer); }
    // WFS-112 : un ami qui passe par Internet (relais local), comme un téléphone en 4G
    g.render(t); toucher(32, g_creer);
    const std::string codeNet = duelSession().code();
    duel::Session web;
    web.join(std::make_unique<reseau::TransportRelais>(reseau::fluxDefaut, reseau::adresseRelais(), codeNet, false), codeNet, false, "Sam", "p911", "", "");
    for (int i = 0; i < 30 && (duelSession().others().empty() || duelSession().transport()->pingMs() < 0); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(35)); web.others(); pas(2);
    }
    capture("salon-internet", "adversaire arrivé par Internet (relais), 2 pilotes, ping");
    WFS_LOG("salon : Internet, salon %s, %zu adversaire(s) vu(s)", codeNet.c_str(), duelSession().others().size());
    duelSession().leave(); web.leave();
    // pas d'Internet : relais injoignable (port fermé) ; message orange après 10 s, le salon reste ouvert à côté
    const reseau::AdresseRelais avant = reseau::adresseRelais();
    reseau::adresseRelais() = reseau::AdresseRelais{"127.0.0.1", 1, false};
    pas(5); g.render(t); toucher(33, g_creer);
    for (int i = 0; i < 110; i++) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); pas(1); }
    capture("salon-hors-ligne", "sans Internet : serveur du duel injoignable (note orange), salon ouvert");
    duelSession().leave();
    reseau::adresseRelais() = avant;
    if (relaisOk) benchDuelAmis(g, r, out, t);
    reseau::adresseRelais() = reseau::AdresseRelais{};
    relais.arreter();
    g_priseEssai = nullptr;
    sauvegarde().nick = nickAvant;
    g.shutdown();
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Salon, 453)   // interface (400-499) : après le terrain (452), avant la pause (455)

} // namespace wfs
