// WaForSpeed — données communes aux shaders de scène
layout(set = 0, binding = 0) uniform Frame {
    mat4 viewProj;
    mat4 invViewProj;
    vec4 camPos;      // xyz caméra, w temps (s)
    vec4 sunDir;      // xyz direction VERS le soleil, w intensité
    vec4 sunColor;    // rgb
    vec4 skyTop;      // rgb zénith
    vec4 skyHorizon;  // rgb horizon
    vec4 groundCol;   // rgb sol (lumière renvoyée)
    vec4 fog;         // rgb couleur, a densité
    vec4 misc;        // x exposition, y nuit (0/1), z intensité ambiante, w réflexions
} F;

// ciel analytique : sert à la fois pour le fond et pour les reflets (voitures, vitres)
vec3 skyColor(vec3 d, float sunK) {
    float h = clamp(d.y, -1.0, 1.0);
    vec3 col = mix(F.skyHorizon.rgb, F.skyTop.rgb, pow(clamp(h, 0.0, 1.0), 0.45));
    col = mix(col, F.groundCol.rgb * 0.6, smoothstep(0.0, -0.25, h));      // sous l'horizon : sol
    float s = max(dot(d, normalize(F.sunDir.xyz)), 0.0);
    col += F.sunColor.rgb * (pow(s, 900.0) * 60.0 * sunK + pow(s, 12.0) * 0.25) * F.sunDir.w;   // disque + halo
    return col;
}
vec3 skyColor(vec3 d) { return skyColor(d, 1.0); }
