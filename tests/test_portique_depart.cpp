// WaForSpeed — WFS-559 : portique de départ en treillis et ses feux (src/world/portique_depart_modele.*), règles vérifiables
// par le calcul.
//   RG-02 : treillis carré de 290 mm, membrures Ø 50 mm, diagonales Ø 20 mm ; largeur = chaussée (15 m) + 1 m de chaque côté ;
//           hauteur libre 5,5 m ; feux Ø 200 mm avec visière et caisson ; mesures à ±1 % ;
//   RG-C1 : aucune forme nue (des centaines de pièces, bien plus que les 12 triangles d'une boîte) ; qualité : aucun triangle
//           dégénéré, normales unitaires, triangles tournés comme leurs normales ; budget de triangles du téléphone tenu ;
//   RG-06 : feux pilotés par le décompte (Course::feuxRouges / feuxVerts : 0, puis 1 toutes les 0,6 s, 5 à −0,6 s, verts au GO).
#include <cmath>
#include <cstdio>
#include "../src/game/course.h"
#include "../src/world/portique_depart_modele.h"

using namespace wfs;
using namespace wfs::portique_depart;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}
static bool proche(float v, float cible, float tol = 0.01f) { return std::fabs(v - cible) <= tol * std::fabs(cible); }

struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
static Boite boite(const Maille& m) {
    Boite b;
    for (const marina::Sommet& s : m.v) {
        b.mn = {std::fmin(b.mn.x, s.p.x), std::fmin(b.mn.y, s.p.y), std::fmin(b.mn.z, s.p.z)};
        b.mx = {std::fmax(b.mx.x, s.p.x), std::fmax(b.mx.y, s.p.y), std::fmax(b.mx.z, s.p.z)};
    }
    return b;
}
static void qualite(const Maille& m, const char* nom, int minimum, int budget) {
    int deg = 0, inv = 0, nonUnit = 0, mauvais = 0;
    for (size_t k = 0; k + 2 < m.i.size(); k += 3) {
        if (m.i[k] >= m.v.size() || m.i[k + 1] >= m.v.size() || m.i[k + 2] >= m.v.size()) { mauvais++; continue; }
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p, n = cross(b - a, c - a);
        if (length(n) < 1e-10f) { deg++; continue; }
        if (dot(n, m.v[m.i[k]].n + m.v[m.i[k + 1]].n + m.v[m.i[k + 2]].n) < 0) inv++;
    }
    for (const marina::Sommet& s : m.v) if (std::fabs(length(s.n) - 1) > 1e-3f || !std::isfinite(s.p.x + s.p.y + s.p.z)) nonUnit++;
    char t[260];
    std::snprintf(t, sizeof t, "%s : %d triangles (%d < n ≤ %d), 0 dégénéré (%d), 0 inversé (%d), normales unitaires (%d hors), indices (%d hors)",
                  nom, m.triangles(), minimum, budget, deg, inv, nonUnit, mauvais);
    verifie(m.triangles() > minimum && m.triangles() <= budget && deg == 0 && inv == 0 && nonUnit == 0 && mauvais == 0, t);
}

int main() {
    char t[300];
    const float chaussee = 15.f, ouverture = chaussee + 2.f, libre = 5.5f;
    const Modele md = construire(ouverture, libre, true);

    // ---------- treillis seul (un tronçon de 3 m le long de x) : section, membrures, diagonales
    {
        Maille tr;
        treillis(tr, {0, 0, 0}, {3, 0, 0}, {0, 1, 0}, TRONCON_POUTRE, ALU);
        const Boite b = boite(tr);
        std::snprintf(t, sizeof t, "treillis : section %.3f × %.3f m (290 mm ± 1 %%), longueur %.3f m (3 m ± 1 %%)", b.mx.y - b.mn.y, b.mx.z - b.mn.z, b.mx.x - b.mn.x);
        verifie(proche(b.mx.y - b.mn.y, 0.29f) && proche(b.mx.z - b.mn.z, 0.29f) && proche(b.mx.x - b.mn.x, 3.f), t);
        verifie(proche(MEMBRURE_D, 0.05f) && proche(DIAGONALE_D, 0.02f) && proche(ENTRAXE + MEMBRURE_D, SECTION), "treillis : membrures Ø 50 mm, diagonales Ø 20 mm, entraxe + Ø = section");
        qualite(tr, "treillis de 3 m", 500, 4000);
    }
    // ---------- portique : largeur, hauteur libre, pieds hors de la chaussée
    {
        const Boite b = boite(md.pieces[ALU]);
        const float largeurHors = ouverture + 2 * SECTION;
        std::snprintf(t, sizeof t, "portique : largeur hors tout %.3f m (chaussée 15 m + 2 × 1 m + 2 pieds de 0,29 m : %.3f ± 1 %%), haut %.3f m (5,5 + 0,29 : %.3f ± 1 %%)",
                      b.mx.x - b.mn.x, largeurHors, b.mx.y, libre + SECTION);
        verifie(proche(b.mx.x - b.mn.x, largeurHors) && proche(b.mx.y, libre + SECTION), t);
        // hauteur libre : aucun sommet du treillis au-dessus de la chaussée (|x| < 8,5 m) sous 5,5 m
        float bas = 1e9f;
        for (const marina::Sommet& s : md.pieces[ALU].v) if (std::fabs(s.p.x) < ouverture / 2 - 0.01f) bas = std::fmin(bas, s.p.y);
        std::snprintf(t, sizeof t, "portique : hauteur libre sous la poutre %.3f m (5,5 m ± 1 %%)", bas);
        verifie(proche(bas, libre), t);
        float pied = 1e9f;
        for (const marina::Sommet& s : md.pieces[ALU].v) if (s.p.y < 4.f) pied = std::fmin(pied, std::fabs(s.p.x));
        std::snprintf(t, sizeof t, "portique : pieds hors de la chaussée et 1 m au-delà (face intérieure à %.3f m de l'axe, 8,5 m ± 1 %%)", pied);
        verifie(proche(pied, chaussee / 2 + 1.f), t);
        const Boite p = boite(md.pieces[ACIER]);
        std::snprintf(t, sizeof t, "platines : %.3f m de profondeur (600 mm ± 1 %%), %.3f m d'épaisseur sous les boulons", p.mx.z - p.mn.z, PLATINE_EP);
        verifie(proche(p.mx.z - p.mn.z, PLATINE) && p.mn.y > -1e-4f, t);
    }
    // ---------- feux : 5 colonnes, lentilles Ø 200 mm, sous la poutre, au-dessus des voitures
    {
        bool ok = true;
        float dmin = 1e9f, dmax = 0;
        for (int k = 0; k < NB_FEUX; k++) {
            const Boite b = boite(md.lentilles[k]);
            const float d = b.mx.x - b.mn.x;
            dmin = std::fmin(dmin, d); dmax = std::fmax(dmax, d);
            ok = ok && proche((b.mx.x + b.mn.x) / 2, (k - 2) * FEU_PAS) && md.lentilles[k].triangles() > 20;
            qualite(md.lentilles[k], "lentilles d'une colonne", 40, 400);
        }
        std::snprintf(t, sizeof t, "feux : 5 colonnes au pas de 0,9 m, lentilles Ø %.3f à %.3f m (200 mm ± 1 %%)", dmin, dmax);
        verifie(ok && proche(dmin, FEU_D) && proche(dmax, FEU_D), t);
        const Boite c = boite(md.pieces[NOIR]);
        std::snprintf(t, sizeof t, "caisson : %.3f m de large (4,6 m ± 1 %%), bas à %.2f m (≥ 4,5 m : au-dessus des véhicules), haut %.2f m (≤ 5,5 m)", c.mx.x - c.mn.x, c.mn.y, c.mx.y);
        verifie(proche(c.mx.x - c.mn.x, CAISSON_L) && c.mn.y >= 4.5f && c.mx.y <= libre + MEMBRURE_D + 0.01f, t);
        const Boite v = boite(md.pieces[NOIR]);
        std::snprintf(t, sizeof t, "visières : saillie de %.3f m devant le caisson (160 mm ± 1 %%, face vers +z)", v.mx.z - (ENTRAXE / 2 + CAISSON_P / 2));
        verifie(proche(v.mx.z - (ENTRAXE / 2 + CAISSON_P / 2), VISIERE), t);
    }
    // ---------- qualité et budget de tout le modèle
    {
        int total = 0;
        for (const Maille& m : md.pieces) total += m.triangles();
        for (const Maille& m : md.lentilles) total += m.triangles();
        qualite(md.pieces[ALU], "treillis du portique", 2000, 22000);
        qualite(md.pieces[ACIER], "platines et boulons", 40, 600);
        qualite(md.pieces[NOIR], "caisson, bagues, visières, colliers", 200, 4000);
        std::snprintf(t, sizeof t, "portique entier : %d triangles (≤ 25 000 : objet unique de la scène, une voiture du joueur en a 120 000)", total);
        verifie(total <= 25000, t);
        const Modele arrivee = construire(ouverture, libre, false);
        verifie(arrivee.pieces[NOIR].triangles() == 0 && arrivee.pieces[ALU].triangles() == md.pieces[ALU].triangles(), "portique sans feux (ARRIVÉE, WFS-560) : même treillis, sans caisson");
    }
    // ---------- RG-06 : feux pilotés par le décompte
    {
        Course c; c.reset();
        bool ok = c.feuxRouges() == 0 && !c.feuxVerts();   // avant le 1er « 3 » (attente du fondu)
        int attendus[6] = {1, 2, 3, 4, 5, 5};
        c.lastCount = 3;
        for (int n = 0; n < 6; n++) { c.clock = -3 + 0.6 * n + 0.01; ok = ok && c.feuxRouges() == attendus[n]; }
        c.clock = -0.61; ok = ok && c.feuxRouges() == 4;
        c.clock = -0.6; ok = ok && c.feuxRouges() == 5;
        c.state = RaceState::Play; c.goShown = 0.7; ok = ok && c.feuxRouges() == 0 && c.feuxVerts();
        c.goShown = 0; ok = ok && !c.feuxVerts();
        verifie(ok, "décompte → feux : 0 avant le « 3 », 1 à −3 s, un toutes les 0,6 s, le 5e à −0,6 s, verts pendant le GO, puis éteints");
    }
    std::printf("%s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
