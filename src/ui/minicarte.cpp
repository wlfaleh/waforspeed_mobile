// WaForSpeed — carte de navigation façon GPS au HUD (WFS-234, à la place de la mini-carte de WFS-173) : voir minicarte.h
// (place, projection, découpe) et navigation.h (manœuvres, distances, noms de rues, terrain, signalements). Système enfichable
// (ordre 422 : après le HUD 420, avant les bandeaux 425) ; lecture seule de la course, du joueur, de la piste, du trafic et
// des autres voitures. Aucun toucher (RG-04 : ce n'est pas un bouton), même place que la mini-carte (placer() de WFS-173 :
// aucun bouton ne bouge), aucune allocation par image (tableaux de taille fixe), aucun dessin 3D, sommets comptés (coût borné).
//   RG-01 : carte orientée dans le sens de la marche (flèche du joueur vers le haut), zoom selon la vitesse, rues autour
//           (route, rues des carrefours, anneau et branches des ronds-points), aplats doux (bâtiments, verdure, eau), clair le
//           jour, sombre la nuit ;
//   RG-02 : itinéraire en couleur, bandeau du prochain virage (flèche, « 250 m », consigne, rue suivante), arrivée, checkpoint,
//           nom (inventé) de la rue en cours ;
//   RG-03 : signalements : police, barrage, adversaires, accident, bouchon (trafic réel), radar et travaux (fictifs, sans effet
//           sur le jeu), apparition et disparition en fondu.
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
#include "../game/traffic_system.h"
#include "../render/eclairage_physique.h"   // nuit (lecture seule)
#include "../render/renderer.h"
#include "../world/zones.h"
#include "ecrans/reglages.h"   // gaucher (lecture seule)
#include "navigation.h"
#include "text.h"

namespace wfs {
namespace {

using namespace minicarte;

// codes sRGB comme uiRect ; teintes du HUD (hud.cpp) pour le bandeau et la flèche du joueur, aplats doux pour la carte (RG-01)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, DANGER = 0xFF1F4B, SOMBRE = 0x0A0B10;
struct Palette { unsigned fond, batiment, batimentBord, verdure, foret, champs, eau, sable, gravier, bordure, route, itineraire, passe; float alpha; };
// jour : carte claire (fond gris très pâle, verts et bleus doux) ; nuit : carte sombre, mêmes repères en teintes assourdies
constexpr Palette JOUR{0xEDEFF2, 0xCACED6, 0xB8BDC7, 0xCBE6C4, 0xAED6A4, 0xE6EBC8, 0xA9D3F0, 0xF1E6C8, 0xE2DED6, 0xAEB4BD, 0xFFFFFF, 0x1A73E8, 0xB9C0CA, 0.94f};
constexpr Palette NUIT{0x1B2029, 0x303744, 0x3A4250, 0x1F3328, 0x1A2C22, 0x2A2F22, 0x17283D, 0x2E2A22, 0x2B2A28, 0x0E1117, 0x4A5363, 0x00C8F0, 0x5A6170, 0.92f};

constexpr int POINTS_MAX = 256;            // une portion de route découpée
constexpr int ROUTE_N = 72;                // points de la route dans la fenêtre (RG-01 : de −0,5 à +1,5 fois la vue avant)
constexpr int SIGNAUX_MAX = 24;            // signalements suivis (fondu)
constexpr int TRAFIC_MAX = 64;             // voitures du trafic lues pour les accidents et bouchons
constexpr float BANDEAU = 38;              // hauteur (px de référence) du haut de la carte réservé au bandeau : calcul de la vue
// WFS-245 (RG-09, ANO-141 ; arbitrage du PO du 2026-11-07 22:05 UTC) : textes du bandeau 2 fois plus grands, dans le même cadre :
// bandeau dessiné sur 56 (au lieu de 38 ; la vue de la carte ne change pas, le bandeau en couvre le haut), flèche 28 × 28,
// rangée 1 : distance 32 (réduite jusqu'à 28 si trop longue), rangée 2 : consigne 20 sur une ligne (jusqu'à 16) ; nom de rue 16
// (jusqu'à 15) sur une étiquette de 18 (avant : 15, 10, 8 et 13) ; valeurs × côté / 150 sur un cadre plus petit
constexpr float BANDEAU_DESSIN = 56, FLECHE = 28, DISTANCE_PX = 32, DISTANCE_MIN = 28, CONSIGNE_PX = 20, CONSIGNE_MIN = 16;
constexpr float NOM = 18, NOM_PX = 16, NOM_MIN = 15, MONTANTES = 0.551f;   // MONTANTES : haut des « d », accents (× la taille)
constexpr float FONDU_S = 0.35f;           // RG-03 : durée du fondu des signalements (s)

// dernière image dessinée : journal du banc (RG-05)
struct EtatCarte { bool dessinee = false, nuit = false; int points = 0, sommets = 0, objets = 0, signaux = 0; float cote = 0, avantM = 0;
                   nav::Prochaine prochaine; std::string consigne, rue, rueSuivante, distance;
                   float tailleDistance = 0, tailleConsigne = 0, tailleRue = 0; };   // WFS-245 : tailles passées à ui::text (px)
EtatCarte g_etat;
const Track* g_piste = nullptr;   // piste de la dernière image (banc : prochaine manœuvre)

// dessin compté (6 sommets par rectangle, quadrilatère ou disque ; dessin2d : comptes de commandes_tactiles.cpp)
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

// trait de largeur ep sans recouvrement (onglets aux jointures) ; frange : bord lissé d'un pixel (seulement pour le trait le
// plus large d'une route : les traits posés dessus restent dedans)
void bande(Pinceau& P, const vec2* p, int n, float ep, const vec4& c, bool frange) {
    if (n < 2 || n > POINTS_MAX) return;
    vec2 nrm[POINTS_MAX];
    for (int i = 0; i < n; i++) {
        const vec2 dA = unitaire(moins(p[std::min(i + 1, n - 1)], p[std::max(i - 1, 0)]));
        const vec2 dS = unitaire(moins(p[i < n - 1 ? i + 1 : i], p[i < n - 1 ? i : i - 1]));
        const vec2 m = {-dA.y, dA.x}, s = {-dS.y, dS.x};
        nrm[i] = fois(m, 1.f / std::max(scal(m, s), 0.5f));
    }
    const float h = ep / 2;
    for (int i = 0; i + 1 < n; i++) {
        const vec2 a = p[i], b = p[i + 1], na = nrm[i], nb = nrm[i + 1];
        const UiVertex coeur[4] = {sv(moins(a, fois(na, h)), c, c.w), sv(moins(b, fois(nb, h)), c, c.w), sv(plus(b, fois(nb, h)), c, c.w), sv(plus(a, fois(na, h)), c, c.w)};
        P.quad(coeur);
        if (!frange) continue;
        const UiVertex f1[4] = {sv(plus(a, fois(na, h)), c, c.w), sv(plus(b, fois(nb, h)), c, c.w), sv(plus(b, fois(nb, h + 1)), c, 0), sv(plus(a, fois(na, h + 1)), c, 0)};
        const UiVertex f2[4] = {sv(moins(a, fois(na, h)), c, c.w), sv(moins(b, fois(nb, h)), c, c.w), sv(moins(b, fois(nb, h + 1)), c, 0), sv(moins(a, fois(na, h + 1)), c, 0)};
        P.quad(f1); P.quad(f2);
    }
}
// polyligne découpée par le rectangle [x0, x1] × [y0, y1] : chaque morceau qui reste dedans est dessiné
void bandeDecoupee(Pinceau& P, const vec2* p, int n, float x0, float y0, float x1, float y1, float ep, const vec4& c, bool frange) {
    vec2 run[POINTS_MAX]; int m = 0;
    for (int i = 0; i + 1 < n; i++) {
        vec2 a = p[i], b = p[i + 1];
        const vec2 a0 = a, b0 = b;
        if (!decouper(a, b, x0, y0, x1, y1)) { bande(P, run, m, ep, c, frange); m = 0; continue; }
        if (m == 0 || a.x != a0.x || a.y != a0.y) { bande(P, run, m, ep, c, frange); m = 0; run[m++] = a; }
        if (m < POINTS_MAX) run[m++] = b;
        if (b.x != b0.x || b.y != b0.y) { bande(P, run, m, ep, c, frange); m = 0; }
    }
    bande(P, run, m, ep, c, frange);
}
// quadrilatère convexe découpé par le rectangle (Sutherland-Hodgman, ≤ 8 sommets), rempli en éventail de quadrilatères
void aplat(Pinceau& P, const vec2 (&q)[4], float x0, float y0, float x1, float y1, const vec4& c) {
    vec2 a[12], b[12]; int n = 4;
    for (int i = 0; i < 4; i++) a[i] = q[i];
    auto passe = [&](auto dedans, auto coupe) {
        int m = 0;
        for (int i = 0; i < n; i++) {
            const vec2 p = a[i], s = a[(i + 1) % n];
            const bool dp = dedans(p), ds = dedans(s);
            if (dp && m < 12) b[m++] = p;
            if (dp != ds && m < 12) b[m++] = coupe(p, s);
        }
        n = m; for (int i = 0; i < n; i++) a[i] = b[i];
    };
    auto lerpX = [](vec2 p, vec2 s, float x) { const float t = (x - p.x) / (s.x - p.x); return vec2{x, p.y + (s.y - p.y) * t}; };
    auto lerpY = [](vec2 p, vec2 s, float y) { const float t = (y - p.y) / (s.y - p.y); return vec2{p.x + (s.x - p.x) * t, y}; };
    passe([&](vec2 p) { return p.x >= x0; }, [&](vec2 p, vec2 s) { return lerpX(p, s, x0); });
    passe([&](vec2 p) { return p.x <= x1; }, [&](vec2 p, vec2 s) { return lerpX(p, s, x1); });
    passe([&](vec2 p) { return p.y >= y0; }, [&](vec2 p, vec2 s) { return lerpY(p, s, y0); });
    passe([&](vec2 p) { return p.y <= y1; }, [&](vec2 p, vec2 s) { return lerpY(p, s, y1); });
    for (int i = 1; i + 1 < n; i += 2) {   // éventail : (0, i, i+1, i+2), le dernier triangle seul a un sommet doublé
        const vec2 d = a[std::min(i + 2, n - 1)];
        const UiVertex v[4] = {sv(a[0], c, c.w), sv(a[i], c, c.w), sv(a[i + 1], c, c.w), sv(d, c, c.w)};
        P.quad(v);
    }
}

vec2 plan(const TrackPoint& q) { return {(float)q.x, (float)q.z}; }   // plan horizontal du monde, en mètres

class MiniCarte : public System {
public:
    const char* name() const override { return "mini-carte"; }

    void drawUi(Renderer& r, const GameView& g) override {
        g_etat = {};
        // exactement quand le HUD de course s'affiche (Hud::drawUi, hud.cpp)
        const Course* c = activeCourse();
        if (!c || !g.player || !g.track || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        if (c->timeLeft > 1e6) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        const bool poursuite = modeCourse() == ModeCourse::Poursuite;
        const Cadre carre = placer(W, H, g.marges, obstacles(W, H, g.marges, poursuite, ecrans::reglages().gaucher), poursuite);
        if (carre.w <= 0) {   // pas assez de place : pas de carte, une seule ligne au journal (WFS-173, RG-03)
            if (!tropPetit_) WFS_LOG("mini-carte : pas de place sous %.0f px de côté (écran %.0f x %.0f) : carte masquée", COTE_MIN * k, W, H);
            tropPetit_ = true;
            return;
        }
        tropPetit_ = false;
        g_piste = g.track;
        const double dt = tempsPrec_ > 0 ? std::clamp(g.time - tempsPrec_, 0.0, 0.25) : 0;
        tempsPrec_ = g.time;
        Pinceau P{r};
        // nuit : celle de l'éclairage (moment choisi au terrain, WFS-208) ; GameView::night n'est rempli par personne
        const eclairage::Etat& ec = eclairage::etat();
        nuit_ = ec.actif ? ec.nuit : g.night;
        carte(P, r, g, *c, carre, k, poursuite, dt);
        g_etat.dessinee = true; g_etat.cote = carre.w; g_etat.sommets = P.sommets; g_etat.nuit = nuit_;
    }

private:
    bool tropPetit_ = false, nuit_ = false;
    double tempsPrec_ = 0, avant_ = -1, cap_ = 0;
    std::array<vec2, ROUTE_N> ecran_{};
    const Track* piste_ = nullptr; const MapDef* carte_ = nullptr;
    std::vector<nav::SignalFixe> fixes_;       // RG-03 : radars et travaux du tour (préparés une fois par carte, hors image)
    struct Suivi { nav::Signal type = nav::Signal::Radar; int id = -1; double s = 0; float lat = 0, a = 0; bool vu = false; };
    std::array<Suivi, SIGNAUX_MAX> suivis_{};
    double rd_[16] = {}; bool fixe_[16] = {};

    // ---- repère : le cap (lissé) de la voiture vers le haut, origine sur la voiture
    struct Vue { Repere R; vec2 joueurPx; float pxParM; float x0, y0, x1, y1; };
    vec2 ecranDe(const Vue& v, vec2 monde) const { const vec2 uv = versCarte(v.R, monde); return {v.joueurPx.x + uv.x * v.pxParM, v.joueurPx.y - uv.y * v.pxParM}; }

    void carte(Pinceau& P, Renderer& r, const GameView& g, const Course& c, const Cadre& carre, float k, bool poursuite, double dt) {
        Track& t = *g.track; const Player& pl = *g.player;
        const Palette& pal = nuit_ ? NUIT : JOUR;
        if (g.track != piste_ || t.map() != carte_) preparer(t);
        // RG-01 : zoom et cap lissés (un GPS ne saute pas) ; le cap suit la voiture : la flèche du joueur reste vers le haut
        avant_ = avant_ < 0 ? nav::avantM(pl.kmh()) : nav::lisser(avant_, nav::avantM(pl.kmh()), dt, nav::TAU_ZOOM);
        cap_ = nav::lisser(cap_, pl.heading, dt, nav::TAU_CAP);
        const double s0 = pl.dist, pas = GPS_PAS_M / M;
        const vec2 tg = unitaire(moins(plan(t.point(s0 + pas, 0)), plan(t.point(s0 - pas, 0))));
        const vec2 droite0 = moins(plan(t.point(s0, 1)), plan(t.point(s0, 0)));
        const vec2 dr = droiteDe(tg, vect(tg, droite0));
        const float ch = std::cos((float)cap_), sh = std::sin((float)cap_);
        const vec2 haut = plus(fois(tg, ch), fois(dr, sh)), droite = moins(fois(dr, ch), fois(tg, sh));
        const float ban = BANDEAU * k;
        Vue v;
        v.R = {plan(t.point(s0, pl.playerX * RW)), haut, droite};
        v.x0 = carre.x + 1; v.x1 = carre.x + carre.w - 1; v.y0 = carre.y + ban; v.y1 = carre.y + carre.h - 1;
        v.joueurPx = {carre.x + carre.w / 2, v.y0 + 0.72f * (v.y1 - v.y0)};
        v.pxParM = (v.joueurPx.y - v.y0) / (float)avant_;
        g_etat.avantM = (float)avant_;
        // fond de la carte
        P.rect(carre.x, carre.y, carre.w, carre.h, hex(pal.fond, pal.alpha));
        // abscisses de la route dans la fenêtre : de −0,5 à +1,5 fois la vue avant (les épingles et lacets reviennent dans le carré)
        const double sA = s0 - 0.5 * avant_ / M, sB = s0 + 1.5 * avant_ / M, ds = (sB - sA) / (ROUTE_N - 1);
        abords(P, g, t, v, pal, sA, sB);
        const float ep = ROUTE * k * 1.35f, epC = ep + 2.5f * k;
        rues(P, t, v, pal, sA, sB, ep, epC, k);
        for (int i = 0; i < ROUTE_N; i++) ecran_[i] = ecranDe(v, plan(t.point(sA + ds * i, 0)));
        bandeDecoupee(P, ecran_.data(), ROUTE_N, v.x0, v.y0, v.x1, v.y1, epC, hex(pal.bordure), true);   // bordure de la route
        // RG-02 : partie passée en clair, itinéraire devant le joueur en couleur ; arrivée : l'itinéraire s'arrête sur la ligne
        const double fin = c.duelFinish > 0 ? c.duelFinish : 0;
        const int iJ = std::clamp((int)((s0 - sA) / ds), 0, ROUTE_N - 2);
        vec2 tr[ROUTE_N + 2]; int m = 0;
        for (int i = 0; i <= iJ; i++) tr[m++] = ecran_[i];
        tr[m++] = v.joueurPx;
        bandeDecoupee(P, tr, m, v.x0, v.y0, v.x1, v.y1, ep, hex(pal.route), false);
        m = 0; tr[m++] = v.joueurPx;
        int iFin = ROUTE_N;
        if (fin > 0 && fin < sB) iFin = std::clamp((int)std::ceil((fin - sA) / ds), iJ + 1, ROUTE_N);
        for (int i = iJ + 1; i < iFin; i++) tr[m++] = ecran_[i];
        if (fin > 0 && fin < sB) tr[m++] = ecranDe(v, plan(t.point(fin, 0)));
        bandeDecoupee(P, tr, m, v.x0, v.y0, v.x1, v.y1, ep, hex(pal.itineraire), false);
        if (iFin < ROUTE_N) {   // au-delà de l'arrivée : route ordinaire
            m = 0; tr[m++] = ecranDe(v, plan(t.point(fin, 0)));
            for (int i = iFin; i < ROUTE_N; i++) tr[m++] = ecran_[i];
            bandeDecoupee(P, tr, m, v.x0, v.y0, v.x1, v.y1, ep, hex(pal.route), false);
        }
        g_etat.points = ROUTE_N;
        // checkpoint et arrivée (RG-02)
        auto dedans = [&](vec2 p, float rayon) { return p.x - rayon >= v.x0 && p.x + rayon <= v.x1 && p.y - rayon >= v.y0 && p.y + rayon <= v.y1; };
        if (!poursuite && c.duelFinish <= 0 && c.nextCp > sA && c.nextCp < sB) {
            const vec2 p = ecranDe(v, plan(t.point(c.nextCp, 0))), d = moins(ecranDe(v, plan(t.point(c.nextCp + pas, 0))), p);
            if (dedans(p, CP_L * k / 2)) checkpoint(P, p, d, k);
        }
        if (fin > 0 && fin > sA && fin < sB) { const vec2 p = ecranDe(v, plan(t.point(fin, 0))); if (dedans(p, DAMIER * k / 2)) arrivee(P, p, k); }
        signaux(P, r, g, c, t, v, k, poursuite, sA, sB, dt);
        joueur(P, v.joueurPx, (float)(pl.heading - cap_), k);   // la voiture vers le haut ; seul l'écart au cap lissé reste
        bandeau(P, r, g, c, t, carre, k, pal);
        nomRue(P, r, g, t, carre, k, pal);
        // bordure de 1 px, angles droits (WFS-173, RG-05)
        const vec4 bord{1, 1, 1, 0.08f};
        P.rect(carre.x, carre.y, carre.w, 1, bord); P.rect(carre.x, carre.y + carre.h - 1, carre.w, 1, bord);
        P.rect(carre.x, carre.y + 1, 1, carre.h - 2, bord); P.rect(carre.x + carre.w - 1, carre.y + 1, 1, carre.h - 2, bord);
    }

    void preparer(Track& t) {
        piste_ = &t; carte_ = t.map();
        fixes_ = nav::signauxFixes(trace::sections(t), M);
        for (Suivi& s : suivis_) s = {};
        avant_ = -1; cap_ = 0;
        WFS_LOG("carte GPS (WFS-234) : %s, %zu sections, %zu signalement(s) fixes (radars, travaux)", carte_ ? carte_->id : "?",
                trace::sections(t).size(), fixes_.size());
    }

    // ---- RG-01 : aplats doux autour de la route : bâtiments en îlots (40 m, rues de 10 m entre eux), verdure, champs, eau
    void abords(Pinceau& P, const GameView& g, Track& t, const Vue& v, const Palette& pal, double sA, double sB) {
        if (!g.map) return;
        const int64_t nSeg = (int64_t)std::llround(t.length() / SEG);
        const double pasA = (sB - sA) / 24;   // bandes de verdure et d'eau : 24 tranches
        for (int i = 0; i < 24; i++) {
            const double a = sA + pasA * i, b = a + pasA;
            const nav::Abords ab = nav::abordsDe(g.map->id, zoneId(*g.map, (int64_t)std::floor((a + b) / 2 / SEG), nSeg));
            for (int cote = -1; cote <= 1; cote += 2) {
                const nav::Terrain te = cote < 0 ? ab.gauche : ab.droite;
                if (te == nav::Terrain::Batiments) continue;
                const float l0 = (float)RW + (te == nav::Terrain::Eau ? 12.f : 2.f), l1 = (float)RW + (te == nav::Terrain::Eau ? 150.f : 90.f);
                if (te == nav::Terrain::Eau) bandeLaterale(P, t, v, a, b, cote, (float)RW + 2, l0, hex(pal.sable));
                bandeLaterale(P, t, v, a, b, cote, l0, l1, hex(teinte(pal, te)));
            }
        }
        // îlots de bâtiments, calés sur l'abscisse (ils ne glissent pas avec la voiture)
        const double ilot = 50 / M, long_ = 40 / M;
        for (double s = std::floor(sA / ilot) * ilot; s < sB; s += ilot) {
            const nav::Abords ab = nav::abordsDe(g.map->id, zoneId(*g.map, (int64_t)std::floor((s + long_ / 2) / SEG), nSeg));
            for (int cote = -1; cote <= 1; cote += 2) {
                if ((cote < 0 ? ab.gauche : ab.droite) != nav::Terrain::Batiments) continue;
                bandeLaterale(P, t, v, s, s + long_, cote, (float)RW + 4, (float)RW + 32, hex(pal.batiment));
            }
        }
    }
    static unsigned teinte(const Palette& p, nav::Terrain t) {
        switch (t) {
            case nav::Terrain::Foret: return p.foret;
            case nav::Terrain::Champs: return p.champs;
            case nav::Terrain::Eau: return p.eau;
            case nav::Terrain::Gravier: return p.gravier;
            default: return p.verdure;
        }
    }
    // bande le long de la route entre les abscisses a et b, de l0 à l1 mètres du centre, du côté cote (−1 gauche, +1 droite)
    void bandeLaterale(Pinceau& P, Track& t, const Vue& v, double a, double b, int cote, float l0, float l1, const vec4& c) {
        const vec2 q[4] = {ecranDe(v, plan(t.point(a, cote * l0))), ecranDe(v, plan(t.point(b, cote * l0))),
                           ecranDe(v, plan(t.point(b, cote * l1))), ecranDe(v, plan(t.point(a, cote * l1)))};
        aplat(P, q, v.x0, v.y0, v.x1, v.y1, c);
    }

    // ---- RG-01 : rues autour : rue qui croise chaque carrefour à feux, anneau et quatre branches de chaque rond-point
    void rues(Pinceau& P, Track& t, const Vue& v, const Palette& pal, double sA, double sB, float ep, float epC, float k) {
        const auto& secs = trace::sections(t);
        const double L = t.length();
        if (L <= 0) return;
        const double tour = std::floor(sA / L) * L;
        for (const trace::Section& s : secs) {
            for (int k2 = 0; k2 < 2; k2++) {
                const double a = tour + k2 * L + s.debutU(), b = tour + k2 * L + s.finU();
                if (b < sA || a > sB) continue;
                if (s.type == trace::Type::Feux && !s.depart) {
                    const double x = (double)s.ligneArret * SEG + trace::CARREFOUR_M / M + (tour + k2 * L);
                    vec2 p[2] = {ecranDe(v, plan(t.point(x, -90))), ecranDe(v, plan(t.point(x, 90)))};
                    bandeDecoupee(P, p, 2, v.x0, v.y0, v.x1, v.y1, epC * 0.85f, hex(pal.bordure), true);
                    bandeDecoupee(P, p, 2, v.x0, v.y0, v.x1, v.y1, ep * 0.8f, hex(pal.route), false);
                } else if (s.type == trace::Type::RondPoint) rondPoint(P, t, v, pal, a, b, ep, epC, k);
            }
        }
    }
    void rondPoint(Pinceau& P, Track& t, const Vue& v, const Palette& pal, double a, double b, float ep, float epC, float k) {
        // centre : cercle par trois points de l'anneau (entre 35 et 65 % de la section : l'anneau de 14 m, trace_cartes.cpp)
        const vec2 p1 = plan(t.point(a + (b - a) * 0.36, 0)), p2 = plan(t.point(a + (b - a) * 0.50, 0)), p3 = plan(t.point(a + (b - a) * 0.64, 0));
        const float d = 2 * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y));
        if (std::fabs(d) < 1e-6f) return;
        const float s1 = scal(p1, p1), s2 = scal(p2, p2), s3 = scal(p3, p3);
        const vec2 o{(s1 * (p2.y - p3.y) + s2 * (p3.y - p1.y) + s3 * (p1.y - p2.y)) / d, (s1 * (p3.x - p2.x) + s2 * (p1.x - p3.x) + s3 * (p2.x - p1.x)) / d};
        const float R = std::sqrt(scal(moins(p1, o), moins(p1, o)));
        // branches : entrée, sortie, et les deux amorces opposées (ville.cpp : abordsRondPoint)
        const vec2 fe = unitaire(moins(plan(t.point(a + 2 / M, 0)), plan(t.point(a, 0)))), fs = unitaire(moins(plan(t.point(b, 0)), plan(t.point(b - 2 / M, 0))));
        const vec2 bras[2] = {fe, fois(fs, -1)};
        for (const vec2& u : bras) {
            vec2 p[2] = {ecranDe(v, plus(o, fois(u, R))), ecranDe(v, plus(o, fois(u, R + 45)))};
            bandeDecoupee(P, p, 2, v.x0, v.y0, v.x1, v.y1, epC, hex(pal.bordure), true);
            bandeDecoupee(P, p, 2, v.x0, v.y0, v.x1, v.y1, ep, hex(pal.route), false);
        }
        vec2 anneau[25];
        for (int i = 0; i <= 24; i++) { const float th = 2 * 3.14159265f * i / 24; anneau[i] = ecranDe(v, plus(o, {R * std::cos(th), R * std::sin(th)})); }
        bandeDecoupee(P, anneau, 25, v.x0, v.y0, v.x1, v.y1, epC, hex(pal.bordure), true);
        bandeDecoupee(P, anneau, 25, v.x0, v.y0, v.x1, v.y1, ep, hex(pal.route), false);
        const vec2 c = ecranDe(v, o);   // îlot central en vert
        const float rIlot = std::max(1.5f * k, (R - 4.5f) * v.pxParM);
        if (c.x - rIlot >= v.x0 && c.x + rIlot <= v.x1 && c.y - rIlot >= v.y0 && c.y + rIlot <= v.y1) P.disque(c, rIlot, hex(pal.verdure));
    }

    // ---- RG-03 : signalements, en fondu ; une icône sort en fondu du bord de la carte et quand le signalement disparaît
    void signaler(nav::Signal type, int id, double s, float lat) {
        Suivi* libre = nullptr;
        for (Suivi& x : suivis_) {
            if (x.id == id && x.type == type && (x.vu || x.a > 0)) { x.s = s; x.lat = lat; x.vu = true; return; }
            if (!libre && x.a <= 0 && !x.vu) libre = &x;
        }
        if (libre) *libre = {type, id, s, lat, 0, true};
    }
    void signaux(Pinceau& P, Renderer& r, const GameView& g, const Course& c, Track& t, const Vue& v, float k, bool poursuite,
                 double sA, double sB, double dt) {
        for (Suivi& x : suivis_) x.vu = false;
        const Player& pl = *g.player;
        const double L = t.length(), tour = L > 0 ? std::floor(sA / L) * L : 0;
        // radars et travaux fictifs (sans effet sur le jeu)
        for (const nav::SignalFixe& f : fixes_) for (int k2 = 0; k2 < 2; k2++) {
            const double s = tour + k2 * L + f.sTour;
            if (s > sA && s < sB) signaler(f.type, f.id * 2 + k2, s, f.type == nav::Signal::Radar ? (float)RW + 3 : 0);
        }
        // police (Poursuite), adversaires (Course à 5, duel)
        if (poursuite) {
            const int n = poursuitePolice(rd_, fixe_, 16);
            for (int i = 0; i < n; i++) signaler(fixe_[i] ? nav::Signal::Barrage : nav::Signal::Police, i, rd_[i], 0);
        }
        int n = course5Positions(rd_, 16);
        if (n == 0 && c.duelFinish > 0) for (const auto& [id, gh] : duelSession().ghosts()) if (n < 16) rd_[n++] = gh.done ? c.duelFinish : gh.rd;
        for (int i = 0; i < n; i++) signaler(nav::Signal::Adversaire, i, rd_[i], 0);
        // trafic : voitures accidentées et bouchons (trafic réel du jeu)
        if (const Traffic* tr = activeTraffic()) {
            double sM[TRAFIC_MAX], kmh[TRAFIC_MAX]; int idx[TRAFIC_MAX], m = 0;
            for (int i = 0; i < (int)tr->cars.size(); i++) {
                const TrafficCar& o = tr->cars[i];
                const double s = pl.dist + t.relZ(o.z - pl.position);
                if (s < sA || s > sB) continue;
                if (o.crashed) signaler(nav::Signal::Accident, i, s, (float)(o.offset * RW));
                if (m < TRAFIC_MAX) {   // tri par insertion (≤ 64 voitures)
                    int j = m++;
                    while (j > 0 && sM[j - 1] > s * M) { sM[j] = sM[j - 1]; kmh[j] = kmh[j - 1]; idx[j] = idx[j - 1]; j--; }
                    sM[j] = s * M; kmh[j] = o.speed / MAX_SPEED * 260.0; idx[j] = i;
                }
            }
            int queues[8];
            const int nb = nav::bouchons(sM, kmh, m, queues, 8);
            for (int q = 0; q < nb; q++) signaler(nav::Signal::Bouchon, (int)std::floor(sM[queues[q]] / 100), sM[queues[q]] / M, 0);
        }
        // fondu et dessin
        const float marge = 10 * k;
        for (Suivi& x : suivis_) {
            if (x.id < 0) continue;
            x.a = std::clamp(x.a + (float)(dt / FONDU_S) * (x.vu ? 1.f : -1.f), 0.f, 1.f);
            if (dt <= 0 && x.vu && x.a <= 0) x.a = 1;   // image figée (banc) : pas de fondu à attendre
            if (x.a <= 0 && !x.vu) { x.id = -1; continue; }
            if (x.s < sA || x.s > sB) continue;
            const vec2 p = ecranDe(v, plan(t.point(x.s, x.lat)));
            const float rayon = (DISQUE / 2 + CONTOUR + 1.5f) * k;
            if (p.x - rayon < v.x0 || p.x + rayon > v.x1 || p.y - rayon < v.y0 || p.y + rayon > v.y1) continue;
            const float bord = std::min({p.x - v.x0, v.x1 - p.x, p.y - v.y0, v.y1 - p.y}) - rayon;   // fondu au bord de la carte
            const float a = x.a * std::clamp(bord / marge, 0.f, 1.f);
            if (a <= 0.02f) continue;
            icone(P, r, g, x.type, x.id, p, k, a);
            g_etat.signaux++;
        }
    }
    void icone(Pinceau& P, Renderer& r, const GameView& g, nav::Signal type, int id, vec2 p, float k, float a) {
        const float d = DISQUE * k, rr = d / 2 + 1.5f * k;
        auto pastille = [&](unsigned fond, const char* lettre) {
            P.disque(p, rr + CONTOUR * k, hex(0xFFFFFF, a)); P.disque(p, rr, hex(fond, a));
            if (lettre) ui::text(r, p.x, p.y - 4.2f * k, 8.5f * k, lettre, hex(0xFFFFFF, a), ui::CENTRE, 0, false);
        };
        switch (type) {
            case nav::Signal::Police: {   // rouge / bleu toutes les 0,25 s, comme les pastilles du HUD de la poursuite
                const bool flash = std::fmod(g.time, 0.5) < 0.25;
                P.disque(p, d / 2 + CONTOUR * k, hex(SOMBRE, a)); P.disque(p, d / 2, (id % 2) ^ flash ? hex(DANGER, a) : vec4{0.25f, 0.45f, 1.f, a});
                break;
            }
            case nav::Signal::Barrage: P.rect(p.x - d / 2, p.y - d / 2, d, d, hex(DANGER, a)); break;
            case nav::Signal::Adversaire: P.disque(p, (d / 2 + CONTOUR * k), hex(SOMBRE, a)); P.disque(p, d / 2, hex(TEXTE, a)); break;
            case nav::Signal::Accident: pastille(0xE53935, "!"); break;
            case nav::Signal::Bouchon: pastille(0xC62828, nullptr); P.rect(p.x - rr * 0.6f, p.y - 1.2f * k, rr * 1.2f, 2.4f * k, hex(0xFFFFFF, a)); break;
            case nav::Signal::Radar: pastille(0x5F6B7A, "R"); break;
            case nav::Signal::Travaux: {   // triangle orange de chantier
                const vec2 tri[3] = {{p.x, p.y - rr * 1.15f}, {p.x + rr * 1.1f, p.y + rr * 0.8f}, {p.x - rr * 1.1f, p.y + rr * 0.8f}};
                P.triangle(tri, hex(0xF29900, a), CONTOUR * k, hex(0xFFFFFF, a));
                break;
            }
        }
        g_etat.objets++;
    }

    // ---- RG-02 : bandeau du prochain virage : flèche, distance arrondie, consigne ; rue suivante si la place le permet
    void bandeau(Pinceau& P, Renderer& r, const GameView& g, const Course& c, Track& t, const Cadre& carre, float k, const Palette& pal) {
        (void)pal;
        const Player& pl = *g.player;
        const auto& secs = trace::sections(t);
        const nav::Prochaine pr = nav::prochaine(secs, pl.dist, t.length(), c.duelFinish, M);
        const float u = echelle(carre, k), h = BANDEAU_DESSIN * u, x = carre.x, y = carre.y, w = carre.w;
        P.rect(x, y, w, h, vec4{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.95f});   // verre sombre du HUD (design-street-racing)
        P.rect(x, y + h - 2 * u, w, 2 * u, hex(ACCENT2, 0.9f));                   // liseré cyan : information
        const float ic = FLECHE * u, cx = x + 5 * u + ic / 2, cy = y + 4 * u + ic / 2;
        fleche(P, pr.m, pr.sortie, {cx, cy}, ic, k);
        const std::string dist = pr.m == nav::Manoeuvre::Aucune ? std::string() : nav::texteDistance(pr.distanceM);
        const std::string cons = nav::texteConsigne(pr), d = dist.empty() ? "Tout droit" : dist;
        // rangée 1 : distance à droite de la flèche, capitales centrées sur elle (y + 18)
        const float tx = x + 38 * u, lwD = x + w - 4 * u - tx;
        const float dp = taille(d, DISTANCE_PX * u, DISTANCE_MIN * u, lwD), baseD = y + 18 * u + 0.5f * hud::CAPITALES * dp;
        ui::text(r, tx, baseD - hud::BASE * dp, dp, ajuster(d, dp, lwD), hex(TEXTE), ui::GAUCHE, 0, false);
        // rangée 2 : consigne sur toute la largeur, une ligne, haut des lettres montantes à y + 37 (liseré à y + 54)
        const float lwC = w - 8 * u, cp = taille(cons, CONSIGNE_PX * u, CONSIGNE_MIN * u, lwC);
        ui::text(r, x + 4 * u, y + 37 * u - (hud::BASE - MONTANTES) * cp, cp, ajuster(cons, cp, lwC), hex(ACCENT2), ui::GAUCHE, 0, false);
        g_etat.prochaine = pr; g_etat.consigne = cons; g_etat.distance = dist; g_etat.tailleDistance = dp; g_etat.tailleConsigne = cp;
        if (pr.section >= 0 && pr.section + 1 < (int)secs.size())
            g_etat.rueSuivante = nav::nomRue(g.map ? g.map->id : "", g.map ? g.map->seed : 0, pr.section + 1, secs[pr.section + 1].type, secs[pr.section + 1].autoroute);
    }
    // WFS-245 (RG-09) : facteur des tailles du bandeau et du nom de rue (px de référence × k), réduit sur un cadre de moins de 150
    static float echelle(const Cadre& carre, float k) { return k * std::min(1.f, carre.w / (COTE * k)); }
    // taille du texte : `px`, réduite par pas de 0,5 jusqu'à `mini` s'il déborde de `lw` (au-delà : coupé avec « … »)
    static float taille(const std::string& s, float px, float mini, float lw) {
        while (px > mini && ui::width(px, s) > lw) px = std::max(mini, px - 0.5f);
        return px;
    }
    static std::string ajuster(const std::string& s, float px, float lw) {   // coupe avec « … » si le texte déborde
        if (ui::width(px, s) <= lw) return s;
        std::string c = s;
        while (!c.empty() && ui::width(px, c + "…") > lw) { c.pop_back(); while (!c.empty() && (c.back() & 0xC0) == 0x80) c.pop_back(); }
        return c + "…";
    }
    // flèche de manœuvre (blanche) : tige verticale puis tête tournée selon le sens ; rond-point : anneau et sortie
    void fleche(Pinceau& P, nav::Manoeuvre m, int sortie, vec2 c, float taille, float k) {
        const vec4 b = hex(TEXTE);
        const float e = std::max(2.f, 3.2f * k), s = taille / 2;
        auto trait = [&](vec2 a, vec2 z) { vec2 p[2] = {a, z}; bande(P, p, 2, e, b, true); };
        auto tete = [&](vec2 pointe, vec2 dir) {
            const vec2 u = unitaire(dir), n{-u.y, u.x};
            const vec2 tri[3] = {plus(pointe, fois(u, 2.5f * k)), plus(moins(pointe, fois(u, 4.5f * k)), fois(n, 5 * k)), moins(moins(pointe, fois(u, 4.5f * k)), fois(n, 5 * k))};
            const vec2 hor[3] = {tri[0], tri[2], tri[1]};
            P.triangle(vect(moins(tri[1], tri[0]), moins(tri[2], tri[0])) > 0 ? tri : hor, b, 0.5f, b);
        };
        if (m == nav::Manoeuvre::Arrivee) {   // damier
            const float d = s * 0.55f;
            for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
                P.rect(c.x - 2 * d / 2 + i * d / 2, c.y - d + j * d / 2, d / 2, d / 2, ((i + j) % 2) ? hex(SOMBRE) : b);
            return;
        }
        if (m == nav::Manoeuvre::RondPoint) {
            vec2 an[17];
            for (int i = 0; i <= 16; i++) { const float th = 2 * 3.14159265f * i / 16; an[i] = {c.x + 0.42f * s * std::cos(th), c.y - 0.1f * s + 0.42f * s * std::sin(th)}; }
            bande(P, an, 17, e * 0.8f, b, true);
            trait({c.x, c.y + s}, {c.x, c.y + 0.32f * s});
            const float th = -3.14159265f / 2 - (sortie - 2) * 3.14159265f / 2;   // 1re : droite, 2e : en face, 3e : gauche
            const vec2 dir{std::cos(th), std::sin(th)}, o{c.x, c.y - 0.1f * s};
            const vec2 a = plus(o, fois(dir, 0.42f * s)), z = plus(o, fois(dir, 0.95f * s));
            trait(a, z); tete(z, dir);
            return;
        }
        const int sens = nav::sensDe(m);
        const vec2 bas{c.x, c.y + s}, coude{c.x, c.y - 0.05f * s};
        if (sens == 0) { trait(bas, {c.x, c.y - 0.7f * s}); tete({c.x, c.y - 0.7f * s}, {0, -1}); return; }
        const bool epingle = m == nav::Manoeuvre::EpingleGauche || m == nav::Manoeuvre::EpingleDroite;
        const bool leger = m == nav::Manoeuvre::LegerGauche || m == nav::Manoeuvre::LegerDroite;
        if (epingle) {   // demi-tour : montée, arc, descente
            const float rx = 0.38f * s, x0 = c.x - sens * rx;   // montée en x0, arc, descente en x0 + 2 rx (centré sur l'icône)
            vec2 tout[12]; int n = 0; tout[n++] = {x0, bas.y};
            for (int i = 0; i < 10; i++) { const float th = 3.14159265f * i / 9; tout[n++] = {x0 + sens * rx * (1 - std::cos(th)), coude.y - 0.25f * s - rx * std::sin(th)}; }
            tout[n++] = {x0 + 2 * sens * rx, coude.y + 0.25f * s};
            bande(P, tout, n, e, b, true);
            tete(tout[n - 1], {0, 1});
            return;
        }
        const vec2 dir = leger ? unitaire(vec2{(float)sens, -1.2f}) : vec2{(float)sens, m == nav::Manoeuvre::SerreGauche || m == nav::Manoeuvre::SerreDroite ? 0.45f : 0};
        const vec2 z = plus(coude, fois(unitaire(dir), 0.62f * s));
        vec2 p[3] = {bas, coude, z};
        bande(P, p, 3, e, b, true);
        tete(z, dir);
    }
    // nom (inventé) de la rue en cours, en bas de la carte, sur une étiquette (RG-02)
    void nomRue(Pinceau& P, Renderer& r, const GameView& g, Track& t, const Cadre& carre, float k, const Palette& pal) {
        const auto& secs = trace::sections(t);
        const int i = nav::sectionDe(secs, g.player->dist, t.length());
        if (i < 0 || !g.map) return;
        const std::string n = nav::nomRue(g.map->id, g.map->seed, i, secs[i].type, secs[i].autoroute);
        g_etat.rue = n;
        // WFS-245 (RG-09) : taille 16 (au lieu de 8, réduite jusqu'à 15), étiquette de 18 à 3 du bas, capitales centrées
        const float u = echelle(carre, k), h = NOM * u, lw = carre.w - 12 * u, px = taille(n, NOM_PX * u, NOM_MIN * u, lw);
        const std::string s = ajuster(n, px, lw);
        const float w = std::min(lw, ui::width(px, s)) + 8 * u, x = carre.x + (carre.w - w) / 2, y = carre.y + carre.h - h - 3 * u;
        P.rect(x, y, w, h, nuit_ ? hex(0x0E1117, 0.88f) : hex(0xFFFFFF, 0.92f));
        const float base = y + h / 2 + 0.5f * hud::CAPITALES * px;
        ui::text(r, carre.x + carre.w / 2, base - hud::BASE * px, px, s, nuit_ ? hex(TEXTE) : hex(0x3C4043), ui::CENTRE, 0, false);
        g_etat.tailleRue = px;
        (void)pal;
    }

    // flèche du joueur : pointe dans le sens de la voiture (angle compté à partir du haut, sens horaire)
    void joueur(Pinceau& P, vec2 p, float angle, float k) {
        const vec2 av{std::sin(angle), -std::cos(angle)}, dr{-av.y, av.x};
        const float l = JOUEUR_L * k, w = JOUEUR_W * k;
        const vec2 pointe = plus(p, fois(av, l * 0.6f)), base = moins(p, fois(av, l * 0.4f));
        const vec2 tri[3] = {pointe, plus(base, fois(dr, w / 2)), moins(base, fois(dr, w / 2))};
        P.triangle(tri, hex(ACCENT), CONTOUR * k, hex(SOMBRE));
    }
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
};

} // namespace

WFS_SYSTEM(MiniCarte, 422)   // interface (400-499) : après le HUD (420), avant les bandeaux (425)

// ---------------------------------------------------------------- banc d'essai (WFS-173 RG-11, WFS-234 RG-05)
void benchMiniCarte(Game& g, Renderer& r, const std::string& out) {
    double t = 900;
    auto image = [&](int i) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); };
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) image(i); };
    auto capture = [&](const std::string& nom) {
        g.render(t += 0.02); g.render(t += 0.02);
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : carte %s (%s), %d points, %d sommets (max %d), côté %.0f px, vue avant %.0f m, %d objets, %d signalement(s) ; "
                "%.2f km, %.0f km/h, mode %s, carte %s", nom.c_str(), g_etat.dessinee ? "dessinée" : "ABSENTE", g_etat.nuit ? "nuit" : "jour",
                g_etat.points, g_etat.sommets, SOMMETS_MAX, g_etat.cote, g_etat.avantM, g_etat.objets, g_etat.signaux,
                g.player().dist * M / 1000, g.player().kmh(), nomMode(modeCourse()), g_piste && g_piste->map() ? g_piste->map()->id : "?");
        WFS_LOG("capture %s : bandeau « %s » « %s » (%.0f m) ; rue « %s » ; rue suivante « %s »", nom.c_str(), g_etat.distance.c_str(),
                g_etat.consigne.c_str(), g_etat.prochaine.distanceM, g_etat.rue.c_str(), g_etat.rueSuivante.c_str());
        WFS_LOG("capture %s (WFS-245 RG-09) : tailles du texte : distance %.1f px (capitales %.1f px), consigne %.1f px (capitales %.1f px), "
                "nom de rue %.1f px (capitales %.1f px)", nom.c_str(), g_etat.tailleDistance, hud::CAPITALES * g_etat.tailleDistance,
                g_etat.tailleConsigne, hud::CAPITALES * g_etat.tailleConsigne, g_etat.tailleRue, hud::CAPITALES * g_etat.tailleRue);
    };
    // roule (pilote automatique) jusqu'à ce que la prochaine manœuvre voulue soit entre dMin et dMax mètres (au plus maxS s)
    auto jusqua = [&](auto voulu, double dMin, double dMax, double maxS) {
        for (int i = 0; i < (int)(maxS * 60); i++) {
            const Course* c = activeCourse();
            if (c && c->state == RaceState::Play) {
                if (!g_piste) { image(i); continue; }
                const nav::Prochaine p = nav::prochaine(trace::sections(*g_piste), g.player().dist, g_piste->length(), c->duelFinish, M);
                if (voulu(p.m) && p.distanceM >= dMin && p.distanceM <= dMax) return true;
            }
            g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 0) g.render(t);   // une image sur 30 : banc plus court sur la CI
        }
        return false;
    };
    auto rondPoint = [](nav::Manoeuvre m) { return m == nav::Manoeuvre::RondPoint; };
    auto serre = [](nav::Manoeuvre m) { return m == nav::Manoeuvre::SerreGauche || m == nav::Manoeuvre::SerreDroite || m == nav::Manoeuvre::EpingleGauche ||
                                              m == nav::Manoeuvre::EpingleDroite || m == nav::Manoeuvre::Lacets; };
    courseBenchRules(true);
    // 1) Checkpoint sur la Ville, pilote automatique : capture quand le prochain checkpoint est entre 100 et 300 m devant
    //    (WFS-173 : nom gardé pour la comparaison avant / après)
    modeCourse() = ModeCourse::Checkpoint;
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true); g.camMode = 0;
        rouler(60 * 4);
        for (int i = 0; i < 90 * 60; i++) {
            const Course* c = activeCourse();
            const double d = c ? (c->nextCp - g.player().dist) * M : 0;
            if (c && c->state == RaceState::Play && d >= 100 && d <= 300) break;
            image(i);
        }
        capture("minicarte-checkpoint");
        g.setAutopilot(false);
        g.shutdown(); g_piste = nullptr;
    }
    // 2) vraie Course à 5 de 4 km, une vingtaine de secondes de course, les 4 ordinateurs sur la carte (WFS-173)
    modeCourse() = ModeCourse::Course5; kmCourse5() = 4;
    if (g.init(r, "golf", "ville")) {
        g.allerA(Ecran::Menu); rouler(30);
        g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
        rouler(60 * 3 + 60 * 20);
        capture("minicarte-ligne");
        g.setAutopilot(false);
        g.shutdown(); g_piste = nullptr;
    }
    // 3) WFS-234 (RG-05) : les 4 cartes, de jour et de nuit, avant un rond-point ou un virage serré, dans 3 modes (le duel est
    //    pris par son propre banc : duel-course) ; Ville et Corniche : rond-point ; Campagne : rond-point ; Grand Prix : virage serré
    struct Cas { const char* carte; ModeCourse mode; bool rp; };
    const Cas cas[] = {{"ville", ModeCourse::Checkpoint, true}, {"plage", ModeCourse::Course5, true}, {"campagne", ModeCourse::Poursuite, true},
                       {"circuit", ModeCourse::Checkpoint, false}};
    const char* ancienMoment = std::getenv("WFS_MOMENT");
    const std::string garde = ancienMoment ? ancienMoment : "";
    for (const Cas& k : cas) for (int nuit = 0; nuit < 2; nuit++) {
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        modeCourse() = k.mode; kmCourse5() = 8;
        if (!g.init(r, "golf", k.carte)) { WFS_LOG("minicarte : carte %s introuvable", k.carte); continue; }
        if (k.mode == ModeCourse::Course5) { g.allerA(Ecran::Menu); rouler(30); g.allerA(Ecran::Course); }
        g.setAutopilot(true); g.camMode = 0;
        rouler(60 * 4);
        const bool ok = k.rp ? jusqua(rondPoint, 80, 200, 120) : jusqua(serre, 80, 200, 120);
        if (!ok) WFS_LOG("minicarte : %s : %s non atteint en 120 s", k.carte, k.rp ? "rond-point" : "virage serré");
        capture(std::string("minicarte-") + k.carte + (nuit ? "-nuit" : "-jour"));
        g.setAutopilot(false);
        g.shutdown(); g_piste = nullptr;
    }
    if (ancienMoment) setenv("WFS_MOMENT", garde.c_str(), 1); else unsetenv("WFS_MOMENT");
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
}

} // namespace wfs
