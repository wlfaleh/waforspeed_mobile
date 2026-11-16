// WaForSpeed — WFS-559 : portique de départ en treillis et ses 5 feux (voir portique_depart_modele.h).
#include "portique_depart_modele.h"
#include <algorithm>
#include <cmath>

namespace wfs {
namespace portique_depart {
namespace {

const vec3 HAUT{0, 1, 0}, AX{1, 0, 0}, AZ{0, 0, 1};

// manchon conique d'assemblage autour d'une membrure (axe ax, centre c) et sa broche
void manchon(Maille& m, vec3 c, vec3 ax, vec3 travers, int couleur) {
    const float r = MEMBRURE_D / 2;
    grandprix::revolution(m, c, ax, {{-0.06f, r}, {-0.045f, 0.03f}, {0.045f, 0.03f}, {0.06f, r}}, 10, couleur);
    grandprix::tube(m, c - travers * 0.034f, c + travers * 0.034f, 0.006f, 6, couleur);   // broche Ø 12 mm (tête affleurante du manchon)
}

// caisson à arêtes arrondies : centre c, largeur w (x), hauteur h (y), profondeur p (z), rayon des arêtes rr
void caisson(Maille& m, vec3 c, float w, float h, float p, float rr, int couleur) {
    auto anneau = [&](float z, float retrait) {
        std::vector<vec3> pts;
        const float ax = w / 2 - rr, ay = h / 2 - rr, rc = std::max(0.002f, rr - retrait);
        const vec2 coins[4] = {{ax, ay}, {-ax, ay}, {-ax, -ay}, {ax, -ay}};
        for (int k = 0; k < 4; k++)
            for (int j = 0; j <= 3; j++) {
                const float a = (k + j / 3.f) * 1.5707963f;
                pts.push_back(c + vec3{coins[k].x + std::cos(a) * rc, coins[k].y + std::sin(a) * rc, z});
            }
        return pts;
    };
    const float ch = 0.012f;   // chanfrein des faces avant et arrière
    std::vector<std::vector<vec3>> an{anneau(-p / 2, ch), anneau(-p / 2 + ch, 0), anneau(p / 2 - ch, 0), anneau(p / 2, ch)};
    marina::loft(m, an, couleur, true, c - AZ * (p / 2), c + AZ * (p / 2));
    marina::eventail(m, an.front(), couleur, -AZ);
    marina::eventail(m, an.back(), couleur, AZ);
}

// platine de pied chanfreinée (acier galvanisé) et ses 4 boulons, centre au sol en (x, z)
void platine(Maille& m, float x, float z) {
    const float a = PLATINE / 2, ch = 0.03f;
    auto contour = [&](float y) {
        return std::vector<vec3>{{x + a, y, z + a - ch}, {x + a - ch, y, z + a}, {x - a + ch, y, z + a}, {x - a, y, z + a - ch},
                                 {x - a, y, z - a + ch}, {x - a + ch, y, z - a}, {x + a - ch, y, z - a}, {x + a, y, z - a + ch}};
    };
    const std::vector<vec3> bas = contour(0.f), haut = contour(PLATINE_EP);
    for (size_t k = 0; k < bas.size(); k++) {
        const size_t j = (k + 1) % bas.size();
        const vec3 mil = (bas[k] + bas[j]) * 0.5f, dehors = normalize(vec3{mil.x - x, 0, mil.z - z});
        grandprix::quadPlat(m, bas[k], bas[j], haut[j], haut[k], ACIER, dehors);
    }
    marina::eventail(m, haut, ACIER, HAUT);
    for (float sx : {-1.f, 1.f}) for (float sz : {-1.f, 1.f})   // boulons M24 : tête hexagonale de 36 mm sur plat, 15 mm de haut
        grandprix::revolution(m, {x + sx * (a - 0.07f), PLATINE_EP, z + sz * (a - 0.07f)}, HAUT,
                              {{0, 0.0205f}, {0.013f, 0.0205f}, {0.015f, 0.017f}}, 6, ACIER);
}

// lanterne de départ : bague, visière (demi-tube au-dessus de la lentille) ; face vers +z, centre de la lentille en c
void lanterne(Maille& noir, Maille& lentille, vec3 c) {
    const float r = FEU_D / 2;
    grandprix::revolution(noir, c, AZ, {{-0.01f, r + 0.018f}, {0.012f, r + 0.018f}, {0.016f, r + 0.008f}, {0.012f, r + 0.002f}, {-0.01f, r + 0.002f}},
                          16, NOIR, 0, 6.2831853f, AX, false);                                                              // bague
    grandprix::revolution(noir, c, AZ, {{0.01f, r + 0.02f}, {VISIERE, r + 0.035f}}, 10, NOIR, 0, 3.1415927f, AX, false);   // visière (dessus)
    // lentille bombée de polycarbonate, Ø 200 mm, légèrement en retrait de la bague
    grandprix::revolution(lentille, c, AZ, {{-0.005f, r}, {0.012f, r * 0.94f}, {0.026f, r * 0.66f}, {0.032f, 0.f}}, 16, LENTILLE);
}

} // namespace

void treillis(Maille& m, vec3 a, vec3 b, vec3 e1, float troncon, int couleur) {
    const vec3 d = b - a;
    const float L = length(d);
    if (L < 0.05f) return;
    const vec3 ax = d * (1.f / L);
    e1 = normalize(e1 - ax * dot(e1, ax));
    const vec3 e2 = cross(ax, e1);
    const float h = ENTRAXE / 2;
    const vec3 C[4] = {e1 * h + e2 * h, e1 * -h + e2 * h, e1 * -h + e2 * -h, e1 * h + e2 * -h};
    for (const vec3& c : C) grandprix::tube(m, a + c, b + c, MEMBRURE_D / 2, 8, couleur);   // membrures
    const float rD = DIAGONALE_D / 2;
    float s0 = 0;
    while (s0 < L - 1e-3f) {
        const float s1 = std::min(L, s0 + troncon);
        // traverses d'extrémité du tronçon (un peu en dedans des manchons)
        for (float s : {s0 + 0.08f, s1 - 0.08f})
            for (int f = 0; f < 4; f++) grandprix::tube(m, a + ax * s + C[f], a + ax * s + C[(f + 1) % 4], rD, 6, couleur);
        // diagonales en zigzag sur les 4 faces (sens alterné d'une face à l'autre : vrai treillis soudé)
        const float l = (s1 - 0.08f) - (s0 + 0.08f);
        const int n = std::max(1, (int)std::lround(l / PAS_DIAGONALE));
        for (int k = 0; k < n; k++) {
            const float sa = s0 + 0.08f + l * k / n, sb = s0 + 0.08f + l * (k + 1) / n;
            for (int f = 0; f < 4; f++) {
                const vec3 p = C[f], q = C[(f + 1) % 4];
                const bool pair = (k + f) % 2 == 0;
                grandprix::tube(m, a + ax * sa + (pair ? p : q), a + ax * sb + (pair ? q : p), rD, 6, couleur);
            }
        }
        if (s1 < L - 1e-3f) for (int f = 0; f < 4; f++) manchon(m, a + ax * s1 + C[f], ax, f % 2 ? e1 : e2, couleur);
        s0 = s1;
    }
}

Modele construire(float ouverture, float hauteurLibre, bool avecFeux) {
    Modele md;
    md.ouverture = ouverture; md.hauteurLibre = hauteurLibre;
    Maille& alu = md.pieces[ALU];
    Maille& noir = md.pieces[NOIR];
    const float xp = ouverture / 2 + SECTION / 2;          // axe des pieds
    const float yp = hauteurLibre + SECTION / 2;           // axe de la poutre
    const float yHaut = hauteurLibre + SECTION;            // dessus de la poutre (haut des pieds)
    for (float s : {-1.f, 1.f}) {
        platine(md.pieces[ACIER], s * xp, 0);
        treillis(alu, {s * xp, PLATINE_EP, 0}, {s * xp, yHaut, 0}, AX, TRONCON_PIED, ALU);
    }
    // poutre d'un bout à l'autre : ses extrémités couvrent le haut des pieds (bloc d'angle)
    treillis(alu, {-xp - SECTION / 2, yp, 0}, {xp + SECTION / 2, yp, 0}, HAUT, TRONCON_POUTRE, ALU);
    if (avecFeux) {
        const float yBas = hauteurLibre;                            // dessous de la membrure basse
        const float yc = yBas - 0.06f - CAISSON_H / 2;             // caisson pendu 60 mm sous la poutre
        const float zc = ENTRAXE / 2;                              // sous la membrure basse avant (côté pilotes)
        const vec3 c{0, yc, zc};
        caisson(noir, c, CAISSON_L, CAISSON_H, CAISSON_P, 0.015f, NOIR);
        // 3 colliers sur la membrure basse avant, plats de suspension jusqu'au dessus du caisson
        for (float x : {-2.0f, 0.f, 2.0f}) {
            const vec3 axeM{x, yp - ENTRAXE / 2, zc};
            grandprix::revolution(noir, axeM, AX, {{-0.025f, MEMBRURE_D / 2 + 0.006f}, {0.025f, MEMBRURE_D / 2 + 0.006f}}, 12, NOIR);
            grandprix::tube(noir, axeM - HAUT * (MEMBRURE_D / 2), {x, yc + CAISSON_H / 2, zc}, 0.012f, 6, NOIR);
        }
        const float zf = zc + CAISSON_P / 2;   // face avant du caisson
        for (int k = 0; k < NB_FEUX; k++)
            for (int j = 0; j < FEUX_PAR_COLONNE; j++) lanterne(noir, md.lentilles[k], {(k - 2) * FEU_PAS, yc + (j - 0.5f) * FEU_PAS_V, zf});
        md.centreFeux = {0, yc, zf + 0.05f};
    }
    return md;
}

} // namespace portique_depart
} // namespace wfs
