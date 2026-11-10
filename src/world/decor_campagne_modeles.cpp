// WaForSpeed — WFS-372 : objets agricoles de la Campagne, modélisés (voir decor_campagne_modeles.h).
#include "decor_campagne_modeles.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace wfs {
namespace campagne {

using marina::eventail;
using marina::ligne;
using marina::loft;
using marina::Sommet;

namespace {

const vec3 HAUT{0, 1, 0};

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
std::vector<vec3> cercle(vec3 c, vec3 e1, vec3 e2, float r1, float r2, int n, float phase = 0) {
    std::vector<vec3> r;
    for (int k = 0; k < n; k++) { const float a = phase + 2 * PI * (float)k / (float)n; r.push_back(c + e1 * (std::cos(a) * r1) + e2 * (std::sin(a) * r2)); }
    return r;
}
// pièce de révolution d'axe ax (unitaire) passant par o : profil (abscisse le long de l'axe, rayon), n côtés ; bouts fermés
// quand le rayon du premier / dernier point n'est pas nul
void revolution(Maille& m, vec3 o, vec3 ax, const std::vector<std::pair<float, float>>& prof, int n, int couleur) {
    vec3 e1 = std::fabs(ax.y) < 0.9f ? normalize(cross(ax, HAUT)) : normalize(cross(ax, vec3{1, 0, 0}));
    const vec3 e2 = cross(ax, e1);
    std::vector<std::vector<vec3>> an;
    for (auto [x, r] : prof) an.push_back(cercle(o + ax * x, e1, e2, std::max(r, 1e-4f), std::max(r, 1e-4f), n));
    loft(m, an, couleur, true, o + ax * prof.front().first, o + ax * prof.back().first);
    if (prof.front().second > 1e-3f) eventail(m, an.front(), couleur, -ax);
    if (prof.back().second > 1e-3f) eventail(m, an.back(), couleur, ax);
}
// repère d'une pièce le long de a → b : axe, « haut » orthogonalisé, côté
void repere(vec3 a, vec3 b, vec3 haut, vec3& ax, vec3& hv, vec3& sd) {
    ax = normalize(b - a);
    hv = haut - ax * dot(haut, ax);
    if (length(hv) < 1e-4f) hv = std::fabs(ax.y) < 0.9f ? HAUT - ax * ax.y : vec3{1, 0, 0};
    hv = normalize(hv);
    sd = cross(ax, hv);
}
// prisme de section quelconque (points dans le repère (côté, haut), sens trigonométrique) de a à b : faces planes (arêtes
// vives), bouts fermés si demandé
void prisme(Maille& m, vec3 a, vec3 b, vec3 haut, const std::vector<vec2>& sec, int couleur, bool bouts) {
    vec3 ax, hv, sd; repere(a, b, haut, ax, hv, sd);
    const size_t n = sec.size();
    std::vector<vec3> A(n), B(n);
    for (size_t k = 0; k < n; k++) { A[k] = a + sd * sec[k].x + hv * sec[k].y; B[k] = b + sd * sec[k].x + hv * sec[k].y; }
    for (size_t k = 0; k < n; k++) {
        const size_t j = (k + 1) % n;
        // normale sortante de l'arête (section donnée dans le sens trigonométrique) : juste aussi pour une section concave
        const float ex = sec[j].x - sec[k].x, ey = sec[j].y - sec[k].y;
        quadPlat(m, A[k], A[j], B[j], B[k], couleur, sd * ey - hv * ex);
    }
    if (bouts) { eventail(m, A, couleur, -ax); eventail(m, B, couleur, ax); }
}

// ---------------------------------------------------------------- mur percé d'ouvertures, embrasures
struct Ouv { float u0, v0, u1, v1; };   // ouverture dans le repère du mur (u depuis le centre du mur, v depuis le bas)
// mur plan : centre du bas en c, u le long du mur (longueur L), haut HAUT (hauteur H), normale dehors d ; ouvertures percées,
// embrasures (tableaux, appui, linteau) de profondeur prof, de la couleur emb
void murPerce(Maille& m, vec3 c, vec3 u, vec3 d, float L, float H, const std::vector<Ouv>& ouv, int couleur, float prof, int emb) {
    std::vector<float> xs{-L / 2, L / 2}, ys{0, H};
    for (const Ouv& o : ouv) { xs.push_back(o.u0); xs.push_back(o.u1); ys.push_back(o.v0); ys.push_back(o.v1); }
    std::sort(xs.begin(), xs.end()); xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    auto P = [&](float x, float y) { return c + u * x + HAUT * y; };
    // cellules pleines regroupées par bande horizontale (moins de triangles)
    for (size_t j = 0; j + 1 < ys.size(); j++) {
        const float y0 = ys[j], y1 = ys[j + 1], ym = (y0 + y1) / 2;
        if (y1 - y0 < 1e-4f) continue;
        float debut = 1e9f;
        for (size_t i = 0; i + 1 < xs.size(); i++) {
            const float xm = (xs[i] + xs[i + 1]) / 2;
            bool trou = false;
            for (const Ouv& o : ouv) if (xm > o.u0 && xm < o.u1 && ym > o.v0 && ym < o.v1) trou = true;
            if (!trou && debut > 1e8f) debut = xs[i];
            if ((trou || i + 2 == xs.size()) && debut < 1e8f) {
                const float fin = trou ? xs[i] : xs[i + 1];
                if (fin - debut > 1e-4f) quadPlat(m, P(debut, y0), P(fin, y0), P(fin, y1), P(debut, y1), couleur, d);
                debut = 1e9f;
            }
        }
    }
    for (const Ouv& o : ouv) {   // embrasures : du nu du mur vers l'intérieur, tournées vers le centre de l'ouverture
        const vec3 e = d * -prof;
        const vec3 a = P(o.u0, o.v0), b = P(o.u1, o.v0), cc = P(o.u1, o.v1), dd = P(o.u0, o.v1);
        quadPlat(m, a, b, b + e, a + e, emb, HAUT);          // appui
        quadPlat(m, dd, cc, cc + e, dd + e, emb, -HAUT);     // linteau
        quadPlat(m, a, dd, dd + e, a + e, emb, u);           // tableau gauche
        quadPlat(m, b, cc, cc + e, b + e, emb, -u);          // tableau droit
    }
}

// fenêtre (ou porte) posée dans une ouverture : vitrage en retrait, dormant, meneau, appui saillant, volets ouverts contre
// le mur. c = centre du bas de l'ouverture sur le nu du mur ; u le long du mur ; d dehors ; w × h
void fenetre(Maille& m, const Couleurs& k, vec3 c, vec3 u, vec3 d, float w, float h, bool porte, bool loin) {
    const float r = 0.16f;   // retrait du vitrage (tableau de 0,20 m, dormant compris)
    auto P = [&](float x, float y, float z) { return c + u * x + HAUT * y + d * z; };
    if (loin) { quadPlat(m, P(-w / 2, 0, -0.05f), P(w / 2, 0, -0.05f), P(w / 2, h, -0.05f), P(-w / 2, h, -0.05f), porte ? k.porte : k.vitre, d); return; }
    if (porte) {   // porte pleine en bois à lames, dormant, seuil de pierre
        quadPlat(m, P(-w / 2, 0, -r), P(w / 2, 0, -r), P(w / 2, h, -r), P(-w / 2, h, -r), k.porte, d);
        for (int i = 1; i < 5; i++) { const float x = -w / 2 + w * i / 5; prisme(m, P(x, 0.02f, -r + 0.008f), P(x, h - 0.02f, -r + 0.008f), d, {{-0.006f, -0.008f}, {0.006f, -0.008f}, {0.006f, 0.008f}, {-0.006f, 0.008f}}, k.menuiserie, false); }
        poutre(m, P(-w / 2 - 0.1f, -0.06f, -0.05f), P(w / 2 + 0.1f, -0.06f, -0.05f), HAUT, 0.36f, 0.12f, 0.015f, k.pierre);
    } else {
        quadPlat(m, P(-w / 2, 0, -r), P(w / 2, 0, -r), P(w / 2, h, -r), P(-w / 2, h, -r), k.vitre, d);
        // dormant de 6 cm et meneau (fenêtre à deux battants), 5 cm d'épaisseur devant le vitrage
        const float fw = 0.06f;
        for (const auto& [a, b] : {std::pair<vec3, vec3>{P(-w / 2 + fw / 2, 0, -r + 0.025f), P(-w / 2 + fw / 2, h, -r + 0.025f)},
                                   {P(w / 2 - fw / 2, 0, -r + 0.025f), P(w / 2 - fw / 2, h, -r + 0.025f)},
                                   {P(0, 0, -r + 0.025f), P(0, h, -r + 0.025f)}})
            poutre(m, a, b, d, fw, 0.05f, 0, k.menuiserie);
        for (const float y : {fw / 2, h - fw / 2, h * 0.62f})
            poutre(m, P(-w / 2 + fw, y, -r + 0.025f), P(w / 2 - fw, y, -r + 0.025f), d, fw * (y == h * 0.62f ? 0.6f : 1.f), 0.05f, 0, k.menuiserie);
        // appui de pierre saillant de 4 cm, 6 cm d'épaisseur, en pente vers l'extérieur
        poutre(m, P(-w / 2 - 0.06f, -0.03f, -0.08f), P(w / 2 + 0.06f, -0.03f, -0.08f), HAUT, 0.24f, 0.06f, 0.012f, k.pierre);
        // volets battants à lames verticales et barres, ouverts à plat contre le mur, à 3 cm du nu
        for (const int cote : {-1, 1}) {
            const float x0 = cote * (w / 2 + 0.03f), x1 = x0 + cote * (w / 2), xm = (x0 + x1) / 2;
            poutre(m, P(xm, 0.01f, 0.035f), P(xm, h - 0.01f, 0.035f), d, w / 2, 0.03f, 0.004f, k.volet);
            for (const float y : {0.18f, h - 0.18f})
                poutre(m, P(x0, y, 0.06f), P(x1, y, 0.06f), d, 0.11f, 0.022f, 0.004f, k.volet);
        }
    }
}

// tuiles canal : surface ondulée (tuiles de courant et de couvert) entre la ligne d'égout et le faîtage ; z de z0 à z1, onde
// de période per (0,22 m pour la tuile canal), creux de ampl
void ondule(Maille& m, vec3 egoutA, vec3 faitA, vec3 dz, float z0, float z1, float per, float ampl, vec3 normale, int couleur, int pts) {
    std::vector<std::vector<vec3>> an(2);
    const int n = std::max(2, (int)std::ceil((z1 - z0) / per * pts));
    for (int i = 0; i <= n; i++) {
        const float z = z0 + (z1 - z0) * i / n, o = ampl * 0.5f * std::sin(2 * PI * (z - z0) / per);
        an[0].push_back(egoutA + dz * z + normale * o);
        an[1].push_back(faitA + dz * z + normale * o);
    }
    // axe sous la surface : les triangles se tournent vers le haut (normale)
    const vec3 m0 = (egoutA + faitA) * 0.5f - normale * 5.f;
    loft(m, an, couleur, false, m0 + dz * z0, m0 + dz * z1);
}

} // namespace

// ---------------------------------------------------------------- outils publics
void poutre(Maille& m, vec3 a, vec3 b, vec3 haut, float w, float h, float ch, int couleur) {
    const float x = w / 2, y = h / 2, c = std::min(ch, std::min(x, y) * 0.5f);
    if (c <= 1e-4f) prisme(m, a, b, haut, {{-x, -y}, {x, -y}, {x, y}, {-x, y}}, couleur, true);
    else prisme(m, a, b, haut, {{-x + c, -y}, {x - c, -y}, {x, -y + c}, {x, y - c}, {x - c, y}, {-x + c, y}, {-x, y - c}, {-x, -y + c}}, couleur, true);
}

void profileI(Maille& m, vec3 a, vec3 b, vec3 haut, float h, float w, float ame, float aile, int couleur) {
    const float x = w / 2, y = h / 2, e = ame / 2, t = aile;
    // contour en I (12 points) : faces planes ; bouts fermés par trois rectangles (ailes et âme)
    prisme(m, a, b, haut, {{-x, -y}, {x, -y}, {x, -y + t}, {e, -y + t}, {e, y - t}, {x, y - t}, {x, y}, {-x, y}, {-x, y - t}, {-e, y - t}, {-e, -y + t}, {-x, -y + t}},
           couleur, false);
    vec3 ax, hv, sd; repere(a, b, haut, ax, hv, sd);
    for (int bout = 0; bout < 2; bout++) {
        const vec3 o = bout ? b : a, dd = bout ? ax : -ax;
        auto R = [&](float x0, float y0, float x1, float y1) { quadPlat(m, o + sd * x0 + hv * y0, o + sd * x1 + hv * y0, o + sd * x1 + hv * y1, o + sd * x0 + hv * y1, couleur, dd); };
        R(-x, -y, x, -y + t); R(-x, y - t, x, y); R(-e, -y + t, e, y - t);
    }
}

void lisse(Maille& m, vec3 a, vec3 b, vec3 plat, int couleur, bool loin) {
    vec3 ax, hv, sd; repere(a, b, plat, ax, hv, sd);   // hv : vers la face plane (contre les poteaux)
    const float r = LISSE_DIAM / 2;
    const int nc = loin ? 3 : 5;
    std::vector<std::vector<vec3>> an;
    const int nr = 3;
    for (int i = 0; i < nr; i++) {
        const float t = (float)i / (nr - 1);
        const vec3 o = a + (b - a) * t - HAUT * (0.012f * 4 * t * (1 - t));   // flèche de 12 mm au milieu
        std::vector<vec3> sec;
        for (int k = 0; k < nc; k++) { const float g = -PI / 2 + PI * k / (nc - 1); sec.push_back(o - hv * (std::cos(g) * r) + sd * (std::sin(g) * r)); }
        an.push_back(sec);
    }
    loft(m, an, couleur, true, a, b);
    eventail(m, an.front(), couleur, -ax);
    eventail(m, an.back(), couleur, ax);
}

void cable(Maille& m, vec3 a, vec3 b, float fleche, float diam, int n, int couleur) {
    vec3 p0 = a;
    for (int i = 1; i <= n; i++) {
        const float t = (float)i / n;
        const vec3 p = a + (b - a) * t - HAUT * (fleche * 4 * t * (1 - t));
        ligne(m, p0, p, diam, couleur);
        p0 = p;
    }
}

// ---------------------------------------------------------------- vigne
Maille piquetVigne(const Couleurs& c, int variante) {
    Maille m;
    Hasard H{(uint32_t)variante * 7919u + 11u};
    const float r = PIQUET_DIAM / 2;
    const vec3 incl{H.sym(0.025f), 0, H.sym(0.025f)};   // piquet un peu penché (± 1,4°)
    std::vector<std::vector<vec3>> an;
    const float ys[5] = {-0.10f, 0.55f, 1.20f, PIQUET_HAUT - 0.06f, PIQUET_HAUT};
    for (int i = 0; i < 5; i++) {
        const float y = ys[i], rr = i == 4 ? r * 0.55f : r * (1 + H.sym(0.08f));   // tête taillée en biseau (chanfrein)
        an.push_back(cercle(vec3{0, y, 0} + incl * (y / PIQUET_HAUT), {1, 0, 0}, {0, 0, 1}, rr, rr * (0.9f + H() * 0.1f), 6, H() * 1.f));
    }
    loft(m, an, c.piquet, true, {0, -0.1f, 0}, {0, PIQUET_HAUT, 0});
    eventail(m, an.back(), c.piquet, HAUT);
    return m;
}

Maille cep(const Couleurs& c, int variante, bool loin) {
    Maille m;
    Hasard H{(uint32_t)variante * 104729u + 3u};
    const int n = loin ? 4 : 5;
    const int nr = loin ? 2 : 4;
    std::vector<std::vector<vec3>> an;
    vec3 o{0, -0.03f, 0};
    for (int i = 0; i < nr; i++) {   // tronc tortueux : chaque anneau décalé de ± 3 cm
        const float t = (float)i / (nr - 1), y = -0.03f + (CEP_HAUT + 0.03f) * t, rr = (0.034f - 0.006f * t) * (1 + H.sym(0.12f));
        if (i > 0) o = vec3{o.x + H.sym(0.03f), y, o.z + H.sym(0.03f)};
        an.push_back(cercle(o, {1, 0, 0}, {0, 0, 1}, rr, rr * 0.85f, n, H() * 2.f));
    }
    const vec3 tete = o;
    loft(m, an, c.cep, true, {0, -0.03f, 0}, tete);
    eventail(m, an.back(), c.cep, HAUT);
    if (loin) return m;
    // cordon : deux bras le long du fil porteur (z), qui montent jusqu'au fil (0,60 m)
    for (const int s : {-1, 1}) {
        std::vector<std::vector<vec3>> br;
        const float L = 0.38f + H() * 0.1f;
        for (int i = 0; i < 3; i++) {
            const float t = (float)i / 2, rr = 0.02f - 0.008f * t;
            const vec3 p = tete + vec3{H.sym(0.015f), (FILS_Y[0] - tete.y + 0.015f) * std::sqrt(t), s * L * t};
            br.push_back(cercle(p, {1, 0, 0}, {0, 1, 0}, rr, rr, 4, 0.4f));
        }
        loft(m, br, c.cep, true, tete, tete + vec3{0, 0, s * L});
        eventail(m, br.back(), c.cep, {0, 0, (float)s});
    }
    return m;
}

// ---------------------------------------------------------------- clôture
Maille poteauCloture(const Couleurs& c, int variante) {
    Maille m;
    Hasard H{(uint32_t)variante * 2654435761u + 5u};
    const float r = CLOTURE_DIAM / 2;
    const vec3 incl{H.sym(0.02f), 0, H.sym(0.02f)};
    std::vector<std::vector<vec3>> an;
    const float ys[4] = {-0.12f, 0.45f, 0.95f, CLOTURE_HAUT - 0.1f};
    for (int i = 0; i < 4; i++) {
        const float rr = r * (1 + H.sym(0.07f));
        an.push_back(cercle(vec3{0, ys[i], 0} + incl * (ys[i] / CLOTURE_HAUT), {1, 0, 0}, {0, 0, 1}, rr, rr, 7, H() * 1.f));
    }
    loft(m, an, c.piquet, true, {0, -0.12f, 0}, {0, CLOTURE_HAUT, 0});
    // pointe taillée en diamant (4 pans) de 10 cm : du dernier anneau au sommet
    const vec3 sommet = vec3{0, CLOTURE_HAUT, 0} + incl;
    const std::vector<vec3>& d = an.back();
    for (size_t k = 0; k < d.size(); k++) triPlat(m, d[k], d[(k + 1) % d.size()], sommet, c.piquet, (d[k] + d[(k + 1) % d.size()]) * 0.5f - vec3{0, CLOTURE_HAUT - 0.2f, 0});
    return m;
}

// ---------------------------------------------------------------- botte ronde
Maille botte(const Couleurs& c, bool loin) {
    Maille m;
    const int n = loin ? 10 : 20;
    const float R = BOTTE_DIAM / 2, X = BOTTE_LARGEUR / 2, yC = R - 0.04f;   // posée : écrasée de 4 cm sur le sol
    const vec3 o{0, yC, 0}, ax{1, 0, 0};
    // flanc et bords arrondis (rayon 0,12 m), à peine bombé (+5 mm au milieu) ; un seul loft : normales lissées
    std::vector<std::pair<float, float>> prof;
    const float rb = 0.12f;
    for (int i = 0; i <= 3; i++) { const float a = PI / 2 * i / 3; prof.push_back({-X + rb - rb * std::cos(a), R - rb + rb * std::sin(a)}); }
    if (!loin) { prof.push_back({-X * 0.45f, R + 0.003f}); prof.push_back({0, R + 0.005f}); prof.push_back({X * 0.45f, R + 0.003f}); }
    for (int i = 3; i >= 0; i--) { const float a = PI / 2 * i / 3; prof.push_back({X - rb + rb * std::cos(a), R - rb + rb * std::sin(a)}); }
    std::vector<std::vector<vec3>> an;
    for (auto [x, r] : prof) {
        std::vector<vec3> a = cercle(o + ax * x, {0, 0, 1}, HAUT, r, r, n);
        for (vec3& p : a) p.y = std::max(p.y, 0.f);   // aplati au contact du sol
        an.push_back(a);
    }
    loft(m, an, c.foin, true, o - ax * X, o + ax * X);
    // bouts : spirale de foin enroulé (anneaux alternés), un peu creusés au centre (3 cm)
    for (const int s : {-1, 1}) {
        const float x0 = s * X;
        std::vector<vec3> prec = an[s < 0 ? 0 : an.size() - 1];
        const int na = loin ? 2 : 5;
        for (int k = 1; k <= na; k++) {
            const float rr = (R - rb) * (1 - (float)k / na), x = x0 - s * 0.03f * ((float)k / na);
            std::vector<vec3> a = cercle(o + ax * x, {0, 0, 1}, HAUT, std::max(rr, 1e-3f), std::max(rr, 1e-3f), n);
            for (vec3& p : a) p.y = std::max(p.y, 0.f);
            if (k < na) {   // anneau plat (face du bout), normale le long de l'axe
                for (size_t q = 0; q < a.size(); q++) {
                    const size_t q1 = (q + 1) % a.size();
                    quadPlat(m, prec[q], prec[q1], a[q1], a[q], k % 2 ? c.foinBout : c.foin, ax * (float)s);
                }
            } else {
                eventail(m, prec, c.foinBout, ax * (float)s);
            }
            prec = a;
        }
    }
    return m;
}

// ---------------------------------------------------------------- corps de ferme
Maille ferme(const Couleurs& c, bool loin) {
    Maille m;
    const float W = FERME_LARGEUR / 2, L = FERME_LONGUEUR / 2, H = FERME_MUR, P = FERME_PENTE, D = FERME_DEBORD, G = 0.25f;
    const float faite = H + W * P, ep = 0.12f;   // faîtage sous la couverture, épaisseur de la couverture
    // ouvertures : façade (porte au milieu), arrière, pignons ; fenêtres de 0,80 × 1,20 m (étage 0,80 × 1,10 m)
    struct Face { vec3 d, u; float L; std::vector<Ouv> o; std::vector<bool> porte; };
    auto F = [](float uc, float w, float v0, float h) { return Ouv{uc - w / 2, v0, uc + w / 2, v0 + h}; };
    std::vector<Face> faces = {
        {{1, 0, 0}, {0, 0, -1}, 2 * L, {F(0, 1.0f, 0, 2.1f), F(-3.0f, 0.8f, 0.9f, 1.2f), F(3.0f, 0.8f, 0.9f, 1.2f), F(-3.5f, 0.8f, 2.95f, 1.1f), F(0, 0.8f, 2.95f, 1.1f), F(3.5f, 0.8f, 2.95f, 1.1f)},
         {true, false, false, false, false, false}},
        {{-1, 0, 0}, {0, 0, 1}, 2 * L, {F(-3.0f, 0.8f, 0.9f, 1.2f), F(3.0f, 0.8f, 0.9f, 1.2f), F(-3.5f, 0.8f, 2.95f, 1.1f), F(3.5f, 0.8f, 2.95f, 1.1f)}, {false, false, false, false}},
        {{0, 0, 1}, {1, 0, 0}, 2 * W, {F(0, 0.8f, 0.9f, 1.2f), F(0, 0.6f, 3.1f, 0.8f)}, {false, false}},
        {{0, 0, -1}, {-1, 0, 0}, 2 * W, {F(0, 0.6f, 3.1f, 0.8f)}, {false}},
    };
    for (const Face& f : faces) {
        const vec3 cb = f.d * (f.d.x != 0 ? W : L);
        if (loin) murPerce(m, cb, f.u, f.d, f.L, H, {}, c.crepi, 0, c.embrasure);
        else murPerce(m, cb, f.u, f.d, f.L, H, f.o, c.crepi, 0.20f, c.embrasure);
        for (size_t k = 0; k < f.o.size(); k++) {
            const Ouv& o = f.o[k];
            fenetre(m, c, cb + f.u * ((o.u0 + o.u1) / 2) + HAUT * o.v0, f.u, f.d, o.u1 - o.u0, o.v1 - o.v0, f.porte[k], loin);
        }
        // soubassement de pierre : 0,6 m hors sol (+ 0,6 m enterré, sol en pente), 3 cm en saillie, glacis en tête
        const vec3 cs = cb + f.d * 0.03f - HAUT * 0.6f;
        std::vector<Ouv> os;
        for (size_t k = 0; k < f.o.size(); k++) if (f.porte[k]) os.push_back({f.o[k].u0, 0.6f, f.o[k].u1, 1.2f});
        murPerce(m, cs, f.u, f.d, f.L + 0.06f, 1.2f, os, c.pierre, 0.03f, c.pierre);
        const float Lg = f.L / 2 + 0.03f;
        std::vector<std::pair<float, float>> bouts{{-Lg, Lg}};
        if (!os.empty()) bouts = {{-Lg, os[0].u0}, {os[0].u1, Lg}};
        for (auto [u0, u1] : bouts) quadPlat(m, cs + f.u * u0 + HAUT * 1.2f, cs + f.u * u1 + HAUT * 1.2f, cb + f.u * u1 + HAUT * 0.64f, cb + f.u * u0 + HAUT * 0.64f, c.pierre, f.d + HAUT);
    }
    // pignons (triangles de crépi jusqu'au faîtage, sous la couverture)
    for (const float s : {-1.f, 1.f}) triPlat(m, {-W, H, s * L}, {W, H, s * L}, {0, faite, s * L}, c.crepi, {0, 0, s});
    // couverture : deux pans de tuiles canal (onde de 0,22 m, 5 cm de creux), débord de 0,45 m en long, 0,25 m sur les pignons
    for (const float s : {-1.f, 1.f}) {
        const vec3 n = normalize(vec3{s * P, 1, 0});
        const vec3 eg{s * (W + D), H + ep - D * P, 0}, fa{s * 0.02f, faite + ep, 0};
        if (loin) quadPlat(m, eg - vec3{0, 0, L + G}, eg + vec3{0, 0, L + G}, fa + vec3{0, 0, L + G}, fa - vec3{0, 0, L + G}, c.tuile, n);
        else ondule(m, eg, fa, {0, 0, 1}, -L - G, L + G, 0.22f, 0.05f, n, c.tuile, 4);
        // sous-face du pan (dessous du débord, visible d'en bas) et rives des pignons
        const vec3 eb = eg - HAUT * ep, fb = fa - HAUT * ep;
        quadPlat(m, eb - vec3{0, 0, L + G}, eb + vec3{0, 0, L + G}, fb + vec3{0, 0, L + G}, fb - vec3{0, 0, L + G}, c.tuile, -n);
        for (const float z : {-(L + G), L + G}) quadPlat(m, eg + vec3{0, 0, z}, fa + vec3{0, 0, z}, fb + vec3{0, 0, z}, eb + vec3{0, 0, z}, c.tuile, {0, 0, z});
        quadPlat(m, eg - vec3{0, 0, L + G}, eg + vec3{0, 0, L + G}, eb + vec3{0, 0, L + G}, eb - vec3{0, 0, L + G}, c.tuile, {s, 0, 0});   // about des tuiles
        if (!loin)   // génoise à deux rangs de tuiles sous l'égout
            for (int r = 0; r < 2; r++) {
                const float sail = r ? 0.13f : 0.27f, y = H - 0.05f - r * 0.1f;
                poutre(m, {s * (W + sail / 2), y, -L}, {s * (W + sail / 2), y, L}, HAUT, sail, 0.09f, 0.02f, c.tuile);
            }
    }
    // faîtage : tuiles faîtières en demi-rond (Ø 0,22 m)
    if (!loin) {
        std::vector<std::vector<vec3>> an;
        for (const float z : {-(L + G), L + G}) {
            std::vector<vec3> sec;
            for (int k = 0; k <= 6; k++) { const float a = PI * k / 6; sec.push_back({std::cos(a) * 0.12f, faite + ep - 0.03f + std::sin(a) * 0.09f, z}); }
            an.push_back(sec);
        }
        loft(m, an, c.tuile, false, {0, faite - 1, -(L + G)}, {0, faite - 1, L + G});
    } else {
        poutre(m, {0, faite + ep, -(L + G)}, {0, faite + ep, L + G}, HAUT, 0.24f, 0.08f, 0, c.tuile);
    }
    // cheminée de maçonnerie crépie, chapeau de pierre débordant
    const vec3 ch{0.35f, 0, 2.8f};
    poutre(m, ch + vec3{0, H, 0}, ch + vec3{0, faite + 0.9f, 0}, {0, 0, 1}, 0.60f, 0.45f, loin ? 0 : 0.01f, c.crepi);
    poutre(m, ch + vec3{0, faite + 0.9f, 0}, ch + vec3{0, faite + 0.97f, 0}, {0, 0, 1}, 0.74f, 0.59f, loin ? 0 : 0.015f, c.pierre);
    return m;
}

// ---------------------------------------------------------------- hangar agricole
Maille hangar(const Couleurs& c, const Maille& botteRangee, bool loin) {
    Maille m;
    const float W = HANGAR_LARGEUR / 2, L = HANGAR_LONGUEUR / 2, H = HANGAR_SABLIERE, T = std::tan(HANGAR_PENTE_DEG * PI / 180.f);
    const float deb = 0.3f;   // débord de la couverture
    auto yT = [&](float x) { return H + 0.30f + (W - std::fabs(x)) * T; };   // dessus des plaques
    const float faite = yT(0);
    // dalle de béton
    poutre(m, {0, -0.10f, -L - 0.1f}, {0, -0.10f, L + 0.1f}, HAUT, 2 * W + 0.2f, 0.24f, 0.02f, c.beton);
    // portiques : poteaux HEA 200, arbalétriers IPE 270, tous les 3,5 m
    for (int i = 0; i <= 4; i++) {
        const float z = -L + 0.1f + (2 * L - 0.2f) * i / 4;
        for (const float s : {-1.f, 1.f}) {
            const vec3 pied{s * (W - 0.1f), 0.02f, z}, tete{s * (W - 0.1f), H, z}, fa{0, faite - 0.33f, z};
            if (loin) { poutre(m, pied, tete, {1, 0, 0}, 0.2f, 0.19f, 0, c.acier); continue; }
            profileI(m, pied, tete, {1, 0, 0}, 0.19f, 0.20f, 0.0065f, 0.010f, c.acier);
            const vec3 nrm = normalize(vec3{s * T, 1, 0});
            profileI(m, tete + vec3{0, 0.0f, 0}, fa, nrm, 0.27f, 0.135f, 0.0066f, 0.0102f, c.acier);
        }
    }
    // pannes en Z (8 par pan simplifiées en 4) sous les plaques
    if (!loin)
        for (const float s : {-1.f, 1.f}) for (const float x : {0.35f, 1.9f, 3.45f, 4.95f}) {
            const vec3 nrm = normalize(vec3{s * T, 1, 0});
            poutre(m, {s * x, yT(x) - 0.10f, -L - 0.05f}, {s * x, yT(x) - 0.10f, L + 0.05f}, nrm, 0.07f, 0.16f, 0, c.acier);
        }
    // muret de béton banché (0,6 m) au fond et sur les pignons
    // muret : fond (x = −W) et pignons (z = ± L), 0,2 m d'épaisseur, chanfreiné
    poutre(m, {-W + 0.1f, 0.3f, -L}, {-W + 0.1f, 0.3f, L}, HAUT, 0.2f, 0.6f, 0.02f, c.beton);
    for (const float s : {-1.f, 1.f}) poutre(m, {-W, 0.3f, s * (L - 0.1f)}, {W, 0.3f, s * (L - 0.1f)}, HAUT, 0.2f, 0.6f, 0.02f, c.beton);
    // bardage de planches verticales de 145 mm (deux teintes de bois) et couvre-joints de 50 × 22 mm, au-dessus du muret
    const float pl = 0.145f;
    auto bardage = [&](vec3 o, vec3 u, vec3 d, float len, std::function<float(float)> haut) {
        const int n = (int)std::ceil(len / pl);
        if (loin) {   // loin : un quadrilatère par tronçon de 1,45 m
            for (int i = 0; i < n; i += 10) {
                const float u0 = i * pl, u1 = std::min(len, (i + 10) * pl);
                quadPlat(m, o + u * u0 + HAUT * 0.6f, o + u * u1 + HAUT * 0.6f, o + u * u1 + HAUT * haut(u1), o + u * u0 + HAUT * haut(u0), c.bardage, d);
            }
            return;
        }
        for (int i = 0; i < n; i++) {
            const float u0 = i * pl, u1 = std::min(len, (i + 1) * pl);
            quadPlat(m, o + u * u0 + HAUT * 0.6f, o + u * u1 + HAUT * 0.6f, o + u * u1 + HAUT * haut(u1), o + u * u0 + HAUT * haut(u0), i % 3 == 1 ? c.bardage2 : c.bardage, d);
            if (i > 0) {   // couvre-joint sur le joint u0
                const float h = haut(u0) - 0.02f;
                prisme(m, o + u * u0 + d * 0.011f + HAUT * 0.6f, o + u * u0 + d * 0.011f + HAUT * h, d, {{-0.025f, -0.011f}, {0.025f, -0.011f}, {0.025f, 0.011f}, {-0.025f, 0.011f}}, i % 2 ? c.bardage2 : c.bardage, false);
            }
        }
    };
    bardage({-W, 0, L}, {0, 0, -1}, {-1, 0, 0}, 2 * L, [&](float) { return yT(W) - 0.31f; });
    for (const float s : {-1.f, 1.f}) {
        const vec3 o{s > 0 ? -W : W, 0, s * L}, u{s > 0 ? 1.f : -1.f, 0, 0};
        bardage(o, u, {0, 0, s}, 2 * W, [&, o, u](float t) { const float x = o.x + u.x * t; return yT(x) - 0.31f; });
    }
    // couverture en plaques ondulées (onde de 177 mm, creux de 51 mm), débord de 0,3 m, faîtière, rives
    for (const float s : {-1.f, 1.f}) {
        const vec3 n = normalize(vec3{s * T, 1, 0});
        const vec3 eg{s * (W + deb), yT(W + deb), 0}, fa{s * 0.01f, faite, 0};
        if (loin) quadPlat(m, eg - vec3{0, 0, L + deb}, eg + vec3{0, 0, L + deb}, fa + vec3{0, 0, L + deb}, fa - vec3{0, 0, L + deb}, c.tole, n);
        else ondule(m, eg, fa, {0, 0, 1}, -L - deb, L + deb, 0.177f, 0.051f, n, c.tole, 4);
        // faîtière : bande pliée de 0,25 m de part et d'autre du faîtage
        quadPlat(m, {0, faite + 0.06f, -L - deb}, {0, faite + 0.06f, L + deb}, {s * 0.25f, faite + 0.06f - 0.25f * T, L + deb}, {s * 0.25f, faite + 0.06f - 0.25f * T, -L - deb}, c.tole, n);
        // rives (tôle pliée) sur les pignons
        for (const float z : {-(L + deb), L + deb}) poutre(m, eg + vec3{0, -0.06f, z}, fa + vec3{0, -0.06f, z}, n, 0.03f, 0.2f, 0, c.zinc);
        if (loin) continue;
        // gouttière demi-ronde (Ø 0,16 m) et descente (Ø 80 mm) aux deux coins
        std::vector<std::vector<vec3>> gt;
        for (const float z : {-(L + deb), L + deb}) {
            std::vector<vec3> sec;
            for (int k = 0; k <= 4; k++) { const float a = PI * k / 4; sec.push_back({s * (W + deb + 0.06f) + std::cos(a) * 0.08f, yT(W + deb) - 0.05f - std::sin(a) * 0.08f, z}); }
            gt.push_back(sec);
        }
        loft(m, gt, c.zinc, false, {s * (W + deb + 0.06f), yT(W + deb) + 1, -(L + deb)}, {s * (W + deb + 0.06f), yT(W + deb) + 1, L + deb});
        for (const float z : {-(L + deb - 0.1f), L + deb - 0.1f})
            revolution(m, {s * (W + deb + 0.06f), 0.15f, z}, HAUT, {{0, 0.04f}, {yT(W + deb) - 0.3f, 0.04f}}, 8, c.zinc);
    }
    // bottes rangées contre le fond : 3 en bas, 2 au-dessus (axe en travers)
    if (!loin) {
        const float x0 = -W + 0.4f + BOTTE_LARGEUR / 2;
        const vec3 pos[5] = {{x0, 0.02f, -L + 1.0f}, {x0, 0.02f, -L + 2.55f}, {x0, 0.02f, -L + 4.1f}, {x0, 1.44f, -L + 1.78f}, {x0, 1.44f, -L + 3.33f}};
        for (const vec3& p : pos) {
            const uint32_t k0 = (uint32_t)m.v.size();
            for (const Sommet& v : botteRangee.v) m.v.push_back({v.p + p, v.n, v.couleur});
            for (uint32_t i : botteRangee.i) m.i.push_back(k0 + i);
        }
    }
    return m;
}

// ---------------------------------------------------------------- poteau de ligne
void accrochesLigne(vec3 out[3]) {
    out[0] = {-0.7f, TRAVERSE_Y + 0.05f + 0.22f, -0.17f};
    out[1] = {0, POTEAU_HAUT + 0.22f, 0};
    out[2] = {0.7f, TRAVERSE_Y + 0.05f + 0.22f, -0.17f};
}

Maille poteauLigne(const Couleurs& c, bool loin) {
    Maille m;
    const int n = loin ? 6 : 10;
    // fût conique de bois traité (Ø 0,28 m au pied, 0,20 m en tête), tête chanfreinée
    revolution(m, {0, -0.2f, 0}, HAUT, {{0, POTEAU_DIAM_PIED / 2}, {3.f, 0.133f}, {6.f, 0.116f}, {POTEAU_HAUT + 0.2f - 0.05f, POTEAU_DIAM_TETE / 2}, {POTEAU_HAUT + 0.2f, POTEAU_DIAM_TETE / 2 - 0.04f}}, n, c.poteau);
    // traverse en cornière d'acier galvanisé de 1,6 m, devant le fût, et deux jambes de force
    const float zT = -0.17f, yT = TRAVERSE_Y;
    if (loin) poutre(m, {-TRAVERSE / 2, yT, zT}, {TRAVERSE / 2, yT, zT}, HAUT, 0.08f, 0.08f, 0, c.ferrure);
    else prisme(m, {-TRAVERSE / 2, yT, zT}, {TRAVERSE / 2, yT, zT}, HAUT, {{-0.04f, -0.04f}, {0.04f, -0.04f}, {0.04f, -0.032f}, {-0.032f, -0.032f}, {-0.032f, 0.04f}, {-0.04f, 0.04f}}, c.ferrure, false);
    for (const float s : {-1.f, 1.f}) poutre(m, {s * 0.6f, yT - 0.04f, zT + 0.02f}, {0, yT - 0.65f, -0.12f}, {0, 0, 1}, 0.05f, 0.012f, 0, c.ferrure);
    poutre(m, {0, yT - 0.06f, -0.12f}, {0, yT + 0.06f, -0.12f}, {0, 0, 1}, 0.24f, 0.06f, 0, c.ferrure);   // collier de fixation
    if (loin) return m;
    // isolateurs rigides à deux jupes (porcelaine), sur la traverse et en tête du fût
    vec3 acc[3]; accrochesLigne(acc);
    const std::vector<std::pair<float, float>> isol{{0, 0.016f}, {0.05f, 0.016f}, {0.06f, 0.035f}, {0.085f, 0.085f}, {0.105f, 0.08f}, {0.115f, 0.035f},
                                                   {0.14f, 0.07f}, {0.155f, 0.066f}, {0.17f, 0.03f}, {0.2f, 0.038f}, {0.22f, 0.02f}};
    for (const vec3& a : acc) revolution(m, a - vec3{0, 0.22f, 0}, HAUT, isol, 10, c.isolateur);
    return m;
}

} // namespace campagne
} // namespace wfs
