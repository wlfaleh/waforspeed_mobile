// WaForSpeed — roues animées (WFS-60, absorbe WFS-61 et WFS-62) : roues séparées dans les voitures, roulage et
// braquage. Port de js/cars/models.js du jeu web : splitWheels (découpe par coin LF / RF / LB / RB, médiane autour du
// centre de la roue), looksLikeWheel, rigWheels (centre et rayon de chaque roue) et animWheels (roulage = distance /
// rayon, braquage des roues avant avec épure d'Ackermann 1,12 / 0,9) ; js/world/scene.js (angle des roues du joueur
// et du trafic). Calcul seul (aucun appel Vulkan) : testé par tests/test_roues.cpp.
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"

namespace wfs {

// roue d'un modèle de voiture : centre (m, repère du modèle, avant vers −Z) et rayon
struct RoueModele { vec3 centre{}; float rayon = 0; bool avant = false; };
// pose des roues pour une image : roulage (rad, cumulé) et braquage des roues avant (rad, + = à droite)
struct RouesPose { float roulage = 0, braquage = 0; };

namespace roues {

constexpr float RW_DEFAUT = 0.34f;      // rayon de roue de référence (car.d.rw des voitures du catalogue)
enum Coin { LF = 0, RF = 1, LB = 2, RB = 3 };

// un maillage du modèle (un par matériau dans les .wfm) ; sommets : x, y, z au début de chaque sommet
struct Maillage {
    const float* sommets = nullptr; uint32_t stride = 8; uint32_t nSommets = 0;
    const uint32_t* idx = nullptr; uint32_t nIdx = 0;
    bool nomme = false;                  // matériau marqué « roue » par la conversion (drapeau 128)
};
struct Decoupe {
    RoueModele roues[4]; int n = 0;      // 4 roues trouvées, ou 0 (voiture inchangée)
    std::vector<std::vector<int8_t>> roueDuTriangle;   // par maillage, par triangle : coin 0-3, ou −1 (fixe)
};
// splitWheels + rigWheels ; puis, dans les matériaux non découpés (jantes fusionnées avec le chrome de la carrosserie,
// disques), les triangles entièrement dans le cylindre d'une roue, s'ils en font le tour (pas les étriers)
Decoupe decouper(const std::vector<Maillage>& m, float rw = RW_DEFAUT);

// matrice d'une roue : T(c) · Ry(−braquage × Ackermann) · Rx(roulage) · T(−c) (animWheels, groupes braquage puis rotation)
mat4 matrice(const RoueModele& r, const RouesPose& p);
// coefficient d'Ackermann : roue intérieure au virage 1,12, extérieure 0,9 (roues avant seulement)
float ackermann(const RoueModele& r, float braquage);

// angles du jeu web (scene.js) : joueur : wa = steer × 0,6 / (1 + |v| / 14), braquage = clamp(wa × 1,6, ±0,6) ;
// roulage = −distance / rayon (distance parcourue en m, signée)
RouesPose poseJoueur(double steer, double speedUnits, double distUnits, float rayon);
// trafic : braquage = cap de changement de file × 1,6 (0 après un accident), roulage selon la position sur la piste
RouesPose poseTrafic(double laneYaw, bool crashed, double zUnits, float rayon);

} // namespace roues
} // namespace wfs
