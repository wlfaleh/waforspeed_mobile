#version 450
// WaForSpeed — passe d'ombre du soleil pour un maillage animé par squelette (WFS-63) : variante de shadow.vert
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inUv;
layout(location = 3) in uvec4 inOs;
layout(location = 4) in vec4 inPoids;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(set = 2, binding = 0) uniform Os { mat4 os[64]; vec4 look[8]; } B;
layout(location = 0) out vec2 vUv;
void main() {
    mat4 s = B.os[inOs.x] * inPoids.x + B.os[inOs.y] * inPoids.y + B.os[inOs.z] * inPoids.z + B.os[inOs.w] * inPoids.w;
    vUv = inUv;
    gl_Position = F.ombreCasc[int(pc.emissive.x + 0.5)] * (pc.model * (s * vec4(inPos, 1.0)));   // WFS-227 : cascade de la tuile
}
