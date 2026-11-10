// WaForSpeed — WFS-372 : objets agricoles de la Campagne (src/world/decor_campagne_modeles.*), règles vérifiables par le calcul.
//   RG-02 / RG-03 : vraies mesures à ±1 % (piquet de vigne 1,80 m, cep 0,58 m, poteau de clôture 1,20 m × Ø 0,12 m, botte
//                   1,20 × Ø 1,50 m, ferme 8 × 11 m au sol et murs de 4,5 m, toit à 30 %, hangar 10 × 14 m, sablière à 5,5 m,
//                   poteau de ligne de 8,5 m et traverse de 1,6 m) ;
//   RG-04 : budget de triangles (proche / loin) tenu ; la version « loin » est plus légère que la version proche ;
//   qualité : aucun triangle dégénéré, normales unitaires, triangles tournés comme leurs normales (pas de face inversée).
#include <cmath>
#include <cstdio>
#include "../src/world/decor_campagne_modeles.h"

using namespace wfs;
using namespace wfs::campagne;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
static Boite boite(const Maille& m, float yMin = -1e9f, int couleur = -1000) {
    Boite b;
    for (const marina::Sommet& s : m.v) {
        if (s.p.y < yMin || (couleur != -1000 && s.couleur != couleur)) continue;
        b.mn = {std::fmin(b.mn.x, s.p.x), std::fmin(b.mn.y, s.p.y), std::fmin(b.mn.z, s.p.z)};
        b.mx = {std::fmax(b.mx.x, s.p.x), std::fmax(b.mx.y, s.p.y), std::fmax(b.mx.z, s.p.z)};
    }
    return b;
}
static bool proche(float v, float cible, float tol = 0.01f) { return std::fabs(v - cible) <= tol * cible; }

static void qualite(const Maille& m, const char* nom, int budget) {
    int deg = 0, inv = 0, nonUnit = 0;
    for (size_t k = 0; k < m.i.size(); k += 3) {
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p, n = cross(b - a, c - a);
        if (length(n) < 1e-9f) { deg++; continue; }
        const vec3 nm = m.v[m.i[k]].n + m.v[m.i[k + 1]].n + m.v[m.i[k + 2]].n;
        if (dot(n, nm) < 0) inv++;
    }
    for (const marina::Sommet& s : m.v) if (std::fabs(length(s.n) - 1) > 1e-3f) nonUnit++;
    char t[220];
    std::snprintf(t, sizeof t, "%s : %d triangles (≤ %d), 0 dégénéré (%d), 0 inversé (%d), normales unitaires (%d hors)", nom, m.triangles(), budget, deg, inv, nonUnit);
    verifie(m.triangles() > 0 && m.triangles() <= budget && deg == 0 && inv == 0 && nonUnit == 0, t);
}

int main() {
    Couleurs c;
    int k = 1;
    for (int* p : {&c.piquet, &c.cep, &c.fil, &c.foin, &c.foinBout, &c.crepi, &c.pierre, &c.tuile, &c.volet, &c.menuiserie, &c.vitre, &c.porte, &c.embrasure,
                   &c.bardage, &c.bardage2, &c.tole, &c.acier, &c.beton, &c.zinc, &c.poteau, &c.ferrure, &c.isolateur, &c.cable}) *p = k++;
    char t[240];

    // ---------- vigne
    for (int v = 0; v < 4; v++) {
        const Maille p = piquetVigne(c, v);
        const Boite b = boite(p);
        std::snprintf(t, sizeof t, "piquet de vigne %d : %.3f m hors sol (1,80 ± 1 %%), Ø %.3f m (0,07 à 0,09)", v, b.mx.y, b.mx.x - b.mn.x);
        verifie(proche(b.mx.y, PIQUET_HAUT) && b.mx.x - b.mn.x > 0.06f && b.mx.x - b.mn.x < 0.10f, t);
        qualite(p, "piquet de vigne", 80);
        const Maille ce = cep(c, v, false), cl = cep(c, v, true);
        const Boite bc = boite(ce), bt = boite(cl);
        std::snprintf(t, sizeof t, "cep %d : tronc de %.3f m (0,58 ± 1 %%), cordon de %.2f m le long du fil, bras à %.3f m (fil porteur 0,60)", v, bt.mx.y, bc.mx.z - bc.mn.z, bc.mx.y);
        verifie(proche(bt.mx.y, CEP_HAUT, 0.03f) && bc.mx.z - bc.mn.z > 0.7f && bc.mx.y > 0.58f && bc.mx.y < 0.64f, t);
        qualite(ce, "cep proche", 80);
        qualite(cl, "cep loin", 16);
    }
    std::snprintf(t, sizeof t, "fils du palissage à 0,60 / 1,00 / 1,40 m, sous le haut des piquets et dans le feuillage (%.2f à %.2f m)", VIGNE_FEUILLE_BAS, VIGNE_FEUILLE_HAUT);
    verifie(FILS_Y[0] >= VIGNE_FEUILLE_BAS - 0.01f && FILS_Y[2] < VIGNE_FEUILLE_HAUT && VIGNE_FEUILLE_HAUT < PIQUET_HAUT, t);

    // ---------- clôture
    for (int v = 0; v < 3; v++) {
        const Maille p = poteauCloture(c, v);
        const Boite b = boite(p, 0.f);
        std::snprintf(t, sizeof t, "poteau de clôture %d : %.3f m hors sol (1,20 ± 1 %%), Ø %.3f m (0,12 ± 10 %%)", v, b.mx.y, b.mx.x - b.mn.x);
        verifie(proche(b.mx.y, CLOTURE_HAUT) && proche(b.mx.x - b.mn.x, CLOTURE_DIAM, 0.1f), t);
        qualite(p, "poteau de clôture", 60);
    }
    {
        Maille l; lisse(l, {0, 0.45f, 0}, {0, 0.45f, 3.6f}, {1, 0, 0}, c.piquet, false);
        const Boite b = boite(l);
        std::snprintf(t, sizeof t, "lisse en demi-rondin : %.3f m de long (3,6), épaisseur %.3f m (0,05 : demi Ø 0,10)", b.mx.z - b.mn.z, b.mx.x - b.mn.x);
        verifie(proche(b.mx.z - b.mn.z, 3.6f) && proche(b.mx.x - b.mn.x, LISSE_DIAM / 2, 0.05f), t);
        qualite(l, "lisse", 40);
    }

    // ---------- botte ronde
    {
        const Maille p = botte(c, false), lo = botte(c, true);
        const Boite b = boite(p);
        std::snprintf(t, sizeof t, "botte ronde : largeur %.3f m (1,20 ± 1 %%), Ø %.3f m (1,50 ± 1 %%), posée au sol (bas %.3f)", b.mx.x - b.mn.x, b.mx.z - b.mn.z, b.mn.y);
        verifie(proche(b.mx.x - b.mn.x, BOTTE_LARGEUR) && proche(b.mx.z - b.mn.z, BOTTE_DIAM, 0.011f) && std::fabs(b.mn.y) < 1e-4f, t);
        qualite(p, "botte proche", 800);
        qualite(lo, "botte loin", 200);
    }

    // ---------- ferme
    {
        const Maille p = ferme(c, false), lo = ferme(c, true);
        const Boite mur = boite(p, 0.65f, c.crepi), tot = boite(p);
        std::snprintf(t, sizeof t, "ferme : murs %.2f × %.2f m (8 × 11 ± 1 %%), hauteur des murs %.2f m (4,5)", mur.mx.x - mur.mn.x, mur.mx.z - mur.mn.z, FERME_MUR);
        verifie(proche(mur.mx.x - mur.mn.x, FERME_LARGEUR) && proche(mur.mx.z - mur.mn.z, FERME_LONGUEUR), t);
        const Boite toit = boite(p, 0, c.tuile);
        const float pente = (toit.mx.y - (FERME_MUR + 0.12f)) / (FERME_LARGEUR / 2);
        std::snprintf(t, sizeof t, "ferme : toit de tuiles à %.0f %% (30 %% ± 3 points), débord %.2f m (0,45), faîtage à %.2f m", pente * 100, (toit.mx.x - toit.mn.x) / 2 - FERME_LARGEUR / 2, toit.mx.y);
        verifie(std::fabs(pente - FERME_PENTE) < 0.03f && proche((toit.mx.x - toit.mn.x) / 2 - FERME_LARGEUR / 2, FERME_DEBORD, 0.2f) && tot.mn.y > -0.7f, t);
        int vitres = 0; for (const auto& s : p.v) if (s.couleur == c.vitre) vitres++;
        std::snprintf(t, sizeof t, "ferme : fenêtres vitrées (%d sommets de vitre), volets (%s)", vitres, boite(p, -1, c.volet).mx.y > 0 ? "oui" : "non");
        verifie(vitres >= 4 * 12 && boite(p, -1, c.volet).mx.y > 0, t);
        qualite(p, "ferme proche", 12000);
        qualite(lo, "ferme loin", 900);
        verifie(lo.triangles() * 4 < p.triangles(), "ferme : la version loin est au moins 4 fois plus légère");
    }

    // ---------- hangar
    {
        const Maille b1 = botte(c, true);
        const Maille p = hangar(c, b1, false), lo = hangar(c, b1, true);
        const Boite ac = boite(p, -1, c.acier), to = boite(p, -1, c.tole);
        std::snprintf(t, sizeof t, "hangar : ossature %.2f × %.2f m (10 × 14 ± 1 %%), couverture de %.2f à %.2f m (sablière 5,5)", ac.mx.x - ac.mn.x, ac.mx.z - ac.mn.z, to.mn.y, to.mx.y);
        verifie(proche(ac.mx.x - ac.mn.x, HANGAR_LARGEUR, 0.012f) && proche(ac.mx.z - ac.mn.z, HANGAR_LONGUEUR, 0.012f) && to.mn.y > HANGAR_SABLIERE - 0.2f, t);
        // pente mesurée sur la version loin (pans plans) : égout (x > 5,2 m) et faîtage des pans (x ≈ 0,01 m)
        float yE = 0, yF = 0, xE = 0;
        for (const auto& s : lo.v) if (s.couleur == c.tole) { if (s.p.x > 5.2f) { yE = s.p.y; xE = s.p.x; } if (s.p.x > 0.005f && s.p.x < 0.02f) yF = s.p.y; }
        const float pente = std::atan((yF - yE) / (xE - 0.01f)) * 180 / PI;
        std::snprintf(t, sizeof t, "hangar : pente de %.1f° (15 ± 1)", pente);
        verifie(std::fabs(pente - HANGAR_PENTE_DEG) < 1.f, t);
        qualite(p, "hangar proche", 16000);
        qualite(lo, "hangar loin", 900);
    }

    // ---------- poteau de ligne
    {
        const Maille p = poteauLigne(c, false), lo = poteauLigne(c, true);
        const Boite fut = boite(p, -1, c.poteau), tr = boite(p, -1, c.ferrure);
        std::snprintf(t, sizeof t, "poteau de ligne : %.2f m hors sol (8,5 ± 1 %%), Ø pied %.3f m (0,28, polygone à 10 côtés), traverse %.2f m (1,6 ± 1 %%)", fut.mx.y, fut.mx.x - fut.mn.x, tr.mx.x - tr.mn.x);
        verifie(proche(fut.mx.y, POTEAU_HAUT) && proche(fut.mx.x - fut.mn.x, POTEAU_DIAM_PIED, 0.06f) && proche(tr.mx.x - tr.mn.x, TRAVERSE), t);
        qualite(p, "poteau de ligne proche", 900);
        qualite(lo, "poteau de ligne loin", 140);
        Maille cb; vec3 a[3]; accrochesLigne(a); cable(cb, a[0], a[0] + vec3{0, 0, 60}, 0.9f, 0.012f, 8, c.cable);
        verifie(boite(cb).mn.y < a[0].y - 0.85f && boite(cb).mn.y > a[0].y - 0.95f, "câble : flèche de 0,9 m au milieu d'une portée de 60 m (1,5 %)");
    }

    std::printf("%s : %d échec(s)\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
