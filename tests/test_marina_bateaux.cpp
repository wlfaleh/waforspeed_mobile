// WaForSpeed — WFS-376 : bateaux de la marina (src/world/marina_bateaux.*), règles vérifiables par le calcul.
//   RG-04 : voilier de 9,6 m (±1 %), bau 3,4 m (±1 %), mât à 13-14 m au-dessus de l'eau (≈ 1,35 × L), bôme à 2,0 m du pont,
//           AUCUNE voile hissée (surface au-dessus de 4 m : mât, génois enroulé et câbles seulement, < 3 m²) ;
//   RG-05 : yacht de 18 m (±1 %), bau 4,9 m (±1 %) ;
//   RG-10 : ≤ 300 triangles par voilier proche, ≤ 250 par bateau à moteur (amarrage compris) ; au loin, coque ≤ 12 triangles ;
//   qualité : aucun triangle dégénéré, normales unitaires, surfaces tournées vers l'extérieur (pas de faces inversées).
#include <cmath>
#include <cstdio>
#include "../src/world/marina_bateaux.h"

using namespace wfs;
using namespace wfs::marina;

static int echecs = 0;
static void verifie(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "[OK]   " : "[ÉCHEC]", quoi);
    if (!ok) echecs++;
}

struct Boite { vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f}; };
static Boite boite(const Maille& m, float yMin = -1e9f) {
    Boite b;
    for (const Sommet& s : m.v) {
        if (s.p.y < yMin) continue;
        b.mn = {std::fmin(b.mn.x, s.p.x), std::fmin(b.mn.y, s.p.y), std::fmin(b.mn.z, s.p.z)};
        b.mx = {std::fmax(b.mx.x, s.p.x), std::fmax(b.mx.y, s.p.y), std::fmax(b.mx.z, s.p.z)};
    }
    return b;
}
static bool proche(float v, float cible, float tol) { return std::fabs(v - cible) <= tol * cible; }

// aucune face dégénérée ; normales unitaires ; la normale de chaque sommet va dans le sens de ses triangles (pas d'inversion)
static void qualite(const Maille& m, const char* nom) {
    int deg = 0, inv = 0, nonUnit = 0;
    for (size_t k = 0; k < m.i.size(); k += 3) {
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p, n = cross(b - a, c - a);
        if (length(n) < 1e-7f) { deg++; continue; }
        const vec3 nm = m.v[m.i[k]].n + m.v[m.i[k + 1]].n + m.v[m.i[k + 2]].n;
        if (dot(n, nm) < 0) inv++;
    }
    for (const Sommet& s : m.v) if (std::fabs(length(s.n) - 1) > 1e-3f) nonUnit++;
    char t[160];
    std::snprintf(t, sizeof t, "%s : %d triangles, 0 dégénéré (%d), 0 inversé (%d), normales unitaires (%d hors)", nom, m.triangles(), deg, inv, nonUnit);
    verifie(deg == 0 && inv == 0 && nonUnit == 0, t);
}

// surface (m²) des triangles entièrement au-dessus de y0 : une grand-voile hissée de 9,6 m fait ≈ 15 m², un génois ≈ 20 m²
static float surfaceAuDessus(const Maille& m, float y0) {
    float s = 0;
    for (size_t k = 0; k < m.i.size(); k += 3) {
        const vec3 a = m.v[m.i[k]].p, b = m.v[m.i[k + 1]].p, c = m.v[m.i[k + 2]].p;
        if (a.y > y0 && b.y > y0 && c.y > y0) s += 0.5f * length(cross(b - a, c - a));
    }
    return s;
}

int main() {
    Couleurs c; c.gelcoat = 1; c.pont = 2; c.vitre = 3; c.mat = 4; c.cable = 5; c.corde = 6; c.amarre = 7; c.beton = 8; c.bois = 9; c.flotteur = 10; c.fonte = 11;
    char t[200];

    const Bateau v = construire(VOILIER, c);
    const Boite bv = boite(v.pres, 0.f);
    const Boite cv = boite(v.pres, -0.01f);
    std::snprintf(t, sizeof t, "RG-04 voilier : longueur %.2f m (9,6 ±1 %%), bau %.2f m (3,4 ±1 %%)", cv.mx.z - cv.mn.z, 2 * cv.mx.x);
    verifie(proche(cv.mx.z - cv.mn.z, 9.6f, 0.01f) && proche(2 * cv.mx.x, 3.4f, 0.01f), t);
    std::snprintf(t, sizeof t, "RG-04 voilier : mât à %.2f m au-dessus de l'eau (13 à 14 m, %.2f × L)", bv.mx.y, bv.mx.y / 9.6f);
    verifie(bv.mx.y >= 13.f && bv.mx.y <= 14.f && bv.mx.y / 9.6f >= 1.30f && bv.mx.y / 9.6f <= 1.45f, t);
    std::snprintf(t, sizeof t, "RG-04 voilier : franc-bord %.2f à %.2f m (≈ 1,1 m)", v.francPoupe, v.francEtrave);
    verifie(v.francPoupe >= 0.95f && v.francEtrave <= 1.25f, t);
    const float sv = surfaceAuDessus(v.pres, 4.f);
    std::snprintf(t, sizeof t, "RG-04 voilier : aucune voile hissée (surface au-dessus de 4 m : %.2f m² < 3 m²)", sv);
    verifie(sv < 3.f, t);
    {   // bôme à 2,0 m au-dessus du pont : sommets de la couleur du mât entre 2,9 et 3,4 m, en arrière du mât
        float yb = 0; int n = 0;
        for (const Sommet& s : v.pres.v) if (s.couleur == c.mat && s.p.z < -1.0f && s.p.y > 2.5f && s.p.y < 4.f) { yb += s.p.y; n++; }
        yb = n ? yb / n : 0;
        std::snprintf(t, sizeof t, "RG-04 voilier : bôme à %.2f m au-dessus de l'eau (pont ≈ 1,1 m + 2,0 m)", yb);
        verifie(yb > 2.9f && yb < 3.5f, t);
    }
    const int tv = v.pres.triangles() + trianglesAmarrage(VOILIER, true);
    std::snprintf(t, sizeof t, "RG-10 voilier proche : %d triangles (modèle %d + amarrage %d) ≤ 300", tv, v.pres.triangles(), trianglesAmarrage(VOILIER, true));
    verifie(tv <= 300, t);
    std::snprintf(t, sizeof t, "RG-10 voilier lointain : %d triangles (coque 11 + mât en 2 quads croisés 4) ≤ 15", v.loin.triangles());
    verifie(v.loin.triangles() <= 15, t);
    qualite(v.pres, "voilier");

    const Bateau y = construire(YACHT, c);
    const Boite by = boite(y.pres, -0.01f);
    std::snprintf(t, sizeof t, "RG-05 yacht : longueur %.2f m (18 ±1 %%), bau %.2f m (4,9 ±1 %%), franc-bord %.2f m", by.mx.z - by.mn.z, 2 * by.mx.x, y.francPoupe);
    verifie(proche(by.mx.z - by.mn.z, 18.f, 0.01f) && proche(2 * by.mx.x, 4.9f, 0.01f) && y.francPoupe >= 1.85f, t);
    const int ty = y.pres.triangles() + trianglesAmarrage(YACHT, true);
    std::snprintf(t, sizeof t, "RG-10 yacht proche : %d triangles ≤ 250", ty);
    verifie(ty <= 250, t);
    qualite(y.pres, "yacht");
    const Bateau ve = construire(VEDETTE, c);
    std::snprintf(t, sizeof t, "RG-05 vedette (échelle 0,6) : %.1f m, %d triangles ≤ 250, pas de flybridge (haut %.2f m < yacht %.2f m)",
                  0.6f * (boite(ve.pres).mx.z - boite(ve.pres).mn.z), ve.pres.triangles() + trianglesAmarrage(VEDETTE, true), boite(ve.pres).mx.y, by.mx.y);
    verifie(ve.pres.triangles() + trianglesAmarrage(VEDETTE, true) <= 250 && boite(ve.pres).mx.y < by.mx.y - 0.8f, t);
    qualite(ve.pres, "vedette");
    {   // coque lointaine ≤ 12 triangles : 11 pour la coque, le reste est le salon ou le mât
        int coque = 0;
        for (size_t k = 0; k < y.loin.i.size(); k += 3) if (y.loin.v[y.loin.i[k]].couleur == COUL_COQUE) coque++;
        std::snprintf(t, sizeof t, "RG-10 au loin : coque de %d triangles ≤ 12 (bateau à moteur %d en tout)", coque, y.loin.triangles());
        verifie(coque <= 12, t);
    }
    for (Modele md : {VOILIER, VEDETTE, YACHT}) for (bool q : {true, false}) {   // budget annoncé = lignes réellement posées
        const Bateau b = construire(md, c);
        const int n = amarrage(b, md, md == VEDETTE ? 0.6f : 1.f, q ? 1.5f : -1.f, 1.45f, c).triangles();
        std::snprintf(t, sizeof t, "amarrage du modèle %d (%s) : %d triangles = %d annoncés", (int)md, q ? "à quai" : "au mouillage", n, trianglesAmarrage(md, q));
        verifie(n == trianglesAmarrage(md, q), t);
    }
    for (Modele md : {VOILIER, VEDETTE, YACHT}) {   // version moyenne (pontons, mouillage)
        const Bateau b = construire(md, c);
        std::snprintf(t, sizeof t, "version moyenne du modèle %d : %d triangles ≤ 140", (int)md, b.moyen.triangles());
        verifie(b.moyen.triangles() <= 140 && b.moyen.triangles() > 40, t);
        qualite(b.moyen, "version moyenne");
        const Boite bm = boite(b.moyen, -0.01f), bp = boite(b.pres, -0.01f);
        std::snprintf(t, sizeof t, "version moyenne du modèle %d : mêmes dimensions que le détail (%.2f / %.2f m, haut %.2f / %.2f m)", (int)md, bm.mx.z - bm.mn.z, bp.mx.z - bp.mn.z, bm.mx.y, bp.mx.y);
        verifie(proche(bm.mx.z - bm.mn.z, bp.mx.z - bp.mn.z, 0.01f) && proche(bm.mx.y, bp.mx.y, 0.02f), t);
    }
    const Maille bi = bitte(c), po = ponton(42, 1.45f, c, false);
    qualite(bi, "bitte d'amarrage");
    qualite(po, "ponton de 42 m");
    std::snprintf(t, sizeof t, "ponton : dessus à %.2f m au-dessus de l'eau (0,4 à 0,6 m), %.2f m de large (2,0 à 2,5 m)", PONTON_HAUT, PONTON_LARGEUR);
    verifie(PONTON_HAUT >= 0.4f && PONTON_HAUT <= 0.6f && PONTON_LARGEUR >= 2.f && PONTON_LARGEUR <= 2.5f, t);
    std::printf(echecs ? "\n%d échec(s)\n" : "\nTout est bon.\n", echecs);
    return echecs ? 1 : 0;
}
