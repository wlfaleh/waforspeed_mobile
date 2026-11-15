// WaForSpeed — WFS-552 (RG-04) : cadrage de la caméra rapprochée du menu (voir menu_parking_cadrage.h).
#include "menu_parking_cadrage.h"
#include <algorithm>
#include <cmath>

namespace wfs::ecrans::parking {
namespace {
// maquette 01 (2400 × 1080) : la voiture occupe 49 % de la hauteur de l'image (du capot au bas des roues) et 44 % de sa largeur,
// son centre à 58 % de la largeur et à 57 % de la hauteur (depuis le haut) ; la console du menu tient le tiers gauche
constexpr float DEMI_HAUTEUR = 0.57f;   // demi-hauteur visée (NDC) : avis de l'agent 3D (PR #421), 15 % plus près que la maquette mesurée
constexpr float DEMI_LARGEUR_MAX = 0.54f;   // demi-largeur au plus (NDC) : la voiture ne passe pas sous la console
constexpr float CENTRE_X = 0.20f, CENTRE_Y = -0.12f;   // centre visé (NDC, y vers le haut)
constexpr float OEIL_Y = 1.0f;          // caméra à 1 m du sol, horizontale : verticales droites (piliers), objectif décentré

Emprise mesurer(const Cadrage& c, float L, float W, float H, float aspect) {
    const mat4 V = lookAt(c.oeil, c.cible, {0, 1, 0});
    const float f = 1.f / std::tan(c.fovY / 2);
    Emprise e{9, -9, 9, -9};
    for (int k = 0; k < 8; k++) {
        const vec3 p{(k & 1 ? 0.5f : -0.5f) * W, k & 2 ? H : 0.f, (k & 4 ? 0.5f : -0.5f) * L};
        const vec3 q = V.transformPoint(p);
        const float w = std::max(0.05f, -q.z);
        const float x = f / aspect * q.x / w + c.decalage, y = f * q.y / w + c.decalageY;
        e.xmin = std::min(e.xmin, x); e.xmax = std::max(e.xmax, x); e.ymin = std::min(e.ymin, y); e.ymax = std::max(e.ymax, y);
    }
    return e;
}
} // namespace

Cadrage cadrage(float L, float W, float H, float aspect, float cap) {
    // trois quarts avant gauche (cap : angle depuis l'axe de la voiture), objectif de 34° de champ vertical, caméra horizontale à
    // 1 m ; la distance est cherchée pour que la voiture ait la taille de la maquette (hauteur), sans dépasser la largeur permise,
    // puis l'objectif est décentré (comme un objectif à bascule) pour la placer à droite de la console, sans pencher la caméra
    Cadrage c;
    c.fovY = 34.f * PI / 180.f;
    c.cible = {0, OEIL_Y, 0};
    float d = 2.5f * L;
    for (int it = 0; it < 6; it++) {
        c.oeil = c.cible + vec3{-std::sin(cap) * d, 0, -std::cos(cap) * d};
        c.decalage = c.decalageY = 0;
        const Emprise e = mesurer(c, L, W, H, aspect);
        const float h = (e.ymax - e.ymin) / 2, l = (e.xmax - e.xmin) / 2;
        d *= std::max(h / DEMI_HAUTEUR, l / DEMI_LARGEUR_MAX);   // taille apparente ≈ 1 / distance
    }
    c.oeil = c.cible + vec3{-std::sin(cap) * d, 0, -std::cos(cap) * d};
    c.decalage = c.decalageY = 0;
    const Emprise e = mesurer(c, L, W, H, aspect);
    c.decalage = CENTRE_X - (e.xmin + e.xmax) / 2;
    c.decalageY = CENTRE_Y - (e.ymin + e.ymax) / 2;
    return c;
}

Emprise emprise(const Cadrage& c, float L, float W, float H, float aspect) { return mesurer(c, L, W, H, aspect); }

} // namespace wfs::ecrans::parking
