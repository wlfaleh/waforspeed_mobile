// WaForSpeed — WFS-558 : pavé du digicode et caméra du duel en ligne, calcul seul (testé par tests/test_salon_digicode.cpp).
#include "salon_parking.h"
#include <algorithm>
#include <cmath>

namespace wfs::ecrans::duel3d {
namespace {
// ordre des touches à l'écran : 2 à 9, puis les lettres (duel::CODE_CHARS sans 0, 1, I, L, O)
constexpr const char* ORDRE = "23456789abcdefghjkmnpqrstuvwxyz";
float doux(float t) { t = std::clamp(t, 0.f, 1.f); return t * t * (3 - 2 * t); }
} // namespace

// ---------------------------------------------------------------- pavé
char caractere(int k) { return k >= 0 && k < 31 ? ORDRE[k] : 0; }

void place(int k, int& col, int& largeur, int& ligne) {
    if (k < 31) { col = k % COLONNES; ligne = k / COLONNES; largeur = 1; return; }
    ligne = LIGNES - 1;   // dernière ligne : Z (col 0), ⌫ (col 1-2), OK (col 3-5)
    if (k == EFFACER) { col = 1; largeur = 2; } else { col = 3; largeur = 3; }
}

void touche(int k, vec3& c, vec2& demi) {
    int col = 0, l = 1, ligne = 0;
    place(k, col, l, ligne);
    const float x0 = -PAS * (COLONNES - 1) / 2;   // centre de la 1re colonne
    c = {x0 + PAS * ((float)col + (float)(l - 1) / 2), Y_LIGNE0 - PAS * (float)ligne, EPAIS + 0.002f};
    demi = {DEMI_TOUCHE + PAS * (float)(l - 1) / 2, DEMI_TOUCHE};
}

// ---------------------------------------------------------------- caméra
Camera camera(float t) {
    // cadrage large de la maquette 04 : œil dans l'allée à hauteur d'homme, voiture du joueur au milieu, poteau à gauche, place
    // voisine à droite ; vue rapprochée : 28 cm devant le digicode, qui remplit la hauteur (fov 40° : 0,19 m vus, digicode 0,18 m)
    const Camera large{{1.15f, 1.30f, -5.6f}, {0.50f, 1.02f, 0.6f}, 46.f * PI / 180.f};
    const Camera proche{{POTEAU_X - 0.02f, DIGI_Y + 0.012f, POTEAU_FACE - 0.28f}, {POTEAU_X, DIGI_Y, POTEAU_FACE - EPAIS}, 40.f * PI / 180.f};
    const float s = doux(t);
    return {lerp(large.oeil, proche.oeil, s), lerp(large.cible, proche.cible, s), large.fovY + (proche.fovY - large.fovY) * s};
}

float avancer(float t, bool proche, float dt) {
    const float pas = std::max(0.f, dt) / 0.40f;   // 0,4 s de bout en bout (RG-05) ; doux() adoucit la courbe
    return std::clamp(proche ? t + pas : t - pas, 0.f, 1.f);
}

} // namespace wfs::ecrans::duel3d
