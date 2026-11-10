// WaForSpeed — volant de la vue Cockpit (WFS-414) : voir vehicule_cockpit_volant.h.
#include "vehicule_cockpit_volant.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "vehicule_cockpit_formes.h"

namespace wfs::cockpit {

using namespace formes;

namespace {

// volant commun (WFS-197 à WFS-323), déplacé de vehicule_cockpit.cpp par WFS-414 sans aucun changement
void volantCommun(const InterieurDef& d, PiecesVolant& p) {
    const mat4 I = mat4::identity();
    const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
    jante(p.jante, I, R, rt, d.meplat, 0, 2 * PI, 96, 14, d.meplatHaut);
    dalleFace(p.moyeu, translate({0, 0, -0.012f}), 0.15f, 0.115f, 0.05f, 0.045f, 0.018f, 0.85f);   // moyeu : coussin du sac gonflable, galbé (WFS-323)
    // branches : 3 (à 9 h, 3 h et 6 h) ou 2 (à 9 h et 3 h, SF90 : branches larges qui portent les commandes)
    const float hub = 0.07f;
    // WFS-202 : 4 branches (Range Rover Sport) : deux presque horizontales, deux en V vers le bas
    std::vector<float> angles = d.branches == 2 ? std::vector<float>{0.f, PI}
                              : d.branches == 4 ? std::vector<float>{0.10f, PI - 0.10f, -PI / 2 + 0.45f, -PI / 2 - 0.45f}
                                                : std::vector<float>{0.f, PI, -PI / 2};
    for (float a : angles) {
        const vec2 bout = ligneJante(a, R, d.meplat, d.meplatHaut);
        const float l = std::sqrt(bout.x * bout.x + bout.y * bout.y) - hub;
        const float large = d.branches == 2 ? 0.060f : d.branches == 4 ? (a < 0 ? 0.026f : 0.036f) : (a == -PI / 2 ? 0.032f : 0.040f);
        profile(p.branches, rotateZ(a), {hub, 0, -0.008f}, {hub + l, 0, -0.008f}, large, 0.016f, 0.007f, -1, large * 0.75f);   // WFS-323 : branche profilée
    }
    // coutures colorées en bas de la jante (M4 : bleu clair, bleu foncé, rouge), de part et d'autre de la branche basse
    const bool coutures = d.couture[0] || d.couture[1] || d.couture[2];
    for (int k = 0; k < 3; k++) if (d.couture[k])
        for (float cote : {-1.f, 1.f}) {
            const float a = -PI / 2 + cote * (0.11f + 0.05f * k);
            jante(p.coutures[k], translate({0, 0, 0.0005f}), R, rt * 1.03f, d.meplat, a - 0.024f, a + 0.024f, 3, 10);
        }
    // repère : M4 (boutons M1 / M2 rouges sur les branches), Golf (agrafe GTI rouge sur la branche basse), SF90 (rouge à midi)
    if (d.repereCol) {
        if (coutures || d.boutonsBranches) for (float sx : {-1.f, 1.f}) dalleFace(p.repere, translate({sx * 0.105f, 0.018f, 0.012f}), 0.022f, 0.014f, 0.008f, 0.005f, 0.002f);
        else if (d.manettino || d.repereHaut) dalleFace(p.repere, translate({0, R + rt * 0.2f, 0.004f}), 0.012f, rt * 2.1f, rt * 2.1f, 0.005f, 0.003f);
        else if (!d.demarreur) dalleFace(p.repere, translate({0, -R * 0.55f, 0.010f}), 0.045f, 0.012f, 0.004f, 0.005f, 0.0015f);   // Giulia : le rouge est le démarreur
    }
    if (d.palettes) for (float sx : {-1.f, 1.f}) dalleFace(p.palettes, translate({sx * (R * 0.80f), 0.02f, -0.045f}) * rotateZ(sx * 0.15f), 0.055f, 0.13f, 0.006f, 0.02f, 0.002f, 0.95f);
    // manettino en bas à droite, démarreur en bas à gauche (sur les branches) ; WFS-201 : démarreur seul (Giulia)
    if (d.manettino) dalleFace(p.manettino, translate({0.085f, -0.055f, 0.012f}), 0.044f, 0.044f, 0.016f, 0.022f, 0.005f, 0.9f);   // WFS-323 : boutons chanfreinés
    if (d.demarreur) dalleFace(p.manettino, translate({-0.085f, -0.055f, 0.010f}), 0.032f, 0.032f, 0.012f, 0.016f, 0.004f, 0.9f);
    // WFS-201 : molettes (Model 3) : deux roulettes de commande sur les branches horizontales, caoutchouc sombre
    if (d.molettes) for (float sx : {-1.f, 1.f}) dalle(p.molettes, translate({sx * 0.105f, 0.f, 0.014f}) * rotateZ(PI / 2), 0.022f, 0.022f, 0.010f, 0.011f, 0.003f);
}

// ---------- WFS-414 : volant de série ----------
// contour du coussin, agrandi de `marge` (m) : super-ellipse |x/a|^n + |y/b|^n = 1 (coins arrondis d'un coussin de sac gonflable),
// moitié basse rétrécie (trapèze, même loi que demiLargeurCoussin) ; N points, sens trigonométrique vu du conducteur (+z)
std::vector<vec2> contourCoussin(const VolantSerie& s, float marge, int N) {
    std::vector<vec2> c((size_t)N);
    const float a = s.moyeuW * 0.5f + marge, b = s.moyeuH * 0.5f + marge, e = 2.f / s.forme;
    for (int k = 0; k < N; k++) {
        const float th = 2 * PI * k / N, cx = std::cos(th), sy = std::sin(th);
        const float x = a * std::copysign(std::pow(std::fabs(cx), e), cx), y = b * std::copysign(std::pow(std::fabs(sy), e), sy);
        const float t = std::clamp(-y / b, 0.f, 1.f);   // trapèze : la moitié basse seulement (la plus grande largeur reste moyeuW)
        c[(size_t)k] = {x * (1 - s.trapeze * t), y};
    }
    return c;
}
// pièce moulée de révolution autour de son contour : anneaux successifs (contour à l'échelle k autour du centre, à la hauteur z),
// du point du milieu de l'arrière (pôle) au point du milieu de l'avant ; un anneau sans pôle laisse la pièce ouverte (joint).
// Faces orientées vers l'extérieur (contour dans le sens trigonométrique, anneaux de l'arrière vers l'avant) ; normales recalculées
// par la moyenne des faces voisines : surface lisse, sans facette (la pièce est modelée, pas une forme posée telle quelle)
struct Anneau { const std::vector<vec2>* c; float k, z; };
void moule(Geo& g, const std::vector<Anneau>& an, const float* poleArriere, const float* poleAvant) {
    const uint32_t v0 = (uint32_t)g.v.size(), i0 = (uint32_t)g.i.size();
    const uint32_t N = (uint32_t)an[0].c->size();
    const mat4 I = mat4::identity();
    auto sommet = [&](vec3 q) { ajouterUV(g, I, q, {0, 0, 1}, q.x * GRAIN, q.y * GRAIN); };
    if (poleArriere) sommet({0, 0, *poleArriere});
    const uint32_t r0 = (uint32_t)g.v.size();
    for (const Anneau& a : an) for (const vec2& p : *a.c) sommet({p.x * a.k, p.y * a.k, a.z});
    const uint32_t nA = (uint32_t)an.size();
    if (poleAvant) sommet({0, 0, *poleAvant});
    auto idx = [&](uint32_t anneau, uint32_t k) { return r0 + anneau * N + k % N; };
    if (poleArriere) for (uint32_t k = 0; k < N; k++) g.i.insert(g.i.end(), {v0, idx(0, k + 1), idx(0, k)});
    for (uint32_t a = 0; a + 1 < nA; a++) for (uint32_t k = 0; k < N; k++) g.quad(idx(a, k), idx(a, k + 1), idx(a + 1, k + 1), idx(a + 1, k));
    if (poleAvant) { const uint32_t c = (uint32_t)g.v.size() - 1; for (uint32_t k = 0; k < N; k++) g.i.insert(g.i.end(), {idx(nA - 1, k), idx(nA - 1, k + 1), c}); }
    // normales : somme des normales des faces (pondérées par leur aire), puis normalisées
    std::vector<vec3> n(g.v.size() - v0, vec3{0, 0, 0});
    for (size_t t = i0; t + 2 < g.i.size(); t += 3) {
        const Vertex &A = g.v[g.i[t]], &B = g.v[g.i[t + 1]], &C = g.v[g.i[t + 2]];
        const vec3 f = cross(vec3{B.px - A.px, B.py - A.py, B.pz - A.pz}, vec3{C.px - A.px, C.py - A.py, C.pz - A.pz});
        for (int j = 0; j < 3; j++) n[g.i[t + j] - v0] = n[g.i[t + j] - v0] + f;
    }
    for (size_t k = 0; k < n.size(); k++) {
        const vec3 m = length(n[k]) > 1e-12f ? normalize(n[k]) : vec3{0, 0, 1};
        Vertex& v = g.v[v0 + k]; v.nx = m.x; v.ny = m.y; v.nz = m.z;
    }
}

// cotes calculées d'une branche (repère de la branche : x le long de la branche, depuis le centre ; z vers le conducteur)
struct Branche { float a, r0, r1, z0, z1, w0, w1; };   // w0, w1 : largeur aux deux bouts (enfouis dans la coque et dans la jante)
float largeurBranche(const Branche& b, float x) { return b.w0 + (b.w1 - b.w0) * std::clamp((x - b.r0) / (b.r1 - b.r0), 0.f, 1.f); }
float faceBranche(const Branche& b, float x) { return b.z0 + (b.z1 - b.z0) * std::clamp((x - b.r0) / (b.r1 - b.r0), 0.f, 1.f) + BRANCHE_T * 0.5f; }
// repère posé sur la face avant de la branche à la distance x du centre (z = surface, normale de la face), enfoncé de `dans`
mat4 surBranche(const Branche& b, float x, float dans) {
    const float pente = std::atan2(b.z1 - b.z0, b.r1 - b.r0);
    return rotateZ(b.a) * translate({x, 0, faceBranche(b, x)}) * rotateY(-pente) * translate({0, 0, -dans});
}

void volantSerie(const InterieurDef& d, const VolantSerie& s, PiecesVolant& p) {
    const mat4 I = mat4::identity();
    const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
    // jante : la même qu'avant (RG-04 : diamètres et méplats des fiches)
    jante(p.jante, I, R, rt, d.meplat, 0, 2 * PI, 96, 14, d.meplatHaut);
    // RG-01 : coussin du sac gonflable bombé, bord arrondi de 8 mm, enchâssé dans la coque ; joint sombre tout autour
    {
        const int N = 72;
        const std::vector<vec2> c = contourCoussin(s, 0, N);
        const float zF = COUSSIN_Z_BORD, zA = zF + COUSSIN_GALBE - COUSSIN_E, rb = 0.008f, kb = rb / (s.moyeuH * 0.5f);
        std::vector<Anneau> an{{&c, 0.96f, zA}, {&c, 1.f, zA + 0.004f}, {&c, 1.f, zF - rb}};
        for (const float deg : {30.f, 55.f, 78.f}) {   // arrondi du bord (quart de cercle jusqu'à la pente du galbe)
            const float ph = deg * PI / 180.f;
            an.push_back({&c, 1 - kb * (1 - std::cos(ph)), zF - rb + rb * std::sin(ph)});
        }
        const float k0 = an.back().k, z0 = an.back().z;
        for (const float t : {0.86f, 0.72f, 0.58f, 0.44f, 0.30f, 0.16f})   // galbe : z = bord + galbe × (1 − t²)
            an.push_back({&c, k0 * t, z0 + COUSSIN_GALBE * (1 - t * t)});
        const float milieu = z0 + COUSSIN_GALBE;
        moule(p.moyeu, an, &zA, &milieu);
        // joint : bande sombre de 4 mm entre le coussin et la coque, en léger creux
        const std::vector<vec2> j0 = contourCoussin(s, -0.0005f, N), j1 = contourCoussin(s, JOINT_L, N), j2 = contourCoussin(s, JOINT_L + 0.001f, N);
        // (anneaux de l'extérieur vers l'intérieur : faces tournées vers le conducteur)
        moule(p.joint, {{&j2, 1.f, COQUE_Z_AVANT - 0.002f}, {&j1, 1.f, COQUE_Z_AVANT + 0.0012f}, {&c, 1.f, COQUE_Z_AVANT + 0.0012f},
                        {&j0, 1.f, COQUE_Z_AVANT - 0.002f}}, nullptr, nullptr);
        // coque du moyeu : dépasse le coussin de 8 mm, arête avant chanfreinée de 3 mm, dépouille à l'arrière ; les branches y naissent
        const std::vector<vec2> q = contourCoussin(s, COQUE_BORD, N);
        const float kq = 1 - 0.003f / (s.moyeuH * 0.5f + COQUE_BORD);
        const float zq0 = COQUE_Z_ARRIERE, zq1 = COQUE_Z_AVANT;
        moule(p.coque, {{&q, 0.95f, zq0}, {&q, 1.f, zq0 + 0.004f}, {&q, 1.f, zq1 - 0.003f}, {&q, kq, zq1}}, &zq0, &zq1);
    }
    // RG-02 : branches à 3 h, 9 h et 6 h : elles naissent 12 mm dans le coussin (dans la coque) et entrent dans la jante (bout à
    // 0,3 × la demi-épaisseur au-delà de la ligne médiane : dans le tube) ; 55 mm de large où elles sortent de la coque, 35 mm où
    // elles rejoignent la jante (largeur linéaire entre les deux, prolongée dans les parties enfouies)
    std::vector<Branche> br;
    for (const float a : {0.f, PI, -PI / 2}) {
        const vec2 bout = ligneJante(a, R, d.meplat, d.meplatHaut);
        const float Rl = std::sqrt(bout.x * bout.x + bout.y * bout.y), bord = a == -PI / 2 ? s.moyeuH * 0.5f : demiLargeurCoussin(s, 0);
        const float r0 = bord - BRANCHE_DANS, r1 = Rl + 0.3f * rt, xa = bord + COQUE_BORD, xb = Rl - rt;
        const float k = (BRANCHE_W1 - BRANCHE_W0) / (xb - xa);
        br.push_back({a, r0, r1, BRANCHE_Z0, BRANCHE_Z1, BRANCHE_W0 + k * (r0 - xa), BRANCHE_W0 + k * (r1 - xa)});
        const Branche& b = br.back();
        profile(p.branches, rotateZ(a), {b.r0, 0, b.z0}, {b.r1, 0, b.z1}, b.w0, BRANCHE_T, 0.006f, -1, b.w1);
    }
    // RG-03 (adaptée par le PO, 2026-11-10) : plaque de commandes de ≈ 40 × 40 mm sur chaque branche horizontale, entre la coque et
    // la jante (34 à 42 mm le long de la branche, jamais plus large qu'elle), 2 × 2 touches en relief, sans logo ni marque ;
    // Giulia : le démarreur rouge prend la place de la touche du bas, côté extérieur, de la plaque gauche
    for (int k = 0; k < 2; k++) {
        const Branche& b = br[(size_t)k];
        // de la coque jusqu'au cuir de la jante, à la hauteur du dessus de la plaque (la plaque bute contre la jante, sans jour)
        const float e = 0.004f, dans = 0.0012f, xa = demiLargeurCoussin(s, 0) + COQUE_BORD;
        const float zh = faceBranche(b, R - rt) + e - dans, xb = R - std::sqrt(std::max(0.f, rt * rt - zh * zh));
        const float L = std::min(0.042f, xb - xa), xc = xa + L * 0.5f;
        const float H = std::min(0.040f, largeurBranche(b, xa + L) - 0.002f);   // jamais plus large que la branche, même au bout fin
        const mat4 P = surBranche(b, xc, dans) * translate({0, 0, e * 0.5f});
        dalleFace(p.plaques, P, L, H, e, 0.006f, 0.0012f);
        const float g = 0.0045f, kw = (L - 3 * g) * 0.5f, kh = (H - 3 * g) * 0.5f, ke = 0.0025f;
        for (const float sx : {-1.f, 1.f}) for (const float sy : {-1.f, 1.f}) {
            const mat4 T = P * translate({sx * (kw + g) * 0.5f, sy * (kh + g) * 0.5f, e * 0.5f + ke * 0.5f - 0.0003f});
            // plaque gauche (a = π) : x du repère vers l'extérieur, y vers le bas : (+, +) = touche du bas, côté extérieur
            if (d.demarreur && b.a == PI && sx > 0 && sy > 0) {
                const float ds = std::min(kw, kh) + 0.002f;
                const mat4 B = P * translate({sx * (kw + g) * 0.5f, sy * (kh + g) * 0.5f, e * 0.5f});
                dalleFace(p.manettino, B * translate({0, 0, 0.0022f}), ds, ds, 0.005f, ds * 0.5f - 1e-4f, 0.0015f, 0.9f);   // bouton bombé
                cadreArrondi(p.plaques, B * translate({0, 0, 0.0018f}), ds + 0.004f, ds + 0.004f, (ds + 0.004f) * 0.5f, 0.0024f, 0.002f);   // bague
                continue;
            }
            dalleFace(p.touches, T, kw, kh, ke, 0.0025f, 0.0007f);
        }
    }
    // coutures colorées en bas de la jante (aucune voiture de série n'en a : même code que le volant commun)
    for (int k = 0; k < 3; k++) if (d.couture[k])
        for (float cote : {-1.f, 1.f}) {
            const float a = -PI / 2 + cote * (0.11f + 0.05f * k);
            jante(p.coutures[k], translate({0, 0, 0.0005f}), R, rt * 1.03f, d.meplat, a - 0.024f, a + 0.024f, 3, 10);
        }
    // agrafe rouge sur la branche basse (Golf, sans logo), juste sous la coque, jamais plus large que la face plate de la branche
    if (d.repereCol && !d.demarreur) {
        const Branche& b = br[2];
        const float x = s.moyeuH * 0.5f + COQUE_BORD + 0.012f, l = std::min(0.040f, largeurBranche(b, x) - 0.012f), e = 0.003f;
        dalleFace(p.repere, surBranche(b, x, 0.001f) * translate({0, 0, e * 0.5f}), 0.011f, l, e, 0.004f, 0.001f);
    }
    // palettes : les mêmes qu'avant (RG-04)
    if (d.palettes) for (float sx : {-1.f, 1.f}) dalleFace(p.palettes, translate({sx * (R * 0.80f), 0.02f, -0.045f}) * rotateZ(sx * 0.15f), 0.055f, 0.13f, 0.006f, 0.02f, 0.002f, 0.95f);
}

} // namespace

float demiLargeurCoussin(const VolantSerie& s, float y) {
    const float b = s.moyeuH * 0.5f, u = std::clamp(std::fabs(y) / b, 0.f, 1.f);
    const float t = std::clamp(-y / b, 0.f, 1.f);   // trapèze : 0 sur la moitié haute, 1 en bas
    return s.moyeuW * 0.5f * std::pow(1 - std::pow(u, s.forme), 1.f / s.forme) * (1 - s.trapeze * t);
}

void construireVolant(const InterieurDef& d, PiecesVolant& p) {
    if (const VolantSerie* s = volantSeriePour(d)) volantSerie(d, *s, p);
    else volantCommun(d, p);
}

Visiere ligneVisiere(const InterieurDef& d) {
    // casquette (visière) du combiné : avance de la lèvre vers le conducteur au-dessus de l'écran (WFS-323 : du pied du pare-brise
    // jusqu'à 7,5 cm devant l'écran, déplacé de vehicule_cockpit.cpp par WFS-414 ; RG-07 : avancée propre au volant de série)
    const float cw = d.combineW * 0.5f + 0.03f, top = d.combinePos.y + d.combineH * 0.5f + 0.02f;
    const float ev = 0.024f, cz = d.combinePos.z, cxv = d.combinePos.x, Yb = d.pareBriseY;
    Visiere v;
    v.ev = ev; v.largeur = 2 * cw;
    const VolantSerie* vs = volantSeriePour(d);   // RG-07 : visière moins avancée, derrière le haut de la jante (volant de série)
    const float avance = vs ? vs->visiereAvance : 0.068f, milieu = vs ? vs->visiereAvance - 0.028f : 0.04f;
    v.pts = {vec3{cxv, Yb - ev * 0.5f, cz - 0.10f}, vec3{cxv, top + 0.015f - ev * 0.5f, cz - 0.03f}, vec3{cxv, top + 0.012f - ev * 0.5f, cz + milieu},
             vec3{cxv, top + 0.004f - ev * 0.5f, cz + avance}};
    return v;
}

} // namespace wfs::cockpit
