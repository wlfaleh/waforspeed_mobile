#version 450
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec2 inUv;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNrm;
layout(location = 2) out vec2 vUv;
void main() {
    vec4 w = pc.model * vec4(inPos, 1.0);
    vWorld = w.xyz;
    vNrm = mat3(pc.model) * inNrm;
    vUv = inUv;
    gl_Position = F.viewProj * w;
}
