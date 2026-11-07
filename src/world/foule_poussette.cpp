// WaForSpeed — WFS-240 (RG-01) : géométrie de la poussette, voir foule_poussette.h.
#include "foule_poussette.h"
#include <cmath>
#include "decor_carte.h"   // primitives (tri, quad, box, cylAxe)
#include "vegetation.h"

namespace wfs::poussette {

namespace {
constexpr float PI_F = 3.14159265f;
const vec2 UV{0.5f, 0.5f};   // couleur unie : celle du matériau de la partie

// tube de a à b (rayon r, n côtés)
void tube(veg::Geo& g, vec3 a, vec3 b, float r, int n = 6) {
    const vec3 d = b - a;
    const float l = length(d);
    if (l > 1e-4f) carte::cylAxe(g, a, d * (1.f / l), r, l, n, UV);
}
// roue : pneu (cylindre couché selon x) centré en c, largeur w
void roue(veg::Geo& g, vec3 c, float r, float w) { carte::cylAxe(g, c - vec3{w * 0.5f, 0, 0}, {1, 0, 0}, r, w, 8, UV); }
} // namespace

int construire(veg::Geo* out) {
    veg::Geo &T = out[TISSU], &M = out[METAL], &P = out[PLASTIQUE];
    const float xR = LARGEUR * 0.5f - 0.03f, xF = xR - 0.05f;   // roues arrière (axe), roues avant un peu rentrées
    const vec3 X{1, 0, 0}, Z{0, 0, 1};
    // roues : arrière Ø 25 cm (4 cm de large), avant Ø 18 cm (3 cm), sous fourche pivotante
    for (float s : {-1.f, 1.f}) {
        roue(P, {s * xR, ROUE_AR, 0}, ROUE_AR, 0.04f);
        roue(P, {s * xF, ROUE_AV, ESSIEU_AVANT_Z}, ROUE_AV, 0.03f);
    }
    // châssis (aluminium, tubes de 2 cm) : jambe arrière de l'essieu à la poignée, jambe avant de la fourche au siège,
    // essieu arrière, barre de poignée
    for (float s : {-1.f, 1.f}) {
        const vec3 ar{s * (xR - 0.03f), ROUE_AR, 0}, av{s * (xF - 0.02f), ROUE_AV + 0.06f, ESSIEU_AVANT_Z - 0.02f};
        const vec3 poignee{s * MAINS_ECART * 0.5f, POIGNEE_Y - 0.02f, POIGNEE_Z + 0.02f}, siege{s * 0.2f, SIEGE_Y - 0.05f, SIEGE_Z};
        tube(M, ar, poignee, 0.012f);
        tube(M, av, siege, 0.012f);
        tube(M, siege, {s * 0.2f, SIEGE_Y + 0.12f, SIEGE_Z - 0.14f}, 0.01f);   // accoudoir vers le dossier
    }
    tube(M, {-(xR - 0.03f), ROUE_AR, 0}, {xR - 0.03f, ROUE_AR, 0}, 0.01f);   // essieu arrière
    tube(P, {-MAINS_ECART * 0.5f - 0.02f, POIGNEE_Y, POIGNEE_Z}, {MAINS_ECART * 0.5f + 0.02f, POIGNEE_Y, POIGNEE_Z}, 0.016f, 6);   // poignée en mousse
    // hamac (tissu) : assise 32 × 32 cm, dossier incliné de 25°, repose-pieds ; panier sous le siège
    carte::box(T, {0, 0, SIEGE_Z + 0.02f}, X, Z, 0.34f, 0.05f, 0.30f, UV, SIEGE_Y - 0.05f);
    {
        const float h = 0.42f, inc = 25.f * PI_F / 180.f;
        const vec3 b0{-0.17f, SIEGE_Y, SIEGE_Z - 0.13f}, b1{0.17f, SIEGE_Y, SIEGE_Z - 0.13f};
        const vec3 haut{0, h * std::cos(inc), -h * std::sin(inc)};
        carte::quad(T, b0, b1, b1 + haut, b0 + haut, UV);
        carte::quad(T, b1, b0, b0 + haut, b1 + haut, UV);   // envers (pas de face cachée vue de dos)
        // capote : demi-coque de 5 arceaux au-dessus du dossier
        const vec3 c0 = vec3{0, SIEGE_Y, SIEGE_Z - 0.13f} + haut;
        for (int k = 0; k < 6; k++) {
            const float a0 = PI_F * k / 6, a1 = PI_F * (k + 1) / 6;
            const vec3 p0{-0.19f * std::cos(a0), 0.19f * std::sin(a0), 0}, p1{-0.19f * std::cos(a1), 0.19f * std::sin(a1), 0};
            const vec3 av{0, -0.04f, 0.26f};   // la capote avance de 26 cm au-dessus de la tête du petit
            carte::quad(T, c0 + p0, c0 + p1, c0 + p1 + av, c0 + p0 + av, UV);
            carte::quad(T, c0 + p1, c0 + p0, c0 + p0 + av, c0 + p1 + av, UV);
        }
    }
    carte::box(T, {0, 0, 0.18f}, X, Z, 0.40f, 0.14f, 0.36f, UV, ROUE_AR + 0.04f);   // panier
    carte::box(T, {0, 0, SIEGE_Z + 0.24f}, X, Z, 0.26f, 0.03f, 0.10f, UV, SIEGE_Y - 0.26f);   // repose-pieds
    int n = 0;
    for (int k = 0; k < NB_PARTIES; k++) n += (int)out[k].i.size() / 3;
    return n;
}

} // namespace wfs::poussette
