// WaForSpeed — règle d'éligibilité du shader léger (WFS-263 RG-01 ; isolée et testée par WFS-264 RG-06).
// Un objet est dessiné avec la variante LEGER de scene.frag quand AUCUN de ses pixels n'est en deçà de la fin du fondu
// (70 m : sphère englobante entièrement au-delà) et que son matériau n'est ni voiture, ni route, ni transparent, ni vitre
// d'immeuble (mêmes exclusions que F_COMPLET de shaders/scene.frag : les deux listes doivent rester identiques).
// Sans dépendance au moteur (drapeaux passés en entier) : testé seul par tests/test_shader_leger.cpp.
#pragma once
#include <cstdint>

namespace wfs::shader_leger {

constexpr float DEBUT_M = 60.f, FIN_M = 70.f;   // fondu du shader complet vers le léger (scene.frag, smoothstep)
// drapeaux Material (renderer.h) : PAINT 1, BLEND 2, TAIL 4, HEAD 8, GLASS 64, WHEEL 128, ROAD 256, FANTOME 131072, INTERIEUR 1048576
constexpr uint32_t COMPLET = 1u | 2u | 4u | 8u | 64u | 128u | 256u | 131072u | 1048576u;

// flags : drapeaux du matériau ; distance : de la caméra au centre de la sphère (m) ; rayon < 0 : sphère inconnue
inline bool eligible(uint32_t flags, float distance, float rayon) {
    if (flags & COMPLET) return false;
    if (rayon < 0.f || !(distance == distance)) return false;   // sphère inconnue ou distance invalide : shader complet
    return distance - rayon > FIN_M;
}

} // namespace wfs::shader_leger
