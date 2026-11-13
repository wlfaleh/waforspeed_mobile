// WaForSpeed — WFS-410 (épic E15) : fenêtres et portes réelles des bâtiments hors kit (voir archi_lieux.h).
//
// Mesures des baies en plein cintre (église de village, références : églises romanes rurales de Provence et du Languedoc,
// conception ; à confirmer au référentiel references/reel/, RG-04 de WFS-410) :
//   - portail : 1,8 à 2,4 m de large, 3,5 à 4,5 m de haut, ébrasement 0,45 m (mur de 0,8 à 1,0 m), arc de claveaux de 0,40 m
//     de profondeur en saillie de 3 cm, clé plus large ; porte à deux vantaux de planches de 14 cm (joints en V de 1 cm),
//     3 pentures de fer forgé (6 cm × 8 mm) par vantail ;
//   - baie : 0,6 à 0,9 m × 1,6 à 2,4 m, vitrail sous plomb tenu par des barlotières de fer tous les 0,45 m (2 cm).
#include "archi_lieux.h"
#include "archi_ouvertures.h"
#include <algorithm>
#include <cmath>

namespace wfs::archi::lieux {
namespace {

constexpr float JOINT = 0.012f;   // joint entre deux claveaux (mortier, le mur se voit au fond)

// quadrilatère plan a b c d, normale n donnée (unitaire) ; ordre des sommets corrigé pour que la face regarde n
void face(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n) {
    vec3 g = cross(b - a, c - a);
    if (length(g) < 1e-12f) g = cross(c - a, d - a);
    if (length(g) < 1e-12f) return;
    if (dot(g, n) < 0) std::swap(b, d);
    const uint32_t k = (uint32_t)m.v.size();
    for (vec3 p : {a, b, c, d}) {   // uv en mètres sur le plan le plus proche (appareil des murs, decor.glsl)
        const vec2 uv = std::fabs(n.y) > 0.7f ? vec2{p.x, p.z} : std::fabs(n.x) > std::fabs(n.z) ? vec2{p.z, p.y} : vec2{p.x, p.y};
        m.v.push_back({p, n, uv});
    }
    m.i.insert(m.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
void tri(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 n) {
    if (dot(cross(b - a, c - a), n) < 0) std::swap(b, c);
    const uint32_t k = (uint32_t)m.v.size();
    for (vec3 p : {a, b, c}) m.v.push_back({p, n, {p.x, p.y}});
    m.i.insert(m.i.end(), {k, k + 1, k + 2});
}
void rect(Maillage& m, float x0, float x1, float y0, float y1, float z) {
    if (x1 - x0 < 1e-4f || y1 - y0 < 1e-4f) return;
    face(m, {x0, y0, z}, {x1, y0, z}, {x1, y1, z}, {x0, y1, z}, {0, 0, 1});
}
// pavé à faces visibles depuis l'avant (avant, côtés, dessus, dessous), de z0 (dos, contre le mur) à z1
void pave(Maillage& m, float x0, float x1, float y0, float y1, float z0, float z1) {
    rect(m, x0, x1, y0, y1, z1);
    face(m, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0});
    face(m, {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0});
    face(m, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0, 1, 0});
    face(m, {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0, -1, 0});
}

// facettes d'un arc en plein cintre : n + 1 points de la naissance gauche à la naissance droite, rayon r, centre (cx, cy)
std::vector<vec2> arc(float cx, float cy, float r, int n) {
    std::vector<vec2> p((size_t)n + 1);
    for (int i = 0; i <= n; i++) {
        const float a = PI * (1.f - (float)i / (float)n);
        p[(size_t)i] = {cx + r * std::cos(a), cy + r * std::sin(a)};
    }
    return p;
}
// hauteur de l'intrados d'un arc (centre cx, naissance cy, rayon r) à l'abscisse x (dans l'ouverture)
float hautArc(float cx, float cy, float r, float x) { const float u = std::clamp((x - cx) / r, -1.f, 1.f); return cy + r * std::sqrt(1 - u * u); }

}  // namespace

size_t triangles(const Batiment& b) { size_t n = 0; for (const Maillage& m : b.part) n += m.triangles(); return n; }

void construire(const Facade& f, int lod, Batiment& out) {
    Maillage& mur = out.part[MUR];
    if (lod >= 2) {   // loin : mur plein, baies peintes 1 cm devant (pas de trou : rien ne se voit au travers)
        rect(mur, 0, f.largeur, f.y0, f.y1, 0);
    } else {   // mur percé : grille des bords des baies, chaque case hors baie est posée
        std::vector<float> xs{0, f.largeur}, ys{f.y0, f.y1};
        for (const Ouv& o : f.baies) { xs.push_back(o.x - o.l / 2); xs.push_back(o.x + o.l / 2); ys.push_back(o.y); ys.push_back(o.y + o.h); }
        auto trier = [](std::vector<float>& v) {
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end(), [](float a, float b) { return std::fabs(a - b) < 1e-4f; }), v.end());
        };
        trier(xs); trier(ys);
        for (size_t i = 0; i + 1 < xs.size(); i++) {
            // cases d'une même colonne fusionnées en bandes (moins de triangles)
            float debut = -1;
            for (size_t j = 0; j + 1 < ys.size(); j++) {
                const float cx = (xs[i] + xs[i + 1]) / 2, cy = (ys[j] + ys[j + 1]) / 2;
                bool trou = cx < 0 || cx > f.largeur || cy < f.y0 || cy > f.y1;
                for (const Ouv& o : f.baies) trou |= cx > o.x - o.l / 2 && cx < o.x + o.l / 2 && cy > o.y && cy < o.y + o.h;
                if (!trou && debut < 0) debut = ys[j];
                if (trou && debut >= 0) { rect(mur, xs[i], xs[i + 1], debut, ys[j], 0); debut = -1; }
            }
            if (debut >= 0) rect(mur, xs[i], xs[i + 1], debut, ys.back(), 0);
        }
    }
    for (const Ouv& o : f.baies) {
        // mur libre de chaque côté : jusqu'au bord de la façade, ou la moitié de l'écart avec la baie voisine du même niveau
        float libre = std::min(o.x - o.l / 2, f.largeur - (o.x + o.l / 2));
        for (const Ouv& v : f.baies) {
            if (&v == &o || v.y + v.h <= o.y || v.y >= o.y + o.h) continue;
            const float ecart = v.x > o.x ? (v.x - v.l / 2) - (o.x + o.l / 2) : (o.x - o.l / 2) - (v.x + v.l / 2);
            if (ecart >= 0) libre = std::min(libre, ecart / 2);
        }
        ouvertures::baieComplete(f.style, f.graine ^ (uint32_t)(o.x * 100.f) * 2654435761u, o.type, o.x, o.y, o.l, o.h, o.plancher,
                                 std::clamp(libre, 0.f, f.murLibre), lod, out, Baies::Reelles);
    }
}

void murArcs(float largeur, float y0, float y1, const std::vector<Arc>& arcs, float d, int lod, Batiment& out) {
    Maillage& mur = out.part[MUR];
    Maillage& tab = out.part[ENCADREMENT];
    const int n = lod == 0 ? 16 : 8;   // facettes de l'arc (une par 11°, de près)
    std::vector<Arc> a = arcs;
    std::sort(a.begin(), a.end(), [](const Arc& p, const Arc& q) { return p.x < q.x; });
    float prev = 0;
    for (const Arc& b : a) {
        const float x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, r = b.l / 2, yn = b.y + b.h - r;   // naissance de l'arc
        rect(mur, prev, x0, y0, y1, 0);
        prev = x1;
        if (lod >= 2 && !b.vide) {   // loin : mur plein, baie peinte devant (rectangle et demi-cercle en éventail)
            rect(mur, x0, x1, y0, y1, 0);
            Maillage& fond = b.porte ? out.part[MENUISERIE] : out.part[VITRE];
            rect(fond, x0, x1, b.y, yn, 0.01f);
            const std::vector<vec2> p = arc(b.x, yn, r, n);
            for (int i = 0; i < n; i++) tri(fond, {b.x, yn, 0.01f}, {p[(size_t)i].x, p[(size_t)i].y, 0.01f}, {p[(size_t)i + 1].x, p[(size_t)i + 1].y, 0.01f}, {0, 0, 1});
            continue;
        }
        rect(mur, x0, x1, y0, b.y, 0);   // allège (ou rien sous un portail posé au sol)
        const std::vector<vec2> p = arc(b.x, yn, r, n);
        for (int i = 0; i < n; i++) {    // écoinçons : de l'arc au haut du mur, facette par facette
            const vec2 u = p[(size_t)i], v = p[(size_t)i + 1];
            face(mur, {u.x, u.y, 0}, {v.x, v.y, 0}, {v.x, y1, 0}, {u.x, y1, 0}, {0, 0, 1});
        }
        // ébrasement : piédroits, intrados de l'arc, fond d'appui
        face(tab, {x0, b.y, 0}, {x0, b.y, -d}, {x0, yn, -d}, {x0, yn, 0}, {1, 0, 0});
        face(tab, {x1, b.y, -d}, {x1, b.y, 0}, {x1, yn, 0}, {x1, yn, -d}, {-1, 0, 0});
        if (b.y > y0 + 1e-3f) face(tab, {x0, b.y, 0}, {x1, b.y, 0}, {x1, b.y, -d}, {x0, b.y, -d}, {0, 1, 0});
        for (int i = 0; i < n; i++) {
            const vec2 u = p[(size_t)i], v = p[(size_t)i + 1], mi{(u.x + v.x) * 0.5f, (u.y + v.y) * 0.5f};
            const vec3 nn = normalize(vec3{b.x - mi.x, yn - mi.y, 0});   // vers le centre de l'arc
            face(tab, {u.x, u.y, 0}, {v.x, v.y, 0}, {v.x, v.y, -d}, {u.x, u.y, -d}, nn);
        }
        // claveaux en saillie de 3 cm (9 voussoirs, clé 30 % plus large), 0,40 m de profondeur dans le mur ; de près seulement
        if (lod == 0) {
            const float e = 0.40f, s = 0.03f;
            const int nv = 9;
            float total = 0; std::vector<float> w(nv);
            for (int k = 0; k < nv; k++) { w[(size_t)k] = k == nv / 2 ? 1.3f : 1.f; total += w[(size_t)k]; }
            float acc = 0;
            for (int k = 0; k < nv; k++) {
                const float a0 = PI * (1 - acc / total), a1 = PI * (1 - (acc + w[(size_t)k]) / total);
                acc += w[(size_t)k];
                const float j = JOINT / r / 2;   // demi-joint en angle
                const int m = 3;   // facettes par claveau
                for (int q = 0; q < m; q++) {
                    const float b0 = a0 - j - (a0 - a1 - 2 * j) * (float)q / m, b1 = a0 - j - (a0 - a1 - 2 * j) * (float)(q + 1) / m;
                    const vec2 c0{std::cos(b0), std::sin(b0)}, c1{std::cos(b1), std::sin(b1)};
                    const vec3 i0{b.x + c0.x * r, yn + c0.y * r, s}, i1{b.x + c1.x * r, yn + c1.y * r, s};
                    const vec3 o0{b.x + c0.x * (r + e), yn + c0.y * (r + e), s}, o1{b.x + c1.x * (r + e), yn + c1.y * (r + e), s};
                    face(tab, i0, i1, o1, o0, {0, 0, 1});   // parement
                    const vec3 no = normalize(vec3{(c0.x + c1.x), (c0.y + c1.y), 0});
                    face(tab, o0, o1, o1 - vec3{0, 0, s}, o0 - vec3{0, 0, s}, no);   // extrados en saillie
                }
                for (float bb : {a0 - j, a1 + j}) {   // lits des claveaux (joints rayonnants), sur la saillie
                    const vec2 c{std::cos(bb), std::sin(bb)};
                    const vec3 in{b.x + c.x * r, yn + c.y * r, s}, ou{b.x + c.x * (r + e), yn + c.y * (r + e), s};
                    const vec3 nl = bb == a0 - j ? vec3{-c.y, c.x, 0} : vec3{c.y, -c.x, 0};
                    face(tab, in, ou, ou - vec3{0, 0, s}, in - vec3{0, 0, s}, nl);
                }
            }
            for (float xi : {x0, x1})   // impostes : tailloir de pierre à la naissance (0,18 m, saillie 4 cm)
                pave(tab, xi - (xi < b.x ? 0.12f : 0.f), xi + (xi < b.x ? 0.f : 0.12f), yn - 0.18f, yn, 0, 0.04f);
        }
        // fond : vitrail sous plomb (barlotières de fer) ou porte à deux vantaux de planches cloutées ; rien (baie campanaire)
        if (b.vide) {
        } else if (!b.porte) {
            Maillage& v = out.part[VITRE];
            rect(v, x0, x1, b.y, yn, -d);
            for (int i = 0; i < n; i++) tri(v, {b.x, yn, -d}, {p[(size_t)i].x, p[(size_t)i].y, -d}, {p[(size_t)i + 1].x, p[(size_t)i + 1].y, -d}, {0, 0, 1});
            Maillage& fer = out.part[METAL];
            for (float y = b.y + 0.45f; y < b.y + b.h - 0.1f; y += 0.45f) {
                const float ux = y <= yn ? r : std::sqrt(std::max(0.f, r * r - (y - yn) * (y - yn)));
                pave(fer, b.x - ux, b.x + ux, y - 0.01f, y + 0.01f, -d, -d + 0.012f);
            }
            pave(fer, b.x - 0.01f, b.x + 0.01f, b.y, b.y + b.h, -d, -d + 0.012f);   // montant
        } else {
            Maillage& bois = out.part[MENUISERIE];
            const int nb = std::max(6, (int)std::lround(b.l / 0.14f));
            const float lp = b.l / (float)nb;
            for (int k = 0; k < nb; k++) {
                const float xa = x0 + lp * (float)k, xb = xa + lp, g = k % (nb / 2) == 0 ? 0.004f : 0.005f;   // joints en V
                const float ya = hautArc(b.x, yn, r, xa), yb = hautArc(b.x, yn, r, xb);
                face(bois, {xa + g, b.y, -d}, {xb - g, b.y, -d}, {xb - g, yb, -d}, {xa + g, ya, -d}, {0, 0, 1});
                // fond du joint, 1 cm en retrait (ombre entre les planches)
                face(bois, {xb - g, b.y, -d - 0.01f}, {xb + g, b.y, -d - 0.01f}, {xb + g, yb, -d - 0.01f}, {xb - g, yb, -d - 0.01f}, {0, 0, 1});
                if (lod == 0) for (float xx : {xa + g, xb - g}) {
                    const float yy = hautArc(b.x, yn, r, xx), sn = xx < xa + lp / 2 ? -1.f : 1.f;
                    face(bois, {xx, b.y, -d}, {xx, b.y, -d - 0.01f}, {xx, yy, -d - 0.01f}, {xx, yy, -d}, {sn, 0, 0});
                }
            }
            if (lod == 0) {   // 3 pentures de fer forgé par vantail, des gonds vers le milieu ; anneau de tirage au battement
                Maillage& fer = out.part[METAL];
                for (float yp : {b.y + 0.35f, b.y + (yn - b.y) * 0.5f, yn - 0.25f})
                    for (int sv : {-1, 1}) {
                        const float xg = sv < 0 ? x0 : x1, xl = xg - sv * b.l * 0.36f;
                        pave(fer, std::min(xg, xl), std::max(xg, xl), yp - 0.03f, yp + 0.03f, -d, -d + 0.008f);
                    }
                for (int sv : {-1, 1}) pave(fer, b.x + sv * 0.05f - 0.012f, b.x + sv * 0.05f + 0.012f, b.y + 1.0f, b.y + 1.12f, -d, -d + 0.02f);
            }
        }
    }
    rect(mur, prev, largeur, y0, y1, 0);
}

void versMonde(Maillage& m, vec3 o, vec3 x, vec3 z) {
    const vec3 y{0, 1, 0};
    for (Sommet& s : m.v) {
        s.p = o + x * s.p.x + y * s.p.y + z * s.p.z;
        s.n = x * s.n.x + y * s.n.y + z * s.n.z;
    }
}

}  // namespace wfs::archi::lieux
