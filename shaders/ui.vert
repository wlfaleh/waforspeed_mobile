#version 450
// interface 2D (pixels logiques, origine en haut à gauche)
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;
layout(push_constant) uniform P { vec4 rot; vec4 params; } pc;   // params.xy = 1 / taille logique
layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;
void main() {
    vec2 ndc = inPos * pc.params.xy * 2.0 - 1.0;
    vUv = inUv; vColor = inColor;
    gl_Position = vec4(mat2(pc.rot.xy, pc.rot.zw) * ndc, 0.0, 1.0);
}
