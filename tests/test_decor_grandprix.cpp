// WaForSpeed — WFS-373 : objets du Grand Prix (src/world/decor_grandprix_modeles.*), règles vérifiables par le calcul.
//   RG-02 / RG-03 : vraies mesures à ±1 % (mur de pneus : piles de 4 pneus 205/55 R16, Ø 632 mm ; panneau publicitaire de
//                   11,5 m ; siège coque 0,44 m, dossier à 0,83 m ; mur des stands 1,1 m + grillage 2,6 m ; stand de 7 m et
//                   auvent de 3 m ; mât de drapeau de 9 m ; panneau de freinage 1,6 × 1,2 m ; semi-remorque de 16,5 m × 4,0 m ;
//                   hélicoptère de 3,1 m de haut, rotor Ø 10,7 m ; tente de 1,3 m ; grillage de 1,9 m ; mât de projecteur de 18 m) ;
//   RG-04 : budget de triangles tenu ; la version « loin » est plus légère que la version proche ;
//   qualité : aucun triangle dégénéré, normales unitaires, triangles tournés comme leurs normales (pas de face inversée) ;
//   formes nues interdites : aucun modèle n'est une boîte seule (au moins 2 pièces et bien plus de 12 triangles).
#include <cmath>
#include <cstdio>
#include "../src/world/decor_grandprix_modeles.h"

using namespace wfs;
using namespace wfs::grandprix;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
static Boite boite(const Maille& m, int couleur = -1000, float yMin = -1e9f) {
    Boite b;
    for (const marina::Sommet& s : m.v) {
        if ((couleur != -1000 && s.couleur != couleur) || s.p.y < yMin) continue;
        b.mn = {std::fmin(b.mn.x, s.p.x), std::fmin(b.mn.y, s.p.y), std::fmin(b.mn.z, s.p.z)};
        b.mx = {std::fmax(b.mx.x, s.p.x), std::fmax(b.mx.y, s.p.y), std::fmax(b.mx.z, s.p.z)};
    }
    return b;
}
static bool proche(float v, float cible, float tol = 0.01f) { return std::fabs(v - cible) <= tol * cible; }

static void qualite(const Maille& m, const char* nom, int budget, int minimum = 24) {
    int deg = 0, inv = 0, nonUnit = 0, mauvais = 0;
    for (size_t k = 0; k < m.i.size(); k += 3) {
        if (m.i[k] >= m.v.size() || m.i[k + 1] >= m.v.size() || m.i[k + 2] >= m.v.size()) { mauvais++; continue; }
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p, n = cross(b - a, c - a);
        if (length(n) < 1e-9f) { deg++; continue; }
        const vec3 nm = m.v[m.i[k]].n + m.v[m.i[k + 1]].n + m.v[m.i[k + 2]].n;
        if (dot(n, nm) < 0) inv++;
    }
    for (const marina::Sommet& s : m.v) if (std::fabs(length(s.n) - 1) > 1e-3f || !std::isfinite(s.p.x + s.p.y + s.p.z)) nonUnit++;
    char t[260];
    std::snprintf(t, sizeof t, "%s : %d triangles (24 < n ≤ %d), 0 dégénéré (%d), 0 inversé (%d), normales unitaires (%d hors), indices (%d hors)",
                  nom, m.triangles(), budget, deg, inv, nonUnit, mauvais);
    verifie(m.triangles() > minimum && m.triangles() <= budget && deg == 0 && inv == 0 && nonUnit == 0 && mauvais == 0, t);
}
static void plusLeger(const Maille& pres, const Maille& loin, const char* nom) {
    char t[160];
    std::snprintf(t, sizeof t, "%s : version loin plus légère (%d < %d triangles)", nom, loin.triangles(), pres.triangles());
    verifie(loin.triangles() < pres.triangles(), t);
}

int main() {
    Couleurs c;
    int k = 1;
    for (int* p : {&c.pneu, &c.pneuRouge, &c.pneuBlanc, &c.sangle, &c.alu, &c.aluSombre, &c.beton, &c.betonNez, &c.acier, &c.tole,
                   &c.bardage, &c.verre, &c.menuiserie, &c.rideau, &c.interieur, &c.dalle, &c.enduit, &c.commissaire, &c.abri, &c.panneau, &c.mat,
                   &c.cabine, &c.chassis, &c.jante, &c.phare, &c.feuArriere, &c.toile, &c.bois, &c.fuselage, &c.pale, &c.plume, &c.plumeSombre}) *p = k++;
    char t[260];
    Maille neon;

    // ---------- mur de pneus
    {
        const Maille p = murPneus(c, false, 0), l = murPneus(c, true, 0);
        const Boite bp = boite(p, c.pneu), bh = boite(p, c.pneuBlanc);
        const float haut = PNEUS_PILE * PNEU_LARGEUR, pas = MUR_PNEUS_L / PILES_MUR, long_ = MUR_PNEUS_L - pas + PNEU_DIAM;
        std::snprintf(t, sizeof t, "mur de pneus : %.3f m de haut (4 pneus de 205 mm : %.3f ± 1 %%), %.3f m de long (7 piles au pas de %.3f m : %.3f ± 1 %%), Ø d'un pneu %.3f m (632 mm ± 1 %%)",
                      bh.mx.y, haut, bp.mx.z - bp.mn.z, pas, long_, bp.mx.x - bp.mn.x);
        verifie(proche(bh.mx.y, haut) && proche(bp.mx.z - bp.mn.z, long_) && proche(bp.mx.x - bp.mn.x, PNEU_DIAM), t);
        qualite(p, "mur de pneus (proche)", 2600); qualite(l, "mur de pneus (loin / Basse)", 840); plusLeger(p, l, "mur de pneus");
    }
    // ---------- panneau publicitaire
    {
        const Maille p = panneauPub(c, false), l = panneauPub(c, true);
        const Boite b = boite(p), bv = boite(p, COUL_VAR);
        std::snprintf(t, sizeof t, "panneau publicitaire : cadre %.2f m (11,5 ± 1 %%), bâche %.2f × %.2f m (11 × 1,2)", b.mx.z - b.mn.z, bv.mx.z - bv.mn.z, bv.mx.y - bv.mn.y);
        verifie(proche(b.mx.z - b.mn.z, PUB_L) && proche(bv.mx.z - bv.mn.z, PUB_BACHE_L) && proche(bv.mx.y - bv.mn.y, PUB_BACHE_H), t);
        qualite(p, "panneau publicitaire (proche)", 900); qualite(l, "panneau publicitaire (loin)", 300); plusLeger(p, l, "panneau publicitaire");
    }
    // ---------- sièges et tribune
    {
        const Maille s = siege(c);
        const Boite b = boite(s, COUL_VAR);
        std::snprintf(t, sizeof t, "siège coque : largeur %.3f m (0,44 ± 1 %%), haut du dossier %.3f m (0,83 ± 1 %%)", b.mx.z - b.mn.z, b.mx.y);
        verifie(proche(b.mx.z - b.mn.z, SIEGE_LARGEUR) && proche(b.mx.y, SIEGE_ASSISE + SIEGE_DOSSIER), t);
        qualite(s, "siège coque", 60);
        qualite(rangeeSieges(c, 30.f), "rangée de sièges (loin)", 16, 10);
        const Maille tp = tribune(c, 30.7f, 6, 0.85f, 0.42f, 0.9f, 10.f, 1.2f, false), tl = tribune(c, 30.7f, 6, 0.85f, 0.42f, 0.9f, 10.f, 1.2f, true);
        const Boite bt = boite(tp, c.beton);
        std::snprintf(t, sizeof t, "tribune : gradins jusqu'à %.2f m (muret 0,9 + 5 × 0,42 = 3,0 m ± 1 %%)", bt.mx.y);
        verifie(proche(bt.mx.y, 0.9f + 5 * 0.42f), t);
        qualite(tp, "tribune (proche)", 9000); qualite(tl, "tribune (loin)", 4000); plusLeger(tp, tl, "tribune");
    }
    // ---------- mur des stands et stand vitré
    {
        const Maille p = murStands(c, 3.6f, false), l = murStands(c, 3.6f, true);
        const Boite bb = boite(p, c.beton), ba = boite(p, c.acier);
        std::snprintf(t, sizeof t, "mur des stands : muret %.3f m (1,1 ± 1 %%), grillage jusqu'à %.2f m (1,1 + 2,6 + repli 0,4 + 0,06)", bb.mx.y, ba.mx.y);
        verifie(proche(bb.mx.y, MURET_STANDS) && proche(ba.mx.y, MURET_STANDS + GRILLAGE_STANDS + 0.4f + 0.057f, 0.02f), t);
        qualite(p, "mur des stands (proche)", 1200); qualite(l, "mur des stands (loin)", 300); plusLeger(p, l, "mur des stands");
        Maille ns;
        const Maille sp = stand(c, 16.3f, ns, false), sl = stand(c, 16.3f, ns, true);
        const Boite bs = boite(sp, c.dalle), bv = boite(sp, COUL_VAR);
        std::snprintf(t, sizeof t, "stand : toiture à %.2f m (7,0 + 0,3), auvent de %.2f m (3,0 ± 1 %%), linteau d'équipe de %.2f à %.2f m (porte 4,2 m)",
                      bs.mx.y, -bs.mn.x, bv.mn.y, bv.mx.y);
        verifie(proche(bs.mx.y, STAND_HAUT + 0.3f) && proche(-bs.mn.x, AUVENT) && proche(bv.mn.y, STAND_PORTE_H), t);
        qualite(sp, "stand vitré (proche)", 5000); qualite(sl, "stand vitré (loin)", 2500); plusLeger(sp, sl, "stand vitré");
    }
    // ---------- drapeaux, poste de commissaire, panneau de freinage
    {
        const Maille d = drapeau(c, false), dl = drapeau(c, true);
        const Boite bm = boite(d, c.mat), bv = boite(d, COUL_VAR);
        std::snprintf(t, sizeof t, "drapeau : mât %.2f m (9 m + pommeau 0,11), toile %.2f × %.2f m (2,4 × 1,6)", bm.mx.y, bv.mx.z - bv.mn.z, bv.mx.y - bv.mn.y);
        verifie(proche(bm.mx.y, MAT_DRAPEAU + 0.11f) && proche(bv.mx.z - bv.mn.z, DRAPEAU_L, 0.02f) && proche(bv.mx.y - bv.mn.y, DRAPEAU_H, 0.02f), t);
        qualite(d, "drapeau (proche)", 400); qualite(dl, "drapeau (loin)", 200); plusLeger(d, dl, "drapeau");
        qualite(drapeauMain(c), "drapeau tenu à la main", 80);
        const Maille pc = posteCommissaire(c, false);
        const Boite bp = boite(pc, c.acier, 0.9f);
        std::snprintf(t, sizeof t, "poste de commissaire : plancher à %.2f m (1,0 ± 1 %%), %.2f m de côté (2,0)", bp.mx.y, bp.mx.z - bp.mn.z);
        verifie(proche(bp.mx.y, COMMISSAIRE_PLANCHER) && proche(bp.mx.z - bp.mn.z, COMMISSAIRE_COTE), t);
        qualite(pc, "poste de commissaire", 2500);
        const Maille f = panneauFrein(c);
        const Boite bf = boite(f, c.panneau);
        std::snprintf(t, sizeof t, "panneau de freinage : %.2f × %.2f m (1,6 × 1,2 ± 1 %%), bas à %.2f m (1,0)", bf.mx.z - bf.mn.z, bf.mx.y - bf.mn.y, bf.mn.y);
        verifie(proche(bf.mx.z - bf.mn.z, FREIN_L) && proche(bf.mx.y - bf.mn.y, FREIN_H) && proche(bf.mn.y, FREIN_BAS), t);
        qualite(f, "panneau de freinage", 500);
    }
    // ---------- portique, passerelle
    {
        const Maille p = portique(c, 7.5f + 1.6f);
        const Boite b = boite(p, c.aluSombre);
        std::snprintf(t, sizeof t, "portique : %.2f m de haut (8,6 m), poutre jusqu'à ±%.2f m (pylônes à ±9,1 m)", b.mx.y, b.mx.x);
        verifie(b.mx.y > 8.5f && b.mx.y < 8.7f && proche(b.mx.x, 7.5f + 1.6f + 0.25f + 0.038f), t);
        qualite(p, "portique des feux", 12000);
        qualite(lentilleFeu(), "lentille de feu", 100);
        qualite(passerelle(c, 7.5f + 2.5f), "passerelle", 6000);
    }
    // ---------- camion
    {
        const Maille p = camion(c, false), l = camion(c, true);
        const Boite b = boite(p), bc = boite(p, c.cabine);
        std::snprintf(t, sizeof t, "camion : %.2f m de long (16,5 ± 1 %%), %.3f m de haut (4,0 ± 1 %%), caisse de %.3f m de large (2,55 ± 1 %%)", b.mx.z - b.mn.z, b.mx.y, bc.mx.x - bc.mn.x);
        verifie(proche(b.mx.z - b.mn.z, CAMION_L) && proche(b.mx.y, CAMION_HAUT) && proche(bc.mx.x - bc.mn.x, CAMION_LARGEUR), t);
        qualite(p, "camion (proche)", 9000); qualite(l, "camion (loin)", 5000); plusLeger(p, l, "camion");
    }
    // ---------- bar d'accueil, tour, hélicoptère
    {
        Maille n1, n2;
        const Maille p = accueil(c, 14, 30, 50, 0, n1, false), l = accueil(c, 14, 30, 50, 0, n2, true);
        qualite(p, "bar d'accueil (proche)", 12000); qualite(l, "bar d'accueil (loin)", 6000); plusLeger(p, l, "bar d'accueil");
        const Maille tp = tour(c, neon, false);
        const Boite bt = boite(tp, c.beton);
        std::snprintf(t, sizeof t, "tour de contrôle : fût de %.2f m (16 ± 1 %%)", bt.mx.y);
        verifie(proche(bt.mx.y, 16.f), t);
        qualite(tp, "tour de contrôle", 6000);
        const Maille h = helico(c, false), hl = helico(c, true);
        const Boite bh = boite(h), bf = boite(h, c.fuselage);
        float rotor = 0;
        for (const marina::Sommet& s : h.v) if (s.couleur == c.pale && s.p.y > 2.6f && std::fabs(s.p.z + 0.3f) < 6.f) rotor = std::fmax(rotor, std::hypot(s.p.x, s.p.z + 0.3f));
        std::snprintf(t, sizeof t, "hélicoptère : %.3f m de haut (3,1 ± 1 %%), fuselage de %.2f m (10,9 ± 1 %%), rotor Ø %.2f m (10,7 ± 1 %%)", bh.mx.y, bf.mx.z - bf.mn.z, 2 * rotor);
        verifie(proche(bh.mx.y, HELICO_HAUT) && proche(bf.mx.z - bf.mn.z, HELICO_L) && proche(2 * rotor, HELICO_ROTOR), t);
        qualite(h, "hélicoptère (proche)", 5000); qualite(hl, "hélicoptère (loin)", 3000); plusLeger(h, hl, "hélicoptère");
    }
    // ---------- espace fans, grande roue, tentes, grillage, oiseaux, projecteur
    {
        qualite(standFans(c, neon), "stand de l'espace fans", 800);
        qualite(portail(c, neon), "portail", 4000);
        const Maille r = roue(c, neon);
        const Boite br = boite(r);
        std::snprintf(t, sizeof t, "grande roue : rayon %.2f m (12 + tube)", br.mx.y);
        verifie(proche(br.mx.y, 12.15f), t);
        qualite(r, "grande roue", 6000);
        qualite(nacelle(c, 3), "nacelle", 400);
        qualite(piedsRoue(c), "pieds de la grande roue", 1200);
        for (int v = 0; v < 3; v++) {
            const Maille te = tente(c, v);
            const Boite bt = boite(te, COUL_VAR);
            std::snprintf(t, sizeof t, "tente %d : %.3f m de haut (1,3 ± 1 %%), %.2f m de long (2,6 ± 1 %%), %.2f m en travers avec l'avancée (2,2 à 2,4 + 0,7)", v, bt.mx.y, bt.mx.z - bt.mn.z, bt.mx.x - bt.mn.x);
            verifie(proche(bt.mx.y, TENTE_H) && proche(bt.mx.z - bt.mn.z, TENTE_L) && proche(bt.mx.x, TENTE_W * (1 + 0.04f * v) / 2) && bt.mx.x - bt.mn.x <= TENTE_W * 1.1f + 0.75f, t);
            qualite(te, "tente", 500);
        }
        const Maille g = grillage(c, false), gl = grillage(c, true);
        std::snprintf(t, sizeof t, "grillage : poteau de %.2f m (1,9 + chapeau)", boite(g).mx.y);
        verifie(proche(boite(g).mx.y, GRILLAGE_H + 0.03f), t);
        qualite(g, "grillage (proche)", 400); qualite(gl, "grillage (loin)", 60); plusLeger(g, gl, "grillage");
        qualite(vol(7, c.plume), "vol de 7 oiseaux", 450);
        Maille n3;
        const Maille pj = projecteur(c, n3, false), pl = projecteur(c, n3, true);
        std::snprintf(t, sizeof t, "projecteur : mât et rampe jusqu'à %.2f m (19 ± 1 %%)", boite(pj, c.acier).mx.y);
        verifie(proche(boite(pj, c.acier).mx.y, PROJ_MAT), t);
        qualite(pj, "projecteur (proche)", 1500); qualite(pl, "projecteur (loin)", 900); plusLeger(pj, pl, "projecteur");
    }
    qualite(neon, "néons (tour, fans, portail, roue)", 3000);
    std::printf("%s : %d échec(s)\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
