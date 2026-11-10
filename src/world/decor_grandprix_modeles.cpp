// WaForSpeed — WFS-373 : objets du Grand Prix modélisés (voir decor_grandprix_modeles.h).
#include "decor_grandprix_modeles.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include "decor_campagne_modeles.h"   // poutre (pièce chanfreinée), profileI (WFS-372)

namespace wfs {
namespace grandprix {

using campagne::poutre;
using campagne::profileI;
using marina::eventail;
using marina::ligne;
using marina::loft;

namespace {

const vec3 HAUT{0, 1, 0}, AX{1, 0, 0}, AZ{0, 0, 1};

void triPlat(Maille& m, vec3 a, vec3 b, vec3 c, int couleur, vec3 dehors) {
    vec3 nn = cross(b - a, c - a);
    if (dot(nn, nn) < 1e-14f) return;
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const vec3& p : {a, b, c}) m.v.push_back({p, nn, couleur});
    if (inverse) m.i.insert(m.i.end(), {k0, k0 + 2, k0 + 1}); else m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2});
}

// polygone (x, y) parcouru dans le sens trigonométrique, extrudé le long de z de z0 à z1 : faces planes ; couleur par arête
// (cols, facultatif) ; bouts fermés seulement si le polygone est convexe (éventail)
void extrudeZ(Maille& m, const std::vector<vec2>& p, float z0, float z1, int col, bool bouts, const std::vector<int>* cols = nullptr) {
    const size_t n = p.size();
    for (size_t k = 0; k < n; k++) {
        const size_t j = (k + 1) % n;
        const float ex = p[j].x - p[k].x, ey = p[j].y - p[k].y;
        quadPlat(m, {p[k].x, p[k].y, z0}, {p[j].x, p[j].y, z0}, {p[j].x, p[j].y, z1}, {p[k].x, p[k].y, z1}, cols ? (*cols)[k] : col, {ey, -ex, 0});
    }
    if (bouts) {
        std::vector<vec3> A, B;
        for (const vec2& q : p) { A.push_back({q.x, q.y, z0}); B.push_back({q.x, q.y, z1}); }
        eventail(m, A, col, -AZ); eventail(m, B, col, AZ);
    }
}

// surface en grille ni × nj (points donnés par P(i, j)), normales lissées par différences ; une seule couleur
void grille(Maille& m, int ni, int nj, const std::function<vec3(int, int)>& P, int couleur) {
    const uint32_t k0 = (uint32_t)m.v.size();
    for (int i = 0; i < ni; i++) for (int j = 0; j < nj; j++) {
        const vec3 du = P(std::min(i + 1, ni - 1), j) - P(std::max(i - 1, 0), j), dv = P(i, std::min(j + 1, nj - 1)) - P(i, std::max(j - 1, 0));
        vec3 nn = normalize(cross(du, dv));
        if (length(nn) < 0.5f) nn = HAUT;
        m.v.push_back({P(i, j), nn, couleur});
    }
    auto id = [&](int i, int j) { return k0 + (uint32_t)(i * nj + j); };
    for (int i = 0; i + 1 < ni; i++) for (int j = 0; j + 1 < nj; j++) {
        const vec3 A = m.v[id(i, j)].p, B = m.v[id(i + 1, j)].p, C = m.v[id(i + 1, j + 1)].p, D = m.v[id(i, j + 1)].p;
        if (dot(cross(B - A, C - A), cross(B - A, C - A)) > 1e-14f) m.i.insert(m.i.end(), {id(i, j), id(i + 1, j), id(i + 1, j + 1)});
        if (dot(cross(C - A, D - A), cross(C - A, D - A)) > 1e-14f) m.i.insert(m.i.end(), {id(i, j), id(i + 1, j + 1), id(i, j + 1)});
    }
}

// toile tendue sur sa hampe : de o, le long de u (longueur L, flottant), vers le bas v (hauteur H) ; ondulation d'amplitude amp
// selon n0, nulle à la hampe et croissante vers le bout
void toile(Maille& m, vec3 o, vec3 u, vec3 v, vec3 n0, float L, float H, int nu, int nv, float amp, float phase, int couleur) {
    grille(m, nu + 1, nv + 1, [&](int i, int j) {
        const float s = (float)i / nu, t = (float)j / nv;
        return o + u * (L * s) + v * (H * t) + n0 * (amp * s * std::sin(s * 2 * PI * 1.25f + phase + t * 0.6f));
    }, couleur);
}

// treillis carré de section a × a, de y0 à y1 (pylône), centré en (x, z) : membrures, traverses et diagonales tous les pas
void pylone(Maille& m, float x, float z, float y0, float y1, float a, float pas, float rM, float rB, int couleur, bool simple) {
    const float h = a / 2;
    const vec2 C[4] = {{-h, -h}, {h, -h}, {h, h}, {-h, h}};
    for (const vec2& c : C) tube(m, {x + c.x, y0, z + c.y}, {x + c.x, y1, z + c.y}, rM, simple ? 4 : 6, couleur);
    const int n = std::max(1, (int)std::lround((y1 - y0) / pas));
    for (int k = 0; k <= n; k++) {
        const float y = y0 + (y1 - y0) * k / n, y2 = y0 + (y1 - y0) * (k + 1) / n;
        for (int f = 0; f < 4; f++) {
            const vec2 a0 = C[f], a1 = C[(f + 1) % 4];
            tube(m, {x + a0.x, y, z + a0.y}, {x + a1.x, y, z + a1.y}, rB, 4, couleur);
            if (k < n && !simple) tube(m, {x + (k % 2 ? a0.x : a1.x), y, z + (k % 2 ? a0.y : a1.y)}, {x + (k % 2 ? a1.x : a0.x), y2, z + (k % 2 ? a1.y : a0.y)}, rB, 4, couleur);
        }
    }
}

// poutre en treillis horizontale de section a × a le long de x, de x0 à x1, centre de la section en (y, z)
void poutreTreillis(Maille& m, float x0, float x1, float y, float z, float a, float pas, float rM, float rB, int couleur) {
    const float h = a / 2;
    const vec2 C[4] = {{-h, -h}, {h, -h}, {h, h}, {-h, h}};   // (y, z)
    for (const vec2& c : C) tube(m, {x0, y + c.x, z + c.y}, {x1, y + c.x, z + c.y}, rM, 6, couleur);
    const int n = std::max(1, (int)std::lround((x1 - x0) / pas));
    for (int k = 0; k <= n; k++) {
        const float x = x0 + (x1 - x0) * k / n, x2 = x0 + (x1 - x0) * (k + 1) / n;
        for (int f = 0; f < 4; f++) {
            const vec2 a0 = C[f], a1 = C[(f + 1) % 4];
            tube(m, {x, y + a0.x, z + a0.y}, {x, y + a1.x, z + a1.y}, rB, 4, couleur);
            if (k < n) tube(m, {x, y + (k % 2 ? a0.x : a1.x), z + (k % 2 ? a0.y : a1.y)}, {x2, y + (k % 2 ? a1.x : a0.x), z + (k % 2 ? a1.y : a0.y)}, rB, 4, couleur);
        }
    }
}

// platine de pied (acier, 4 tiges filetées) sous un poteau en (x, z)
void platine(Maille& m, float x, float z, float cote, int couleur, bool tiges) {
    poutre(m, {x, 0.008f, z - cote / 2}, {x, 0.008f, z + cote / 2}, HAUT, cote, 0.016f, 0.003f, couleur);
    if (tiges) for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})
        tube(m, {x + sx * cote * 0.35f, 0.016f, z + sz * cote * 0.35f}, {x + sx * cote * 0.35f, 0.05f, z + sz * cote * 0.35f}, 0.01f, 4, couleur);
}

// roue (pneu et jante) d'axe ax (unitaire, vers l'extérieur du véhicule), centre o, Ø d, largeur w
void roue(Maille& m, vec3 o, vec3 ax, float d, float w, float jante, const Couleurs& c, bool loin) {
    const float R = d / 2, rj = jante / 2, h = w / 2;
    const int n = loin ? 8 : 14;
    if (loin) revolution(m, o, ax, {{-h, rj}, {-h + 0.02f, R - 0.03f}, {h - 0.02f, R - 0.03f}, {h, rj}}, n, c.pneu);
    else revolution(m, o, ax, {{-h, rj + 0.01f}, {-h + 0.01f, R - 0.07f}, {-h + 0.045f, R - 0.012f}, {0, R}, {h - 0.045f, R - 0.012f}, {h - 0.01f, R - 0.07f}, {h, rj + 0.01f}}, n, c.pneu);
    // jante voilée (disque embouti, trous d'aération suggérés par le creux) et moyeu
    revolution(m, o, ax, {{h - 0.012f, rj + 0.012f}, {h - 0.03f, rj * 0.8f}, {h - 0.07f, rj * 0.55f}, {h - 0.06f, rj * 0.3f}, {h - 0.02f, rj * 0.22f}, {h + 0.01f, 0.f}}, n, c.jante);
}

// cercle de n points dans le plan (e1, e2)
std::vector<vec3> cercle(vec3 o, vec3 e1, vec3 e2, float r1, float r2, int n, float phase = 0) {
    std::vector<vec3> r;
    for (int k = 0; k < n; k++) { const float a = phase + 2 * PI * k / n; r.push_back(o + e1 * (std::cos(a) * r1) + e2 * (std::sin(a) * r2)); }
    return r;
}

// garde-corps en tubes le long de a → b : poteaux tous les pas, main courante à h, lisse à h / 2
void gardeCorps(Maille& m, vec3 a, vec3 b, float h, float pas, int couleur, bool lisse = true) {
    const float L = length(b - a);
    const int n = std::max(1, (int)std::ceil(L / pas));
    for (int k = 0; k <= n; k++) { const vec3 p = a + (b - a) * ((float)k / n); tube(m, p, p + HAUT * h, 0.024f, 6, couleur); }
    tube(m, a + HAUT * h, b + HAUT * h, 0.024f, 6, couleur);
    if (lisse) tube(m, a + HAUT * (h * 0.5f), b + HAUT * (h * 0.5f), 0.017f, 5, couleur);
}

ObjetBanc g_banc[NB_OBJETS_BANC];

} // namespace

// ---------------------------------------------------------------- outils publics
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

void revolution(Maille& m, vec3 o, vec3 ax, const std::vector<std::pair<float, float>>& prof, int n, int couleur, float a0, float a1, vec3 ref, bool bouts) {
    ax = normalize(ax);
    vec3 e1 = ref - ax * dot(ref, ax);
    if (length(e1) < 1e-4f) e1 = std::fabs(ax.y) < 0.9f ? cross(ax, HAUT) : cross(ax, AX);
    e1 = normalize(e1);
    const vec3 e2 = cross(ax, e1);
    const bool complet = a1 - a0 > 2 * PI - 1e-3f;
    std::vector<std::vector<vec3>> an;
    for (auto [x, r] : prof) {
        std::vector<vec3> ring;
        const int np = complet ? n : n + 1;
        for (int k = 0; k < np; k++) {
            const float a = a0 + (a1 - a0) * k / n;
            ring.push_back(o + ax * x + e1 * (std::cos(a) * std::max(r, 1e-4f)) + e2 * (std::sin(a) * std::max(r, 1e-4f)));
        }
        an.push_back(ring);
    }
    loft(m, an, couleur, complet, o + ax * prof.front().first, o + ax * prof.back().first);
    if (bouts && complet && prof.front().second > 1e-3f) eventail(m, an.front(), couleur, -ax);
    if (bouts && complet && prof.back().second > 1e-3f) eventail(m, an.back(), couleur, ax);
}

void tube(Maille& m, vec3 a, vec3 b, float r, int n, int couleur) {
    const float L = length(b - a);
    if (L < 1e-4f) return;
    revolution(m, a, b - a, {{0, r}, {L, r}}, n, couleur);
}

void bacAcier(Maille& m, vec3 o, vec3 u, vec3 v, vec3 dehors, float largeur, float longueur, float p, float h, int couleur) {
    u = normalize(u); v = normalize(v); dehors = normalize(dehors);
    // nervure trapézoïdale : plat bas 45 %, flanc 10 %, plat haut 35 %, flanc 10 % du pas
    std::vector<vec2> prof;   // (abscisse le long de u, hauteur)
    for (float s = 0; s < largeur - 1e-4f; s += p) {
        const float q[4] = {0.f, 0.45f, 0.55f, 0.9f}, y[4] = {0.f, 0.f, h, h};
        for (int k = 0; k < 4; k++) if (s + q[k] * p < largeur) prof.push_back({s + q[k] * p, y[k]});
    }
    prof.push_back({largeur, 0.f});
    for (size_t k = 0; k + 1 < prof.size(); k++) {
        const vec3 a = o + u * prof[k].x + dehors * prof[k].y, b = o + u * prof[k + 1].x + dehors * prof[k + 1].y;
        const vec3 t = normalize(b - a), nn = normalize(cross(v, t));
        quadPlat(m, a, b, b + v * longueur, a + v * longueur, couleur, dot(nn, dehors) >= 0 ? nn : -nn);
    }
}

void poser(Maille& dst, const Maille& m, vec3 o, vec3 X, vec3 Z, int var) {
    const uint32_t k0 = (uint32_t)dst.v.size();
    dst.v.reserve(dst.v.size() + m.v.size());
    for (const marina::Sommet& v : m.v)
        dst.v.push_back({o + X * v.p.x + HAUT * v.p.y + Z * v.p.z, X * v.n.x + HAUT * v.n.y + Z * v.n.z, v.couleur == COUL_VAR ? var : v.couleur});
    for (uint32_t i : m.i) dst.i.push_back(k0 + i);
}

void colorier(Maille& m, int de, int vers) { for (marina::Sommet& v : m.v) if (v.couleur == de) v.couleur = vers; }

ObjetBanc objetBanc(int sorte) { return sorte >= 0 && sorte < NB_OBJETS_BANC ? g_banc[sorte] : ObjetBanc{}; }
void noterObjet(int sorte, int64_t seg, int side, float lat) { if (sorte >= 0 && sorte < NB_OBJETS_BANC && g_banc[sorte].seg < 0) g_banc[sorte] = {seg, side, lat}; }
void oublierObjets() { for (ObjetBanc& o : g_banc) o = {}; }

// ---------------------------------------------------------------- mur de pneus (ANO-220 : pneus empilés et sanglés)
Maille murPneus(const Couleurs& c, bool loin, int decalage) {
    Maille m;
    const float R = PNEU_DIAM / 2, rj = PNEU_JANTE / 2, w = PNEU_LARGEUR, pas = MUR_PNEUS_L / PILES_MUR, H = PNEUS_PILE * w;
    const float xc = R + 0.004f;   // face des piles (bande de roulement) en x ≈ 0
    for (int i = 0; i < PILES_MUR; i++) {
        const vec3 o{xc, 0, pas * (i + 0.5f)};
        const int haut = (i + decalage) % 2 ? c.pneuRouge : c.pneuBlanc;   // pneu du haut peint (jeu web : rouge / blanc)
        const vec3 ref{std::cos(i * 0.37f), 0, std::sin(i * 0.37f)};       // les piles ne sont pas toutes tournées pareil
        // pneus du bas : flancs bombés, bande de roulement, creux entre deux pneus (pas de fond : posés au sol, cachés)
        std::vector<std::pair<float, float>> bas;
        for (int k = 0; k < PNEUS_PILE - 1; k++) {
            const float y0 = k * w;
            if (loin) { bas.push_back({y0 + 0.004f, R - 0.04f}); bas.push_back({y0 + w / 2, R}); continue; }
            bas.push_back({y0 + 0.004f, R - 0.045f}); bas.push_back({y0 + 0.035f, R - 0.008f}); bas.push_back({y0 + w / 2, R}); bas.push_back({y0 + w - 0.035f, R - 0.008f});
        }
        bas.push_back({(PNEUS_PILE - 1) * w + 0.004f, R - (loin ? 0.04f : 0.045f)});
        revolution(m, o, HAUT, bas, loin ? 6 : 8, c.pneu, 0, 2 * PI, ref, false);
        // pneu du haut (peint), en entier : flanc supérieur jusqu'au talon et creux de la jante (fond du creux fermé)
        const float y0 = (PNEUS_PILE - 1) * w;
        if (loin) revolution(m, o, HAUT, {{y0 + 0.004f, R - 0.04f}, {y0 + w / 2, R}, {y0 + w, R - 0.07f}, {y0 + w, rj}}, 6, haut, 0, 2 * PI, ref, false);
        else revolution(m, o, HAUT, {{y0 + 0.004f, R - 0.045f}, {y0 + 0.035f, R - 0.008f}, {y0 + w / 2, R}, {y0 + w - 0.035f, R - 0.008f}, {y0 + w - 0.008f, R - 0.04f},
                                     {y0 + w, R - 0.075f}, {y0 + w, rj + 0.035f}, {y0 + w - 0.015f, rj}, {y0 + 0.02f, rj}}, 8, haut, 0, 2 * PI, ref);
    }
    // sangles de polyester jaunes (50 mm, 2 rangs) qui ceinturent la rangée : devant et derrière les piles, retours aux bouts
    for (float y : {0.25f * H, 0.68f * H}) {
        for (float x : {-0.003f, xc + R + 0.003f}) poutre(m, {x, y, pas * 0.5f - 0.05f}, {x, y, MUR_PNEUS_L - pas * 0.5f + 0.05f}, HAUT, 0.004f, 0.05f, 0.f, c.sangle);
        if (!loin) for (float z : {pas * 0.5f - R + 0.02f, MUR_PNEUS_L - pas * 0.5f + R - 0.02f}) poutre(m, {0.f, y, z}, {xc + R, y, z}, HAUT, 0.004f, 0.05f, 0.f, c.sangle);
    }
    return m;
}

// ---------------------------------------------------------------- panneau publicitaire
Maille panneauPub(const Couleurs& c, bool loin) {
    Maille m;
    const float L = PUB_L / 2, B = PUB_BACHE_L / 2, y0 = PUB_BAS, y1 = PUB_BAS + PUB_BACHE_H, e = 0.06f;
    // bâche tendue : face route et dos (lisible des deux côtés), chants entre les deux faces
    quadPlat(m, {0, y0, -B}, {0, y1, -B}, {0, y1, B}, {0, y0, B}, COUL_VAR, -AX);
    quadPlat(m, {e, y0, -B}, {e, y1, -B}, {e, y1, B}, {e, y0, B}, COUL_VAR, AX);
    for (float z : {-B, B}) quadPlat(m, {0, y0, z}, {e, y0, z}, {e, y1, z}, {0, y1, z}, c.aluSombre, {0, 0, z});
    // cadre : lisses haute et basse en tube d'aluminium 70 × 40 mm, montants tous les 2,3 m
    for (float y : {y0 - 0.02f, y1 + 0.02f}) poutre(m, {e / 2, y, -L}, {e / 2, y, L}, HAUT, 0.07f, 0.04f, 0.004f, c.alu);
    for (int k = 0; k <= 5; k++) {
        const float z = -L + 0.03f + k * (PUB_L - 0.06f) / 5;
        poutre(m, {e + 0.03f, 0.f, z}, {e + 0.03f, y1 + 0.05f, z}, AX, 0.05f, 0.05f, 0.004f, c.alu);
        if (loin) continue;
        // jambe de force et platines (lestées au sol)
        tube(m, {e + 0.05f, y1 - 0.25f, z}, {0.45f, 0.02f, z}, 0.02f, 6, c.alu);
        poutre(m, {e - 0.07f, 0.006f, z}, {e + 0.13f, 0.006f, z}, HAUT, 0.1f, 0.012f, 0.003f, c.acier);
        poutre(m, {0.37f, 0.006f, z}, {0.5f, 0.006f, z}, HAUT, 0.1f, 0.012f, 0.003f, c.acier);
    }
    return m;
}

// ---------------------------------------------------------------- siège coque (EN 13200-4) et rangée allégée
Maille siege(const Couleurs& c) {
    Maille m;
    // profil de la coque (x vers le dossier, y) : lèvre avant, assise creusée, cambrure, dossier ; 3 sections en travers
    const vec2 P[7] = {{-0.21f, 0.425f}, {-0.18f, 0.452f}, {-0.02f, 0.44f}, {0.13f, 0.445f}, {0.19f, 0.52f}, {0.22f, 0.70f}, {0.235f, SIEGE_ASSISE + SIEGE_DOSSIER}};
    const float Z[3] = {-SIEGE_LARGEUR / 2, 0, SIEGE_LARGEUR / 2};
    grille(m, 7, 3, [&](int i, int j) {
        const float creux = j == 1 ? 1.f : 0.f;   // assise creusée au milieu, dossier enveloppant
        return vec3{P[i].x + (i >= 4 ? 0.012f * creux : 0.f), P[i].y - (i >= 1 && i <= 3 ? 0.012f * creux : 0.f), Z[j]};
    }, COUL_VAR);
    // chant (épaisseur 2 cm) de la lèvre avant et du haut du dossier
    quadPlat(m, {P[0].x, P[0].y, Z[0]}, {P[0].x, P[0].y - 0.02f, Z[0]}, {P[0].x, P[0].y - 0.02f, Z[2]}, {P[0].x, P[0].y, Z[2]}, COUL_VAR, -AX);
    quadPlat(m, {P[6].x, P[6].y, Z[0]}, {P[6].x + 0.02f, P[6].y, Z[0]}, {P[6].x + 0.02f, P[6].y, Z[2]}, {P[6].x, P[6].y, Z[2]}, COUL_VAR, HAUT);
    // pied central (fixé sur le gradin) et support sous l'assise
    poutre(m, {0.02f, 0.f, 0.f}, {0.02f, 0.40f, 0.f}, AX, 0.05f, 0.08f, 0.f, c.acier);
    poutre(m, {-0.12f, 0.41f, 0.f}, {0.14f, 0.41f, 0.f}, HAUT, 0.06f, 0.03f, 0.f, c.acier);
    return m;
}

Maille rangeeSieges(const Couleurs&, float l) {
    Maille m;
    extrudeZ(m, {{-0.2f, 0.40f}, {0.12f, 0.43f}, {0.20f, 0.50f}, {0.24f, 0.84f}, {0.21f, 0.84f}, {0.17f, 0.52f}, {-0.2f, 0.45f}}, -l / 2, l / 2, COUL_VAR, false);
    return m;
}

// ---------------------------------------------------------------- tribune (WFS-199 pour les mesures des rangs)
Maille tribune(const Couleurs& c, float l, int rangs, float rang, float marche, float muret, float entreAllees, float allee, bool loin) {
    Maille m;
    auto hRang = [&](int k) { return muret + k * marche; };
    const float L = l / 2, front = -rang / 2, back = (rangs - 0.5f) * rang, haut = hRang(rangs - 1), nez = 0.03f;
    // gradins de béton : un seul profil en escalier (nez de marche chanfreinés, peints en clair)
    {
        std::vector<vec2> p{{front, 0}, {back - 0.15f, 0}, {back - 0.15f, haut}};
        std::vector<int> cols{c.beton, c.beton, c.beton};
        for (int k = rangs - 1; k >= 0; k--) {
            const float x0 = (k - 0.5f) * rang, h = hRang(k), hb = k ? hRang(k - 1) : 0.f;
            p.push_back({x0 + nez, h}); cols.push_back(c.betonNez);    // nez de marche chanfreiné (arête suivante)
            p.push_back({x0, h - nez}); cols.push_back(c.beton);       // contremarche (au rang 0 : muret avant)
            p.push_back({x0, hb}); cols.push_back(c.beton);            // giron du rang inférieur, ou retour au sol
        }
        extrudeZ(m, p, -L, L, c.beton, false, &cols);
        for (int k = 0; k < rangs; k++) for (float z : {-L, L})   // joues des extrémités (un rectangle par rang)
            quadPlat(m, {(k - 0.5f) * rang, 0, z}, {(k + 0.5f) * rang - (k == rangs - 1 ? 0.15f : 0.f), 0, z}, {(k + 0.5f) * rang - (k == rangs - 1 ? 0.15f : 0.f), hRang(k), z},
                     {(k - 0.5f) * rang, hRang(k), z}, c.beton, {0, 0, z});
    }
    // allées d'escalier : une marche intermédiaire par rang, main courante au milieu de l'allée
    for (float d = -L + entreAllees; d < L - 3; d += entreAllees) {
        for (int k = 0; k + 1 < rangs; k++) {
            const float x0 = k * rang, x1 = (k + 0.5f) * rang, h = hRang(k), h2 = h + marche * 0.5f;
            extrudeZ(m, {{x0, h}, {x1, h}, {x1, h2}, {x0 + nez, h2}, {x0, h2 - nez}}, d - allee / 2, d + allee / 2, c.beton, true);
        }
        if (!loin) {
            tube(m, {front + 0.35f, hRang(0) + 0.9f, d}, {back - 0.5f, haut + 0.9f, d}, 0.024f, 6, c.acier);
            for (int k = 0; k < rangs; k += 2) tube(m, {k * rang, hRang(k), d}, {k * rang, hRang(k) + 0.9f + 0.02f, d}, 0.024f, 6, c.acier);
        }
    }
    // garde-corps avant sur le muret (EN 13200-3 : 1,0 m), poteaux tous les 2 m
    gardeCorps(m, {front + 0.05f, muret, -L + 0.2f}, {front + 0.05f, muret, L - 0.2f}, 1.0f, 2.f, c.acier, !loin);
    // charpente : poteaux arrière en HEA 300 et poteaux avant en tubes Ø 219 mm tous les 8 m, arbalétriers en IPE 300
    const float yA = haut + 3.45f, yF = haut + 3.2f, xF = front - 0.3f, xB = back + 0.3f;
    for (float d = -L + 0.5f; d <= L; d += 8.f) {
        profileI(m, {xB, 0, d}, {xB, yA, d}, AX, 0.29f, 0.3f, 0.0085f, 0.014f, c.acier);
        tube(m, {xF, 0, d}, {xF, yF - 0.15f, d}, 0.11f, 8, c.acier);
        profileI(m, {xB, yA - 0.16f, d}, {xF - 0.4f, yF - 0.16f, d}, HAUT, 0.3f, 0.15f, 0.0071f, 0.0107f, c.acier);
        platine(m, xF, d, 0.4f, c.acier, !loin);
    }
    // pannes (IPE 160) le long de la tribune
    if (!loin) for (float t : {0.1f, 0.5f, 0.9f}) {
        const float x = xB + (xF - 0.4f - xB) * t, y = yA + (yF - yA) * t;
        profileI(m, {x, y - 0.02f, -L - 0.5f}, {x, y - 0.02f, L + 0.5f}, HAUT, 0.16f, 0.082f, 0.005f, 0.0074f, c.acier);
    }
    // couverture en bac acier (nervures dans le sens de la pente), chéneau à l'avant
    {
        const vec3 o{xB, yA + 0.05f, -L - 0.5f}, fin{xF - 0.5f, yF + 0.05f, -L - 0.5f};
        bacAcier(m, o, AZ, fin - o, HAUT, l + 1, length(fin - o), loin ? 1.0f : 0.25f, 0.04f, c.tole);
        extrudeZ(m, {{xF - 0.65f, yF - 0.15f}, {xF - 0.45f, yF - 0.15f}, {xF - 0.45f, yF + 0.1f}, {xF - 0.65f, yF + 0.1f}}, -L - 0.5f, L + 0.5f, c.tole, true);
    }
    // mur du fond : bardage nervuré vertical sur lisses, soubassement de béton
    extrudeZ(m, {{back - 0.15f, 0}, {back + 0.15f, 0}, {back + 0.15f, 0.6f}, {back - 0.15f, 0.6f}}, -L, L, c.beton, true);
    bacAcier(m, {back + 0.05f, 0.6f, -L}, AZ, HAUT, AX, l, yA - 0.6f, loin ? 1.0f : 0.25f, 0.035f, c.bardage);
    return m;
}

// ---------------------------------------------------------------- mur des stands
Maille murStands(const Couleurs& c, float l, bool loin) {
    Maille m;
    // muret de béton préfabriqué (0,4 m d'épaisseur, 1,1 m), arêtes chanfreinées, joint de 1 cm entre deux éléments
    extrudeZ(m, {{-0.2f, 0}, {0.2f, 0}, {0.2f, MURET_STANDS - 0.04f}, {0.16f, MURET_STANDS}, {-0.16f, MURET_STANDS}, {-0.2f, MURET_STANDS - 0.04f}}, 0.005f, l - 0.005f, c.beton, true);
    // grillage de protection : poteau (tube Ø 114 mm) au joint, haut replié vers la piste ; câbles horizontaux ; maille losangée
    const float y0 = MURET_STANDS, y1 = MURET_STANDS + GRILLAGE_STANDS;
    tube(m, {0.05f, y0 - 0.05f, 0}, {0.05f, y1, 0}, 0.057f, loin ? 6 : 8, c.acier);
    tube(m, {0.05f, y1, 0}, {-0.35f, y1 + 0.4f, 0}, 0.057f, loin ? 6 : 8, c.acier);
    poutre(m, {0.05f, y0 + 0.01f, -0.15f}, {0.05f, y0 + 0.01f, 0.15f}, HAUT, 0.3f, 0.02f, 0.004f, c.acier);   // platine sur le muret
    const int nc = loin ? 3 : 6;
    for (int k = 0; k < nc; k++) {
        const float y = y0 + 0.25f + (GRILLAGE_STANDS - 0.3f) * k / (nc - 1);
        ligne(m, {-0.01f, y, 0}, {-0.01f, y, l}, 0.02f, c.acier);
    }
    ligne(m, {-0.25f, y1 + 0.3f, 0}, {-0.25f, y1 + 0.3f, l}, 0.02f, c.acier);
    if (!loin) {
        const float pas = 0.3f, H = y1 - (y0 + 0.1f);
        for (float s = -H; s < l; s += pas) for (int sens : {1, -1}) {
            // segment de pente ±1 dans le plan x = −0,005 : z = s + t, y = y0 + 0,1 + t (ou descendant)
            float t0 = std::max(0.f, -s), t1 = std::min(H, l - s);
            if (t1 <= t0) continue;
            const float ya = sens > 0 ? y0 + 0.1f + t0 : y1 - t0, yb = sens > 0 ? y0 + 0.1f + t1 : y1 - t1;
            ligne(m, {-0.005f, ya, s + t0}, {-0.005f, yb, s + t1}, 0.004f, c.acier);
        }
    }
    return m;
}

// ---------------------------------------------------------------- stand d'écurie vitré avec auvent (ANO-220)
Maille stand(const Couleurs& c, float l, Maille& neon, bool loin) {
    Maille m;
    const float L = l / 2, P = STAND_PROF, Hp = STAND_PORTE_H, yA = 4.8f;
    // poteaux de béton de la façade (40 × 40 cm, chanfreinés) aux bouts et au milieu
    for (float z : {-L + 0.2f, 0.f, L - 0.2f}) poutre(m, {0.2f, 0, z}, {0.2f, yA, z}, AX, 0.4f, 0.4f, 0.02f, c.enduit);
    // deux portes : rideau métallique à lames de 100 mm à moitié levé, intérieur du garage (sol époxy, murs, plafond)
    for (int k = 0; k < 2; k++) {
        const float z0 = k ? 0.2f : -L + 0.4f, z1 = k ? L - 0.4f : -0.2f, levee = k ? 2.4f : 2.9f;
        bacAcier(m, {0.12f, levee, z0}, HAUT, AZ, -AX, Hp - levee, z1 - z0, loin ? 0.5f : 0.1f, 0.012f, c.rideau);
        poutre(m, {0.1f, levee - 0.05f, z0}, {0.1f, levee - 0.05f, z1}, HAUT, 0.06f, 0.1f, 0.01f, c.aluSombre);   // lame finale
        quadPlat(m, {6.f, 0, z0}, {6.f, Hp, z0}, {6.f, Hp, z1}, {6.f, 0, z1}, c.interieur, -AX);
        quadPlat(m, {0.15f, 0.01f, z0}, {6.f, 0.01f, z0}, {6.f, 0.01f, z1}, {0.15f, 0.01f, z1}, c.dalle, HAUT);
        quadPlat(m, {0.15f, Hp, z0}, {6.f, Hp, z0}, {6.f, Hp, z1}, {0.15f, Hp, z1}, c.interieur, -HAUT);
        for (float x : {1.5f, 3.5f}) poutre(neon, {x, Hp - 0.04f, z0 + 0.5f}, {x, Hp - 0.04f, z1 - 0.5f}, HAUT, 0.12f, 0.05f, 0.f, c.panneau);   // réglettes du garage
        for (float z : {z0, z1}) quadPlat(m, {0.15f, 0, z}, {6.f, 0, z}, {6.f, Hp, z}, {0.15f, Hp, z}, c.interieur, {0, 0, (z0 + z1) / 2 - z});
        if (!loin) {   // coffre du rideau et coulisses
            tube(m, {0.35f, Hp - 0.25f, z0}, {0.35f, Hp - 0.25f, z1}, 0.22f, 8, c.rideau);
            for (float z : {z0, z1}) poutre(m, {0.12f, 0, z}, {0.12f, Hp, z}, AX, 0.08f, 0.08f, 0.005f, c.aluSombre);
        }
    }
    // linteau : bandeau aux couleurs de l'équipe
    extrudeZ(m, {{-0.02f, Hp}, {0.4f, Hp}, {0.4f, yA}, {-0.02f, yA}}, -L, L, COUL_VAR, true);
    // auvent en porte-à-faux de 3 m (dalle de 22 cm, nez chanfreiné) sur consoles en IPE ; garde-corps vitré de la terrasse
    extrudeZ(m, {{-AUVENT, yA}, {0.4f, yA}, {0.4f, yA + 0.22f}, {-AUVENT + 0.03f, yA + 0.22f}, {-AUVENT, yA + 0.19f}}, -L, L, c.dalle, true);
    for (float z : {-L + 0.6f, 0.f, L - 0.6f}) {
        profileI(m, {0, yA - 0.12f, z}, {-AUVENT + 0.2f, yA - 0.12f, z}, HAUT, 0.24f, 0.12f, 0.0062f, 0.0098f, c.acier);
        if (!loin) profileI(m, {0, Hp - 0.6f, z}, {-AUVENT * 0.6f, yA - 0.24f, z}, HAUT, 0.16f, 0.08f, 0.005f, 0.0074f, c.acier);
    }
    poutre(neon, {-AUVENT + 0.5f, yA - 0.03f, -L + 0.3f}, {-AUVENT + 0.5f, yA - 0.03f, L - 0.3f}, HAUT, 0.06f, 0.03f, 0.f, c.panneau);   // éclairage sous l'auvent
    const float xG = -AUVENT + 0.15f, yG = yA + 0.22f;
    for (float z = -L + 0.2f; z <= L; z += 1.5f) tube(m, {xG, yG, z}, {xG, yG + 1.05f, z}, 0.02f, 6, c.alu);
    tube(m, {xG, yG + 1.08f, -L}, {xG, yG + 1.08f, L}, 0.03f, 6, c.alu);
    quadPlat(m, {xG + 0.02f, yG + 0.08f, -L + 0.1f}, {xG + 0.02f, yG + 1.0f, -L + 0.1f}, {xG + 0.02f, yG + 1.0f, L - 0.1f}, {xG + 0.02f, yG + 0.08f, L - 0.1f}, c.verre, -AX);
    // étage : façade vitrée en retrait (mur-rideau, meneaux tous les 1,35 m, traverses), acrotère et toiture
    const float xV = 0.6f, yV0 = yG, yV1 = STAND_HAUT;
    quadPlat(m, {xV + 0.03f, yV0, -L}, {xV + 0.03f, yV1, -L}, {xV + 0.03f, yV1, L}, {xV + 0.03f, yV0, L}, c.verre, -AX);
    const int nm = std::max(2, (int)std::lround(l / (loin ? 2.7f : 1.35f)));
    for (int k = 0; k <= nm; k++) { const float z = -L + l * k / nm; poutre(m, {xV, yV0, z}, {xV, yV1, z}, AX, 0.15f, 0.06f, 0.f, c.alu); }
    for (float y : {yV0 + 0.05f, yV0 + 1.1f, yV1 - 0.05f}) poutre(m, {xV, y, -L}, {xV, y, L}, HAUT, 0.15f, 0.06f, 0.f, c.alu);
    extrudeZ(m, {{0.3f, yV1}, {P, yV1}, {P, yV1 + 0.3f}, {0.3f, yV1 + 0.3f}}, -L, L, c.dalle, true);                       // dalle de toiture
    extrudeZ(m, {{0.3f, yV1 + 0.3f}, {0.55f, yV1 + 0.3f}, {0.55f, yV1 + 0.75f}, {0.33f, yV1 + 0.78f}, {0.3f, yV1 + 0.75f}}, -L, L, c.enduit, true);   // acrotère et couvertine
    // murs : pignons entre deux stands (enduit, chanfreinés) et façade arrière
    for (float z : {-L + 0.1f, L - 0.1f}) extrudeZ(m, {{6.f, 0}, {P, 0}, {P, yV1}, {6.f, yV1}}, z - 0.1f, z + 0.1f, c.enduit, true);
    extrudeZ(m, {{P - 0.2f, 0}, {P, 0}, {P, yV1}, {P - 0.2f, yV1}}, -L, L, c.enduit, true);
    return m;
}

// ---------------------------------------------------------------- drapeaux
Maille drapeau(const Couleurs& c, bool loin) {
    Maille m;
    // mât conique en aluminium (Ø 100 → 60 mm), platine boulonnée, pommeau, drisse
    revolution(m, {0, 0, 0}, HAUT, {{0, 0.05f}, {MAT_DRAPEAU, 0.03f}}, loin ? 6 : 8, c.mat);
    revolution(m, {0, MAT_DRAPEAU, 0}, HAUT, {{0, 0.03f}, {0.02f, 0.045f}, {0.06f, 0.045f}, {0.1f, 0.02f}, {0.11f, 0.f}}, 8, c.mat);
    platine(m, 0, 0, 0.3f, c.acier, !loin);
    if (!loin) ligne(m, {0.04f, 1.2f, 0}, {0.04f, MAT_DRAPEAU, 0}, 0.006f, c.panneau);
    // toile (2,4 × 1,6 m) qui flotte vers +z, ondulée
    toile(m, {0, MAT_DRAPEAU - 0.15f, 0.04f}, AZ, -HAUT, AX, DRAPEAU_L, DRAPEAU_H, loin ? 3 : 8, loin ? 1 : 3, 0.12f, 0.f, COUL_VAR);
    return m;
}

Maille drapeauMain(const Couleurs& c) {
    Maille m;
    tube(m, {0, 0, 0}, {0, 1.9f, 0}, 0.012f, 5, c.bois);
    toile(m, {0, 1.88f, -0.01f}, -AZ, -HAUT, AX, 1.2f, 0.8f, 6, 2, 0.07f, 0.8f, COUL_VAR);
    return m;
}

// ---------------------------------------------------------------- poste de commissaire
Maille posteCommissaire(const Couleurs& c, bool loin) {
    Maille m;
    const float h = COMMISSAIRE_COTE / 2 - 0.05f, y = COMMISSAIRE_PLANCHER, toitAv = 2.85f, toitAr = 3.05f;
    for (float x : {-h, h}) for (float z : {-h, h}) {
        poutre(m, {x, 0, z}, {x, x < 0 ? toitAv : toitAr, z}, AX, 0.08f, 0.08f, 0.006f, c.commissaire);
        platine(m, x, z, 0.2f, c.acier, !loin);
    }
    // plancher en caillebotis (dalle de 6 cm à rebord), longrines
    extrudeZ(m, {{-h - 0.05f, y - 0.06f}, {h + 0.05f, y - 0.06f}, {h + 0.05f, y}, {-h - 0.05f, y}}, -h - 0.05f, h + 0.05f, c.acier, true);
    for (float z : {-h, h}) poutre(m, {-h, y - 0.15f, z}, {h, y - 0.15f, z}, HAUT, 0.06f, 0.12f, 0.004f, c.commissaire);
    // garde-corps sur trois côtés (face piste et côtés), panneau plein en bas à l'arrière
    gardeCorps(m, {-h, y, -h}, {-h, y, h}, 1.0f, 1.0f, c.commissaire);
    gardeCorps(m, {-h, y, -h}, {h, y, -h}, 1.0f, 1.0f, c.commissaire);
    gardeCorps(m, {-h, y, h}, {h - 0.8f, y, h}, 1.0f, 1.0f, c.commissaire);
    quadPlat(m, {h + 0.02f, y, -h}, {h + 0.02f, y + 1.0f, -h}, {h + 0.02f, y + 1.0f, h}, {h + 0.02f, y, h}, c.abri, AX);
    // toit à une pente (bac acier), débord de 20 cm
    const vec3 o{h + 0.25f, toitAr + 0.04f, -h - 0.25f}, fin{-h - 0.25f, toitAv + 0.04f, -h - 0.25f};
    bacAcier(m, o, AZ, fin - o, HAUT, 2 * h + 0.5f, length(fin - o), 0.2f, 0.03f, c.abri);
    for (float z : {-h, h}) poutre(m, {h, toitAr - 0.05f, z}, {-h, toitAv - 0.05f, z}, HAUT, 0.06f, 0.1f, 0.004f, c.commissaire);
    // escalier (4 marches) à l'arrière, côté droit
    if (!loin) {
        const float z0 = h - 0.75f, z1 = h - 0.05f;
        for (float z : {z0, z1}) poutre(m, {h + 1.2f, 0, z}, {h, y, z}, AX, 0.04f, 0.16f, 0.004f, c.commissaire);
        for (int k = 1; k <= 4; k++) {
            const float t = k / 5.f, x = h + 1.2f * (1 - t), yy = y * t;
            poutre(m, {x, yy, z0}, {x, yy, z1}, HAUT, 0.24f, 0.03f, 0.004f, c.acier);
        }
    }
    return m;
}

// ---------------------------------------------------------------- panneau de freinage
Maille panneauFrein(const Couleurs& c) {
    Maille m;
    const float yc = FREIN_BAS + FREIN_H / 2;
    for (float z : {-0.6f, 0.6f}) { tube(m, {0.05f, 0, z}, {0.05f, FREIN_BAS + FREIN_H, z}, 0.038f, 8, c.alu); platine(m, 0.05f, z, 0.2f, c.acier, true); }
    poutre(m, {0, yc, -FREIN_L / 2}, {0, yc, FREIN_L / 2}, HAUT, 0.04f, FREIN_H, 0.008f, c.panneau);
    // cadre d'aluminium (profil de 40 mm) autour de la face
    const float a = FREIN_L / 2, b = FREIN_H / 2;
    for (float s : {-1.f, 1.f}) {
        poutre(m, {-0.025f, yc + s * (b - 0.02f), -a}, {-0.025f, yc + s * (b - 0.02f), a}, HAUT, 0.012f, 0.04f, 0.002f, c.alu);
        poutre(m, {-0.025f, yc - b, s * (a - 0.02f)}, {-0.025f, yc + b, s * (a - 0.02f)}, AX, 0.012f, 0.04f, 0.002f, c.alu);
    }
    return m;
}

// ---------------------------------------------------------------- portique des feux de départ
Maille portique(const Couleurs& c, float demi) {
    Maille m;
    // pylônes en treillis (0,5 × 0,5 m, membrures Ø 76 mm, entretoises Ø 42 mm) sur massifs de béton
    for (float s : {-1.f, 1.f}) {
        pylone(m, s * demi, 0, 0.3f, 8.6f, 0.5f, 0.6f, 0.038f, 0.021f, c.aluSombre, false);
        extrudeZ(m, {{s * demi - 0.5f, 0}, {s * demi + 0.5f, 0}, {s * demi + 0.5f, 0.27f}, {s * demi + 0.47f, 0.3f}, {s * demi - 0.47f, 0.3f}, {s * demi - 0.5f, 0.27f}}, -0.5f, 0.5f, c.beton, true);
    }
    // poutre en treillis (0,6 × 0,6 m) d'un pylône à l'autre
    poutreTreillis(m, -demi - 0.25f, demi + 0.25f, 8.3f, 0, 0.56f, 0.6f, 0.038f, 0.021f, c.aluSombre);
    // caisson des feux (5,6 × 1,2 × 0,35 m, arêtes arrondies), suspentes ; visière au-dessus de chaque feu
    poutre(m, {-2.8f, 7.4f, 0.1f}, {2.8f, 7.4f, 0.1f}, HAUT, 0.35f, 1.2f, 0.03f, c.aluSombre);
    for (float x : {-2.4f, 2.4f}) tube(m, {x, 8.0f, 0.1f}, {x, 8.04f, 0.1f}, 0.04f, 6, c.aluSombre);
    for (int k = 0; k < 5; k++) {
        const float x = (k - 2) * 1.0f;
        revolution(m, {x, 7.25f, -0.075f}, -AZ, {{0, 0.17f}, {0.26f, 0.17f}}, 7, c.aluSombre, 0, PI, AX);   // visière (demi-tube)
        revolution(m, {x, 7.25f, -0.075f}, -AZ, {{0, 0.15f}, {0.01f, 0.15f}}, 12, c.aluSombre);             // bague du feu
    }
    return m;
}

Maille lentilleFeu() {
    Maille m;
    // lentille bombée de polycarbonate (Ø 240 mm) : la couleur est celle du matériau du feu (allumé / éteint)
    revolution(m, {0, 0, 0}, -AZ, {{0, 0.12f}, {0.02f, 0.115f}, {0.04f, 0.08f}, {0.05f, 0.f}}, 12, 0);
    return m;
}

// ---------------------------------------------------------------- passerelle publicitaire
Maille passerelle(const Couleurs& c, float demi) {
    Maille m;
    const float yT = 6.2f, xT = demi + 1.0f;
    // piles de béton chanfreinées et chevêtre ; tablier caisson (2,4 × 0,5 m, chanfreins), dalle de marche, garde-corps à barreaudage
    for (float s : {-1.f, 1.f}) {
        poutre(m, {s * demi, 0, 0}, {s * demi, yT - 0.3f, 0}, AX, 1.0f, 1.0f, 0.05f, c.beton);
        poutre(m, {s * demi, yT - 0.3f, 0}, {s * demi, yT, 0}, AX, 1.3f, 1.6f, 0.04f, c.beton);
    }
    poutre(m, {-xT, yT + 0.25f, 0}, {xT, yT + 0.25f, 0}, HAUT, 2.4f, 0.5f, 0.04f, c.beton);
    poutre(m, {-xT + 0.05f, yT + 0.51f, 0}, {xT - 0.05f, yT + 0.51f, 0}, HAUT, 2.2f, 0.02f, 0.f, c.dalle);
    for (float z : {-1.12f, 1.12f}) {
        gardeCorps(m, {-xT + 0.1f, yT + 0.5f, z}, {xT - 0.1f, yT + 0.5f, z}, 1.1f, 1.5f, c.acier, false);
        for (float x = -xT + 0.2f; x < xT - 0.1f; x += 0.12f) ligne(m, {x, yT + 0.55f, z}, {x, yT + 1.55f, z}, 0.016f, c.acier);
    }
    // cadres des panneaux publicitaires sous le tablier (lisses haute et basse)
    for (float z : {-1.27f, 1.27f}) for (float y : {4.62f, 6.18f}) poutre(m, {-demi + 0.6f, y, z}, {demi - 0.6f, y, z}, HAUT, 0.05f, 0.05f, 0.004f, c.alu);
    for (float z : {-1.27f, 1.27f}) for (float s : {-1.f, 1.f}) poutre(m, {s * (demi - 0.6f), 4.6f, z}, {s * (demi - 0.6f), 6.2f, z}, AX, 0.05f, 0.05f, 0.004f, c.alu);
    return m;
}

// ---------------------------------------------------------------- camion d'équipe (semi-remorque fourgon, sans marque)
Maille camion(const Couleurs& c, bool loin) {
    Maille m;
    const float W = CAMION_LARGEUR / 2, zR0 = -REMORQUE_L / 2, zR1 = REMORQUE_L / 2, yP = 1.2f, H = CAMION_HAUT;
    // caisse de la remorque : panneaux à arêtes arrondies, montants d'angle et rives de toit en aluminium
    poutre(m, {0, (yP + H) / 2, zR0}, {0, (yP + H) / 2, zR1}, HAUT, CAMION_LARGEUR, H - yP, 0.06f, c.cabine);
    for (float sx : {-1.f, 1.f}) {
        quadPlat(m, {sx * (W + 0.002f), 1.95f, zR0 + 0.3f}, {sx * (W + 0.002f), 2.95f, zR0 + 0.3f}, {sx * (W + 0.002f), 2.95f, zR1 - 0.3f}, {sx * (W + 0.002f), 1.95f, zR1 - 0.3f}, COUL_VAR, {sx, 0, 0});
        poutre(m, {sx * (W - 0.02f), H - 0.02f, zR0}, {sx * (W - 0.02f), H - 0.02f, zR1}, HAUT, 0.06f, 0.06f, 0.01f, c.alu);
        poutre(m, {sx * (W - 0.02f), yP + 0.03f, zR0}, {sx * (W - 0.02f), yP + 0.03f, zR1}, HAUT, 0.06f, 0.08f, 0.01f, c.alu);
        for (float z : {zR0 + 0.03f, zR1 - 0.03f}) poutre(m, {sx * (W - 0.02f), yP, z}, {sx * (W - 0.02f), H, z}, AX, 0.07f, 0.07f, 0.01f, c.alu);
        // jupes latérales (carénage) entre les essieux
        poutre(m, {sx * (W - 0.05f), 0.85f, -3.0f}, {sx * (W - 0.05f), 0.85f, 4.2f}, HAUT, 0.02f, 0.6f, 0.005f, c.cabine);
    }
    // portes arrière : deux vantaux, charnières et barres de fermeture
    if (!loin) {
        poutre(m, {0, (yP + H) / 2, zR0 - 0.012f}, {0, (yP + H) / 2, zR0 - 0.008f}, HAUT, 0.02f, H - yP - 0.1f, 0.f, c.aluSombre);
        for (float x : {-0.75f, -0.45f, 0.45f, 0.75f}) tube(m, {x, yP + 0.15f, zR0 - 0.04f}, {x, H - 0.15f, zR0 - 0.04f}, 0.017f, 6, c.alu);
        for (float x : {-W + 0.05f, W - 0.05f}) for (float y : {1.6f, 2.6f, 3.6f}) poutre(m, {x, y, zR0 - 0.02f}, {x, y + 0.1f, zR0 - 0.02f}, AX, 0.05f, 0.04f, 0.f, c.alu);
    }
    for (float sx : {-1.f, 1.f}) {   // feux arrière, pare-cycliste
        quadPlat(m, {sx * 0.95f, 0.75f, zR0 - 0.06f}, {sx * 1.2f, 0.75f, zR0 - 0.06f}, {sx * 1.2f, 0.9f, zR0 - 0.06f}, {sx * 0.95f, 0.9f, zR0 - 0.06f}, c.feuArriere, -AZ);
        profileI(m, {sx * 0.5f, 1.0f, zR0 + 0.2f}, {sx * 0.5f, 1.0f, zR1 - 0.5f}, HAUT, 0.36f, 0.12f, 0.007f, 0.012f, c.chassis);   // longerons
    }
    profileI(m, {-1.2f, 0.7f, zR0 - 0.05f}, {1.2f, 0.7f, zR0 - 0.05f}, HAUT, 0.14f, 0.1f, 0.005f, 0.008f, c.chassis);   // pare-chocs arrière
    // béquilles de la remorque
    for (float sx : {-1.f, 1.f}) { tube(m, {sx * 0.9f, 0.15f, 4.3f}, {sx * 0.9f, yP, 4.3f}, 0.05f, 6, c.chassis); poutre(m, {sx * 0.9f, 0.04f, 4.15f}, {sx * 0.9f, 0.04f, 4.45f}, HAUT, 0.25f, 0.06f, 0.01f, c.chassis); }
    // trois essieux de remorque, deux du tracteur (roues 385/65 R22,5 et 315/80 R22,5)
    for (float z : {-3.4f, -4.71f, -6.02f}) for (float sx : {-1.f, 1.f}) roue(m, {sx * (W - 0.24f), ROUE_CAMION / 2, z}, {sx, 0, 0}, ROUE_CAMION, 0.385f, 0.572f, c, loin);
    for (float z : {5.45f, 8.9f}) for (float sx : {-1.f, 1.f}) roue(m, {sx * (W - 0.22f), ROUE_CAMION / 2, z}, {sx, 0, 0}, ROUE_CAMION, z < 6 ? 0.6f : 0.315f, 0.572f, c, loin);
    // tracteur : châssis, sellette, réservoir, cabine avancée galbée (anneaux horizontaux à coins arrondis)
    for (float sx : {-1.f, 1.f}) profileI(m, {sx * 0.45f, 0.95f, 4.9f}, {sx * 0.45f, 0.95f, 9.5f}, HAUT, 0.3f, 0.09f, 0.007f, 0.01f, c.chassis);
    poutre(m, {0, 1.15f, 5.1f}, {0, 1.15f, 6.1f}, HAUT, 1.0f, 0.12f, 0.03f, c.chassis);
    poutre(m, {-0.95f, 0.95f, 6.3f}, {-0.95f, 0.95f, 7.2f}, HAUT, 0.55f, 0.55f, 0.12f, c.alu);
    const float zA = 7.3f, zF = CAMION_AVANT - 0.06f;
    auto anneau = [&](float y, float zf, float rf, float rb, float w) {
        std::vector<vec3> r;
        const int n = loin ? 2 : 4;
        auto coin = [&](float cx, float cz, float rr, float a0) { for (int k = 0; k <= n; k++) { const float a = a0 + PI / 2 * k / n; r.push_back({cx + std::cos(a) * rr, y, cz + std::sin(a) * rr}); } };
        coin(w - rf, zf - rf, rf, 0);           // avant droit
        coin(-w + rf, zf - rf, rf, PI / 2);     // avant gauche
        coin(-w + rb, zA + rb, rb, PI);         // arrière gauche
        coin(w - rb, zA + rb, rb, 1.5f * PI);   // arrière droit
        return r;
    };
    std::vector<std::vector<vec3>> an = {anneau(1.3f, zF - 0.05f, 0.2f, 0.06f, 1.18f), anneau(2.3f, zF, 0.25f, 0.06f, 1.24f), anneau(3.2f, zF - 0.12f, 0.3f, 0.06f, 1.22f),
                                         anneau(3.85f, zF - 0.45f, 0.35f, 0.08f, 1.12f), anneau(3.95f, zF - 0.7f, 0.3f, 0.08f, 1.0f)};
    loft(m, an, COUL_VAR, true, {0, 1.3f, (zA + zF) / 2}, {0, 3.95f, (zA + zF) / 2});
    eventail(m, an.back(), COUL_VAR, HAUT);
    eventail(m, an.front(), c.chassis, -HAUT);
    // pare-brise incliné, vitres latérales, calandre à lames horizontales (sans marque), pare-chocs, phares, rétroviseurs
    auto avant = [&](float y) { return y < 2.3f ? zF - 0.05f + (y - 1.3f) * 0.05f : zF - (y - 2.3f) * 0.12f / 0.9f; };
    quadPlat(m, {-1.05f, 2.4f, avant(2.4f) + 0.008f}, {1.05f, 2.4f, avant(2.4f) + 0.008f}, {1.0f, 3.15f, avant(3.15f) + 0.008f}, {-1.0f, 3.15f, avant(3.15f) + 0.008f}, c.verre, AZ);
    for (float sx : {-1.f, 1.f}) {
        const float x = sx * 1.245f;
        quadPlat(m, {x, 2.35f, zF - 1.1f}, {x, 3.1f, zF - 1.1f}, {x, 3.1f, zF - 0.35f}, {x, 2.35f, zF - 0.3f}, c.verre, {sx, 0, 0});
        tube(m, {sx * 1.2f, 2.9f, zF - 0.3f}, {sx * 1.42f, 2.95f, zF - 0.25f}, 0.02f, 6, c.chassis);
        poutre(m, {sx * 1.45f, 2.6f, zF - 0.25f}, {sx * 1.45f, 3.0f, zF - 0.25f}, AX, 0.1f, 0.06f, 0.02f, c.chassis);
        quadPlat(m, {sx * 0.75f, 1.18f, zF + 0.062f}, {sx * 1.08f, 1.18f, zF + 0.062f}, {sx * 1.08f, 1.32f, zF + 0.062f}, {sx * 0.75f, 1.32f, zF + 0.062f}, c.phare, AZ);
        for (int k = 0; k < 2; k++) poutre(m, {sx * 1.21f, 0.55f + k * 0.3f, zF - 1.6f}, {sx * 1.21f, 0.55f + k * 0.3f, zF - 1.1f}, HAUT, 0.12f, 0.04f, 0.008f, c.alu);   // marchepieds
    }
    for (int k = 0; k < 5; k++) poutre(m, {-0.7f, 1.6f + k * 0.12f, zF + 0.01f}, {0.7f, 1.6f + k * 0.12f, zF + 0.01f}, HAUT, 0.05f, 0.05f, 0.01f, c.aluSombre);
    poutre(m, {-1.2f, 1.15f, zF + 0.0f}, {1.2f, 1.15f, zF + 0.0f}, HAUT, 0.12f, 0.35f, 0.04f, c.chassis);
    return m;
}

// ---------------------------------------------------------------- bar d'accueil (2 niveaux vitrés, terrasse VIP)
Maille accueil(const Couleurs& c, float w, float l, int parasol0, int variante, Maille& neon, bool loin) {
    Maille m;
    const float L = l / 2, y1 = 3.3f, y2 = 3.8f, y3 = 6.9f, yT = 7.5f;
    // dalles débordantes (nez chanfreiné) et soubassement
    extrudeZ(m, {{0.f, 0}, {w, 0}, {w, 0.3f}, {0.f, 0.3f}}, -L, L, c.beton, true);
    for (auto [ya, yb] : {std::pair{y1, y2}, std::pair{y3, yT}})
        extrudeZ(m, {{-0.3f, ya}, {w + 0.3f, ya}, {w + 0.3f, yb}, {-0.27f, yb}, {-0.3f, yb - 0.03f}}, -L - 0.3f, L + 0.3f, c.dalle, true);
    // murs-rideaux sur les 4 faces : vitrage, meneaux tous les 1,5 m
    const float pas = loin ? 3.f : 1.5f;
    for (auto [ya, yb] : {std::pair{0.3f, y1}, std::pair{y2, y3}}) {
        quadPlat(m, {0.03f, ya, -L}, {0.03f, yb, -L}, {0.03f, yb, L}, {0.03f, ya, L}, c.verre, -AX);
        quadPlat(m, {w - 0.03f, ya, -L}, {w - 0.03f, yb, -L}, {w - 0.03f, yb, L}, {w - 0.03f, ya, L}, c.verre, AX);
        for (float z : {-L + 0.03f, L - 0.03f}) quadPlat(m, {0, ya, z}, {w, ya, z}, {w, yb, z}, {0, yb, z}, c.verre, {0, 0, z});
        for (float z = -L; z <= L + 0.01f; z += pas) for (float x : {0.f, w}) poutre(m, {x, ya, z}, {x, yb, z}, AX, 0.12f, 0.06f, 0.f, c.alu);
        for (float x = pas; x < w - 0.01f; x += pas) for (float z : {-L, L}) poutre(m, {x, ya, z}, {x, yb, z}, AX, 0.06f, 0.12f, 0.f, c.alu);
    }
    // bandeaux lumineux (LED) sous les nez de dalle côté route
    for (float y : {y1 - 0.04f, y3 - 0.04f}) poutre(neon, {-0.25f, y, -L}, {-0.25f, y, L}, HAUT, 0.04f, 0.04f, 0.f, variante ? c.phare : c.panneau);
    // terrasse : platelage de bois, garde-corps vitré (poteaux, main courante), parasols
    quadPlat(m, {0.2f, yT + 0.01f, -L + 0.2f}, {w - 0.2f, yT + 0.01f, -L + 0.2f}, {w - 0.2f, yT + 0.01f, L - 0.2f}, {0.2f, yT + 0.01f, L - 0.2f}, c.bois, HAUT);
    auto rambarde = [&](vec3 a, vec3 b, vec3 dehors) {
        const float Lr = length(b - a);
        const int n = std::max(1, (int)std::ceil(Lr / 1.5f));
        for (int k = 0; k <= n; k++) { const vec3 p = a + (b - a) * ((float)k / n); tube(m, p, p + HAUT * 1.1f, 0.025f, 6, c.alu); }
        tube(m, a + HAUT * 1.12f, b + HAUT * 1.12f, 0.03f, 6, c.alu);
        quadPlat(m, a + HAUT * 0.08f, b + HAUT * 0.08f, b + HAUT * 1.02f, a + HAUT * 1.02f, c.verre, dehors);
    };
    rambarde({0.1f, yT, -L}, {0.1f, yT, L}, -AX); rambarde({w - 0.1f, yT, -L}, {w - 0.1f, yT, L}, AX);
    rambarde({0.1f, yT, -L}, {w - 0.1f, yT, -L}, -AZ); rambarde({0.1f, yT, L}, {w - 0.1f, yT, L}, AZ);
    for (int k = 0; k < 4; k++) {   // parasols à 8 baleines (toile en 16 points, festonnée), pied lesté
        const vec3 q{w / 2, yT, -L + 4 + k * (l - 8) / 3};
        const int col = parasol0 + (k + variante) % 6;
        revolution(m, q, HAUT, {{0, 0.28f}, {0.06f, 0.28f}, {0.1f, 0.08f}, {0.14f, 0.03f}, {0.15f, 0.f}}, 10, c.aluSombre);
        tube(m, q, q + HAUT * 2.5f, 0.025f, 6, c.alu);
        if (loin) { revolution(m, q + HAUT * 1.95f, HAUT, {{0, 1.6f}, {0.5f, 0.05f}}, 8, col); continue; }
        std::vector<std::vector<vec3>> an;
        for (auto [r, y] : {std::pair{0.06f, 2.48f}, std::pair{0.8f, 2.25f}, std::pair{1.6f, 1.95f}}) {
            std::vector<vec3> ring;
            for (int i = 0; i < 16; i++) {
                const float a = 2 * PI * i / 16, creux = i % 2 ? 0.04f * r / 1.6f : 0.f;   // toile tendue entre les baleines
                ring.push_back(q + vec3{std::cos(a) * (r - creux), y - creux * 0.5f, std::sin(a) * (r - creux)});
            }
            an.push_back(ring);
        }
        loft(m, an, col, true, q + HAUT * 3, q + HAUT * 1.5f);
        std::vector<std::vector<vec3>> lam{an.back(), an.back()};
        for (vec3& p : lam[1]) p.y -= 0.14f;   // lambrequin
        loft(m, lam, col, true, q, q + HAUT * 2);
        for (int i = 0; i < 16; i += 2) ligne(m, an[0][i] - HAUT * 0.02f, an[2][i] - HAUT * 0.02f, 0.012f, c.alu);
    }
    // enseigne (caisson d'aluminium de 7 × 1,2 m) sur deux potelets, côté route ; texte posé par circuit.cpp
    poutre(m, {-0.05f, 8.8f, -3.5f}, {-0.05f, 8.8f, 3.5f}, HAUT, 0.1f, 1.2f, 0.01f, c.aluSombre);
    for (float z : {-2.8f, 2.8f}) tube(m, {0.05f, yT, z}, {0.05f, 8.3f, z}, 0.04f, 6, c.aluSombre);
    return m;
}

// ---------------------------------------------------------------- tour de contrôle
Maille tour(const Couleurs& c, Maille& neon, bool loin) {
    Maille m;
    const float a = 1.75f, ch = 0.25f, yV = 16.f;
    // fût de béton à angles chanfreinés, pilastres d'angle, fentes vitrées de la cage d'escalier côté route
    extrudeZ(m, {{-a + ch, -a}, {a - ch, -a}, {a, -a + ch}, {a, a - ch}, {a - ch, a}, {-a + ch, a}, {-a, a - ch}, {-a, -a + ch}}, 0, 1, c.beton, false);
    for (auto& v : m.v) { const float z = v.p.z; v.p.z = v.p.y; v.p.y = z * yV; v.n = {v.n.x, v.n.z, v.n.y}; }   // profil du plan (x, z), extrudé selon y
    for (size_t k = 0; k < m.i.size(); k += 3) std::swap(m.i[k + 1], m.i[k + 2]);   // échange de y et z : sens des triangles rétabli
    for (int k = 0; k < 4; k++) {
        const float y0 = 2.f + k * 3.4f;
        quadPlat(m, {-a - 0.01f, y0, -0.3f}, {-a - 0.01f, y0 + 2.2f, -0.3f}, {-a - 0.01f, y0 + 2.2f, 0.3f}, {-a - 0.01f, y0, 0.3f}, c.verre, -AX);
        poutre(m, {-a - 0.06f, y0 - 0.06f, -0.4f}, {-a - 0.06f, y0 - 0.06f, 0.4f}, HAUT, 0.12f, 0.06f, 0.01f, c.dalle);   // appui
    }
    quadPlat(m, {-a - 0.01f, 0.f, -0.55f}, {-a - 0.01f, 2.3f, -0.55f}, {-a - 0.01f, 2.3f, 0.55f}, {-a - 0.01f, 0.f, 0.55f}, c.menuiserie, -AX);   // porte
    extrudeZ(m, {{-a - 1.2f, 2.4f}, {-a, 2.4f}, {-a, 2.55f}, {-a - 1.2f, 2.52f}}, -0.9f, 0.9f, c.dalle, true);   // auvent de la porte
    // vigie : dalle, allège, vitrage incliné vers l'extérieur (verrière évasée), meneaux, toit débordant
    extrudeZ(m, {{-3.25f, yV}, {3.25f, yV}, {3.25f, yV + 0.4f}, {-3.22f, yV + 0.4f}, {-3.25f, yV + 0.37f}}, -3.25f, 3.25f, c.dalle, true);
    const float b0 = 3.0f, b1 = 3.25f, yb = yV + 0.9f, yh = yV + 3.0f;
    extrudeZ(m, {{-b0, yV + 0.4f}, {b0, yV + 0.4f}, {b0, yb}, {-b0, yb}}, -b0, b0, c.enduit, true);
    const vec3 B[4] = {{-b0, yb, -b0}, {b0, yb, -b0}, {b0, yb, b0}, {-b0, yb, b0}}, T[4] = {{-b1, yh, -b1}, {b1, yh, -b1}, {b1, yh, b1}, {-b1, yh, b1}};
    for (int k = 0; k < 4; k++) {
        const int j = (k + 1) % 4;
        const vec3 milieu = (B[k] + B[j] + T[k] + T[j]) * 0.25f;
        quadPlat(m, B[k], B[j], T[j], T[k], c.verre, {milieu.x, 0, milieu.z});
        tube(m, B[k], T[k], 0.05f, 6, c.menuiserie);
        if (!loin) for (float t : {1 / 3.f, 2 / 3.f}) tube(m, lerp(B[k], B[j], t), lerp(T[k], T[j], t), 0.035f, 6, c.menuiserie);
    }
    extrudeZ(m, {{-3.7f, yh}, {3.7f, yh}, {3.7f, yh + 0.45f}, {-3.67f, yh + 0.45f}, {-3.7f, yh + 0.42f}}, -3.7f, 3.7f, c.dalle, true);
    poutre(neon, {-3.6f, yh - 0.03f, -3.6f}, {-3.6f, yh - 0.03f, 3.6f}, HAUT, 0.06f, 0.04f, 0.f, c.panneau);   // éclairage de la sous-face (côté route)
    // mât d'antennes en treillis, balise d'obstacle
    pylone(m, 0, 0, yh + 0.45f, yh + 4.5f, 0.3f, 0.5f, 0.02f, 0.012f, c.acier, loin);
    for (float y : {yh + 3.0f, yh + 4.0f}) tube(m, {-0.6f, y, 0}, {0.6f, y, 0}, 0.015f, 4, c.acier);
    revolution(neon, {0, yh + 4.5f, 0}, HAUT, {{0, 0.06f}, {0.08f, 0.06f}, {0.12f, 0.f}}, 8, c.feuArriere);
    return m;
}

// ---------------------------------------------------------------- hélicoptère léger monoturbine (sans marque)
Maille helico(const Couleurs& c, bool loin) {
    Maille m;
    // sections du fuselage (z, demi-largeur, demi-hauteur, hauteur du centre) : nez, cabine, poutre de queue
    struct Sec { float z, w, h, y; };
    const Sec S[] = {{2.65f, 0.10f, 0.12f, 1.05f}, {2.35f, 0.55f, 0.55f, 1.15f}, {1.7f, 0.85f, 0.75f, 1.3f}, {0.6f, 0.93f, 0.8f, 1.35f},
                     {-0.6f, 0.93f, 0.78f, 1.37f}, {-1.4f, 0.75f, 0.68f, 1.5f}, {-2.2f, 0.4f, 0.42f, 1.72f}, {-4.4f, 0.24f, 0.24f, 1.8f}, {-7.5f, 0.13f, 0.14f, 1.92f}};
    const int nS = (int)(sizeof(S) / sizeof(S[0])), na = loin ? 10 : 18;
    auto point = [&](const Sec& s, float a) {   // superellipse (exposant 2,6) : flancs plats, arêtes arrondies
        const float ca = std::cos(a), sa = std::sin(a), e = 2.f / 2.6f;
        return vec3{s.w * (ca < 0 ? -1.f : 1.f) * std::pow(std::fabs(ca), e), s.y + s.h * (sa < 0 ? -1.f : 1.f) * std::pow(std::fabs(sa), e), s.z};
    };
    // verrière : moitié haute de l'avant (de 0,3 à 2,5 m), en verre teinté ; le reste en peinture du fuselage
    auto bande = [&](int i0, int i1, float a0, float a1, int couleur) {
        std::vector<std::vector<vec3>> an;
        const int n = std::max(2, (int)std::lround(na * (a1 - a0) / (2 * PI)));
        for (int i = i0; i <= i1; i++) { std::vector<vec3> r; for (int k = 0; k <= n; k++) r.push_back(point(S[i], a0 + (a1 - a0) * k / n)); an.push_back(r); }
        loft(m, an, couleur, false, {0, 1.4f, S[i0].z}, {0, 1.4f, S[i1].z});
    };
    bande(0, 3, 0.25f, PI - 0.25f, c.verre);
    bande(0, 3, PI - 0.25f, 2 * PI + 0.25f, c.fuselage);
    bande(3, nS - 1, 0, 2 * PI, c.fuselage);
    { std::vector<vec3> r; for (int k = 0; k < na; k++) r.push_back(point(S[nS - 1], 2 * PI * k / na)); eventail(m, r, c.fuselage, -AZ); }
    // capot moteur sur le toit, mât et moyeu du rotor, 3 pales (Ø 10,7 m)
    poutre(m, {0, 2.25f, -1.6f}, {0, 2.25f, 0.5f}, HAUT, 0.9f, 0.4f, 0.15f, c.fuselage);
    tube(m, {0, 2.4f, -0.3f}, {0, HELICO_HAUT - 0.22f, -0.3f}, 0.08f, 8, c.pale);
    revolution(m, {0, HELICO_HAUT - 0.22f, -0.3f}, HAUT, {{0, 0.25f}, {0.12f, 0.25f}, {0.18f, 0.1f}, {0.22f, 0.f}}, 10, c.pale);
    for (int k = 0; k < 3; k++) {
        const float a = 2 * PI * k / 3 + 0.4f;
        const vec3 d{std::cos(a), 0, std::sin(a)}, o{0, HELICO_HAUT - 0.17f, -0.3f};
        poutre(m, o + d * 0.25f, o + d * (HELICO_ROTOR / 2) - HAUT * 0.12f, HAUT, 0.3f, 0.04f, 0.012f, c.pale);
    }
    // dérive, empennage horizontal, rotor anticouple
    for (float s : {-1.f, 1.f}) {
        const vec3 a0{s * 0.03f, 1.75f, -7.25f}, a1{s * 0.03f, 1.75f, -7.95f}, b1{s * 0.03f, HELICO_HAUT - 0.15f, HELICO_FIN}, b0{s * 0.03f, HELICO_HAUT - 0.15f, HELICO_FIN + 0.4f};
        quadPlat(m, a0, a1, b1, b0, c.fuselage, {s, 0, 0});
    }
    quadPlat(m, {-0.03f, HELICO_HAUT - 0.15f, HELICO_FIN + 0.4f}, {0.03f, HELICO_HAUT - 0.15f, HELICO_FIN + 0.4f}, {0.03f, HELICO_HAUT - 0.15f, HELICO_FIN}, {-0.03f, HELICO_HAUT - 0.15f, HELICO_FIN}, c.fuselage, HAUT);
    quadPlat(m, {-0.03f, 1.75f, -7.25f}, {0.03f, 1.75f, -7.25f}, {0.03f, HELICO_HAUT - 0.15f, HELICO_FIN + 0.4f}, {-0.03f, HELICO_HAUT - 0.15f, HELICO_FIN + 0.4f}, c.fuselage, AZ);
    poutre(m, {-1.0f, 1.85f, -6.4f}, {1.0f, 1.85f, -6.4f}, HAUT, 0.4f, 0.05f, 0.015f, c.fuselage);
    revolution(m, {0.12f, 2.1f, -7.85f}, AX, {{0, 0.07f}, {0.08f, 0.07f}, {0.12f, 0.f}}, 8, c.pale);
    for (int k = 0; k < 2; k++) { const float a = k * PI / 2 + 0.3f; const vec3 d{0, std::cos(a), std::sin(a)}, o{0.18f, 2.1f, -7.85f}; poutre(m, o - d * 0.85f, o + d * 0.85f, AX, 0.12f, 0.02f, 0.006f, c.pale); }
    // train à patins : tubes Ø 80 mm relevés à l'avant, traverses cintrées
    for (float s : {-1.f, 1.f}) {
        const float x = s * 1.15f;
        tube(m, {x, 0.05f, -1.6f}, {x, 0.05f, 1.9f}, 0.04f, 8, c.alu);
        tube(m, {x, 0.05f, 1.9f}, {x, 0.32f, 2.35f}, 0.04f, 8, c.alu);
        for (float z : {-0.9f, 1.1f}) { tube(m, {x, 0.05f, z}, {s * 1.0f, 0.55f, z}, 0.035f, 8, c.alu); tube(m, {s * 1.0f, 0.55f, z}, {s * 0.6f, 0.68f, z}, 0.035f, 8, c.alu); }
    }
    for (float z : {-0.9f, 1.1f}) tube(m, {-0.6f, 0.68f, z}, {0.6f, 0.68f, z}, 0.035f, 8, c.alu);
    return m;
}

// ---------------------------------------------------------------- espace fans : stand de marché et portail
Maille standFans(const Couleurs& c, Maille& neon) {
    Maille m;
    const float X = 1.5f, Z = 2.0f, ye = 2.6f, yf = 3.25f;
    // comptoir (façade, plateau de bois), parois basses, fond
    extrudeZ(m, {{-X, 0}, {-X + 0.06f, 0}, {-X + 0.06f, 1.0f}, {-X, 1.0f}}, -Z + 0.05f, Z - 0.05f, c.enduit, true);
    extrudeZ(m, {{-X - 0.1f, 1.0f}, {-X + 0.5f, 1.0f}, {-X + 0.5f, 1.05f}, {-X - 0.1f, 1.05f}}, -Z, Z, c.bois, true);
    for (float z : {-Z + 0.02f, Z - 0.02f}) quadPlat(m, {-X, 0, z}, {X, 0, z}, {X, 2.2f, z}, {-X, 1.1f, z}, c.enduit, {0, 0, z});
    quadPlat(m, {X - 0.02f, 0, -Z}, {X - 0.02f, 2.4f, -Z}, {X - 0.02f, 2.4f, Z}, {X - 0.02f, 0, Z}, c.enduit, AX);
    // ossature : 4 poteaux carrés et faîtage
    for (float x : {-X, X}) for (float z : {-Z, Z}) poutre(m, {x, 0, z}, {x, ye, z}, AX, 0.05f, 0.05f, 0.004f, c.alu);
    tube(m, {0, yf - 0.03f, -Z}, {0, yf - 0.03f, Z}, 0.02f, 6, c.alu);
    // toit de toile à deux pans (bâche tendue, flèche entre les arceaux), pignons, lambrequin festonné ; COUL_VAR
    for (float s : {-1.f, 1.f}) {
        grille(m, 3, 6, [&](int i, int j) {
            const float u = i / 2.f, zz = -Z - 0.1f + (2 * Z + 0.2f) * j / 5.f, fl = (j % 5 ? 0.025f : 0.f) * (i == 1 ? 1.f : 0.5f);
            return vec3{s * (X + 0.15f) * u, yf + (ye - yf) * u - fl, zz};
        }, COUL_VAR);
        // lambrequin : bande de 0,25 m le long de l'égout, bord bas en festons de 0,4 m
        grille(m, 2, 21, [&](int i, int j) {
            const float zz = -Z - 0.1f + (2 * Z + 0.2f) * j / 20.f, feston = i ? 0.25f - 0.06f * std::fabs(std::sin(PI * j / 2.f)) : 0.f;
            return vec3{s * (X + 0.15f), ye - feston, zz};
        }, COUL_VAR);
    }
    for (float z : {-Z - 0.1f, Z + 0.1f}) {   // pignons de toile, un peu creusés au milieu
        const vec3 a{-X - 0.15f, ye, z}, b{X + 0.15f, ye, z}, t{0, yf, z}, mi{0, ye + (yf - ye) * 0.35f, z - (z > 0 ? 0.03f : -0.03f)};
        triPlat(m, a, b, mi, COUL_VAR, {0, 0, z}); triPlat(m, b, t, mi, COUL_VAR, {0, 0, z}); triPlat(m, t, a, mi, COUL_VAR, {0, 0, z});
    }
    poutre(neon, {-X + 0.05f, ye - 0.15f, -Z + 0.1f}, {-X + 0.05f, ye - 0.15f, Z - 0.1f}, HAUT, 0.03f, 0.03f, 0.f, c.panneau);   // réglette LED
    return m;
}

Maille portail(const Couleurs& c, Maille& neon) {
    Maille m;
    for (float z : {-3.f, 3.f}) {
        pylone(m, 0, z, 0.f, 5.5f, 0.3f, 0.5f, 0.024f, 0.014f, c.alu, false);
        poutre(m, {0, 0.01f, z - 0.25f}, {0, 0.01f, z + 0.25f}, HAUT, 0.5f, 0.02f, 0.004f, c.acier);
    }
    poutre(m, {0, 4.8f, -3.4f}, {0, 4.8f, 3.4f}, HAUT, 0.3f, 1.2f, 0.03f, c.aluSombre);
    poutre(neon, {-0.2f, 4.15f, -3.3f}, {-0.2f, 4.15f, 3.3f}, HAUT, 0.05f, 0.05f, 0.f, c.panneau);
    return m;
}

// ---------------------------------------------------------------- grande roue (rayon 12 m, 12 nacelles)
Maille roue(const Couleurs& c, Maille& neon) {
    Maille m;
    const int N = 48;
    for (float x : {-1.2f, 1.2f}) {
        // jante en treillis : deux anneaux de tube (Ø 300 et 200 mm) reliés en zigzag
        for (auto [R, r] : {std::pair{12.f, 0.15f}, std::pair{11.4f, 0.1f}}) {
            std::vector<std::vector<vec3>> an;
            for (int k = 0; k <= N; k++) {
                const float a = 2 * PI * k / N;
                const vec3 o{x, R * std::cos(a), R * std::sin(a)}, e1 = vec3{0, std::cos(a), std::sin(a)};
                an.push_back(cercle(o, e1, AX, r, r, 6));
            }
            loft(m, an, c.acier, false, {x - 1, 0, 0}, {x + 1, 0, 0});   // (normales lissées sur le tore)
        }
        for (int k = 0; k < N; k++) {
            const float a = 2 * PI * k / N, b = 2 * PI * (k + 1) / N;
            tube(m, {x, 11.4f * std::cos(a), 11.4f * std::sin(a)}, {x, 12.f * std::cos(b), 12.f * std::sin(b)}, 0.05f, 4, c.acier);
            if (k % 2 == 0) ligne(neon, {x * 1.04f, 12.2f * std::cos(a), 12.2f * std::sin(a)}, {x * 1.04f, 12.2f * std::cos(b), 12.2f * std::sin(b)}, 0.06f, c.panneau);
        }
        // rayons : câbles tendus du moyeu à la jante
        for (int k = 0; k < 24; k++) {
            const float a = 2 * PI * k / 24 + (x < 0 ? 0.f : PI / 24);
            ligne(m, {x * 1.25f, 0.6f * std::cos(a), 0.6f * std::sin(a)}, {x, 11.4f * std::cos(a), 11.4f * std::sin(a)}, 0.04f, c.acier);
        }
    }
    for (int k = 0; k < 24; k++) { const float a = 2 * PI * k / 24; tube(m, {-1.2f, 12.f * std::cos(a), 12.f * std::sin(a)}, {1.2f, 12.f * std::cos(a), 12.f * std::sin(a)}, 0.06f, 4, c.acier); }
    revolution(m, {0, 0, 0}, AX, {{-1.6f, 0.f}, {-1.6f, 0.45f}, {-1.3f, 0.65f}, {1.3f, 0.65f}, {1.6f, 0.45f}, {1.6f, 0.f}}, 12, c.acier);   // moyeu
    return m;
}

Maille nacelle(const Couleurs& c, int couleur) {
    Maille m;
    // bras d'accroche, toit galbé, vitrage, caisse peinte, plancher
    tube(m, {-0.3f, 0, 0}, {0.3f, 0, 0}, 0.05f, 6, c.acier);
    tube(m, {0, 0, 0}, {0, -0.55f, 0}, 0.05f, 6, c.acier);
    auto anneau = [&](float y, float hx, float hz, float rr) {
        std::vector<vec3> r;
        auto coin = [&](float cx, float cz, float a0) { for (int k = 0; k <= 3; k++) { const float a = a0 + PI / 2 * k / 3; r.push_back({cx + std::cos(a) * rr, y, cz + std::sin(a) * rr}); } };
        coin(hx - rr, hz - rr, 0); coin(-hx + rr, hz - rr, PI / 2); coin(-hx + rr, -hz + rr, PI); coin(hx - rr, -hz + rr, 1.5f * PI);
        return r;
    };
    std::vector<std::vector<vec3>> toit{anneau(-0.55f, 0.35f, 0.3f, 0.15f), anneau(-0.68f, 0.85f, 0.75f, 0.3f), anneau(-0.75f, 0.95f, 0.85f, 0.32f)};
    loft(m, toit, couleur, true, {0, -0.4f, 0}, {0, -0.8f, 0});
    eventail(m, toit.front(), couleur, HAUT);
    std::vector<std::vector<vec3>> vitre{anneau(-0.75f, 0.9f, 0.8f, 0.3f), anneau(-1.45f, 0.9f, 0.8f, 0.3f)};
    loft(m, vitre, c.verre, true, {0, 0, 0}, {0, -2, 0});
    std::vector<std::vector<vec3>> caisse{anneau(-1.45f, 0.92f, 0.82f, 0.31f), anneau(-2.0f, 0.88f, 0.78f, 0.29f)};
    loft(m, caisse, couleur, true, {0, 0, 0}, {0, -2, 0});
    eventail(m, caisse.back(), couleur, -HAUT);
    for (int k = 0; k < 4; k++) { const float a = PI / 4 + k * PI / 2; tube(m, {std::cos(a) * 0.82f, -0.75f, std::sin(a) * 0.72f}, {std::cos(a) * 0.82f, -1.45f, std::sin(a) * 0.72f}, 0.03f, 4, c.alu); }
    return m;
}

Maille piedsRoue(const Couleurs& c) {
    Maille m;
    const float yA = 14.5f;
    for (float x : {-1.6f, 1.6f}) {
        for (float z : {-7.f, 7.f}) {
            revolution(m, {x, 0.6f, z}, vec3{0, yA - 0.6f, -z}, {{0, 0.3f}, {length(vec3{0, yA - 0.6f, -z}) - 0.3f, 0.2f}}, 10, c.acier);
            extrudeZ(m, {{x - 0.6f, 0.6f}, {x + 0.6f, 0.6f}, {x + 0.6f, 1.0f}, {x - 0.6f, 1.0f}}, z - 0.6f, z + 0.6f, c.beton, true);   // massif du pied
        }
        tube(m, {x, 6.f, -7.f * (1 - 5.4f / 13.9f)}, {x, 6.f, 7.f * (1 - 5.4f / 13.9f)}, 0.15f, 8, c.acier);   // entretoise
        revolution(m, {x, yA, 0}, AX, {{-0.35f, 0.f}, {-0.35f, 0.55f}, {0.35f, 0.55f}, {0.35f, 0.f}}, 12, c.acier);   // palier
    }
    // socle de béton (6 × 18 m, chanfreiné) et escalier d'accès
    extrudeZ(m, {{-3.f, 0}, {3.f, 0}, {3.f, 0.55f}, {2.95f, 0.6f}, {-2.95f, 0.6f}, {-3.f, 0.55f}}, -9.f, 9.f, c.beton, true);
    for (int k = 0; k < 3; k++) extrudeZ(m, {{-3.f - 0.3f * (k + 1), 0}, {-3.f - 0.3f * k, 0}, {-3.f - 0.3f * k, 0.6f - 0.2f * k}, {-3.f - 0.3f * (k + 1), 0.6f - 0.2f * k}}, -1.5f, 1.5f, c.beton, true);
    return m;
}

// ---------------------------------------------------------------- tente dôme
Maille tente(const Couleurs& c, int variante) {
    Maille m;
    const float a = TENTE_W / 2 * (1 + 0.04f * (variante % 3)), b = TENTE_L / 2, H = TENTE_H;
    const int nr = 6, np = 20;
    std::vector<std::vector<vec3>> an;
    for (int i = 0; i < nr; i++) {
        const float t = (float)i / (nr - 1) * 0.97f, ech = std::pow(std::cos(t * PI / 2), 0.55f), y = H * std::sin(t * PI / 2);
        std::vector<vec3> r;
        for (int k = 0; k < np; k++) {
            const float ang = 2 * PI * k / np, ca = std::cos(ang), sa = std::sin(ang);
            // toile tendue sur les arceaux en diagonale (angles de 45°), creusée entre eux
            const float creux = 1.f - 0.15f * std::pow(std::fabs(std::cos(2 * ang)), 1.2f) * std::sin(PI * t);   // creux nul au sol (tapis) et au sommet
            const float e = 2.f / 3.f;
            r.push_back({a * ech * creux * (ca < 0 ? -1.f : 1.f) * std::pow(std::fabs(ca), e), y, b * ech * creux * (sa < 0 ? -1.f : 1.f) * std::pow(std::fabs(sa), e)});
        }
        an.push_back(r);
    }
    loft(m, an, COUL_VAR, true, {0, 0, 0}, {0, H * 2, 0});
    eventail(m, an.back(), COUL_VAR, HAUT);
    {   // tapis de sol « baignoire » (bâche grise relevée de 10 cm au pied de la toile)
        std::vector<std::vector<vec3>> tapis{an[0], an[0]};
        for (vec3& q : tapis[0]) q = q * 1.004f;
        for (vec3& q : tapis[1]) { q = q * 1.004f; q.y = 0.1f; }
        loft(m, tapis, c.aluSombre, true, {0, 0, 0}, {0, H * 2, 0});
    }
    {   // avancée (abside) côté porte : toile tendue par un arceau, de la porte au sol à 0,7 m devant
        std::vector<std::vector<vec3>> av;
        for (int i = 0; i < 5; i++) {
            const float t = (float)i / 4, ang = PI * t;   // arc de la porte, de z = −b · 0,6 à + b · 0,6
            std::vector<vec3> r;
            for (int k = 0; k <= 4; k++) {
                const float u = (float)k / 4;   // 0 : contre la tente (sur le dôme), 1 : piquet au sol devant la porte
                const float y = H * 0.82f * std::sin(ang) * (1 - u * u), x = -a * 0.92f * std::cos(std::asin(std::min(1.f, y / H))) - 0.7f * u;
                r.push_back({x, y, -b * 0.6f * std::cos(ang) * (1 - 0.3f * u)});
            }
            av.push_back(r);
        }
        loft(m, av, COUL_VAR, false, {0, 0, 0}, {-3, 0, 0});
    }
    // porte (fermeture à glissière : arc sombre) côté −x, arceaux, haubans et sardines
    const int kp = np / 2;
    for (int i = 0; i + 1 < nr - 2; i++) ligne(m, an[i][kp] - AX * 0.01f, an[i + 1][kp] - AX * 0.01f, 0.012f, c.aluSombre);
    for (int k : {np / 8, 3 * np / 8, 5 * np / 8, 7 * np / 8})
        for (int i = 0; i + 1 < nr; i++) ligne(m, an[i][k] * 1.006f, an[i + 1][k] * 1.006f, 0.014f, c.aluSombre);
    for (int k : {0, np / 4, np / 2, 3 * np / 4}) {
        const vec3 p = an[2][k], sol = vec3{p.x * 1.6f, 0.02f, p.z * 1.6f};
        ligne(m, p, sol, 0.006f, c.panneau);
        tube(m, sol, sol + HAUT * 0.06f, 0.006f, 4, c.alu);
    }
    return m;
}

// ---------------------------------------------------------------- grillage à maille losangée
Maille grillage(const Couleurs& c, bool loin) {
    Maille m;
    revolution(m, {0, 0, 0}, HAUT, {{0, 0.024f}, {GRILLAGE_H, 0.024f}, {GRILLAGE_H + 0.015f, 0.028f}, {GRILLAGE_H + 0.03f, 0.f}}, 6, c.acier);   // poteau et chapeau
    for (float y : {0.3f, 1.0f, GRILLAGE_H - 0.1f}) ligne(m, {0.f, y, 0.f}, {0.f, y, GRILLAGE_PAS}, 0.005f, c.acier);
    if (!loin) {
        const float y0 = 0.05f, H = GRILLAGE_H - 0.15f, pas = 0.25f;
        for (float s = -H; s < GRILLAGE_PAS; s += pas) for (int sens : {1, -1}) {
            const float t0 = std::max(0.f, -s), t1 = std::min(H, GRILLAGE_PAS - s);
            if (t1 <= t0) continue;
            ligne(m, {0.005f, sens > 0 ? y0 + t0 : y0 + H - t0, s + t0}, {0.005f, sens > 0 ? y0 + t1 : y0 + H - t1, s + t1}, 0.003f, c.acier);
        }
    }
    return m;
}

// ---------------------------------------------------------------- oiseaux (vol en V)
Maille oiseau(float battement, int C) {
    Maille m;
    revolution(m, {0, 0, 0.14f}, -AZ, {{0, 0.f}, {0.03f, 0.022f}, {0.12f, 0.035f}, {0.22f, 0.025f}, {0.28f, 0.f}}, 6, C);   // corps, bec vers −z
    for (float s : {-1.f, 1.f}) {
        const float d1 = 0.25f * battement, d2 = 0.55f * battement;   // dièdre de l'aile intérieure et du fouet
        const vec3 r0{s * 0.025f, 0.01f, -0.05f}, r1{s * 0.025f, 0.01f, 0.06f};
        const vec3 m0{s * 0.17f, 0.17f * d1, -0.03f}, m1{s * 0.17f, 0.17f * d1, 0.07f};
        const vec3 t0{s * 0.33f, 0.17f * d1 + 0.16f * d2, 0.04f}, t1{s * 0.33f, 0.17f * d1 + 0.16f * d2, 0.08f};
        quadPlat(m, r0, m0, m1, r1, C, HAUT); quadPlat(m, m0, t0, t1, m1, C, HAUT);
    }
    quadPlat(m, {-0.02f, 0, 0.13f}, {0.02f, 0, 0.13f}, {0.05f, 0, 0.22f}, {-0.05f, 0, 0.22f}, C, HAUT);   // queue
    return m;
}

Maille vol(int nombre, int couleur) {
    Maille m;
    for (int i = 0; i < nombre; i++) {
        const vec3 o{(i % 2 ? 1.f : -1.f) * (i + 1) / 2 * 3.f, (float)(i % 3) * 0.6f, (float)((i + 1) / 2) * 2.5f};
        const Maille o2 = oiseau(std::sin(i * 1.9f), couleur);
        poser(m, o2, o, AX, AZ);
    }
    return m;
}

// ---------------------------------------------------------------- projecteur de circuit (mât et rampe de 6 projecteurs)
Maille projecteur(const Couleurs& c, Maille& neon, bool loin) {
    Maille m;
    revolution(m, {0, 0, 0}, HAUT, {{0, 0.225f}, {PROJ_MAT, 0.1f}}, loin ? 6 : 8, c.acier);
    platine(m, 0, 0, 0.7f, c.acier, !loin);
    extrudeZ(m, {{-0.5f, -0.05f}, {0.5f, -0.05f}, {0.5f, 0.0f}, {-0.5f, 0.0f}}, -0.5f, 0.5f, c.beton, true);
    // cadre : deux traverses en profilé I le long de la route, montants, console vers la piste
    for (float y : {18.0f, 18.9f}) profileI(m, {-0.55f, y, -1.6f}, {-0.55f, y, 1.6f}, AX, 0.12f, 0.064f, 0.0044f, 0.0063f, c.acier);
    for (float z : {-1.6f, 1.6f}) poutre(m, {-0.55f, 17.9f, z}, {-0.55f, 19.0f, z}, AX, 0.08f, 0.08f, 0.f, c.acier);
    for (float y : {18.0f, 18.9f}) tube(m, {0, y, 0}, {-0.55f, y, 0}, 0.06f, 6, c.acier);
    // projecteurs : boîtier à arêtes arrondies incliné de 15° vers le bas, lyre, verre (lumineux la nuit)
    const float inc = 15.f * PI / 180.f;
    const vec3 av{-std::cos(inc), -std::sin(inc), 0}, ht{-std::sin(inc), std::cos(inc), 0};   // avant (vers la piste), haut du boîtier
    for (float y : {18.05f, 18.85f}) for (float z : {-1.05f, 0.f, 1.05f}) {
        const vec3 o{-0.85f, y, z};
        poutre(m, o - av * 0.12f, o + av * 0.12f, ht, 0.55f, 0.45f, 0.04f, c.aluSombre);
        if (!loin) {
            poutre(m, o + av * 0.12f + ht * 0.25f, o + av * 0.3f + ht * 0.2f, ht, 0.55f, 0.01f, 0.f, c.aluSombre);   // visière
            for (float s : {-1.f, 1.f}) tube(m, {-0.55f, y, z + s * 0.3f}, o + vec3{0, 0, s * 0.3f}, 0.015f, 4, c.acier);   // lyre
        }
        const vec3 f = o + av * 0.125f, u = ht * 0.2f, w{0, 0, 0.25f};
        quadPlat(neon, f - u - w, f + u - w, f + u + w, f - u + w, c.panneau, av);
    }
    return m;
}

} // namespace grandprix
} // namespace wfs
