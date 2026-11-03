#version 450
// WaForSpeed — passe d'ombre du soleil (WFS-07) : profondeur vue du soleil
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inUv;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec2 vUv;
void main() {
    vUv = inUv;
    gl_Position = F.shadowMat * (pc.model * vec4(inPos, 1.0));
}
