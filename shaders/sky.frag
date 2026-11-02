#version 450
#include "common.glsl"
layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;
void main() {
    vec4 a = F.invViewProj * vec4(vNdc, 1.0, 1.0);
    vec4 b = F.invViewProj * vec4(vNdc, 0.5, 1.0);
    vec3 d = normalize(b.xyz / b.w - a.xyz / a.w);
    vec3 col = skyColor(d);
    col = mix(col, F.fog.rgb, smoothstep(0.12, -0.02, d.y) * 0.7);   // brume à l'horizon
    outColor = vec4(col, 1.0);
}
