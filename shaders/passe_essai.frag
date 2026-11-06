#version 450
// WaForSpeed — passe d'essai (WFS-150) : lit la pré-passe et écrit une image de contrôle (jamais affichée en jeu) :
// moitié gauche = normales monde (× 0,5 + 0,5), moitié droite = distance (blanc près, noir à 400 m et au-delà, ciel noir)
layout(set = 0, binding = 0) uniform sampler2D normales;
layout(set = 0, binding = 1) uniform sampler2D profondeur;
layout(push_constant) uniform P { vec4 params; } pc;   // x : plan proche de la caméra (m)
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;
void main() {
    if (vUv.x < 0.5) {
        vec4 n = texture(normales, vec2(vUv.x * 2.0, vUv.y));
        outColor = vec4(n.rgb * n.a, 1.0);
    } else {
        float d = texture(profondeur, vec2(vUv.x * 2.0 - 1.0, vUv.y)).r;   // profondeur inversée, plan lointain à l'infini : d = proche / z
        float z = d > 0.0 ? pc.params.x / d : 1e9;
        outColor = vec4(vec3(clamp(1.0 - log2(1.0 + z) / log2(401.0), 0.0, 1.0)), 1.0);
    }
}
