// WaForSpeed — carte Corniche (plage), ticket WFS-14, épic E2.
// Port du décor « beach » de js/world/decor.js et de l'ambiance envFor (js/world/maps.js) :
//   - bas-côtés par zone (GROUND.beach) : côté mer promenade, sable, sable mouillé (corniche, village), quai (marina),
//     rochers (côte rocheuse) ; côté terre dunes puis collines ; au-delà, l'océan (plan d'eau #1e7fa8 de jour) ;
//   - palmiers (vegPalm, src/world/vegetation.cpp) des deux côtés, parasols et transats sur le sable ;
//   - ambiance de la plage : brouillard 950 m de jour (#cfe3f0), nuit #141428 à 640 m (WFS_MOMENT=nuit au banc d'essai).
// Couleurs unies (sol, parasols, mâts, transats, gaines, noix) : une seule palette (texture 32 × 1) → 1 maillage par
// morceau ; avec l'océan, le stipe et les palmes : 5 appels de dessin par morceau (≤ 300 au total, journal du banc).
// Placement des objets généré par la graine de la carte (mulberry32(seed · 31 + 7)), avec les pas du jeu web.
// WFS-139 : décor posé sur les sections du tracé 2 (WFS-136) : falaise des lacets (paroi de roche côté montagne, glissière
// et chute de la falaise côté mer, terrasses entre deux branches des lacets), tunnel taillé dans la roche (voûte brute,
// éclairage sodium renforcé à l'entrée), rond-point de la station (îlot, palmiers, ancre géante), feu tricolore et rue
// transversale du front de mer ; mer à altitude fixe ; rien sur la chaussée ni sur une autre branche de la route.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../render/eclairage.h"
#include "../render/renderer.h"
#include "enseignes.h"   // WFS-180 : vie::Baigneur (danseurs et joueurs pour les piétons), vie::hauteurSable
#include "vegetation.h"
#include "zones.h"

namespace wfs {

// ---------------------------------------------------------------- WFS-180 : lieux de la plage (fêtes, jeux)
// Partagés avec les piétons (foule qui danse, joueurs de beach-volley) et les serviettes (pas posées dessus).
namespace plage {
struct Lieu { int64_t seg; bool fete; };   // centre du lieu (segment, sur un tour) ; côté mer, sur le sable
struct Lieux { std::vector<Lieu> lieux; std::vector<vie::Baigneur> gens; bool fait = false; };
constexpr float FETE_LAT = (float)RW + 21;   // centre de la piste de danse (sable : RW + 5 à RW + 38)
constexpr float SCENE_LAT = (float)RW + 30;  // scène de 10 × 5 m, face à la route
constexpr float VOLLEY_LAT = (float)RW + 22; // terrain de 16 × 8 m (FIVB), long côté le long de la route
// segment de plage de sable, loin des virages serrés, de la falaise, du tunnel et des places (RG-C1)
static bool plageLibre(Track& t, const MapDef& map, int64_t len, int64_t n) {
    const char* z = zoneId(map, n, len);
    if (std::strcmp(z, "corniche") && std::strcmp(z, "village")) return false;
    const trace::Section* s = trace::sectionA(t, (double)(((n % len) + len) % len) * SEG);
    if (s && (t.pose(n).y > 1 || s->type == trace::Type::Lacets || s->type == trace::Type::Tunnel ||
              s->type == trace::Type::RondPoint || s->type == trace::Type::Feux)) return false;
    return trace::rayonInterieur(t, (double)n * SEG).droite > 150;   // côté mer : pas l'intérieur d'une courbe serrée
}
const Lieux& lieux(const MapDef& map, int64_t len) {
    static std::map<std::pair<uint32_t, int64_t>, Lieux> cache;
    Lieux& L = cache[{map.seed, len}];
    if (L.fait || std::strcmp(map.id, "plage")) return L;
    L.fait = true;
    Track t; t.build(map, trace::versionCarte(map.id));
    if ((int64_t)t.segmentCount() != len) return L;
    // lieux de 60 m (±25 segments) entièrement libres, espacés d'au moins 250 m : fête, jeux, fête, jeux, fête
    int nf = 0, nj = 0;
    for (int64_t n = 40; n < len - 40 && nf + nj < 5; n++) {
        bool ok = true;
        for (int64_t k = n - 25; k <= n + 25 && ok; k += 5) ok = plageLibre(t, map, len, k);
        if (!ok) continue;
        const bool fete = (nf + nj) % 2 == 0 ? nf < 3 : nj >= 2;
        L.lieux.push_back({n, fete}); (fete ? nf : nj)++;
        n += 210;
    }
    // foule (12 danseurs devant la scène, tournés vers elle) et joueurs (2 contre 2) ; Mulberry32 : mêmes places à chaque fois
    Mulberry32 D(map.seed * 31 + 37);
    for (const Lieu& l : L.lieux) {
        if (l.fete) for (int k = 0; k < 12; k++)
            L.gens.push_back({(double)l.seg + ((k % 6) - 2.5) * 1.6 / SEG_M + (D.next() - 0.5) * 0.6 / SEG_M,
                              FETE_LAT - 3 + (k / 6) * 2.2f + (float)D.next() * 1.2f, 1.5708f + ((float)D.next() - 0.5f) * 0.8f});
        else for (int k = 0; k < 4; k++)   // une équipe de chaque côté du filet (à ±4 m le long de la route)
            L.gens.push_back({(double)l.seg + (k < 2 ? -4.0 : 4.0) / SEG_M, VOLLEY_LAT + (k % 2 ? 2.f : -2.f), k < 2 ? 0.f : 3.1416f});
    }
    WFS_LOG("Plage (WFS-180) : %d fête(s), %d terrain(s) de jeux, %zu personnes", nf, nj, L.gens.size());
    return L;
}
// danseurs et joueurs : lus par les piétons (pietons.cpp)
const std::vector<vie::Baigneur>& gens(const MapDef& map, int64_t len) { return lieux(map, len).gens; }
// segment occupé par une fête ou des jeux (± 35 m) : les serviettes n'y sont pas posées (commerces.cpp)
bool occupe(const MapDef& map, int64_t len, int64_t n) {
    for (const Lieu& l : lieux(map, len).lieux) { int64_t d = ((n - l.seg) % len + len) % len; if (d > len / 2) d = len - d; if (d < 30) return true; }
    return false;
}
} // namespace plage

namespace {

constexpr float STIPE_K = 1.5f, STIPE_E = 0.8f;   // ANO-13 : gain d'albédo et part propre (émissive) du stipe, de jour
// P_NEON : boules des lampadaires (WFS-81) ; WFS-139 : P_ROC roche (texture), P_ACIER glissières, P_SODIUM luminaires du
// tunnel, P_ROUTE chaussée de la rue transversale du carrefour, P_VOUTE roche de la voûte (lumière du ciel masquée) ;
// WFS-180 : P_GUIRL ampoules des guirlandes, P_FILET filet de beach-volley (transparent)
enum Part { P_UNI, P_OCEAN, P_STIPE, P_PALME, P_SECHE, P_NEON, P_ROC, P_ACIER, P_SODIUM, P_ROUTE, P_VOUTE, P_GUIRL, P_FILET, P_COUNT };
constexpr int AHEAD = 22, BEHIND = 2;
constexpr double LOD_FAR_M = 200;     // au-delà : palmiers allégés, petits objets (parasols) non dessinés
constexpr float OCEAN_Y = -1.75f;     // niveau de la mer sous la route (le sable mouillé y plonge)
constexpr float OCEAN_W = 1400.f;     // largeur du plan d'eau (m)

// palette des couleurs unies (GCOL, GCOL2, C du jeu web) ; indice = case de la texture 32 × 1
enum Col { C_PROM, C_SABLE, C_MOUILLE, C_DUNE, C_COLLINE, C_ROCHER, C_QUAI, C_HERBE, C_MUR, C_TRANSAT, C_GAINE, C_NOIX,
           C_PARASOL0, C_POTEAU = C_PARASOL0 + 5, C_LAMPE,
           // WFS-94 : villas (C2.walls, C.roofs, glassDark), tour de sauveteur, bornes, bateaux (C2.hull, sail), rochers,
           // glissière, pins et arbres ronds (C.trunk, pine, leaf, leaf2) de decor.js
           C_MURV0, C_TOIT0 = C_MURV0 + 6, C_VITRE = C_TOIT0 + 3, C_TOUR, C_BORNE, C_COQUE0, C_VOILE = C_COQUE0 + 4, C_CABINE,
           C_MAT, C_ROC, C_ROC2, C_GLISS_P, C_GLISS, C_TRONC, C_PIN, C_FEUILLE, C_FEUILLE2,
           // WFS-139 : garrigue (maquis bas, albédo linéaire moyen 0,15 : bas de « herbe 0,15-0,25 »), caisson et mât du
           // feu (gris anthracite), ancre (acier peint), gazon
           C_GARRIGUE, C_CAISSON, C_ANCRE, C_GAZON,
           // WFS-180 : planches de la scène (bois teinté), toile noire, enceintes, mât de guirlande (pin), pierres et bûches du
           // feu, lignes du terrain (bleu FIVB), coque blanche (gelcoat), pont en teck, voile (dacron), vitres teintées, coques
           // des pédalos et du toboggan (jaune, bleu, rouge), sable mouillé du château, combinaison du pilote de jet-ski
           C_PLANCHE, C_TOILE, C_HP, C_MATBOIS, C_PIERRE, C_BUCHE, C_LIGNE, C_GELCOAT, C_TECK, C_DACRON, C_VITRE2,
           C_JAUNE, C_BLEU, C_ROUGE, C_SABLE_M, C_COMBI, C_BALLON, C_PEAU, C_NB };
const uint32_t PALETTE[C_NB] = {0xb8ab8c, 0xcdb683, 0xa38d60, 0x9aa65e, 0x7a8a4c, 0x7d7a70, 0x9a958a, 0x5f6a3a,
                                0xf2efe6, 0xf2efe6, veg::PALM_SHEATH, veg::PALM_COCO,
                                0xe2262b, 0xffd23a, 0x1d8fb8, 0xffffff, 0xff6a00,
                                0xe8e8e8, 0xffd9a8,   // WFS-81 : poteau « beach », lampWarm (decor.js)
                                0xf2e6c9, 0xe9c9a4, 0xf4dcd0, 0xd9e6ef, 0xf0e1a8, 0xe2d5c3, 0xb5532e, 0x2f5f8f, 0x6b6b6b, 0x1d2a38,
                                0xd63a2a, 0x2a2c30, 0xf2f2f0, 0x1d3f8f, 0xb8352a, 0x2f6f5f, 0xf7f5ee, 0xe9e6dc, 0xd8d8d8,
                                0x6f6b62, 0x8a857a, 0x8a8f96, 0xb8bec6, 0x6b4a2e, 0x2f5a2c, 0x3f6f2a, 0x557f34,
                                0x787850, 0x2a2d31, 0x30343a, 0x5b7a37,
                                0x6b5238, 0x161618, 0x202022, 0x8a6a48, 0x8c877c, 0x4a3426, 0x2f6fc2, 0xeeeeea, 0x9a7650, 0xf2f0ea,
                                0x24323c, 0xf2c21a, 0x1f63b8, 0xc8322a, 0xa8925f, 0x22262c, 0xf4e04a, 0xc69a76};
constexpr int PAL_W = 128;   // cases de la palette (≥ C_NB) ; WFS-180 : 64 → 128
static_assert(C_NB <= PAL_W, "palette trop petite");
inline vec2 palUv(int c) { return {(c + 0.5f) / PAL_W, 0.5f}; }

using Geo = veg::Geo;

struct Palm { int64_t seg; float lat, h, lean, rot; };          // lat : signé (> 0 côté mer, à droite)
struct Umbrella { int64_t seg; float lat, along; int col; };
struct Lampe { int64_t seg; int side; };                           // WFS-81 : lampadaire « beach » (decor.js, lamp)
// WFS-94 : objets des quartiers (DECOR.villa, tower, bollard, boat, rock, guard, tree de decor.js) ; lat non signé, side ±1
enum TypeObjet { O_VILLA, O_TOUR, O_BORNE, O_BATEAU, O_ROCHER, O_GLISSIERE, O_PIN, O_ARBRE };
struct Objet { int64_t seg; int t, side; float lat, w = 0, d = 0, h = 0, l = 0, rot = 0, along = 0; int col = 0, roof = 0, floors = 1; bool voile = false; };

// bande de sol : de a à b (m depuis l'axe, côté positif), hauteurs ya → yb, couleur (ROC : roche texturée, WFS-139)
struct Band { float a, b, ya, yb; int col; };
constexpr int ROC = -1;
// WFS-139 : nature du sol d'un segment, et forme d'un bas-côté
enum Sol : uint8_t { S_PLAGE, S_FALAISE, S_TUNNEL, S_PLACE };
enum Cote { K_PLAGE, K_PENTE, K_MER, K_TERRE, K_TUNNEL };

// GROUND.beach du jeu web, à partir du bord extérieur du trottoir de Game (RW + 4 m) ; côté s = +1 mer, −1 terre
int profil(const char* zone, int s, Band out[4]) {
    const float R = (float)RW, W0 = R + 4.f;
    int n = 0;
    if (s > 0) {
        // WFS-139 (RG-A, ANO-82) : après le mur du quai, l'eau du port (plus de bande de rochers sous les voiliers) ; le mur
        // descend sous la surface (OCEAN_Y = −1,75)
        if (!std::strcmp(zone, "marina")) { out[n++] = {W0, R + 7, 0.1f, 0.1f, C_QUAI}; out[n++] = {R + 7, R + 7.2f, 0.1f, -2.2f, C_QUAI}; }
        else if (!std::strcmp(zone, "rochers")) { out[n++] = {W0, R + 40, -0.18f, -1.8f, C_ROCHER}; out[n++] = {R + 40, R + 46, -1.8f, -2.4f, C_ROCHER}; }
        else { out[n++] = {W0, R + 5, 0.1f, 0.1f, C_PROM}; out[n++] = {R + 5, R + 38, 0.05f, -1.1f, C_SABLE}; out[n++] = {R + 38, R + 48, -1.1f, -1.9f, C_MOUILLE}; }
    } else {
        if (!std::strcmp(zone, "rochers")) { out[n++] = {W0, R + 40, 0.1f + (9 - 0.1f) * (1 / 37.f), 9, C_HERBE}; out[n++] = {R + 40, R + 170, 9, 45, C_ROCHER}; out[n++] = {R + 170, R + 264, 45, 52, C_ROCHER}; }
        else {
            const int c = !std::strcmp(zone, "village") || !std::strcmp(zone, "marina") ? C_PROM : C_DUNE;
            out[n++] = {W0, R + 30, 0.1f + 1.5f / 27.f, 1.6f, c}; out[n++] = {R + 30, R + 170, 1.6f, 30, C_COLLINE}; out[n++] = {R + 170, R + 264, 30, 36, C_COLLINE};
        }
    }
    return n;
}
// sandY du jeu web : hauteur du sable côté mer (corniche) à la distance lat
float sableY(float lat) {
    Band b[4] = {}; const int n = profil("corniche", 1, b);
    for (int i = 0; i < n; i++) if (lat >= b[i].a && lat <= b[i].b) return b[i].ya + (b[i].yb - b[i].ya) * (lat - b[i].a) / (b[i].b - b[i].a);
    return -1.9f;
}

// groundY du jeu web : hauteur du sol (depuis la route) à la distance lat du côté side, d'après le profil de la zone
float solY(const char* zone, int side, float lat) {
    Band b[4] = {}; const int n = profil(zone, side, b);
    if (lat < (float)RW + 4.f) return 0.15f;   // trottoir de Game
    for (int i = 0; i < n; i++) if (lat >= b[i].a && lat <= b[i].b) return b[i].ya + (b[i].yb - b[i].ya) * (lat - b[i].a) / (b[i].b - b[i].a);
    return n ? b[n - 1].yb : 0.f;
}

struct Frame { vec3 o, r, f; };
Frame frameAt(Track& t, double s) {
    TrackPoint p = t.point(s, 0);
    return {{(float)p.x, (float)p.y, (float)p.z}, {(float)std::cos(p.th), 0, (float)std::sin(p.th)}, {(float)std::sin(p.th), 0, (float)-std::cos(p.th)}};
}

void tri(Geo& g, vec3 a, vec3 b, vec3 c, vec2 uv) {
    const vec3 n = normalize(cross(b - a, c - a));
    const uint32_t k = (uint32_t)g.v.size();
    for (const vec3& p : {a, b, c}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2});
}
void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv) { tri(g, a, b, c, uv); tri(g, a, c, d, uv); }

// boîte orientée posée en c (axes r, f, demi-tailles hx, hz, hauteur h)
void box(Geo& g, vec3 c, vec3 r, vec3 f, float hx, float hz, float h, vec2 uv) {
    const vec3 up{0, 1, 0};
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    quad(g, P(-hx, 0, -hz), P(hx, 0, -hz), P(hx, h, -hz), P(-hx, h, -hz), uv);
    quad(g, P(hx, 0, -hz), P(hx, 0, hz), P(hx, h, hz), P(hx, h, -hz), uv);
    quad(g, P(hx, 0, hz), P(-hx, 0, hz), P(-hx, h, hz), P(hx, h, hz), uv);
    quad(g, P(-hx, 0, hz), P(-hx, 0, -hz), P(-hx, h, -hz), P(-hx, h, hz), uv);
    quad(g, P(-hx, h, -hz), P(hx, h, -hz), P(hx, h, hz), P(-hx, h, hz), uv);
}
// cylindre vertical, cône (parasol), boule (gaines, noix)
void cylinder(Geo& g, vec3 b, float rad, float h, int n, vec2 uv) {
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, b + d1 * rad, b + d0 * rad, b + d0 * rad + vec3{0, h, 0}, b + d1 * rad + vec3{0, h, 0}, uv);
    }
}
void cone(Geo& g, vec3 b, float rad, float h, int n, vec2 uv) {
    const vec3 top = b + vec3{0, h, 0};
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 p0 = b + vec3{std::cos(a0), 0, std::sin(a0)} * rad, p1 = b + vec3{std::cos(a1), 0, std::sin(a1)} * rad;
        tri(g, p1, p0, top, uv);   // dessus
        tri(g, p0, p1, b, uv);     // dessous (ombre du parasol vue d'en bas)
    }
}
void ball(Geo& g, vec3 c, vec3 s, vec2 uv) {
    const int nu = 8, nv = 5;
    auto P = [&](int i, int j) {
        const float u = 2 * PI * i / nu, v = PI * j / nv;
        return c + vec3{std::sin(v) * std::cos(u) * s.x, std::cos(v) * s.y, std::sin(v) * std::sin(u) * s.z};
    };
    for (int j = 0; j < nv; j++) for (int i = 0; i < nu; i++) {
        const vec3 a = P(i, j), b = P(i + 1, j), cc = P(i + 1, j + 1), d = P(i, j + 1);
        if (j > 0) tri(g, a, b, cc, uv);
        if (j < nv - 1) tri(g, a, cc, d, uv);
    }
}

vec3 lin(vec3 c) { return {std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f)}; }
vec3 linHex(uint32_t h) { return lin(veg::hex(h)); }

// ---------------------------------------------------------------- WFS-139 : falaise, tunnel, rond-point, feu
inline vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline vec2 operator*(vec2 a, float k) { return {a.x * k, a.y * k}; }
inline vec2 operator/(vec2 a, float k) { return {a.x / k, a.y / k}; }
inline float length(vec2 a) { return std::hypot(a.x, a.y); }
// quadrilatère avec une coordonnée de texture par sommet (roche : uv en mètres / ROC_TUILE)
void quadUv(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    auto t = [&](vec3 p, vec3 q, vec3 r, vec2 up, vec2 uq, vec2 ur) {
        const vec3 n = normalize(cross(q - p, r - p));
        const uint32_t k = (uint32_t)g.v.size();
        g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, up.x, up.y}); g.v.push_back({q.x, q.y, q.z, n.x, n.y, n.z, uq.x, uq.y});
        g.v.push_back({r.x, r.y, r.z, n.x, n.y, n.z, ur.x, ur.y});
        g.i.insert(g.i.end(), {k, k + 1, k + 2});
    };
    t(a, b, c, ua, ub, uc); t(a, c, d, ua, uc, ud);
}

constexpr float ROC_TUILE = 4.f;          // m couverts par une répétition de la texture de roche
constexpr float TUNNEL_W = (float)RW + 4.f;   // piédroits au bord extérieur des trottoirs de service (largeur roulable inchangée)
constexpr float TUNNEL_PIED = 3.2f, TUNNEL_H = 7.6f;   // piédroits, clé de voûte (gabarit ≥ 4,50 m + équipements, CETU)
constexpr float MASSIF_H = 16.f, MASSIF_L = 40.f;      // roche au-dessus du tunnel (m) et demi-largeur des têtes
constexpr float MASSIF_MONT = 14.f;       // le massif monte de 14 m de plus vers la montagne (côté gauche)
constexpr float VOISIN_M = 150.f;         // recherche d'une autre branche de la route sur chaque côté (m)
constexpr float GLISS_LAT = (float)RW + 4.3f;   // glissière au bord extérieur de l'accotement

// texture de roche calcaire (WFS-139). Les sites de textures libres (ambientCG, Poly Haven) sont bloqués par le réseau des
// agents : texture calculée ici (œuvre du projet, CC0), 256 × 256, répétable. Albédo moyen 0,45 en linéaire = « pierre
// claire 0,40-0,55 » (DA-realisme.md § 3), strates horizontales du calcaire, fissures sombres ; rugosité 0,85-1,0 ; normales
// tirées du relief. h : relief 0..1 (périodique) ; sorties RGBA 8 bits.
float bruitValeur(float x, float y, int periode, uint32_t graine) {
    auto h = [&](int i, int j) {
        uint32_t v = (uint32_t)(((i % periode) + periode) % periode) * 73856093u ^ (uint32_t)(((j % periode) + periode) % periode) * 19349663u ^ graine * 83492791u;
        v ^= v >> 13; v *= 0x5bd1e995u; v ^= v >> 15;
        return (v & 0xffffff) / float(0xffffff);
    };
    const int i = (int)std::floor(x), j = (int)std::floor(y);
    const float fx = x - i, fy = y - j, sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    const float a = h(i, j) + (h(i + 1, j) - h(i, j)) * sx, b = h(i, j + 1) + (h(i + 1, j + 1) - h(i, j + 1)) * sx;
    return a + (b - a) * sy;
}
void genererRoche(std::vector<uint8_t>& col, std::vector<uint8_t>& nrm, std::vector<uint8_t>& mr) {
    constexpr int N = 256;
    std::vector<float> h(N * N);
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        float v = 0, a = 0.5f; int p = 4;
        for (int o = 0; o < 5; o++, a *= 0.5f, p *= 2) v += a * bruitValeur(x * p / (float)N, y * p / (float)N, p, 11 + o);
        const float strate = 0.5f + 0.5f * std::sin((y / (float)N) * 2 * PI * 7 + v * 5);   // bancs du calcaire (v = le long du relief)
        h[y * N + x] = std::clamp(0.7f * v + 0.3f * strate * v * 1.6f, 0.f, 1.f);
    }
    col.assign(N * N * 4, 255); nrm.assign(N * N * 4, 255); mr.assign(N * N * 4, 255);
    auto srgb = [](float l) { l = std::clamp(l, 0.f, 1.f); return (uint8_t)std::lround(255 * (l <= 0.0031308f ? 12.92f * l : 1.055f * std::pow(l, 1 / 2.4f) - 0.055f)); };
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        const float v = h[y * N + x];
        const float fissure = std::clamp((v - 0.22f) * 6.f, 0.f, 1.f);   // creux sombres
        const float alb = 0.53f * (0.78f + 0.44f * v) * (0.6f + 0.4f * fissure);   // moyenne mesurée : 0,45 (test local)
        const int k = (y * N + x) * 4;
        col[k] = srgb(alb * 1.04f); col[k + 1] = srgb(alb * 0.99f); col[k + 2] = srgb(alb * 0.88f);   // calcaire beige clair
        const float dx = h[y * N + (x + 1) % N] - h[y * N + (x + N - 1) % N], dy = h[((y + 1) % N) * N + x] - h[((y + N - 1) % N) * N + x];
        const vec3 n = normalize(vec3{-dx * 6.f, -dy * 6.f, 1.f});
        nrm[k] = (uint8_t)std::lround(127.5f + 127.f * n.x); nrm[k + 1] = (uint8_t)std::lround(127.5f + 127.f * n.y); nrm[k + 2] = (uint8_t)std::lround(127.5f + 127.f * n.z);
        mr[k] = 255; mr[k + 1] = (uint8_t)std::lround(255 * (0.86f + 0.14f * (1 - v))); mr[k + 2] = 0;   // g : rugosité, b : métal (0)
    }
}

// ---------------------------------------------------------------- WFS-180 : bateaux (repère local : avant vers +Z)
// coque lissée : sections de la poupe (largeur 0,85 du maître-bau) à l'étrave (pointe), bordé jusqu'au franc-bord, fond
// en V jusqu'au tirant d'eau ; pont plat. L, B : longueur, maître-bau ; franc : franc-bord ; tirant : sous la flottaison
void coque(Geo& g, float L, float B, float franc, float tirant, vec2 uvBord, vec2 uvPont) {
    constexpr int N = 14;
    auto demi = [&](float z) {   // demi-largeur à l'abscisse z (de −L/2 à +L/2)
        const float z0 = -0.1f * L, h = B / 2;
        if (z <= z0) return h * (0.85f + 0.15f * (z + L / 2) / (z0 + L / 2));
        const float u = (z - z0) / (L / 2 - z0);
        return h * std::sqrt(std::max(0.f, 1 - u * u));
    };
    for (int i = 0; i < N; i++) {
        const float za = -L / 2 + L * i / N, zb = -L / 2 + L * (i + 1) / N, wa = demi(za), wb = demi(zb);
        const float fa = franc + 0.15f * franc * (float)i / N, fb = franc + 0.15f * franc * (float)(i + 1) / N;   // tonture
        for (float sd : {-1.f, 1.f}) {
            quad(g, {sd * wa, fa, za}, {sd * wb, fb, zb}, {sd * wb * 0.8f, 0, zb}, {sd * wa * 0.8f, 0, za}, uvBord);
            quad(g, {sd * wa * 0.8f, 0, za}, {sd * wb * 0.8f, 0, zb}, {0, -tirant, zb}, {0, -tirant, za}, uvBord);
            quad(g, {0, fa, za}, {0, fb, zb}, {sd * wb, fb, zb}, {sd * wa, fa, za}, uvPont);
        }
    }
    const float w = demi(-L / 2);   // tableau arrière
    quad(g, {-w, franc, -L / 2}, {w, franc, -L / 2}, {w * 0.8f, 0, -L / 2}, {-w * 0.8f, 0, -L / 2}, uvBord);
    tri(g, {-w * 0.8f, 0, -L / 2}, {w * 0.8f, 0, -L / 2}, {0, -tirant, -L / 2}, uvBord);
}
// voilier de croisière de 10 m (Bénéteau Oceanis 30.1 : 9,6 m, bau 3,4 m ; mât ≈ 14 m au-dessus de l'eau), voiles hissées
void voilier(Geo& g) {
    coque(g, 10.f, 3.4f, 1.1f, 0.6f, palUv(C_GELCOAT), palUv(C_TECK));
    box(g, {0, 1.1f, -0.8f}, {1, 0, 0}, {0, 0, 1}, 1.1f, 1.9f, 0.6f, palUv(C_GELCOAT));   // rouf
    box(g, {0, 1.25f, 0.35f}, {1, 0, 0}, {0, 0, 1}, 1.12f, 0.6f, 0.25f, palUv(C_VITRE2));  // hublots
    cylinder(g, {0, 1.1f, 0.9f}, 0.07f, 13.f, 6, palUv(C_MAT));
    tri(g, {0, 2.1f, 0.85f}, {0, 13.6f, 0.85f}, {0, 2.1f, -3.0f}, palUv(C_DACRON));       // grand-voile
    tri(g, {0, 1.3f, 4.6f}, {0, 12.2f, 0.95f}, {0.2f, 1.6f, 1.2f}, palUv(C_DACRON));      // génois
}
// yacht à moteur de 18 m (bau 4,9 m, deux ponts)
void yacht(Geo& g) {
    coque(g, 18.f, 4.9f, 1.9f, 0.9f, palUv(C_GELCOAT), palUv(C_TECK));
    box(g, {0, 1.9f, -1.5f}, {1, 0, 0}, {0, 0, 1}, 2.1f, 5.f, 2.2f, palUv(C_GELCOAT));
    box(g, {0, 2.7f, 0.5f}, {1, 0, 0}, {0, 0, 1}, 2.13f, 2.6f, 0.9f, palUv(C_VITRE2));     // pare-brise et vitres
    box(g, {0, 4.1f, -2.4f}, {1, 0, 0}, {0, 0, 1}, 1.6f, 2.8f, 1.6f, palUv(C_GELCOAT));
    box(g, {0, 4.6f, -1.4f}, {1, 0, 0}, {0, 0, 1}, 1.62f, 1.4f, 0.7f, palUv(C_VITRE2));
}
// jet-ski (Sea-Doo GTI : 3,3 × 1,2 m, 1,1 m de haut) et son pilote (combinaison)
void jetski(Geo& g) {
    coque(g, 3.3f, 1.2f, 0.45f, 0.25f, palUv(C_GELCOAT), palUv(C_BLEU));
    box(g, {0, 0.45f, -0.4f}, {1, 0, 0}, {0, 0, 1}, 0.28f, 0.7f, 0.32f, palUv(C_COMBI));   // selle
    box(g, {0, 0.45f, 0.55f}, {1, 0, 0}, {0, 0, 1}, 0.4f, 0.35f, 0.35f, palUv(C_BLEU));    // capot
    cylinder(g, {0, 0.77f, -0.35f}, 0.2f, 0.62f, 8, palUv(C_COMBI));                       // pilote (1,75 m, accroupi)
    ball(g, {0, 1.55f, -0.25f}, {0.12f, 0.14f, 0.13f}, palUv(C_PEAU));
    box(g, {0, 0.9f, 0.6f}, {1, 0, 0}, {0, 0, 1}, 0.35f, 0.04f, 0.04f, palUv(C_CAISSON));  // guidon
}
// paquebot (≈ 290 m, 32 m de large, 60 m au-dessus de l'eau), au large
void paquebot(Geo& g) {
    coque(g, 290.f, 32.f, 14.f, 8.f, palUv(C_GELCOAT), palUv(C_TECK));
    for (int k = 0; k < 5; k++) box(g, {0, 14.f + k * 9.f, -20.f - k * 6.f}, {1, 0, 0}, {0, 0, 1}, 15.5f - k, 110.f - k * 14.f, 9.f, palUv(k % 2 ? C_VITRE2 : C_GELCOAT));
    box(g, {0, 59.f, -60.f}, {1, 0, 0}, {0, 0, 1}, 4.f, 8.f, 9.f, palUv(C_ROUGE));          // cheminée
}
// sillage d'un jet-ski : écume en V, 18 m derrière (repère local, posé sur l'eau)
void sillage(Geo& g) {
    quad(g, {-0.6f, 0.03f, -1.6f}, {0.6f, 0.03f, -1.6f}, {4.5f, 0.03f, -19.f}, {-4.5f, 0.03f, -19.f}, {0, 0});
}

// banc d'essai : vue de dessus de la Corniche (décor compris), réglée par benchCorniche
struct VueDessus { bool actif = false; vec3 oeil{}, cible{}, haut{0, 0, -1}; float fov = 50; } g_vueDessus;

class Plage : public System {
public:
    const char* name() const override { return "plage"; }

    void init(Renderer& r, const GameView& g) override {
        active_ = g.map && std::strcmp(g.map->id, "plage") == 0;
        if (!active_) return;
        const char* m = std::getenv("WFS_MOMENT");   // banc d'essai : WFS_MOMENT=nuit (choix à l'écran : WFS-41)
        night_ = m && std::strcmp(m, "nuit") == 0;
        // palette des couleurs unies (valeurs d'affichage du jeu web : texture sRGB)
        uint8_t px[PAL_W * 4] = {};
        for (int c = 0; c < C_NB; c++) {
            const uint32_t h = PALETTE[c];
            px[c * 4] = (h >> 16) & 255; px[c * 4 + 1] = (h >> 8) & 255; px[c * 4 + 2] = h & 255; px[c * 4 + 3] = 255;
        }
        palTex_ = r.vk().uploadTexture(px, PAL_W, 1, true);
        Material mu; mu.rough = 0.85f; mu.flags = MAT_DOUBLE;   // petits objets vus de tous les côtés
        mats_[P_UNI] = r.makeMaterial(mu, &palTex_, nullptr, nullptr);
        Material mo; const vec3 w = linHex(night_ ? 0x0a1828 : 0x1e7fa8);   // ENV.water
        mo.color = {w.x, w.y, w.z, 1}; mo.rough = 0.12f; mo.flags = MAT_DOUBLE;
        mats_[P_OCEAN] = r.makeMaterial(mo, nullptr, nullptr, nullptr);
        // WFS-81 : boules des lampadaires, part propre (neonK du jeu web : allumées la nuit, 0,5 de jour, comme CarteDecor)
        Material mn; mn.rough = 0.4f; mn.flags = MAT_DOUBLE; mn.emissive = linHex(0xffd9a8) * (night_ ? 2.f : 0.5f);
        mats_[P_NEON] = r.makeMaterial(mn, &palTex_, nullptr, nullptr);
        initVegetation(r);
        initFalaise(r);
        basse_ = r.qualiteBasse;
        len_ = (int64_t)g.track->segmentCount();
        preparer(*g.track);
        { const char* dep = std::getenv("WFS_DEPART"); depart_ = dep ? std::atoll(dep) : 0; }   // WFS-94 : banc d'essai
        objets_.clear();
        place(*g.track);
        initPlage(r, *g.track, *g.map);
    }

    // stipe (2 texels : BARK.palm / palm2), palmes (atlas du jeu web, case 2) vertes et sèches
    void initVegetation(Renderer& r) {
        std::vector<uint8_t> png; int w = 0, h = 0, n = 0;
        stbi_uc* px = readAsset("world/feuillage.png", png) ? stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4) : nullptr;
        if (px) { leafTex_ = r.vk().uploadTexture(px, (uint32_t)w, (uint32_t)h, true); stbi_image_free(px); }
        else WFS_ERR("Atlas de feuillage introuvable (world/feuillage.png)");
        auto leafMat = [&](vec3 tint) {
            Material m; const vec3 c = lin(tint); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.8f; m.cutoff = 0.5f;
            m.flags = MAT_MASK | MAT_DOUBLE | MAT_VTINT;
            return r.makeMaterial(m, leafTex_.img ? &leafTex_ : nullptr, nullptr, nullptr);
        };
        mats_[P_PALME] = leafMat(veg::TINT_PALM);
        mats_[P_SECHE] = leafMat(veg::hex(veg::PALM_DEAD) * 1.4f);
        uint8_t bark[8];
        for (int k = 0; k < 2; k++) {
            const vec3 c = veg::hex(k ? veg::BARK_PALM2 : veg::BARK_PALM);
            bark[k * 4] = (uint8_t)(c.x * 255 + 0.5f); bark[k * 4 + 1] = (uint8_t)(c.y * 255 + 0.5f); bark[k * 4 + 2] = (uint8_t)(c.z * 255 + 0.5f); bark[k * 4 + 3] = 255;
        }
        barkTex_ = r.vk().uploadTexture(bark, 2, 1, true);
        // ANO-13 : le jeu web (three r128 sans encodage de sortie) éclaire les couleurs de sommet brutes ; ici la texture
        // sRGB est linéarisée, et le stipe, presque toujours à contre-jour, sortait gris anthracite.
        Material mb; mb.rough = 0.9f; mb.flags = MAT_VTINT; mb.color = {STIPE_K, STIPE_K, STIPE_K, 1};
        // part propre brun BARK.palm : garde la teinte du jeu web à contre-jour (presque éteinte la nuit)
        mb.emissive = lin(veg::hex(veg::BARK_PALM)) * (night_ ? 0.05f : STIPE_E);
        mats_[P_STIPE] = r.makeMaterial(mb, &barkTex_, nullptr, nullptr);
    }

    // WFS-139 : matériaux de la falaise et du tunnel
    void initFalaise(Renderer& r) {
        std::vector<uint8_t> c, n, m;
        genererRoche(c, n, m);
        rocTex_ = r.vk().uploadTexture(c.data(), 256, 256, true);
        rocNrm_ = r.vk().uploadTexture(n.data(), 256, 256, false);
        rocMr_ = r.vk().uploadTexture(m.data(), 256, 256, false);
        Material mr; mr.rough = 1.f; mr.flags = MAT_DOUBLE;   // rugosité × texture (0,86-1,0), roche non métallique
        mats_[P_ROC] = r.makeMaterial(mr, &rocTex_, &rocNrm_, &rocMr_);
        // voûte : le moteur n'a ni occultation ambiante ni éclairage d'environnement (DA-realisme.md § 1) ; sous 200 m de
        // roche, la lumière du ciel n'arrive pas : couleur × 0,3 pour ne pas éclairer la voûte comme dehors
        mr.color = {0.3f, 0.3f, 0.3f, 1};
        mats_[P_VOUTE] = r.makeMaterial(mr, &rocTex_, &rocNrm_, &rocMr_);
        // acier galvanisé des glissières : métal, albédo ≈ 0,56 (un peu sous l'inox de references/reel/materiaux.json,
        // mat.acier_inox 0,64 : zinc terni), rugosité 0,45
        Material ma; ma.color = {0.56f, 0.57f, 0.58f, 1}; ma.metal = 1; ma.rough = 0.45f; ma.flags = MAT_DOUBLE;
        mats_[P_ACIER] = r.makeMaterial(ma, nullptr, nullptr, nullptr);
        // luminaires du tunnel : sodium haute pression ≈ 2000 K (DA-realisme.md : 1900-2200 K), allumés jour et nuit
        Material ms; ms.color = {0.2f, 0.2f, 0.2f, 1}; ms.rough = 0.5f; ms.flags = MAT_DOUBLE;
        ms.emissive = vec3{1.f, 0.42f, 0.09f} * (night_ ? 3.f : 5.f);
        mats_[P_SODIUM] = r.makeMaterial(ms, nullptr, nullptr, nullptr);
        Material mo; mo.flags = MAT_ROAD; mo.rough = 0.8f;   // même surface procédurale que la chaussée de Game
        mats_[P_ROUTE] = r.makeMaterial(mo, nullptr, nullptr, nullptr);
        // lentilles du feu tricolore : éteinte (verre sombre), rouge, orange, vert (LED, allumées)
        Material ml; ml.color = {0.03f, 0.03f, 0.03f, 1}; ml.rough = 0.2f; ml.flags = MAT_DOUBLE;
        lentilleEteinte_ = r.makeMaterial(ml, nullptr, nullptr, nullptr);
        const vec3 couleurs[3] = {{1.f, 0.04f, 0.02f}, {1.f, 0.42f, 0.01f}, {0.05f, 1.f, 0.45f}};
        for (int k = 0; k < 3; k++) { ml.emissive = couleurs[k] * 6.f; lentille_[k] = r.makeMaterial(ml, nullptr, nullptr, nullptr); }
        // WFS-180 : ampoules des guirlandes (blanc chaud ≈ 2700 K, éteintes le jour : 0,3), filet (mailles noires : 45 % couvrant)
        Material mg; mg.color = {0.9f, 0.85f, 0.7f, 1}; mg.rough = 0.3f; mg.flags = MAT_DOUBLE;
        mg.emissive = vec3{1.f, 0.66f, 0.36f} * (night_ ? 4.f : 0.3f);
        mats_[P_GUIRL] = r.makeMaterial(mg, nullptr, nullptr, nullptr);
        Material mf; mf.color = {0.03f, 0.03f, 0.03f, 0.45f}; mf.rough = 0.8f; mf.flags = MAT_BLEND | MAT_DOUBLE;
        mats_[P_FILET] = r.makeMaterial(mf, nullptr, nullptr, nullptr);
        // gelcoat des coques (polyester : diélectrique lisse, rugosité 0,25), mêmes couleurs que la palette
        Material mc; mc.rough = 0.3f; mc.flags = MAT_DOUBLE;
        coqueMat_ = r.makeMaterial(mc, &palTex_, nullptr, nullptr);
        // sillage : écume blanche (albédo ≈ 0,8), à moitié transparente
        Material mw; mw.color = {0.8f, 0.82f, 0.84f, 0.55f}; mw.rough = 0.9f; mw.flags = MAT_BLEND | MAT_DOUBLE;
        sillageMat_ = r.makeMaterial(mw, nullptr, nullptr, nullptr);
        // faisceaux des lyres de la scène (la nuit : lumière ajoutée, couleur réglée par image), flamme du feu de camp
        for (int k = 0; k < 3; k++) {
            const vec3 c[3] = {{0.9f, 0.1f, 0.6f}, {0.1f, 0.4f, 1.f}, {0.2f, 1.f, 0.5f}};
            Material mb; mb.color = {c[k].x * 0.12f, c[k].y * 0.12f, c[k].z * 0.12f, 1}; mb.flags = MAT_ADD | MAT_BLEND | MAT_DOUBLE;
            faisceauMat_[k] = r.makeMaterial(mb, nullptr, nullptr, nullptr);
            Material ml2; ml2.color = {0.1f, 0.1f, 0.1f, 1}; ml2.emissive = c[k] * 5.f; ml2.flags = MAT_DOUBLE;
            lyreMat_[k] = r.makeMaterial(ml2, nullptr, nullptr, nullptr);
        }
        Material mfl; mfl.color = {0.1f, 0.05f, 0.f, 1}; mfl.emissive = vec3{1.f, 0.45f, 0.08f} * 6.f; mfl.flags = MAT_DOUBLE;   // ≈ 1800 K
        flammeMat_ = r.makeMaterial(mfl, nullptr, nullptr, nullptr);
    }

    int64_t mod(int64_t n) const { return ((n % len_) + len_) % len_; }
    int ix(int64_t n, int side) const { return (int)(mod(n) * 2 + (side > 0 ? 1 : 0)); }

    // WFS-139 (RG-C1) : analyse du tracé 2 sur un tour. Pour chaque segment et chaque côté, distance latérale à laquelle
    // on touche une AUTRE branche de la route (lacets, rond-point) et son altitude ; nature du sol (plage, falaise,
    // tunnel, place) ; zones à garder dégagées avant les virages serrés et le feu.
    void preparer(Track& t) {
        const int64_t L = len_;
        sol_.assign((size_t)L, S_PLAGE); cache_.assign((size_t)L, 0);
        touche_.assign((size_t)L * 2, 1e9f); autreY_.assign((size_t)L * 2, 0.f);
        feux_.clear(); tunDeb_ = tunFin_ = rpSeg_ = rpDeb_ = rpFin_ = -1;
        const auto& S = trace::sections(t);
        if (S.empty()) return;   // tracé 1 : décor d'avant, inchangé
        // centres de la route (tour 1, avec la fin du tour 0 et le début du tour 2) dans une grille de 16 m
        const int64_t a0 = L - 450, a1 = 2 * L + 450;
        std::vector<vec3> c((size_t)(a1 - a0));
        for (int64_t a = a0; a < a1; a++) { const Pose& p = t.pose(a); c[(size_t)(a - a0)] = {(float)p.x, (float)p.y, (float)p.z}; }
        constexpr float CELL = 16;
        auto cle = [](int64_t i, int64_t j) { return (i << 32) ^ (j & 0xffffffff); };
        std::unordered_map<int64_t, std::vector<int32_t>> grille;
        for (size_t i = 0; i < c.size(); i++) grille[cle((int64_t)std::floor(c[i].x / CELL), (int64_t)std::floor(c[i].z / CELL))].push_back((int32_t)i);
        const float R = (float)RW, seuil = (R + 1) * (R + 1);
        // une autre branche à moins de RW + 1 m du point q ? (y : son altitude) — les segments voisins de n sont ignorés
        auto surRoute = [&](vec3 q, int64_t n, float& y) {
            const int64_t ci = (int64_t)std::floor(q.x / CELL), cj = (int64_t)std::floor(q.z / CELL);
            for (int64_t di = -1; di <= 1; di++) for (int64_t dj = -1; dj <= 1; dj++) {
                auto f = grille.find(cle(ci + di, cj + dj));
                if (f == grille.end()) continue;
                for (int32_t i : f->second) {
                    if (std::abs(a0 + i - n) < 30) continue;   // la route elle-même (±36 m)
                    const float dx = q.x - c[(size_t)i].x, dz = q.z - c[(size_t)i].z;
                    if (dx * dx + dz * dz < seuil) { y = c[(size_t)i].y; return true; }
                }
            }
            return false;
        };
        for (int64_t n = 0; n < L; n++) {
            const Pose& p = t.pose(n + L);
            const vec3 o{(float)p.x, (float)p.y, (float)p.z}, rr{(float)std::cos(p.th), 0, (float)std::sin(p.th)};
            for (int side : {-1, 1})
                for (float lat = R + 1.5f; lat <= VOISIN_M; lat += 2) {
                    float y = 0;
                    if (!surRoute(o + rr * (side * lat), n + L, y)) continue;
                    touche_[(size_t)ix(n, side)] = lat; autreY_[(size_t)ix(n, side)] = y;
                    break;
                }
        }
        // nature du sol : falaise = lacets, ou route à plus de 3 m au-dessus de la mer ; tunnel ; place (rond-point, carrefour)
        for (int64_t n = 0; n < L; n++) if (t.pose(n).y > 3.0) sol_[(size_t)n] = S_FALAISE;
        int serres = 0;
        for (const trace::Section& s : S) {
            if (s.type == trace::Type::Lacets) for (int64_t n = s.debut; n < s.fin; n++) sol_[(size_t)mod(n)] = S_FALAISE;
            if (s.type == trace::Type::Tunnel) { tunDeb_ = s.debut; tunFin_ = s.fin; for (int64_t n = s.debut; n < s.fin; n++) sol_[(size_t)mod(n)] = S_TUNNEL; }
            if (s.type == trace::Type::RondPoint) {
                for (int64_t n = s.debut - 15; n < s.fin + 15; n++) sol_[(size_t)mod(n)] = S_PLACE;
                rpDeb_ = s.debut; rpFin_ = s.fin;
                double cmax = 0;   // milieu de l'anneau : courbure la plus forte (rayon 14 m)
                for (int64_t n = s.debut; n < s.fin; n++) if (std::abs(t.segAt(n).curve) > cmax) { cmax = std::abs(t.segAt(n).curve); rpSeg_ = n; }
                int64_t f = rpSeg_; while (f < s.fin && std::abs(t.segAt(f).curve) > cmax * 0.99) f++;
                rpSeg_ = (rpSeg_ + f) / 2;
            }
            if (s.type == trace::Type::Feux && !s.depart) {
                const int64_t croise = s.ligneArret + (int64_t)std::lround(trace::CARREFOUR_M / SEG_M);
                feux_.push_back({s.ligneArret, croise, s.feu});
                for (int64_t n = croise - 17; n <= croise + 17; n++) sol_[(size_t)mod(n)] = S_PLACE;   // ±20 m : terre-plein
                for (int64_t n = s.ligneArret - 50; n < s.ligneArret + 10; n++) cache_[(size_t)mod(n)] |= 3;   // 60 m avant le feu
            }
            const bool serre = s.type == trace::Type::Epingle || s.type == trace::Type::Lacets || s.type == trace::Type::RondPoint ||
                               s.type == trace::Type::Chicane || (s.type == trace::Type::Virage && s.rayon <= 100);
            if (!serre) continue;
            serres++;
            const uint8_t bits = (uint8_t)(s.type == trace::Type::Virage || s.type == trace::Type::Epingle ? (s.angle > 0 ? 2 : 1) : 3);
            for (int64_t n = s.debut - 50; n < s.fin; n++) cache_[(size_t)mod(n)] |= bits;   // 60 m avant, et dans le virage
        }
        // éclairage de nuit du tunnel (nuit.cpp : les 4 plus proches) : un point toutes les 15 m, en quinconce, à plus de 25 m
        // des têtes (ces lumières ne sont pas masquées par la roche : près d'une tête, elles éclairaient sa face extérieure)
        if (tunDeb_ >= 0) for (int64_t n = tunDeb_ + 21, k = 0; n < tunFin_ - 21; n += 12, k++)
            lampadaires().push_back({mod(n), (k % 2 ? 1.f : -1.f) * (TUNNEL_W - 1.5f), TUNNEL_H - 1.6f, StyleLampadaire::Ville});
        int voisins = 0; for (float v : touche_) voisins += v < 1e8f;
        WFS_LOG("Plage (WFS-139) : %zu sections, %d virages serrés, %d bas-côtés bornés par une autre branche, tunnel %lld-%lld, "
                "rond-point au segment %lld, %zu feu(x)", S.size(), serres, voisins, (long long)tunDeb_, (long long)tunFin_,
                (long long)rpSeg_, feux_.size());
    }

    // bord à ne pas dépasser (m) du côté side au segment absolu a : 90 % du rayon intérieur des courbes (le sol retomberait
    // sur la route), et le milieu entre cette route et une autre branche voisine (chaque branche pose son sol jusque-là)
    float bordSol(Track& t, int64_t a, int side) const {
        float lim = 1e9f;
        const trace::Rayon ri = trace::rayonInterieur(t, (double)a * SEG);
        const double rs = side > 0 ? ri.droite : ri.gauche;
        if (rs < 1e8) lim = (float)(0.9 * rs);
        const float tc = touche_.empty() ? 1e9f : touche_[(size_t)ix(a, side)];
        if (tc < 1e8f) lim = std::min(lim, (tc + (float)RW + 1) / 2);
        return lim;
    }
    // objets : jamais à moins d'1 m d'une autre chaussée (≥ RW + 1 m, RG-C1), ni au-delà du rayon intérieur ;
    // vérifié sur toute la longueur de l'objet (±demi-longueur)
    bool placeLibre(Track& t, int64_t n, int side, float lat, float ext, float demiLong = 2.f) const {
        if (touche_.empty()) return true;
        const int64_t w = (int64_t)std::ceil(demiLong / SEG_M);
        for (int64_t k = n - w; k <= n + w; k += std::max<int64_t>(1, w / 3)) {
            if (lat + ext >= touche_[(size_t)ix(k, side)]) return false;
            const trace::Rayon ri = trace::rayonInterieur(t, (double)k * SEG);
            const double rs = side > 0 ? ri.droite : ri.gauche;
            if (rs < 1e8 && lat + ext > 0.9 * rs) return false;
        }
        return true;
    }
    // objet haut (immeuble, palmier, tour) : pas du côté intérieur d'un virage serré ni près du feu dans les 60 m avant
    bool cacheVue(int64_t n, int side, float lat) const {
        return !cache_.empty() && lat < 60 && (cache_[(size_t)mod(n)] & (side < 0 ? 1 : 2));
    }
    bool anneau(int64_t n) const { return rpDeb_ >= 0 && mod(n - rpDeb_) < rpFin_ - rpDeb_ + 15; }
    int solA(int64_t n) const { return sol_.empty() ? (int)S_PLAGE : (int)sol_[(size_t)mod(n)]; }

    // placement (generateDecor, beach) : pas du jeu web pour chaque zone, tirages de la graine de la carte
    void place(Track& t) {
        const MapDef& map = *t.map();
        const int64_t len = (int64_t)t.segmentCount();
        Mulberry32 D(map.seed * 31 + 7);
        auto d = [&]() { return (float)D.next(); };
        const float R = (float)RW;
        // plages [z0, z1) de même zone
        std::vector<std::pair<int64_t, int64_t>> ranges;
        for (int64_t n = 0; n < len; n++) if (n == 0 || zoneIndex(map, n, len) != zoneIndex(map, n - 1, len)) ranges.push_back({n, len});
        for (size_t i = 0; i + 1 < ranges.size(); i++) ranges[i].second = ranges[i + 1].first;
        for (auto [z0, z1] : ranges) {
            const char* z = zoneId(map, z0, len);
            const bool sable = !std::strcmp(z, "corniche") || !std::strcmp(z, "village");
            if (sable) {   // corniche (et village, même plage) : palmiers et parasols côté mer
                for (int64_t n = z0 + 6; n < z1; n += 9 + (int64_t)(d() * 4)) palms_.push_back({n, R + 3.2f, 7 + d() * 4, (d() - 0.5f) * 0.5f, d() * 6});
                for (int64_t n = z0 + 3; n < z1; n += 7 + (int64_t)(d() * 6)) { const float lat = R + 12 + d() * 20; const int col = (int)(d() * 5); umbrellas_.push_back({n, lat, (d() - 0.5f) * 4, col}); }
            }
            // côté terre : palmiers sur la dune (toutes les zones sauf la côte rocheuse, plus clairsemée)
            const int pas = !std::strcmp(z, "rochers") ? 30 : 14;
            for (int64_t n = z0 + 9; n < z1; n += pas + (int64_t)(d() * 8)) palms_.push_back({n, -(R + 5 + d() * 3), 6 + d() * 4, (d() - 0.5f) * 0.4f, d() * 6});
            // WFS-81 : lampadaires « beach », pas et côté de chaque zone (decor.js : corniche, marina, rochers, village)
            int64_t l0 = 15, lp = 30; int ls = 1;
            if (!std::strcmp(z, "marina")) lp = 28; else if (!std::strcmp(z, "rochers")) { l0 = 20; lp = 40; } else if (!std::strcmp(z, "village")) { l0 = 12; lp = 26; ls = -1; }
            for (int64_t n = z0 + l0; n < z1; n += lp) lamps_.push_back({n % len, ls});
        }
        placeQuartiers(map, ranges);
        filtrer(t);
        // éclairage de nuit (src/render/nuit.cpp) : lumière du jeu web (scene.js) à RW + 0,6, 5,6 m, côté du poteau
        for (const Lampe& l : lamps_) lampadaires().push_back({l.seg, l.side * (R + 0.6f), 5.6f, StyleLampadaire::Plage});
        WFS_LOG("Plage : %zu palmiers, %zu parasols, %zu lampadaires, %s", palms_.size(), umbrellas_.size(), lamps_.size(), night_ ? "nuit" : "jour");
    }

    // WFS-139 (RG-C1, RG-C2) : objets du tracé 1 retirés de la chaussée et des autres branches de la route, des falaises
    // et du tunnel, des places (rond-point, carrefour), et du côté intérieur des virages serrés et des 60 m avant le feu
    // (objets hauts) ; bateaux remis dans l'eau au pied de la falaise ; en Basse, un parasol sur deux
    void filtrer(Track& t) {
        if (trace::sections(t).empty()) return;   // tracé 1 : rien ne change
        const MapDef& map = *t.map();
        const float R = (float)RW;
        auto garde = [&](int64_t n, int side, float lat, float ext, float demi, bool haut) {
            const int s = solA(n);
            if (s == S_TUNNEL || s == S_FALAISE || (s == S_PLACE && lat < R + 30)) return false;
            if (haut && cacheVue(n, side, lat)) return false;
            return placeLibre(t, n, side, lat, ext, demi);
        };
        const size_t p0 = palms_.size(), u0 = umbrellas_.size(), l0 = lamps_.size(), o0 = objets_.size();
        // WFS-180 : fêtes et jeux de plage : ni parasol ni objet côté mer (tour, villa, rocher) sur leur emprise
        std::erase_if(umbrellas_, [&](const Umbrella& u) { return plage::occupe(map, len_, u.seg); });
        std::erase_if(objets_, [&](const Objet& o) { return o.side > 0 && o.lat > R + 8 && o.t != O_BATEAU && plage::occupe(map, len_, o.seg); });
        std::erase_if(palms_, [&](const Palm& p) { return !garde(p.seg, p.lat > 0 ? 1 : -1, std::fabs(p.lat), 4.f, 3.f, true); });
        int k = 0;
        std::erase_if(umbrellas_, [&](const Umbrella& u) { return !garde(u.seg, 1, u.lat, 1.6f, 2.f, false) || (basse_ && (k++ & 1)); });
        std::erase_if(lamps_, [&](const Lampe& l) { const int s = solA(l.seg); return (s != S_PLAGE && s != S_PLACE) || !placeLibre(t, l.seg, l.side, R + 0.8f, 0.4f); });
        std::erase_if(objets_, [&](Objet& o) {
            const int s = solA(o.seg);
            switch (o.t) {
            case O_BATEAU: {
                const float ext = o.l * 0.3f;
                if (s == S_FALAISE) {   // au pied de la falaise, côté mer libre seulement
                    if (formeCote(o.seg, 1) != K_MER) return true;
                    o.lat = finFalaise(t, o.seg) + 6 + ext + (o.lat - R - 30) * 6;   // au mouillage, 6 à 70 m du pied
                    return !placeLibre(t, o.seg, 1, o.lat, ext, o.l * 0.5f);
                }
                return !garde(o.seg, 1, o.lat, ext, o.l * 0.5f, false);
            }
            case O_VILLA:   // village perché : maisons du côté montagne, sur la terrasse de la falaise
                if (s == S_FALAISE) return !(o.side < 0 && villagePerche(t, o.seg) && formeCote(o.seg, -1) == K_TERRE &&
                                             o.lat + o.d <= R + 21 && !cacheVue(o.seg, -1, o.lat) && placeLibre(t, o.seg, -1, o.lat, o.d, o.w * 0.5f));
                return !garde(o.seg, o.side, o.lat, o.d, o.w * 0.5f, true);
            case O_TOUR: return !garde(o.seg, o.side, o.lat, 1.3f, 1.3f, true);
            case O_BORNE: return !garde(o.seg, o.side, o.lat, 0.2f, 0.2f, false);
            case O_ROCHER: return !garde(o.seg, o.side, o.lat, o.h * 1.2f, o.h * 1.2f, false);
            case O_GLISSIERE: return !garde(o.seg, o.side, o.lat, 0.1f, 2.f, false);
            default: return !garde(o.seg, o.side, o.lat, o.h * 0.3f, o.h * 0.3f + std::fabs(o.along), true);   // pins, arbres
            }
        });
        // RG-B (ANO-83) : au village au bord de l'eau, aucune maison côté mer devant une barque (trouée de ±11 m)
        std::vector<int64_t> barques;
        for (const Objet& o : objets_) if (o.t == O_BATEAU && !std::strcmp(zoneId(map, o.seg, len_), "village")) barques.push_back(o.seg);
        std::erase_if(objets_, [&](const Objet& o) {
            if (o.t != O_VILLA || o.side < 0) return false;
            for (int64_t b : barques) if (std::abs(mod(o.seg - b + len_ / 2) - len_ / 2) < 9) return true;
            return false;
        });
        WFS_LOG("Plage (WFS-139) : retirés du tracé 2 : %zu palmiers, %zu parasols, %zu lampadaires, %zu objets ; %zu barques au village",
                p0 - palms_.size(), u0 - umbrellas_.size(), l0 - lamps_.size(), o0 - objets_.size(), barques.size());
    }

    // forme du bas-côté side au segment a (tracé 2) : plage d'avant, pente vers une autre branche des lacets, falaise
    // côté mer, paroi côté montagne, massif du tunnel
    int formeCote(int64_t a, int side) const {
        const int s = solA(a);
        if (s == S_TUNNEL) return K_TUNNEL;
        if (s != S_FALAISE) return K_PLAGE;
        if (touche_[(size_t)ix(a, side)] < 1e8f) return K_PENTE;
        return side > 0 ? K_MER : K_TERRE;
    }
    // falaise côté mer : du bord de l'accotement à la mer (chute D), une partie raide puis un éboulis ; x[0..2] : début,
    // rupture de pente, pied dans l'eau (m depuis l'axe)
    static void profilFalaise(float D, float x[3]) {
        x[0] = (float)RW + 4.8f; x[1] = x[0] + std::max(3.f, 0.25f * D); x[2] = x[1] + std::max(3.f, 0.3f * D);
    }
    float finFalaise(Track& t, int64_t n) const { float x[3]; profilFalaise((float)t.pose(n).y - OCEAN_Y, x); return x[2]; }
    // hauteur de la paroi côté montagne (m) : 9 à 18 m, variée le long de la route (deux sinus lents, sans répétition visible)
    static float hauteurParoi(int64_t n) { return 13.f + 3.f * (float)std::sin(n * 0.013) + 3.f * (float)std::sin(n * 0.051 + 1.3); }

    // segments jusqu'à la tête de tunnel la plus proche (hors du tunnel), grand sans tunnel
    int64_t distTunnel(int64_t n) const {
        if (tunDeb_ < 0) return INT64_MAX;
        const int64_t m = mod(n), d0 = mod(tunDeb_ - m), d1 = mod(m - (tunFin_ - 1));
        return std::min(d0, d1);
    }
    // village perché (zone « village » sur la falaise, hors des lacets) : terrasse de maisons côté montagne
    bool villagePerche(Track& t, int64_t a) const {
        const trace::Section* s = trace::sectionA(t, (double)mod(a) * SEG);
        return !std::strcmp(zoneId(*t.map(), a, len_), "village") && !(s && s->type == trace::Type::Lacets);
    }

    // bandes du bas-côté side : forme donnée par le segment a, valeurs prises au bout e (a ou a + 1) ; même nombre de
    // bandes aux deux bouts. Hauteurs relatives à la route au bout e. mer : début de la mer (m), 1e9 sans mer.
    int bandes(Track& t, int64_t a, int64_t e, int side, Band out[8], float& mer) const {
        const MapDef& map = *t.map();
        const float R = (float)RW, W0 = R + 4.f, ye = (float)t.pose(e).y, sea = OCEAN_Y - ye;
        int n = 0; mer = 1e9f;
        const int k = formeCote(a, side);
        float tc = touche_.empty() ? 1e9f : touche_[(size_t)ix(e, side)], ay = touche_.empty() ? 0.f : autreY_[(size_t)ix(e, side)];
        if (tc > 1e8f && !touche_.empty()) { tc = touche_[(size_t)ix(a, side)]; ay = autreY_[(size_t)ix(a, side)]; }
        const bool cache = cacheVue(a, side, 0);
        switch (k) {
        case K_PLAGE: {
            const char* z = zoneId(map, a, len_);
            n = profil(z, anneau(a) ? -1 : side, out);   // anneau : terre des deux côtés (voir la mer plus bas)
            if (side > 0) mer = !std::strcmp(z, "marina") ? R + 7.2f : R + 40;
            if (solA(a) == S_PLACE) {   // terre-plein pavé jusqu'à RW + 30 (rond-point, carrefour), puis le profil de la zone
                Band tmp[8]; int m = 0; const float P = R + 30;
                tmp[m++] = {W0, P, 0.1f, 0.1f, C_PROM};
                for (int i = 0; i < n; i++) {
                    if (out[i].b <= P) continue;
                    Band b = out[i];
                    if (b.a < P) { b.ya = b.ya + (b.yb - b.ya) * (P - b.a) / (b.b - b.a); b.a = P; }
                    tmp[m++] = b;
                }
                if (side > 0 && m == 1) { tmp[m++] = {P, P + 0.2f, 0.1f, sea - 0.45f, C_QUAI}; mer = P + 0.2f; }   // quai
                else if (side > 0) mer = std::max(mer, P);
                n = m; std::copy(tmp, tmp + m, out);
                // l'anneau tourne de 270° : sa mer couvrirait la station de tous les côtés ; celle des routes voisines suffit
                if (anneau(a)) mer = 1e9f;
            }
            break;
        }
        case K_PENTE: {   // vers l'autre branche : chacune pose son sol jusqu'au milieu, à mi-hauteur des deux routes
            const float mid = std::max(W0 + 0.6f, (tc + R + 1) / 2), h = (ay - ye) / 2;
            const float ha = (autreY_[(size_t)ix(a, side)] - (float)t.pose(a).y) / 2;   // forme : montée ou descente au segment a
            out[n++] = {W0, W0 + 0.6f, 0.1f, 0.1f, C_GARRIGUE};
            if (ha > 1 && !cache) {   // talus de roche taillé (≤ 6 m), puis terrasse de garrigue
                const float c = std::clamp(h, 0.1f, 6.f), x = std::min(mid, W0 + 0.6f + 0.3f * c);
                out[n++] = {W0 + 0.6f, x, 0.1f, x < mid ? c : h, ROC}; out[n++] = {x, mid, x < mid ? c : h, h, C_GARRIGUE};
            } else {
                const float midA = std::max(W0 + 1.6f, (touche_[(size_t)ix(a, side)] + R + 1) / 2);
                const bool raide = -ha / (midA - W0 - 0.6f) > 0.8f;
                out[n++] = {W0 + 0.6f, W0 + 0.6f, 0.1f, 0.1f, ROC}; out[n++] = {W0 + 0.6f, mid, 0.1f, h, raide ? ROC : C_GARRIGUE};
            }
            break;
        }
        case K_MER: {   // accotement, falaise, éboulis jusque sous la mer
            float x[3]; profilFalaise(ye - OCEAN_Y, x);
            out[n++] = {W0, x[0], 0.1f, 0.1f, C_GARRIGUE};
            out[n++] = {x[0], x[1], 0.1f, 0.1f + 0.6f * (sea - 0.1f), ROC};
            out[n++] = {x[1], x[2], 0.1f + 0.6f * (sea - 0.1f), sea - 0.4f, ROC};
            mer = x[2] - 0.6f;
            break;
        }
        case K_TERRE: {   // paroi de roche, puis la montagne couverte de garrigue ; village perché : terrasse de 18 m d'abord
            float H = cache ? 2.f : hauteurParoi(e);
            const int64_t dt = distTunnel(e);   // la paroi rejoint la tête du tunnel (côté montagne : MASSIF_H + MASSIF_MONT)
            if (dt < 60) H += (MASSIF_H + MASSIF_MONT - H) * (1 - dt / 60.f);
            const bool village = villagePerche(t, a);
            const float x0 = village ? R + 22 : W0 + 0.6f;
            out[n++] = {W0, x0, 0.1f, 0.1f, village ? C_PROM : C_GARRIGUE};
            out[n++] = {x0, x0 + 3.5f, 0.1f, H, ROC};
            out[n++] = {x0 + 3.5f, R + 90, H, H + 22, C_GARRIGUE};
            out[n++] = {R + 90, R + 220, H + 22, H + 50, C_COLLINE};
            break;
        }
        case K_TUNNEL: {   // dessus du massif (au-dessus de la voûte), puis côté mer la falaise, côté montagne la pente
            if (side > 0) {
                const float l = std::max(4.f, 0.3f * (MASSIF_H - sea));
                out[n++] = {0, MASSIF_L, MASSIF_H + 2, MASSIF_H, ROC};
                out[n++] = {MASSIF_L, MASSIF_L + l, MASSIF_H, sea - 0.4f, ROC}; mer = MASSIF_L + l - 0.6f;
            } else {
                out[n++] = {0, MASSIF_L, MASSIF_H + 2, MASSIF_H + MASSIF_MONT, ROC};
                out[n++] = {MASSIF_L, R + 160, MASSIF_H + MASSIF_MONT, MASSIF_H + 50, C_GARRIGUE};
            }
            break;
        }
        }
        // jamais au-delà du bord permis (rayon intérieur, milieu vers une autre branche) : bandes rabotées, pas retirées
        const float lim = bordSol(t, e, side);
        for (int i = 0; i < n; i++) {
            Band& b = out[i];
            if (b.a >= lim) { b.a = b.b = lim; b.yb = b.ya; }
            else if (b.b > lim) { b.yb = b.ya + (b.yb - b.ya) * (lim - b.a) / (b.b - b.a); b.b = lim; }
        }
        if (mer >= lim) mer = 1e9f;
        return n;
    }

    // WFS-94 (RG-01 à RG-04) : objets de beach.corniche, marina, rochers, village (pas et tailles de decor.js). Tirage à
    // part (graine de la carte · 31 + 11) : les palmiers et parasols déjà placés ne bougent pas.
    void placeQuartiers(const MapDef& map, const std::vector<std::pair<int64_t, int64_t>>& ranges) {
        Mulberry32 D(map.seed * 31 + 11);
        auto d = [&]() { return (float)D.next(); };
        const float R = (float)RW;
        for (auto [z0, z1] : ranges) {
            const char* z = zoneId(map, z0, len_);
            if (!std::strcmp(z, "corniche")) {
                for (int64_t n = z0 + 40; n < z1; n += 160 + (int64_t)(d() * 80)) objets_.push_back({n, O_TOUR, 1, R + 22});
                for (int64_t n = z0 + 2; n < z1 - 12; n += 12 + (int64_t)(d() * 10)) if (d() > 0.25f) {
                    Objet o{n, O_VILLA, -1, R + 12 + d() * 6}; o.w = 8 + d() * 6; o.d = 8 + d() * 5; o.h = 4 + d() * 5;
                    o.roof = (int)(d() * 3); o.floors = d() < 0.4f ? 2 : 1; o.col = (int)(d() * 6); objets_.push_back(o);
                }
            } else if (!std::strcmp(z, "marina")) {
                for (int64_t n = z0 + 2; n < z1; n += 3) objets_.push_back({n, O_BORNE, 1, R + 6.5f});
                for (int64_t n = z0 + 4; n < z1 - 6; n += 6 + (int64_t)(d() * 4)) {
                    Objet o{n, O_BATEAU, 1, R + 13 + d() * 18}; o.l = 7 + d() * 7; o.voile = d() < 0.6f; o.col = (int)(d() * 4); o.rot = (d() - 0.5f) * 0.5f;
                    objets_.push_back(o);
                }
                for (int64_t n = z0 + 2; n < z1 - 8; n += 8 + (int64_t)(d() * 4)) {
                    Objet o{n, O_VILLA, -1, R + 7}; o.w = 7 + d() * 3; o.d = 8 + d() * 3; o.h = 6 + d() * 5; o.roof = (int)(d() * 3);
                    o.floors = 2; o.col = (int)(d() * 6); objets_.push_back(o);
                }
            } else if (!std::strcmp(z, "rochers")) {
                for (int64_t n = z0 + 2; n < z1; n += 2 + (int64_t)(d() * 3)) { Objet o{n, O_ROCHER, 1, R + 6 + d() * 30}; o.h = 1 + d() * 3.5f; o.rot = d() * 6; objets_.push_back(o); }
                for (int64_t n = z0 + 1; n < z1; n += 3 + (int64_t)(d() * 3)) {
                    Objet o{n, d() < 0.75f ? O_PIN : O_ARBRE, -1, R + 6 + d() * 30}; o.h = 7 + d() * 7; o.along = (d() - 0.5f) * 3; objets_.push_back(o);
                }
                for (int64_t n = z0; n < z1; n += 3) objets_.push_back({n, O_GLISSIERE, 1, R + 1.2f});
            } else if (!std::strcmp(z, "village")) {
                for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 1 : 5); n < z1 - 7; n += 7 + (int64_t)(d() * 3)) {
                    if (side > 0 && d() < 0.5f) continue;
                    Objet o{n, O_VILLA, side, side < 0 ? R + 5 + d() * 2 : R + 18 + d() * 6}; o.w = 6 + d() * 3; o.d = 7 + d() * 3; o.h = 5 + d() * 4;
                    o.roof = (int)(d() * 3); o.floors = d() < 0.6f ? 2 : 1; o.col = (int)(d() * 6); objets_.push_back(o);
                }
                for (int64_t n = z0 + 4; n < z1; n += 11 + (int64_t)(d() * 6)) {
                    Objet o{n, O_BATEAU, 1, R + 30 + d() * 10}; o.l = 5 + d() * 3; o.col = (int)(d() * 4); o.rot = 1.2f + d(); objets_.push_back(o);
                }
            }
        }
        for (Objet& o : objets_) o.seg %= len_;
        WFS_LOG("Plage : %zu objets des quartiers (villas, bateaux, rochers, bornes, glissières, pins) (WFS-94)", objets_.size());
    }

    // dessin d'un objet (DECOR de decor.js) au segment absolu a ; petits objets (bornes, glissières) seulement de près
    void dessineObjet(Track& t, const Objet& o, int64_t a, bool lod, Geo& g) const {
        const MapDef& map = *t.map();
        const char* z = zoneId(map, a, len_);
        const Frame fr = frameAt(t, (double)a * SEG);
        const vec3 r = fr.r * (float)o.side, f = fr.f;   // r vers l'extérieur de la route
        switch (o.t) {
        case O_VILLA: {   // boîte, 3 fenêtres par étage sur la façade côté route, toit à deux pans
            const float lat = o.lat + o.d / 2;
            const vec3 c = fr.o + r * lat + vec3{0, solY(z, o.side, lat) - 0.2f, 0};
            const int mur = C_MURV0 + (o.roof == 1 ? o.col : (o.col + 2) % 6);
            box(g, c, r, f, o.d / 2, o.w / 2, o.h, palUv(mur));
            for (int i = -1; i <= 1; i++) for (int fl = 0; fl < o.floors; fl++)
                box(g, c - r * (o.d / 2 + 0.02f) + f * (i * o.w * 0.3f) + vec3{0, 1.f + fl * 2.6f, 0}, r, f, 0.03f, 0.6f, 1.3f, palUv(C_VITRE));
            const vec3 t0 = c + vec3{0, o.h, 0}, A = r * (o.d * 0.53f), F = f * (o.w * 0.53f), H{0, 1.8f, 0};
            const vec2 tu = palUv(C_TOIT0 + o.roof);
            quad(g, t0 - A - F, t0 - A + F, t0 + H + F, t0 + H - F, tu); quad(g, t0 + A + F, t0 + A - F, t0 + H - F, t0 + H + F, tu);
            tri(g, t0 - A - F, t0 + H - F, t0 + A - F, palUv(mur)); tri(g, t0 + A + F, t0 + H + F, t0 - A + F, palUv(mur));
            break;
        }
        case O_TOUR: {   // tour de sauveteur : 4 pieds, cabine rouge, toit blanc
            const vec3 b = fr.o + r * o.lat + vec3{0, sableY(o.lat), 0};
            for (float dx : {-0.8f, 0.8f}) for (float dz : {-0.8f, 0.8f}) box(g, b + r * dx + f * dz, r, f, 0.08f, 0.08f, 2.4f, palUv(C_MUR));
            box(g, b + vec3{0, 2.4f, 0}, r, f, 1.1f, 1.1f, 1.6f, palUv(C_TOUR));
            box(g, b + vec3{0, 4.0f, 0}, r, f, 1.3f, 1.3f, 0.12f, palUv(C_MUR));
            break;
        }
        case O_BORNE: if (!lod) cylinder(g, fr.o + r * o.lat + vec3{0, 0.1f, 0}, 0.18f, 0.6f, 8, palUv(C_BORNE)); break;
        case O_BATEAU: {   // coque, cabine, mât et voile (bateaux à voile 6 fois sur 10 dans la marina, barques au village)
            // WFS-139 (RG-A, RG-B) : coque posée sur l'eau (mer à altitude fixe, tirant d'eau 0,35 m), ou échouée sur le sable
            vec3 b = fr.o + r * o.lat;
            b.y = solA(a) == S_FALAISE ? OCEAN_Y - 0.35f : std::max(OCEAN_Y - 0.35f, fr.o.y + solY(z, o.side, o.lat) - 0.2f);
            const float cs = std::cos(o.rot), sn = std::sin(o.rot);
            const vec3 bf = f * cs + r * sn, br = r * cs - f * sn;   // bateau tourné de rot
            box(g, b, br, bf, o.l * 0.15f, o.l * 0.5f, 0.9f, palUv(C_COQUE0 + o.col));
            box(g, b + vec3{0, 0.9f, 0}, br, bf, o.l * 0.11f, o.l * 0.2f, 0.7f, palUv(C_CABINE));
            if (o.voile && !lod) {
                cylinder(g, b, 0.07f, o.l * 1.3f, 6, palUv(C_MAT));
                tri(g, b + vec3{0, 1.8f, 0}, b + vec3{0, o.l * 1.25f, 0}, b + vec3{0, 1.8f, 0} - bf * (o.l * 0.5f), palUv(C_VOILE));
            }
            break;
        }
        case O_ROCHER: {   // boule aplatie (rock), grise plus claire pour les petits
            const vec3 b = fr.o + r * o.lat + vec3{0, solY(z, o.side, o.lat) - o.h * 0.3f, 0};
            ball(g, b, {o.h, o.h * 0.7f, o.h * 1.2f}, palUv(o.h > 2.5f ? C_ROC : C_ROC2));
            break;
        }
        case O_GLISSIERE:
            if (!lod) {
                const vec3 b = fr.o + r * o.lat;
                box(g, b + vec3{0, 0.1f, 0}, r, f, 0.06f, 0.06f, 0.75f, palUv(C_GLISS_P));
                box(g, b + vec3{0, 0.45f, 0} + f * (1.5f * (float)SEG_M), r, f, 0.025f, 1.5f * (float)SEG_M, 0.3f, palUv(C_GLISS));
            }
            break;
        case O_PIN: case O_ARBRE: {   // tree (decor.js) : pin à deux cônes, ou arbre rond
            const vec3 b = fr.o + r * o.lat + f * o.along + vec3{0, solY(z, o.side, o.lat) - 0.1f, 0};
            if (o.t == O_PIN) {
                cylinder(g, b, 0.25f, o.h * 0.3f, 6, palUv(C_TRONC));
                cone(g, b + vec3{0, o.h * 0.2f, 0}, o.h * 0.28f, o.h * 0.55f, 8, palUv(C_PIN));
                cone(g, b + vec3{0, o.h * 0.48f, 0}, o.h * 0.21f, o.h * 0.5f, 8, palUv(C_PIN));
            } else {
                cylinder(g, b, 0.3f, o.h * 0.45f, 6, palUv(C_TRONC));
                ball(g, b + vec3{0, o.h * 0.62f, 0}, vec3{0.36f, 0.32f, 0.36f} * o.h, palUv(C_FEUILLE));
                ball(g, b + vec3{o.h * 0.15f, o.h * 0.78f, 0}, vec3{0.24f, 0.22f, 0.24f} * o.h, palUv(C_FEUILLE2));
            }
            break;
        }
        }
    }

    // banc d'essai : départ avancé dans une zone (WFS_DEPART, comme CarteDecor et la ville), une fois
    void collide(double, Player& p, const GameView& g) override {
        if (!active_ || !depart_) return;
        const double dd = (double)depart_ * SEG;
        p.position = g.track->wrap(p.position + dd); p.dist += dd;
        depart_ = 0;
    }

    bool remplaceSol(const GameView&) const override { return active_; }

    // ambiance de la plage (envFor) : brouillard plus loin de jour, nuit bleutée
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!active_ || g.sceneDediee) return;
        // Game : brouillard exponentiel 0,0009 pour la ville (fogFar 620 m du jeu web) → même rapport pour la plage
        if (!night_) {
            fp.fogColor = linHex(0xcfe3f0);
            fp.fogDensity = 0.0009f * 620.f / 950.f;
        } else {
            const float k = 2.2f;   // éclaircit les couleurs de nuit du jeu web (son étalonnage n'est pas porté)
            fp.skyTop = linHex(0x04040a) * k; fp.skyHorizon = linHex(0x2b2240) * k; fp.ground = linHex(0x0a0a10) * k;
            fp.fogColor = linHex(0x141428) * k; fp.fogDensity = 0.0009f * 620.f / 640.f;
            fp.sunDir = normalize(vec3{-0.4f, 0.8f, 0.3f}); fp.sunColor = linHex(0x8a96ff); fp.sunIntensity = 0.9f;   // lune
            fp.ambient = 0.7f; fp.exposure = 1.1f; fp.night = true; fp.shadows = false;
        }
    }

    // ---------------------------------------------------------------- WFS-180 : fêtes, jeux de plage, bateaux
    // maillages des bateaux (une fois), flotte placée sur toute la mer libre, lumières de nuit des fêtes
    void initPlage(Renderer& r, Track& t, const MapDef& map) {
        auto fait = [&](void (*f)(Geo&), Mesh& m) { Geo g; f(g); m = r.makeMesh(g.v, g.i, false); };
        fait(voilier, modeleBateau_[B_VOILIER]); fait(yacht, modeleBateau_[B_YACHT]); fait(jetski, modeleBateau_[B_JETSKI]);
        fait(paquebot, modeleBateau_[B_PAQUEBOT]); fait(sillage, sillage_);
        { Geo g; for (int k = 0; k < 10; k++) {   // faisceau : cône ouvert de 25 m, 1,8 m de rayon au bout (≈ 8°), le long de +Z
            const float a0 = 2 * PI * k / 10, a1 = 2 * PI * (k + 1) / 10;
            tri(g, {0, 0, 0}, {std::cos(a0) * 1.8f, std::sin(a0) * 1.8f, 25}, {std::cos(a1) * 1.8f, std::sin(a1) * 1.8f, 25}, {0, 0}); }
          faisceau_ = r.makeMesh(g.v, g.i, false); }
        { Geo g; cone(g, {0, 0, 0}, 0.35f, 1.f, 8, {0, 0}); cone(g, {0.15f, 0, 0.1f}, 0.2f, 0.7f, 6, {0, 0}); flamme_ = r.makeMesh(g.v, g.i, false); }
        { Geo g; ball(g, {0, 0, 0}, {0.18f, 0.2f, 0.18f}, {0, 0}); lyre_ = r.makeMesh(g.v, g.i, false); }
        bateaux_.clear();
        const plage::Lieux& L = plage::lieux(map, len_);
        Mulberry32 D(map.seed * 31 + 41);
        auto d = [&]() { return (float)D.next(); };
        const float R = (float)RW;
        // mer libre au large du segment n : plage sans autre branche côté mer, ou falaise côté mer
        auto large = [&](int64_t n) { const int k = formeCote(n, 1); return k == K_MER || (k == K_PLAGE && solA(n) != S_PLACE && touche_[(size_t)ix(n, 1)] > 1e8f && bordSol(t, n, 1) > 600); };
        for (int64_t n = 30; n < len_; n += 70 + (int64_t)(d() * 90)) {   // voiliers et yachts tous les 85 à 190 m
            const float u = d(), lat = R + 110 + d() * 480, rot = (d() - 0.5f) * 6.2832f, ph = d() * 6.2832f;
            if (large(n)) bateaux_.push_back({u < 0.62f ? (int)B_VOILIER : (int)B_YACHT, n, lat, rot, ph});
        }
        if (large(len_ / 3)) bateaux_.push_back({B_PAQUEBOT, len_ / 3, 1150.f, 0.3f, 0.f});   // un paquebot au large
        for (const plage::Lieu& l : L.lieux) for (int k = 0; k < 2; k++)   // 2 jet-skis devant chaque lieu (4 à 10 en tout)
            if (large(l.seg)) { Bateau b{B_JETSKI, l.seg, R + 62 + k * 22 + d() * 8, 0.f, d() * 6.2832f}; b.base = l.seg; bateaux_.push_back(b); }
        // nuit : projecteur au-dessus de la scène et feu de camp (lumières des 4 plus proches, nuit.cpp)
        for (const plage::Lieu& l : L.lieux) if (l.fete) {
            lampadaires().push_back({l.seg, plage::SCENE_LAT - 3.f, 6.f, StyleLampadaire::Ville});
            lampadaires().push_back({mod(l.seg + (int64_t)(14 / SEG_M)), plage::FETE_LAT - 1, 1.2f, StyleLampadaire::Ville});
        }
        WFS_LOG("Plage (WFS-180) : %zu bateaux (voiliers, yachts, jet-skis, paquebot)", bateaux_.size());
    }

    // décor fixe d'une fête ou d'un terrain de jeux, posé dans le morceau qui contient son segment (repère du segment)
    void decorLieux(Track& t, int64_t a, bool lod, Geo out[P_COUNT]) const {
        const plage::Lieux& L = plage::lieux(*t.map(), len_);
        for (const plage::Lieu& l : L.lieux) {
            if (mod(a) != mod(l.seg)) continue;
            const Frame fr = frameAt(t, (double)a * SEG);
            const vec3 X = fr.r, Z = fr.f, up{0, 1, 0};
            auto S = [&](float lat, float al, float y) { return fr.o + X * lat + Z * al + up * (y + sableY(lat)); };   // sur le sable
            auto E = [&](float lat, float al, float y) { vec3 q = fr.o + X * lat + Z * al; q.y = OCEAN_Y + y; return q; };   // sur l'eau
            if (l.fete) {
                const float sl = plage::SCENE_LAT, fl = plage::FETE_LAT;
                box(out[P_UNI], S(sl, 0, 0), X, Z, 2.5f, 5.f, 1.f, palUv(C_PLANCHE));                // scène 10 × 5 m, 1 m
                box(out[P_UNI], S(sl + 2.45f, 0, 1.f), X, Z, 0.05f, 5.f, 5.f, palUv(C_TOILE));        // fond de scène
                box(out[P_UNI], S(sl - 0.3f, 0, 1.f), X, Z, 0.4f, 1.f, 1.05f, palUv(C_TOILE));        // table du DJ
                for (float px : {-2.4f, 2.4f}) for (float pz : {-4.9f, 4.9f})                          // structure alu 29 cm
                    box(out[P_ACIER], S(sl + px, pz, 0), X, Z, 0.145f, 0.145f, 6.2f, {0, 0});
                for (float px : {-2.4f, 2.4f}) box(out[P_ACIER], S(sl + px, 0, 5.9f), X, Z, 0.145f, 5.05f, 0.29f, {0, 0});
                for (float pz : {-4.9f, 4.9f}) box(out[P_ACIER], S(sl, pz, 5.9f), X, Z, 2.55f, 0.145f, 0.29f, {0, 0});
                for (float pz : {-6.f, 6.f}) {                                                          // enceintes et caissons
                    box(out[P_UNI], S(sl - 2.6f, pz, 0), X, Z, 0.35f, 0.35f, 1.8f, palUv(C_HP));
                    box(out[P_UNI], S(sl - 2.6f, pz, 1.8f), X, Z, 0.3f, 0.4f, 1.1f, palUv(C_HP));
                }
                // guirlandes : 4 mâts de 4 m autour de la piste (20 × 12 m), ampoules tous les 0,8 m, flèche de 0,6 m
                const vec2 mats[4] = {{fl - 6, -10}, {fl + 6, -10}, {fl + 6, 10}, {fl - 6, 10}};
                for (const vec2& m : mats) cylinder(out[P_UNI], S(m.x, m.y, 0), 0.07f, 4.1f, 6, palUv(C_MATBOIS));
                const int paires[6][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 2}, {1, 3}};
                int k = 0;
                for (const auto& pr : paires) {
                    const vec3 A = S(mats[pr[0]].x, mats[pr[0]].y, 4.f), B = S(mats[pr[1]].x, mats[pr[1]].y, 4.f);
                    const int n = (int)(length(B - A) / 0.8f);
                    for (int i = 1; i < n; i++, k++) {
                        if ((lod || basse_) && (k & 1)) continue;   // en Basse et au loin : une ampoule sur deux (RG-C2)
                        const float u = (float)i / n;
                        ball(out[P_GUIRL], A + (B - A) * u - up * (2.4f * u * (1 - u)), {0.05f, 0.06f, 0.05f}, {0, 0});
                    }
                }
                // feu de camp (cercle de pierres de 1,4 m, trois bûches) ; flamme dessinée la nuit
                for (int i = 0; i < 9; i++) { const float an = 2 * PI * i / 9; ball(out[P_UNI], S(fl - 1 + std::cos(an) * 0.7f, 14 + std::sin(an) * 0.7f, 0.05f), {0.16f, 0.12f, 0.15f}, palUv(C_PIERRE)); }
                for (int i = 0; i < 3; i++) { const float an = 2 * PI * i / 3; const vec3 dr = X * std::cos(an) + Z * std::sin(an);
                    box(out[P_UNI], S(fl - 1, 14, 0.05f), dr, cross(up, dr), 0.45f, 0.07f, 0.14f, palUv(C_BUCHE)); }
            } else {
                const float vl = plage::VOLLEY_LAT;
                // terrain FIVB 16 × 8 m, lignes de 6 cm (règles officielles du beach-volley, art. 1.3)
                auto ligne = [&](float l0, float a0, float l1, float a1) {
                    const vec3 d = normalize(S(l1, a1, 0) - S(l0, a0, 0)), n = normalize(cross(up, d)) * 0.03f;
                    quad(out[P_UNI], S(l0, a0, 0.01f) - n, S(l1, a1, 0.01f) - n, S(l1, a1, 0.01f) + n, S(l0, a0, 0.01f) + n, palUv(C_LIGNE));
                };
                ligne(vl - 4, -8, vl - 4, 8); ligne(vl + 4, -8, vl + 4, 8); ligne(vl - 4, -8, vl + 4, -8); ligne(vl - 4, 8, vl + 4, 8);
                // filet : 8,5 m de long, 1 m de haut, bord supérieur à 2,43 m (hommes) ; poteaux de 2,55 m à 0,85 m des lignes
                const float y0 = 2.43f - sableY(vl);   // hauteur prise au centre du terrain
                quad(out[P_FILET], S(vl - 4.25f, 0, y0 - 1.f + sableY(vl) - sableY(vl - 4.25f)), S(vl + 4.25f, 0, y0 - 1.f + sableY(vl) - sableY(vl + 4.25f)),
                     S(vl + 4.25f, 0, y0 + sableY(vl) - sableY(vl + 4.25f)), S(vl - 4.25f, 0, y0 + sableY(vl) - sableY(vl - 4.25f)), {0, 0});
                box(out[P_UNI], S(vl, 0, 2.43f - 0.07f), X, Z, 4.25f, 0.01f, 0.07f, palUv(C_MUR));   // bande blanche de 7 cm
                for (float sd : {-1.f, 1.f}) cylinder(out[P_UNI], S(vl + sd * 4.85f, 0, 0), 0.05f, 2.55f, 8, palUv(C_LIGNE));
                ball(out[P_UNI], S(vl + 0.6f, 1.8f, 3.1f), {0.105f, 0.105f, 0.105f}, palUv(C_BALLON));   // ballon : 66-68 cm de tour
                // château de sable au bord de l'eau
                const vec3 cb = S((float)RW + 35.5f, -9, 0);
                cone(out[P_UNI], cb, 0.9f, 0.45f, 12, palUv(C_SABLE_M));
                for (int i = 0; i < 4; i++) { const float an = PI / 2 * i + 0.4f; cylinder(out[P_UNI], cb + vec3{std::cos(an) * 0.55f, 0.15f, std::sin(an) * 0.55f}, 0.13f, 0.32f, 8, palUv(C_SABLE_M)); }
                cylinder(out[P_UNI], cb + vec3{0, 0.35f, 0}, 0.18f, 0.35f, 8, palUv(C_SABLE_M));
                // pédalos (2,6 × 1,5 m : deux flotteurs, siège, toboggan) au bord de l'eau ; un seul en Basse
                for (int i = 0; i < (basse_ ? 1 : 2); i++) {
                    const float al = -4 + i * 3.4f, pl = (float)RW + 52;
                    for (float sd : {-0.55f, 0.55f}) box(out[P_UNI], E(pl + sd, al, -0.15f), X, Z, 0.25f, 1.3f, 0.4f, palUv(i ? C_BLEU : C_JAUNE));
                    box(out[P_UNI], E(pl, al - 0.3f, 0.25f), X, Z, 0.7f, 0.45f, 0.18f, palUv(C_GELCOAT));
                    quad(out[P_UNI], E(pl - 0.3f, al + 0.6f, 1.3f), E(pl + 0.3f, al + 0.6f, 1.3f), E(pl + 0.3f, al + 1.4f, 0.3f), E(pl - 0.3f, al + 1.4f, 0.3f), palUv(C_ROUGE));
                }
                // toboggan gonflable sur l'eau (7 m, sommet à 3,5 m) : ponton, rampe, boudins latéraux
                const float tl = (float)RW + 48, ta = 9;
                box(out[P_UNI], E(tl, ta, -0.2f), X, Z, 1.6f, 3.5f, 0.8f, palUv(C_BLEU));
                quad(out[P_UNI], E(tl - 1.2f, ta - 3.3f, 3.5f), E(tl + 1.2f, ta - 3.3f, 3.5f), E(tl + 1.2f, ta + 3.3f, 0.6f), E(tl - 1.2f, ta + 3.3f, 0.6f), palUv(C_JAUNE));
                for (float sd : {-1.35f, 1.35f}) for (int i = 0; i < 7; i++) ball(out[P_UNI], E(tl + sd, ta - 3.f + i, 3.6f - i * 0.45f), {0.3f, 0.3f, 0.5f}, palUv(C_BLEU));
                box(out[P_UNI], E(tl, ta - 3.6f, 0.6f), X, Z, 1.3f, 0.4f, 3.f, palUv(C_BLEU));   // tour d'accès
            }
        }
    }

    // repère d'un objet : axes x (droite), y (haut), z (avant), origine o
    static mat4 repere(vec3 o, vec3 x, vec3 y, vec3 z) {
        mat4 m = mat4::identity();
        const vec3 c[4] = {x, y, z, o};
        for (int j = 0; j < 4; j++) { m.at(0, j) = c[j].x; m.at(1, j) = c[j].y; m.at(2, j) = c[j].z; }
        return m;
    }
    // bateaux sur la houle (période 6 s ; pilonnement ±0,25 m, roulis ±3°, tangage ±2° pour les petits, presque rien pour
    // le paquebot), jet-skis qui longent la côte à ≈ 15 m/s ; faisceaux et flammes des fêtes la nuit
    int dessineMer(Renderer& r, Track& t, int64_t segJ, double temps, bool t2) {
        int draws = 0;
        auto relatif = [&](int64_t seg) { int64_t d = ((seg - segJ) % len_ + len_) % len_; return d > len_ / 2 ? d - len_ : d; };
        for (const Bateau& b : bateaux_) {
            double sg = (double)b.seg; float sens = 1;
            if (b.type == B_JETSKI) {   // aller-retour de ±60 m le long du rivage
                const double w = 0.25, ph = temps * w + b.phase;
                sg = (double)b.base + 60.0 / SEG_M * std::sin(ph); sens = std::cos(ph) >= 0 ? 1.f : -1.f;
            }
            const int64_t rel = relatif((int64_t)std::floor(sg));
            const double portee = b.type == B_PAQUEBOT ? 2500 : 900;
            if (rel * SEG_M < -200 || rel * SEG_M > portee) continue;
            const double abs = (double)(segJ + rel) + (sg - std::floor(sg));
            t.pose((int64_t)abs - 1); t.pose((int64_t)abs + 2);
            const TrackPoint p = t.point(abs * SEG, b.lat);
            const vec3 rr{(float)std::cos(p.th), 0, (float)std::sin(p.th)}, ff{(float)std::sin(p.th), 0, (float)-std::cos(p.th)};
            const float rot = b.type == B_JETSKI ? (sens > 0 ? 0.f : PI) : b.rot;
            const vec3 z = ff * std::cos(rot) + rr * std::sin(rot);
            const float k = b.type == B_PAQUEBOT ? 0.05f : b.type == B_JETSKI ? 0.4f : 1.f;
            const float ph = (float)temps * 1.047f + b.phase;   // 2π / 6 s
            const vec3 up0{0, 1, 0}, x0 = cross(up0, z);
            const vec3 up = normalize(up0 + x0 * (0.052f * k * std::sin(ph)) + z * (0.035f * k * std::sin(ph * 1.3f + 1)));
            const vec3 x = normalize(cross(up, z)), zz = cross(x, up);
            vec3 o{(float)p.x, (t2 ? 0.f : (float)p.y) + OCEAN_Y + 0.25f * k * std::sin(ph + 0.7f), (float)p.z};
            r.draw(modeleBateau_[b.type], coqueMat_, repere(o, x, up, zz)); draws++;
            if (b.type == B_JETSKI) { r.draw(sillage_, sillageMat_, repere(vec3{o.x, (t2 ? 0.f : (float)p.y) + OCEAN_Y, o.z}, cross(up0, z), up0, z)); draws++; }
        }
        // fête la plus proche devant le joueur : lyres colorées (jour et nuit), faisceaux qui balaient et flamme (nuit)
        for (const plage::Lieu& l : plage::lieux(*t.map(), len_).lieux) {
            if (!l.fete) continue;
            const int64_t rel = relatif(l.seg);
            if (rel * SEG_M < -60 || rel * SEG_M > 500) continue;
            const int64_t abs = segJ + rel;
            t.pose(abs - 1); t.pose(abs + 14);
            const Frame fr = frameAt(t, (double)abs * SEG);
            const float sl = plage::SCENE_LAT;
            for (int i = 0; i < 4; i++) {
                const float al = -3.6f + 2.4f * i;
                const vec3 o = fr.o + fr.r * (sl - 2.4f) + fr.f * al + vec3{0, sableY(sl - 2.4f) + 5.6f, 0};
                const int c = ((int)(temps * 0.5) + i) % 3;
                r.draw(lyre_, lyreMat_[c], translate(o)); draws++;
                if (!night_) continue;
                const float a1 = (float)temps * 0.9f + i * 1.3f;   // balayage : vers la piste et le ciel
                const vec3 z = normalize(fr.r * (-0.6f + 0.5f * std::sin(a1)) + fr.f * (0.6f * std::cos(a1 * 0.7f)) + vec3{0, -0.55f + 0.5f * std::sin(a1 * 0.5f), 0});
                const vec3 x = normalize(cross(vec3{0, 1, 0}, z)), y = cross(z, x);
                r.draw(faisceau_, faisceauMat_[c], repere(o, x, y, z)); draws++;
            }
            if (night_) {
                const float f = 1 + 0.15f * std::sin((float)temps * 13) + 0.1f * std::sin((float)temps * 7.3f + 1);
                const vec3 o = fr.o + fr.r * (plage::FETE_LAT - 1) + fr.f * 14.f + vec3{0, sableY(plage::FETE_LAT - 1) + 0.12f, 0};
                r.draw(flamme_, flammeMat_, translate(o) * scale({1, f, 1})); draws++;
            }
        }
        return draws;
    }

    // géométrie d'un morceau (segments [c·CH, (c+1)·CH)) — appelée en parallèle, lit seulement la piste
    // profil de la voûte du tunnel (lat, y), du pied du piédroit droit au pied du piédroit gauche
    static int profilVoute(vec2 pts[24]) {
        int n = 0;
        pts[n++] = {TUNNEL_W, 0.f};
        for (int k = 0; k <= 16; k++) {
            const float f = PI * k / 16;
            pts[n++] = {TUNNEL_W * std::cos(f), TUNNEL_PIED + (TUNNEL_H - TUNNEL_PIED) * std::sin(f)};
        }
        pts[n++] = {-TUNNEL_W, 0.f};
        return n;
    }

    // WFS-139 (RG-01) : décor d'un segment du tracé 2 : glissière (côté mer), muret (au-dessus d'une autre branche), voûte,
    // luminaires et têtes du tunnel, îlot du rond-point, feu tricolore et rue transversale du carrefour
    void decorFalaise(Track& t, int64_t a, bool lod, Geo out[P_COUNT]) const {
        const double s0 = (double)a * SEG, s1 = (double)(a + 1) * SEG;
        auto P = [&](double s, float lat, float y) { TrackPoint p = t.point(s, lat); return vec3{(float)p.x, (float)p.y + y, (float)p.z}; };
        const float v0 = (float)std::fmod((double)a * SEG_M, 400.0) / ROC_TUILE, v1 = v0 + (float)SEG_M / ROC_TUILE;
        for (int side : {-1, 1}) {
            const int k = formeCote(a, side);
            const float sd = (float)side;
            if (k == K_MER && GLISS_LAT + 0.3f < std::min(bordSol(t, a, side), bordSol(t, a + 1, side))) {
                // glissière métallique à double ondulation (lisse de 0,31 m de haut, bord supérieur à 0,75 m ; supports tous
                // les 4 m) — dimensions de la glissière simple GS de la norme NF EN 1317 telles que décrites par le SETRA,
                // à confirmer sur la source (réseau bloqué)
                static const vec2 W[5] = {{0, 0.44f}, {0.05f, 0.52f}, {0, 0.60f}, {0.05f, 0.67f}, {0, 0.75f}};
                for (int i = 0; i < 4; i++)
                    quad(out[P_ACIER], P(s0, sd * (GLISS_LAT + W[i].x), W[i].y), P(s0, sd * (GLISS_LAT + W[i + 1].x), W[i + 1].y),
                         P(s1, sd * (GLISS_LAT + W[i + 1].x), W[i + 1].y), P(s1, sd * (GLISS_LAT + W[i].x), W[i].y), {0, 0});
                if (!lod && std::floor(s0 * M / 4) != std::floor(s1 * M / 4)) {
                    const Frame fr = frameAt(t, s0);
                    box(out[P_ACIER], fr.o + fr.r * (sd * (GLISS_LAT + 0.12f)) + vec3{0, 0.1f, 0}, fr.r, fr.f, 0.03f, 0.05f, 0.62f, {0, 0});
                }
            }
            if (k == K_PENTE && autreY_[(size_t)ix(a, side)] < (float)t.pose(a).y - 2 && (float)RW + 4.6f < bordSol(t, a, side)) {
                // muret de pierre (parapet maçonné 0,6 m au-dessus de l'accotement, 0,4 m d'épaisseur) au bord d'une pente
                const float l0 = (float)RW + 4.1f, l1 = l0 + 0.4f;
                const vec3 a0 = P(s0, sd * l0, 0.1f), a1 = P(s0, sd * l0, 0.7f), a2 = P(s0, sd * l1, 0.7f), a3 = P(s0, sd * l1, -0.4f);
                const vec3 b0 = P(s1, sd * l0, 0.1f), b1 = P(s1, sd * l0, 0.7f), b2 = P(s1, sd * l1, 0.7f), b3 = P(s1, sd * l1, -0.4f);
                quadUv(out[P_ROC], a0, a1, b1, b0, {0, v0}, {0.15f, v0}, {0.15f, v1}, {0, v1});
                quadUv(out[P_ROC], a1, a2, b2, b1, {0.15f, v0}, {0.25f, v0}, {0.25f, v1}, {0.15f, v1});
                quadUv(out[P_ROC], a2, a3, b3, b2, {0.25f, v0}, {0.52f, v0}, {0.52f, v1}, {0.25f, v1});
            }
        }
        if (solA(a) == S_TUNNEL) {
            vec2 pts[24]; const int n = profilVoute(pts);
            float u = 0;
            for (int i = 0; i + 1 < n; i++) {   // voûte brute, taillée dans la roche
                const float l = length(pts[i + 1] - pts[i]);
                quadUv(out[P_VOUTE], P(s0, pts[i].x, pts[i].y), P(s1, pts[i].x, pts[i].y), P(s1, pts[i + 1].x, pts[i + 1].y), P(s0, pts[i + 1].x, pts[i + 1].y),
                       {u / ROC_TUILE, v0}, {u / ROC_TUILE, v1}, {(u + l) / ROC_TUILE, v1}, {(u + l) / ROC_TUILE, v0});
                u += l;
            }
            // luminaires sodium (≈ 1,2 m) sous la voûte : zone d'entrée (50 m après chaque tête) renforcée, deux rangées
            // tous les 3,6 m ; intérieur : une rangée tous les 8,4 m (CIE 88:2004 : zone de seuil plus éclairée)
            const int64_t dEntree = mod(a) - mod(tunDeb_), dSortie = mod(tunFin_) - 1 - mod(a);
            const bool seuil = dEntree < 42 || dSortie < 42;
            for (int side : {-1, 1}) {
                if (!(seuil ? a % 3 == 0 : (side > 0 && a % 7 == 0))) continue;
                const float f = side > 0 ? 0.62f : PI - 0.62f, lat = 0.97f * TUNNEL_W * std::cos(f), y = TUNNEL_PIED + (TUNNEL_H - TUNNEL_PIED) * std::sin(f) - 0.15f;
                const Frame fr = frameAt(t, s0);
                box(out[P_SODIUM], P(s0, lat, y), fr.r, fr.f, 0.14f, 0.6f, 0.1f, {0, 0});
            }
            // têtes du tunnel : paroi de roche entre la voûte et un rectangle de 2 × MASSIF_L par MASSIF_H
            for (int bout : {0, 1}) {
                if (bout == 0 ? mod(a) != mod(tunDeb_) : mod(a + 1) != mod(tunFin_)) continue;
                const double s = bout ? s1 : s0;
                // contour : côté mer à MASSIF_H, sommet au-dessus de la voûte, côté montagne plus haut (pente du massif)
                const vec2 Rp[5] = {{MASSIF_L, 0}, {MASSIF_L, MASSIF_H}, {0, MASSIF_H + 2}, {-MASSIF_L, MASSIF_H + MASSIF_MONT}, {-MASSIF_L, 0}};
                float tot = 0; for (int j = 0; j < 4; j++) tot += length(Rp[j + 1] - Rp[j]);
                auto bord = [&](float q) {   // point du contour à la fraction q de sa longueur
                    float d = q * tot;
                    for (int j = 0; j < 4; j++) { const float l = length(Rp[j + 1] - Rp[j]); if (d <= l) return Rp[j] + (Rp[j + 1] - Rp[j]) * (d / l); d -= l; }
                    return Rp[4];
                };
                float arc = 0, cum[24] = {};
                for (int i = 0; i + 1 < n; i++) { arc += length(pts[i + 1] - pts[i]); cum[i + 1] = arc; }
                for (int i = 0; i + 1 < n; i++) {
                    const vec2 q0 = bord(cum[i] / arc), q1 = bord(cum[i + 1] / arc);
                    quadUv(out[P_ROC], P(s, pts[i].x, pts[i].y), P(s, pts[i + 1].x, pts[i + 1].y), P(s, q1.x, q1.y), P(s, q0.x, q0.y),
                           vec2{pts[i].x, pts[i].y} / ROC_TUILE, vec2{pts[i + 1].x, pts[i + 1].y} / ROC_TUILE, q1 / ROC_TUILE, q0 / ROC_TUILE);
                }
            }
        }
        if (rpSeg_ >= 0 && mod(a) == mod(rpSeg_)) {   // îlot du rond-point : gazon, ancre géante, trois palmiers
            const Pose& p = t.pose(a);
            const double c = t.segAt(a).curve, rr = SEG_M / (std::abs(c) * CURVE_K), kc = c > 0 ? 1 : -1;
            const vec3 ctr{(float)(p.x + std::cos(p.th) * rr * kc), (float)p.y, (float)(p.z + std::sin(p.th) * rr * kc)};
            const float ri = (float)(rr - RW - 4) - 0.1f;   // bord intérieur du trottoir de l'anneau
            if (ri > 1.f) {
                cylinder(out[P_UNI], ctr, ri, 0.25f, 20, palUv(C_PROM));   // bordure
                for (int k = 0; k < 20; k++) {   // gazon
                    const float a0 = 2 * PI * k / 20, a1 = 2 * PI * (k + 1) / 20;
                    tri(out[P_UNI], ctr + vec3{0, 0.25f, 0}, ctr + vec3{std::cos(a1) * ri, 0.25f, std::sin(a1) * ri}, ctr + vec3{std::cos(a0) * ri, 0.25f, std::sin(a0) * ri}, palUv(C_GAZON));
                }
                // ancre marine monumentale (4 m) : verge, jas, organeau, bras et pattes ; acier peint
                const vec3 b = ctr + vec3{0, 0.25f, 0}, X{1, 0, 0}, Z{0, 0, 1};
                box(out[P_UNI], b, X, Z, 0.14f, 0.14f, 3.6f, palUv(C_ANCRE));
                box(out[P_UNI], b + vec3{0, 3.0f, 0}, Z, X, 0.1f, 1.1f, 0.2f, palUv(C_ANCRE));   // jas, en travers des bras
                ball(out[P_UNI], b + vec3{0, 3.85f, 0}, {0.3f, 0.3f, 0.08f}, palUv(C_ANCRE));
                for (float sx : {-1.f, 1.f}) {
                    const vec3 dir = normalize(vec3{sx * 1.f, 0.75f, 0});
                    const vec3 o = b + vec3{0, 0.35f, 0};
                    for (int i = 0; i < 6; i++) ball(out[P_UNI], o + dir * (0.25f * i) + vec3{sx * 0.08f * i, 0, 0}, {0.13f, 0.13f, 0.13f}, palUv(C_ANCRE));
                    const vec3 pt = o + vec3{sx * 1.5f, 1.15f, 0};
                    tri(out[P_UNI], pt + vec3{0, 0.45f, 0}, pt + vec3{-sx * 0.3f, -0.1f, 0.25f}, pt + vec3{-sx * 0.3f, -0.1f, -0.25f}, palUv(C_ANCRE));   // patte
                }
                Mulberry32 Rn(4242);
                for (int k = 0; k < 3; k++) {
                    const float an = 2 * PI * k / 3 + 0.5f, d = std::min(ri - 0.4f, 1.7f);
                    veg::palmier(out[P_STIPE], out[P_PALME], out[P_SECHE], ctr + vec3{std::cos(an) * d, 0.2f, std::sin(an) * d}, 7.f + k * 0.8f, an, 0.08f, Rn, lod);
                }
            }
        }
        for (const FeuDecor& fd : feux_) {
            if (mod(a) == mod(fd.ligne)) {   // feux tricolores latéraux des deux côtés, avant la ligne d'arrêt
                const Frame fr = frameAt(t, s0);
                for (float sd : {-1.f, 1.f}) {
                    const vec3 b = fr.o + fr.r * (sd * ((float)RW + 0.7f));
                    cylinder(out[P_UNI], b + vec3{0, 0.15f, 0}, 0.06f, 3.5f, 8, palUv(C_CAISSON));
                    box(out[P_UNI], b + vec3{0, 2.55f, 0} - fr.f * 0.1f, fr.r, fr.f, 0.17f, 0.12f, 0.95f, palUv(C_CAISSON));
                }
            }
            if (mod(a) == mod(fd.croise)) {   // rue transversale (2 voies, 9 m) de part et d'autre, jusqu'à RW + 30 m
                const double ds = 4.5 / M;
                for (float sd : {-1.f, 1.f}) {
                    const float l1 = std::min((float)RW + 30, bordSol(t, a, sd > 0 ? 1 : -1));
                    const vec3 A = P(s0 - ds, sd * (float)RW, 0.17f), B = P(s0 + ds, sd * (float)RW, 0.17f), C = P(s0 + ds, sd * l1, 0.17f), D = P(s0 - ds, sd * l1, 0.17f);
                    const vec2 ua{-4.5f, 0}, ub{4.5f, 0}, uc{4.5f, l1 - (float)RW}, ud{-4.5f, l1 - (float)RW};
                    if (sd > 0) quadUv(out[P_ROUTE], A, D, C, B, ua, ud, uc, ub); else quadUv(out[P_ROUTE], A, B, C, D, ua, ub, uc, ud);
                }
            }
        }
    }

    void buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]) {
        const MapDef& map = *t.map();
        const int64_t len = (int64_t)t.segmentCount();
        const int64_t a0 = c * CH, a1 = a0 + CH;
        // sol et mer, segment par segment (WFS-139 : forme du bas-côté selon la section du tracé 2, bandes rabotées au bord
        // permis ; roche texturée, uv en mètres / ROC_TUILE)
        const bool t2 = !trace::sections(t).empty();
        for (int64_t a = a0; a < a1; a++) {
            const double s0 = (double)a * SEG, s1 = (double)(a + 1) * SEG;
            const float v0 = (float)std::fmod((double)a * SEG_M, 400.0) / ROC_TUILE, v1 = v0 + (float)SEG_M / ROC_TUILE;
            for (int side : {-1, 1}) {
                Band b0[8] = {}, b1[8] = {}; float m0 = 1e9f, m1 = 1e9f;
                const int n = bandes(t, a, a, side, b0, m0);
                bandes(t, a, a + 1, side, b1, m1);
                auto P = [&](double s, float lat, float y) { TrackPoint p = t.point(s, side * lat); return vec3{(float)p.x, (float)p.y + y, (float)p.z}; };
                float u0 = 0, u1 = 0;
                for (int i = 0; i < n; i++) {
                    const Band &x = b0[i], &y = b1[i];
                    const float l0 = std::hypot(x.b - x.a, x.yb - x.ya), l1 = std::hypot(y.b - y.a, y.yb - y.ya);
                    if (l0 < 0.01f && l1 < 0.01f) continue;   // bande rabotée à rien
                    vec3 A = P(s0, x.a, x.ya), B = P(s0, x.b, x.yb), C = P(s1, y.b, y.yb), Dd = P(s1, y.a, y.ya);
                    if (x.col == ROC) {
                        const vec2 ua{u0 / ROC_TUILE, v0}, ub{(u0 + l0) / ROC_TUILE, v0}, uc{(u1 + l1) / ROC_TUILE, v1}, ud{u1 / ROC_TUILE, v1};
                        if (side > 0) quadUv(out[P_ROC], A, B, C, Dd, ua, ub, uc, ud); else quadUv(out[P_ROC], A, Dd, C, B, ua, ud, uc, ub);
                    } else if (side > 0) quad(out[P_UNI], A, B, C, Dd, palUv(x.col)); else quad(out[P_UNI], A, Dd, C, B, palUv(x.col));   // face vers le haut
                    u0 += l0; u1 += l1;
                }
                // mer : à altitude fixe au tracé 2 (la route monte jusqu'à 60 m au-dessus), jusqu'au bord permis
                if (side < 0 || m0 > 1e8f || m1 > 1e8f) continue;
                const float f0 = std::min((float)RW + OCEAN_W, bordSol(t, a, 1)), f1 = std::min((float)RW + OCEAN_W, bordSol(t, a + 1, 1));
                if (f0 <= m0 || f1 <= m1) continue;
                auto Q = [&](double s, float lat) { TrackPoint p = t.point(s, lat); return vec3{(float)p.x, t2 ? OCEAN_Y : (float)p.y + OCEAN_Y, (float)p.z}; };
                quad(out[P_OCEAN], Q(s0, m0), Q(s0, f0), Q(s1, f1), Q(s1, m1), {0, 0});
            }
            if (t2) decorFalaise(t, a, lod, out);
            decorLieux(t, a, lod, out);   // WFS-180
        }
        const int64_t lap = (int64_t)std::floor((double)a0 / len) * len;
        auto inChunk = [&](int64_t seg, int64_t& abs) {
            for (int64_t L : {lap, lap + len}) { const int64_t a = seg + L; if (a >= a0 && a < a1) { abs = a; return true; } }
            return false;
        };
        for (const Palm& pm : palms_) {
            int64_t a; if (!inChunk(pm.seg, a)) continue;
            const int side = pm.lat > 0 ? 1 : -1;
            TrackPoint p = t.point((double)a * SEG, pm.lat);
            Band b[4] = {}; const int n = profil(zoneId(map, a, len), side, b);
            float gy = 0.15f;   // trottoir de Game
            const float al = std::fabs(pm.lat);
            for (int i = 0; i < n; i++) if (al >= b[i].a && al <= b[i].b) gy = b[i].ya + (b[i].yb - b[i].ya) * (al - b[i].a) / (b[i].b - b[i].a);
            const vec3 base{(float)p.x, (float)p.y + gy - 0.05f, (float)p.z};
            Mulberry32 R((uint32_t)((int)(pm.h * 977) + (int)(pm.rot * 1000)));
            const vec3 top = veg::palmier(out[P_STIPE], out[P_PALME], out[P_SECHE], base, pm.h, pm.rot, pm.lean, R, lod);
            ball(out[P_UNI], top + vec3{0, -0.25f, 0}, {0.42f, 0.55f, 0.42f}, palUv(C_GAINE));   // gaines des palmes
            if (!lod) {
                Mulberry32 Rn((uint32_t)((int)(pm.h * 31) + 7));
                for (int k = 0; k < 5; k++) { const float an = (float)Rn.next() * 6.283f; ball(out[P_UNI], top + vec3{std::cos(an) * 0.3f, -0.55f, std::sin(an) * 0.3f}, {0.22f, 0.32f, 0.22f}, palUv(C_NOIX)); }
            }
        }
        // WFS-81 : lampadaire « beach » (decor.js, lamp) : poteau blanc 0,18 × 5,8 à RW + 0,8, boule lampWarm 0,35 à 6,1 m
        for (const Lampe& l : lamps_) {
            int64_t a; if (!inChunk(l.seg, a)) continue;
            const Frame fr = frameAt(t, (double)a * SEG);
            const vec3 b = fr.o + fr.r * (l.side * ((float)RW + 0.8f));
            box(out[P_UNI], b + vec3{0, 0.1f, 0}, fr.r, fr.f, 0.09f, 0.09f, 5.8f, palUv(C_POTEAU));
            ball(out[P_NEON], b + vec3{0, 6.1f, 0}, {0.35f, 0.35f, 0.35f}, palUv(C_LAMPE));
        }
        for (const Objet& o : objets_) { int64_t a; if (inChunk(o.seg, a)) dessineObjet(t, o, a, lod, out[P_UNI]); }   // WFS-94
        if (!lod) for (const Umbrella& u : umbrellas_) {   // petits objets : pas au-delà de 200 m
            int64_t a; if (!inChunk(u.seg, a)) continue;
            const Frame fr = frameAt(t, (double)a * SEG);
            const vec3 b = fr.o + fr.r * u.lat + fr.f * u.along + vec3{0, sableY(u.lat), 0};
            cylinder(out[P_UNI], b, 0.04f, 2.3f, 6, palUv(C_MUR));
            cone(out[P_UNI], b + vec3{0, 1.9f, 0}, 1.5f, 0.55f, 10, palUv(C_PARASOL0 + u.col));
            box(out[P_UNI], b + fr.r * 1.2f, fr.r, fr.f, 0.35f, 0.95f, 0.3f, palUv(C_TRANSAT));
        }
    }

    // banc d'essai (WFS-139) : vue de dessus de la Corniche avec son décor (route et voiture du joueur dessinées par Game)
    bool scene(FrameParams& fp, SceneDediee& sd, const GameView& g) override {
        if (!active_ || !g_vueDessus.actif) return false;
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        fp.view = lookAt(g_vueDessus.oeil, g_vueDessus.cible, g_vueDessus.haut);
        fp.proj = perspectiveVk(g_vueDessus.fov * PI / 180.f, aspect, 0.5f);
        fp.camPos = g_vueDessus.oeil; fp.fogDensity = 0; fp.shadows = false; fp.hasFocus = false;
        sd.route = true; sd.joueur = true; sd.cible = g_vueDessus.cible;
        return true;
    }

    void render(Renderer& r, const GameView& g) override {
        if (!active_ || (g.sceneDediee && !g_vueDessus.actif)) return;
        frame_++;
        Track& t = *g.track;
        const int64_t pc = (int64_t)std::floor(g.player->dist / SEG) / CH;
        for (auto it = chunks_.begin(); it != chunks_.end();) {
            const bool far = std::abs(it->first - pc) * CH * SEG_M > LOD_FAR_M;
            if (it->first < pc - BEHIND || it->first > pc + AHEAD || far != it->second.lod) {
                for (Mesh& m : it->second.m) if (m.vb.buf) graveyard_.push_back({frame_, m});
                it = chunks_.erase(it);
            } else ++it;
        }
        std::vector<int64_t> missing; std::vector<char> lods;
        for (int64_t c = pc - BEHIND; c <= pc + AHEAD; c++) if (!chunks_.count(c)) { missing.push_back(c); lods.push_back(std::abs(c - pc) * CH * SEG_M > LOD_FAR_M); }
        if (!missing.empty()) {
            // poses calculées ici (thread principal) ; les threads ne font que lire la piste
            t.pose(missing.front() * CH - 2); t.pose((missing.back() + 1) * CH + 40);
            std::vector<std::array<Geo, P_COUNT>> cpu(missing.size());
            jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunk(t, missing[i], lods[i], cpu[i].data()); });
            for (size_t i = 0; i < missing.size(); i++) {
                Chunk& ch = chunks_[missing[i]]; ch.lod = lods[i];
                for (int p = 0; p < P_COUNT; p++) if (!cpu[i][p].i.empty()) ch.m[p] = r.makeMesh(cpu[i][p].v, cpu[i][p].i, true);
            }
        }
        for (auto it = graveyard_.begin(); it != graveyard_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        int draws = 0;
        for (auto& [k, ch] : chunks_) for (int p = 0; p < P_COUNT; p++) if (ch.m[p].indexCount) {
            if (p == P_ROUTE) r.draw(ch.m[p], mats_[p], I, 4.5f); else r.draw(ch.m[p], mats_[p], I);   // rue transversale : 9 m
            draws++;
        }
        draws += dessineLentilles(r, t, (int64_t)std::floor(g.player->dist / SEG));
        draws += dessineMer(r, t, (int64_t)std::floor(g.player->dist / SEG), g.time, !trace::sections(t).empty());   // WFS-180
        lastDraws_ = draws;
        if (frame_ % 300 == 1) WFS_LOG("Plage : %d appels de dessin (%zu morceaux)", draws, chunks_.size());
    }

    // WFS-139 (RG-01) : lampes du feu tricolore, allumées selon trace::feuA (rouge, orange, vert) ; maillages construits à
    // l'occurrence du carrefour la plus proche du joueur (le tour change la position dans le monde)
    int dessineLentilles(Renderer& r, Track& t, int64_t segJoueur) {
        lentilles_.resize(feux_.size());
        int draws = 0;
        for (size_t i = 0; i < feux_.size(); i++) {
            const FeuDecor& fd = feux_[i];
            int64_t abs = segJoueur - mod(segJoueur) + mod(fd.ligne);
            if (abs - segJoueur > len_ / 2) abs -= len_; else if (segJoueur - abs > len_ / 2) abs += len_;
            if (std::abs(abs - segJoueur) * SEG_M > 400) continue;
            Lentilles& le = lentilles_[i];
            if (le.abs != abs) {
                for (Mesh& m : le.m) if (m.vb.buf) { graveyard_.push_back({frame_, m}); m = Mesh{}; }
                le.abs = abs;
                t.pose(abs - 2); t.pose(abs + 2);
                const Frame fr = frameAt(t, (double)abs * SEG);
                for (int k = 0; k < 3; k++) {   // optiques de 200 mm, rouge en haut ; face aux conducteurs qui arrivent
                    Geo gk;
                    for (float sd : {-1.f, 1.f}) {
                        const vec3 c = fr.o + fr.r * (sd * ((float)RW + 0.7f)) - fr.f * 0.23f + vec3{0, 3.2f - 0.3f * k, 0};
                        ball(gk, c, {0.1f, 0.1f, 0.1f}, {0, 0});
                    }
                    le.m[k] = r.makeMesh(gk.v, gk.i, false);
                }
            }
            const trace::Feu f = trace::feuA(fd.feu);
            const int allume = f == trace::Feu::Rouge ? 0 : f == trace::Feu::Orange ? 1 : f == trace::Feu::Vert ? 2 : -1;
            for (int k = 0; k < 3; k++) if (le.m[k].indexCount) { r.draw(le.m[k], k == allume ? lentille_[k] : lentilleEteinte_, mat4::identity()); draws++; }
        }
        return draws;
    }

    void shutdown(Renderer& r) override {
        if (!active_) return;
        WFS_LOG("Plage : %d appels de dessin à la dernière image", lastDraws_);
        r.vk().waitIdle();
        for (auto& [k, ch] : chunks_) for (Mesh& m : ch.m) if (m.vb.buf) r.destroy(m);
        chunks_.clear();
        for (auto& gm : graveyard_) r.destroy(gm.second);
        graveyard_.clear();
        for (Mesh* m : {&modeleBateau_[0], &modeleBateau_[1], &modeleBateau_[2], &modeleBateau_[3], &sillage_, &faisceau_, &flamme_, &lyre_})
            if (m->vb.buf) { r.destroy(*m); *m = Mesh{}; }   // WFS-180
        for (Lentilles& le : lentilles_) for (Mesh& m : le.m) if (m.vb.buf) r.destroy(m);
        lentilles_.clear();
        for (Image* im : {&palTex_, &leafTex_, &barkTex_, &rocTex_, &rocNrm_, &rocMr_}) if (im->img) r.vk().destroy(*im);
        active_ = false;
    }

private:
    bool active_ = false, night_ = false;
    std::vector<Palm> palms_; std::vector<Umbrella> umbrellas_; std::vector<Lampe> lamps_;
    std::vector<Objet> objets_;   // WFS-94
    // WFS-139 : analyse du tracé 2 (preparer), textures de roche, feux tricolores
    bool basse_ = false;
    std::vector<uint8_t> sol_, cache_;   // nature du sol (Sol), côtés à dégager (bit 1 : gauche, 2 : droite)
    std::vector<float> touche_, autreY_;   // [segment · 2 + (côté > 0)] : latéral où l'on touche une autre branche, son altitude
    int64_t tunDeb_ = -1, tunFin_ = -1, rpSeg_ = -1, rpDeb_ = -1, rpFin_ = -1;
    struct FeuDecor { int64_t ligne, croise; int feu; };
    std::vector<FeuDecor> feux_;
    Image rocTex_{}, rocNrm_{}, rocMr_{};
    Material lentilleEteinte_, lentille_[3];
    struct Lentilles { int64_t abs = INT64_MIN; Mesh m[3]{}; };   // lampes rouge, orange, vert des deux têtes d'un feu
    std::vector<Lentilles> lentilles_;
    // WFS-180 : bateaux animés (repère local, un maillage par type, dessiné à chaque instance), faisceaux, flammes
    enum TypeBateau { B_VOILIER, B_YACHT, B_JETSKI, B_PAQUEBOT, B_NB };
    struct Bateau { int type; int64_t seg; float lat, rot, phase; int64_t base = 0; };
    std::vector<Bateau> bateaux_;
    Mesh modeleBateau_[B_NB]{}, sillage_{}, faisceau_{}, flamme_{}, lyre_{};
    Material coqueMat_, sillageMat_, faisceauMat_[3], lyreMat_[3], flammeMat_;
    int64_t len_ = 1, depart_ = 0;
    Material mats_[P_COUNT];
    Image palTex_{}, leafTex_{}, barkTex_{};
    struct Chunk { std::array<Mesh, P_COUNT> m{}; bool lod = false; };
    std::map<int64_t, Chunk> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
    int lastDraws_ = 0;
};

} // namespace

WFS_SYSTEM(Plage, 105)

// ---------------------------------------------------------------- banc d'essai (WFS-139, RG-C5)
// Corniche au tracé 2 en caméra de course (pilote automatique), de jour et de nuit, devant chaque lieu des sections :
// carte-plage-falaise, -tunnel, -tunnel-dedans, -rond-point, -feu, -marina (RG-A), -village (RG-B), et deux vues de dessus
// (carte-plage-lacets-dessus, carte-plage-rond-point-dessus) pour vérifier qu'aucun décor ne touche la chaussée.
void benchCorniche(Game& g, Renderer& r, const std::string& out) {
    Track t; t.build(mapById("plage"), trace::versionCarte("plage"));
    const auto& S = t.sections();
    if (S.empty()) { WFS_ERR("banc Corniche (WFS-139) : la Corniche n'est pas au tracé 2"); return; }
    const int64_t L = (int64_t)t.segmentCount();
    const trace::Section *la = nullptr, *tu = nullptr, *rp = nullptr, *fx = nullptr;
    for (const trace::Section& s : S) {
        if (s.type == trace::Type::Lacets && !la) la = &s;
        if (s.type == trace::Type::Tunnel && !tu) tu = &s;
        if (s.type == trace::Type::RondPoint && !rp) rp = &s;
        if (s.type == trace::Type::Feux && !fx) fx = &s;
    }
    int64_t village = -1;   // village au tracé 2 : sur la descente de la falaise, hors des lacets
    for (int64_t a = 0; a < L && village < 0; a++) {
        const trace::Section* s = trace::sectionA(t, a * SEG);
        if (!std::strcmp(zoneName(mapById("plage"), a, L), "Village de pêcheurs") && s && s->type != trace::Type::Lacets && t.pose(a).y > 3) village = a + 40;
    }
    int64_t fete = -1, jeux = -1;
    for (const plage::Lieu& l : plage::lieux(mapById("plage"), L).lieux) { if (l.fete && fete < 0 && l.seg >= 60) fete = l.seg; if (!l.fete && jeux < 0 && l.seg >= 60) jeux = l.seg; }
    struct Lieu { const char* nom; int64_t depart; bool nuit; int images = 240; };
    const Lieu LIEUX[] = {
        {"carte-plage-falaise", la ? la->debut + 260 : -1, false}, {"carte-plage-falaise-nuit", la ? la->debut + 260 : -1, true},
        {"carte-plage-tunnel", tu ? tu->debut - 75 : -1, false}, {"carte-plage-tunnel-nuit", tu ? tu->debut - 75 : -1, true},
        {"carte-plage-tunnel-dedans", tu ? tu->debut + 30 : -1, false},
        {"carte-plage-rond-point", rp ? rp->debut - 70 : -1, false}, {"carte-plage-feu", fx ? fx->ligneArret - 30 : -1, false, 40},
        {"carte-plage-marina", rp ? rp->fin + 30 : -1, false}, {"carte-plage-village", village, false},
        // WFS-180 : fête (jour, nuit), jeux de plage, bateaux et jet-skis
        {"carte-plage-fete", fete >= 0 ? fete - 45 : -1, false, 60}, {"carte-plage-fete-nuit", fete >= 0 ? fete - 45 : -1, true, 60},
        {"carte-plage-jeux", jeux >= 0 ? jeux - 40 : -1, false, 60}, {"carte-plage-bateaux", jeux >= 0 ? jeux + 30 : -1, false, 60}};
    for (const Lieu& l : LIEUX) {
        if (l.depart < 0) { WFS_ERR("banc Corniche : section absente pour %s", l.nom); continue; }
        setenv("WFS_MOMENT", l.nuit ? "nuit" : "jour", 1);
        setenv("WFS_DEPART", std::to_string(l.depart).c_str(), 1);
        g.init(r, "golf", "plage"); g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < l.images; i++) { g.update(1.0 / 60); if (i % 60 == 0) g.render(400 + i / 60.0); }
        g.render(404); g.render(404.02); r.capturePng(out + "/" + l.nom + ".png");
        WFS_LOG("capture %s : plage, caméra de course, départ %lld segments, %.0f m", l.nom, (long long)l.depart, g.player().dist * M);
        g.shutdown();
    }
    // vues de dessus : centre des lacets (≈ 380 m de large), rond-point
    auto dessus = [&](const char* nom, int64_t depart, int64_t centre, float h) {
        setenv("WFS_MOMENT", "jour", 1);
        setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
        g.init(r, "golf", "plage"); g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); if (i % 15 == 0) g.render(400 + i / 60.0); }
        const Pose& p = t.pose(centre);
        // la partie reconstruit la même piste : même repère que t (construction déterministe)
        g_vueDessus.actif = true; g_vueDessus.cible = {(float)p.x, (float)p.y, (float)p.z}; g_vueDessus.oeil = g_vueDessus.cible + vec3{0, h, 0.01f};
        g_vueDessus.haut = {0, 0, -1}; g_vueDessus.fov = 50;
        g.render(404); g.render(404.02); r.capturePng(out + "/" + nom + ".png");
        g_vueDessus.actif = false;
        WFS_LOG("capture %s : vue de dessus à %.0f m", nom, h);
        g.shutdown();
    };
    if (la) { dessus("carte-plage-lacets-dessus", la->debut + 150, la->debut + (la->fin - la->debut) / 2, 520); dessus("carte-plage-epingle-dessus", la->debut + 60, la->debut + 140, 110); }
    if (rp) dessus("carte-plage-rond-point-dessus", rp->debut - 20, (rp->debut + rp->fin) / 2, 110);
    // WFS-180 : vues depuis la promenade (1,7 m au-dessus du trottoir, à RW + 3 m), tournées vers la fête, les jeux et la mer
    auto promenade = [&](const char* nom, int64_t seg, float recul, float latCible, float hCible, bool nuit) {
        if (seg < 0) { WFS_ERR("banc Corniche : lieu absent pour %s", nom); return; }
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, seg - 30)).c_str(), 1);
        g.init(r, "golf", "plage"); g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 120; i++) { g.update(1.0 / 60); if (i % 40 == 0) g.render(400 + i / 60.0); }
        const TrackPoint o = t.point(((double)seg - recul / SEG_M) * SEG, (float)RW + 3), c = t.point((double)seg * SEG, latCible);
        g_vueDessus.actif = true; g_vueDessus.oeil = {(float)o.x, (float)o.y + 1.85f, (float)o.z};
        g_vueDessus.cible = {(float)c.x, (float)c.y + hCible, (float)c.z}; g_vueDessus.haut = {0, 1, 0}; g_vueDessus.fov = 55;
        g.render(404); g.render(404.02); r.capturePng(out + "/" + nom + ".png");
        g_vueDessus.actif = false;
        WFS_LOG("capture %s : vue depuis la promenade", nom);
        g.shutdown();
    };
    promenade("carte-plage-fete-vue", fete, 22, plage::SCENE_LAT, 1.5f, false);
    promenade("carte-plage-fete-vue-nuit", fete, 22, plage::SCENE_LAT, 1.5f, true);
    promenade("carte-plage-jeux-vue", jeux, 14, plage::VOLLEY_LAT + 8, 0.f, false);
    promenade("carte-plage-bateaux-vue", jeux, 0, (float)RW + 250, -1.5f, false);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
