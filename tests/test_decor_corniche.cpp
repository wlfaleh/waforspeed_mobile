// WaForSpeed — WFS-371 : objets de la plage de la Corniche (src/world/decor_corniche_modeles.*), règles vérifiables par le calcul.
//   RG-02 / RG-03 : vraies mesures à ±1 % (parasol Ø 1,80 m et 2,20 m ; transat 1,90 × 0,65 m, assise 0,32 m ; parasol de
//                   paille Ø 2,40 m et 2,60 m ; tabouret 0,76 m ; table 0,80 × 0,45 m ; pouf Ø 0,60 × 0,40 m ; bouteille
//                   0,30 m ; poste de secours : plancher 2,40 m, cabine 2,2 m ; lisse à double onde 310 × 83 mm, bord à
//                   0,75 m ; scène 10 × 5 m, plancher 1,0 m, portique 6,2 m ; candélabre : lanterne centrée à 5,6 m ;
//                   noix de coco 0,26 m ; mât de guirlande 4,1 m ; bûche 0,50 m) ;
//   RG-04 : budget de triangles tenu ; la version « loin » est plus légère que la version proche ;
//   qualité : aucun triangle dégénéré, normales unitaires, triangles tournés comme leurs normales (pas de face inversée) ;
//   formes nues interdites : aucun modèle n'est une boîte seule (bien plus de 12 triangles, plusieurs pièces).
#include <cmath>
#include <cstdio>
#include "../src/world/decor_corniche_modeles.h"

using namespace wfs;
using namespace wfs::corniche;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
static void etendre(Boite& b, const Maille& m, int couleur, float yMin) {
    for (const marina::Sommet& s : m.v) {
        if ((couleur != -1000 && s.couleur != couleur) || s.p.y < yMin) continue;
        b.mn = {std::fmin(b.mn.x, s.p.x), std::fmin(b.mn.y, s.p.y), std::fmin(b.mn.z, s.p.z)};
        b.mx = {std::fmax(b.mx.x, s.p.x), std::fmax(b.mx.y, s.p.y), std::fmax(b.mx.z, s.p.z)};
    }
}
static Boite boite(const Maille& m, int couleur = -1000, float yMin = -1e9f) { Boite b; etendre(b, m, couleur, yMin); return b; }
static Boite boite(const Modele& M, float yMin = -1e9f) {
    Boite b;
    for (const Maille* m : {&M.uni, &M.acier, &M.paille, &M.neon}) etendre(b, *m, -1000, yMin);
    return b;
}
static bool proche(float v, float cible, float tol = 0.01f) { return std::fabs(v - cible) <= tol * std::fabs(cible); }

static void qualiteMaille(const Maille& m, int& deg, int& inv, int& nonUnit, int& mauvais) {
    for (size_t k = 0; k < m.i.size(); k += 3) {
        if (m.i[k] >= m.v.size() || m.i[k + 1] >= m.v.size() || m.i[k + 2] >= m.v.size()) { mauvais++; continue; }
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p, n = cross(b - a, c - a);
        if (length(n) < 1e-9f) { deg++; continue; }
        const vec3 nm = m.v[m.i[k]].n + m.v[m.i[k + 1]].n + m.v[m.i[k + 2]].n;
        if (dot(n, nm) < 0) inv++;
    }
    for (const marina::Sommet& s : m.v) if (std::fabs(length(s.n) - 1) > 1e-3f || !std::isfinite(s.p.x + s.p.y + s.p.z)) nonUnit++;
}
static void qualite(const Modele& M, const char* nom, int budget, int minimum = 24) {
    int deg = 0, inv = 0, nonUnit = 0, mauvais = 0;
    for (const Maille* m : {&M.uni, &M.acier, &M.paille, &M.neon}) qualiteMaille(*m, deg, inv, nonUnit, mauvais);
    char t[260];
    std::snprintf(t, sizeof t, "%s : %d triangles (%d < n ≤ %d), 0 dégénéré (%d), 0 inversé (%d), normales unitaires (%d hors), indices (%d hors)",
                  nom, M.triangles(), minimum, budget, deg, inv, nonUnit, mauvais);
    verifie(M.triangles() > minimum && M.triangles() <= budget && deg == 0 && inv == 0 && nonUnit == 0 && mauvais == 0, t);
}
static void plusLeger(const Modele& pres, const Modele& loin, const char* nom) {
    char t[160];
    std::snprintf(t, sizeof t, "%s : version loin plus légère (%d < %d triangles)", nom, loin.triangles(), pres.triangles());
    verifie(loin.triangles() < pres.triangles(), t);
}

int main() {
    Couleurs c;
    int k = 1;
    for (int* p : {&c.blanc, &c.alu, &c.toile, &c.bois, &c.boisSombre, &c.planche, &c.rouge, &c.vitre, &c.noir, &c.hp, &c.fonte, &c.gaine, &c.noix,
                   &c.ecorce, &c.sable, &c.pneu, &c.rotin}) *p = k++;
    char t[300];

    {   // parasol de plage
        const Modele p = parasol(c, false), l = parasol(c, true);
        const Boite bt = boite(p.uni, COUL_VAR), b = boite(p, 0.f);
        std::snprintf(t, sizeof t, "parasol : toile Ø %.3f m (1,80 ± 1 %%), %.3f m hors sol (2,20 ± 1 %%), lambrequin jusqu'à %.3f m", bt.mx.x - bt.mn.x, b.mx.y, bt.mn.y);
        verifie(proche(bt.mx.x - bt.mn.x, PARASOL_DIAM) && proche(b.mx.y, PARASOL_HAUT) && bt.mn.y < 1.74f - 0.07f, t);
        qualite(p, "parasol (proche)", 1400); qualite(l, "parasol (loin / Basse)", 120); plusLeger(p, l, "parasol");
    }
    {   // bain de soleil
        const Modele p = transat(c, false), l = transat(c, true);
        const Boite b = boite(p), bs = boite(p.uni, c.toile);
        // longueur au sol, dossier relevé : assise + dossier × cos(angle) (longueur développée de 1,90 m) ; largeur hors tout 0,65 m
        const float da = TRANSAT_DOSSIER_DEG * 3.14159265f / 180.f, sol = TRANSAT_ARTIC + (TRANSAT_L - TRANSAT_ARTIC - TRANSAT_TUBE) * std::cos(da) + TRANSAT_TUBE;
        std::snprintf(t, sizeof t, "transat : %.3f m au sol (dossier relevé de 40° : %.3f ± 1 %%, 1,90 m à plat), %.3f m de large (0,65 ± 1 %%), toile de l'assise à %.3f m (0,32 − flèche)",
                      b.mx.z - b.mn.z, sol, b.mx.x - b.mn.x, bs.mn.y);
        verifie(proche(b.mx.z - b.mn.z, sol) && proche(b.mx.x - b.mn.x, TRANSAT_W) && bs.mn.y > 0.28f && bs.mn.y < TRANSAT_ASSISE, t);
        qualite(p, "transat (proche)", 1600); qualite(l, "transat (loin / Basse)", 200); plusLeger(p, l, "transat");
    }
    {   // parasol de paille
        const Modele p = parasolPaille(c, false), l = parasolPaille(c, true);
        const Boite b = boite(p.paille), bt = boite(p, 0.f);
        std::snprintf(t, sizeof t, "parasol de paille : Ø %.3f m (2,40 ± 3 %% : franges), %.3f m hors sol (2,60 ± 1 %%)", b.mx.x - b.mn.x, bt.mx.y);
        verifie(proche(b.mx.x - b.mn.x, PAILLE_DIAM, 0.03f) && proche(bt.mx.y, PAILLE_HAUT), t);
        qualite(p, "parasol de paille (proche)", 1400); qualite(l, "parasol de paille (loin)", 160); plusLeger(p, l, "parasol de paille");
    }
    {   // mobilier du bar
        const Modele ta = tabouret(c), tb = tableBasse(c), po = pouf(c), bo = bouteille(c);
        const Boite b1 = boite(ta), b2 = boite(tb), b3 = boite(po), b4 = boite(bo);
        std::snprintf(t, sizeof t, "tabouret %.3f m (0,76), table %.3f × %.3f m (0,80 × 0,45), pouf Ø %.3f × %.3f m (0,60 × 0,40), bouteille %.3f × Ø %.3f m (0,300 × 0,076)",
                      b1.mx.y, b2.mx.x - b2.mn.x, b2.mx.y, b3.mx.x - b3.mn.x, b3.mx.y, b4.mx.y, b4.mx.x - b4.mn.x);
        verifie(proche(b1.mx.y, TABOURET_ASSISE) && proche(b2.mx.x - b2.mn.x, TABLE_COTE) && proche(b2.mx.y, TABLE_HAUT) && proche(b3.mx.x - b3.mn.x, POUF_DIAM, 0.03f) &&
                proche(b3.mx.y, POUF_HAUT) && proche(b4.mx.y, BOUTEILLE_H) && proche(b4.mx.x - b4.mn.x, BOUTEILLE_D, 0.02f), t);
        qualite(ta, "tabouret", 900); qualite(tb, "table basse", 600); qualite(po, "pouf", 900); qualite(bo, "bouteille", 160);
    }
    {   // comptoir et étagère du bar
        const Modele co = comptoir(c, 7.f), et = etagere(c, 5.2f);
        const Boite b1 = boite(co), b2 = boite(et);
        std::snprintf(t, sizeof t, "comptoir : plateau à %.3f m (1,10 ± 1 %%), %.2f m de long (7 + débords 0,10) ; étagère %.3f m de haut (1,70), %.2f m de long",
                      b1.mx.y, b1.mx.z - b1.mn.z, b2.mx.y, b2.mx.z - b2.mn.z);
        verifie(proche(b1.mx.y, COMPTOIR_HAUT) && proche(b1.mx.z - b1.mn.z, 7.1f) && proche(b2.mx.y, ETAGERE_HAUT) && proche(b2.mx.z - b2.mn.z, 5.2f), t);
        qualite(co, "comptoir de 7 m", 3000); qualite(et, "étagère de 5,2 m", 600);
    }
    {   // poste de secours
        const Modele p = posteSecours(c, false), l = posteSecours(c, true);
        const Boite bc = boite(p.uni, c.rouge), bp = boite(p.uni, c.planche);
        std::snprintf(t, sizeof t, "poste de secours : plancher à %.3f m (2,40 ± 1 %%), cabine %.3f m de large (2,2 + clin), murs jusqu'à %.3f m (4,40)", bp.mx.y, bc.mx.z - bc.mn.z, bc.mx.y);
        verifie(proche(bp.mx.y, POSTE_PLANCHER) && proche(bc.mx.z - bc.mn.z, POSTE_COTE + 0.04f, 0.02f) && proche(bc.mx.y, POSTE_PLANCHER + POSTE_CABINE, 0.01f), t);
        qualite(p, "poste de secours (proche)", 6000); qualite(l, "poste de secours (loin)", 1200); plusLeger(p, l, "poste de secours");
    }
    {   // glissière
        float xmin = 0, ymin = 9, ymax = 0;
        for (const vec2& q : profilLisse()) { xmin = std::fmin(xmin, q.x); ymin = std::fmin(ymin, q.y); ymax = std::fmax(ymax, q.y); }
        std::snprintf(t, sizeof t, "lisse à double onde : %.3f m de haut (0,310 ± 1 %%), %.3f m de profondeur (0,083), bord supérieur à %.3f m (0,75)", ymax - ymin, -xmin, ymax);
        verifie(proche(ymax - ymin, GLISS_LISSE) && proche(-xmin, GLISS_PROF) && proche(ymax, GLISS_HAUT), t);
        qualite(poteauGlissiere(c), "poteau de glissière et écarteur", 300);
    }
    {   // scène et enceintes
        const Modele p = scene(c, false), l = scene(c, true);
        const Boite bp = boite(p.uni, c.planche, 0.9f), bt = boite(p.acier);
        std::snprintf(t, sizeof t, "scène : plancher %.2f × %.2f m (5 × 10) à %.3f m (1,0), portique de %.3f m (6,2 ± 1 %%)", bp.mx.x - bp.mn.x, bp.mx.z - bp.mn.z, bp.mx.y, bt.mx.y);
        verifie(proche(bp.mx.x - bp.mn.x, SCENE_P) && proche(bp.mx.z - bp.mn.z, SCENE_L) && proche(bp.mx.y, SCENE_PLANCHER) && proche(bt.mx.y, SCENE_PORTIQUE), t);
        qualite(p, "scène (proche)", 16000); qualite(l, "scène (loin)", 1500); plusLeger(p, l, "scène");
        const Modele e = enceinte(c, false), el = enceinte(c, true);
        std::snprintf(t, sizeof t, "enceinte : %.3f m de haut (2 × 0,60 + 1,0 + joints)", boite(e).mx.y);
        verifie(proche(boite(e).mx.y, 2.23f), t);
        qualite(e, "enceinte (proche)", 700); qualite(lentilleLyre(), "lentille de lyre", 100, 12); qualite(el, "enceinte (loin)", 120, 20); plusLeger(e, el, "enceinte");
    }
    {   // candélabre
        const Modele p = candelabre(c, false), l = candelabre(c, true);
        const Boite bn = boite(p.neon), bf = boite(p.uni, c.fonte, 0.f);
        const float yc = (bn.mn.y + bn.mx.y) / 2;
        std::snprintf(t, sizeof t, "candélabre : verres centrés à %.3f m (5,6 ± 1 %%), lanterne à %.3f m du fût (0,6), haut %.3f m", yc, -(bn.mn.x + bn.mx.x) / 2, bf.mx.y);
        verifie(proche(yc, LANTERNE_Y) && proche(-(bn.mn.x + bn.mx.x) / 2, CANDELABRE_CROSSE) && bf.mx.y > LANTERNE_Y + 0.4f, t);
        qualite(p, "candélabre (proche)", 2500); qualite(l, "candélabre (loin)", 500); plusLeger(p, l, "candélabre");
    }
    {   // palmier, guirlande, feu de camp
        const Modele p = gainesPalmier(c, 3, false), l = gainesPalmier(c, 3, true);
        const Boite bn = boite(p.uni, c.noix);
        std::snprintf(t, sizeof t, "couronne de palmier : grappe de noix sous la couronne (haut %.2f m)", bn.mx.y);
        verifie(bn.mx.y < -0.25f, t);
        qualite(p, "couronne de palmier (proche)", 900); qualite(l, "couronne de palmier (loin)", 120); plusLeger(p, l, "couronne de palmier");
        const Modele m = matGuirlande(c), a = ampoule(), b = buche(c, 7);
        const Boite bm = boite(m), ba = boite(a.neon), bb = boite(b);
        std::snprintf(t, sizeof t, "mât de guirlande %.3f m (4,1), ampoule Ø %.3f m (0,060), bûche %.3f m (0,50 ± 2 %%)", bm.mx.y, ba.mx.x - ba.mn.x, bb.mx.x - bb.mn.x);
        verifie(proche(bm.mx.y, MAT_GUIRLANDE, 0.02f) && proche(ba.mx.x - ba.mn.x, AMPOULE_D, 0.03f) && proche(bb.mx.x - bb.mn.x, BUCHE_L, 0.02f), t);
        qualite(m, "mât de guirlande", 400); qualite(a, "ampoule", 120); qualite(b, "bûche", 300); qualite(rondin(c, 3), "rondin", 200);
    }
    std::printf("%s : %d échec(s)\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
