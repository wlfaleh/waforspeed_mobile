// WaForSpeed — intérieurs par modèle de la vue cockpit (WFS-197) : voir vehicule_cockpit.h.
#include "vehicule_cockpit.h"
#include <algorithm>
#include <cstring>
#include "../core/log.h"
#include "vehicule_cockpit_details.h"   // WFS-325 : pièces propres au modèle (lot 1 : Golf, M4, 911, SF90)
#include "vehicule_cockpit_formes.h"   // WFS-414 : pièces modelées partagées avec le volant
#include "vehicule_cockpit_geo.h"
#include "retroviseurs.h"   // WFS-324 : glaces à reflet
#include "vehicule_cockpit_volant.h"   // WFS-414 : volant et visière calculés sans rendu
#include "vk.h"

namespace wfs::cockpit {

// WFS-282 : pièces construites par code, vues de l'intérieur : jamais de retrait des faces arrière (WFS-246 retire les faces
// arrière des matériaux opaques sans MAT_DOUBLE : le dessus de la planche de bord disparaissait, on voyait la route au travers)
static Material deuxFaces(Material m) { m.flags |= MAT_DOUBLE; return m; }
// WFS-323 (ANO-185) : pièces éclairées de l'habitacle : occlusion rangée dans la longueur de la normale (occlusionHabitacle)
static Material habitacle(Material m) { m.flags |= MAT_DOUBLE | MAT_AO; return m; }
// WFS-282 (RG-04) : lumière d'ambiance de nuit de l'habitacle, en part de l'albédo de chaque matière (émission, mêmes unités que les écrans)
constexpr float AMBIANCE_NUIT = 0.25f;

bool g_interieurCle = false;   // banc « cockpit » (WFS-235, RG-05) : voir vehicule_cockpit.h
bool g_gabaritCeinture = false;   // WFS-349 (RG-03) : banc « cockpit-voisine » : voir vehicule_cockpit.h
bool g_bancCockpitVoisine = false;   // WFS-349 : gabarit et clé douce construits (banc seulement)
PareBriseVu& pareBriseVu() { static PareBriseVu p; return p; }   // WFS-334 : voir vehicule_cockpit.h
const Material& Interieur::mat(const Material& m) const {
    if (g_interieurCle && g_gabaritCeinture && cleDouce_.set) return cleDouce_;   // WFS-349 : clé douce du banc « cockpit-voisine »
    return g_interieurCle && cleMat_.set ? cleMat_ : m;
}

// définitions par modèle (DEFS, interieurPour) : vehicule_cockpit_defs.cpp (sans rendu : testées par tests/test_cockpit.cpp)

// ---------------------------------------------------------------- géométrie
namespace {
using namespace formes;   // WFS-414 : pièces modelées (vehicule_cockpit_formes.h)

// centre de la voiture dans le repère de l'œil : Interieur::cx_ (WFS-315 : écart œil-axe du modèle, 0,36 m avant)

vec3 lin(unsigned h) {
    auto l = [](float c) { return std::pow(c, 2.2f); };
    return {l(((h >> 16) & 255) / 255.f), l(((h >> 8) & 255) / 255.f), l((h & 255) / 255.f)};
}

// profil (y, z) extrudé le long de x, de x0 à x1 : une bande par segment du profil ; normale dans le plan (y, z)
void extruder(Geo& g, const mat4& M, const std::vector<vec2>& profil, float x0, float x1) {
    float s = 0;
    for (size_t k = 0; k + 1 < profil.size(); k++) {
        const vec2 a = profil[k], b = profil[k + 1];
        const vec2 d{b.x - a.x, b.y - a.y};
        const float l = std::sqrt(d.x * d.x + d.y * d.y);
        if (l < 1e-5f) continue;
        const vec3 n = normalize(vec3{0, -d.y, d.x});   // profil parcouru du pare-brise vers le conducteur : normale vers le haut / l'œil
        const uint32_t b0 = (uint32_t)g.v.size();
        ajouterUV(g, M, {x0, a.x, a.y}, n, x0 * GRAIN, s * GRAIN); ajouterUV(g, M, {x1, a.x, a.y}, n, x1 * GRAIN, s * GRAIN);
        ajouterUV(g, M, {x1, b.x, b.y}, n, x1 * GRAIN, (s + l) * GRAIN); ajouterUV(g, M, {x0, b.x, b.y}, n, x0 * GRAIN, (s + l) * GRAIN);
        g.quad(b0, b0 + 1, b0 + 2, b0 + 3);
        s += l;
    }
    // joues aux deux bouts (le profil fermé par le bas)
    for (float x : {x0, x1}) {
        const uint32_t c0 = (uint32_t)g.v.size();
        const vec3 n{x == x0 ? -1.f : 1.f, 0, 0};
        float yc = 0, zc = 0; for (const vec2& p : profil) { yc += p.x; zc += p.y; }
        yc /= (float)profil.size(); zc /= (float)profil.size();
        ajouterUV(g, M, {x, yc, zc}, n, 0, 0);
        for (const vec2& p : profil) ajouterUV(g, M, {x, p.x, p.y}, n, p.y, p.x);
        for (size_t k = 0; k + 1 < profil.size(); k++) g.i.insert(g.i.end(), {c0, c0 + 1 + (uint32_t)k, c0 + 2 + (uint32_t)k});
    }
}


// aérateur : cadre (accent) et lames sombres, face +z, centré en c, taille w × h
// WFS-323 : lunette moulée à bords arrondis (cadreArrondi) et lames profilées (dalles fines à bords ronds), au lieu de boîtes
void aerateur(Geo& cadre, Geo& lames, const mat4& M, float w, float h, bool rond) {
    if (rond) {   // aérateur rond (911, Ferrari, Abarth) : lunette ronde de 10 mm, 5 lames horizontales coupées à la forme du rond
        const float d = std::min(w, h);
        cadreArrondi(cadre, M, d, d, d * 0.5f, d * 0.10f, 0.014f);
        for (int k = -2; k <= 2; k++) {
            const float y = k * d * 0.15f, l = 2 * std::sqrt(std::max(0.f, d * d * 0.16f - y * y));
            if (l > 0.01f) dalleFace(lames, M * translate({0, y, -0.006f}) * rotateX(0.35f), l, d * 0.05f, 0.003f, d * 0.025f, 0.001f);
        }
        return;
    }
    const float e = std::min(w, h) * 0.10f;
    cadreArrondi(cadre, M, w, h, std::min(w, h) * 0.30f, e, 0.014f);
    const int n = std::max(2, (int)(h / 0.016f));
    for (int k = 0; k < n; k++) {
        const float y = -h / 2 + e + (h - 2 * e) * (k + 0.5f) / n, lh = (h - 2 * e) / n * 0.55f;
        dalleFace(lames, M * translate({0, y, -0.006f}) * rotateX(0.35f), w - 2 * e, lh, 0.003f, lh * 0.5f, 0.001f);
    }
}

// orientation d'un écran tourné vers l'œil (normale +z vers le conducteur), avec un lacet donné
mat4 versOeil(vec3 p, float lacet) { return translate(p) * rotateY(lacet) * rotateX(std::atan2(p.y, -p.z)); }

// normal map du grain du cuir (256 × 256, répétable) : bosses de cellules irrégulières (Worley simplifié) ; RGBA linéaire
std::vector<uint8_t> grainCuir(int N) {
    std::vector<float> h((size_t)N * N);
    uint32_t s = 0x2545F491u;
    auto rnd = [&]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 16777216.f; };
    const int C = 32; std::vector<vec2> pts((size_t)C * C);
    for (auto& p : pts) p = {rnd(), rnd()};
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        const float fx = (float)x / N * C, fy = (float)y / N * C; const int cx = (int)fx, cy = (int)fy;
        float d1 = 9, d2 = 9;
        for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
            const int gx = (cx + i + C) % C, gy = (cy + j + C) % C; const vec2 p = pts[(size_t)gy * C + gx];
            const float dx = cx + i + p.x - fx, dy = cy + j + p.y - fy, d = dx * dx + dy * dy;
            if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) d2 = d;
        }
        h[(size_t)y * N + x] = std::sqrt(d2) - std::sqrt(d1);   // creux le long des bords des cellules
    }
    std::vector<uint8_t> o((size_t)N * N * 4);
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        auto H = [&](int i, int j) { return h[(size_t)((j + N) % N) * N + (i + N) % N]; };
        const float dx = (H(x + 1, y) - H(x - 1, y)) * 1.2f, dy = (H(x, y + 1) - H(x, y - 1)) * 1.2f;
        const vec3 n = normalize(vec3{-dx, -dy, 1});
        uint8_t* q = &o[((size_t)y * N + x) * 4];
        q[0] = (uint8_t)std::lround((n.x * 0.5f + 0.5f) * 255); q[1] = (uint8_t)std::lround((n.y * 0.5f + 0.5f) * 255);
        q[2] = (uint8_t)std::lround((n.z * 0.5f + 0.5f) * 255); q[3] = 255;
    }
    return o;
}

// ---------- WFS-324 : rétroviseurs (repère de la glace G : x le long de la largeur, y en haut, +z = normale vers le conducteur)
// contour d'une glace w × h dans le plan (x, y) : coins côté extérieur de la voiture (signe sOut de x) de rayon rOut, les autres de
// rayon rIn (sOut = 0 : tous rIn) ; `n` pas par coin ; points dans le sens trigonométrique, normales vers l'extérieur
Contour contourGlace(float w, float h, float rIn, float rOut, float sOut, int n = 6) {
    Contour c;
    const vec2 coin[4] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};   // haut droit, haut gauche, bas gauche, bas droit
    for (int k = 0; k < 4; k++) {
        const float r = std::clamp(sOut != 0 && coin[k].x * sOut > 0 ? rOut : rIn, 0.001f, std::min(w, h) * 0.5f - 1e-4f);
        const vec2 ctr{coin[k].x * (w * 0.5f - r), coin[k].y * (h * 0.5f - r)};
        for (int j = 0; j <= n; j++) {
            const float a = (k + (float)j / n) * PI * 0.5f;
            const vec2 d{std::cos(a), std::sin(a)};
            c.p.push_back({ctr.x + d.x * r, ctr.y + d.y * r}); c.n.push_back(d);
        }
    }
    return c;
}
// glace : face plane à z = 0 (uv 0..1 sur le rectangle englobant, v vers le bas : texel du reflet, retroviseurs.h), chant de 2 mm
void glaceRetro(Geo& g, const mat4& G, float w, float h, float rIn, float rOut, float sOut) {
    const Contour c = contourGlace(w, h, rIn, rOut, sOut);
    const size_t n = c.p.size();
    // uv rentrés de 0,5 % : jamais lus au bord de l'image du reflet (l'échantillonneur répète la texture : un trait clair du bas de
    // l'image, la route, apparaissait en haut de la glace, vu au banc)
    constexpr float RENTRE = 0.99f;
    auto uv = [&](vec2 p) { return vec2{0.5f + RENTRE * p.x / w, 0.5f - RENTRE * p.y / h}; };
    const uint32_t c0 = (uint32_t)g.v.size();
    ajouterUV(g, G, {0, 0, 0}, {0, 0, 1}, 0.5f, 0.5f);
    for (size_t k = 0; k < n; k++) { const vec2 t = uv(c.p[k]); ajouterUV(g, G, {c.p[k].x, c.p[k].y, 0}, {0, 0, 1}, t.x, t.y); }
    for (size_t k = 0; k < n; k++) triOriente(g, c0, c0 + 1 + (uint32_t)k, c0 + 1 + (uint32_t)((k + 1) % n));
    const uint32_t a0 = (uint32_t)g.v.size();   // chant (verre de 2 mm), normales vers l'extérieur
    for (int z = 0; z < 2; z++) for (size_t k = 0; k < n; k++) {
        const vec2 t = uv(c.p[k]);
        ajouterUV(g, G, {c.p[k].x, c.p[k].y, z ? -0.002f : 0.f}, {c.n[k].x, c.n[k].y, 0}, t.x, t.y);
    }
    for (size_t k = 0; k < n; k++) {
        const uint32_t i0 = a0 + (uint32_t)k, i1 = a0 + (uint32_t)((k + 1) % n);
        quadOriente(g, i0, i1, i1 + (uint32_t)n, i0 + (uint32_t)n);
    }
}
// coque d'un rétroviseur extérieur : surface balayée de la lèvre (autour de la glace, 1 cm devant elle) au nez arrondi (11 cm devant),
// section à coins arrondis qui grossit puis se resserre et glisse vers la portière (côté −sOut) ; logement de la glace en retrait de
// 6 mm, fermé par un fond. Normales lissées (moyenne des faces voisines). Mesures : coques de série de 9 à 12 cm de profondeur (E)
void coqueRetro(Geo& g, const mat4& G, float w, float h, float rIn, float rOut, float sOut) {
    struct Station { float z, dw, dh, k, decal, nz; };   // dw, dh : marge autour de la glace (m) ; k : échelle ; decal : vers la portière (m)
    const Station S[] = {   // coque de 1,5 cm plus large que la glace de chaque côté (Golf 8 : glace ≈ 200 × 118, coque ≈ 230 × 150 mm, E)
        {0.003f, 0.003f, 0.003f, 1.f, 0, 0.9f},      // bord intérieur de la lèvre (3 mm autour de la glace)
        {0.008f, 0.008f, 0.008f, 1.f, 0, 0.75f},     // haut de la lèvre (arrondi)
        {0.004f, 0.013f, 0.013f, 1.f, 0, 0.f},       // dos de la lèvre
        {-0.030f, 0.016f, 0.016f, 1.f, 0, 0.f},
        {-0.060f, 0.015f, 0.014f, 0.93f, 0.010f, -0.15f},
        {-0.085f, 0.012f, 0.010f, 0.74f, 0.026f, -0.45f},
        {-0.100f, 0.008f, 0.006f, 0.46f, 0.040f, -0.75f},
        {-0.107f, 0.005f, 0.004f, 0.16f, 0.050f, -0.97f},
    };
    constexpr int NS = (int)(sizeof(S) / sizeof(S[0]));
    std::vector<std::vector<vec3>> P(NS);
    size_t n = 0;
    for (int s = 0; s < NS; s++) {
        const float ww = (w + 2 * S[s].dw) * S[s].k, hh = (h + 2 * S[s].dh) * S[s].k;
        const Contour c = contourGlace(ww, hh, (rIn + S[s].dw) * S[s].k, (rOut + S[s].dw) * S[s].k, sOut);
        n = c.p.size();
        for (size_t k = 0; k < n; k++) P[(size_t)s].push_back({c.p[k].x - sOut * S[s].decal, c.p[k].y, S[s].z});
    }
    // normales : produit vectoriel des tangentes (le long du contour, le long du balayage), orientées vers l'extérieur de la coque
    std::vector<uint32_t> debut((size_t)NS);
    for (int s = 0; s < NS; s++) {
        debut[(size_t)s] = (uint32_t)g.v.size();
        const vec3 axe{-sOut * S[s].decal, 0, S[s].z};
        for (size_t k = 0; k < n; k++) {
            const vec3 tk = P[(size_t)s][(k + 1) % n] - P[(size_t)s][(k + n - 1) % n];
            const vec3 ts = P[(size_t)std::min(s + 1, NS - 1)][k] - P[(size_t)std::max(s - 1, 0)][k];
            vec3 nn = normalize(cross(tk, ts));
            const vec3 dehors = normalize(vec3{P[(size_t)s][k].x - axe.x, P[(size_t)s][k].y - axe.y, 0} * (1 - std::fabs(S[s].nz)) + vec3{0, 0, S[s].nz});
            if (dot(nn, dehors) < 0) nn = nn * -1.f;
            if (s == 0) nn = dehors;   // bord de la lèvre : arrondi vers la glace
            const vec3 q = P[(size_t)s][k];
            ajouterUV(g, G, q, nn, q.x * GRAIN, (q.y + q.z) * GRAIN);
        }
    }
    for (int s = 0; s + 1 < NS; s++) for (size_t k = 0; k < n; k++) {
        const uint32_t i0 = debut[(size_t)s] + (uint32_t)k, i1 = debut[(size_t)s] + (uint32_t)((k + 1) % n);
        quadOriente(g, i0, i1, i1 + (uint32_t)n, i0 + (uint32_t)n);
    }
    {   // nez : calotte fermée
        const Station& st = S[NS - 1];
        const uint32_t c0 = (uint32_t)g.v.size();
        ajouterUV(g, G, {-sOut * st.decal, 0, st.z - 0.002f}, {0, 0, -1}, 0, 0);
        for (size_t k = 0; k < n; k++) triOriente(g, c0, debut[(size_t)NS - 1] + (uint32_t)k, debut[(size_t)NS - 1] + (uint32_t)((k + 1) % n));
    }
    {   // logement de la glace : paroi de la lèvre jusqu'à 6 mm derrière la glace, puis fond (normales vers l'intérieur du logement)
        const Contour c = contourGlace(w + 0.006f, h + 0.006f, rIn + 0.003f, rOut + 0.003f, sOut);
        const uint32_t a0 = (uint32_t)g.v.size();
        for (int z = 0; z < 2; z++) for (size_t k = 0; k < c.p.size(); k++)
            ajouterUV(g, G, {c.p[k].x, c.p[k].y, z ? -0.006f : 0.003f}, {-c.n[k].x, -c.n[k].y, 0}, c.p[k].x * GRAIN, c.p[k].y * GRAIN);
        const size_t m = c.p.size();
        for (size_t k = 0; k < m; k++) {
            const uint32_t i0 = a0 + (uint32_t)k, i1 = a0 + (uint32_t)((k + 1) % m);
            quadOriente(g, i0, i1, i1 + (uint32_t)m, i0 + (uint32_t)m);
        }
        const uint32_t c0 = (uint32_t)g.v.size();
        ajouterUV(g, G, {0, 0, -0.006f}, {0, 0, 1}, 0, 0);
        for (size_t k = 0; k < m; k++) triOriente(g, c0, a0 + (uint32_t)(m + k), a0 + (uint32_t)(m + (k + 1) % m));
    }
}

// ---------- matériaux de l'intérieur (indices de Interieur::mats_)
enum : int { M_CUIR, M_PLANCHE_HAUT, M_PLANCHE_BAS, M_ACCENT, M_LAQUE, M_BRANCHES, M_COUTURE0, M_COUTURE1, M_COUTURE2, M_REPERE,
             M_FOND_COMBINE, M_FOND_ECRAN, M_FOND_ECRAN_NUIT, M_LAMES, M_PAVILLON, M_MOYEU, M_CHROME, M_GLACE, M_PORTE,
             // WFS-325 : aluminium brossé, guide de lumière d'ambiance (jour : éteint ; nuit : allumé) des pièces propres du lot 1
             M_ALU, M_AMBIANCE, M_AMBIANCE_NUIT,
             // WFS-324 : glaces à reflet (intérieur, gauche, droite), coques et bras des rétroviseurs extérieurs (dehors : sans occlusion
             // de l'habitacle ni lumière d'ambiance de nuit de l'habitacle)
             M_RETRO0, M_RETRO1, M_RETRO2, M_COQUE_EXT, M_BRAS_EXT, M_N };
// écrans dynamiques
enum : int { D_ACTIF, D_FOND, D_ALERTE, D_ROUTE, D_FLECHE, D_N };
constexpr size_t DYN_SOMMETS = 4096;

// ---------- dessin 2D dans le plan d'un écran (repère local : x à droite, y en haut, m ; z = décollement)
struct Plan2D {
    std::vector<Vertex>* v; std::vector<uint32_t>* i; float z;
    void quad(vec2 a, vec2 b, vec2 c, vec2 d) {
        if (v->size() + 4 > DYN_SOMMETS) return;
        const uint32_t k = (uint32_t)v->size();
        for (vec2 p : {a, b, c, d}) v->push_back({p.x, p.y, z, 0, 0, 1, 0, 0});
        i->insert(i->end(), {k, k + 1, k + 2, k, k + 2, k + 3});
    }
    void rect(float x0, float y0, float x1, float y1) { quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}); }
    // trait de c, de longueur l et largeur w, orienté par a (rad, 0 = vers la droite)
    void trait(vec2 c, float a, float l0, float l1, float w) {
        const vec2 d{std::cos(a), std::sin(a)}, n{-d.y * w / 2, d.x * w / 2};
        const vec2 p0{c.x + d.x * l0, c.y + d.y * l0}, p1{c.x + d.x * l1, c.y + d.y * l1};
        quad({p0.x - n.x, p0.y - n.y}, {p1.x - n.x, p1.y - n.y}, {p1.x + n.x, p1.y + n.y}, {p0.x + n.x, p0.y + n.y});
    }
    void segment(vec2 a, vec2 b, float w) {
        const float dx = b.x - a.x, dy = b.y - a.y, l = std::sqrt(dx * dx + dy * dy);
        if (l < 1e-6f) return;
        trait(a, std::atan2(dy, dx), -w / 2, l + w / 2, w);
    }
};
// chiffre à 7 segments de hauteur h, coin bas gauche (x, y)
void chiffre(Plan2D& p, float x, float y, float h, uint8_t seg) {
    const float w = h * 0.52f, e = h * 0.13f, m = y + h / 2;
    if (seg & 1) p.rect(x + e, y + h - e, x + w - e, y + h);
    if (seg & 2) p.rect(x + w - e, m, x + w, y + h - e);
    if (seg & 4) p.rect(x + w - e, y + e, x + w, m);
    if (seg & 8) p.rect(x + e, y, x + w - e, y + e);
    if (seg & 16) p.rect(x, y + e, x + e, m);
    if (seg & 32) p.rect(x, m, x + e, y + h - e);
    if (seg & 64) p.rect(x + e, m - e / 2, x + w - e, m + e / 2);
}
// nombre aligné à droite sur xd ; zéros de tête omis
void nombre(Plan2D& p, float xd, float y, float h, int n, int chiffresMax) {
    const float pas = h * 0.66f;
    int k = 0;
    do { chiffre(p, xd - pas * (k + 1), y, h, segments7(n % 10)); n /= 10; k++; } while (n > 0 && k < chiffresMax);
}
void rapport(Plan2D& p, float x, float y, float h, int r) {
    chiffre(p, x, y, h, r > 0 ? segments7(std::min(r, 9)) : r < 0 ? SEG_R : SEG_N);
}
// arc de n barres de rayon ri → ro, de l'angle a0 à a1 ; les barres sous `part` vont dans `allume`, les autres dans
// `eteint` ; au-delà de `rouge` (part de l'arc), les barres allumées vont dans `alerte`
void arcBarres(Plan2D& allume, Plan2D& eteint, Plan2D& alerte, vec2 c, float ri, float ro, float a0, float a1, int n, float part,
               float rouge, float largeur) {
    for (int k = 0; k < n; k++) {
        const float f = (k + 0.5f) / n, a = a0 + (a1 - a0) * f;
        Plan2D& cible = f <= part ? (f >= rouge ? alerte : allume) : eteint;
        cible.trait(c, a, ri, ro, largeur);
    }
}

// WFS-201 : cadran à aiguille (arc de 270°, de 225° à −45°) : graduations (fond), zone rouge au-delà de `rouge` (alerte), aiguille
// à la part `part` (actif), pivot au centre ; partagé par les combinés analogiques du lot 2 (Audi, Alfa, Lamborghini)
void cadran(Plan2D& F, Plan2D& A, Plan2D& R, vec2 c, float r, float part, float rouge, int graduations, float epaisseur) {
    arcBarres(F, F, R, c, r * 0.84f, r, PI * 1.25f, -PI * 0.25f, graduations, 1.f, rouge, epaisseur);
    A.trait(c, PI * 1.25f - PI * 1.5f * std::clamp(part, 0.f, 1.f), -r * 0.12f, r * 0.90f, epaisseur * 1.3f);
}
// lettre « d » (marche avant) des voitures à boîte automatique sans rapports affichés (Model 3)
constexpr uint8_t SEG_D = 0x5E;

} // namespace

// ---------------------------------------------------------------- construction
// WFS-323 (ANO-185) : occlusion de l'habitacle, rangée dans la longueur de la normale (MAT_AO, même codage que le décor de
// WFS-269) : sous le toit, une pièce ne voit le ciel que par les vitres ; elle reçoit une part de l'ambiance du ciel et de la sonde
// d'éclairage selon son orientation : vers le pare-brise (−z) et vers le haut, plus ; vers le bas et vers l'arrière, moins (0,3 à
// 0,8 : estimé, ordre de grandeur d'une occlusion d'habitacle). Sans elle, toute la planche prenait la sonde en plein : quand la
// sonde se recapture (nuit, lampadaires), la planche passait d'un coup du noir au blanc (ANO-185). Écrans et glace : sans occlusion
constexpr float AO_MIN = 0.3f, AO_MAX = 0.8f;
static float occlusionHabitacle(vec3 n) {
    return std::clamp(0.35f + 0.35f * std::max(0.f, n.y) + 0.2f * std::max(0.f, -n.z), AO_MIN, AO_MAX);
}

void Interieur::ajouter(Renderer& r, std::vector<Vertex>& v, std::vector<uint32_t>& i, int mat, int attache) {
    if (v.empty()) return;
    if (mat != M_FOND_COMBINE && mat != M_FOND_ECRAN && mat != M_FOND_ECRAN_NUIT && mat != M_GLACE && mat < M_RETRO0)
        for (Vertex& s : v) {
            const vec3 n = normalize(vec3{s.nx, s.ny, s.nz}) * occlusionHabitacle(normalize(vec3{s.nx, s.ny, s.nz}));
            s.nx = n.x; s.ny = n.y; s.nz = n.z;
        }
    octets_ += v.size() * sizeof(Vertex) + i.size() * 4;
    pieces_.push_back({r.makeMesh(v, i), mat, attache});
    v.clear(); i.clear();
}

bool Interieur::construire(Renderer& r, const InterieurDef& d, const retro::Carrosserie& carrosserie) {
    detruire(r);
    def_ = &d; octets_ = 0;
    {   // WFS-315 (RG-01) : largeur de l'habitacle du modèle
        const Largeur l = largeurPour(d);
        cx_ = l.oeil; demi_ = l.coudes * 0.5f; montant_ = l.montants * 0.5f;
    }
    miroirs_ = retro::miroirs(d, cx_, demi_, carrosserie);   // WFS-324 : place des trois glaces
    for (int k : {retro::GAUCHE, retro::DROITE}) {
        const mat4& G = miroirs_[(size_t)k].repere;
        const vec3 dv = miroirs_[(size_t)k].decalageVue;
        WFS_LOG("rétroviseurs (WFS-324) : %s, glace %s %.0f × %.0f mm, centre (%.3f ; %.3f ; %.3f) m de l'œil ; %s (%+.2f ; %+.2f ; %+.2f m) ; plan proche %.2f m derrière",
                d.car, k == retro::GAUCHE ? "gauche" : "droite", miroirs_[(size_t)k].w * 1000, miroirs_[(size_t)k].h * 1000, G.at(0, 3), G.at(1, 3), G.at(2, 3),
                miroirs_[(size_t)k].mesure ? "reflet vu depuis le rétroviseur du modèle 3D" : "rétroviseur du modèle non trouvé, reflet vu depuis la glace", dv.x, dv.y, dv.z,
                miroirs_[(size_t)k].recul);
    }
    // matériaux (valeurs physiques : charte du réel, DA-realisme.md ; cuir albédo 0,04-0,12, rugosité 0,55-0,75)
    {
        const std::vector<uint8_t> px = grainCuir(256);
        grain_ = r.vk().uploadTexture(px.data(), 256, 256, false);
        octets_ += px.size();
    }
    mats_.assign(M_N, Material{});
    auto mat = [&](int k, unsigned c, float rough, float metal, bool grain = false) {
        Material m; const vec3 l = lin(c); m.color = {l.x, l.y, l.z, 1}; m.rough = rough; m.metal = metal;
        mats_[(size_t)k] = r.makeMaterial(habitacle(m), nullptr, grain && grain_.view ? &grain_ : nullptr, nullptr);
    };
    auto ecran = [&](int k, unsigned c, float force) {
        Material m; m.color = {0, 0, 0, 1}; m.rough = 0.2f; m.metal = 0; m.emissive = lin(c) * force;
        mats_[(size_t)k] = r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr);
    };
    // cuir du volant : couleur du cuir ramenée dans la plage de la charte (albédo linéaire ≥ 0,04 : cuir noir réel ≈ 0,04-0,05)
    {
        Material m; vec3 l = lin(d.cuir); const float y = 0.2126f * l.x + 0.7152f * l.y + 0.0722f * l.z;
        if (y < 0.04f) l = y > 1e-5f ? l * (0.045f / y) : vec3{0.045f, 0.045f, 0.045f};
        m.color = {l.x, l.y, l.z, 1}; m.rough = 0.65f; m.metal = 0;
        mats_[M_CUIR] = r.makeMaterial(habitacle(m), nullptr, grain_.view ? &grain_ : nullptr, nullptr);
    }
    mat(M_PLANCHE_HAUT, d.plancheHaut, 0.72f, 0, true);    // haut de planche : cuir ou matière moussée grainée
    mat(M_PLANCHE_BAS, d.plancheBas, 0.85f, 0, true);      // plastique grainé (aucun reflet métallique)
    {   // WFS-349 : portières, plancher et tablier : même plastique grainé que le bas de planche, jamais noir (couleurPorte)
        Material m; const vec3 l = couleurPorte(d); m.color = {l.x, l.y, l.z, 1}; m.rough = 0.85f; m.metal = 0;
        mats_[M_PORTE] = r.makeMaterial(habitacle(m), nullptr, grain_.view ? &grain_ : nullptr, nullptr);
    }
    // WFS-325 (RG-07) : lot 1 : carbone vernis et noir laqué dans la plage du vernis (0,02-0,08) ; les autres voitures : valeurs de main
    const bool lot1 = detailsPropres(d);
    mat(M_ACCENT, d.accent, lot1 && d.carbone ? RUGO_CARBONE_LOT1 : d.accentRugo, d.accentMetal);   // alu brossé (métal 1), carbone verni (0,3), noir laqué
    mat(M_LAQUE, 0x0a0a0b, lot1 ? RUGO_LAQUE_LOT1 : 0.12f, 0);                       // noir laqué des cadres d'écran, rétroviseur
    mat(M_BRANCHES, d.branchesCol, d.branchesCol > 0x505050 ? 0.35f : 0.3f, d.branchesCol > 0x505050 ? 1.f : 0.f);
    for (int k = 0; k < 3; k++) mat(M_COUTURE0 + k, d.couture[k] ? d.couture[k] : 0x202020, 0.7f, 0);
    // repères et commandes rouges (manettino, démarreur, boutons M, agrafe GTI) : plastique rouge satiné, teinte assombrie de
    // moitié (la couleur vive de la marque, vue sous le ciel sans toit au-dessus, sortait rose)
    {
        const unsigned c = d.repereCol ? d.repereCol : 0x202020;
        mat(M_REPERE, ((((c >> 16) & 255) / 2) << 16) | ((((c >> 8) & 255) / 2) << 8) | ((c & 255) / 2), 0.7f, 0);
    }
    mat(M_MOYEU, 0x1c1c1e, 0.85f, 0);                       // coussin du sac gonflable : plastique moussé (pièce sans uv : pas de grain)
    // enjoliveurs des aérateurs : chrome satiné (mat.chrome, albédo 0,65-0,70) ; alu brossé sur la 911
    // WFS-325 (RG-07) : lot 1 : chrome dans la plage de la charte (rugosité 0,05-0,15 : 0,12) ; les autres voitures gardent 0,18
    const bool propres = lot1;
    if (d.accentMetal > 0.5f) mat(M_CHROME, d.accent, 0.35f, 1.f); else mat(M_CHROME, 0xd2d6da, propres ? RUGO_CHROME_LOT1 : 0.18f, 1.f);
    // WFS-325 (RG-07) : aluminium brossé (grilles de haut-parleur, grille de la SF90) : métal, F0 linéaire ≈ 0,91 (aluminium pur,
    // base « Physically Based » de Doug Wright, de mémoire : page non ouverte), rugosité 0,3-0,5 pour un brossé (guide PBR de
    // Substance, de mémoire) : 0,38 ; teinte un peu sous l'aluminium pur (alliage anodisé incolore)
    mat(M_ALU, 0xe2e5e8, RUGO_ALU, 1.f);
    {   // WFS-325 : guide de lumière d'ambiance : jour, plastique diffusant sombre et satiné (diélectrique, rugosité 0,45) ; nuit, la DEL
        // l'allume (émission ≈ celle des chiffres du combiné de nuit, jamais éblouissante) ; couleur du modèle (couleurAmbiance)
        mat(M_AMBIANCE, 0x2a2b2e, 0.45f, 0);
        Material m; m.color = {0, 0, 0, 1}; m.rough = 0.45f; m.metal = 0; m.emissive = lin(couleurAmbiance(d) ? couleurAmbiance(d) : 0x2a2b2e) * (couleurAmbiance(d) ? 0.35f : 0.f);
        mats_[M_AMBIANCE_NUIT] = r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr);
    }
    ecran(M_FOND_COMBINE, d.combineFond, 1.0f);
    ecran(M_FOND_ECRAN, 0x1b2430, 1.0f);
    ecran(M_FOND_ECRAN_NUIT, 0x0b0f15, 1.0f);
    mat(M_LAMES, 0x0a0a0a, 0.8f, 0);
    // ciel de toit (tissu, Alcantara) ; WFS-323 : anthracite 0x3a3b3e (albédo linéaire 0,042, tissu sombre de la charte) : vu par
    // en dessous, sous le toit, le 0x26272a d'avant sortait noir (aplat au-dessus du pare-brise)
    mat(M_PAVILLON, 0x3a3b3e, 0.95f, 0);
    // WFS-282 (RG-03) : glace du rétroviseur : surface claire et lisse, éclairée par le ciel (claire le jour) ; en métal, elle ne
    // reflétait presque rien dans l'habitacle (bloc presque noir, refus du lead dev sur la PR #251)
    mat(M_GLACE, 0xa4aab0, 0.12f, 0);
    // WFS-324 (RG-02) : glaces à vrai reflet (image du reflet × réflexion de la glace) ; sans image (création impossible) : glace claire
    for (int k = 0; k < retro::NB_GLACES; k++) {
        if (retro::image(r, k)) mats_[(size_t)(M_RETRO0 + k)] = retro::materiau(r, k, miroirs_[(size_t)k].reflexion);
        else mat(M_RETRO0 + k, 0xa4aab0, 0.12f, 0);
    }
    // WFS-324 : coques des rétroviseurs (intérieur et extérieurs) : plastique noir satiné (jamais noir pur : albédo linéaire 0,01 ; en
    // vernis lisse, la lèvre renvoyait le ciel en un trait blanc crénelé au-dessus de la glace intérieure, et la rue éclairée la nuit
    // comme un anneau de chrome autour des extérieures, vu au banc) ; bras et aile de portière : plastique noir grainé mat
    mat(M_COQUE_EXT, 0x1c1c1e, 0.42f, 0);
    mat(M_BRAS_EXT, 0x1e1e20, 0.75f, 0, true);
    // WFS-282 (RG-04) : la nuit, l'habitacle reçoit un peu de lumière (compteurs, écran, rue, ≈ 0,5 à 1 lux) : chaque matière
    // garde sa couleur, plus une petite émission proportionnelle à son albédo (métaux : un tiers) ; sans elle, planche,
    // volant et montants sortaient en aplats noirs
    // ; ni les écrans (déjà lumineux), ni la glace du rétroviseur (la nuit, elle reste sombre comme le ciel qu'elle reflète)
    matsNuit_ = mats_;
    for (int k = 0; k < M_N; k++) {
        Material& m = matsNuit_[(size_t)k];
        const bool lumineux = m.emissive.x + m.emissive.y + m.emissive.z > 0.f;
        if (k == M_GLACE) {   // glace de nuit : position nuit d'un rétroviseur (réflexion 0,5 %, sans diffus, glace lisse : les lampes ne laissent qu'un petit reflet) : jamais plus claire que le ciel de nuit
            m.color = {0.005f, 0.005f, 0.005f, m.color.w}; m.metal = 1.f; m.rough = 0.15f;   // métal, rugosité : constantes du dessin
            continue;
        }
        if (lumineux || k >= M_COQUE_EXT) continue;   // WFS-324 : coques et bras des rétroviseurs (dehors, ou glace : pas de lueur d'habitacle)
        m.emissive = vec3{m.color.x, m.color.y, m.color.z} * (AMBIANCE_NUIT * (m.metal > 0.5f ? 0.33f : 1.f));
    }
    std::vector<Vertex> v; std::vector<uint32_t> i;
    auto prend = [&](Geo& g, int m, int attache = 0) { v = std::move(g.v); i = std::move(g.i); ajouter(r, v, i, m, attache); g = Geo{}; };
    const mat4 I = mat4::identity();
    const float Y = d.plancheY, Z = d.plancheZ, Yb = d.pareBriseY, Zb = d.pareBriseZ, x0 = cx_ - demi_, x1 = cx_ + demi_;   // WFS-315 : planche sur toute la largeur aux coudes
    // planche (profil y, z du pied du pare-brise vers le conducteur) : dessus, lèvre, face en retrait derrière le combiné et
    // l'écran central (z ≈ −0,86), puis la partie basse qui revient vers les genoux
    const bool combine = d.combine != Combine::Aucun && d.combineW > 0;   // WFS-201 : Model 3 : aucun combiné derrière le volant
    const float zFace = zFacePlanche(d);   // WFS-325 : même calcul, sorti dans vehicule_cockpit.h
    {
        Geo haut, face, creux;
        // devant le conducteur, la planche descend sous le combiné (creux) : l'écran, posé au-dessus, se voit dans l'ouverture
        // haute du volant, sous sa visière ; ailleurs, le dessus de planche va du pare-brise à la lèvre
        const float xa = d.combinePos.x - d.combineW * 0.5f - 0.03f, xb = d.combinePos.x + d.combineW * 0.5f + 0.03f;
        const float Yc = std::min(Y, d.combinePos.y - d.combineH * 0.5f - 0.012f);
        const std::vector<vec2> pHaut{{Yb, Zb}, {Y + 0.005f, (Zb + Z) * 0.5f}, {Y, Z - 0.02f}, {Y - 0.035f, Z}};
        const std::vector<vec2> pCreux{{Yb, Zb}, {Yc, d.combinePos.z - 0.03f}, {Yc, Z - 0.02f}, {Yc - 0.03f, Z}};
        if (combine) { extruder(haut, I, pHaut, x0, xa); extruder(haut, I, pHaut, xb, x1); extruder(creux, I, pCreux, xa, xb); }
        else extruder(haut, I, pHaut, x0, x1);
        // WFS-323 (ANO-188) : devant le combiné, la face part du bas du creux (Yc − 0,03), plus de la lèvre (Y − 0,035) : le haut de la
        // face, 8 cm devant l'écran, passait à mi-hauteur du combiné vu de l'œil et cachait le bas des chiffres (« 58 » lu « SA »)
        const std::vector<vec2> pFace{{Y - 0.035f, Z}, {Y - 0.09f, zFace}, {Y - 0.42f, zFace}, {Y - 0.58f, zFace + 0.20f}};
        const std::vector<vec2> pFaceCreux{{Yc - 0.03f, Z}, {std::min(Y - 0.09f, Yc - 0.05f), zFace}, {Y - 0.42f, zFace}, {Y - 0.58f, zFace + 0.20f}};
        if (combine) { extruder(face, I, pFace, x0, xa); extruder(face, I, pFace, xb, x1); extruder(face, I, pFaceCreux, xa, xb); }
        else extruder(face, I, pFace, x0, x1);
        prend(haut, M_PLANCHE_HAUT); prend(creux, M_PLANCHE_HAUT); prend(face, M_PLANCHE_BAS);
        // casquette (visière) du combiné : avance de la lèvre vers le conducteur au-dessus de l'écran
        Geo cas;
        // WFS-323 (revue du lead dev) : visière moulée (épaisseur 2,4 cm, arêtes arrondies de 1,1 cm, lèvre fermée par une calotte), le
        // long de la même ligne qu'avant (du pied du pare-brise jusqu'à 7,5 cm devant l'écran) ; avant : profil extrudé à arêtes vives (bloc)
        // WFS-414 (RG-07) : ligne calculée sans rendu (ligneVisiere, vehicule_cockpit_volant.cpp), vérifiée par tests/test_cockpit.cpp
        const Visiere vis = ligneVisiere(d);
        lisse(cas, I, {vis.pts.begin(), vis.pts.end()}, vis.largeur, vis.ev, 0.011f);
        if (combine) prend(cas, M_PLANCHE_HAUT); else cas = Geo{};
        // jonc décoratif sur toute la largeur, sous la lèvre (alu brossé, carbone, ou liseré rouge de la Golf)
        Geo jonc;
        // WFS-323 : profilé à bords arrondis (rayon 2,8 mm ; 6 mm en carbone) au lieu d'une boîte
        profile(jonc, I, {x0 + 0.02f, Y - 0.10f, zFace + 0.004f}, {x1 - 0.02f, Y - 0.10f, zFace + 0.004f}, d.carbone ? 0.05f : 0.010f, 0.006f,
                d.carbone ? 0.006f : 0.0028f);
        prend(jonc, M_ACCENT);
    }
    // aérateurs : deux aux extrémités, deux au centre (sous l'écran central quand il est sur la planche) ; WFS-325 : lot 1 :
    // aérateurs du modèle (construireDetails)
    if (!propres) {
        Geo cadre, lames;
        const bool rond = d.combine == Combine::Porsche5 || d.combine == Combine::FerrariIncurve;
        const float ay = Y - 0.17f, az = zFace + 0.012f;
        for (float x : {x0 + 0.11f, x1 - 0.11f}) aerateur(cadre, lames, translate({x, ay, az}) * rotateX(-0.2f), 0.13f, rond ? 0.09f : 0.055f, rond);
        for (float x : {cx_ - 0.09f, cx_ + 0.09f}) aerateur(cadre, lames, translate({x, ay - 0.04f, az + 0.01f}) * rotateX(-0.2f), 0.11f, 0.045f, false);
        prend(cadre, M_CHROME); prend(lames, M_LAMES);
    }
    // WFS-315 (RG-02) : place passager : boîte à gants (porte en léger relief, joint sombre autour) sous la lèvre, couvercle du sac
    // gonflable passager sur le dessus de planche (joint fin) : on voit que la planche continue devant un 2e siège
    {
        Geo bag, joint, sac;
        const float gx = cx_ + (demi_ * 0.55f), gw = std::min(0.40f, demi_ * 0.75f);   // milieu de la place passager
        // WFS-323 : porte moulée (coins de 3 cm, chanfrein de 6 mm, dépouille) et joint arrondi, au lieu de boîtes
        dalleFace(bag, translate({gx, Y - 0.26f, zFace + 0.008f}), gw, 0.15f, 0.016f, 0.03f, 0.006f, 0.96f);
        dalleFace(joint, translate({gx, Y - 0.26f, zFace + 0.002f}), gw + 0.012f, 0.162f, 0.008f, 0.034f, 0.002f);
        // couvercle : sur la partie plate du dessus (du point le plus haut, à mi-profondeur, jusqu'à la lèvre), côté conducteur du
        // point le plus haut : vu de l'œil, il reste sous la ligne de la planche (posé à hauteur fixe sur la pente vers le
        // pare-brise, il flottait de ≈ 3 cm devant la rue : refus du lead dev, PR #262)
        const float zm = (Zb + Z) * 0.5f, za = Z - 0.02f, zc = (zm + za) * 0.5f, ym = Y + 0.005f + (Y - (Y + 0.005f)) * 0.5f;
        dalle(sac, translate({gx, ym + 0.0005f, zc}), 0.30f, std::max(0.04f, (za - zm) * 0.8f), 0.002f, 0.03f, 0.0008f);   // WFS-323 : coins arrondis
        prend(bag, M_PLANCHE_BAS); prend(joint, M_PLANCHE_HAUT); prend(sac, M_PLANCHE_HAUT);   // joints mats (le noir laqué brillait la nuit)
    }
    // console centrale et sélecteur ; WFS-325 : lot 1 : console du modèle (construireDetails)
    if (!propres) {
        Geo con, sel;
        // WFS-323 : console moulée (coins de 5 cm, chanfrein de 2,5 cm, dépouille) ; sélecteur : soufflet, tige qui s'affine et
        // pommeau galbé (au lieu d'une boîte, d'un cylindre et d'une boîte)
        dalle(con, translate({cx_, Y - 0.50f, zFace + 0.32f}) * rotateX(-0.40f), 0.26f, 0.60f, 0.12f, 0.05f, 0.025f, 0.94f);
        dalle(sel, translate({cx_, Y - 0.455f, zFace + 0.40f}) * rotateX(-0.2f), 0.07f, 0.09f, 0.012f, 0.025f, 0.005f, 0.9f);
        profile(sel, I, {cx_, Y - 0.452f, zFace + 0.40f}, {cx_, Y - 0.392f, zFace + 0.404f}, 0.026f, 0.026f, 0.013f, 0.02f, 0.02f);
        dalle(sel, translate({cx_, Y - 0.375f, zFace + 0.40f}), 0.05f, 0.075f, 0.03f, 0.022f, 0.011f, 0.85f);
        prend(con, M_PLANCHE_BAS); prend(sel, M_ACCENT);
    }
    // montants de pare-brise, pavillon, pare-soleil, rétroviseur intérieur, portières
    // WFS-323 (RG-ANO-207) : l'habitacle a un toit : traverse avant au haut du pare-brise (HAUT_PARE_BRISE_*), ciel de toit tendu
    // de la traverse jusque derrière la tête, rails de toit au-dessus des portières, deux pare-soleil repliés ; montants arrondis
    // (garniture moulée) qui montent jusqu'à la traverse. Avant : montants jusqu'à 0,40 m au-dessus de l'œil et pavillon à
    // 0,43 m, hors de l'image : on voyait le ciel au-dessus du pare-brise (cabriolet, ANO-207)
    {
        Geo mont, pav, ret, porte, glace, chrome, axes, glaceG, glaceD, coqueExt, brasExt;
        const float xh = montant_ - RETRAIT_MONTANT_HAUT, yh = HAUT_PARE_BRISE_Y, zh = HAUT_PARE_BRISE_Z;
        const vec3 bas[2] = {{cx_ - montant_, Yb, Zb}, {cx_ + montant_, Yb, Zb}};   // WFS-315 : pieds des montants à leur écart réel
        const vec3 haut[2] = {{cx_ - xh, yh, zh}, {cx_ + xh, yh, zh}};
        // WFS-354 : section dans les cotes de la vitre de la pluie (LARGEUR × EPAISSEUR), arêtes arrondies (rayon 2 cm, garniture)
        for (int k = 0; k < 2; k++) profile(mont, I, bas[k], haut[k], LARGEUR_MONTANT, EPAISSEUR_MONTANT, 0.025f);
        // ciel de toit : de la traverse (yh) il monte vers CIEL_DE_TOIT_Y à 0,25 m devant l'œil, puis reste plat jusqu'à 0,5 m
        // derrière ; bombé en travers (les bords descendent de 3 cm vers les rails, sauf contre la traverse) ; normale vers le bas
        auto hautToit = [&](float z) { return hauteurCielDeToit(z); };   // WFS-325 : même calcul, sorti dans vehicule_cockpit.h
        auto bord = [&](float z) { return xh + (demi_ - 0.02f - xh) * std::clamp((z - zh) / (0.40f - zh), 0.f, 1.f); };   // demi-largeur du ciel de toit
        auto bombe = [&](float z) { return 0.03f * std::clamp((z - zh) / 0.30f, 0.f, 1.f); };
        auto pointToit = [&](float u, float z) { return vec3{cx_ + u * bord(z), hautToit(z) - bombe(z) * u * u, z}; };
        {
            constexpr int NU = 16, NZ = 12;
            const uint32_t b0 = (uint32_t)pav.v.size();
            for (int j = 0; j <= NZ; j++) for (int i = 0; i <= NU; i++) {
                const float u = -1.f + 2.f * i / NU, z = zh + (0.50f - zh) * j / NZ;
                const vec3 p = pointToit(u, z), pu = pointToit(u + 0.01f, z) - pointToit(u - 0.01f, z), pz = pointToit(u, z + 0.01f) - pointToit(u, z - 0.01f);
                vec3 n = normalize(cross(pu, pz)); if (n.y > 0) n = n * -1.f;
                ajouterUV(pav, I, p, n, p.x * GRAIN, p.z * GRAIN);
            }
            for (int j = 0; j < NZ; j++) for (int i = 0; i < NU; i++) {
                const uint32_t a = b0 + j * (NU + 1) + i;
                quadOriente(pav, a, a + 1, a + NU + 2, a + NU + 1);
            }
        }
        // traverse avant (bord du ciel de toit qui revient sur le haut du pare-brise) et rails de toit au-dessus des portières :
        // profilés arrondis ; les rails suivent le bord du ciel de toit en 4 tronçons
        profile(pav, I, {cx_ - xh - 0.01f, yh - 0.004f, zh + 0.025f}, {cx_ + xh + 0.01f, yh - 0.004f, zh + 0.025f}, 0.028f, 0.05f, 0.013f);
        for (float s : {-1.f, 1.f}) for (int k = 0; k < 4; k++) {
            const float za = zh + (0.50f - zh) * k / 4, zb = zh + (0.50f - zh) * (k + 1) / 4;
            const vec3 a = pointToit(s, za) + vec3{0, -0.02f, 0}, b = pointToit(s, zb) + vec3{0, -0.02f, 0};
            profile(pav, I, a, b, 0.05f, 0.07f, 0.022f);
        }
        // pare-soleil repliés contre le ciel de toit (rembourrés : 38 × 17 cm, 2,8 cm d'épaisseur, coins de 3 cm), axe d'articulation
        // côté montant ; celui du conducteur devant lui, celui du passager en miroir de l'axe de la voiture
        {
            const float ws = std::min(0.38f, xh * 0.62f), zs = zh + 0.11f, ys = hautToit(zs) - 0.017f;
            const float pente = std::atan2(hautToit(zs + 0.05f) - hautToit(zs - 0.05f), 0.10f);
            for (float s : {-1.f, 1.f}) {
                const float xs = cx_ + s * (xh * 0.55f);
                dalle(pav, translate({xs, ys, zs}) * rotateX(pente), ws, 0.17f, 0.028f, 0.03f, 0.009f, 0.97f);
                // axe du pare-soleil : plastique mat (en chrome, son reflet faisait un point blanc en haut de l'image : avis agent 3D)
                profile(axes, I, {xs - s * (ws * 0.5f + 0.005f), ys + 0.004f, zs - 0.07f}, {xs - s * (ws * 0.5f + 0.005f), ys + 0.012f, zs + 0.03f}, 0.011f, 0.011f, 0.0055f);
            }
        }
        // rétroviseur intérieur (≈ 250 × 70 mm, coque moulée à dépouille, glace à coins arrondis) accroché au pare-brise sous la
        // traverse, sur l'axe de la voiture, tourné vers le conducteur ; pied collé sur la vitre et tige qui s'affine. Avant : boîte grise
        // posée en l'air à 0,20 m au-dessus de l'œil, tige noire qui montait dans le ciel (ANO-207). WFS-324 : place et réglage de la
        // glace calculés par retro::miroirs (la même glace sert à la projection du reflet), glace à vrai reflet
        {
            const vec3 eu = normalize(vec3{0, yh - Yb, zh - Zb}), en = normalize(cross(vec3{1, 0, 0}, eu));   // plan du pare-brise (normale vers l'habitacle)
            const vec3 pied = vec3{cx_, Yb, Zb} + eu * (length(vec3{0, yh - Yb, zh - Zb}) - 0.05f) + en * 0.004f;
            const retro::Miroir& mi = miroirs_[retro::INTERIEUR];
            const mat4 R = mi.repere * translate({0, 0, -0.0205f});   // centre de la coque, 2 cm derrière la glace
            const vec3 c = R.transformPoint({0, 0, 0});
            dalleFace(ret, R, 0.25f, 0.07f, 0.04f, 0.028f, 0.010f, 0.82f);
            glaceRetro(glace, mi.repere, mi.w, mi.h, 0.022f, 0.022f, 0);   // cadre de 7 mm
            {   // pied collé sur la vitre : dalle dans le plan du pare-brise (y du repère = normale de la vitre)
                const vec3 ex{1, 0, 0}, ez = cross(ex, en);
                mat4 B = mat4::identity();
                B.at(0, 0) = ex.x; B.at(1, 0) = ex.y; B.at(2, 0) = ex.z; B.at(0, 1) = en.x; B.at(1, 1) = en.y; B.at(2, 1) = en.z;
                B.at(0, 2) = ez.x; B.at(1, 2) = ez.y; B.at(2, 2) = ez.z;
                dalle(ret, translate(pied) * B, 0.05f, 0.035f, 0.008f, 0.012f, 0.003f);
            }
            profile(ret, I, pied + en * 0.004f, c + vec3{0, 0.02f, -0.012f}, 0.024f, 0.020f, 0.009f, 0.016f, 0.017f);
        }
        // WFS-324 (RG-01) : rétroviseurs extérieurs, gauche et droit : coque galbée (glace en retrait dans sa lèvre, nez arrondi qui
        // se resserre vers la portière), bras profilé et aile collée à l'angle avant de la vitre latérale
        for (int k : {retro::GAUCHE, retro::DROITE}) {
            const retro::Miroir& mi = miroirs_[(size_t)k];
            const float sx = k == retro::GAUCHE ? -1.f : 1.f;
            coqueRetro(coqueExt, mi.repere, mi.w, mi.h, 0.018f, mi.rayonExterieur, sx);
            glaceRetro(k == retro::GAUCHE ? glaceG : glaceD, mi.repere, mi.w, mi.h, 0.018f, mi.rayonExterieur, sx);
            // bras : de l'avant de la coque, côté portière (caché derrière elle, vu de l'œil), vers l'aile fixée sur la portière à l'angle
            // avant de la vitre, 14 cm devant la glace (repère de l'œil : −z devant)
            const vec3 a = mi.repere.transformPoint({-sx * (mi.w * 0.5f - 0.03f), -mi.h * 0.25f, -0.075f});
            const float xPorte = cx_ + sx * (faceVitre(demi_) + 0.03f), yc = ligneCeinture(d), zAile = mi.repere.at(2, 3) - 0.14f;
            const vec3 b{xPorte, yc + 0.035f, zAile};
            profile(brasExt, I, a, b, 0.045f, 0.026f, 0.011f, 0.032f, 0.05f);
            // aile : garniture moulée à l'angle avant de la vitre (repère : x vertical, y vers l'extérieur, z le long de la voiture)
            dalle(brasExt, translate({xPorte + sx * 0.004f, yc + 0.04f, zAile + 0.01f}) * rotateZ(-sx * PI / 2), 0.075f, 0.12f, 0.016f, 0.025f, 0.005f, 0.9f);
        }
        // contre-portes : panneau intérieur sous la vitre, accoudoir (place du conducteur et du passager)
        // WFS-349 (RG-02) : habitacle fermé (boitesFermeture, vehicule_cockpit.h) : chaque portière est un bloc plein de 12 cm, de
        // la face intérieure (WFS-315, inchangée) à la vitre ; son dessus, l'appui de vitre, est à la ligne de ceinture ; de 6 cm
        // devant le pied du pare-brise (sous le pied du montant) à 0,6 m derrière l'œil, du plancher à la ceinture ; plancher et
        // tablier ferment la zone des pieds. Avant : plaque de 6 cm, de 15 cm derrière le pied du pare-brise à 0,8 m sous l'œil :
        // vue de l'œil, la vitre descendait jusqu'au coin bas de l'image et la voisine s'y voyait (ANO-181, 183)
        // WFS-323 : mêmes volumes, arêtes arrondies (1,5 cm, chanfrein de 8 mm), plus la garniture d'une vraie contre-porte :
        // appui de vitre arrondi, panneau gainé, accoudoir rembourré (même place qu'avant), poignée chromée
        for (const BoiteHab& b : boitesFermeture(d, cx_, demi_)) dalle(porte, translate(b.centre), b.taille.x, b.taille.z, b.taille.y, 0.015f, 0.008f);
        const float yc = ligneCeinture(d), zAv = d.pareBriseZ - 0.06f, zAr = 0.60f;
        for (float sx : {-1.f, 1.f}) {
            const float xi = cx_ + sx * (demi_ - 0.01f);   // face intérieure de la portière
            profile(porte, I, {xi + sx * EPAISSEUR_PORTE * 0.5f, yc + 0.004f, zAv + 0.02f}, {xi + sx * EPAISSEUR_PORTE * 0.5f, yc + 0.004f, zAr}, 0.03f, EPAISSEUR_PORTE + 0.02f, 0.013f);
            if (propres) continue;   // WFS-325 : lot 1 : garnitures du modèle (construireDetails) ; appui de vitre et bloc inchangés
            // panneau gainé : repère de la face (rotateZ : y vers l'habitacle, x vertical)
            dalle(pav, translate({xi - sx * 0.002f, yc - 0.20f, (zAv + zAr) * 0.5f + 0.05f}) * rotateZ(sx * PI / 2), 0.30f, zAr - zAv - 0.30f, 0.008f, 0.06f, 0.004f);
            dalle(porte, translate({cx_ + sx * (demi_ - 0.02f), Y - 0.24f, -0.20f}), 0.06f, 0.45f, 0.05f, 0.025f, 0.015f, 0.9f);   // accoudoir
            dalle(chrome, translate({xi - sx * 0.004f, Y - 0.12f, -0.50f}) * rotateZ(sx * PI / 2), 0.03f, 0.11f, 0.012f, 0.012f, 0.004f);   // poignée
        }
        prend(mont, M_PAVILLON); prend(pav, M_PAVILLON); prend(ret, M_COQUE_EXT); prend(glace, M_RETRO0); prend(porte, M_PORTE); prend(chrome, M_CHROME); prend(axes, M_PLANCHE_BAS);
        prend(glaceG, M_RETRO1); prend(glaceD, M_RETRO2); prend(coqueExt, M_COQUE_EXT); prend(brasExt, M_BRAS_EXT);   // WFS-324
    }
    // WFS-325 (RG-06) : lot 1 : aérateurs, commandes de la planche, console, garnitures des portières et plafonnier du modèle,
    // construits sans rendu (vehicule_cockpit_details.cpp) ; un maillage par matière
    if (propres) {
        PiecesDetails pd;
        construireDetails(d, pd);
        constexpr int VERS[(int)MatDetail::N] = {M_PLANCHE_HAUT, M_PLANCHE_BAS, M_ACCENT, M_LAQUE, M_CHROME, M_LAMES, M_CUIR, M_PAVILLON,
                                                 M_REPERE, M_ALU, M_GLACE, M_AMBIANCE};
        for (int k = 0; k < (int)MatDetail::N; k++) prend(pd.g[(size_t)k], VERS[k]);
    }
    // écrans : fond du combiné, cadre et fond de l'écran central
    {
        Geo fc, cc, cadre, fe;
        // WFS-323 : dalles d'écran à coins arrondis (4 mm) et cadres moulés (coins de 8 à 10 mm, chanfrein), au lieu de plans et de boîtes
        dalleFace(fc, versOeil(d.combinePos, 0), d.combineW, d.combineH, 0.001f, 0.004f, 0.0003f);
        dalleFace(cc, versOeil(d.combinePos, 0) * translate({0, 0, -0.008f}), d.combineW + 0.012f, d.combineH + 0.012f, 0.012f, 0.008f, 0.003f);
        if (combine) { prend(fc, M_FOND_COMBINE); prend(cc, M_LAQUE); } else { fc = Geo{}; cc = Geo{}; }
        if (d.ecranW > 0) {
            const mat4 E = versOeil(d.ecranPos, d.ecranLacet);
            dalleFace(cadre, E * translate({0, 0, -0.010f}), d.ecranW + 0.016f, d.ecranH + 0.016f, 0.018f, 0.010f, 0.004f, 0.9f);
            dalleFace(fe, E * translate({0, 0, 0.0005f}), d.ecranW, d.ecranH, 0.001f, 0.004f, 0.0003f);
            prend(cadre, M_LAQUE);
            prend(fe, M_FOND_ECRAN);   // deux matériaux (jour, nuit) : même maillage dessiné avec l'un ou l'autre
        }
    }
    // volant (repère du volant : plan xy, +z vers le conducteur) : géométrie sans rendu (WFS-414 : vehicule_cockpit_volant.cpp)
    {
        PiecesVolant p;
        construireVolant(d, p);
        prend(p.jante, M_CUIR, true); prend(p.moyeu, M_MOYEU, true); prend(p.branches, M_BRANCHES, true);
        for (int k = 0; k < 3; k++) prend(p.coutures[k], M_COUTURE0 + k, true);
        prend(p.repere, M_REPERE, true); prend(p.palettes, d.carbone ? M_ACCENT : M_BRANCHES, d.palettesColonne ? 2 : 1); prend(p.manettino, M_REPERE, true);
        prend(p.molettes, M_LAMES, true);
        // WFS-414 : volant de série seulement (vides sinon : aucune pièce ajoutée)
        // coque : plastique noir grainé (comme le bas de planche), même sur les volants à branches alu (S3) ; joint : noir mat
        prend(p.coque, M_PLANCHE_BAS, true); prend(p.joint, M_LAMES, true); prend(p.plaques, M_LAQUE, true); prend(p.touches, M_MOYEU, true);
    }
    // essuie-glaces : bras + balai, dans le repère du balai (x le long du bras, y vers le haut du pare-brise)
    {
        Material m; m.color = {0.012f, 0.012f, 0.012f, 1}; m.rough = 0.55f; m.metal = 0; brasMat_ = r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr);
        brasNuit_ = brasMat_; brasNuit_.emissive = vec3{m.color.x, m.color.y, m.color.z} * AMBIANCE_NUIT;   // WFS-282 (RG-04)
        for (int k = 0; k < 2; k++) {
            const float L = k == 0 ? d.balaiG : d.balaiD;
            if (L <= 0) continue;
            Geo g;
            profile(g, mat4::identity(), {0, 0, 0.012f}, {L * 0.9f, 0, 0.012f}, 0.014f, 0.010f, 0.004f, -1, 0.010f);   // bras (WFS-323 : profilé qui s'affine)
            profile(g, mat4::identity(), {0, 0, 0.004f}, {L, 0, 0.004f}, 0.010f, 0.012f, 0.004f);                     // balai (caoutchouc)
            bras_[k] = r.makeMesh(g.v, g.i); octets_ += g.v.size() * sizeof(Vertex) + g.i.size() * 4;
        }
    }
    // écrans dynamiques (capacité fixe : aucune allocation par image)
    for (int k = 0; k < D_N; k++) {
        dv_[k].assign(DYN_SOMMETS, Vertex{}); di_[k].assign(DYN_SOMMETS / 4 * 6, 0);
        dyn_[k] = r.makeMesh(dv_[k], di_[k], true);
        octets_ += DYN_SOMMETS * sizeof(Vertex) + DYN_SOMMETS / 4 * 6 * 4;
        dv_[k].clear(); di_[k].clear();
    }
    auto emis = [&](unsigned c, float f) { Material m; m.color = {0, 0, 0, 1}; m.rough = 0.2f; m.emissive = lin(c) * f; return r.makeMaterial(deuxFaces(m), nullptr, nullptr, nullptr); };
    const unsigned cols[D_N] = {d.combineActif, 0x2c3440, d.combineAlerte, 0xc8d0da, 0xff6a00};
    for (int k = 0; k < D_N; k++) { dynMat_[k] = emis(cols[k], 1.0f); dynNuit_[k] = emis(cols[k], 0.45f); }   // nuit : moins lumineux, jamais éblouissant
    { Material c; c.color = {0, 0, 0, 1}; c.rough = 1; c.emissive = {16, 0, 16}; cleMat_ = r.makeMaterial(deuxFaces(c), nullptr, nullptr, nullptr); }   // banc (WFS-235, RG-05)
    // WFS-349 (RG-03) : clé douce (émission 1,5 au lieu de 16) : la clé de WFS-235 sature les pixels de bord, et la lueur
    // déborde de ≈ 10 pixels hors de l'habitacle (vu au banc) ; celle-ci garde des bords nets. Banc seulement
    // construits seulement quand le banc « cockpit-voisine » le demande (g_bancCockpitVoisine, avant la construction) : rien en course
    if (g_bancCockpitVoisine) { Material c; c.color = {0, 0, 0, 1}; c.rough = 1; c.emissive = {1.5f, 0, 1.5f}; cleDouce_ = r.makeMaterial(deuxFaces(c), nullptr, nullptr, nullptr); }
    if (g_bancCockpitVoisine) {   // WFS-349 (RG-03) : gabarit du banc : les parois que l'habitacle doit fermer, vues de l'œil : plan de chaque portière
        // (plan de la vitre latérale) sous la ligne de ceinture, du pied du pare-brise à 1,5 m derrière l'œil, et plan du pied du
        // pare-brise sous la planche ; un rayon de l'œil qui traverse ces plans doit rencontrer l'habitacle avant
        Geo g;
        const float yc = ligneCeinture(d), bas = -3.f, za = d.pareBriseZ, zb = 1.5f;
        for (float sx : {-1.f, 1.f})
            boite(g, translate({cx_ + sx * faceVitre(demi_), (yc + bas) * 0.5f, (za + zb) * 0.5f}), 0.002f, yc - bas, zb - za);   // plan de la vitre latérale
        boite(g, translate({cx_, (d.pareBriseY + bas) * 0.5f, za - 0.03f}), 2 * faceVitre(demi_), d.pareBriseY - bas, 0.002f);   // devant le tablier
        gabarit_ = r.makeMesh(g.v, g.i);
    }
    WFS_LOG("cockpit (WFS-197) : intérieur %s construit : %zu pièces, %.2f Mo", d.car, pieces_.size(), octets_ / 1048576.0);
    return true;
}

void Interieur::detruire(Renderer& r) {
    if (!def_ && pieces_.empty()) return;
    r.vk().waitIdle();
    for (Piece& p : pieces_) r.destroy(p.m);
    pieces_.clear();
    for (Material& m : mats_) r.destroy(m);
    mats_.clear(); matsNuit_.clear();   // copies de nuit : même jeu de descripteurs (rien à détruire)
    for (Mesh& m : bras_) if (m.indexCount) r.destroy(m);
    if (brasMat_.set) r.destroy(brasMat_);
    for (int k = 0; k < D_N; k++) { r.destroy(dyn_[k]); r.destroy(dynMat_[k]); r.destroy(dynNuit_[k]); dyn_[k] = {}; dynMat_[k] = dynNuit_[k] = {}; }
    for (Mesh& m : bras_) m = {};
    brasMat_ = {}; brasNuit_ = {};
    if (cleMat_.set) r.destroy(cleMat_);
    cleMat_ = {};
    if (gabarit_.indexCount) r.destroy(gabarit_);
    gabarit_ = {};
    if (cleDouce_.set) r.destroy(cleDouce_);
    cleDouce_ = {};
    if (grain_.view) r.vk().destroy(grain_);
    grain_ = {};
    def_ = nullptr; octets_ = 0;
}

// ---------------------------------------------------------------- dessin
void Interieur::dessinerCombine(Renderer& r, const mat4& cam, const Valeurs& val) {
    const InterieurDef& d = *def_;
    for (int k : {D_ACTIF, D_FOND, D_ALERTE}) { dv_[k].clear(); di_[k].clear(); }
    Plan2D A{&dv_[D_ACTIF], &di_[D_ACTIF], 0.001f}, F{&dv_[D_FOND], &di_[D_FOND], 0.0008f}, R{&dv_[D_ALERTE], &di_[D_ALERTE], 0.001f};
    const bool surEcran = d.combine == Combine::Aucun;   // WFS-201 : Model 3 : bande de gauche (34 %) de l'écran central
    const float W = surEcran ? d.ecranW * 0.34f : d.combineW, H = surEcran ? d.ecranH * 0.90f : d.combineH, kmh = (float)val.kmh;
    const float vit = std::clamp(kmh / 320.f, 0.f, 1.f), reg = std::clamp(val.regime, 0.f, 1.f), nit = std::clamp(val.nitro, 0.f, 1.f);
    // WFS-323 (ANO-188) : vitesse en chiffres d'au moins 22 % de la hauteur, ramenée dans la partie du combiné vue par l'ouverture
    // haute du volant (bandeLisibleCombine) ; i-Cockpit (vu au-dessus du volant) et Model 3 (écran central) : position du style
    // renvoie le haut des chiffres (le rapport peut se poser juste au-dessus)
    float basV = 0, hV = 0;   // WFS-414 : bas et hauteur des chiffres de la vitesse (rapport placé par rapport à eux)
    auto vitesse = [&](float xd, float y0, float h) {
        h = std::max(h, H * 0.22f);
        if (!surEcran && d.combine != Combine::PeugeotICockpit) placerDansBande(bandeLisibleCombine(d), H, y0, h);
        nombre(A, xd, y0, h, (int)kmh, 3);
        basV = y0; hV = h;
        return y0 + h;
    };
    // WFS-414 (avis de l'agent 3D sur la PR #343) : volant de série : rapport centré au-dessus des deux chiffres de la vitesse (xd : bord
    // droit de la vitesse, comme nombre()), un jour de 5 % de la hauteur du combiné entre les deux, sans dépasser la partie vue par le volant
    // marges sous le haut de la partie vue (bandeLisibleCombine prend le bord du tube de la jante dans son plan du milieu ; vu de
    // l'œil, le tube rond cache un peu plus bas) : 3 % sur l'Abarth (jour net mesuré, 2e avis), 5 % sur la S3 et la Giulia (3e avis :
    // le rapport touchait la jante, et la tête qui avance au freinage fait descendre la jante sur le combiné)
    constexpr float MARGE_JANTE = 0.03f, MARGE_JANTE_RAPPORT = 0.05f;
    auto rapportAuDessus = [&](float xd, float hMax) {
        const float y = basV + hV + H * 0.05f, h = std::min(hMax, bandeLisibleCombine(d).haut - H * MARGE_JANTE_RAPPORT - y);
        const float centre = xd - hV * 0.73f;   // milieu de deux chiffres : de xd − 1,32 h à xd − 0,14 h
        rapport(A, centre - h * 0.26f, y, h, val.rapport);
    };
    auto barreNitro = [&](float x0, float y0, float x1, float y1) {   // jauge de nitro horizontale
        F.rect(x0, y0, x1, y1); A.rect(x0, y0, x0 + (x1 - x0) * nit, y1);
    };
    switch (d.combine) {
    case Combine::VwDigital: {   // deux cadrans ronds (vitesse à gauche, régime à droite), vitesse et rapport au centre
        const float rc = H * 0.44f;
        arcBarres(A, F, R, {-W * 0.30f, 0}, rc * 0.78f, rc, PI * 1.25f, -PI * 0.25f, 40, vit, 2.f, 0.0028f);
        arcBarres(A, F, R, {W * 0.30f, 0}, rc * 0.78f, rc, PI * 1.25f, -PI * 0.25f, 40, reg, 0.85f, 0.0028f);
        float y0 = -H * 0.08f, h = H * 0.40f;   // WFS-323 (ANO-188) : chiffres de 40 % de la hauteur (30 % avant), dans la bande vue
        if (!surEcran) placerDansBande(bandeLisibleCombine(d), H, y0, h);
        nombre(A, 0.022f, y0, h, (int)kmh, 3);
        rapport(A, W * 0.30f - H * 0.08f, -H * 0.12f, H * 0.26f, val.rapport);
        barreNitro(-W * 0.10f, -H * 0.38f, W * 0.10f, -H * 0.33f);
        break;
    }
    case Combine::BmwLive: {   // vitesse en chiffres à gauche du centre, rapport au centre, régime en barres qui montent vers la droite
        // WFS-323 (ANO-188) : chiffres de 42 % de la hauteur (34 % avant), rapprochés du centre (fin à −3 % au lieu de −16 % de la largeur) :
        // vus par l'ouverture haute du volant ; à gauche, la jante qui descend les cachait (M4, X5)
        float y0 = -H * 0.14f, h = H * 0.42f;
        placerDansBande(bandeLisibleCombine(d), H, y0, h);   // X5 : sous le haut de la jante
        nombre(A, -W * 0.03f, y0, h, (int)kmh, 3);
        const int n = 34;
        for (int k = 0; k < n; k++) {
            const float f = (k + 0.5f) / n, x = W * (0.10f + 0.36f * f), h = H * (0.12f + 0.55f * f * f);
            Plan2D& c = f <= reg ? (f >= 0.82f ? R : A) : F;
            c.rect(x, -H * 0.30f, x + W * 0.36f / n * 0.6f, -H * 0.30f + h);
        }
        rapport(A, W * 0.005f, y0, h * 0.72f, val.rapport);
        barreNitro(W * 0.10f, -H * 0.42f, W * 0.46f, -H * 0.37f);   // sous les barres du régime (la vitesse descend sur la X5)
        break;
    }
    case Combine::Porsche5: {   // 5 cadrans : compte-tours analogique au centre (vitesse numérique dedans), 4 autour
        const float rc = H * 0.47f, rs = H * 0.33f;
        F.trait({0, 0}, 0, rc * 0.96f, rc, 0.001f);
        arcBarres(F, F, F, {0, 0}, rc * 0.86f, rc, PI * 1.25f, -PI * 0.25f, 9, 1.f, 2.f, 0.0022f);   // graduations
        arcBarres(R, R, R, {0, 0}, rc * 0.86f, rc, PI * 1.25f - PI * 1.5f * 0.84f, -PI * 0.25f, 3, 1.f, 2.f, 0.0022f);   // zone rouge
        const float aig = PI * 1.25f - PI * 1.5f * reg;
        A.trait({0, 0}, aig, -rc * 0.12f, rc * 0.92f, 0.0026f);
        vitesse(H * 0.20f, -rc * 0.60f, H * 0.22f);
        rapport(A, -H * 0.04f, rc * 0.18f, H * 0.16f, val.rapport);
        for (int k : {-2, -1, 1, 2}) {   // cadrans latéraux : vitesse (à gauche du centre), nitro (à droite), deux écrans
            const vec2 c{k * W * 0.205f, -H * 0.04f};
            arcBarres(F, F, F, c, rs * 0.88f, rs, PI * 1.25f, -PI * 0.25f, 24, 1.f, 2.f, 0.0018f);
            if (k == -1) A.trait(c, PI * 1.25f - PI * 1.5f * vit, -rs * 0.1f, rs * 0.85f, 0.0020f);
            if (k == 1) arcBarres(A, F, R, c, rs * 0.70f, rs * 0.86f, PI * 1.25f, -PI * 0.25f, 24, nit, 2.f, 0.0018f);
        }
        break;
    }
    case Combine::FerrariIncurve: {   // écran large : compte-tours circulaire au centre (rouge en haut), vitesse et rapport dedans
        const float rc = H * 0.46f;
        arcBarres(A, F, R, {0, 0}, rc * 0.80f, rc, PI * 1.30f, -PI * 0.30f, 52, reg, 0.80f, 0.0026f);
        // WFS-323 (ANO-188) : vitesse de 30 % de la hauteur dans la bande vue, rapport (24 %) posé juste au-dessus (SF90, Abarth : la
        // bande remontait la vitesse sur le rapport)
        // WFS-414 (avis de l'agent 3D sur la PR #343) : volant de série (Abarth) : le moyeu plus haut ne laisse plus la place du rapport
        // au-dessus de la vitesse (il passait derrière le haut de la jante) et la carte occupe le tiers droit du cadran : vitesse de 22 %
        // posée au bas de la partie vue (3 % au-dessus du moyeu), rapport centré au-dessus avec un jour de 3,5 %
        const BandeCombine bc = bandeLisibleCombine(d);
        float y0 = -rc * 0.80f, h = H * 0.30f;
        placerDansBande(bc, H, y0, h);
        if (volantSeriePour(d) && y0 + h + H * 0.27f > bc.haut) {
            basV = bc.bas + H * 0.03f; hV = H * 0.22f;
            nombre(A, H * 0.17f, basV, hV, (int)kmh, 3);
            const float yr = basV + hV + H * 0.035f, hr = std::min(H * 0.18f, bc.haut - H * MARGE_JANTE - yr);
            rapport(A, H * 0.17f - hV * 0.73f - hr * 0.26f, yr, hr, val.rapport);
        } else {
            const float hautV = vitesse(H * 0.17f, -rc * 0.80f, H * 0.30f);
            rapport(A, -H * 0.07f, hautV + H * 0.03f, H * 0.24f, val.rapport);
        }
        barreNitro(-W * 0.44f, -H * 0.30f, -W * 0.24f, -H * 0.24f);   // panneau gauche (véhicule) ; la carte est à droite
        break;
    }
    case Combine::AudiVirtual: {   // Audi virtual cockpit, vue classique : vitesse à gauche, régime à droite (aiguilles), rapport au centre
        const float rc = H * 0.46f;
        cadran(F, A, R, {-W * 0.29f, 0}, rc, vit, 2.f, 26, 0.0022f);
        cadran(F, A, R, {W * 0.29f, 0}, rc, reg, 0.85f, 26, 0.0022f);
        if (volantSeriePour(d)) {   // WFS-414 : volant de série (S3) : le moyeu plus haut remonte la vitesse ; rapport centré au-dessus
            // (0,235 : bloc de 3 chiffres centré entre les graduations des deux cadrans ; plus à droite, il les mordait : 2e et 3e avis)
            vitesse(H * 0.235f, -H * 0.36f, H * 0.22f);
            rapportAuDessus(H * 0.235f, H * 0.26f);
        } else {
            rapport(A, -H * 0.07f, H * 0.02f, H * 0.26f, val.rapport);
            vitesse(H * 0.30f, -H * 0.36f, H * 0.22f);
        }
        barreNitro(-W * 0.10f, -H * 0.44f, W * 0.10f, -H * 0.40f);
        break;
    }
    case Combine::AlfaCadrans: {   // Giulia QV : deux cadrans à aiguille dans leurs tunnels, petit écran au centre (rapport, vitesse)
        const float rc = H * 0.46f;
        cadran(F, A, R, {-W * 0.30f, 0}, rc, vit, 2.f, 24, 0.0024f);
        cadran(F, A, R, {W * 0.30f, 0}, rc, reg, 0.82f, 24, 0.0024f);
        F.rect(-W * 0.12f, -H * 0.40f, W * 0.12f, H * 0.40f);   // écran central du combiné
        if (volantSeriePour(d)) {   // WFS-414 : volant de série (Giulia) : rapport centré au-dessus de la vitesse remontée par le moyeu
            vitesse(W * 0.11f, -H * 0.34f, H * 0.22f);
            rapportAuDessus(W * 0.11f, H * 0.28f);
        } else {
            rapport(A, -H * 0.08f, -H * 0.02f, H * 0.28f, val.rapport);
            vitesse(W * 0.11f, -H * 0.34f, H * 0.22f);
        }
        break;
    }
    case Combine::PeugeotICockpit: {   // i-Cockpit 3D (208), vu au-dessus du volant : régime en barre en haut, vitesse et rapport au centre
        const int n = 30;
        for (int k = 0; k < n; k++) {
            const float f = (k + 0.5f) / n, x = -W * 0.42f + W * 0.84f * f;
            Plan2D& c = f <= reg ? (f >= 0.84f ? R : A) : F;
            c.rect(x, H * 0.22f, x + W * 0.84f / n * 0.62f, H * 0.40f);
        }
        nombre(A, H * 0.30f, -H * 0.30f, H * 0.42f, (int)kmh, 3);
        rapport(A, W * 0.28f, -H * 0.22f, H * 0.30f, val.rapport);
        barreNitro(-W * 0.42f, -H * 0.36f, -W * 0.20f, -H * 0.30f);
        break;
    }
    case Combine::LamboCompteTours: {   // Gallardo : grand compte-tours au centre (rapport numérique dedans), vitesse à droite, jauges à gauche
        const float rc = H * 0.48f, rs = H * 0.30f;
        cadran(F, A, R, {0, 0}, rc, reg, 0.86f, 30, 0.0026f);
        rapport(A, -H * 0.06f, -H * 0.02f, H * 0.18f, val.rapport);
        cadran(F, A, R, {W * 0.33f, -H * 0.06f}, rs, vit, 2.f, 18, 0.0018f);
        vitesse(H * 0.21f, -H * 0.42f, H * 0.22f);   // WFS-323 : au centre, sous le rapport (à droite, la jante la cachait)
        cadran(F, A, R, {-W * 0.33f, -H * 0.06f}, rs, nit, 2.f, 18, 0.0018f);   // jauge : réserve de nitro
        break;
    }
    case Combine::FordDigital: {   // Shelby GT500 : compte-tours en arc sur toute la largeur, vitesse au centre, rapport à droite
        const vec2 c{0, -H * 0.62f};
        arcBarres(A, F, R, c, H * 0.90f, H * 1.05f, PI * 0.88f, PI * 0.12f, 46, reg, 0.86f, 0.0030f);
        vitesse(H * 0.32f, -H * 0.30f, H * 0.40f);
        rapport(A, W * 0.30f, -H * 0.24f, H * 0.30f, val.rapport);
        barreNitro(-W * 0.44f, -H * 0.40f, -W * 0.24f, -H * 0.34f);
        break;
    }
    case Combine::Aucun: {   // Model 3 : pas de combiné ; bande de gauche de l'écran central : vitesse, « D » / « R », nitro
        nombre(A, W * 0.30f, H * 0.02f, H * 0.32f, (int)kmh, 3);
        chiffre(A, -W * 0.38f, H * 0.06f, H * 0.22f, val.rapport < 0 ? SEG_R : val.kmh < 1 ? SEG_N : SEG_D);
        barreNitro(-W * 0.38f, -H * 0.30f, W * 0.30f, -H * 0.25f);
        break;
    }
    }
    const mat4 M = surEcran ? cam * versOeil(d.ecranPos, d.ecranLacet) * translate({-d.ecranW * 0.5f + W * 0.5f + 0.008f, 0, 0.0004f})
                            : cam * versOeil(d.combinePos, 0);
    for (int k : {D_FOND, D_ACTIF, D_ALERTE}) {
        if (di_[k].empty()) continue;
        r.updateMesh(dyn_[k], dv_[k], di_[k]);
        r.draw(dyn_[k], mat(val.nuit ? dynNuit_[k] : dynMat_[k]), M); appels_++;
    }
}

void Interieur::tailleCarte(float& w, float& h) const {
    if (!def_) { w = h = 0; return; }
    if (def_->combine == Combine::Aucun) { w = def_->ecranW * 0.62f; h = def_->ecranH * 0.90f; return; }   // WFS-201 : Model 3
    if (def_->ecranW > 0) { w = def_->ecranW; h = def_->ecranH; return; }
    w = def_->combineW / 3 * 0.92f; h = def_->combineH * 0.86f;   // SF90 : tiers droit du combiné
}

void Interieur::dessinerCarte(Renderer& r, const mat4& cam, const std::vector<vec2>& route, vec2, double, bool nuit) {
    const InterieurDef& d = *def_;
    for (int k : {D_ROUTE, D_FLECHE}) { dv_[k].clear(); di_[k].clear(); }
    Plan2D Ro{&dv_[D_ROUTE], &di_[D_ROUTE], 0.0012f}, Fl{&dv_[D_FLECHE], &di_[D_FLECHE], 0.0016f};
    float W, H; tailleCarte(W, H);
    // `route` : segments déjà dans le repère de l'écran (paires de points, de −W/2 à W/2 et de −H/2 à H/2, découpés)
    for (size_t k = 0; k + 1 < route.size(); k += 2) Ro.segment(route[k], route[k + 1], H * 0.035f);
    // flèche du joueur : pointe vers le haut (devant), à CARTE_JOUEUR_Y de la hauteur
    const float y = -H / 2 + H * CARTE_JOUEUR_Y, s = H * 0.07f;
    Fl.quad({-s * 0.6f, y - s * 0.55f}, {0, y - s * 0.25f}, {s * 0.6f, y - s * 0.55f}, {0, y + s * 0.65f});
    (void)W;
    const mat4 M = d.combine == Combine::Aucun   // WFS-201 : Model 3 : partie droite de l'écran central
                       ? cam * versOeil(d.ecranPos, d.ecranLacet) * translate({d.ecranW * 0.5f - W * 0.5f - 0.008f, 0, 0.0004f})
                 : d.ecranW > 0 ? cam * versOeil(d.ecranPos, d.ecranLacet)
                                : cam * versOeil(d.combinePos, 0) * translate({d.combineW / 3, 0, 0.0004f});   // SF90 : tiers droit du combiné
    for (int k : {D_ROUTE, D_FLECHE}) {
        if (di_[k].empty()) continue;
        r.updateMesh(dyn_[k], dv_[k], di_[k]);
        r.draw(dyn_[k], mat(nuit ? dynNuit_[k] : dynMat_[k]), M); appels_++;
    }
}

void Interieur::dessiner(Renderer& r, const mat4& cam, const EtatInterieur& e, const std::vector<vec2>& route, vec2 joueur, double cap) {
    if (!def_) return;
    const InterieurDef& d = *def_;
    appels_ = 0;
    const mat4 colonne = cam * translate(d.volantPos) * rotateX(d.volantIncl), vol = colonne * rotateZ(e.volant);
    // WFS-324 : l'habitacle ne se voit pas dans les rétroviseurs (la voiture du joueur y est dessinée entière, game.cpp)
    r.horsRetro = true;
    for (const Piece& p : pieces_) {
        int m = p.mat;
        if (m == M_FOND_ECRAN && e.v.nuit) m = M_FOND_ECRAN_NUIT;
        if (m == M_AMBIANCE && e.v.nuit) m = M_AMBIANCE_NUIT;   // WFS-325 : lumière d'ambiance allumée la nuit
        const Material& mm = (e.v.nuit && m < (int)matsNuit_.size() ? matsNuit_ : mats_)[(size_t)m];
        // WFS-324 : banc « cockpit-voisine » (WFS-349) : une glace montre la voisine par reflet, comme une vitre : jamais en couleur clé
        const bool glaceVraie = g_interieurCle && g_gabaritCeinture && m >= M_RETRO0 && m <= M_RETRO2;
        r.draw(p.m, glaceVraie ? mm : mat(mm), p.attache == 1 ? vol : p.attache == 2 ? colonne : cam); appels_++;
    }
    // essuie-glaces : pivots au bas du pare-brise ; plan du pare-brise (axe x, axe « haut » le long des montants)
    {
        const vec3 base{cx_, d.pareBriseY - 0.01f, d.pareBriseZ}, haut{cx_, HAUT_PARE_BRISE_Y, HAUT_PARE_BRISE_Z};   // WFS-323 : vrai haut du pare-brise
        const vec3 ex{1, 0, 0}, eu = normalize(haut - base), en = cross(ex, eu);
        auto repere = [&](vec3 o, float sens) {   // sens +1 : repos vers la droite, tourne vers le haut ; −1 : en miroir
            mat4 B = mat4::identity();
            const vec3 x = ex * sens;
            B.at(0, 0) = x.x; B.at(1, 0) = x.y; B.at(2, 0) = x.z;
            B.at(0, 1) = eu.x; B.at(1, 1) = eu.y; B.at(2, 1) = eu.z;
            B.at(0, 2) = en.x * sens; B.at(1, 2) = en.y * sens; B.at(2, 2) = en.z * sens;
            return translate(o) * B;
        };
        {   // WFS-334 : pare-brise et balais de l'image, pour la pluie (gouttes derrière la vitre et sur la vitre)
            PareBriseVu& pb = pareBriseVu();
            pb.vu = true; pb.repere = cam; pb.ex = ex; pb.eu = eu; pb.angle = e.essuie;
            coinsVitre(cx_, montant_, d.pareBriseY, d.pareBriseZ, haut.y, haut.z, pb.coins);   // WFS-354 (RG-01) : axes des montants
            const float dx[2] = {d.essuie == Essuie::Unique ? -0.02f : -0.62f, d.essuie == Essuie::Opposition ? 0.62f : 0.08f};
            const float sn[2] = {d.essuie == Essuie::Unique ? -1.f : 1.f, d.essuie == Essuie::Opposition ? -1.f : 1.f};
            pb.balais = 0;
            for (int k = 0; k < (d.essuie == Essuie::Unique ? 1 : 2); k++)
                if (bras_[k].indexCount) { pb.pivot[pb.balais] = base + vec3{dx[k], 0, 0}; pb.sens[pb.balais] = sn[k]; pb.longueur[pb.balais] = k == 0 ? d.balaiG : d.balaiD; pb.balais++; }
        }
        switch (d.essuie) {
        case Essuie::Paralleles:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.62f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            if (bras_[1].indexCount) { r.draw(bras_[1], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{0.08f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            break;
        case Essuie::Opposition:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.62f, 0, 0}, 1) * rotateZ(e.essuie)); appels_++; }
            if (bras_[1].indexCount) { r.draw(bras_[1], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{0.62f, 0, 0}, -1) * rotateZ(e.essuie)); appels_++; }
            break;
        case Essuie::Unique:
            if (bras_[0].indexCount) { r.draw(bras_[0], mat(e.v.nuit ? brasNuit_ : brasMat_), cam * repere(base + vec3{-0.02f, 0, 0}, -1) * rotateZ(e.essuie)); appels_++; }
            break;
        }
    }
    dessinerCombine(r, cam, e.v);
    dessinerCarte(r, cam, route, joueur, cap, e.v.nuit);
    if (g_interieurCle && g_gabaritCeinture && gabarit_.indexCount) r.draw(gabarit_, cleDouce_, cam);   // WFS-349 : banc seulement (non compté)
    r.horsRetro = false;
}

} // namespace wfs::cockpit
