#version 450
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D fontAtlas;   // atlas de police (WFS-26)
void main() {
    float a = vColor.a;
    if (vUv.x >= 5.5) {   // WFS-551 : lettre en tube de néon (contour creux du champ de distance : onglet ouvert du garage)
        float d = texture(fontAtlas, vUv - vec2(6.0, 0.0)).r;
        float w = max(fwidth(d) * 0.75, 0.01);
        a *= smoothstep(0.38 - w, 0.38 + w, d) * (1.0 - smoothstep(0.56 - w, 0.56 + w, d));
        outColor = vec4(vColor.rgb * a, a);
        return;
    }
    if (vUv.x >= 3.5) {   // WFS-551 (RG-03) : icône du pistolet (masques G, B, A de l'atlas) : gris gardé, part colorée teintée
        vec4 m = texture(fontAtlas, vUv - vec2(4.0, 0.0));
        vec3 c = mix(vec3(m.g), vColor.rgb * min(1.0, m.g * 1.25), m.b);
        a *= m.a;
        outColor = vec4(c * a, a);
        return;
    }
    if (vUv.x >= 1.5) {   // caractère : champ de distance signé, bord net à toutes les tailles
        float d = texture(fontAtlas, vUv - vec2(2.0, 0.0)).r;
        float w = max(fwidth(d) * 0.75, 0.01);
        a *= smoothstep(0.5 - w, 0.5 + w, d);
    } else if (vUv.x >= 0.0) {   // disque / anneau : uv dans [0,1]
        float r = length(vUv * 2.0 - 1.0);
        float w = fwidth(r);
        a *= 1.0 - smoothstep(1.0 - w, 1.0, r);
    }
    outColor = vec4(vColor.rgb * a, a);   // alpha prémultiplié
}
