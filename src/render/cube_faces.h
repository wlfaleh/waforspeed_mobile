// WaForSpeed — faces d'une cubemap (WFS-150, RG-03) : calcul seul (aucun appel Vulkan), testé par tests/test_passes.cpp.
// Convention des cubemaps Vulkan (spécification, « Cube Map Face Selection ») : face 0..5 = +X, −X, +Y, −Y, +Z, −Z ;
// texel (s, t) : +X (sc −z, tc −y), −X (+z, −y), +Y (+x, +z), −Y (+x, −z), +Z (+x, −y), −Z (−x, −y), s = (sc / |ma| + 1) / 2.
// Rendue avec ces axes et une image NON retournée, la rangée 0 de la face est du côté −haut : un shader échantillonne
// ensuite la direction monde telle quelle (samplerCube).
#pragma once
#include <algorithm>
#include "../core/math.h"

namespace wfs::environnement {

inline void axesFace(int f, vec3& avant, vec3& haut) {
    static const vec3 A[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    static const vec3 H[6] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};
    const int i = std::clamp(f, 0, 5);
    avant = A[i]; haut = H[i];
}

// vue-projection de la face f depuis p : 90°, carrée, profondeur inversée à l'infini, image non retournée
inline mat4 vueProjFace(int f, const vec3& p, float pres = 0.1f) {
    vec3 a, h; axesFace(f, a, h);
    mat4 P = perspectiveVk(PI / 2, 1.f, pres);
    P.at(1, 1) = -P.at(1, 1);
    return P * lookAt(p, p + a, h);
}

} // namespace wfs::environnement
