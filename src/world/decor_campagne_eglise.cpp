// WaForSpeed — WFS-410 (RG-04) : église de village de la Campagne (voir decor_campagne_eglise.h).
// Les murs percés en plein cintre (portail, baies, baies campanaires, claveaux, ébrasements, vitraux, vantaux et pentures) viennent
// de archi::lieux::murArcs (archi_lieux.h, commun aux lieux hors kit) ; la couverture de tuiles canal et la génoise, des outils de
// la ferme (decor_campagne_modeles.h).
#include "decor_campagne_eglise.h"
#include <cmath>
#include "archi_lieux.h"

namespace wfs::campagne {
namespace {

const vec3 Y{0, 1, 0};

// polygone plan tourné vers dehors
void poly(Maille& m, std::vector<vec3> p, int couleur, vec3 dehors) { marina::eventail(m, p, couleur, dehors); }

// murs d'une façade (repère de archi_lieux : origine o, axe x, normale z) recopiés dans la maille, une couleur par pièce
void versMaille(const archi::Batiment& b, vec3 o, vec3 x, vec3 z, const CouleursEglise& c, int vitrail, Maille& m, Maille& vitraux) {
    for (int k = 0; k < archi::NB_PARTS; k++) {
        archi::Maillage t = b.part[k];
        if (t.v.empty()) continue;
        archi::lieux::versMonde(t, o, x, z);
        Maille& dst = k == archi::VITRE ? vitraux : m;
        const int coul = k == archi::VITRE ? vitrail : k == archi::MENUISERIE ? c.porte : k == archi::METAL || k == archi::FIN ? c.fer
                       : k == archi::ENCADREMENT ? c.pierre2 : c.pierre;
        const uint32_t k0 = (uint32_t)dst.v.size();
        for (const archi::Sommet& s : t.v) dst.v.push_back({s.p, s.n, coul});
        for (uint32_t i : t.i) dst.i.push_back(k0 + i);
    }
}
// mur en plein cintre posé dans le repère de l'église : o coin bas gauche vu de dehors, z normale sortante
void mur(Maille& m, Maille& vitraux, const CouleursEglise& c, vec3 o, vec3 z, float largeur, float y0, float y1,
         const std::vector<archi::lieux::Arc>& arcs, float d, bool loin, int vitrail) {
    archi::Batiment b;
    archi::lieux::murArcs(largeur, y0, y1, arcs, d, loin ? 2 : 0, b);
    versMaille(b, o, archi::lieux::axeFacade(z), z, c, vitrail, m, vitraux);
}
// cloche de bronze (profil d'une cloche d'église courante, Ø 0,70 m à la pince, 0,62 m de haut, anses), suspendue par son
// mouton de bois (joug) au sommet de la baie ; o = haut de la cloche, joug selon jx
void cloche(Maille& m, vec3 o, vec3 jx, float portee, const CouleursEglise& c, bool loin) {
    // profil (hauteur depuis le haut, rayon) : cerveau, épaules, robe qui s'évase, pince épaisse, lèvre
    const std::vector<std::pair<float, float>> prof = {{0.f, 0.001f}, {0.f, 0.17f}, {0.04f, 0.20f}, {0.14f, 0.205f}, {0.30f, 0.22f},
                                                       {0.44f, 0.26f}, {0.54f, 0.31f}, {0.60f, 0.35f}, {0.62f, 0.345f}, {0.62f, 0.30f}};
    const int n = loin ? 8 : 16;
    std::vector<std::vector<vec3>> an;
    const vec3 e1 = jx, e2 = normalize(cross(jx, Y));
    for (auto [h, r] : prof) {
        std::vector<vec3> a;
        for (int k = 0; k < n; k++) { const float t = 2 * PI * k / n; a.push_back(o - Y * h + e1 * (std::cos(t) * r) + e2 * (std::sin(t) * r)); }
        an.push_back(a);
    }
    marina::loft(m, an, c.bronze, true, o, o - Y * 0.7f);
    // mouton (joug) de chêne de 0,22 × 0,26 m d'une tableau à l'autre, anses de bronze
    poutre(m, o - jx * (portee / 2) + Y * 0.16f, o + jx * (portee / 2) + Y * 0.16f, Y, 0.22f, 0.26f, loin ? 0 : 0.02f, c.bois);
    if (!loin) poutre(m, o + Y * 0.0f, o + Y * 0.05f, jx, 0.10f, 0.05f, 0.01f, c.bronze);
}

}  // namespace

Eglise eglise(const CouleursEglise& c, bool loin) {
    Eglise e;
    Maille& m = e.m;
    const float W = EGLISE_LARGEUR / 2, L = EGLISE_LONGUEUR / 2, H = EGLISE_MUR, P = EGLISE_PENTE, D = 0.40f;
    const float faite = H + W * P, ep = 0.12f, d = 0.6f;   // faîtage, épaisseur de la couverture, ébrasement des baies
    const float F = -3.f;   // fondations : les murs descendent 3 m sous le sol (terrain en pente du village : aucun vide sous l'église)
    using A = archi::lieux::Arc;
    // gouttereau du midi (vers la route, +x) : portail au milieu, deux baies ; x de la façade de z = +L vers z = −L
    mur(m, e.vitraux, c, {W, 0, L}, {1, 0, 0}, 2 * L, F, H,
        {A{L, 0.f, EGLISE_PORTAIL_L, EGLISE_PORTAIL_H, true}, A{L - 5.5f, 4.2f, 0.8f, 2.2f, false}, A{L + 5.5f, 4.2f, 0.8f, 2.2f, false}}, d, loin, c.vitrail[0]);
    // gouttereau nord : deux baies
    mur(m, e.vitraux, c, {-W, 0, -L}, {-1, 0, 0}, 2 * L, F, H, {A{L - 5.5f, 4.2f, 0.8f, 2.2f, false}, A{L + 5.5f, 4.2f, 0.8f, 2.2f, false}}, d, loin, c.vitrail[2]);
    // façade ouest (z = −L) : grande baie axiale ; chevet (z = +L) : mur plein derrière l'abside
    mur(m, e.vitraux, c, {W, 0, -L}, {0, 0, -1}, 2 * W, F, H, {A{W, 3.0f, 0.9f, 2.4f, false}}, d, loin, c.vitrail[1]);
    mur(m, e.vitraux, c, {-W, 0, L}, {0, 0, 1}, 2 * W, F, H, {}, d, loin, c.vitrail[1]);
    // pignons sous la couverture : est entier, ouest de part et d'autre du clocher-mur
    const float cl = EGLISE_CLOCHER_L / 2, hc = H + (W - cl) * P;
    poly(m, {{-W, H, L}, {W, H, L}, {0, faite, L}}, c.pierre, {0, 0, 1});
    for (const float s : {-1.f, 1.f}) poly(m, {{s * W, H, -L}, {s * cl, H, -L}, {s * cl, hc, -L}}, c.pierre, {0, 0, -1});
    // clocher-mur : face ouest (dans le plan de la façade) et face est, percées des deux baies campanaires, tranches et pignon
    const float Hc = EGLISE_CLOCHER_H, ec = EGLISE_CLOCHER_EP, yb = 10.4f, lb = 1.0f, hb = 2.0f;
    const std::vector<A> baies = {A{cl - 1.1f, yb, lb, hb, false, true}, A{cl + 1.1f, yb, lb, hb, false, true}};
    mur(m, e.vitraux, c, {cl, 0, -L}, {0, 0, -1}, 2 * cl, H, Hc, baies, ec, loin, c.vitrail[1]);
    mur(m, e.vitraux, c, {-cl, 0, -L + ec}, {0, 0, 1}, 2 * cl, hc - 0.6f, Hc, baies, 0, loin, c.vitrail[1]);
    for (const float s : {-1.f, 1.f}) poly(m, {{s * cl, H, -L}, {s * cl, H, -L + ec}, {s * cl, Hc, -L + ec}, {s * cl, Hc, -L}}, c.pierre, {s, 0, 0});
    const float pc = Hc + cl * 0.34f;   // pignon du clocher-mur (pente de 34 %)
    for (const float z : {-L, -L + ec}) poly(m, {{-cl, Hc, z}, {cl, Hc, z}, {0, pc, z}}, c.pierre, {0, 0, z < -L + ec / 2 ? -1.f : 1.f});
    for (const float s : {-1.f, 1.f}) {   // chaperon : dalles de pierre de 0,12 m débordant de 6 cm
        const vec3 a{s * (cl + 0.08f), Hc - 0.02f, -L + ec / 2}, b{0, pc + 0.05f, -L + ec / 2};
        poutre(m, a, b, normalize(vec3{-s * 0.34f, 1, 0}), ec + 0.12f, 0.12f, loin ? 0 : 0.015f, c.pierre2);
    }
    {   // croix de fer forgé de 1,2 m sur le pignon
        const vec3 b{0, pc + 0.1f, -L + ec / 2};
        poutre(m, b, b + Y * 1.2f, {0, 0, 1}, 0.05f, 0.05f, 0, c.fer);
        poutre(m, b + vec3{-0.35f, 0.82f, 0}, b + vec3{0.35f, 0.82f, 0}, Y, 0.05f, 0.05f, 0, c.fer);
    }
    for (const A& b : baies) {   // cloches au milieu de l'épaisseur du mur, sous le haut de la baie
        const float xb = cl - b.x;   // x de la façade ouest → x de l'église (axe de la façade : −x)
        cloche(m, {xb, b.y + b.h - b.l / 2 - 0.05f, -L + ec / 2}, {1, 0, 0}, b.l, c, loin);
    }
    // couverture de la nef : deux pans de tuiles canal, débord de 0,40 m en long, contre le clocher-mur à l'ouest, 0,30 m à l'est
    const float z0 = -L + 0.03f, z1 = L + 0.30f;
    for (const float s : {-1.f, 1.f}) {
        const vec3 n = normalize(vec3{s * P, 1, 0});
        const vec3 eg{s * (W + D), H + ep - D * P, 0}, fa{s * 0.02f, faite + ep, 0};
        if (loin) poly(m, {eg + vec3{0, 0, z0}, eg + vec3{0, 0, z1}, fa + vec3{0, 0, z1}, fa + vec3{0, 0, z0}}, c.tuile, n);
        else tuilesCanal(m, eg, fa, {0, 0, 1}, z0, z1, n, c.tuile);
        const vec3 eb = eg - Y * ep, fb = fa - Y * ep;
        poly(m, {eb + vec3{0, 0, z0}, eb + vec3{0, 0, z1}, fb + vec3{0, 0, z1}, fb + vec3{0, 0, z0}}, c.tuile, n * -1.f);
        poly(m, {eg + vec3{0, 0, z1}, fa + vec3{0, 0, z1}, fb + vec3{0, 0, z1}, eb + vec3{0, 0, z1}}, c.tuile, {0, 0, 1});   // rive est
        if (!loin) for (int r = 0; r < 2; r++) {   // génoise à deux rangs
            const float sail = r ? 0.13f : 0.27f, y = H - 0.05f - r * 0.1f;
            poutre(m, {s * (W + sail / 2), y, -L}, {s * (W + sail / 2), y, L}, Y, sail, 0.09f, 0.02f, c.tuile);
        }
    }
    {   // faîtage : tuiles faîtières en demi-rond (Ø 0,22 m)
        std::vector<std::vector<vec3>> an;
        for (const float z : {z0, z1}) {
            std::vector<vec3> sec;
            for (int k = 0; k <= 6; k++) { const float a = PI * k / 6; sec.push_back({std::cos(a) * 0.12f, faite + ep - 0.03f + std::sin(a) * 0.09f, z}); }
            an.push_back(sec);
        }
        marina::loft(m, an, c.tuile, false, {0, faite - 1, z0}, {0, faite - 1, z1});
    }
    // abside semi-circulaire : murs en 11 pans, baie axiale en plein cintre, toit de tuiles en éventail (11 pans à côtes de
    // tuiles de couvert), génoise
    {
        const int n = loin ? 5 : 11;   // nombre impair : un pan dans l'axe, percé de la baie axiale
        const float R = EGLISE_ABSIDE_R, Ha = EGLISE_ABSIDE_H, Da = 0.35f, sommet = Ha + R * P + ep;
        for (int k = 0; k < n; k++) {
            const float a0 = PI * k / n, a1 = PI * (k + 1) / n, am = (a0 + a1) / 2;
            const vec3 p0{R * std::cos(a0), 0, L + R * std::sin(a0)}, p1{R * std::cos(a1), 0, L + R * std::sin(a1)}, nn{std::cos(am), 0, std::sin(am)};
            if (k == n / 2)   // pan d'axe : baie en plein cintre de 0,5 × 1,6 m (ébrasement de 0,3 m), vitrail rubis
                mur(m, e.vitraux, c, p1, nn, length(p1 - p0), F, Ha, {A{length(p1 - p0) / 2, 3.0f, 0.5f, 1.6f, false}}, 0.3f, loin, c.vitrail[1]);
            else poly(m, {p0 + Y * F, p1 + Y * F, p1 + Y * Ha, p0 + Y * Ha}, c.pierre, nn);
            // pan de toit : de l'égout (débord Da) au sommet contre le chevet, tuiles en relief : 3 côtes par pan
            const vec3 e0{(R + Da) * std::cos(a0), Ha + ep - Da * P, L + (R + Da) * std::sin(a0)}, e1{(R + Da) * std::cos(a1), Ha + ep - Da * P, L + (R + Da) * std::sin(a1)};
            const vec3 sp{0, sommet, L};
            poly(m, {e0, e1, sp}, c.tuile, normalize(nn + Y * 2.f));
            if (!loin) {
                for (int q = 0; q < 3; q++) {   // tuiles de couvert : côtes de 6 cm le long de la pente
                    const float t = (q + 0.5f) / 3;
                    const vec3 eq = e0 + (e1 - e0) * t;
                    poutre(m, eq + Y * 0.03f, sp + (eq - sp) * 0.15f + Y * 0.03f, normalize(nn + Y * 2.f), 0.12f, 0.05f, 0.02f, c.tuile);
                }
                for (int r = 0; r < 2; r++) {   // génoise
                    const float sail = r ? 0.13f : 0.27f, y = Ha - 0.05f - r * 0.1f;
                    const vec3 g0{(R + sail / 2) * std::cos(a0), y, L + (R + sail / 2) * std::sin(a0)}, g1{(R + sail / 2) * std::cos(a1), y, L + (R + sail / 2) * std::sin(a1)};
                    poutre(m, g0, g1, Y, sail, 0.09f, 0.02f, c.tuile);
                }
            }
        }
    }
    // contreforts à glacis : deux par gouttereau, entre le portail et les baies
    for (const float s : {-1.f, 1.f}) for (const float z : {-2.9f, 2.9f}) {
        const float x0 = s * W, x1 = s * (W + 0.7f), yt = 5.0f;
        poutre(m, {(x0 + x1) / 2, F, z}, {(x0 + x1) / 2, yt, z}, {s, 0, 0}, 0.8f, 0.7f, loin ? 0 : 0.02f, c.pierre2);
        for (const float zz : {z - 0.4f, z + 0.4f}) poly(m, {{x0, yt, zz}, {x1, yt, zz}, {x0, yt + 1.0f, zz}}, c.pierre2, {0, 0, zz - z});
        poly(m, {{x1, yt, z - 0.4f}, {x1, yt, z + 0.4f}, {x0, yt + 1.0f, z + 0.4f}, {x0, yt + 1.0f, z - 0.4f}}, c.pierre2, normalize(vec3{s, 0.7f, 0}));
    }
    // emmarchement du portail : deux marches de pierre de 16 cm (giron 0,30 m) sous le seuil, massif jusqu'aux fondations (terrain en pente)
    for (int k = 0; k < 2; k++) {
        const float g = 0.30f * (2 - k), haut = -0.16f * (1 - k), bas = k ? -0.16f : F;
        poutre(m, {W + g / 2, (haut + bas) / 2, -1.7f + 0.15f * k}, {W + g / 2, (haut + bas) / 2, 1.7f - 0.15f * k}, Y, g, haut - bas, loin ? 0 : 0.01f, c.pierre2);
    }
    // soubassement : plinthe de 0,5 m en saillie de 5 cm, chanfreinée (sauf devant le portail)
    if (!loin) for (const float s : {-1.f, 1.f}) {
        const float x = s * (W + 0.025f);
        if (s > 0) {
            poutre(m, {x, 0.25f, -L}, {x, 0.25f, -EGLISE_PORTAIL_L / 2}, Y, 0.05f, 0.5f, 0.01f, c.pierre2);
            poutre(m, {x, 0.25f, EGLISE_PORTAIL_L / 2}, {x, 0.25f, L}, Y, 0.05f, 0.5f, 0.01f, c.pierre2);
        } else poutre(m, {x, 0.25f, -L}, {x, 0.25f, L}, Y, 0.05f, 0.5f, 0.01f, c.pierre2);
    }
    // pierre de taille : teintes du calcaire variées (bancs de 0,3 à 0,6 m), tuiles cuites inégalement
    if (!loin) {
        teinte(m, 0, c.pierre, {0.45f, 0.32f, 0.45f}, 0.10f, 113u);
        teinte(m, 0, c.tuile, {0.5f, 0.5f, 0.5f}, 0.10f, 71u);
    }
    return e;
}

}  // namespace wfs::campagne
