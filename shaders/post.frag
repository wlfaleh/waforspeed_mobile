#version 450
// WaForSpeed — passe finale : exposition, tonemapping ACES, vignette, gamma, tramage
layout(set = 0, binding = 0) uniform sampler2D hdr;
layout(push_constant) uniform P { vec4 rot; vec4 params; } pc;   // params : x exposition, y sortie sRGB matérielle
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }
void main() {
    vec3 c = texture(hdr, vUv).rgb * pc.params.x;
    // léger halo autour des zones très lumineuses (soleil, reflets).
    // 4 × 4 lectures bilinéaires, chacune à cheval sur 2 × 2 pixels, espacées de 2 pixels : elles couvrent tout le
    // carré de 8 × 8 pixels sans trou (WFS-32 : l'ancienne grille de 5 × 5 lectures espacées de 3 pixels transformait
    // chaque pixel très brillant en une grille de points blancs). Chaque lecture est plafonnée pour qu'un seul pixel
    // extrême ne fasse pas une tache.
    vec2 px = 1.0 / vec2(textureSize(hdr, 0));
    vec3 glow = vec3(0.0);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
        vec3 s = texture(hdr, vUv + (vec2(i, j) * 2.0 - 3.5) * px).rgb * pc.params.x;   // -3,5 : à cheval entre 2 pixels
        glow += max(min(s, vec3(24.0)) - 2.5, 0.0);
    }
    c += glow / 16.0 * 0.35;
    c = aces(c);
    vec2 q = vUv - 0.5;
    c *= 1.0 - dot(q, q) * 0.35;
    if (pc.params.y < 0.5) c = pow(c, vec3(1.0 / 2.2));
    c += (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) / 255.0;
    outColor = vec4(c, 1.0);
}
