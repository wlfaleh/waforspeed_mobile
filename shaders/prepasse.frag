#version 450
// WaForSpeed — pré-passe profondeur + normales (WFS-150, RG-02) : demi-résolution, objets opaques de la scène.
// Sommets : scene.vert ou scene_skin.vert (mêmes sorties aux emplacements 0 à 2). Normale MONDE tournée vers la caméra
// (pièces double face), codée × 0,5 + 0,5 dans A2B10G10R10 ; a = 1 là où il y a un objet (0 : ciel).
#include "common.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNrm;
layout(location = 2) in vec2 vUv;
layout(set = 1, binding = 0) uniform sampler2D texColor;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec4 outN;
const uint F_MASK = 32u;
void main() {
    uint flags = uint(pc.params.w + 0.5);
    if ((flags & F_MASK) != 0u && texture(texColor, vUv).a * pc.color.a < pc.params.z) discard;   // feuillage, grilles (comme l'ombre)
    vec3 N = dot(vNrm, vNrm) > 1e-12 ? normalize(vNrm) : vec3(0.0, 1.0, 0.0);
    if (dot(N, F.camPos.xyz - vWorld) < 0.0) N = -N;
    outN = vec4(N * 0.5 + 0.5, 1.0);
}
