// WaForSpeed — masquage des objets (WFS-82, épic E4) : hors champ et au loin, pour moins d'appels de dessin.
// Équivalent natif du masquage automatique de three.js (Object3D.frustumCulled, sphère englobante) : la sphère
// englobante de chaque objet est testée contre les plans d'une matrice vue × projection (caméra, miroir des reflets,
// boîte d'ombre du soleil) ; les petits objets sont aussi coupés au loin (RG-02). Calcul seul, testé par
// tests/test_masquage.cpp.
#pragma once
#include <cmath>
#include "../core/math.h"

namespace wfs::masquage {

struct Sphere { vec3 c{}; float r = -1; };          // r < 0 : inconnue → jamais masquée
struct Plan { vec3 n{}; float d = 0; };             // n·p + d ≥ 0 : du bon côté
struct Frustum { Plan p[6]; bool actif = false; };

// plans de clip d'une matrice vue × projection Vulkan (x, y ∈ [−w, w], z ∈ [0, w]) : gauche, droite, bas, haut,
// z ≥ 0, z ≤ w ; convient à la caméra (profondeur inversée, plan lointain à l'infini) et à la boîte d'ombre (orthographique)
Frustum depuis(const mat4& viewProj);
// sphère entièrement du mauvais côté d'un plan → hors champ ; à cheval ou dedans → visible
bool visible(const Frustum& f, const Sphere& s);
// sphère d'un objet après sa matrice monde (rayon × plus grande échelle)
Sphere monde(const mat4& m, const Sphere& local);
// boîte (min, max) → sphère
inline Sphere deBoite(const vec3& mn, const vec3& mx) { return {(mn + mx) * 0.5f, length(mx - mn) * 0.5f}; }

// RG-02 : distance au-delà de laquelle un objet n'est plus dessiné (m) ; infini = jamais coupé par la distance.
// rayon < 3 m : 200 m (150 m en Basse) ; 3 à 10 m : fin du brouillard (420 m au plus en Basse) ; au-delà : jamais
float distanceMax(float rayon, bool basse, float finBrouillard);
// WFS-453 (ANO-247) : voitures et deux-roues du TRAFIC en Basse. Le trafic les choisit jusqu'à 120 m du JOUEUR
// (Traffic::dessines) ; la coupe des petits objets (110 m de la CAMÉRA, WFS-265) les perdait de 107 à 120 m en vue Poursuite
// (caméra 8 m derrière, 2,6 m au-dessus). Distance de la caméra gardée : 120 m + 8 m de recul + marge = 130 m ; c'est la
// sélection du trafic (120 m du joueur) qui borne. Haute et autres objets : distanceMax, inchangée.
constexpr float TRAFIC_BASSE_M = 130.f;
float distanceMaxTrafic(float rayon, bool basse, float finBrouillard);
// fin du brouillard (m) : Fog linéaire (fogFar) s'il existe, sinon distance où le brouillard exponentiel atteint 98 %
inline float finBrouillard(float fogFar, float densite) {
    if (fogFar > 0) return fogFar;
    return densite > 0 ? 3.912f / densite : 1e9f;   // 1 − exp(−d × densité) = 0,98
}

} // namespace wfs::masquage
