#version 450
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inTan;   // WFS-149 (RG-04) : tangente (xyz, signe de la bitangente), nulle = sans tangente
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNrm;
layout(location = 2) out vec2 vUv;
layout(location = 3) out vec4 vTan;
void main() {
    vec4 w = pc.model * vec4(inPos, 1.0);
    vWorld = w.xyz;
    vNrm = mat3(pc.model) * inNrm;
    vUv = inUv;
    vTan = vec4(mat3(pc.model) * inTan.xyz, inTan.w);
    gl_Position = F.viewProj * w;
}
