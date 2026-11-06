// WaForSpeed — mini-carte du parcours au HUD (WFS-173) : voir minicarte.h. Système enfichable (ordre 422 : après le HUD
// 420, avant les bandeaux 425) ; lecture seule de la course, du joueur, de la piste et des autres voitures. Aucun toucher
// (RG-04 : ce n'est pas un bouton), aucune allocation par image (tableaux de taille fixe), aucun dessin 3D (RG-09).
#include "minicarte.h"
#include <array>
#include <cmath>
#include <cstdio>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/duel_system.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/poursuite.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "ecrans/reglages.h"   // gaucher (lecture seule)
#include "text.h"

namespace wfs {
namespace {

using namespace minicarte;

// teintes du HUD (hud.cpp, poursuite_system.cpp), en codes sRGB comme uiRect : aucune nouvelle palette (RG-05)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.08f}, BLEU{0.25f, 0.45f, 1.f, 1};
const unsigned TEXTE = 0xF2F4F8, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, DANGER = 0xFF1F4B, SOMBRE = 0x0A0B10;
constexpr int POINTS_MAX = 256;           // une portion de route découpée (≤ 128 points + coupures)
constexpr int AUTRES_MAX = 16;            // police ou adversaires montrés au plus

// dernière image dessinée : journal du banc (RG-11)
struct EtatCarte { bool dessinee = false, gps = false; int points = 0, sommets = 0, objets = 0; float cote = 0; };
EtatCarte g_etat;

// dessin compté (RG-09 : 6 sommets par rectangle, quadrilatère ou disque ; dessin2d : comptes de commandes_tactiles.cpp)
struct Pinceau {
    Renderer& r; int sommets = 0;
    void rect(float x, float y, float w, float h, const vec4& c) { r.uiRect(x, y, w, h, c); sommets += 6; }
    void quad(const UiVertex q[4]) { r.uiQuad(q); sommets += 6; }
    void disque(vec2 p, float rayon, const vec4& c) { r.uiCircle(p.x, p.y, rayon, c); sommets += 6; }
    void triangle(const vec2 (&p)[3], const vec4& fond, float ep, const vec4& bord) {
        dessin2d::polygone(r, p, 3, dessin2d::uni(fond)); sommets += 4 * 6;   // éventail (1 quad) + frange (3)
        dessin2d::contour(r, p, 3, ep, bord); sommets += 9 * 6;               // 3 quads par côté (ep > 1)
    }
};
UiVertex sv(vec2 p, const vec4& c, float a) { return {p.x, p.y, -1, -1, c.x, c.y, c.z, a}; }

// trait de largeur ep sans recouvrement (onglets aux jointures) : une route translucide (45 %) ne doit pas avoir de grains
// plus sombres à chaque sommet, ce que ferait dessin2d::trait (traits opaques à bouts ronds) ; frange d'un pixel lissée
void bande(Pinceau& P, const vec2* p, int n, float ep, const vec4& c) {
    if (n < 2 || n > POINTS_MAX) return;
    vec2 nrm[POINTS_MAX];
    for (int i = 0; i < n; i++) {
        const vec2 dA = unitaire(moins(p[std::min(i + 1, n - 1)], p[std::max(i - 1, 0)]));   // tangente moyenne au sommet
        const vec2 dS = unitaire(moins(p[i < n - 1 ? i + 1 : i], p[i < n - 1 ? i : i - 1])); // tangente du segment suivant
        const vec2 m = {-dA.y, dA.x}, s = {-dS.y, dS.x};
        nrm[i] = fois(m, 1.f / std::max(scal(m, s), 0.5f));   // onglet borné (angle de 120° au plus)
    }
    const float h = ep / 2;
    for (int i = 0; i + 1 < n; i++) {
        const vec2 a = p[i], b = p[i + 1], na = nrm[i], nb = nrm[i + 1];
        const UiVertex coeur[4] = {sv(moins(a, fois(na, h)), c, c.w), sv(moins(b, fois(nb, h)), c, c.w), sv(plus(b, fois(nb, h)), c, c.w), sv(plus(a, fois(na, h)), c, c.w)};
        const UiVertex f1[4] = {sv(plus(a, fois(na, h)), c, c.w), sv(plus(b, fois(nb, h)), c, c.w), sv(plus(b, fois(nb, h + 1)), c, 0), sv(plus(a, fois(na, h + 1)), c, 0)};
        const UiVertex f2[4] = {sv(moins(a, fois(na, h)), c, c.w), sv(moins(b, fois(nb, h)), c, c.w), sv(moins(b, fois(nb, h + 1)), c, 0), sv(moins(a, fois(na, h + 1)), c, 0)};
        P.quad(coeur); P.quad(f1); P.quad(f2);
    }
}

// polyligne découpée par le rectangle [x0, x1] × [y0, y1] : chaque morceau qui reste dedans est dessiné (RG-06)
void bandeDecoupee(Pinceau& P, const vec2* p, int n, float x0, float y0, float x1, float y1, float ep, const vec4& c) {
    vec2 run[POINTS_MAX]; int m = 0;
    for (int i = 0; i + 1 < n; i++) {
        vec2 a = p[i], b = p[i + 1];
        const vec2 a0 = a, b0 = b;
        if (!decouper(a, b, x0, y0, x1, y1)) { bande(P, run, m, ep, c); m = 0; continue; }
        if (m == 0 || a.x != a0.x || a.y != a0.y) { bande(P, run, m, ep, c); m = 0; run[m++] = a; }
        if (m < POINTS_MAX) run[m++] = b;
        if (b.x != b0.x || b.y != b0.y) { bande(P, run, m, ep, c); m = 0; }
    }
    bande(P, run, m, ep, c);
}

// points d'une polyligne à moins d'un pixel l'un de l'autre fusionnés (RG-07)
int fusionner(const vec2* p, int n, vec2* out) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        if (m > 0 && i < n - 1) { const vec2 d = moins(p[i], out[m - 1]); if (scal(d, d) < 1) continue; }
        if (m < POINTS_MAX) out[m++] = p[i];
    }
    return m;
}

vec2 plan(const TrackPoint& q) { return {(float)q.x, (float)q.z}; }   // plan horizontal du monde, en mètres

class MiniCarte : public System {
public:
    const char* name() const override { return "mini-carte"; }

    void drawUi(Renderer& r, const GameView& g) override {
        g_etat = {};
        // RG-01 : exactement quand le HUD de course s'affiche (Hud::drawUi, hud.cpp)
        const Course* c = activeCourse();
        if (!c || !g.player || !g.track || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        if (c->timeLeft > 1e6) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        const bool poursuite = modeCourse() == ModeCourse::Poursuite;
        const Cadre carre = placer(W, H, g.marges, obstacles(W, H, g.marges, poursuite, ecrans::reglages().gaucher), poursuite);
        if (carre.w <= 0) {   // RG-03 : pas assez de place : pas de carte, une seule ligne au journal
            if (!tropPetit_) WFS_LOG("mini-carte : pas de place sous %.0f px de côté (écran %.0f x %.0f) : carte masquée", COTE_MIN * k, W, H);
            tropPetit_ = true;
            return;
        }
        tropPetit_ = false;
        Pinceau P{r};
        // RG-05 : panneau vitré, bordure de 1 px, angles droits
        P.rect(carre.x, carre.y, carre.w, carre.h, GLASS);
        P.rect(carre.x, carre.y, carre.w, 1, BORD); P.rect(carre.x, carre.y + carre.h - 1, carre.w, 1, BORD);
        P.rect(carre.x, carre.y + 1, 1, carre.h - 2, BORD); P.rect(carre.x + carre.w - 1, carre.y + 1, 1, carre.h - 2, BORD);
        if (!poursuite && c->duelFinish > 0) parcours(P, g, *c, carre, k);
        else gps(P, g, *c, carre, k, poursuite);
        g_etat.dessinee = true; g_etat.cote = carre.w; g_etat.sommets = P.sommets;
    }

private:
    bool tropPetit_ = false;
    std::array<vec2, PARCOURS_MAX> uv_{};      // parcours entier dans le repère de la carte (m), calculé une fois par course
    std::array<vec2, POINTS_MAX> ecran_{}, trace_{};
    int nParcours_ = 0;
    const Track* piste_ = nullptr; const MapDef* carte_ = nullptr; double fin_ = -1, tempsVu_ = 0; int mode_ = -1;
    double rd_[AUTRES_MAX] = {}; bool fixe_[AUTRES_MAX] = {};

    // flèche du joueur : pointe dans le sens de la voiture (angle compté à partir du haut, sens horaire)
    void joueur(Pinceau& P, vec2 p, float angle, float k) {
        const vec2 av{std::sin(angle), -std::cos(angle)}, dr{-av.y, av.x};
        const float l = JOUEUR_L * k, w = JOUEUR_W * k;
        const vec2 pointe = plus(p, fois(av, l * 0.6f)), base = moins(p, fois(av, l * 0.4f));
        const vec2 tri[3] = {pointe, plus(base, fois(dr, w / 2)), moins(base, fois(dr, w / 2))};   // horaire à l'écran
        P.triangle(tri, hex(ACCENT), CONTOUR * k, hex(SOMBRE));
    }
    void disque(Pinceau& P, vec2 p, const vec4& c, float k) { P.disque(p, (DISQUE / 2 + CONTOUR) * k, hex(SOMBRE)); P.disque(p, DISQUE / 2 * k, c); g_etat.objets++; }
    void barrage(Pinceau& P, vec2 p, float k) { const float d = DISQUE * k; P.rect(p.x - d / 2, p.y - d / 2, d, d, hex(DANGER)); g_etat.objets++; }
    void arrivee(Pinceau& P, vec2 p, float k) {   // damier 2 × 2
        const float d = DAMIER * k / 2;
        P.rect(p.x - d, p.y - d, d, d, hex(TEXTE)); P.rect(p.x, p.y - d, d, d, hex(SOMBRE));
        P.rect(p.x - d, p.y, d, d, hex(SOMBRE)); P.rect(p.x, p.y, d, d, hex(TEXTE));
        g_etat.objets++;
    }
    void checkpoint(Pinceau& P, vec2 p, vec2 dir, float k) {   // trait cyan en travers de la route
        const vec2 av = unitaire(dir), dr{-av.y, av.x};
        const vec2 a = fois(dr, CP_L * k / 2), b = fois(av, CP_W * k / 2);
        const vec4 cy = hex(ACCENT2);
        const UiVertex q[4] = {sv(moins(moins(p, a), b), cy, 1), sv(moins(plus(p, a), b), cy, 1), sv(plus(plus(p, a), b), cy, 1), sv(plus(moins(p, a), b), cy, 1)};
        P.quad(q); g_etat.objets++;
    }

    // RG-06 : carte « GPS » : de 150 m derrière à 350 m devant, cap de la route vers le haut, joueur à 70 % de la hauteur
    void gps(Pinceau& P, const GameView& g, const Course& c, const Cadre& carre, float k, bool poursuite) {
        Track& t = *g.track; const Player& pl = *g.player;
        const double s0 = pl.dist, pas = GPS_PAS_M / M;
        const vec2 haut = unitaire(moins(plan(t.point(s0 + pas, 0)), plan(t.point(s0 - pas, 0))));
        const vec2 droite0 = moins(plan(t.point(s0, 1)), plan(t.point(s0, 0)));   // Track::point : lat en mètres à droite
        const Repere R{plan(t.point(s0, pl.playerX * RW)), haut, droiteDe(haut, vect(haut, droite0))};
        const VueGps v = vueGps(carre, k);
        for (int i = 0; i < GPS_POINTS; i++) ecran_[i] = versEcranGps(v, versCarte(R, plan(t.point(s0 + (-GPS_ARRIERE_M + GPS_PAS_M * i) / M, 0))));
        const float ep = ROUTE * k, bord = ep / 2 + 1, x0 = carre.x + bord, y0 = carre.y + bord, x1 = carre.x + carre.w - bord, y1 = carre.y + carre.h - bord;
        bandeDecoupee(P, ecran_.data(), GPS_POINTS, x0, y0, x1, y1, ep, hex(TEXTE, 0.45f));
        g_etat.gps = true; g_etat.points = GPS_POINTS;
        auto dedans = [&](vec2 p, float rayon) { return p.x - rayon >= x0 && p.x + rayon <= x1 && p.y - rayon >= y0 && p.y + rayon <= y1; };
        auto dansFenetre = [&](double s) { return s >= s0 - GPS_ARRIERE_M / M && s <= s0 + GPS_AVANT_M / M; };
        auto ecranDe = [&](double s) { return versEcranGps(v, versCarte(R, plan(t.point(s, 0)))); };
        if (!poursuite && c.duelFinish <= 0 && dansFenetre(c.nextCp)) {   // Checkpoint : prochain checkpoint
            const vec2 p = ecranDe(c.nextCp), d = moins(ecranDe(c.nextCp + pas), p);
            if (dedans(p, CP_L * k / 2)) checkpoint(P, p, d, k);
        }
        if (poursuite) {
            if (c.duelFinish > 0 && dansFenetre(c.duelFinish)) { const vec2 p = ecranDe(c.duelFinish); if (dedans(p, DAMIER * k / 2)) arrivee(P, p, k); }
            const int n = poursuitePolice(rd_, fixe_, AUTRES_MAX);
            const bool flash = std::fmod(g.time, 0.5) < 0.25;   // rouge / bleu toutes les 0,25 s, comme les pastilles du HUD de la poursuite
            for (int i = 0; i < n; i++) {
                if (!dansFenetre(rd_[i])) continue;
                const vec2 p = ecranDe(rd_[i]);
                if (!dedans(p, (DISQUE / 2 + CONTOUR) * k)) continue;
                if (fixe_[i]) barrage(P, p, k); else disque(P, p, (i % 2) ^ flash ? hex(DANGER) : BLEU, k);
            }
        }
        joueur(P, v.joueurPx, (float)pl.heading, k);   // la route au joueur pointe vers le haut : seul l'écart de cap compte
    }

    // RG-07 : parcours entier, départ en bas, arrivée en haut, échelle uniforme ; calculé une fois par course
    void parcours(Pinceau& P, const GameView& g, const Course& c, const Cadre& carre, float k) {
        Track& t = *g.track; const Player& pl = *g.player;
        const double fin = c.duelFinish;
        if (g.track != piste_ || t.map() != carte_ || fin != fin_ || (int)modeCourse() != mode_ || c.raceTime + 0.5 < tempsVu_) preparer(t, fin);
        tempsVu_ = c.raceTime;
        const Ajustement a = ajuster(uv_.data(), nParcours_, carre, k);
        for (int i = 0; i < nParcours_; i++) ecran_[i] = versEcran(a, uv_[i]);
        g_etat.points = nParcours_;
        // partie parcourue (cyan) puis reste (gris) : chaque portion dessinée une fois, coupure au joueur
        int iJ = 0;
        const vec2 pJ = surParcours(ecran_.data(), nParcours_, pl.dist, fin, &iJ);
        int m = 0;
        for (int i = 0; i <= iJ; i++) trace_[m++] = ecran_[i];
        trace_[m++] = pJ;
        vec2 fus[POINTS_MAX];
        const float ep = ROUTE * k;
        bande(P, fus, fusionner(trace_.data(), m, fus), ep, hex(ACCENT2));
        m = 0; trace_[m++] = pJ;
        for (int i = iJ + 1; i < nParcours_; i++) trace_[m++] = ecran_[i];
        bande(P, fus, fusionner(trace_.data(), m, fus), ep, hex(TEXTE, 0.45f));
        arrivee(P, ecran_[nParcours_ - 1], k);
        // RG-08 : adversaires (Course à 5 : 4 ordinateurs ; duel : joueurs du salon), posés sur le parcours ; un arrivé reste à l'arrivée
        int n = course5Positions(rd_, AUTRES_MAX);
        if (n == 0) for (const auto& [id, gh] : duelSession().ghosts()) if (n < AUTRES_MAX) rd_[n++] = gh.done ? fin : gh.rd;
        for (int i = 0; i < n; i++) disque(P, surParcours(ecran_.data(), nParcours_, rd_[i], fin), hex(TEXTE), k);
        // flèche : sens de la route sur la carte à hauteur du joueur, plus l'écart de cap de la voiture
        const vec2 d = moins(ecran_[std::min(iJ + 1, nParcours_ - 1)], ecran_[iJ]);
        joueur(P, pJ, std::atan2(d.x, -d.y) + (float)pl.heading, k);
    }
    void preparer(Track& t, double fin) {
        piste_ = &t; carte_ = t.map(); fin_ = fin; mode_ = (int)modeCourse();
        nParcours_ = PARCOURS_MAX;
        std::array<vec2, PARCOURS_MAX> w{};
        for (int i = 0; i < nParcours_; i++) w[i] = plan(t.point(fin * i / (nParcours_ - 1), 0));
        // départ → arrivée vers le haut ; s'ils sont à moins de 200 m, cap de la route au départ vers le haut
        const vec2 dep = w[0], d = moins(w[nParcours_ - 1], dep);
        const vec2 cap0 = unitaire(moins(plan(t.point(GPS_PAS_M / M, 0)), dep));
        const vec2 haut = std::sqrt(scal(d, d)) >= DROITE_MIN_M ? unitaire(d) : cap0;
        const vec2 droite0 = moins(plan(t.point(0, 1)), plan(t.point(0, 0)));
        const Repere R{dep, haut, droiteDe(haut, vect(cap0, droite0))};
        for (int i = 0; i < nParcours_; i++) uv_[i] = versCarte(R, w[i]);
        WFS_LOG("mini-carte : parcours de %.2f km préparé (%d points)", fin * M / 1000, nParcours_);
    }
};

} // namespace

WFS_SYSTEM(MiniCarte, 422)   // interface (400-499) : après le HUD (420), avant les bandeaux (425)

// ---------------------------------------------------------------- banc d'essai (RG-11)
void benchMiniCarte(Game& g, Renderer& r, const std::string& out) {
    double t = 900;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t += 0.02); g.render(t += 0.02);
        r.capturePng(out + "/" + nom + ".png");
        const Course* c = activeCourse();
        WFS_LOG("capture %s : carte %s, forme %s, %d points, %d sommets (max %d), côté %.0f px, %d objets ; %.2f km, %.0f km/h, mode %s",
                nom, g_etat.dessinee ? "dessinée" : "ABSENTE", g_etat.gps ? "GPS" : "parcours", g_etat.points, g_etat.sommets, SOMMETS_MAX,
                g_etat.cote, g_etat.objets, g.player().dist * M / 1000, g.player().kmh(), nomMode(modeCourse()));
        if (c && c->duelFinish <= 0) WFS_LOG("capture %s : prochain checkpoint à %.0f m", nom, (c->nextCp - g.player().dist) * M);
    };
    courseBenchRules(true);
    // 1) Checkpoint sur la Ville, pilote automatique : capture quand le prochain checkpoint est entre 100 et 300 m devant
    modeCourse() = ModeCourse::Checkpoint;
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true); g.camMode = 0;
        rouler(60 * 4);   // décompte (3 s) et départ
        for (int i = 0; i < 90 * 60; i++) {
            const Course* c = activeCourse();
            const double d = c ? (c->nextCp - g.player().dist) * M : 0;
            if (c && c->state == RaceState::Play && d >= 100 && d <= 300) break;
            g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t);
        }
        capture("minicarte-checkpoint");
        g.setAutopilot(false);
        g.shutdown();
    }
    // 2) vraie Course à 5 de 4 km, une vingtaine de secondes de course, les 4 ordinateurs sur la carte
    modeCourse() = ModeCourse::Course5; kmCourse5() = 4;
    if (g.init(r, "golf", "ville")) {
        g.allerA(Ecran::Menu); rouler(30);
        g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
        rouler(60 * 3 + 60 * 20);
        capture("minicarte-ligne");
        g.setAutopilot(false);
        g.shutdown();
    }
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
}

} // namespace wfs
