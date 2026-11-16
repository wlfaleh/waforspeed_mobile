// WaForSpeed — WFS-552 : parking souterrain du menu (voir menu_parking.h).
//
// Mesures du réel (sources) :
//   - NF P 91-100 (parcs de stationnement accessibles au public) et guides de conception (CERTU, « Les parcs de stationnement
//     souterrains ») : place 2,50 × 5,00 m, allée à double sens 6,00 m, hauteur libre ≥ 2,20 m sous poutre ;
//   - structure en béton armé d'un parc courant : trame de poteaux de 7,50 à 8,00 m (3 places + 1 poteau), poteaux de 40 à 60 cm,
//     poutres retombées de 40 cm, dalle de 25 cm sur prédalles de 2,40 m (joints en V), voiles banchés en panneaux de 2,50 m ;
//   - marquage au sol : lignes blanches de 12 cm (épaisseur d'une peinture routière ≈ 2 mm) ; butées de roue en caoutchouc recyclé
//     550 × 150 × 100 mm à bandes jaunes rétroréfléchissantes ; bande de danger jaune et noire peinte sur les poteaux (ISO 3864) ;
//   - éclairage : 75 lux dans les allées, 50 lux sur les places (EN 12464-1, parcs de stationnement intérieurs), réglettes LED
//     étanches IP65 de 1 500 mm (≈ 4 000 lm, 4 000 K), posées en plafond tous les 4 m ;
//   - sprinklers (NF EN 12845) : collecteur DN 80 et antennes DN 25 en acier peint rouge RAL 3000, têtes pendantes en laiton à
//     déflecteur denté, suspentes en tige filetée M10 tous les 2,5 m au plus.
// Albédos et rugosités : regles-de-gestion/DA-realisme.md § 3 (béton 0,25-0,35, marquage blanc 0,65-0,80).
#include "menu_parking.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../world/mobilier_ville.h"
#include "garage_commun.h"

namespace wfs::ecrans::parking {
namespace {

using mobilier::Repere;
using mobilier::Sortie;
using garage::lin;

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };

// trame du parking (repère de la voiture) : poteaux tous les 8 m en x (3 places de 2,50 m + 0,50 m de poteau), deux rangées de
// places de part et d'autre de l'allée ; la voiture est au milieu de la place centrale de la rangée du fond
constexpr float X_MAX = 20.f;                                  // demi-largeur du niveau montré
constexpr float Z_ALLEE0 = -PLACE_P / 2, Z_ALLEE1 = Z_ALLEE0 - ALLEE;   // bords de l'allée (−2,5 et −8,5)
constexpr float Z_FOND = PLACE_P / 2 + 0.25f;                   // nu du voile du fond (derrière la bordure)
constexpr float Z_FACE = Z_ALLEE1 - PLACE_P - 0.25f;            // nu du voile d'en face
constexpr float Z_POTEAUX[2] = {1.60f, Z_ALLEE1 - 1.60f - 0.0f};   // poteaux en retrait de 0,90 m du fond des places
constexpr float X_POTEAUX[6] = {-20.f, -12.f, -4.f, 4.f, 12.f, 20.f};
constexpr float SOUS_POUTRE = HAUTEUR_DALLE - RETOMBEE_POUTRE;   // 2,20 m

const Repere MONDE{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
constexpr vec2 UV0{0, 0};

// uv en mètres (surfaces procédurales : béton de decor.glsl), projetés sur le plan le plus face à la normale ; v = hauteur sur
// les faces verticales (salissure en pied de voile et de poteau), sols et plafonds décalés de 30 m (aucune salissure de pied)
void uvMetres(Geo& g, size_t depuis) {
    for (size_t k = depuis; k < g.v.size(); k++) {
        Vertex& s = g.v[k];
        const float ax = std::fabs(s.nx), ay = std::fabs(s.ny), az = std::fabs(s.nz);
        if (ay >= ax && ay >= az) { s.u = s.px; s.v = s.pz + 30.f; }
        else if (ax >= az) { s.u = s.pz; s.v = s.py; }
        else { s.u = s.px; s.v = s.py; }
    }
}

// pavé chanfreiné du kit du mobilier (WFS-369) dans le repère du parking
void pave(Geo& g, vec3 c, vec3 h, float ch) { mobilier::pave(Sortie(g), MONDE, c, h, ch, UV0); }

// ---------------------------------------------------------------- poteau
// poteau carré de 50 cm aux 4 arêtes chanfreinées (2 cm, baguettes de coffrage), du sol à la poutre ; bande de danger peinte de
// 0,30 à 1,50 m : bandes verticales jaunes et noires de 10 cm qui font le tour (maquette 01). Section octogonale extrudée par
// tronçons : béton, bande (un quadrilatère par bande de couleur), béton ; mêmes sommets aux jonctions (aucune fente, aucune
// face en double).
void poteau(Geo& beton, Geo& jaune, Geo& noir, float cx, float cz, float h) {
    constexpr float A = POTEAU / 2, C = 0.02f, BANDE = 0.10f, Y0 = 0.30f, Y1 = 1.50f;
    const vec2 coins[8] = {{A - C, -A}, {A, -A + C}, {A, A - C}, {A - C, A}, {-A + C, A}, {-A, A - C}, {-A, -A + C}, {-A + C, -A}};
    float s0 = 0;   // abscisse curviligne autour du poteau (uv u, rang des bandes)
    for (int k = 0; k < 8; k++) {
        const vec2 a = coins[k], b = coins[(k + 1) % 8];
        const vec2 d{b.x - a.x, b.y - a.y};
        const float L = std::sqrt(d.x * d.x + d.y * d.y);
        const vec3 n = normalize(vec3{d.y, 0, -d.x});   // normale sortante (contour parcouru dans le sens direct vu de dessus)
        auto quad = [&](Geo& g, float t0, float t1, float y0, float y1) {
            const vec3 p0{cx + a.x + d.x * t0, y0, cz + a.y + d.y * t0}, p1{cx + a.x + d.x * t1, y0, cz + a.y + d.y * t1};
            const uint32_t k0 = (uint32_t)g.v.size();
            const float u0 = s0 + L * t0, u1 = s0 + L * t1;
            g.v.push_back({p0.x, y0, p0.z, n.x, n.y, n.z, u0, y0});
            g.v.push_back({p1.x, y0, p1.z, n.x, n.y, n.z, u1, y0});
            g.v.push_back({p1.x, y1, p1.z, n.x, n.y, n.z, u1, y1});
            g.v.push_back({p0.x, y1, p0.z, n.x, n.y, n.z, u0, y1});
            // sens direct vu de dehors (normale n)
            const vec3 e1{p1.x - p0.x, 0, p1.z - p0.z}, e2{0, 1, 0};
            if (dot(cross(e1, e2), n) >= 0) g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
            else g.i.insert(g.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
        };
        quad(beton, 0, 1, 0, Y0);
        quad(beton, 0, 1, Y1, h);
        // bande peinte : coupée aux limites des bandes de 10 cm (abscisse curviligne continue d'une face à l'autre)
        float t = 0;
        while (t < 1 - 1e-5f) {
            const float s = s0 + L * t;
            const int rang = (int)std::floor(s / BANDE + 1e-4f);
            const float t1 = std::min(1.f, ((float)(rang + 1) * BANDE - s0) / L);
            quad(rang % 2 ? noir : jaune, t, t1, Y0, Y1);
            t = t1;
        }
        s0 += L;
    }
}

// ---------------------------------------------------------------- réglette LED étanche
// IP65 de 1 500 mm : boîtier gris clair (polycarbonate) à dos plat contre la dalle, diffuseur opale bombé, embouts et deux clips
// inox ; section (largeur 100 mm, hauteur 75 mm) extrudée le long de x, normales lissées sur le bombé
void reglette(Geo& boitier, Geo& diffuseur, Geo& acier, vec3 c) {
    constexpr float L = 1.50f, DEMI = L / 2;
    // profil du boîtier (z, y) de la dalle vers le bas : flancs légèrement rentrants, lèvres de fixation du diffuseur
    const std::vector<vec2> corps = {{-0.050f, 0.f}, {-0.050f, -0.010f}, {-0.047f, -0.032f}, {-0.045f, -0.036f}};
    // diffuseur : demi-ellipse de 90 mm × 39 mm sous le boîtier
    std::vector<vec2> bombe;
    for (int k = 0; k <= 12; k++) { const float a = PI * (float)k / 12.f; bombe.push_back({-0.045f * std::cos(a), -0.036f - 0.039f * std::sin(a)}); }
    auto extruder = [&](Geo& g, std::vector<vec2> prof, bool lisse) {
        // côté gauche puis miroir à droite pour le boîtier ; profil fermé implicite par la dalle (dos invisible)
        const int m = (int)prof.size();
        for (int seg = 0; seg + 1 < m; seg++) {
            const vec2 p0 = prof[(size_t)seg], p1 = prof[(size_t)seg + 1];
            auto normale = [&](int j) {
                const vec2 a = prof[(size_t)std::max(0, j - 1)], b = prof[(size_t)std::min(m - 1, j + 1)];
                vec2 t{b.x - a.x, b.y - a.y}; const float l = std::sqrt(t.x * t.x + t.y * t.y);
                // normale sortante (vers l'extérieur du profil : côté où z s'éloigne de 0 ou vers le bas)
                vec3 n{0, -t.x / l, t.y / l};
                const vec2 mid{(a.x + b.x) / 2, (a.y + b.y) / 2};
                if (n.z * mid.x + n.y * (mid.y + 0.036f) < 0) n = n * -1.f;
                return n;
            };
            vec3 n0 = lisse ? normale(seg) : normale(seg), n1 = lisse ? normale(seg + 1) : normale(seg);
            if (!lisse) {   // facette : normale du segment seul
                vec2 t{p1.x - p0.x, p1.y - p0.y}; const float l = std::sqrt(t.x * t.x + t.y * t.y);
                vec3 n{0, -t.x / l, t.y / l}; const vec2 mid{(p0.x + p1.x) / 2, (p0.y + p1.y) / 2};
                if (n.z * mid.x + n.y * (mid.y + 0.036f) < 0) n = n * -1.f;
                n0 = n1 = n;
            }
            const uint32_t k0 = (uint32_t)g.v.size();
            g.v.push_back({c.x - DEMI, c.y + p0.y, c.z + p0.x, n0.x, n0.y, n0.z, 0, 0});
            g.v.push_back({c.x + DEMI, c.y + p0.y, c.z + p0.x, n0.x, n0.y, n0.z, 0, 0});
            g.v.push_back({c.x + DEMI, c.y + p1.y, c.z + p1.x, n1.x, n1.y, n1.z, 0, 0});
            g.v.push_back({c.x - DEMI, c.y + p1.y, c.z + p1.x, n1.x, n1.y, n1.z, 0, 0});
            const vec3 e1{1, 0, 0}, e2{0, p1.y - p0.y, p1.x - p0.x};
            if (dot(cross(e1, e2), n0 + n1) >= 0) g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
            else g.i.insert(g.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
        }
    };
    std::vector<vec2> droite = corps;
    for (vec2& p : droite) p.x = -p.x;
    extruder(boitier, corps, false);
    extruder(boitier, droite, false);
    // lèvre plate entre le boîtier et le diffuseur (2 mm de chaque côté)
    extruder(boitier, {{-0.045f, -0.036f}, {-0.043f, -0.036f}}, false);
    extruder(boitier, {{0.043f, -0.036f}, {0.045f, -0.036f}}, false);
    extruder(diffuseur, bombe, true);
    // embouts : profil complet fermé (boîtier + diffuseur), 6 mm d'épaisseur, gris du boîtier
    for (int e : {-1, 1}) {
        std::vector<vec3> pts;
        for (const vec2& p : corps) pts.push_back({c.x + e * (DEMI + 0.006f), c.y + p.y, c.z + p.x});
        for (size_t k = 1; k + 1 < bombe.size(); k++) pts.push_back({c.x + e * (DEMI + 0.006f), c.y + bombe[k].y, c.z + bombe[k].x});
        for (auto it = droite.rbegin(); it != droite.rend(); ++it) pts.push_back({c.x + e * (DEMI + 0.006f), c.y + it->y, c.z + it->x});
        const uint32_t k0 = (uint32_t)boitier.v.size();
        const vec3 n{(float)e, 0, 0};
        for (const vec3& p : pts) boitier.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, 0, 0});
        for (uint32_t k = 1; k + 1 < (uint32_t)pts.size(); k++) {
            if (e > 0) boitier.i.insert(boitier.i.end(), {k0, k0 + k, k0 + k + 1});
            else boitier.i.insert(boitier.i.end(), {k0, k0 + k + 1, k0 + k});
        }
        // tranche de l'embout (6 mm) : bande qui fait le tour du profil, entre la face du bout et le corps
        const size_t np = pts.size();
        for (size_t k = 0; k < np; k++) {
            const vec3 p = pts[k], q = pts[(k + 1) % np];
            const vec3 t = normalize(q - p);
            vec3 nn = normalize(cross(t, vec3{1, 0, 0}));
            if (dot(nn, (p + q) * 0.5f - vec3{p.x, c.y - 0.036f, c.z}) < 0) nn = nn * -1.f;   // vers l'extérieur du profil
            const float xi = c.x + e * DEMI;
            const uint32_t b = (uint32_t)boitier.v.size();
            boitier.v.push_back({p.x, p.y, p.z, nn.x, nn.y, nn.z, 0, 0});
            boitier.v.push_back({q.x, q.y, q.z, nn.x, nn.y, nn.z, 0, 0});
            boitier.v.push_back({xi, q.y, q.z, nn.x, nn.y, nn.z, 0, 0});
            boitier.v.push_back({xi, p.y, p.z, nn.x, nn.y, nn.z, 0, 0});
            const vec3 e1 = q - p, e2{xi - p.x, 0, 0};
            if (dot(cross(e1, e2), nn) >= 0) boitier.i.insert(boitier.i.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
            else boitier.i.insert(boitier.i.end(), {b, b + 2, b + 1, b, b + 3, b + 2});
        }
    }
    // clips inox en U autour du boîtier, à 0,30 m des bouts
    for (float x : {-DEMI + 0.30f, DEMI - 0.30f})
        mobilier::tube(Sortie(acier), MONDE, {{c.x + x, c.y, c.z - 0.052f}, {c.x + x, c.y - 0.034f, c.z - 0.050f}, {c.x + x, c.y - 0.034f, c.z + 0.050f}, {c.x + x, c.y, c.z + 0.052f}}, 0.0025f, 6, UV0, false);
}

// ---------------------------------------------------------------- sprinklers
// tête pendante (laiton) : raccord fileté 1/2", corps, deux bras de cadre et déflecteur denté (18 dents) ; vers le bas
void tete(Geo& laiton, vec3 c) {
    const Repere R{c, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    mobilier::tour(Sortie(laiton), R, {0, 0, 0}, {{0.f, -0.012f}, {0.0105f, -0.012f}, {0.0105f, 0.f}, {0.008f, 0.f}}, 10, UV0);   // raccord
    mobilier::tour(Sortie(laiton), R, {0, 0, 0}, {{0.f, -0.030f}, {0.006f, -0.030f}, {0.009f, -0.022f}, {0.0105f, -0.012f}}, 10, UV0);   // corps
    for (int s : {-1, 1}) mobilier::tube(Sortie(laiton), R, {{s * 0.007f, -0.024f, 0}, {s * 0.010f, -0.040f, 0}, {s * 0.003f, -0.052f, 0}}, 0.0018f, 5, UV0);
    mobilier::tour(Sortie(laiton), R, {0, -0.052f, 0}, {{0.f, -0.0015f}, {0.020f, -0.0015f}, {0.021f, 0.f}, {0.f, 0.f}}, 36, UV0, 18, 0.12f);   // déflecteur denté
}

// collecteur ou antenne de a à b (rayon r) avec ses suspentes : tige filetée M10 jusqu'à la dalle et collier autour du tube
void canalisation(Geo& rouge, Geo& acier, vec3 a, vec3 b, float r, float pas) {
    mobilier::tube(Sortie(rouge), MONDE, {a, b}, r, 12, UV0);
    const vec3 d = b - a; const float L = length(d);
    const vec3 t = d * (1.f / L), u = normalize(cross(t, {0, 1, 0}));
    for (float s = pas / 2; s < L; s += pas) {
        const vec3 p = a + t * s;
        mobilier::tube(Sortie(acier), MONDE, {{p.x, p.y + r + 0.006f, p.z}, {p.x, HAUTEUR_DALLE, p.z}}, 0.005f, 6, UV0, false);
        std::vector<vec3> collier;
        for (int k = 0; k <= 12; k++) { const float an = 2 * PI * (float)k / 12.f; collier.push_back(p + u * ((r + 0.004f) * std::cos(an)) + vec3{0, (r + 0.004f) * std::sin(an), 0}); }
        mobilier::tube(Sortie(acier), MONDE, collier, 0.003f, 5, UV0, false);
    }
}

// ---------------------------------------------------------------- butée de roue
// caoutchouc recyclé 550 × 150 × 100 mm : profil en trapèze aux arêtes arrondies (base 150 mm, dessus 70 mm) extrudé le long de x ;
// deux bandes jaunes rétroréfléchissantes de 80 mm moulées dans le profil (même section, autre matière : aucune pièce posée
// dessus) ; deux têtes de tire-fond Ø 28 mm à 75 mm des bouts ; embouts biseautés
void butee(Geo& noir, Geo& jaune, Geo& acier, vec3 c) {
    // profil (z, y), de la base avant à la base arrière par le dessus : flancs penchés, arêtes du dessus arrondies (r ≈ 12 mm)
    const float B = 0.075f, T = 0.035f, H = 0.10f;
    const std::vector<vec2> prof = {{-B, 0.f}, {-T - 0.012f, H - 0.012f}, {-T - 0.005f, H - 0.003f}, {-T + 0.004f, H},
                                    {T - 0.004f, H}, {T + 0.005f, H - 0.003f}, {T + 0.012f, H - 0.012f}, {B, 0.f}};
    const int m = (int)prof.size();
    auto troncon = [&](Geo& g, float x0, float x1, float biseau0, float biseau1) {
        // biseau : les bouts rentrent de 30 mm en haut (embout chanfreiné)
        auto X = [&](float x, float y, float bz) { return x + bz * (y / H); };
        for (int j = 0; j + 1 < m; j++) {
            const vec2 p0 = prof[(size_t)j], p1 = prof[(size_t)j + 1];
            vec3 n = normalize(vec3{0, p1.x - p0.x, -(p1.y - p0.y)});   // perpendiculaire au segment
            if (dot(n, vec3{0, (p0.y + p1.y) * 0.5f - 0.03f, (p0.x + p1.x) * 0.5f}) < 0) n = n * -1.f;   // vers l'extérieur du profil
            const uint32_t k0 = (uint32_t)g.v.size();
            g.v.push_back({c.x + X(x0, p0.y, biseau0), c.y + p0.y, c.z + p0.x, n.x, n.y, n.z, 0, 0});
            g.v.push_back({c.x + X(x1, p0.y, biseau1), c.y + p0.y, c.z + p0.x, n.x, n.y, n.z, 0, 0});
            g.v.push_back({c.x + X(x1, p1.y, biseau1), c.y + p1.y, c.z + p1.x, n.x, n.y, n.z, 0, 0});
            g.v.push_back({c.x + X(x0, p1.y, biseau0), c.y + p1.y, c.z + p1.x, n.x, n.y, n.z, 0, 0});
            const vec3 e1{1, 0, 0}, e2{0, p1.y - p0.y, p1.x - p0.x};
            if (dot(cross(e1, e2), n) >= 0) g.i.insert(g.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
            else g.i.insert(g.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
        }
        for (int e = 0; e < 2; e++) {   // faces des bouts (seulement aux extrémités de la butée)
            const float bz = e ? biseau1 : biseau0;
            if (bz == 0.f) continue;
            const float x = e ? x1 : x0;
            const uint32_t k0 = (uint32_t)g.v.size();
            const vec3 n = normalize(vec3{e ? 1.f : -1.f, std::fabs(bz) / H, 0});
            for (const vec2& p : prof) g.v.push_back({c.x + X(x, p.y, bz), c.y + p.y, c.z + p.x, n.x, n.y, n.z, 0, 0});
            for (uint32_t k = 1; k + 1 < (uint32_t)m; k++) {
                if (e) g.i.insert(g.i.end(), {k0, k0 + k, k0 + k + 1}); else g.i.insert(g.i.end(), {k0, k0 + k + 1, k0 + k});
            }
        }
    };
    const float L = 0.275f, b0 = 0.06f, b1 = 0.14f;   // bandes jaunes de 80 mm, de 60 à 140 mm du centre, de chaque côté
    troncon(noir, -L, -b1, 0.03f, 0.f); troncon(jaune, -b1, -b0, 0.f, 0.f); troncon(noir, -b0, b0, 0.f, 0.f);
    troncon(jaune, b0, b1, 0.f, 0.f); troncon(noir, b1, L, 0.f, -0.03f);
    for (float x : {-0.20f, 0.20f}) {   // têtes de tire-fond (rondelle + tête bombée), dans le dessus noir
        const Repere R{{c.x + x, c.y + H - 0.004f, c.z}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        mobilier::tour(Sortie(acier), R, {0, 0, 0}, {{0.f, 0.f}, {0.014f, 0.f}, {0.014f, 0.0025f}, {0.011f, 0.003f}, {0.008f, 0.0065f}, {0.f, 0.0075f}}, 12, UV0);
    }
}

Mesh envoyer(Renderer& r, const Geo& g, bool& ok) {
    if (g.i.empty()) return {};
    Mesh m = r.essayerMesh(g.v, g.i);
    if (m.vb.buf == VK_NULL_HANDLE || m.ib.buf == VK_NULL_HANDLE) ok = false;
    return m;
}

} // namespace

bool Parking::init(Renderer& r) {
    Geo g[NB_MATS];
    // ---- dalle du sol : panneaux de béton surfacé de 5 m (joints sciés de 4 mm), sol époxy gris
    for (float x = -X_MAX; x < X_MAX - 0.01f; x += 5.f)
        for (float z = Z_FACE; z < Z_FOND - 0.01f; z += 5.f) {
            const float z1 = std::min(z + 5.f, Z_FOND);
            pave(g[SOL], {x + 2.5f, -0.10f, (z + z1) / 2}, {2.5f, 0.10f, (z1 - z) / 2}, 0.004f);
        }
    // ---- sous-face de la dalle : prédalles de 2,40 m (joints en V de 1 cm), le long de z
    for (float x = -X_MAX - 0.5f; x < X_MAX + 0.5f; x += 2.4f)
        pave(g[BETON], {x + 1.2f, HAUTEUR_DALLE + 0.125f, (Z_FOND + Z_FACE) / 2}, {1.2f, 0.125f, (Z_FOND - Z_FACE) / 2 + 0.3f}, 0.01f);
    // ---- poutres retombées (45 × 40 cm) sur les files de poteaux, dans les deux sens
    for (float z : Z_POTEAUX) pave(g[BETON], {0, SOUS_POUTRE + RETOMBEE_POUTRE / 2, z}, {X_MAX + 0.5f, RETOMBEE_POUTRE / 2, 0.225f}, 0.02f);
    for (float x : X_POTEAUX) pave(g[BETON], {x, SOUS_POUTRE + RETOMBEE_POUTRE / 2, (Z_FOND + Z_FACE) / 2}, {0.225f, RETOMBEE_POUTRE / 2, (Z_FOND - Z_FACE) / 2}, 0.02f);
    // ---- poteaux (bande de danger) sous les croisements des poutres
    for (float z : Z_POTEAUX) for (float x : X_POTEAUX) poteau(g[BETON], g[JAUNE], g[NOIR], x, z, SOUS_POUTRE);
    // ---- voiles banchés : panneaux de 2,50 m (joints de reprise de 8 mm) au fond, en face et sur les côtés
    for (float x = -X_MAX - 0.5f; x < X_MAX + 0.5f; x += 2.5f) {
        pave(g[BETON], {x + 1.25f, HAUTEUR_DALLE / 2, Z_FOND + 0.10f}, {1.25f, HAUTEUR_DALLE / 2, 0.10f}, 0.008f);
        pave(g[BETON], {x + 1.25f, HAUTEUR_DALLE / 2, Z_FACE - 0.10f}, {1.25f, HAUTEUR_DALLE / 2, 0.10f}, 0.008f);
    }
    for (float s : {-1.f, 1.f})
        for (float z = Z_FACE; z < Z_FOND - 0.01f; z += 2.5f) {
            const float z1 = std::min(z + 2.5f, Z_FOND);
            pave(g[BETON], {s * (X_MAX + 0.6f), HAUTEUR_DALLE / 2, (z + z1) / 2}, {0.10f, HAUTEUR_DALLE / 2, (z1 - z) / 2}, 0.008f);
        }
    // ---- bordures chasse-roues en béton (15 × 25 cm) le long des voiles, nez peint en jaune
    for (float zb : {Z_FOND - 0.125f, Z_FACE + 0.125f}) {
        const float sens = zb > 0 ? -1.f : 1.f;
        pave(g[BETON], {0, 0.075f, zb}, {X_MAX + 0.5f, 0.075f, 0.125f}, 0.015f);
        pave(g[JAUNE], {0, 0.075f, zb + sens * 0.1265f}, {X_MAX + 0.5f, 0.06f, 0.0015f}, 0.0005f);   // peinture du nez (3 mm)
    }
    uvMetres(g[BETON], 0);   // béton : uv en mètres (poteaux compris : leurs uv sont déjà u = tour, v = hauteur, recalculés pareil)
    uvMetres(g[SOL], 0);
    // ---- marquage peint (12 cm, 2 mm d'épaisseur) : séparations des places des deux rangées
    for (int grp = -2; grp <= 2; grp++)
        for (float dx : {-3.75f, -1.25f, 1.25f, 3.75f}) {
            const float x = 8.f * (float)grp + dx;
            if (std::fabs(x) > X_MAX - 0.3f) continue;
            pave(g[BLANC], {x, 0.001f, Z_ALLEE0 + PLACE_P / 2 - 0.15f}, {0.06f, 0.001f, PLACE_P / 2 - 0.15f}, 0.0005f);
            pave(g[BLANC], {x, 0.001f, Z_ALLEE1 - PLACE_P / 2 + 0.15f}, {0.06f, 0.001f, PLACE_P / 2 - 0.15f}, 0.0005f);
        }
    // ---- butées de roue (caoutchouc 550 × 150 × 100 mm, 2 par place, bandes jaunes de 80 mm)
    for (int grp = -2; grp <= 2; grp++)
        for (float dp : {-2.5f, 0.f, 2.5f}) {
            const float xp = 8.f * (float)grp + dp;
            if (std::fabs(xp) > X_MAX - 1.f) continue;
            for (float rangee : {1.70f, Z_ALLEE1 - PLACE_P + 0.80f})
                for (float s : {-0.55f, 0.55f}) butee(g[CAOUTCHOUC], g[JAUNE], g[ACIER], {xp + s, 0.f, rangee});
        }
    // ---- réglettes LED en plafond : allée (tous les 4 m) et milieu de chaque groupe de places
    for (float x = -18.f; x <= 18.f; x += 4.f) reglette(g[ALU], g[OPALE], g[ACIER], {x, HAUTEUR_DALLE, (Z_ALLEE0 + Z_ALLEE1) / 2});
    for (float x = -16.f; x <= 16.f; x += 8.f)
        for (float z : {0.20f, Z_ALLEE1 - PLACE_P / 2}) reglette(g[ALU], g[OPALE], g[ACIER], {x, HAUTEUR_DALLE, z});
    // ---- sprinklers : collecteur DN 80 le long de l'allée, antennes DN 25 vers les places, têtes tous les 3 m
    const float yc = HAUTEUR_DALLE - 0.16f, ya = HAUTEUR_DALLE - 0.12f;
    canalisation(g[ROUGE], g[ACIER], {-X_MAX, yc, Z_ALLEE0 - 0.6f}, {X_MAX, yc, Z_ALLEE0 - 0.6f}, 0.0445f, 2.5f);
    for (float x = -14.f; x <= 14.f; x += 4.f) {
        canalisation(g[ROUGE], g[ACIER], {x + 2.f, ya, Z_ALLEE0 - 0.6f}, {x + 2.f, ya, Z_FOND - 0.6f}, 0.0168f, 2.5f);
        for (float z = Z_ALLEE0 + 0.4f; z < Z_FOND - 0.6f; z += 3.f) tete(g[LAITON], {x + 2.f, ya - 0.0168f, z});
    }
    // ---- envoi : un maillage par matériau ; mémoire graphique refusée → rien n'est gardé (repli : fond d'avant)
    bool ok = true;
    triangles_ = 0;
    for (int k = 0; k < NB_MATS; k++) { mesh_[k] = envoyer(r, g[k], ok); triangles_ += g[k].i.size() / 3; }
    if (!ok) {
        WFS_ERR("menu (WFS-552) : mémoire graphique refusée pour le parking : fond d'avant gardé (repli)");
        r.vk().waitIdle();
        for (Mesh& m : mesh_) if (m.vb.buf != VK_NULL_HANDLE || m.ib.buf != VK_NULL_HANDLE) r.destroy(m);
        for (Mesh& m : mesh_) m = Mesh{};
        return pret_ = false;
    }
    // ---- matériaux (albédo linéaire, DA-realisme § 3)
    auto mat = [&](int k, vec3 c, float rough, float metal, uint32_t flags = 0, float cutoff = 0.5f, vec3 em = {0, 0, 0}) {
        Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal; m.flags = flags; m.cutoff = cutoff; m.emissive = em;
        mat_[k] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        mat_[k].flags |= flags;
    };
    mat(BETON, {0.30f, 0.30f, 0.29f}, 0.90f, 0, MAT_FACADE, 0.2f * 4);    // béton brut de décoffrage (appareil « béton » de decor.glsl)
    mat(SOL, {0.12f, 0.125f, 0.13f}, 0.26f, 0, MAT_FACADE, 0.2f * 4);     // dalle surfacée et vernie (résine époxy gris anthracite clair)
    mat(BLANC, {0.72f, 0.72f, 0.70f}, 0.55f, 0);                          // peinture routière blanche
    mat(JAUNE, lin(0xF0B20A), 0.50f, 0);                                  // jaune de signalisation (RAL 1003)
    mat(NOIR, lin(0x1C1C1E), 0.55f, 0);                                   // noir de signalisation (RAL 9004)
    mat(ROUGE, lin(0xA52A1E), 0.40f, 0);                                  // rouge feu RAL 3000, peinture glycéro
    mat(ALU, lin(0xC4C8CC), 0.45f, 0);                                    // boîtier en polycarbonate gris clair
    mat(OPALE, {0.80f, 0.80f, 0.80f}, 0.30f, 0, 0, 0.5f, vec3{1.0f, 0.96f, 0.90f} * 9.f);   // diffuseur opale allumé (4 000 K)
    mat(CAOUTCHOUC, lin(0x18181A), 0.90f, 0);                             // caoutchouc recyclé
    mat(ACIER, {0.56f, 0.57f, 0.58f}, 0.45f, 1);                          // acier galvanisé / inox
    mat(LAITON, {0.91f, 0.78f, 0.42f}, 0.35f, 1);                         // laiton des têtes de sprinkler
    WFS_LOG("menu (WFS-552) : parking souterrain préparé : %zu triangles, %d appels de dessin", triangles_, appels());
    return pret_ = true;
}

int Parking::appels() const { int n = 0; for (const Mesh& m : mesh_) if (m.indexCount) n++; return n; }

void Parking::shutdown(Renderer& r) {
    if (!pret_) return;
    for (Mesh& m : mesh_) if (m.indexCount) r.destroy(m);
    for (Material& m : mat_) if (m.set != VK_NULL_HANDLE) r.destroy(m);
    for (Mesh& m : mesh_) m = Mesh{};
    for (Material& m : mat_) m = Material{};
    pret_ = false;
}

void Parking::regler(FrameParams& fp, SceneDediee& s, const GameView& g, const CarDef& car, const mat4& M) const {
    const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
    Cadrage c = cadrage(car.d.L, car.d.W, car.d.H, aspect, banc::cap >= 0 ? banc::cap : 0.42f);
    if (banc::decor) { c.oeil = {-2.6f, 1.45f, -6.4f}; c.cible = {4.0f, 1.9f, 1.6f}; c.decalage = c.decalageY = 0; c.fovY = 50.f * PI / 180.f; }   // poteau du fond à droite
    const vec3 oeil = M.transformPoint(c.oeil), cible = M.transformPoint(c.cible), haut = normalize(M.transformDir({0, 1, 0}));
    fp.view = lookAt(oeil, cible, haut);
    fp.proj = translate({c.decalage, -c.decalageY, 0}) * perspectiveVk(c.fovY, aspect, 0.1f);   // objectif décentré (y de Vulkan vers le bas)
    fp.camPos = oeil;
    // souterrain : aucune lumière du ciel, ni soleil ni lune (RG-06 : même image de jour et de nuit) ; fond noir derrière les
    // voiles (jamais vu) ; exposition du garage
    fp.exposure = 1.15f;
    fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = vec3{0.004f, 0.004f, 0.005f};
    // WFS-550 (maquette 01) : « ciel » de reflets du studio (jamais vu : le parking le cache), pour que la carrosserie brille comme
    // sur la maquette (réglettes et murs clairs reflétés) ; avant : noir, carrosserie mate
    fp.skyTop = vec3{0.10f, 0.10f, 0.11f}; fp.skyHorizon = vec3{0.42f, 0.38f, 0.33f}; fp.ground = vec3{0.02f, 0.02f, 0.025f};
    fp.fogDensity = 0; fp.fogNear = 30; fp.fogFar = 80;
    fp.sunIntensity = 0; fp.ambient = 1; fp.shadows = false; fp.night = false;
    fp.feux = false; fp.mouille = false; fp.reflets = false;
    // lumière renvoyée par le béton (albédo 0,30) : ambiance grise, plus sombre vers le sol
    fp.hemiSky = vec3{0.36f, 0.36f, 0.37f}; fp.hemiGround = vec3{0.08f, 0.08f, 0.08f}; fp.hemiIntensity = 0.20f;   // WFS-550 (maquette 01) : fond plus sombre (0,26 avant)
    // réglettes : un projecteur large (≈ 120°) sous chaque réglette proche de la voiture, 4 000 K
    const vec3 led{1.0f, 0.93f, 0.84f};
    int n = 0;
    auto spot = [&](vec3 p, float force) {
        if (n >= FrameParams::SPOTS_MAX) return;
        FrameParams::Spot sp;
        sp.pos = M.transformPoint(p); sp.target = M.transformPoint(p - vec3{0, 2.f, 0});
        sp.color = led; sp.intensity = force; sp.distance = 16; sp.angle = 1.35f; sp.penumbra = 1.0f; sp.decay = 1.2f;
        fp.spots[n++] = sp;
    };
    const float zA = (Z_ALLEE0 + Z_ALLEE1) / 2, y = HAUTEUR_DALLE - 0.08f;
    // WFS-550 (maquette 01) : voiture plus éclairée, murs du fond plus sombres (avant : 2,4 / 1,4 ; 3,2 / 1,0 ; 0,8)
    for (float x : {-6.f, -2.f, 2.f, 6.f, 10.f, -10.f}) spot({x, y, zA}, std::fabs(x) < 3 ? 3.0f : 0.9f);   // les deux réglettes devant la voiture : sa face avant
    for (float x : {0.f, 8.f, -8.f}) spot({x, y, 0.20f}, x == 0 ? 4.2f : 0.7f);   // réglette au-dessus de la voiture
    for (float x : {0.f, 8.f}) spot({x, y, Z_ALLEE1 - PLACE_P / 2}, 0.5f);
    fp.spotCount = n;
    fp.bloom = 0.22f;   // avis de l'agent 3D : halo des réglettes resserré
    s.route = false; s.joueur = true; s.commandes = false; s.cible = cible;
}

void Parking::dessiner(Renderer& r, const mat4& M) {
    if (!pret_) return;
    for (int k = 0; k < NB_MATS; k++) if (mesh_[k].indexCount) r.draw(mesh_[k], mat_[k], M);
}

} // namespace wfs::ecrans::parking
