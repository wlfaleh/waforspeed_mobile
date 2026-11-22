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
#include "hud.h"                          // WFS-245 : taille du panneau du temps (hud::TEMPS_L, TEMPS_H)
#include "toasts.h"                       // bandeauxBordDroit, bandeauxHaut

namespace wfs {

class Renderer;
class Game;
// banc d'essai (RG-11) : captures minicarte-checkpoint et minicarte-ligne (dans outDir), puis g.shutdown()
void benchMiniCarte(Game& g, Renderer& r, const std::string& outDir);
// WFS-596 : banc « carte-cockpit » : carte de l'écran central en vue Cockpit (Giulia, Model 3, SF90), vue arrière, suite en roulant
void benchCarteCockpit(Game& g, Renderer& r, const std::string& outDir);

namespace minicarte {

// ---- réglages du PO (regles-de-gestion/WFS-173.md), en pixels de référence 720 p (× k = H / 720) sauf mention
// WFS-557 (ordre de l'utilisateur du 2026-11-16 : maquettes R19 à 100 %, maquette 17) : panneau de guidage 189 × 149 (avant : carré
// de 150), bord droit sur celui du score (23 du bord), haut à 151 ; COTE = hauteur, largeur = COTE × LARGEUR_SUR_HAUT
constexpr float COTE = 149, COTE_MIN = 100, ECART = 8, LARGEUR_SUR_HAUT = 189.f / 149.f;   // RG-02 / RG-03
constexpr float GOUTTIERE = 23, HAUT = 151, HAUT_POURSUITE = 14, MARGE_INT = 10;
constexpr float ROUTE = 4;                                   // RG-05 : trait de la route
// WFS-557 (demande de l'utilisateur du 2026-11-16 : flèche orange de la carte un peu plus grande, maquette 17 : 16 × 14,5) : 18 × 18 (avant : 12 × 9)
constexpr float JOUEUR_L = 18, JOUEUR_W = 18, CONTOUR = 1.5f, DISQUE = 8, DAMIER = 10, CP_L = 16, CP_W = 3;
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

// ---- WFS-245 (RG-09, ANO-141 ; arbitrage du PO du 2026-11-07 22:05 UTC ; vérifié par tests/test_bandeau_gps.cpp) : textes du
// bandeau du prochain virage et du nom de rue, en px de référence (720 p, × k ; × côté / COTE sur un cadre plus petit)
constexpr float BANDEAU = 38;              // haut de la carte réservé au bandeau dans le calcul de la vue (inchangé : même carte)
constexpr float BANDEAU_DESSIN = 56, BANDEAU_MAX = 60;   // bandeau dessiné (avant : 38), au plus 60 ; liseré cyan de 2 en bas
constexpr float FLECHE = 28, FLECHE_X = 5, FLECHE_Y = 4;  // flèche de manœuvre 28 × 28 en (x + 5, y + 4)
constexpr float DISTANCE_X = 38, DISTANCE_MILIEU = 18;    // rangée 1 : distance à x + 38, capitales centrées à y + 18
constexpr float CONSIGNE_X = 4, CONSIGNE_HAUT = 37;       // rangée 2 : consigne de x + 4 à x + côté − 4, haut des lettres montantes à y + 37
constexpr float DISTANCE_PX = 32, DISTANCE_MIN = 28, CONSIGNE_PX = 20, CONSIGNE_MIN = 16;   // tailles (avant : 15 et 10)
constexpr float NOM = 18, NOM_PX = 16, NOM_MIN = 15, NOM_BAS = 3;   // nom de rue : étiquette de 18 à 3 du bas (avant : 13), taille 16 (avant : 8)
constexpr float AVANT_DISTANCE_PX = 15, AVANT_CONSIGNE_PX = 10, AVANT_NOM_PX = 8;
constexpr float MONTANTES = 0.551f, JAMBAGES = 0.22f;      // haut des « d » et des accents, bas des « p » et des « g » (× la taille)
// taille d'un texte : `px`, réduite par pas de 0,5 jusqu'à `mini` si `largeur(taille)` dépasse `lw` (au-delà : coupé avec « … »)
template <class Largeur> float tailleTexte(Largeur largeur, float px, float mini, float lw) {
    while (px > mini && largeur(px) > lw) px = std::max(mini, px - 0.5f);
    return px;
}

// ---- RG-02 / RG-03 : place de la carte
// cadres que la carte ne recouvre jamais (pixels de l'interface), écart de ECART × k compris par placer()
struct Obstacles { Cadre c[12]; int n = 0; void ajouter(const Cadre& x) { if (n < 12) c[n++] = x; } };
// gaucher : commandes en miroir (ecrans::miroirX et ecrans::margesCommandes de reglages.h, recopiés ici sans lire les
// réglages pour être testables) ; poursuite : pas de panneau du score en haut à droite, HUD de la poursuite en haut au centre
inline Obstacles obstacles(float W, float H, const Marges& m, bool poursuite, bool gaucher) {
    Obstacles o;
    const float k = H / 720.f, u = H / 100.f;
    o.ajouter({m.gauche + hud::R19_GOUTTIERE_L * k, m.haut + hud::R19_GOUTTIERE_H * k, hud::TEMPS_L * k, hud::TEMPS_H * k});   // temps (hud.cpp ; WFS-557 : 276 × 80)
    if (!poursuite) o.ajouter({W - m.droite - (hud::R19_GOUTTIERE_L + hud::SCORE_R19_L) * k, m.haut + hud::R19_GOUTTIERE_H * k, hud::SCORE_R19_L * k, hud::SCORE_R19_H_COMBO * k});   // score, combo compris
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

// panneau de la carte (WFS-557 : COTE de haut, jusqu'à COTE × LARGEUR_SUR_HAUT de large) : bord droit sur celui du score, haut
// sous le score (ou à 14 k en poursuite) ; la hauteur se calcule comme le carré d'avant (elle diminue jusqu'à laisser ECART × k
// avec chaque obstacle ; sous COTE_MIN × k : 0, pas de carte), puis la largeur s'étend vers la gauche jusqu'à COTE × LARGEUR_SUR_HAUT
// tant qu'aucun obstacle ne gêne (au moins le carré : jamais moins de place qu'avant)
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
    float w = std::min(cote * LARGEUR_SUR_HAUT, droite - m.gauche);
    for (int i = 0; i < o.n; i++) {
        const Cadre& c = o.c[i];
        const float x1 = c.x + c.w + e, y0 = c.y - e, y1 = c.y + c.h + e;
        if (y0 >= haut + cote || y1 <= haut || x1 <= droite - w || c.x - e >= droite) continue;   // hors de la bande du panneau
        w = std::max(cote, droite - x1);
    }
    return {droite - w, haut, w, cote};
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

// ---- WFS-596 : vue Cockpit : la même carte sur l'écran central de la planche (vehicule_cockpit.h : placeCarte, carteEcranVue)
// où dessiner la carte : au HUD (vues Poursuite, Capot, Proche ; cockpit commun sans intérieur par modèle, qui n'a pas d'écran),
// ou sur l'écran de la planche (vue Cockpit avec l'intérieur du modèle : RG-02, plus de carte par-dessus l'image)
enum class LieuCarte { Hud, Ecran };
inline LieuCarte lieuCarte(bool cockpit, bool interieur) { return cockpit && interieur ? LieuCarte::Ecran : LieuCarte::Hud; }
constexpr double ECRAN_HZ = 30;            // RG-07 : carte de l'écran recalculée au plus 30 fois par seconde (reprojetée à chaque image)
constexpr float ECRAN_NUIT = 0.60f;        // RG-03 : luminosité de l'écran la nuit (jour : 1) ; dalle de l'écran de nuit : 0,45 (vehicule_cockpit.cpp)
constexpr unsigned ECRAN_FOND = 0x0B0E14;  // RG-03 : fond d'écran de voiture : noir mat légèrement bleuté (pas de cyan)
constexpr unsigned ECRAN_ITINERAIRE = 0x3A86FF, ECRAN_GAINE = 0x3B4049, ECRAN_CHECKPOINT = 0xF2F4F8;   // itinéraire bleu (pas de cyan)
// cadre virtuel où la carte est dessinée avant d'être projetée : COTE de haut (u = 1 : mêmes tailles que le panneau du HUD), largeur
// aux proportions de l'écran réel (w × h en m)
inline Cadre cadreEcran(float w, float h) { return {0, 0, h > 0 ? COTE * w / h : COTE, COTE}; }

// point du monde → pixel de l'interface (même calcul que les étiquettes : fantome_etiquette.cpp) ; faux derrière la caméra
inline bool versUi(const mat4& vp, vec3 p, float W, float H, vec2& o) {
    const float cx = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3);
    const float cy = vp.at(1, 0) * p.x + vp.at(1, 1) * p.y + vp.at(1, 2) * p.z + vp.at(1, 3);
    const float cw = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
    if (cw < 0.05f) return false;
    o = {(cx / cw * 0.5f + 0.5f) * W, (cy / cw * 0.5f + 0.5f) * H};
    return true;
}
// projection d'un carré unité sur un quadrilatère quelconque (Heckbert, « Fundamentals of Texture Mapping », 1989) :
// (0, 0) → q[0], (1, 0) → q[1], (1, 1) → q[2], (0, 1) → q[3] ; X = (a u + b v + c) / (g u + h v + 1), Y = (d u + e v + f) / (…)
struct Homographie { float a = 1, b = 0, c = 0, d = 0, e = 1, f = 0, g = 0, h = 0; };
inline Homographie carreVers(const vec2 (&q)[4]) {
    Homographie H;
    const float dx1 = q[1].x - q[2].x, dx2 = q[3].x - q[2].x, dx3 = q[0].x - q[1].x + q[2].x - q[3].x;
    const float dy1 = q[1].y - q[2].y, dy2 = q[3].y - q[2].y, dy3 = q[0].y - q[1].y + q[2].y - q[3].y;
    const float det = dx1 * dy2 - dx2 * dy1;
    if ((std::fabs(dx3) < 1e-6f && std::fabs(dy3) < 1e-6f) || std::fabs(det) < 1e-12f) {   // parallélogramme : application affine
        H.g = H.h = 0;
    } else {
        H.g = (dx3 * dy2 - dx2 * dy3) / det;
        H.h = (dx1 * dy3 - dx3 * dy1) / det;
    }
    H.a = q[1].x - q[0].x + H.g * q[1].x; H.b = q[3].x - q[0].x + H.h * q[3].x; H.c = q[0].x;
    H.d = q[1].y - q[0].y + H.g * q[1].y; H.e = q[3].y - q[0].y + H.h * q[3].y; H.f = q[0].y;
    return H;
}
inline vec2 appliquer(const Homographie& H, vec2 uv) {
    const float w = H.g * uv.x + H.h * uv.y + 1;
    const float iw = std::fabs(w) > 1e-9f ? 1.f / w : 0.f;
    return {(H.a * uv.x + H.b * uv.y + H.c) * iw, (H.d * uv.x + H.e * uv.y + H.f) * iw};
}

} // namespace minicarte
} // namespace wfs
