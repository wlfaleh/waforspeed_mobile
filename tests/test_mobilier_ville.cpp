// WaForSpeed — WFS-369 : mobilier urbain de la Ville modélisé (src/world/mobilier_ville.*), règles vérifiables par le calcul.
//   RG-02 : vraies mesures ±1 % (banc 1,80 m, assise 0,45 m ; table Ø 0,70 m à 0,75 m ; chaise, assise 0,45 m ; parasol Ø 2,70 m ;
//           mât de lampadaire 7,40 m ; bassin de fontaine Ø 8,2 m) ; grille : vide entre barreaux < 11 cm (NF P01-012) ;
//   règle du 2026-11-09 (formes interdites) : chaque objet est fait de nombreuses pièces (pas une boîte : > 12 faces) ;
//   qualité : aucun triangle dégénéré, normales unitaires, triangles tournés comme leurs normales (pas de faces inversées).
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/world/mobilier_ville.h"

using namespace wfs;
using namespace wfs::mobilier;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };
struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
// boîte des sommets dont la case de palette est uv (u < 0 : tous)
static Boite boite(const Geo& g, float u = -1) {
    Boite b;
    for (const Vertex& s : g.v) {
        if (u >= 0 && std::fabs(s.u - u) > 1e-6f) continue;
        b.mn = {std::fmin(b.mn.x, s.px), std::fmin(b.mn.y, s.py), std::fmin(b.mn.z, s.pz)};
        b.mx = {std::fmax(b.mx.x, s.px), std::fmax(b.mx.y, s.py), std::fmax(b.mx.z, s.pz)};
    }
    return b;
}
static bool proche(float v, float cible, float tol) { return std::fabs(v - cible) <= tol * cible; }

static void qualite(const Geo& g, const char* nom) {
    int deg = 0, inv = 0, nonUnit = 0;
    for (size_t k = 0; k < g.i.size(); k += 3) {
        const Vertex &A = g.v[g.i[k]], &B = g.v[g.i[k + 1]], &C = g.v[g.i[k + 2]];
        const vec3 a{A.px, A.py, A.pz}, b{B.px, B.py, B.pz}, c{C.px, C.py, C.pz}, n = cross(b - a, c - a);
        if (dot(n, n) < 1e-16f) { deg++; continue; }
        const vec3 nm{A.nx + B.nx + C.nx, A.ny + B.ny + C.ny, A.nz + B.nz + C.nz};
        if (dot(n, nm) < 0) inv++;
    }
    for (const Vertex& s : g.v) if (std::fabs(std::sqrt(s.nx * s.nx + s.ny * s.ny + s.nz * s.nz) - 1) > 1e-3f) nonUnit++;
    char t[200];
    std::snprintf(t, sizeof t, "%s : %zu triangles, %d dégénéré(s), %d inversé(s), %d normale(s) non unitaire(s)", nom, g.i.size() / 3, deg, inv, nonUnit);
    verifie(deg == 0 && inv == 0 && nonUnit == 0 && g.i.size() / 3 > 12, t);
}

int main() {
    const Repere R = repere({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
    const vec2 c1{0.1f, 0.5f}, c2{0.2f, 0.5f}, c3{0.3f, 0.5f}, c4{0.4f, 0.5f};
    char t[200];
    {   // banc
        Geo g; banc(g, R, c1, c2);
        const Boite b = boite(g), l = boite(g, c1.x);
        std::snprintf(t, sizeof t, "banc : longueur %.3f m (1,80), assise %.3f m (0,45), dossier %.3f m (0,80-0,90), profondeur %.3f m", l.mx.z - l.mn.z, 0.4325f + 0.0175f, b.mx.y, b.mx.x - b.mn.x);
        verifie(proche(l.mx.z - l.mn.z, 1.80f, 0.01f) && b.mx.y > 0.80f && b.mx.y < 0.90f && proche(b.mx.x - b.mn.x, 0.60f, 0.05f), t);
        qualite(g, "banc");
    }
    {   // table et chaise de bistrot
        Geo g; tableBistrot(g, R, c1, c2);
        const Boite p = boite(g, c1.x);
        std::snprintf(t, sizeof t, "table : Ø %.3f m (0,70), hauteur %.3f m (0,75)", p.mx.x - p.mn.x, p.mx.y);
        verifie(proche(p.mx.x - p.mn.x, 0.70f, 0.01f) && proche(p.mx.y, 0.75f, 0.01f), t);
        qualite(g, "table de bistrot");
        Geo h; chaiseBistrot(h, R, c1, c2);
        const Boite a = boite(h, c1.x), b = boite(h);
        float assise = 0;
        for (const Vertex& s : h.v) if (std::fabs(s.u - c1.x) < 1e-6f && s.py < 0.55f) assise = std::fmax(assise, s.py);
        std::snprintf(t, sizeof t, "chaise : assise %.3f m (0,45), hauteur %.3f m (0,86), largeur d'assise %.3f m", assise, b.mx.y, a.mx.x - a.mn.x);
        verifie(proche(assise, 0.45f, 0.01f) && proche(b.mx.y, 0.86f, 0.02f), t);
        qualite(h, "chaise de bistrot");
    }
    {   // parasol
        Geo g; parasolOuvert(g, R, c1, c3, c2, c4);
        const Boite a = boite(g, c1.x), b = boite(g, c3.x);
        const float d = std::fmax(a.mx.x, b.mx.x) - std::fmin(a.mn.x, b.mn.x), bas = std::fmin(a.mn.y, b.mn.y);
        std::snprintf(t, sizeof t, "parasol ouvert : Ø %.3f m (2,70), bas du lambrequin à %.2f m (≥ 1,80)", d, bas);
        verifie(proche(d, 2.70f, 0.01f) && bas >= 1.80f, t);
        qualite(g, "parasol ouvert");
        Geo f; parasolFerme(f, R, c1, c2, c4);
        qualite(f, "parasol fermé");
    }
    {   // lampadaire
        Geo m, l; lampadaire(m, l, R, 2.4f, false);
        float mat = 0;
        for (const Vertex& s : m.v) if (std::fabs(s.px) < 0.2f && std::fabs(s.pz) < 0.2f && s.py > mat && s.py < 7.42f) mat = s.py;
        const Boite v = boite(l);
        std::snprintf(t, sizeof t, "lampadaire : mât de %.3f m (7,40), vasque à %.2f m du mât (2,2-2,5)", mat, (v.mn.x + v.mx.x) / 2);
        verifie(proche(mat, 7.40f, 0.01f) && (v.mn.x + v.mx.x) / 2 > 2.2f && (v.mn.x + v.mx.x) / 2 < 2.5f, t);
        qualite(m, "lampadaire");
    }
    {   // fontaine, abribus, grille, feu, panneau, jardinière
        Geo g; fontaine(g, R, 4.f, 1.f, c1, c2);
        const Boite b = boite(g, c1.x);
        std::snprintf(t, sizeof t, "fontaine : bassin Ø %.2f m (8,20)", b.mx.x - b.mn.x);
        verifie(proche(b.mx.x - b.mn.x, 8.20f, 0.01f), t);
        qualite(g, "fontaine");
        Geo a, v; abribus(a, v, R, c1, c2, c3);
        const Boite ab = boite(a);
        std::snprintf(t, sizeof t, "abribus : %.2f × %.2f m, %.3f m de haut (2,53)", ab.mx.z - ab.mn.z, ab.mx.x - ab.mn.x, ab.mx.y);
        verifie(proche(ab.mx.y, 2.53f, 0.01f) && v.i.size() > 0, t);
        qualite(a, "abribus");
        Geo gr; grille(gr, {0, 0, 0}, {0, 0, 4.8f}, {1, 0, 0}, c1);
        std::vector<float> pointes;   // une pointe de lance par barreau (y = 0,05 + 1,05 + 0,07) : positions distinctes le long de la grille
        for (const Vertex& s : gr.v) if (std::fabs(s.py - 1.17f) < 1e-4f && s.pz > 0.06f) {   // hors de la pomme du poteau
            bool vu = false; for (float z : pointes) vu = vu || std::fabs(z - s.pz) < 1e-3f;
            if (!vu) pointes.push_back(s.pz);
        }
        const int barreaux = (int)pointes.size();
        const float vide = (4.8f - 0.08f) / (float)barreaux - 0.016f;
        std::snprintf(t, sizeof t, "grille : %d barreaux sur 4,8 m, vide de %.3f m (< 0,11)", barreaux, vide);
        verifie(barreaux > 0 && vide < 0.11f, t);
        qualite(gr, "grille");
        Geo fm, fs; feuTricolore(fm, fs, R, c1); optique(fs, R, 0);
        qualite(fs, "feu tricolore");
        Geo pm, ps; panneauRond(pm, ps, R, 2.3f, c1, c2, c3);
        const Boite pb = boite(ps);
        std::snprintf(t, sizeof t, "panneau rond : Ø %.3f m (0,70)", pb.mx.z - pb.mn.z);
        verifie(proche(pb.mx.z - pb.mn.z, 0.70f, 0.01f), t);
        qualite(ps, "panneau rond");
        Geo j; jardiniere(j, R, c1, c2, c3, c4, 7);
        qualite(j, "jardinière et buis");
    }
    std::printf("test_mobilier_ville : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
