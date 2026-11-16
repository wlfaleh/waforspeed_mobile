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

mat4 parkingDepuisVoiture() { return rotateY(-LACET_VOITURE) * translate({-POSE_VOITURE.x, -POSE_VOITURE.y, -POSE_VOITURE.z}); }

Camera camera(float aspect) {
    // œil au bout de l'allée, à droite (10,4 m devant le voile, 5,1 m à droite du tableau), à 1,57 m de haut. Cadrage ajusté sur
    // la maquette 11 (image 2400 × 1080, en fraction de la demi-image ; 2e revue du lead dev) : maquette : tableau de −0,44 à
    // +0,26 en largeur et de −0,42 à +0,40 en hauteur, voiture de trois quarts de +0,23 à +0,76 et de −0,82 à +0,04, toit sous
    // le panneau SCORE (bas à +0,16, gauche à +0,61). Aux vraies mesures (tableau de 3,10 m, Golf de 4,29 m), une voiture vue de
    // trois quarts (35° entre son avant et l'œil) est plus large à l'image que le tableau posé à côté : le tableau ne peut pas
    // faire 35 % de la largeur comme sur la maquette (il en fait 25 %) sans mettre la voiture de face. Obtenu (Golf) : tableau
    // −0,37 à +0,14 et −0,37 à +0,29 ; voiture +0,08 à +0,82 et −0,78 à −0,03 (tout le toit sous le panneau SCORE ; test_tableau_led).
    // Angle vertical de 24,5° en 20:9 ; les écrans moins larges gardent la même largeur vue (angle horizontal constant)
    Camera c;
    c.oeil = {5.90f, 1.57f, -7.61f};
    c.cible = {0.07f, 1.54f, Z_FOND};
    const float demiH = std::tan(12.26f * PI / 180.f) * (2400.f / 1080.f) / std::max(1.f, aspect);
    c.fovY = 2.f * std::atan(demiH);
    c.decalageY = 0.f;
    if (banc::panneau) { c.oeil = {CENTRE_TABLEAU.x - 0.9f, CENTRE_TABLEAU.y - 0.05f, faceZ() - 1.6f}; c.cible = {CENTRE_TABLEAU.x - 0.6f, CENTRE_TABLEAU.y + 0.25f, faceZ()}; c.fovY = 30.f * PI / 180.f; }
    return c;
}

} // namespace wfs::resultats3d
