// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E9 Routes. Bruits (aussi lus par decor.glsl : sol), chaussée sèche et mouillée, trottoirs.

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p); vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) { float s = 0.0, a = 0.5; for (int i = 0; i < 4; i++) { s += a * noise(p); p *= 2.03; a *= 0.5; } return s; }
// WFS-16 : bruit de la route mouillée (rh12 / rnoise de roadShader, js/world/renderer.js), en coordonnées monde
float rh12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float rnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(rh12(i), rh12(i + vec2(1.0, 0.0)), f.x), mix(rh12(i + vec2(0.0, 1.0)), rh12(i + vec2(1.0, 1.0)), f.x), f.y);
}
// chaussée (F_ROAD) : uv = mètres (x latéral, y le long de la route)
void surfaceRoute(inout vec4 base, inout float rough, inout float metal, inout vec3 N, inout bool wetRoad, inout float flaque) {
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
    // WFS-142 (RG-02) : asphalte sec, jour et nuit : mat (rugosité 0,75 à 0,9), lignes peintes 0,55, jamais métallique
    rough = mix(0.9 - grain * 0.15, 0.55, paint); metal = 0.0;
    if (F.nuitA.x > 0.5) {
        // WFS-142 (RG-03) : asphalte mouillé (pluie). Le roadShader du jeu web (rugosité 0,22, métal 0,4, flaques à 0,02 sur
        // ≈ 18 % de la chaussée) faisait un miroir : il n'est plus suivi. Ici : jamais métallique, rugosité 0,35 à 0,5, asphalte
        // assombri par l'eau (× 0,8), flaques rares aux bords doux (≈ 8 % de la chaussée, seuil 0,66 à 0,8 au lieu de 0,6 à 0,7), hors traces de roues et
        // hors peinture, rugosité ≥ 0,08
        wetRoad = true;
        vec2 w = vWorld.xz;
        float rn = rnoise(w * 0.11) * 0.62 + rnoise(w * 0.43 + 7.0) * 0.38;      // deux échelles, pas de motif répété
        float lane = fract((p.x + halfW) / (2.0 * halfW) * 3.0);                   // position dans la voie (0..1)
        float trk = exp(-pow((lane - 0.3) * 13.0, 2.0)) + exp(-pow((lane - 0.7) * 13.0, 2.0));   // traces de roues
        float puddle = smoothstep(0.66, 0.8, rn) * (1.0 - min(trk, 1.0)) * (1.0 - paint) * F.nuitA.y;   // flaques (Haute)
        base.rgb *= (0.9 + 0.2 * rnoise(w * 0.031 + 3.0)) * (1.0 - trk * 0.07) * mix(0.8, 0.72, puddle);   // eau (flaque à peine plus sombre : pas de « trou » noir), réparations
        rough = clamp(mix(0.38, 0.5, paint) + (rn - 0.5) * 0.12 + trk * 0.04, 0.35, 0.5);   // traces de roues : un peu plus mates
        rough = mix(rough, 0.08, puddle);
        metal = 0.0;
        flaque = puddle;
        N = normalize(mix(N, normalize(vNrm), puddle * 0.7));                       // flaque : surface plus plane, pas un miroir
    }
}
// trottoirs (F_WALK)
void surfaceTrottoir(inout vec4 base, inout float rough, inout float metal) {
    vec2 p = vUv;
    vec2 tile = fract(p / 0.6);
    float joint = 1.0 - smoothstep(0.0, 0.04, min(min(tile.x, 1.0 - tile.x), min(tile.y, 1.0 - tile.y)));
    float stone = 0.32 + 0.1 * hash(floor(p / 0.6)) + 0.06 * fbm(p * 8.0);
    base.rgb = vec3(stone) * vec3(1.0, 0.98, 0.95) * (1.0 - joint * 0.45);
    rough = 0.85; metal = 0.0;
}
