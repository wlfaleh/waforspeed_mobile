// WaForSpeed — volant de la vue Cockpit (WFS-414) : voir vehicule_cockpit_volant.h.
#include "vehicule_cockpit_volant.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "vehicule_cockpit_formes.h"

namespace wfs::cockpit {

using namespace formes;

namespace {

// volant commun (WFS-197 à WFS-323), déplacé de vehicule_cockpit.cpp par WFS-414 sans aucun changement
void volantCommun(const InterieurDef& d, PiecesVolant& p) {
    const mat4 I = mat4::identity();
    const float R = d.volantD * 0.5f - d.janteEp * 0.5f, rt = d.janteEp * 0.5f;
    jante(p.jante, I, R, rt, d.meplat, 0, 2 * PI, 96, 14, d.meplatHaut);
    dalleFace(p.moyeu, translate({0, 0, -0.012f}), 0.15f, 0.115f, 0.05f, 0.045f, 0.018f, 0.85f);   // moyeu : coussin du sac gonflable, galbé (WFS-323)
    // branches : 3 (à 9 h, 3 h et 6 h) ou 2 (à 9 h et 3 h, SF90 : branches larges qui portent les commandes)
    const float hub = 0.07f;
    // WFS-202 : 4 branches (Range Rover Sport) : deux presque horizontales, deux en V vers le bas
    std::vector<float> angles = d.branches == 2 ? std::vector<float>{0.f, PI}
                              : d.branches == 4 ? std::vector<float>{0.10f, PI - 0.10f, -PI / 2 + 0.45f, -PI / 2 - 0.45f}
                                                : std::vector<float>{0.f, PI, -PI / 2};
    for (float a : angles) {
        const vec2 bout = ligneJante(a, R, d.meplat, d.meplatHaut);
        const float l = std::sqrt(bout.x * bout.x + bout.y * bout.y) - hub;
        const float large = d.branches == 2 ? 0.060f : d.branches == 4 ? (a < 0 ? 0.026f : 0.036f) : (a == -PI / 2 ? 0.032f : 0.040f);
        profile(p.branches, rotateZ(a), {hub, 0, -0.008f}, {hub + l, 0, -0.008f}, large, 0.016f, 0.007f, -1, large * 0.75f);   // WFS-323 : branche profilée
    }
    // coutures colorées en bas de la jante (M4 : bleu clair, bleu foncé, rouge), de part et d'autre de la branche basse
    const bool coutures = d.couture[0] || d.couture[1] || d.couture[2];
    for (int k = 0; k < 3; k++) if (d.couture[k])
        for (float cote : {-1.f, 1.f}) {
            const float a = -PI / 2 + cote * (0.11f + 0.05f * k);
            jante(p.coutures[k], translate({0, 0, 0.0005f}), R, rt * 1.03f, d.meplat, a - 0.024f, a + 0.024f, 3, 10);
        }
    // repère : M4 (boutons M1 / M2 rouges sur les branches), Golf (agrafe GTI rouge sur la branche basse), SF90 (rouge à midi)
    if (d.repereCol) {
        if (coutures || d.boutonsBranches) for (float sx : {-1.f, 1.f}) dalleFace(p.repere, translate({sx * 0.105f, 0.018f, 0.012f}), 0.022f, 0.014f, 0.008f, 0.005f, 0.002f);
        else if (d.manettino || d.repereHaut) dalleFace(p.repere, translate({0, R + rt * 0.2f, 0.004f}), 0.012f, rt * 2.1f, rt * 2.1f, 0.005f, 0.003f);
        else if (!d.demarreur) dalleFace(p.repere, translate({0, -R * 0.55f, 0.010f}), 0.045f, 0.012f, 0.004f, 0.005f, 0.0015f);   // Giulia : le rouge est le démarreur
    }
    if (d.palettes) for (float sx : {-1.f, 1.f}) dalleFace(p.palettes, translate({sx * (R * 0.80f), 0.02f, -0.045f}) * rotateZ(sx * 0.15f), 0.055f, 0.13f, 0.006f, 0.02f, 0.002f, 0.95f);
    // manettino en bas à droite, démarreur en bas à gauche (sur les branches) ; WFS-201 : démarreur seul (Giulia)
    if (d.manettino) dalleFace(p.manettino, translate({0.085f, -0.055f, 0.012f}), 0.044f, 0.044f, 0.016f, 0.022f, 0.005f, 0.9f);   // WFS-323 : boutons chanfreinés
    if (d.demarreur) dalleFace(p.manettino, translate({-0.085f, -0.055f, 0.010f}), 0.032f, 0.032f, 0.012f, 0.016f, 0.004f, 0.9f);
    // WFS-201 : molettes (Model 3) : deux roulettes de commande sur les branches horizontales, caoutchouc sombre
    if (d.molettes) for (float sx : {-1.f, 1.f}) dalle(p.molettes, translate({sx * 0.105f, 0.f, 0.014f}) * rotateZ(PI / 2), 0.022f, 0.022f, 0.010f, 0.011f, 0.003f);
}

} // namespace

float demiLargeurCoussin(const VolantSerie& s, float y) {
    const float t = std::clamp(0.5f - y / s.moyeuH, 0.f, 1.f);   // 0 en haut, 1 en bas
    return s.moyeuW * 0.5f * (1 - s.trapeze * t);
}

void construireVolant(const InterieurDef& d, PiecesVolant& p) {
    volantCommun(d, p);
}

Visiere ligneVisiere(const InterieurDef& d) {
    // casquette (visière) du combiné : avance de la lèvre vers le conducteur au-dessus de l'écran (WFS-323 : du pied du pare-brise
    // jusqu'à 7,5 cm devant l'écran, déplacé de vehicule_cockpit.cpp par WFS-414 ; RG-07 : avancée propre au volant de série)
    const float cw = d.combineW * 0.5f + 0.03f, top = d.combinePos.y + d.combineH * 0.5f + 0.02f;
    const float ev = 0.024f, cz = d.combinePos.z, cxv = d.combinePos.x, Yb = d.pareBriseY;
    Visiere v;
    v.ev = ev; v.largeur = 2 * cw;
    v.pts = {vec3{cxv, Yb - ev * 0.5f, cz - 0.10f}, vec3{cxv, top + 0.015f - ev * 0.5f, cz - 0.03f}, vec3{cxv, top + 0.012f - ev * 0.5f, cz + 0.04f},
             vec3{cxv, top + 0.004f - ev * 0.5f, cz + 0.068f}};
    return v;
}

} // namespace wfs::cockpit
