// WaForSpeed — WFS-562 : portique de checkpoint en treillis et bâche imprimée (src/world/portique_checkpoint_modele.*), règles
// vérifiables par le calcul.
//   RG-05 bis : vrai objet, mesures réelles ±1 % (treillis 290 mm du portique de départ, hauteur libre 5,2 m, 1 m entre les
//               poutres, bâche entre les axes des poutres, œillets tous les 0,5 m, ligne peinte de 0,30 m) ; aucune forme nue :
//               des centaines de pièces ; qualité : aucun triangle dégénéré, normales unitaires, triangles tournés comme leurs
//               normales ; budget du téléphone (de loin et de près) ;
//   courbe serrée ou tunnel : ligne peinte seule (aucun pied sur la chaussée).
#include <cmath>
#include <cstdio>
#include "../src/world/portique_checkpoint_modele.h"

using namespace wfs;
using namespace wfs::portique_checkpoint;

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
    char t[320];
    const float chaussee = 15.f, ouverture = chaussee + 2.f;
    const Modele md = construire(ouverture, chaussee);
    // ---------- portique : largeur, hauteurs, pieds hors de la chaussée
    {
        const Boite b = boite(md.pieces[ALU]);
        const float largeurHors = ouverture + 2 * SECTION, haut = HAUTEUR_LIBRE + 2 * SECTION + ENTRE_POUTRES;
        std::snprintf(t, sizeof t, "portique : largeur hors tout %.3f m (%.3f ± 1 %%), haut %.3f m (5,2 + 2 × 0,29 + 1,0 = %.3f ± 1 %%)", b.mx.x - b.mn.x, largeurHors, b.mx.y, haut);
        verifie(proche(b.mx.x - b.mn.x, largeurHors) && proche(b.mx.y, haut) && proche(md.hauteurTotale, haut), t);
        float bas = 1e9f;   // hauteur libre : aucune pièce au-dessus de la chaussée sous 5,2 m (projecteurs compris)
        for (int p = 0; p < NB_PIECES; p++) if (p != PEINTURE) for (const marina::Sommet& s : md.pieces[p].v) if (std::fabs(s.p.x) < chaussee / 2) bas = std::fmin(bas, s.p.y);
        std::snprintf(t, sizeof t, "portique : hauteur libre au-dessus de la chaussée %.3f m (5,2 m ± 1 %% : plus haut qu'un poids lourd de 4 m)", bas);
        verifie(proche(bas, HAUTEUR_LIBRE), t);
        float pied = 1e9f;
        for (const marina::Sommet& s : md.pieces[ALU].v) if (s.p.y < 4.f) pied = std::fmin(pied, std::fabs(s.p.x));
        std::snprintf(t, sizeof t, "portique : pieds à 1 m du bord de la chaussée (face intérieure à %.3f m de l'axe, 8,5 m ± 1 %%)", pied);
        verifie(proche(pied, chaussee / 2 + 1.f), t);
        float projBas = 1e9f, projX = 0;
        for (const marina::Sommet& s : md.pieces[VERRE].v) { projBas = std::fmin(projBas, s.p.y); projX = std::fmax(projX, std::fabs(s.p.x)); }
        std::snprintf(t, sizeof t, "projecteurs : vitres à %.2f m de haut au moins, hors de la chaussée (|x| ≥ %.2f m > 7,5 m)", projBas, chaussee / 2);
        float projIn = 1e9f;
        for (const marina::Sommet& s : md.pieces[VERRE].v) projIn = std::fmin(projIn, std::fabs(s.p.x));
        verifie(projBas > 2.5f && projIn > chaussee / 2, t);
    }
    // ---------- bâche : entre les axes des poutres, largeur, œillets, damier
    {
        const Boite b = boite(md.pieces[BACHE]);
        std::snprintf(t, sizeof t, "bâches : %.3f m de haut (axes des poutres : 1,29 m ± 1 %%), %.3f m de large (%.3f ± 1 %%), avant et arrière (z de %.3f à %.3f)",
                      b.mx.y - b.mn.y, b.mx.x - b.mn.x, ouverture - 0.2f, b.mn.z, b.mx.z);
        verifie(proche(b.mx.y - b.mn.y, BACHE_H) && proche(b.mx.x - b.mn.x, ouverture - 0.2f) && b.mn.z < -SECTION / 2 && b.mx.z > SECTION / 2, t);
        // œillets : un tous les ≈ 0,5 m, haut et bas, deux bâches ; Ø extérieur 24 mm
        const int n = (int)std::lround((ouverture - 0.2f) / OEILLET_PAS) + 1, attendus = n * 2 * 2;
        Maille un;
        un.v.assign(md.pieces[LAITON].v.begin(), md.pieces[LAITON].v.begin() + md.pieces[LAITON].v.size() / attendus);
        const Boite o = boite(un);
        const float de = std::fmax(o.mx.x - o.mn.x, o.mx.y - o.mn.y);
        const float pas = (ouverture - 0.2f - 0.12f) / (n - 1);
        std::snprintf(t, sizeof t, "œillets : %d (%d par bord), pas %.3f m (≤ 0,5 m + 2 %%), Ø extérieur %.4f m (24 mm ± 1 %%)", attendus, n, pas, de);
        verifie(md.pieces[LAITON].v.size() % attendus == 0 && pas <= OEILLET_PAS * 1.02f && proche(de, OEILLET_DE), t);
        const Boite noir = boite(md.pieces[ENCRE_NOIRE]), blanc = boite(md.pieces[ENCRE_BLANCHE]);
        std::snprintf(t, sizeof t, "damier : cases noires et blanches de %.2f m aux deux bouts, dans la bâche", DAMIER);
        verifie(md.pieces[ENCRE_NOIRE].triangles() > 0 && md.pieces[ENCRE_BLANCHE].triangles() > 0 && noir.mn.y >= b.mn.y && noir.mx.y <= b.mx.y
                && std::fabs(noir.mn.x) <= (ouverture - 0.2f) / 2 && blanc.mx.x <= (ouverture - 0.2f) / 2, t);
    }
    // ---------- ligne de chronométrage peinte : 0,30 m, 3 mm, toute la chaussée
    {
        const Boite l = boite(md.pieces[PEINTURE]);
        std::snprintf(t, sizeof t, "ligne peinte : %.3f m de large (0,30 m ± 1 %%), %.4f m d'épaisseur (3 mm), %.2f m de long (chaussée %.1f m)", l.mx.z - l.mn.z, l.mx.y - l.mn.y, l.mx.x - l.mn.x, chaussee);
        verifie(proche(l.mx.z - l.mn.z, LIGNE_L) && proche(l.mx.y - l.mn.y, LIGNE_EP, 0.02f) && proche(l.mx.x - l.mn.x, chaussee) && l.mn.y > -1e-5f, t);
    }
    // ---------- qualité, budget (de loin : sans œillets ni tendeurs ; de près : tout)
    {
        int loin = 0, pres = 0;
        for (int p = 0; p < NB_PIECES; p++) { pres += md.pieces[p].triangles(); if (p != LAITON && p != ATTACHE) loin += md.pieces[p].triangles(); }
        qualite(md.pieces[ALU], "treillis (2 pieds, 2 poutres)", 4000, 36000);
        qualite(md.pieces[ACIER], "platines, boulons, bras et lyres", 40, 3000);
        qualite(md.pieces[NOIR], "boîtiers des projecteurs", 100, 3000);
        qualite(md.pieces[BACHE], "bâches et ourlets", 20, 400);
        qualite(md.pieces[LAITON], "œillets", 1000, 12000);
        qualite(md.pieces[ATTACHE], "tendeurs à boule", 1000, 12000);
        qualite(md.pieces[VERRE], "vitres des projecteurs", 1, 20);
        qualite(md.pieces[PEINTURE], "ligne peinte", 10, 100);
        std::snprintf(t, sizeof t, "budget : %d triangles de loin (≤ 40 000), %d de près (≤ 55 000) ; un seul portique dans la scène, la voiture du joueur en a 120 000", loin, pres);
        verifie(loin <= 40000 && pres <= 55000, t);
        int pieces = 0; for (int p = 0; p < NB_PIECES; p++) pieces += md.pieces[p].triangles() > 0;
        verifie(pieces == NB_PIECES, "aucune forme nue : les 10 pièces existent (treillis, platines, boîtiers, bâches, encres, œillets, vitres, peinture, tendeurs)");
    }
    // ---------- courbe serrée ou tunnel : ligne seule
    {
        const Modele l = construire(ouverture, chaussee, true);
        int autres = 0; for (int p = 0; p < NB_PIECES; p++) if (p != PEINTURE) autres += l.pieces[p].triangles();
        verifie(autres == 0 && l.pieces[PEINTURE].triangles() == md.pieces[PEINTURE].triangles(), "courbe serrée ou tunnel : ligne peinte seule, aucun pied ni poutre");
    }
    std::printf("%s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
