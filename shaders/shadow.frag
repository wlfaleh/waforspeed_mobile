#version 450
// WaForSpeed — passe d'ombre (WFS-07), matériaux à alpha découpé seulement (feuillage, grilles) : trous dans l'ombre
layout(location = 0) in vec2 vUv;
layout(set = 1, binding = 0) uniform sampler2D texColor;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
void main() {
    if (texture(texColor, vUv).a * pc.color.a < pc.params.z) discard;
}
