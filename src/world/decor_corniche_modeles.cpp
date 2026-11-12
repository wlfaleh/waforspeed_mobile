// WaForSpeed — WFS-371 : objets de la plage de la Corniche, modélisés (voir decor_corniche_modeles.h).
#include "decor_corniche_modeles.h"
#include <algorithm>
#include <cmath>
#include "decor_campagne_modeles.h"   // poutre (pièce chanfreinée), WFS-372

namespace wfs {
namespace corniche {

using marina::eventail;
using marina::ligne;
using marina::loft;
using campagne::poutre;

namespace {

const vec3 HAUT{0, 1, 0}, AX{1, 0, 0}, AZ{0, 0, 1};

// petit générateur déterministe (variantes des modèles), sans état global
struct Hasard {
    uint32_t s;
    float operator()() { s = s * 1664525u + 1013904223u; return (float)((s >> 8) & 0xffff) / 65535.f; }
    float sym(float a) { return ((*this)() * 2 - 1) * a; }
};

// quadrilatère plan (2 triangles), tourné vers dehors
void quadPlat(Maille& m, vec3 a, vec3 b, vec3 c, vec3 d, int couleur, vec3 dehors) {
    vec3 nn = cross(b - a, c - a);
    if (dot(nn, nn) < 1e-14f) nn = cross(c - a, d - a);
    if (dot(nn, nn) < 1e-14f) return;
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const vec3& p : {a, b, c, d}) m.v.push_back({p, nn, couleur});
    if (inverse) m.i.insert(m.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
    else m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
}
void triPlat(Maille& m, vec3 a, vec3 b, vec3 c, int couleur, vec3 dehors) {
    vec3 nn = cross(b - a, c - a);
    if (dot(nn, nn) < 1e-14f) return;
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const vec3& p : {a, b, c}) m.v.push_back({p, nn, couleur});
    if (inverse) m.i.insert(m.i.end(), {k0, k0 + 2, k0 + 1}); else m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2});
}
// base orthonormée autour d'un axe
void base(vec3 ax, vec3& e1, vec3& e2) {
    e1 = std::fabs(ax.y) < 0.9f ? normalize(cross(ax, HAUT)) : normalize(cross(ax, AX));
    e2 = cross(ax, e1);
}
// pièce de révolution d'axe ax passant par o : profil (abscisse le long de l'axe, rayon), n côtés ; bouts fermés quand le
// rayon du premier / dernier point n'est pas nul
void revolution(Maille& m, vec3 o, vec3 ax, const std::vector<std::pair<float, float>>& prof, int n, int couleur) {
    ax = normalize(ax);
    vec3 e1, e2; base(ax, e1, e2);
    std::vector<std::vector<vec3>> an;
    for (auto [x, r] : prof) {
        std::vector<vec3> ring;
        for (int k = 0; k < n; k++) { const float a = 2 * PI * k / n; ring.push_back(o + ax * x + e1 * (std::cos(a) * std::max(r, 1e-4f)) + e2 * (std::sin(a) * std::max(r, 1e-4f))); }
        an.push_back(ring);
    }
    loft(m, an, couleur, true, o + ax * prof.front().first, o + ax * prof.back().first);
    if (prof.front().second > 1e-3f) eventail(m, an.front(), couleur, -ax);
    if (prof.back().second > 1e-3f) eventail(m, an.back(), couleur, ax);
}
void tube(Maille& m, vec3 a, vec3 b, float r, int n, int couleur) {
    const float L = length(b - a);
    if (L < 1e-4f) return;
    revolution(m, a, b - a, {{0, r}, {L, r}}, n, couleur);
}
// tube cintré le long d'une ligne brisée (cadres, crosse) : anneaux perpendiculaires à la tangente moyenne ; bouts fermés
void tubeCourbe(Maille& m, const std::vector<vec3>& pts, float r, int n, int couleur) {
    if (pts.size() < 2) return;
    std::vector<std::vector<vec3>> an;
    vec3 e1prec{0, 0, 0};
    for (size_t k = 0; k < pts.size(); k++) {
        const vec3 t = normalize(k == 0 ? pts[1] - pts[0] : k + 1 == pts.size() ? pts[k] - pts[k - 1] : normalize(pts[k] - pts[k - 1]) + normalize(pts[k + 1] - pts[k]));
        vec3 e1, e2; base(t, e1, e2);
        if (k > 0) {   // repère transporté (pas de torsion d'un anneau au suivant)
            e1 = e1prec - t * dot(e1prec, t);
            if (length(e1) < 1e-4f) base(t, e1, e2); else { e1 = normalize(e1); e2 = cross(t, e1); }
        }
        e1prec = e1;
        std::vector<vec3> ring;
        for (int j = 0; j < n; j++) { const float a = 2 * PI * j / n; ring.push_back(pts[k] + e1 * (std::cos(a) * r) + e2 * (std::sin(a) * r)); }
        an.push_back(ring);
    }
    // loft orienté vers l'extérieur du tube : sens donné par le premier anneau (rayon vers dehors)
    const size_t i0 = m.i.size(), v0 = m.v.size();
    loft(m, an, couleur, true, pts.front(), pts.back());
    // loft() oriente par rapport au segment premier → dernier point : pour un tube très cintré, on vérifie triangle par triangle
    for (size_t k = i0; k < m.i.size(); k += 3) {
        const uint32_t a = m.i[k], b = m.i[k + 1], c = m.i[k + 2];
        const size_t anneau = (a - v0) / (size_t)n;
        const vec3 nn = cross(m.v[b].p - m.v[a].p, m.v[c].p - m.v[a].p), centre = pts[std::min(anneau, pts.size() - 1)];
        if (dot(nn, m.v[a].p - centre) < 0) std::swap(m.i[k + 1], m.i[k + 2]);
    }
    for (size_t k = v0; k < m.v.size(); k++) {   // normales radiales (tube lisse)
        const size_t anneau = (k - v0) / (size_t)n;
        m.v[k].n = normalize(m.v[k].p - pts[anneau]);
    }
    eventail(m, an.front(), couleur, pts.front() - pts[1]);
    eventail(m, an.back(), couleur, pts.back() - pts[pts.size() - 2]);
}
// tore (anneau de repose-pied, passepoil, piton) d'axe ax, rayon R, section r
void tore(Maille& m, vec3 c, vec3 ax, float R, float r, int n1, int n2, int couleur) {
    vec3 e1, e2; base(normalize(ax), e1, e2);
    std::vector<vec3> pts;
    for (int k = 0; k <= n1; k++) { const float a = 2 * PI * k / n1; pts.push_back(c + e1 * (std::cos(a) * R) + e2 * (std::sin(a) * R)); }
    // anneau fermé : on construit le tube sur le cercle et on referme sans bouts (premier = dernier point)
    std::vector<std::vector<vec3>> an;
    const size_t v0 = m.v.size(), i0 = m.i.size();
    for (int k = 0; k <= n1; k++) {
        const vec3 rad = normalize(pts[k] - c), up = normalize(ax);
        std::vector<vec3> ring;
        for (int j = 0; j < n2; j++) { const float a = 2 * PI * j / n2; ring.push_back(pts[k] + rad * (std::cos(a) * r) + up * (std::sin(a) * r)); }
        an.push_back(ring);
    }
    loft(m, an, couleur, true, c - ax, c + ax);
    for (size_t k = i0; k < m.i.size(); k += 3) {
        const uint32_t a = m.i[k], b = m.i[k + 1], cc = m.i[k + 2];
        const vec3 nn = cross(m.v[b].p - m.v[a].p, m.v[cc].p - m.v[a].p), centre = pts[(a - v0) / (size_t)n2];
        if (dot(nn, m.v[a].p - centre) < 0) std::swap(m.i[k + 1], m.i[k + 2]);
    }
    for (size_t k = v0; k < m.v.size(); k++) m.v[k].n = normalize(m.v[k].p - pts[(k - v0) / (size_t)n2]);
}
// pièce irrégulière en bois (rondin, bûche) d'axe a → b : anneaux bosselés (graine), bouts coupés de la couleur bout
void bois(Maille& m, vec3 a, vec3 b, float r0, float r1, int n, int anneaux, uint32_t graine, int couleur, int bout) {
    Hasard H{graine};
    const vec3 ax = normalize(b - a);
    vec3 e1, e2; base(ax, e1, e2);
    std::vector<float> bosse(n);
    for (float& v : bosse) v = H.sym(0.06f);
    std::vector<std::vector<vec3>> an;
    for (int i = 0; i <= anneaux; i++) {
        const float u = (float)i / anneaux, r = r0 + (r1 - r0) * u;
        const vec3 c = a + (b - a) * u + (e1 * H.sym(0.015f) + e2 * H.sym(0.015f)) * r * (i > 0 && i < anneaux ? 1.f : 0.f);
        std::vector<vec3> ring;
        for (int k = 0; k < n; k++) {
            const float t = 2 * PI * k / n, rr = r * (1 + bosse[k] + H.sym(0.03f));
            ring.push_back(c + e1 * (std::cos(t) * rr) + e2 * (std::sin(t) * rr));
        }
        an.push_back(ring);
    }
    loft(m, an, couleur, true, a, b);
    eventail(m, an.front(), bout, -ax);
    eventail(m, an.back(), bout, ax);
}

} // namespace

std::vector<vec2> profilLisse() {
    // double onde (W) : deux crêtes de 83 mm, creux central de boulonnage à plat ; bords roulés en haut et en bas
    return {{-0.012f, 0.440f}, {-0.030f, 0.448f}, {-0.062f, 0.462f}, {-0.080f, 0.478f}, {-0.083f, 0.495f}, {-0.078f, 0.512f},
            {-0.055f, 0.535f}, {-0.020f, 0.560f}, {0.f, 0.575f}, {0.f, 0.615f}, {-0.020f, 0.630f}, {-0.055f, 0.655f},
            {-0.078f, 0.678f}, {-0.083f, 0.695f}, {-0.080f, 0.712f}, {-0.062f, 0.728f}, {-0.030f, 0.742f}, {-0.012f, 0.750f}};
}

// ---------------------------------------------------------------- parasol de plage
Modele parasol(const Couleurs& c, bool loin) {
    Modele M;
    const float R = PARASOL_DIAM / 2, yA = 2.10f, yB = 1.74f;   // sommet de la toile, bord (pente de 22° : toile tendue)
    const int nb = PARASOL_BALEINES, parPan = loin ? 1 : 4, np = nb * parPan;
    auto toileY = [&](float f) { return yA - (yA - yB) * std::pow(f, 1.15f); };
    // mât en deux parties (Ø 28 mm puis 25 mm), bague de jonction, vrille d'ancrage dans le sable (collerette et poignée)
    if (loin) tube(M.uni, {0, 0, 0}, {0, yA, 0}, PARASOL_MAT / 2, 5, c.blanc);
    else {
        tube(M.uni, {0, -0.05f, 0}, {0, 1.12f, 0}, PARASOL_MAT / 2, 8, c.blanc);
        revolution(M.uni, {0, 1.08f, 0}, HAUT, {{0, 0.017f}, {0.06f, 0.017f}}, 8, c.alu);
        tube(M.uni, {0, 1.12f, 0}, {0, yA + 0.02f, 0}, 0.0125f, 8, c.blanc);
        revolution(M.acier, {0, -0.02f, 0}, HAUT, {{0, 0.045f}, {0.03f, 0.045f}, {0.05f, 0.03f}, {0.06f, 0.f}}, 8, c.alu);   // collerette
        tube(M.acier, {-0.09f, 0.035f, 0}, {0.09f, 0.035f, 0}, 0.007f, 5, c.alu);                                            // poignée
        // coulisse (noix) à 1,55 m, embout de la toile (pointe) au sommet
        revolution(M.uni, {0, 1.52f, 0}, HAUT, {{0, 0.02f}, {0.02f, 0.022f}, {0.08f, 0.022f}, {0.1f, 0.016f}}, 8, c.alu);
        revolution(M.uni, {0, yA - 0.01f, 0}, HAUT, {{0, 0.03f}, {0.04f, 0.026f}, {0.07f, 0.012f}, {PARASOL_HAUT - yA + 0.01f, 0.f}}, 8, c.blanc);
    }
    // toile : 8 pans tendus entre les baleines (creux au milieu de chaque pan), lambrequin festonné
    const std::vector<float> fr = loin ? std::vector<float>{0.03f, 1.f} : std::vector<float>{0.03f, 0.3f, 0.65f, 1.f};
    std::vector<std::vector<vec3>> an;
    for (float f : fr) {
        std::vector<vec3> ring;
        for (int i = 0; i < np; i++) {
            const float a = 2 * PI * i / np, u = (float)(i % parPan) / parPan, creux = 0.035f * f * std::sin(PI * u);   // 0 sur la baleine
            const float r = R * f - creux;
            ring.push_back({std::cos(a) * r, toileY(f) - creux * 0.6f, std::sin(a) * r});
        }
        an.push_back(ring);
    }
    loft(M.uni, an, COUL_VAR, true, {0, 3, 0}, {0, 1.2f, 0});
    if (!loin) {
        std::vector<std::vector<vec3>> lam{an.back(), an.back()};
        for (int i = 0; i < np; i++) {   // festons : 2 arrondis par pan, bord du bas à 12 cm (8 cm entre deux festons)
            const float u = (float)(i % parPan) / parPan;
            lam[1][i].y -= PARASOL_LAMBREQUIN - 0.04f * (1 - std::fabs(std::sin(2 * PI * u)));
        }
        loft(M.uni, lam, COUL_VAR, true, {0, 0, 0}, {0, 2, 0});
        // baleines (sous la toile) et tiges de coulisse (de la noix au milieu de chaque baleine)
        for (int k = 0; k < nb; k++) {
            const float a = 2 * PI * k / nb;
            const vec3 d{std::cos(a), 0, std::sin(a)};
            const vec3 p0 = d * 0.03f + HAUT * (yA - 0.02f), p1 = d * (R * 0.98f) + HAUT * (toileY(0.98f) - 0.015f);
            ligne(M.acier, p0, p1, 0.009f, c.alu);
            const vec3 q = p0 + (p1 - p0) * 0.5f;
            ligne(M.acier, d * 0.022f + HAUT * 1.60f, q - HAUT * 0.01f, 0.006f, c.alu);
        }
    }
    return M;
}

// ---------------------------------------------------------------- bain de soleil
Modele transat(const Couleurs& c, bool loin) {
    Modele M;
    // largeur hors tout (roues comprises) de 0,65 m ; longueur développée (à plat) de 1,90 m : assise de TRANSAT_ARTIC, dossier
    // du reste, relevé de TRANSAT_DOSSIER_DEG
    const float rt = TRANSAT_TUBE / 2, w = TRANSAT_W / 2 - 0.055f, ya = TRANSAT_ASSISE - rt;
    const float zp = -TRANSAT_L / 2 + rt, zc = zp + TRANSAT_ARTIC, ld = TRANSAT_L - TRANSAT_ARTIC - 2 * rt, da = TRANSAT_DOSSIER_DEG * PI / 180.f;
    const vec3 haut{0, ya + ld * std::sin(da), zc + ld * std::cos(da)};   // haut du dossier (bout de la tête)
    // longerons cintrés : pied arrondi, assise, articulation, dossier relevé
    for (float s : {-1.f, 1.f}) {
        const std::vector<vec3> L = {{s * w, ya - 0.03f, zp}, {s * w, ya, zp + 0.06f}, {s * w, ya, zc}, {s * w, haut.y, haut.z}};
        tubeCourbe(M.uni, L, rt, loin ? 5 : 8, c.alu);
    }
    if (!loin) {
        tube(M.uni, {-w, ya - 0.03f, zp}, {w, ya - 0.03f, zp}, rt * 0.8f, 6, c.alu);   // barre du pied
        tube(M.uni, {-w, haut.y, haut.z}, {w, haut.y, haut.z}, rt * 0.8f, 6, c.alu);                                               // barre de tête
        tube(M.uni, {-w, ya, zc}, {w, ya, zc}, rt * 0.7f, 6, c.alu);                                                               // traverse
        // pieds : arceaux au pied, jambes de force sous la tête, roues Ø 16 cm à la tête
        for (float s : {-1.f, 1.f}) {
            tube(M.uni, {s * w, ya, zp + 0.18f}, {s * (w - 0.01f), 0.01f, zp + 0.14f}, rt * 0.85f, 6, c.alu);
            revolution(M.uni, {s * (w - 0.01f), 0, zp + 0.14f}, HAUT, {{0, 0.02f}, {0.015f, 0.018f}}, 6, c.noir);   // patin
            tube(M.uni, {s * w, ya, zc - 0.05f}, {s * w, 0.08f, zc + 0.30f}, rt * 0.85f, 6, c.alu);
            revolution(M.uni, {s * (w + 0.035f), 0.08f, zc + 0.30f}, AX, {{-0.02f, 0.05f}, {-0.016f, 0.07f}, {-0.005f, 0.08f}, {0.005f, 0.08f}, {0.016f, 0.07f}, {0.02f, 0.05f}}, 10, c.pneu);
            revolution(M.uni, {s * (w + 0.035f), 0.08f, zc + 0.30f}, AX, {{-0.022f, 0.02f}, {0.022f, 0.02f}}, 6, c.alu);   // moyeu
        }
        tube(M.uni, {-w, 0.08f, zc + 0.30f}, {w, 0.08f, zc + 0.30f}, 0.006f, 5, c.alu);   // essieu
    }
    // toile textilène tendue entre les longerons (assise et dossier), légère flèche au milieu
    auto toile = [&](vec3 a0, vec3 a1, int nl, int nt) {
        std::vector<std::vector<vec3>> an;
        for (int i = 0; i <= nl; i++) {
            const vec3 p = a0 + (a1 - a0) * ((float)i / nl);
            std::vector<vec3> ring;
            for (int j = 0; j <= nt; j++) {
                const float x = -w + rt * 0.5f + (2 * w - rt) * j / nt, fl = 0.018f * (1 - std::pow(x / w, 2.f));
                ring.push_back({x, p.y - fl, p.z});
            }
            an.push_back(ring);
        }
        loft(M.uni, an, c.toile, false, a0 + vec3{0, -1, 0}, a1 + vec3{0, -1, 0});
    };
    toile({0, ya, zp + 0.10f}, {0, ya, zc}, loin ? 1 : 4, loin ? 2 : 6);
    toile({0, ya, zc}, haut - vec3{0, 0, 0.03f}, loin ? 1 : 3, loin ? 2 : 6);
    return M;
}

// ---------------------------------------------------------------- parasol de paille
Modele parasolPaille(const Couleurs& c, bool loin) {
    Modele M;
    const float R = PAILLE_DIAM / 2;
    bois(M.uni, {0, -0.3f, 0}, {0, PAILLE_HAUT - 0.15f, 0}, PAILLE_MAT / 2, PAILLE_MAT / 2 * 0.85f, loin ? 6 : 8, loin ? 2 : 5, 71, c.bois, c.bois);
    Hasard H{1234};
    // trois rangs de chaume (du haut vers le bas), bords à franges irrégulières ; dessous visible (épaisseur de 6 à 10 cm)
    struct Rang { float ri, ro, yh, yb; };
    const Rang rangs[3] = {{0.f, 0.58f * R, PAILLE_HAUT, 2.40f}, {0.45f * R, 0.86f * R, 2.46f, 2.20f}, {0.76f * R, R, 2.27f, 2.02f}};
    const int n = loin ? 12 : 40;
    for (int k = 0; k < (loin ? 1 : 3); k++) {
        const Rang g = loin ? Rang{0.f, R, PAILLE_HAUT, 2.02f} : rangs[k];
        std::vector<vec3> h, b, d;
        for (int i = 0; i < n; i++) {
            const float a = 2 * PI * i / n + H.sym(0.02f), frange = loin ? 0 : (i % 2 ? 0.06f : 0.f) + H.sym(0.035f);
            const float ro = g.ro + frange * 0.6f;
            h.push_back({std::cos(a) * std::max(g.ri, 0.02f), g.yh, std::sin(a) * std::max(g.ri, 0.02f)});
            b.push_back({std::cos(a) * ro, g.yb - frange, std::sin(a) * ro});
            d.push_back({std::cos(a) * (ro - 0.12f), g.yb + 0.08f, std::sin(a) * (ro - 0.12f)});
        }
        loft(M.paille, {h, b}, c.bois, true, {0, g.yh + 2, 0}, {0, g.yb - 1, 0});
        if (!loin) loft(M.paille, {b, d}, c.bois, true, {0, g.yb + 0.5f, 0}, {0, g.yb - 0.5f, 0});
    }
    if (!loin) {   // ligature du sommet (corde) et 8 tiges de bambou sous le chaume
        revolution(M.uni, {0, PAILLE_HAUT - 0.14f, 0}, HAUT, {{0, 0.06f}, {0.05f, 0.07f}, {0.10f, 0.05f}, {0.14f, 0.f}}, 8, c.rotin);
        for (int k = 0; k < 8; k++) {
            const float a = 2 * PI * k / 8 + 0.2f;
            tube(M.uni, {std::cos(a) * 0.04f, 2.30f, std::sin(a) * 0.04f}, {std::cos(a) * 0.95f * R, 2.06f, std::sin(a) * 0.95f * R}, 0.016f, 5, c.rotin);
        }
    }
    return M;
}

// ---------------------------------------------------------------- tabouret, table basse, pouf, bouteille
Modele tabouret(const Couleurs& c) {
    Modele M;
    const float rE = TABOURET_EMBASE / 2;
    revolution(M.acier, {0, 0, 0}, HAUT, {{0, rE}, {0.008f, rE}, {0.018f, rE - 0.02f}, {0.035f, 0.06f}, {0.05f, 0.03f}, {0.06f, 0.f}}, 16, c.alu);   // embase bombée
    tube(M.acier, {0, 0.05f, 0}, {0, TABOURET_ASSISE - 0.05f, 0}, 0.025f, 10, c.alu);                                                           // fût Ø 50 mm
    tore(M.acier, {0, 0.30f, 0}, HAUT, 0.20f, 0.011f, 16, 6, c.alu);                                                                            // repose-pied
    for (int k = 0; k < 3; k++) { const float a = 2 * PI * k / 3; tube(M.acier, {std::cos(a) * 0.024f, 0.30f, std::sin(a) * 0.024f}, {std::cos(a) * 0.192f, 0.30f, std::sin(a) * 0.192f}, 0.008f, 5, c.alu); }
    revolution(M.acier, {0, TABOURET_ASSISE - 0.07f, 0}, HAUT, {{0, 0.03f}, {0.015f, 0.08f}, {0.025f, 0.08f}}, 10, c.alu);                    // platine sous l'assise
    const float r = TABOURET_DIAM / 2;   // assise de bois, chant arrondi, légèrement creusée
    revolution(M.uni, {0, TABOURET_ASSISE - 0.045f, 0}, HAUT, {{0, 0.f}, {0.f, r - 0.02f}, {0.012f, r}, {0.033f, r}, {0.045f, r - 0.015f}, {0.040f, r * 0.6f}, {0.036f, 0.f}}, 20, c.boisSombre);
    return M;
}

Modele comptoir(const Couleurs& c, float L) {
    Modele M;
    const float hz = L / 2, y = COMPTOIR_HAUT, ep = 0.05f;
    // socle en retrait de 8 cm (plinthe sombre), caisson à lames verticales de 12 cm (joints de 6 mm) sur les 4 faces
    poutre(M.uni, {0.05f, 0.f, -hz + 0.1f}, {0.05f, 0.f, hz - 0.1f}, HAUT, 0.44f, 0.20f, 0.f, c.boisSombre);
    const float lame = 0.12f;
    for (float z = -hz; z < hz - 1e-3f; z += lame) {
        const float zm = std::min(z + lame, hz), m = (z + zm) / 2;
        poutre(M.uni, {-0.29f, 0.10f, m}, {-0.29f, y - ep, m}, AX, zm - z - 0.006f, 0.022f, 0.003f, c.bois);   // face aux clients
    }
    for (float x = -0.28f; x < 0.28f; x += lame) for (float s : {-1.f, 1.f})   // côtés
        poutre(M.uni, {x + lame / 2, 0.10f, s * (hz - 0.011f)}, {x + lame / 2, y - ep, s * (hz - 0.011f)}, AZ, std::min(lame, 0.30f - x) - 0.006f, 0.022f, 0.003f, c.bois);
    poutre(M.uni, {0.29f, 0.10f, 0}, {0.29f, y - ep, 0}, AX, 2 * hz, 0.02f, 0.f, c.bois);   // dos (côté barman)
    poutre(M.uni, {-0.06f, y - ep / 2, -hz - 0.05f}, {-0.06f, y - ep / 2, hz + 0.05f}, HAUT, 0.72f, ep, 0.012f, c.boisSombre);   // plateau
    poutre(M.uni, {0.10f, 0.80f, -hz + 0.05f}, {0.10f, 0.80f, hz - 0.05f}, HAUT, 0.36f, 0.025f, 0.004f, c.boisSombre);         // tablette du barman
    return M;
}

Modele etagere(const Couleurs& c, float L) {
    Modele M;
    const float hz = L / 2, H = ETAGERE_HAUT, p = 0.40f;
    poutre(M.uni, {0.19f, 0, 0}, {0.19f, H, 0}, AX, 2 * hz, 0.02f, 0.f, c.boisSombre);   // dos
    const int nm = std::max(2, (int)std::round(L / 1.3f) + 1);   // montants tous les ≈ 1,3 m
    for (int k = 0; k < nm; k++) {
        const float z = -hz + 0.011f + (2 * hz - 0.022f) * k / (nm - 1);
        poutre(M.uni, {0, 0, z}, {0, H, z}, AX, 0.022f, p, 0.003f, c.boisSombre);
    }
    for (float y : {0.10f, 0.55f, 1.05f, 1.55f, H - 0.011f})   // tablettes
        poutre(M.uni, {0, y, -hz}, {0, y, hz}, HAUT, p - 0.01f, 0.022f, 0.003f, c.boisSombre);
    return M;
}

Modele tableBasse(const Couleurs& c) {
    Modele M;
    const float h = TABLE_COTE / 2, y = TABLE_HAUT, ep = 0.022f;
    const int nl = 7;
    const float lw = (TABLE_COTE - (nl - 1) * 0.011f) / nl;
    for (int k = 0; k < nl; k++) {   // plateau à lattes, chants chanfreinés
        const float x = -h + lw / 2 + k * (lw + 0.011f);
        poutre(M.uni, {x, y - ep / 2, -h}, {x, y - ep / 2, h}, HAUT, lw, ep, 0.004f, c.bois);
    }
    for (float s : {-1.f, 1.f}) {   // ceinture et pieds de 50 mm chanfreinés
        poutre(M.uni, {-h + 0.05f, y - ep - 0.035f, s * (h - 0.04f)}, {h - 0.05f, y - ep - 0.035f, s * (h - 0.04f)}, HAUT, 0.022f, 0.07f, 0.003f, c.bois);
        poutre(M.uni, {s * (h - 0.04f), y - ep - 0.035f, -h + 0.05f}, {s * (h - 0.04f), y - ep - 0.035f, h - 0.05f}, HAUT, 0.022f, 0.07f, 0.003f, c.bois);
        for (float t : {-1.f, 1.f}) poutre(M.uni, {s * (h - 0.05f), 0, t * (h - 0.05f)}, {s * (h - 0.05f), y - ep, t * (h - 0.05f)}, AX, 0.05f, 0.05f, 0.005f, c.bois);
    }
    return M;
}

Modele pouf(const Couleurs& c) {
    (void)c;
    Modele M;
    const float r = POUF_DIAM / 2, H = POUF_HAUT;
    // profil d'un coussin rond rembourré : flancs bombés, dessus affaissé au centre, plis légers (ondulation radiale)
    const std::vector<std::pair<float, float>> prof = {{0.f, r * 0.80f}, {0.03f, r * 0.96f}, {0.12f, r}, {0.28f, r}, {H - 0.03f, r * 0.95f},
                                                       {H, r * 0.78f}, {H - 0.02f, r * 0.45f}, {H - 0.035f, 0.f}};
    const int n = 24;
    std::vector<std::vector<vec3>> an;
    for (auto [y, rr] : prof) {
        std::vector<vec3> ring;
        for (int k = 0; k < n; k++) { const float a = 2 * PI * k / n, pli = 1 + 0.018f * std::cos(8 * a) * (rr > r * 0.9f ? 1.f : 0.3f); ring.push_back({std::cos(a) * std::max(rr, 1e-4f) * pli, y, std::sin(a) * std::max(rr, 1e-4f) * pli}); }
        an.push_back(ring);
    }
    loft(M.uni, an, COUL_VAR, true, {0, 0, 0}, {0, H, 0});
    eventail(M.uni, an.front(), COUL_VAR, -HAUT);
    tore(M.uni, {0, H - 0.015f, 0}, HAUT, r * 0.88f, 0.008f, 24, 4, COUL_VAR);   // passepoil du dessus
    tore(M.uni, {0, 0.02f, 0}, HAUT, r * 0.88f, 0.008f, 24, 4, COUL_VAR);        // passepoil du dessous
    return M;
}

Modele bouteille(const Couleurs& c) {
    Modele M;
    const float r = BOUTEILLE_D / 2;
    revolution(M.uni, {0, 0, 0}, HAUT, {{0.f, r * 0.9f}, {0.004f, r}, {0.19f, r}, {0.215f, r * 0.8f}, {0.235f, 0.016f}, {0.275f, 0.0145f}, {0.280f, 0.f}}, 8, COUL_VAR);
    revolution(M.uni, {0, 0.275f, 0}, HAUT, {{0.f, 0.0155f}, {BOUTEILLE_H - 0.275f, 0.0155f}}, 8, c.noir);   // capsule
    return M;
}

// ---------------------------------------------------------------- poste de surveillance de plage
Modele posteSecours(const Couleurs& c, bool loin) {
    Modele M;
    const float P = POSTE_PLANCHER, h = POSTE_COTE / 2, hp = h + 0.25f, yc = P + POSTE_CABINE, pente = 0.30f;
    // 4 poteaux de pin de 15 cm (chanfreinés, enfouis de 0,4 m) et croix de Saint-André sur les 4 faces
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f}) poutre(M.uni, {sx * (hp - 0.1f), -0.4f, sz * (hp - 0.1f)}, {sx * (hp - 0.1f), P - 0.12f, sz * (hp - 0.1f)}, AX, 0.15f, 0.15f, 0.015f, c.bois);
    if (!loin) for (int f = 0; f < 4; f++) {
        const vec3 u = f < 2 ? AZ : AX, n = f < 2 ? AX * (f ? 1.f : -1.f) : AZ * (f == 3 ? 1.f : -1.f);
        const vec3 o = n * (hp - 0.1f + 0.1f);
        poutre(M.uni, o - u * (hp - 0.1f) + HAUT * 0.25f, o + u * (hp - 0.1f) + HAUT * (P - 0.35f), n, 0.12f, 0.04f, 0.006f, c.bois);
        poutre(M.uni, o + u * (hp - 0.1f) + HAUT * 0.25f, o - u * (hp - 0.1f) + HAUT * (P - 0.35f), n, 0.12f, 0.04f, 0.006f, c.bois);
    }
    // plancher à lames sur solives (débord de 0,25 m autour de la cabine)
    for (float s : {-1.f, 1.f}) poutre(M.uni, {-hp, P - 0.16f, s * (hp - 0.1f)}, {hp, P - 0.16f, s * (hp - 0.1f)}, HAUT, 0.075f, 0.18f, 0.008f, c.bois);
    const int nl = loin ? 1 : 16;
    for (int k = 0; k < nl; k++) {
        const float lw = 2 * hp / nl, z = -hp + lw * (k + 0.5f);
        poutre(M.uni, {-hp, P - 0.025f, z}, {hp, P - 0.025f, z}, HAUT, lw - (loin ? 0.f : 0.008f), 0.05f, 0.004f, c.planche);
    }
    // cabine : murs en planches à clin (rouge), baies vitrées côté mer (+x) et sur les côtés, porte côté terre (−x)
    struct Baie { float u0, u1, v0, v1; bool porte; };
    auto mur = [&](vec3 o, vec3 u, vec3 n, const std::vector<Baie>& baies) {
        const float L = POSTE_COTE;
        if (loin) {
            quadPlat(M.uni, o - u * h + HAUT * P, o + u * h + HAUT * P, o + u * h + HAUT * yc, o - u * h + HAUT * yc, c.rouge, n);
            for (const Baie& b : baies) quadPlat(M.uni, o + u * b.u0 + HAUT * (P + b.v0) + n * 0.01f, o + u * b.u1 + HAUT * (P + b.v0) + n * 0.01f,
                                                 o + u * b.u1 + HAUT * (P + b.v1) + n * 0.01f, o + u * b.u0 + HAUT * (P + b.v1) + n * 0.01f, b.porte ? c.blanc : c.vitre, n);
            return;
        }
        const float lame = 0.16f;
        for (float v = 0; v < POSTE_CABINE - 1e-3f; v += lame) {
            const float v1 = std::min(v + lame, POSTE_CABINE);
            std::vector<std::pair<float, float>> plein{{-L / 2, L / 2}};
            for (const Baie& b : baies) if (v1 > b.v0 + 1e-3f && v < b.v1 - 1e-3f) {
                std::vector<std::pair<float, float>> r;
                for (auto [a, z] : plein) { if (b.u0 > a) r.push_back({a, std::min(z, b.u0)}); if (b.u1 < z) r.push_back({std::max(a, b.u1), z}); }
                plein = r;
            }
            for (auto [a, z] : plein) if (z - a > 1e-3f) {   // lame inclinée (clin) : bas 2 cm plus dehors que le haut
                const vec3 p0 = o + u * a + HAUT * (P + v) + n * 0.02f, p1 = o + u * z + HAUT * (P + v) + n * 0.02f;
                const vec3 q0 = o + u * a + HAUT * (P + v1 + 0.01f), q1 = o + u * z + HAUT * (P + v1 + 0.01f);
                quadPlat(M.uni, p0, p1, q1, q0, c.rouge, n + HAUT * 0.1f);
                quadPlat(M.uni, p0, p1, p1 - n * 0.02f, p0 - n * 0.02f, c.rouge, -HAUT);   // chant du bas de la lame
            }
        }
        for (const Baie& b : baies) {   // dormant blanc de 6 cm, vitrage en retrait, porte à lames verticales
            const vec3 a = o + u * b.u0 + HAUT * (P + b.v0), d = o + u * b.u1 + HAUT * (P + b.v1);
            const float w = b.u1 - b.u0, ht = b.v1 - b.v0;
            const vec3 cbas = o + u * ((b.u0 + b.u1) / 2) + HAUT * (P + b.v0);
            quadPlat(M.uni, a - n * 0.05f, a + u * w - n * 0.05f, d - n * 0.05f, d - u * w - n * 0.05f, b.porte ? c.blanc : c.vitre, n);
            for (float s : {-1.f, 1.f}) poutre(M.uni, cbas + u * (s * (w / 2 + 0.03f)) + n * 0.01f, cbas + u * (s * (w / 2 + 0.03f)) + HAUT * ht + n * 0.01f, u, 0.06f, 0.08f, 0.005f, c.blanc);
            for (float yy : {0.f, ht}) poutre(M.uni, cbas + HAUT * yy - u * (w / 2 + 0.06f) + n * 0.01f, cbas + HAUT * yy + u * (w / 2 + 0.06f) + n * 0.01f, n, 0.08f, 0.06f, 0.005f, c.blanc);
            if (!b.porte) poutre(M.uni, cbas + n * -0.03f, cbas + HAUT * ht + n * -0.03f, u, 0.04f, 0.04f, 0.004f, c.blanc);   // meneau
            else for (int k = 1; k < 5; k++) poutre(M.uni, a + u * (w * k / 5) - n * 0.045f, a + u * (w * k / 5) + HAUT * ht - n * 0.045f, u, 0.012f, 0.01f, 0.f, c.blanc);
        }
    };
    mur({h, 0, 0}, AZ, AX, {{-0.85f, 0.85f, 0.95f, 1.75f, false}});
    mur({-h, 0, 0}, AZ, -AX, {{-0.4f, 0.4f, 0.f, 1.90f, true}});
    mur({0, 0, h}, AX, AZ, {{-0.5f, 0.5f, 0.95f, 1.75f, false}});
    mur({0, 0, -h}, AX, -AZ, {{-0.5f, 0.5f, 0.95f, 1.75f, false}});
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})   // cornières d'angle blanches
        poutre(M.uni, {sx * (h + 0.02f), P, sz * (h + 0.02f)}, {sx * (h + 0.02f), yc, sz * (h + 0.02f)}, AX, 0.09f, 0.09f, 0.008f, c.blanc);
    // toit à une pente (haut côté mer), débord de 0,3 m : plaque, rives et chevrons visibles dessous
    const float d = POSTE_DEBORD, y0 = yc + 0.05f, y1 = yc + 0.05f + pente;
    const vec3 A{-h - d, y0, -h - d}, B{h + d, y1, -h - d}, C{h + d, y1, h + d}, D{-h - d, y0, h + d};
    const vec3 nt = normalize(cross(D - A, B - A)), e = HAUT * 0.06f;
    quadPlat(M.uni, A + e, B + e, C + e, D + e, c.blanc, nt);
    quadPlat(M.uni, A, B, C, D, c.blanc, -nt);
    for (auto [p, q] : {std::pair{A, B}, std::pair{B, C}, std::pair{C, D}, std::pair{D, A}}) {   // rives de 0,15 m
        const vec3 mid = (p + q) * 0.5f, dehors = normalize(vec3{mid.x, 0, mid.z});
        quadPlat(M.uni, p - HAUT * 0.09f, q - HAUT * 0.09f, q + e, p + e, c.blanc, dehors);
    }
    if (!loin) for (int k = 0; k < 5; k++) {   // chevrons
        const float z = -h - d + 0.2f + k * (2 * (h + d) - 0.4f) / 4;
        poutre(M.uni, {-h - d + 0.05f, y0 - 0.05f, z}, {h + d - 0.05f, y1 - 0.05f, z}, HAUT, 0.05f, 0.08f, 0.005f, c.bois);
    }
    // garde-corps de 1,0 m autour du plancher (sauf l'arrivée de l'échelle), échelle de meunier côté terre
    const float yg = P + 1.0f;
    auto gc = [&](vec3 a, vec3 b) {
        const float L = length(b - a);
        const int n = std::max(1, (int)std::ceil(L / 0.9f));
        for (int k = 0; k <= n; k++) { const vec3 p = a + (b - a) * ((float)k / n); poutre(M.uni, p + HAUT * P, p + HAUT * yg, AX, 0.07f, 0.07f, 0.006f, c.blanc); }
        poutre(M.uni, a + HAUT * yg, b + HAUT * yg, HAUT, 0.09f, 0.04f, 0.006f, c.blanc);
        if (!loin) poutre(M.uni, a + HAUT * (P + 0.5f), b + HAUT * (P + 0.5f), HAUT, 0.06f, 0.03f, 0.004f, c.blanc);
    };
    gc({hp - 0.03f, 0, -hp + 0.03f}, {hp - 0.03f, 0, hp - 0.03f});
    gc({-hp + 0.03f, 0, hp - 0.03f}, {hp - 0.03f, 0, hp - 0.03f});
    gc({-hp + 0.03f, 0, -hp + 0.03f}, {hp - 0.03f, 0, -hp + 0.03f});
    const float ze = -hp + 0.35f;   // échelle (60 cm de large, 65° : de x = −hp − 1,1 au sol au bord du plancher)
    for (float s : {-0.3f, 0.3f}) poutre(M.uni, {-hp - 1.12f, 0, ze + 0.3f + s}, {-hp, P, ze + 0.3f + s}, AZ, 0.04f, 0.12f, 0.005f, c.bois);
    if (!loin) for (int k = 1; k < 9; k++) {
        const float u = k / 9.f;
        poutre(M.uni, {-hp - 1.12f + 1.12f * u, P * u, ze + 0.03f}, {-hp - 1.12f + 1.12f * u, P * u, ze + 0.57f}, HAUT, 0.12f, 0.03f, 0.004f, c.bois);
    }
    return M;
}

// ---------------------------------------------------------------- glissière à double onde : poteau et écarteur
Modele poteauGlissiere(const Couleurs& c) {
    Modele M;
    // poteau C 100 × 50 × 5 (âme côté route, ailes vers l'extérieur), enfoncé de 1 m ; tête à 0,72 m
    const float xa = GLISS_ECARTEUR / 2 + 0.05f;
    poutre(M.acier, {xa, -1.0f, 0}, {xa, 0.72f, 0}, AX, 0.005f, 0.10f, 0.f, c.alu);
    for (float s : {-1.f, 1.f}) poutre(M.acier, {xa + 0.025f, -1.0f, s * 0.0475f}, {xa + 0.025f, 0.72f, s * 0.0475f}, AX, 0.05f, 0.005f, 0.f, c.alu);
    // écarteur (tronçon de profilé en C de 0,30 m, côté lisse plié en double onde) entre la lisse (x = 0) et le poteau
    const float y0 = GLISS_HAUT - GLISS_LISSE + 0.0f, y1 = GLISS_HAUT;
    poutre(M.acier, {0.075f, 0.575f, 0}, {0.075f, 0.615f, 0}, HAUT, 0.15f, 0.12f, 0.003f, c.alu);
    for (float s : {-1.f, 1.f}) poutre(M.acier, {0.075f, y0 + 0.01f, s * 0.05f}, {0.075f, y1 - 0.01f, s * 0.05f}, HAUT, 0.15f, 0.005f, 0.f, c.alu);
    // boulons de la lisse (têtes bombées) dans le creux central
    for (float y : {0.585f, 0.605f}) revolution(M.acier, {-0.002f, y, 0}, -AX, {{0, 0.011f}, {0.004f, 0.010f}, {0.007f, 0.005f}, {0.008f, 0.f}}, 6, c.alu);
    return M;
}

// ---------------------------------------------------------------- scène d'été (podium, portique, fond de scène, DJ)
namespace {
// poutre treillis carrée (29 cm) de a à b : 4 membrures Ø 48 mm, diagonales Ø 20 mm sur les 4 faces tous les 0,5 m
void treillis(Maille& m, vec3 a, vec3 b, vec3 haut, int couleur, bool loin) {
    const vec3 ax = normalize(b - a);
    vec3 hv = haut - ax * dot(haut, ax); hv = normalize(hv);
    const vec3 sd = cross(ax, hv);
    const float e = TRUSS / 2 - 0.024f, L = length(b - a);
    const vec3 coins[4] = {sd * e + hv * e, sd * -e + hv * e, sd * -e + hv * -e, sd * e + hv * -e};
    for (const vec3& k : coins) tube(m, a + k, b + k, 0.024f, loin ? 4 : 6, couleur);
    if (loin) return;
    const int n = std::max(1, (int)std::round(L / 0.5f));
    for (int f = 0; f < 4; f++) {
        const vec3 p = coins[f], q = coins[(f + 1) % 4];
        for (int k = 0; k < n; k++) {
            const vec3 s0 = a + ax * (L * k / n), s1 = a + ax * (L * (k + 1) / n);
            if (k % 2 == 0) tube(m, s0 + p, s1 + q, 0.01f, 4, couleur); else tube(m, s0 + q, s1 + p, 0.01f, 4, couleur);
        }
    }
}
} // namespace

Modele scene(const Couleurs& c, bool loin) {
    Modele M;
    const float hx = SCENE_P / 2, hz = SCENE_L / 2, y = SCENE_PLANCHER;
    // praticables de 2 × 1 m (plateau de 6 cm, chants chanfreinés, joints de 5 mm) sur pieds d'échafaudage Ø 48 mm
    if (loin) poutre(M.uni, {0, y - 0.03f, -hz}, {0, y - 0.03f, hz}, HAUT, SCENE_P, 0.06f, 0.01f, c.planche);
    else for (int i = 0; i < 5; i++) for (int j = 0; j < 5; j++) {
        const float x = -hx + 0.5f + i, z = -hz + 1 + 2 * j;
        poutre(M.uni, {x, y - 0.03f, z - 0.9975f}, {x, y - 0.03f, z + 0.9975f}, HAUT, 0.995f, 0.06f, 0.008f, c.planche);
    }
    if (!loin) for (int i = 0; i <= 5; i++) for (int j = 0; j <= 5; j++) {
        const vec3 p{-hx + i * 1.f - (i == 5 ? 0.05f : i == 0 ? -0.05f : 0.f), 0, -hz + j * 2.f - (j == 5 ? 0.05f : j == 0 ? -0.05f : 0.f)};
        tube(M.acier, p, p + HAUT * (y - 0.06f), 0.024f, 5, c.alu);
        revolution(M.acier, p, HAUT, {{0, 0.07f}, {0.01f, 0.07f}}, 6, c.alu);   // vérin à platine
    }
    // jupe de scène (toile noire plissée) sur la face et les côtés
    auto jupe = [&](vec3 a, vec3 b, vec3 dehors) {
        const float L = length(b - a);
        const int n = loin ? 2 : std::max(4, (int)(L / 0.12f));
        std::vector<vec3> hautJ, basJ;
        for (int k = 0; k <= n; k++) {
            const vec3 p = a + (b - a) * ((float)k / n) + dehors * (loin ? 0.f : (k % 2 ? 0.035f : 0.f));
            hautJ.push_back(p + HAUT * (y - 0.06f)); basJ.push_back(p + HAUT * 0.03f);
        }
        loft(M.uni, {hautJ, basJ}, c.noir, false, a + dehors + HAUT * 2, a + dehors - HAUT);
    };
    jupe({-hx - 0.02f, 0, -hz}, {-hx - 0.02f, 0, hz}, -AX);
    jupe({-hx, 0, -hz - 0.02f}, {hx, 0, -hz - 0.02f}, -AZ);
    jupe({-hx, 0, hz + 0.02f}, {hx, 0, hz + 0.02f}, AZ);
    // escalier d'accès de 4 marches côté −z, à l'arrière
    if (!loin) for (int k = 0; k < 4; k++) {
        const float yy = (k + 1) * y / 5;
        poutre(M.uni, {hx - 1.4f, yy - 0.025f, -hz - 0.3f - (3 - k) * 0.28f}, {hx - 0.4f, yy - 0.025f, -hz - 0.3f - (3 - k) * 0.28f}, HAUT, 0.28f, 0.05f, 0.006f, c.planche);
        for (float s : {-1.f, 1.f}) poutre(M.uni, {hx - 0.9f + s * 0.5f, 0, -hz - 0.3f - (3 - k) * 0.28f}, {hx - 0.9f + s * 0.5f, yy - 0.05f, -hz - 0.3f - (3 - k) * 0.28f}, AX, 0.04f, 0.04f, 0.f, c.alu);
    }
    // portique : 4 tours et 4 poutres treillis de 29 cm, platines au sol ; fond de scène (toile tendue)
    const float px = hx - 0.15f, pz = hz - 0.15f, yt = SCENE_PORTIQUE - TRUSS / 2;
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f}) {
        treillis(M.acier, {sx * px, 0.f, sz * pz}, {sx * px, SCENE_PORTIQUE - TRUSS, sz * pz}, AX, c.alu, loin);
        poutre(M.acier, {sx * px, 0.f, sz * pz}, {sx * px, 0.012f, sz * pz}, AX, 0.6f, 0.6f, 0.01f, c.alu);   // platine 60 × 60 cm
    }
    for (float sx : {-1.f, 1.f}) treillis(M.acier, {sx * px, yt, -pz - TRUSS / 2}, {sx * px, yt, pz + TRUSS / 2}, HAUT, c.alu, loin);
    for (float sz : {-1.f, 1.f}) treillis(M.acier, {-px + TRUSS / 2, yt, sz * pz}, {px - TRUSS / 2, yt, sz * pz}, HAUT, c.alu, loin);
    {
        const int n = loin ? 1 : 12;
        std::vector<std::vector<vec3>> an;
        for (float yy : {y, (y + yt) / 2, yt - TRUSS / 2}) {
            std::vector<vec3> ring;
            for (int k = 0; k <= n; k++) { const float z = -pz + 0.2f + (2 * pz - 0.4f) * k / n; ring.push_back({px + 0.02f + (yy < yt - 1 && yy > y + 0.5f && k % 2 ? 0.03f : 0.f), yy, z}); }
            an.push_back(ring);
        }
        loft(M.uni, an, c.noir, false, {px - 1, y, 0}, {px - 1, yt, 0});
    }
    // 4 lyres (projecteurs asservis) pendues sous la poutre de façade : embase serrée sur la membrure, lyre en U, tête ronde
    // (lentille éclairée dessinée à part par plage.cpp, sous la tête)
    if (!loin) for (int i = 0; i < 4; i++) {
        const vec3 o{LYRE_X, LYRE_Y, -3.6f + 2.4f * i};
        poutre(M.uni, o + vec3{0, 0.09f, 0}, o + vec3{0, 0.16f, 0}, AX, 0.30f, 0.30f, 0.02f, c.noir);   // embase
        tube(M.acier, o + vec3{0, 0.16f, 0}, o + vec3{0, yt - TRUSS / 2 + 0.01f - LYRE_Y, 0}, 0.012f, 5, c.alu);   // crochet
        for (float s : {-1.f, 1.f}) poutre(M.uni, o + vec3{0, 0.09f, s * 0.135f}, o + vec3{0, -0.03f, s * 0.135f}, AX, 0.025f, 0.08f, 0.006f, c.noir);   // bras
        revolution(M.uni, o + vec3{0, -0.16f, 0}, HAUT, {{0.f, 0.085f}, {0.02f, 0.11f}, {0.20f, 0.11f}, {0.24f, 0.07f}, {0.25f, 0.f}}, 12, c.noir);   // tête
    }
    // table du DJ : plateau de 2,0 × 0,7 m à 1,0 m au-dessus de la scène, nappe noire, deux platines et une table de mixage
    const float xd = -0.3f, yd = y + 1.0f;
    poutre(M.uni, {xd, yd - 0.02f, -1.0f}, {xd, yd - 0.02f, 1.0f}, HAUT, 0.7f, 0.04f, 0.006f, c.noir);
    if (!loin) {
        std::vector<vec3> h, b;
        for (int k = 0; k <= 16; k++) { const float z = -1.0f + 2.0f * k / 16; h.push_back({xd - 0.355f, yd - 0.04f, z}); b.push_back({xd - 0.355f - (k % 2 ? 0.02f : 0.f), y + 0.02f, z}); }
        loft(M.uni, {h, b}, c.noir, false, {xd + 1, yd, 0}, {xd + 1, y, 0});
        for (float s : {-1.f, 1.f}) for (float t : {-1.f, 1.f}) tube(M.acier, {xd + s * 0.3f, y, t * 0.9f}, {xd + s * 0.3f, yd - 0.04f, t * 0.9f}, 0.016f, 5, c.alu);
        for (float z : {-0.55f, 0.55f}) {   // platines (socle chanfreiné, plateau tournant)
            poutre(M.uni, {xd, yd, z}, {xd, yd + 0.10f, z}, AX, 0.36f, 0.45f, 0.012f, c.hp);
            revolution(M.uni, {xd + 0.02f, yd + 0.10f, z - 0.03f}, HAUT, {{0, 0.15f}, {0.012f, 0.15f}, {0.014f, 0.004f}, {0.03f, 0.004f}}, 16, c.alu);
        }
        poutre(M.uni, {xd, yd, 0}, {xd, yd + 0.09f, 0}, AX, 0.32f, 0.36f, 0.015f, c.hp);
    }
    return M;
}

// enceinte de façade : 2 caissons de basses (0,70 × 0,70 × 0,60 m) surmontés d'une enceinte à 2 haut-parleurs de 38 cm et
// pavillon ; face vers −x ; pied en (0, 0, 0)
Modele enceinte(const Couleurs& c, bool loin) {
    Modele M;
    for (int k = 0; k < 2; k++) {
        const float y0 = k * 0.61f;
        poutre(M.uni, {0, y0, 0}, {0, y0 + 0.60f, 0}, AX, 0.70f, 0.70f, 0.02f, c.hp);
        if (loin) continue;
        quadPlat(M.uni, {-0.352f, y0 + 0.05f, -0.30f}, {-0.352f, y0 + 0.05f, 0.30f}, {-0.352f, y0 + 0.55f, 0.30f}, {-0.352f, y0 + 0.55f, -0.30f}, c.noir, -AX);   // grille
        for (float z : {-0.27f, 0.27f}) poutre(M.uni, {-0.36f, y0 + 0.08f, z}, {-0.36f, y0 + 0.52f, z}, AX, 0.012f, 0.03f, 0.f, c.alu);                         // poignées
    }
    const float y0 = 1.23f;
    poutre(M.uni, {0, y0, 0}, {0, y0 + 1.0f, 0}, AX, 0.50f, 0.55f, 0.02f, c.hp);
    if (!loin) {
        for (float y : {y0 + 0.24f, y0 + 0.66f})   // haut-parleurs : saladier, suspension, membrane conique, cache-noyau
            revolution(M.uni, {-0.252f, y, 0}, -AX, {{0, 0.19f}, {0.006f, 0.185f}, {0.012f, 0.17f}, {0.006f, 0.16f}, {-0.06f, 0.05f}, {-0.05f, 0.03f}, {-0.04f, 0.f}}, 16, c.noir);
        const float y = y0 + 0.9f;   // pavillon (évasé rectangulaire)
        const vec3 a0{-0.255f, y - 0.08f, -0.2f}, a1{-0.255f, y - 0.08f, 0.2f}, a2{-0.255f, y + 0.08f, 0.2f}, a3{-0.255f, y + 0.08f, -0.2f};
        const vec3 b0{-0.10f, y - 0.02f, -0.03f}, b1{-0.10f, y - 0.02f, 0.03f}, b2{-0.10f, y + 0.02f, 0.03f}, b3{-0.10f, y + 0.02f, -0.03f};
        quadPlat(M.uni, a0, a1, b1, b0, c.noir, HAUT); quadPlat(M.uni, a1, a2, b2, b1, c.noir, -AZ);
        quadPlat(M.uni, a2, a3, b3, b2, c.noir, -HAUT); quadPlat(M.uni, a3, a0, b0, b3, c.noir, AZ);
    }
    return M;
}

Modele lentilleLyre() {
    Modele M;
    revolution(M.uni, {0, 0, 0}, -HAUT, {{0.f, 0.078f}, {0.008f, 0.07f}, {0.016f, 0.045f}, {0.02f, 0.f}}, 12, 0);
    return M;
}

// ---------------------------------------------------------------- candélabre de promenade (lanterne de style)
Modele candelabre(const Couleurs& c, bool loin) {
    Modele M;
    const int n = loin ? 8 : 16;
    // socle mouluré (tore, gorge, doucine)
    if (loin) revolution(M.uni, {0, 0, 0}, HAUT, {{0.f, 0.17f}, {0.08f, 0.15f}, {0.60f, 0.11f}, {0.70f, 0.085f}}, n, c.fonte);
    else revolution(M.uni, {0, 0, 0}, HAUT, {{0.f, 0.17f}, {0.06f, 0.17f}, {0.08f, 0.15f}, {0.12f, 0.155f}, {0.16f, 0.12f}, {0.55f, 0.105f}, {0.60f, 0.12f}, {0.66f, 0.12f}, {0.70f, 0.085f}}, n, c.fonte);
    // fût cannelé (16 cannelures) de Ø 160 mm à Ø 100 mm, bague à 1,6 m et chapiteau en tête
    {
        std::vector<std::vector<vec3>> an;
        const int nf = loin ? 8 : 32;
        for (float yy : {0.70f, 2.0f, 3.5f, CANDELABRE_FUT}) {
            const float r = 0.08f - 0.03f * (yy - 0.7f) / (CANDELABRE_FUT - 0.7f);
            std::vector<vec3> ring;
            for (int k = 0; k < nf; k++) { const float a = 2 * PI * k / nf, rr = loin ? r : r - (k % 2 ? 0.006f : 0.f); ring.push_back({std::cos(a) * rr, yy, std::sin(a) * rr}); }
            an.push_back(ring);
        }
        loft(M.uni, an, c.fonte, true, {0, 0.7f, 0}, {0, CANDELABRE_FUT, 0});
    }
    if (!loin) revolution(M.uni, {0, 1.55f, 0}, HAUT, {{0, 0.085f}, {0.03f, 0.095f}, {0.07f, 0.095f}, {0.10f, 0.08f}}, n, c.fonte);
    revolution(M.uni, {0, CANDELABRE_FUT - 0.02f, 0}, HAUT, {{0, 0.055f}, {0.05f, 0.075f}, {0.10f, 0.08f}, {0.13f, 0.06f}, {0.25f, 0.045f}, {0.30f, 0.f}}, n, c.fonte);
    // crosse : bras cintré vers −x jusqu'au crochet de la lanterne, volute de renfort sous le bras
    const float xL = -CANDELABRE_CROSSE, yH = LANTERNE_Y + LANTERNE_H / 2 + 0.25f;
    std::vector<vec3> bras;
    for (int k = 0; k <= (loin ? 3 : 8); k++) {
        const float t = (float)k / (loin ? 3 : 8), a = t * PI / 2;
        bras.push_back({xL * (1 - std::cos(a)) * 0.6f + xL * 0.4f * t, CANDELABRE_FUT + 0.15f + (yH - CANDELABRE_FUT - 0.15f) * std::sin(a), 0});
    }
    tubeCourbe(M.uni, bras, 0.022f, loin ? 5 : 8, c.fonte);
    if (!loin) {
        std::vector<vec3> volute;
        for (int k = 0; k <= 14; k++) {   // spirale d'Archimède dans le plan (x, y), du fût vers le bras
            const float t = (float)k / 14, a = t * 1.7f * PI, r = 0.05f + 0.13f * t;
            volute.push_back({-0.20f + r * std::cos(a + 0.4f) * -1.f, CANDELABRE_FUT + 0.35f + r * std::sin(a + 0.4f), 0});
        }
        tubeCourbe(M.uni, volute, 0.009f, 6, c.fonte);
        tore(M.uni, {xL, yH - 0.06f, 0}, AZ, 0.035f, 0.007f, 8, 4, c.fonte);   // piton de suspension
    }
    // lanterne à 4 faces : culot, montants, verres éclairés (part neon), chapeau pyramidal débordant, fleuron
    const float a = LANTERNE_COTE / 2, yb = LANTERNE_Y - 0.30f, yh = LANTERNE_Y + 0.30f;
    const vec3 o{xL, 0, 0};
    revolution(M.uni, o + vec3{0, yb - 0.12f, 0}, HAUT, {{0.f, 0.f}, {0.04f, 0.03f}, {0.10f, 0.12f}, {0.12f, 0.15f}}, 4, c.fonte);   // culot (pyramide renversée)
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})
        poutre(M.uni, o + vec3{sx * a * 0.85f, yb - 0.01f, sz * a * 0.85f}, o + vec3{sx * a, yh + 0.01f, sz * a}, AX, 0.025f, 0.025f, 0.003f, c.fonte);
    for (int f = 0; f < 4; f++) {
        const float t0 = PI / 4 + f * PI / 2, t1 = t0 + PI / 2, rb = a * 0.85f * 1.4142f, rh = a * 1.4142f;
        const vec3 p0 = o + vec3{std::cos(t0) * rb, yb, std::sin(t0) * rb}, p1 = o + vec3{std::cos(t1) * rb, yb, std::sin(t1) * rb};
        const vec3 q1 = o + vec3{std::cos(t1) * rh, yh, std::sin(t1) * rh}, q0 = o + vec3{std::cos(t0) * rh, yh, std::sin(t0) * rh};
        const vec3 dehors{std::cos((t0 + t1) / 2), 0, std::sin((t0 + t1) / 2)};
        quadPlat(M.neon, p0, p1, q1, q0, 0, dehors);
    }
    revolution(M.uni, o + vec3{0, yh, 0}, HAUT, {{0.f, a * 1.45f}, {0.02f, a * 1.55f}, {0.04f, a * 1.5f}, {0.18f, 0.05f}, {0.22f, 0.03f}, {0.26f, 0.f}}, 4, c.fonte);   // chapeau
    return M;
}

// ---------------------------------------------------------------- couronne du palmier : gaines et noix de coco
Modele gainesPalmier(const Couleurs& c, uint32_t graine, bool loin) {
    Modele M;
    Hasard H{graine * 2654435761u + 17};
    // manchon de gaines fibreuses sous la couronne (12 côtes bosselées)
    {
        const int n = loin ? 6 : 12;
        std::vector<std::vector<vec3>> an;
        for (auto [y, r] : {std::pair{-0.75f, 0.17f}, std::pair{-0.45f, 0.24f}, std::pair{-0.20f, 0.22f}, std::pair{0.05f, 0.10f}}) {
            std::vector<vec3> ring;
            for (int k = 0; k < n; k++) { const float a = 2 * PI * k / n, rr = r * (1 + (k % 2 ? 0.08f : -0.04f) + H.sym(0.04f)); ring.push_back({std::cos(a) * rr, y, std::sin(a) * rr}); }
            an.push_back(ring);
        }
        loft(M.uni, an, c.gaine, true, {0, -1, 0}, {0, 0.2f, 0});
    }
    if (!loin) for (int k = 0; k < 10; k++) {   // pétioles : base large et plate, effilée vers la palme (relevés comme les palmes)
        const float a = 2 * PI * k / 10 + H.sym(0.2f), el = 0.15f + H() * 0.6f;
        const vec3 d = normalize(vec3{std::cos(a) * std::cos(el), std::sin(el), std::sin(a) * std::cos(el)});
        const vec3 p0 = vec3{std::cos(a) * 0.14f, -0.30f + H.sym(0.08f), std::sin(a) * 0.14f}, p1 = p0 + d * GAINE_PETIOLE;
        const vec3 lat = normalize(cross(d, HAUT));
        const vec3 nn = normalize(cross(lat, d));
        std::vector<std::vector<vec3>> an;
        for (auto [u, w, e] : {std::tuple{0.f, 0.07f, 0.025f}, std::tuple{0.35f, 0.045f, 0.02f}, std::tuple{1.f, 0.02f, 0.012f}}) {
            const vec3 p = p0 + (p1 - p0) * u;
            an.push_back({p + lat * w, p + nn * e, p - lat * w, p - nn * e * 0.6f});
        }
        loft(M.uni, an, c.bois, true, p0, p1);
    }
    // noix de coco en grappe (5 de près, 3 de loin) : coque ovoïde à 3 arêtes, calice, pédoncule
    const int nn = loin ? 2 : 5;
    for (int k = 0; k < nn; k++) {
        const float a = 2 * PI * k / nn + H.sym(0.3f);
        const vec3 ctr{std::cos(a) * 0.24f, -0.55f + H.sym(0.06f), std::sin(a) * 0.24f};
        const vec3 ax = normalize(vec3{std::cos(a) * 0.5f, -1.f, std::sin(a) * 0.5f});
        vec3 e1, e2; base(ax, e1, e2);
        const int ns = loin ? 5 : 8;
        std::vector<std::vector<vec3>> an;
        const float L = NOIX_LONG, R = NOIX_DIAM / 2;
        const std::vector<std::pair<float, float>> prof = loin ? std::vector<std::pair<float, float>>{{-0.5f, 0.15f}, {-0.2f, 0.95f}, {0.25f, 0.9f}, {0.5f, 0.05f}}
                                                              : std::vector<std::pair<float, float>>{{-0.5f, 0.15f}, {-0.36f, 0.72f}, {-0.1f, 1.f}, {0.22f, 0.9f}, {0.42f, 0.45f}, {0.5f, 0.05f}};
        for (auto [u, r] : prof) {
            std::vector<vec3> ring;
            for (int j = 0; j < ns; j++) { const float t = 2 * PI * j / ns, rr = R * r * (1 + 0.07f * std::cos(3 * t)); ring.push_back(ctr + ax * (u * L) + e1 * (std::cos(t) * rr) + e2 * (std::sin(t) * rr)); }
            an.push_back(ring);
        }
        loft(M.uni, an, c.noix, true, ctr - ax * L, ctr + ax * L);
        if (!loin) {
            revolution(M.uni, ctr - ax * (L * 0.48f), -ax, {{0, 0.035f}, {0.015f, 0.045f}, {0.03f, 0.f}}, 6, c.gaine);   // calice
            tube(M.uni, ctr - ax * (L * 0.5f), vec3{std::cos(a) * 0.12f, -0.38f, std::sin(a) * 0.12f}, 0.008f, 4, c.gaine);
        }
    }
    return M;
}

// ---------------------------------------------------------------- guirlande, feu de camp, rondins
Modele matGuirlande(const Couleurs& c) {
    Modele M;
    bois(M.uni, {0, -0.5f, 0}, {0, MAT_GUIRLANDE, 0}, 0.07f, 0.05f, 8, 4, 913, c.bois, c.bois);
    tore(M.acier, {0.06f, MAT_GUIRLANDE - 0.12f, 0}, AZ, 0.03f, 0.006f, 8, 4, c.alu);   // piton
    return M;
}

Modele ampoule() {
    Modele M;
    revolution(M.uni, {0, 0, 0}, -HAUT, {{0.f, 0.015f}, {0.03f, 0.015f}}, 6, 0);   // douille (couleur posée par plage.cpp)
    revolution(M.neon, {0, -0.03f, 0}, -HAUT, {{0.f, 0.012f}, {0.018f, AMPOULE_D / 2}, {0.040f, AMPOULE_D / 2 * 0.9f}, {AMPOULE_H - 0.03f, 0.f}}, 8, 0);
    return M;
}

Modele buche(const Couleurs& c, uint32_t graine) {
    Modele M;
    bois(M.uni, {-BUCHE_L / 2, 0, 0}, {BUCHE_L / 2, 0, 0}, BUCHE_D / 2, BUCHE_D / 2 * 0.92f, 8, 3, graine, c.ecorce, c.bois);
    Hasard H{graine + 5};
    const float x = H.sym(0.2f);   // chicot de branche
    bois(M.uni, {x, 0.03f, 0}, {x + 0.04f, 0.06f, 0.10f}, 0.022f, 0.016f, 5, 1, graine + 9, c.ecorce, c.bois);
    return M;
}

Modele rondin(const Couleurs& c, uint32_t graine) {
    Modele M;
    bois(M.uni, {0, 0, 0}, {0, 1, 0}, 0.5f, 0.44f, 8, 4, graine, c.bois, c.boisSombre);
    return M;
}

} // namespace corniche
} // namespace wfs
