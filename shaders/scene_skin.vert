#version 450
// WaForSpeed — maillage animé par squelette (WFS-63) : variante de scene.vert, mêmes sorties pour scene.frag
// (mêmes matériaux PBR, ombres et brouillard que les voitures). Skinning linéaire : 4 os par sommet.
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec2 inUv;
layout(location = 3) in uvec4 inOs;
layout(location = 4) in vec4 inPoids;
layout(location = 5) in vec4 inCloth;   // WFS-72 : _cloth du jeu web
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(set = 2, binding = 0) uniform Os { mat4 os[64]; vec4 look[8]; } B;   // palette + apparence de l'instance (WFS-72)
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNrm;
layout(location = 2) out vec2 vUv;
layout(location = 3) out vec4 vCloth;   // WFS-72
layout(location = 4) out vec3 vBind;
void main() {
    mat4 s = B.os[inOs.x] * inPoids.x + B.os[inOs.y] * inPoids.y + B.os[inOs.z] * inPoids.z + B.os[inOs.w] * inPoids.w;
    vec4 w = pc.model * (s * vec4(inPos, 1.0));
    vWorld = w.xyz;
    vNrm = mat3(pc.model) * (mat3(s) * inNrm);
    vUv = inUv;
    vCloth = inCloth; vBind = inPos;
    gl_Position = F.viewProj * w;
}
