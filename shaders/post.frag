#version 450
// WaForSpeed — passe finale : exposition, tonemapping ACES, vignette, gamma, tramage
layout(set = 0, binding = 0) uniform sampler2D hdr;
layout(push_constant) uniform P { vec4 rot; vec4 params; } pc;   // params : x exposition, y sortie sRGB matérielle, z force du bloom, w seuil
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
    // bloom (WFS-15, UnrealBloomPass du jeu web, force et seuil d'ENV.bloom) : halo large autour des feux, néons, phares.
    // 2 couronnes de 8 lectures (rayons 1,6 % et 4,5 % de la hauteur) ; seule la part au-dessus du seuil de luminance
    // brille ; force 0 (qualité basse) : rien n'est lu, image identique
    if (pc.params.z > 0.0) {
        vec2 k = vec2(float(textureSize(hdr, 0).y) / float(textureSize(hdr, 0).x), 1.0);
        vec3 b = vec3(0.0); float poids = 0.0;
        for (int anneau = 0; anneau < 2; anneau++) {
            float rayon = anneau == 0 ? 0.016 : 0.045, w = anneau == 0 ? 1.0 : 0.6;
            for (int i = 0; i < 8; i++) {
                float a = (float(i) + 0.5 * float(anneau)) * 0.785398;
                vec3 s = min(texture(hdr, vUv + vec2(cos(a), sin(a)) * rayon * k).rgb * pc.params.x, vec3(24.0));
                float l = dot(s, vec3(0.2126, 0.7152, 0.0722));
                b += s * (max(l - pc.params.w, 0.0) / max(l, 1e-4)) * w; poids += w;
            }
        }
        c += b / poids * pc.params.z;
    }
    c = aces(c);
    vec2 q = vUv - 0.5;
    c *= 1.0 - dot(q, q) * 0.35;
    if (pc.params.y < 0.5) c = pow(c, vec3(1.0 / 2.2));
    c += (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) / 255.0;
    outColor = vec4(c, 1.0);
}
