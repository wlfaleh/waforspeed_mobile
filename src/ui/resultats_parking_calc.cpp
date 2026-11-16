// WaForSpeed — WFS-561 : placement du tableau LED et caméra de l'écran des résultats, calcul seul (aucun Vulkan : testé par
// tests/test_tableau_led.cpp). Voir resultats_parking.h.
#include <algorithm>
#include <cmath>
#include "resultats_parking.h"

namespace wfs::resultats3d {

float faceZ() { return Z_MASQUE - 0.0005f; }

vec3 positionPoint(int c, int r) {
    const float x0 = CENTRE_TABLEAU.x - (led::COLONNES - 1) * led::PAS / 2, y0 = CENTRE_TABLEAU.y + (led::RANGEES - 1) * led::PAS / 2;
    // face tournée vers −z (l'allée) : vue de l'allée, la colonne 0 est à gauche, donc vers +x (repère de la voiture, image
    // droite = −x pour une caméra qui regarde +z)
    return {x0 + (float)(led::COLONNES - 1 - c) * led::PAS, y0 - (float)r * led::PAS, faceZ()};
}

vec3 refletSol(const vec3& E, const vec3& P) {
    const vec3 m{P.x, -P.y, P.z};                 // image miroir sous le plan du sol
    const float t = E.y / std::max(1e-4f, E.y - m.y);
    return {E.x + (m.x - E.x) * t, 0.f, E.z + (m.z - E.z) * t};
}

Camera camera(float aspect) {
    // œil au bout de l'allée, à 9,5 m devant la voiture et un peu sur sa droite, à 1,45 m de haut ; regarde le mur du fond
    // au milieu du tableau. Cadrage ajusté sur la maquette 11 (image 2400 × 1080, en fraction de la demi-image) : tableau de
    // −0,44 à +0,26 en largeur et de −0,42 à +0,40 en hauteur ; voiture de +0,23 à +0,76 et de −0,82 à +0,04 ; obtenu ici :
    // tableau −0,38 à +0,21 et −0,27 à +0,38 ; voiture (Golf) +0,16 à +0,78 et −0,92 à +0,11. Angle vertical de 22° en 20:9 ;
    // les écrans moins larges gardent la même largeur vue (angle horizontal constant)
    Camera c;
    c.oeil = {1.25f, 1.45f, -9.5f};
    c.cible = {1.75f, 1.20f, Z_FOND};
    const float demiH = std::tan(11.f * PI / 180.f) * (2400.f / 1080.f) / std::max(1.f, aspect);
    c.fovY = 2.f * std::atan(demiH);
    c.decalageY = 0.f;
    if (banc::panneau) { c.oeil = {CENTRE_TABLEAU.x - 0.9f, 1.25f, faceZ() - 1.6f}; c.cible = {CENTRE_TABLEAU.x - 0.6f, 1.55f, faceZ()}; c.fovY = 30.f * PI / 180.f; }
    return c;
}

} // namespace wfs::resultats3d
