// WaForSpeed — données de l'image envoyées aux shaders de scène (même ordre que shaders/common.glsl).
// WFS-150 : déplacé tel quel de renderer.cpp, pour que les passes enfichables (passes.cpp, environnement.cpp) recopient
// l'image courante en changeant seulement la caméra (capture d'environnement).
#pragma once
#include "../core/math.h"

namespace wfs {

struct FrameUBO {
    mat4 viewProj, invViewProj;
    vec4 camPos, sunDir, sunColor, skyTop, skyHorizon, groundCol, fog, misc;
    mat4 shadowMat; vec4 shadowP;
    // WFS-45 : même ordre que shaders/common.glsl
    vec4 camFwd, fogLin, hemiSky, hemiGround, spotN;
    vec4 spotPos[8], spotDir[8], spotCol[8], spotCone[8];   // WFS-09 : 8 lumières
    vec4 nuitA, nuitB;                                      // WFS-09 / WFS-16 : route mouillée, reflets
    vec4 iblSH[9], iblA, iblB, cielP;                       // WFS-151 (accroche E13) : IBL, exposition, ciel (common.glsl)
    mat4 ombreCasc[4]; vec4 ombreFin, ombreDemi, ombreP;   // WFS-227 (accroche E13) : cascades d'ombre du soleil (common.glsl)
};

} // namespace wfs
