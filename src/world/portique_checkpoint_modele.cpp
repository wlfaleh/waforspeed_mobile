// WaForSpeed — WFS-562 : portique de checkpoint en treillis et bâche imprimée (voir portique_checkpoint_modele.h).
#include "portique_checkpoint_modele.h"
#include <algorithm>
#include <cmath>
#include "decor_campagne_modeles.h"   // poutre (prisme chanfreiné, WFS-372)

namespace wfs {
namespace portique_checkpoint {
namespace {

const vec3 HAUT{0, 1, 0}, AX{1, 0, 0}, AZ{0, 0, 1};
using portique_depart::ENTRAXE;
using portique_depart::MEMBRURE_D;

// œillet de laiton serti (anneau à collerette) d'axe ax, centre c
void oeillet(Maille& m, vec3 c, vec3 ax) {
    const float ri = OEILLET_DI / 2, re = OEILLET_DE / 2, e = 0.0015f;
    grandprix::revolution(m, c, ax, {{-e, ri}, {-e, re}, {e, re}, {e, ri}, {-e, ri}}, 8, LAITON, 0, 2 * PI, AX, false);
}

// tendeur à boule : sandow Ø 4 mm de l'œillet (c) autour de la membrure (axe en m), boule de Ø 25 mm derrière la membrure
void tendeur(Maille& m, vec3 c, vec3 membrure, vec3 arriere) {
    const vec3 boule = membrure + arriere * (MEMBRURE_D / 2 + 0.014f);
    grandprix::tube(m, c, boule, 0.002f, 4, ATTACHE);
    grandprix::revolution(m, boule, arriere, {{-0.0125f, 0.f}, {-0.009f, 0.009f}, {0.f, 0.0125f}, {0.009f, 0.009f}, {0.0125f, 0.f}}, 6, ATTACHE);
}

// projecteur LED : bras serré sur la membrure (collier), lyre, boîtier à ailettes, vitre ; o : centre de la vitre, d : visée
void projecteur(Maille& m, vec3 pince, vec3 o, vec3 d) {
    const vec3 cote = normalize(cross(HAUT, d)), haut = normalize(cross(d, cote));
    // collier (demi-coupleur) autour de la membrure verticale, puis bras horizontal jusqu'à la lyre
    grandprix::revolution(m, pince, HAUT, {{-0.035f, MEMBRURE_D / 2 + 0.006f}, {0.035f, MEMBRURE_D / 2 + 0.006f}}, 12, ACIER);
    const vec3 dos = o - d * (PROJ_P + 0.05f);   // axe de la lyre, derrière le boîtier
    grandprix::tube(m, pince, dos, 0.017f, 8, ACIER);
    // lyre en U : traverse derrière le boîtier, deux bras jusqu'aux flancs (pivots)
    for (float s : {-1.f, 1.f}) {
        const vec3 flanc = o - d * (PROJ_P / 2) + cote * (s * (PROJ_W / 2 + 0.012f));
        grandprix::tube(m, dos + cote * (s * (PROJ_W / 2 + 0.012f)), flanc, 0.006f, 6, ACIER);
        grandprix::revolution(m, flanc, cote, {{-0.008f, 0.016f}, {0.008f, 0.016f}}, 10, ACIER);   // molette de serrage
    }
    grandprix::tube(m, dos - cote * (PROJ_W / 2 + 0.012f), dos + cote * (PROJ_W / 2 + 0.012f), 0.006f, 6, ACIER);
    // boîtier d'aluminium moulé, arêtes chanfreinées ; ailettes de refroidissement au dos ; cadre de la vitre
    const vec3 c = o - d * (PROJ_P / 2);
    campagne::poutre(m, c - d * (PROJ_P / 2), c + d * (PROJ_P / 2), haut, PROJ_W, PROJ_H, 0.012f, NOIR);
    for (int k = 0; k < 7; k++) {
        const float y = -PROJ_H / 2 + 0.03f + (PROJ_H - 0.06f) * k / 6;
        const vec3 a = c - d * (PROJ_P / 2) + haut * y;
        campagne::poutre(m, a, a - d * 0.025f, haut, PROJ_W - 0.04f, 0.004f, 0.f, NOIR);
    }
    const vec3 f = o + d * 0.0015f, u = haut * (PROJ_H / 2 - 0.02f), w = cote * (PROJ_W / 2 - 0.02f);
    grandprix::quadPlat(m, f - u - w, f - u + w, f + u + w, f + u - w, VERRE, d);
}

// bâche imprimée de face « avant » (+z ou −z), tendue entre les axes des deux poutres, sur la face des membrures
void bache(Modele& md, float L, float yBas, float yHaut, float z, float sens) {
    Maille& b = md.pieces[BACHE];
    const vec3 avant = AZ * sens;
    const float yc = (yBas + yHaut) / 2, h = yHaut - yBas;
    // toile (0,5 mm) et ourlet soudé (double épaisseur, 40 mm) sur le pourtour
    campagne::poutre(b, {-L / 2, yc, z}, {L / 2, yc, z}, HAUT, BACHE_EP, h, 0.f, BACHE);
    for (float y : {yBas + OURLET / 2, yHaut - OURLET / 2})
        campagne::poutre(b, {-L / 2, y, z}, {L / 2, y, z}, HAUT, 2 * BACHE_EP, OURLET, 0.f, BACHE);
    for (float x : {-L / 2 + OURLET / 2, L / 2 - OURLET / 2})
        campagne::poutre(b, {x, yBas + OURLET, z}, {x, yHaut - OURLET, z}, AZ, OURLET, 2 * BACHE_EP, 0.f, BACHE);
    // œillets tous les 0,5 m dans l'ourlet haut et bas, tendeurs autour des membrures de la face
    const int n = std::max(2, (int)std::lround(L / OEILLET_PAS)) + 1;
    const float zm = sens * ENTRAXE / 2, yPoutreBas = yBas, yPoutreHaut = yHaut;   // membrures de la face, sur l'axe des poutres ± entraxe / 2
    for (int k = 0; k < n; k++) {
        const float x = -L / 2 + 0.06f + (L - 0.12f) * k / (n - 1);
        for (float s : {-1.f, 1.f}) {
            const float y = s < 0 ? yBas + OURLET / 2 : yHaut - OURLET / 2;
            const vec3 c{x, y, z};
            oeillet(md.pieces[LAITON], c, avant);
            // membrure la plus proche de l'œillet : basse de la poutre basse (s < 0) ou haute de la poutre haute (s > 0)
            const vec3 mem{x, s < 0 ? yPoutreBas - ENTRAXE / 2 : yPoutreHaut + ENTRAXE / 2, zm};
            tendeur(md.pieces[ATTACHE], c, mem, avant * -1.f);
        }
    }
    // impression : damier noir et blanc aux deux bouts (cases de 0,30 m sur la hauteur imprimable), posée sur la toile
    const float zi = z + sens * (BACHE_EP / 2 + 0.003f), marge = 0.08f;   // encre à 3 mm : aucun scintillement de profondeur à 60 m
    const int rangs = std::max(1, (int)std::floor((h - 2 * marge) / DAMIER));
    const float y0 = yc - rangs * DAMIER / 2;
    for (float bout : {-1.f, 1.f})
        for (int i = 0; i < DAMIER_COLONNES; i++)
            for (int j = 0; j < rangs; j++) {
                const float xa = bout * (L / 2 - marge - i * DAMIER), xb = xa - bout * DAMIER, ya = y0 + j * DAMIER;
                const int piece = (i + j) % 2 ? ENCRE_BLANCHE : ENCRE_NOIRE;
                grandprix::quadPlat(md.pieces[piece], {xa, ya, zi}, {xb, ya, zi}, {xb, ya + DAMIER, zi}, {xa, ya + DAMIER, zi}, piece, avant);
            }
}

} // namespace

Modele construire(float ouverture, float largeurChaussee, bool sansPortique) {
    Modele md;
    md.ouverture = ouverture;
    // ligne de chronométrage peinte en travers de la chaussée (bande thermoplastique de 3 mm, arêtes adoucies)
    campagne::poutre(md.pieces[PEINTURE], {-largeurChaussee / 2, LIGNE_EP / 2, 0}, {largeurChaussee / 2, LIGNE_EP / 2, 0}, HAUT, LIGNE_L, LIGNE_EP, 0.001f, PEINTURE);
    if (sansPortique) return md;
    Maille& alu = md.pieces[ALU];
    const float xp = ouverture / 2 + SECTION / 2;                         // axe des pieds
    const float yA = HAUTEUR_LIBRE + SECTION / 2;                          // axe de la poutre basse
    const float yB = yA + SECTION + ENTRE_POUTRES;                         // axe de la poutre haute
    md.hauteurTotale = yB + SECTION / 2;
    for (float s : {-1.f, 1.f}) {
        portique_depart::platine(md.pieces[ACIER], s * xp, 0);
        // lest (avis de l'agent 3D, R3) : 2 plaques d'acier galvanisé de 600 × 150 × 25 mm (≈ 17,7 kg chacune) de part et d'autre
        // du pied, posées sur la platine, arêtes chanfreinées
        for (float sz : {-1.f, 1.f})
            for (int c = 0; c < LEST_PLAQUES; c++) {
                const float y = portique_depart::PLATINE_EP + LEST_EP * (c + 0.5f), z = sz * (SECTION / 2 + 0.005f + LEST_L / 2);   // dans l'emprise de la platine
                campagne::poutre(md.pieces[ACIER], {s * xp - portique_depart::PLATINE / 2, y, z}, {s * xp + portique_depart::PLATINE / 2, y, z}, HAUT, LEST_L, LEST_EP, 0.003f, ACIER);
            }
        portique_depart::treillis(alu, {s * xp, portique_depart::PLATINE_EP, 0}, {s * xp, md.hauteurTotale, 0}, AX, portique_depart::TRONCON_PIED, ALU);
    }
    // poutres d'un bout à l'autre : leurs extrémités couvrent les pieds (blocs d'angle)
    for (float y : {yA, yB}) portique_depart::treillis(alu, {-xp - SECTION / 2, y, 0}, {xp + SECTION / 2, y, 0}, HAUT, portique_depart::TRONCON_POUTRE, ALU);
    // bâches avant et arrière, sur les faces des membrures (à 3 mm), entre les faces intérieures des pieds (10 cm de jeu)
    md.bacheL = ouverture - 0.2f;
    const float zb = ENTRAXE / 2 + MEMBRURE_D / 2 + 0.003f;
    for (float sens : {1.f, -1.f}) bache(md, md.bacheL, yA, yB, sens * zb, sens);
    md.bacheCentre = {0, (yA + yB) / 2, zb + BACHE_EP / 2};
    // projecteurs : un par pied, au bout d'un bras de 0,6 m vers l'avant, tournés vers le milieu de leur moitié de bâche
    for (int k = 0; k < 2; k++) {
        const float s = k ? 1.f : -1.f;
        const vec3 pince{s * (xp - ENTRAXE / 2), PROJ_Y, ENTRAXE / 2};
        const vec3 cible{s * md.bacheL * 0.22f, (yA + yB) / 2, zb};
        const vec3 o0 = pince + AZ * (PROJ_BRAS + PROJ_P + 0.05f);
        const vec3 d = normalize(cible - o0);
        projecteur(md.pieces[NOIR], pince, o0 + d * PROJ_P, d);
        md.projecteurs[k] = o0 + d * PROJ_P; md.visee[k] = d;
    }
    // le projecteur met ses vitres dans la pièce NOIR (même appel) : on les déplace dans VERRE (matériau allumé la nuit)
    Maille& noir = md.pieces[NOIR];
    Maille& verre = md.pieces[VERRE];
    Maille reste;
    for (size_t t = 0; t + 2 < noir.i.size(); t += 3) {
        const bool vitre = noir.v[noir.i[t]].couleur == VERRE;
        Maille& dst = vitre ? verre : reste;
        for (int q = 0; q < 3; q++) { dst.i.push_back((uint32_t)dst.v.size()); dst.v.push_back(noir.v[noir.i[t + q]]); }
    }
    noir = std::move(reste);
    return md;
}

} // namespace portique_checkpoint
} // namespace wfs
