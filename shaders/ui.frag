#version 450
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D fontAtlas;   // atlas de police (WFS-26)
void main() {
    float a = vColor.a;
    if (vUv.x >= 1.5) {   // caractère : champ de distance signé, bord net à toutes les tailles
        float d = texture(fontAtlas, vUv - vec2(2.0, 0.0)).r;
        float w = max(fwidth(d) * 0.75, 0.01);
        // WFS-102 (RG-17, ANO-17) : petits corps (fwidth grand) légèrement épaissis, aucun trait fin ne disparaît
        float bord = 0.5 - min(w * 0.4, 0.06);
        a *= smoothstep(bord - w, bord + w, d);
    } else if (vUv.x >= 0.0) {   // disque / anneau : uv dans [0,1]
        float r = length(vUv * 2.0 - 1.0);
        float w = fwidth(r);
        a *= 1.0 - smoothstep(1.0 - w, 1.0, r);
    }
    outColor = vec4(vColor.rgb * a, a);   // alpha prémultiplié
}
