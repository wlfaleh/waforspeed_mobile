// WaForSpeed — mini-carte du parcours au HUD (WFS-173, épic E5) : calculs purs (placement, projection, découpe au bord,
// abscisses ramenées au parcours), sans dépendance au rendu : utilisés par le système MiniCarte (minicarte.cpp) et par
// tests/test_minicarte.cpp. La route du jeu ne reboucle jamais en 3D (track.h) : carte « GPS » autour du joueur en
// Checkpoint et en Poursuite, carte du parcours entier (départ → arrivée) en Course à 5 et en Duel.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include "../core/math.h"
#include "../core/system.h"               // Marges
#include "../game/commandes_tactiles.h"   // Cadre, dispositionCommandes (lecture seule)
#include "../game/direction.h"            // boutonsFleches (lecture seule)
#include "toasts.h"                       // bandeauxBordDroit, bandeauxHaut

namespace wfs {

class Renderer;
class Game;
// banc d'essai (RG-11) : captures minicarte-checkpoint et minicarte-ligne (dans outDir), puis g.shutdown()
void benchMiniCarte(Game& g, Renderer& r, const std::string& outDir);

namespace minicarte {

// ---- réglages du PO (regles-de-gestion/WFS-173.md), en pixels de référence 720 p (× k = H / 720) sauf mention
constexpr float COTE = 150, COTE_MIN = 100, ECART = 8;      // RG-02 / RG-03
constexpr float GOUTTIERE = 16, HAUT = 116, HAUT_POURSUITE = 14, MARGE_INT = 10;
constexpr float ROUTE = 4;                                   // RG-05 : trait de la route
constexpr float JOUEUR_L = 12, JOUEUR_W = 9, CONTOUR = 1.5f, DISQUE = 8, DAMIER = 10, CP_L = 16, CP_W = 3;
constexpr double GPS_ARRIERE_M = 150, GPS_AVANT_M = 350, GPS_PAS_M = 5;   // RG-06
constexpr int GPS_POINTS = 101;
constexpr float JOUEUR_Y = 0.70f;                            // RG-06 : joueur à 70 % de la hauteur intérieure
constexpr int PARCOURS_MAX = 128;                            // RG-07
constexpr double DROITE_MIN_M = 200;                         // RG-07 : départ et arrivée plus proches : cap du départ vers le haut
constexpr int SOMMETS_MAX = 4000;                            // RG-09

// ---- petits calculs de vecteurs 2D (vec2 de math.h n'a pas d'opérateurs)
inline vec2 plus(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline vec2 moins(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline vec2 fois(vec2 a, float s) { return {a.x * s, a.y * s}; }
inline float scal(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
inline float vect(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
inline vec2 unitaire(vec2 a) { const float l = std::sqrt(scal(a, a)); return l > 1e-9f ? fois(a, 1.f / l) : vec2{0, 1}; }

// ---- RG-02 / RG-03 : place de la carte
// cadres que la carte ne recouvre jamais (pixels de l'interface), écart de ECART × k compris par placer()
struct Obstacles { Cadre c[12]; int n = 0; void ajouter(const Cadre& x) { if (n < 12) c[n++] = x; } };
// gaucher : commandes en miroir (ecrans::miroirX et ecrans::margesCommandes de reglages.h, recopiés ici sans lire les
// réglages pour être testables) ; poursuite : pas de panneau du score en haut à droite, HUD de la poursuite en haut au centre
inline Obstacles obstacles(float W, float H, const Marges& m, bool poursuite, bool gaucher) {
    Obstacles o;
    const float k = H / 720.f, u = H / 100.f;
    o.ajouter({m.gauche + 16 * k, m.haut + 14 * k, 200 * k, 112 * k});                       // temps (hud.cpp)
    if (!poursuite) o.ajouter({W - m.droite - 16 * k - 180 * k, m.haut + 14 * k, 180 * k, 92 * k});   // score, combo compris
    else {
        const float cx = (m.gauche + W - m.droite) / 2;                                       // HUD de la poursuite (poursuite_system.cpp)
        o.ajouter({cx - 20 * u, m.haut + 2 * u, 40 * u, 15.5f * u});
    }
    const float xr = bandeauxBordDroit(W, H, m), yb = bandeauxHaut(H, m);                     // pile des bandeaux au repos
    o.ajouter({0, yb, xr, H - yb});
    const Marges mc = gaucher ? Marges{m.droite, m.haut, m.gauche, m.bas} : m;               // = ecrans::margesCommandes
    auto miroir = [&](Cadre c) { if (gaucher) c.x = W - c.x - c.w; return c; };             // = ecrans::miroirX (cadre entier)
    const DispositionCommandes d = dispositionCommandes(W, H, mc);
    o.ajouter(miroir(d.gaz)); o.ajouter(miroir(d.frein)); o.ajouter(miroir(d.nitro));
    const BoutonsFleches f = boutonsFleches(W, H, mc);
    o.ajouter(miroir({f.xg, f.y, f.c, f.c})); o.ajouter(miroir({f.xd, f.y, f.c, f.c}));
    return o;
}

// carré de la carte : bord droit sur celui du score, haut sous le score (ou à 14 k en poursuite) ; le côté diminue jusqu'à
// laisser ECART × k avec chaque obstacle ; sous COTE_MIN × k : côté 0 (pas de carte)
inline Cadre placer(float W, float H, const Marges& m, const Obstacles& o, bool poursuite) {
    const float k = H / 720.f, e = ECART * k;
    const float droite = W - m.droite - GOUTTIERE * k, haut = m.haut + (poursuite ? HAUT_POURSUITE : HAUT) * k;
    float cote = std::min({COTE * k, droite - m.gauche, H - m.bas - haut});
    for (int i = 0; i < o.n; i++) {
        const Cadre& c = o.c[i];
        const float x0 = c.x - e, x1 = c.x + c.w + e, y0 = c.y - e, y1 = c.y + c.h + e;
        if (droite <= x0 || haut >= y1) continue;            // obstacle à droite ou au-dessus du coin haut droit : jamais touché
        cote = std::min(cote, std::max(droite - x1, y0 - haut));   // le carré reste à droite de l'obstacle, ou au-dessus
    }
    if (cote < COTE_MIN * k) return {droite, haut, 0, 0};
    return {droite - cote, haut, cote, cote};
}
inline bool recoupe(const Cadre& a, const Cadre& b, float ecart) {
    return a.w > 0 && b.w > 0 && a.x < b.x + b.w + ecart && b.x < a.x + a.w + ecart && a.y < b.y + b.h + ecart && b.y < a.y + a.h + ecart;
}

// ---- projection
// repère de la carte, en mètres dans le plan horizontal du monde (x, z) : origine, axe « haut » et axe « droite » unitaires
struct Repere { vec2 o, haut, droite; };
inline vec2 versCarte(const Repere& R, vec2 p) { const vec2 d = moins(p, R.o); return {scal(d, R.droite), scal(d, R.haut)}; }
// perpendiculaire à `haut` du côté droit de la route : sens = signe de vect(avant, droite) mesuré sur la piste (le plan du
// monde vu de dessus n'a pas d'orientation fixée par avance)
inline vec2 droiteDe(vec2 haut, float sens) { return sens >= 0 ? vec2{-haut.y, haut.x} : vec2{haut.y, -haut.x}; }

// GPS (RG-06) : écran = point du joueur dans le carré + (u, −v) × échelle ; 500 m = côté intérieur
struct VueGps { vec2 joueurPx; float pxParM = 1; };
inline VueGps vueGps(const Cadre& carre, float k) {
    const float in = carre.w - 2 * MARGE_INT * k;
    return {{carre.x + carre.w / 2, carre.y + MARGE_INT * k + JOUEUR_Y * in}, in / (float)(GPS_ARRIERE_M + GPS_AVANT_M)};
}
inline vec2 versEcranGps(const VueGps& v, vec2 uv) { return {v.joueurPx.x + uv.x * v.pxParM, v.joueurPx.y - uv.y * v.pxParM}; }

// parcours entier (RG-07) : boîte des points (u, v) mise à l'échelle uniforme dans le carré intérieur, centrée
struct Ajustement { float e = 1; vec2 centreUV, centrePx; };
inline Ajustement ajuster(const vec2* uv, int n, const Cadre& carre, float k) {
    Ajustement a;
    if (n <= 0) return a;
    float u0 = uv[0].x, u1 = u0, v0 = uv[0].y, v1 = v0;
    for (int i = 1; i < n; i++) { u0 = std::min(u0, uv[i].x); u1 = std::max(u1, uv[i].x); v0 = std::min(v0, uv[i].y); v1 = std::max(v1, uv[i].y); }
    const float in = carre.w - 2 * MARGE_INT * k, ext = std::max({u1 - u0, v1 - v0, 1e-3f});
    a.e = in / ext;
    a.centreUV = {(u0 + u1) / 2, (v0 + v1) / 2};
    a.centrePx = {carre.x + carre.w / 2, carre.y + carre.w / 2};
    return a;
}
inline vec2 versEcran(const Ajustement& a, vec2 uv) { return {a.centrePx.x + (uv.x - a.centreUV.x) * a.e, a.centrePx.y - (uv.y - a.centreUV.y) * a.e}; }

// ---- découpe d'un segment par un rectangle (Liang-Barsky) : false si le segment est dehors ; sinon a et b ramenés dedans
inline bool decouper(vec2& a, vec2& b, float x0, float y0, float x1, float y1) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    float t0 = 0, t1 = 1;
    const float p[4] = {-dx, dx, -dy, dy}, q[4] = {a.x - x0, x1 - a.x, a.y - y0, y1 - a.y};
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) { if (q[i] < 0) return false; continue; }
        const float r = q[i] / p[i];
        if (p[i] < 0) { if (r > t1) return false; t0 = std::max(t0, r); }
        else { if (r < t0) return false; t1 = std::min(t1, r); }
    }
    const vec2 a0 = a;
    a = {a0.x + t0 * dx, a0.y + t0 * dy};
    b = {a0.x + t1 * dx, a0.y + t1 * dy};
    return true;
}

// ---- abscisses (RG-07) : points du parcours à pas régulier de 0 à `fin` ; une abscisse hors de [0, fin] va au bout le plus proche
inline double ramener(double s, double fin) { return std::clamp(s, 0.0, std::max(fin, 0.0)); }
// position sur la polyligne p[0..n−1] (abscisses régulières 0 → fin) ; i : indice du segment qui contient s
inline vec2 surParcours(const vec2* p, int n, double s, double fin, int* i = nullptr) {
    if (n <= 0) return {};
    if (n == 1 || fin <= 0) { if (i) *i = 0; return p[0]; }
    const double f = ramener(s, fin) / fin * (n - 1);
    const int a = std::min((int)f, n - 2);
    if (i) *i = a;
    const float t = (float)(f - a);
    return plus(p[a], fois(moins(p[a + 1], p[a]), t));
}

} // namespace minicarte
} // namespace wfs
