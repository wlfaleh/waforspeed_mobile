#version 450
// WaForSpeed — matériaux PBR (GGX), peinture vernie, vitres, route et trottoirs procéduraux.
#include "common.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNrm;
layout(location = 2) in vec2 vUv;
layout(set = 1, binding = 0) uniform sampler2D texColor;
layout(set = 1, binding = 1) uniform sampler2D texNormal;
layout(set = 1, binding = 2) uniform sampler2D texMR;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;
const uint F_PAINT = 1u, F_BLEND = 2u, F_TAIL = 4u, F_HEAD = 8u, F_MASK = 32u, F_GLASS = 64u,
           F_ROAD = 256u, F_WALK = 512u, F_GROUND = 1024u, F_SHADOW = 2048u, F_HASNORMAL = 4096u, F_HASMR = 8192u;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p); vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) { float s = 0.0, a = 0.5; for (int i = 0; i < 4; i++) { s += a * noise(p); p *= 2.03; a *= 0.5; } return s; }

float D_GGX(float NoH, float a) { float a2 = a * a; float d = NoH * NoH * (a2 - 1.0) + 1.0; return a2 / (PI * d * d); }
float V_Smith(float NoV, float NoL, float a) {
    float k = a * 0.5; return 0.25 / max((NoV * (1.0 - k) + k) * (NoL * (1.0 - k) + k), 1e-4);
}
vec3 F_Schlick(vec3 f0, float VoH) { return f0 + (1.0 - f0) * pow(1.0 - VoH, 5.0); }

// repère tangent sans tangentes : dérivées écran (normal maps)
vec3 perturb(vec3 N, vec3 p, vec2 uv, vec3 tn) {
    vec3 dp1 = dFdx(p), dp2 = dFdy(p); vec2 du1 = dFdx(uv), du2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, N), dp1perp = cross(N, dp1);
    vec3 T = dp2perp * du1.x + dp1perp * du2.x, B = dp2perp * du1.y + dp1perp * du2.y;
    float inv = inversesqrt(max(max(dot(T, T), dot(B, B)), 1e-12));
    return normalize(mat3(T * inv, B * inv, N) * tn);
}

void main() {
    uint flags = uint(pc.params.w + 0.5);
    if ((flags & F_SHADOW) != 0u) {   // ombre de contact : rectangle aux bords très doux
        vec2 q = abs(vUv * 2.0 - 1.0);
        float a = (1.0 - smoothstep(0.55, 1.0, q.x)) * (1.0 - smoothstep(0.6, 1.0, q.y));
        outColor = vec4(0.0, 0.0, 0.0, a * pc.color.a);
        return;
    }
    vec3 N = normalize(vNrm);
    vec3 V = normalize(F.camPos.xyz - vWorld);
    if (!gl_FrontFacing) N = -N;   // pièces double face
    vec4 base = texture(texColor, vUv) * pc.color;
    float metal = pc.params.x, rough = pc.params.y;
    if ((flags & F_HASMR) != 0u) { vec4 mr = texture(texMR, vUv); rough *= mr.g; metal *= mr.b; }
    if ((flags & F_HASNORMAL) != 0u) N = perturb(N, vWorld, vUv, normalize(texture(texNormal, vUv).xyz * 2.0 - 1.0));
    if ((flags & F_MASK) != 0u && base.a < pc.params.z) discard;

    // ---------- surfaces procédurales (uv = mètres : x latéral, y le long de la route) ----------
    if ((flags & F_ROAD) != 0u) {
        vec2 p = vUv;
        float grain = fbm(p * 3.1) * 0.6 + fbm(p * 23.0) * 0.4;
        vec3 asphalt = mix(vec3(0.045, 0.047, 0.052), vec3(0.11, 0.11, 0.115), grain);
        asphalt *= 0.85 + 0.3 * fbm(p * vec2(0.08, 0.05));                         // zones usées / rapiécées
        float halfW = pc.emissive.w;                                                 // demi-largeur de chaussée (m)
        float edgeL = 1.0 - smoothstep(0.08, 0.1, abs(abs(p.x) - (halfW - 0.45)));  // lignes de rive continues
        float lane = 0.0;
        for (int k = -1; k <= 1; k += 2) {                                           // 3 voies : 2 lignes pointillées
            float lx = float(k) * halfW / 3.0;
            float dash = step(fract(p.y / 12.0), 0.45);
            lane = max(lane, (1.0 - smoothstep(0.06, 0.08, abs(p.x - lx))) * dash);
        }
        float paint = max(edgeL, lane) * (0.75 + 0.25 * noise(p * 6.0));
        base.rgb = mix(asphalt, vec3(0.78, 0.78, 0.74), paint);
        rough = mix(0.82 - grain * 0.15, 0.55, paint); metal = 0.0;
    } else if ((flags & F_WALK) != 0u) {
        vec2 p = vUv;
        vec2 tile = fract(p / 0.6);
        float joint = 1.0 - smoothstep(0.0, 0.04, min(min(tile.x, 1.0 - tile.x), min(tile.y, 1.0 - tile.y)));
        float stone = 0.32 + 0.1 * hash(floor(p / 0.6)) + 0.06 * fbm(p * 8.0);
        base.rgb = vec3(stone) * vec3(1.0, 0.98, 0.95) * (1.0 - joint * 0.45);
        rough = 0.85; metal = 0.0;
    } else if ((flags & F_GROUND) != 0u) {
        vec2 p = vUv;
        float g = fbm(p * 0.35) * 0.6 + fbm(p * 4.0) * 0.4;
        base.rgb = mix(vec3(0.05, 0.09, 0.03), vec3(0.16, 0.22, 0.07), g);
        rough = 0.95; metal = 0.0;
    }

    vec3 L = normalize(F.sunDir.xyz);
    vec3 H = normalize(V + L);
    float NoL = max(dot(N, L), 0.0), NoV = max(dot(N, V), 1e-3), NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
    float a = max(rough * rough, 0.002);
    vec3 albedo = base.rgb;
    vec3 f0 = mix(vec3(0.04), albedo, metal);
    vec3 Fs = F_Schlick(f0, VoH);
    vec3 spec = D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;
    vec3 diff = (1.0 - Fs) * (1.0 - metal) * albedo / PI;
    vec3 sun = F.sunColor.rgb * F.sunDir.w;
    vec3 col = (diff + spec) * sun * NoL;

    // lumière ambiante : ciel (hémisphère) + reflets de l'environnement
    vec3 R = reflect(-V, N);
    vec3 skyAmb = mix(F.groundCol.rgb, F.skyTop.rgb, N.y * 0.5 + 0.5) * F.misc.z;
    vec3 Fe = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
    // le disque du soleil ne se reflète nettement que sur les surfaces lisses (sinon tache blanche sur la route)
    float sunK = pow(1.0 - clamp(rough, 0.0, 1.0), 6.0);
    vec3 env = mix(skyColor(R, sunK), mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5), clamp(rough * 1.2, 0.0, 1.0)) * F.misc.z;
    float specOcc = mix(1.0, 0.18, clamp(rough, 0.0, 1.0));   // les surfaces rugueuses reflètent peu le ciel
    col += albedo * (1.0 - metal) * skyAmb + env * Fe * F.misc.w * specOcc;

    // peinture : couche de vernis brillante par-dessus (reflets nets du ciel et du soleil)
    if ((flags & F_PAINT) != 0u) {
        float Fc = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
        float ac = 0.03;
        col = col * (1.0 - Fc) + (skyColor(R) * F.misc.z * F.misc.w + D_GGX(NoH, ac * ac) * V_Smith(NoV, NoL, ac * ac) * sun * NoL) * Fc;
    }
    float alpha = base.a;
    if ((flags & F_GLASS) != 0u) {
        float Fg = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
        col = albedo * 0.08 * skyAmb + skyColor(R) * F.misc.z * Fg + D_GGX(NoH, 0.0004) * 0.25 * sun * NoL;
        alpha = clamp(0.55 + Fg, 0.0, 1.0);
    }
    col += pc.emissive.rgb;

    // brouillard (perspective atmosphérique)
    float d = length(F.camPos.xyz - vWorld);
    float fogK = 1.0 - exp(-d * F.fog.a);
    col = mix(col, F.fog.rgb, fogK);
    outColor = vec4(col, ((flags & F_BLEND) != 0u) ? alpha : 1.0);
}
