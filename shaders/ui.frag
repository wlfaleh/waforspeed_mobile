#version 450
layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;
void main() {
    float a = vColor.a;
    if (vUv.x >= 0.0) {   // disque / anneau : uv dans [0,1]
        float r = length(vUv * 2.0 - 1.0);
        float w = fwidth(r);
        a *= 1.0 - smoothstep(1.0 - w, 1.0, r);
    }
    outColor = vec4(vColor.rgb * a, a);   // alpha prémultiplié
}
