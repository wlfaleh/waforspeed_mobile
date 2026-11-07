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
// ---------- WFS-205 (RG-01, RG-03) : revêtements réels (scans CC0 calibrés, src/render/route_revetements.*) ----------
// Cartes du matériau : texColor (albédo sRGB), texNormal (normales, vert = +v), texMR (occlusion, rugosité, métal).
// Lecture sans répétition visible (avis de l'expert-realisme-3d, journal de WFS-205) ; mode = pc.params.z (qualité) :
//   2 (Ultra, Haute) : pavage hexagonal (Mikkelsen, JCGT 2022 ; grille triangulaire de Heitz et Neyret 2018) : 3 lectures par
//     carte, rotation et décalage aléatoires par cellule, poids accentués (contraste gardé), dérivées tournées (pas de couture
//     de mipmap) ; normales tournées avec la cellule ;
//   1 (Moyenne) : deux échelles (1 et × 0,41 tournée de 37°) mélangées par la macro-variation : 2 lectures (albédo, normale), 1 (ORM) ;
//   0 (Basse) : une lecture.
// Toujours : macro-variations procédurales à 23 et 47 m (périodes sans multiple commun) : albédo ± 12 %, rugosité ± 0,04.
const float COTE_SCAN = 2.0;   // côté d'un scan en mètres (Poly Haven, E : references/reel/routes/revetements.json)
struct Revet { vec3 albedo; vec3 n; float rough; float ao; float lon; };   // lon : longueur moyenne des normales filtrées (Toksvig)
vec2 hash22(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.xx + p3.yz) * p3.zy); }
// Filtrage des normales (Toksvig 2005, LEAN) : les mipmaps de tools/textures/convertir.py sont RENORMALISÉES (WFS-149) : une
// mipmap grossière garde des pentes aussi fortes que le niveau 0, et la chaussée vue de loin (ou dans la capture d'environnement
// 256 px de WFS-150) brille autrement qu'au plus près. Ici, l'empreinte du pixel en texels (ρ) dit quelle part du relief est
// « sous le pixel » : le relief s'aplatit (facteur 1 / (1 + (ρ − 1) / 2) au-delà d'un texel) et la pente perdue passe dans la
// rugosité (α² += σ² × (1 − facteur²), σ² = variance des pentes d'un scan d'asphalte, E). Même reflet moyen à toutes les distances.
const float SIGMA2_PENTES = 0.08;
float facteurRelief(vec2 dx, vec2 dy) {
    float rho = max(length(dx), length(dy)) * float(textureSize(texNormal, 0).x);
    return 1.0 / (1.0 + max(rho - 1.0, 0.0) * 0.5);
}
float roughToksvig(float rough, float lon) {   // lon : facteur de relief gardé (1 : tout le relief au pixel)
    float v = SIGMA2_PENTES * (1.0 - lon * lon);
    return clamp(pow(pow(rough, 4.0) + v, 0.25), 0.0, 1.0);
}
Revet lire1(vec2 st, vec2 dx, vec2 dy, mat2 R) {
    Revet r;
    r.albedo = textureGrad(texColor, st, dx, dy).rgb;
    vec3 n = textureGrad(texNormal, st, dx, dy).xyz * 2.0 - 1.0;
    r.n = vec3(transpose(R) * n.xy, n.z);   // texture lue tournée de R : la pente revient dans le repère de la route
    r.lon = facteurRelief(dx, dy);          // part du relief que le pixel voit (empreinte en texels)
    r.n.xy *= r.lon;
    vec3 orm = textureGrad(texMR, st, dx, dy).rgb;
    r.ao = orm.r; r.rough = orm.g;
    return r;
}
mat2 rotation(float a) { float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }
Revet lireRevetement(vec2 uvM, float mode, bool joints) {
    vec2 st = uvM / COTE_SCAN, dx = dFdx(st), dy = dFdy(st);
    if (joints || mode < 0.5) {   // Basse, ou dalles à joints (trottoir) : pas de rotation (les joints resteraient tordus)
        return lire1(st, dx, dy, mat2(1.0));
    }
    if (mode < 1.5) {   // Moyenne : deux échelles
        mat2 R = rotation(0.6458);   // 37°
        Revet a = lire1(st, dx, dy, mat2(1.0));
        Revet b = lire1(R * st * 0.41 + 0.37, R * dx * 0.41, R * dy * 0.41, R);
        float m = smoothstep(0.3, 0.7, rnoise(uvM * 0.043 + 11.0));
        Revet r; r.albedo = mix(a.albedo, b.albedo, m); r.n = normalize(mix(a.n, b.n, m));
        r.rough = mix(a.rough, b.rough, m); r.ao = mix(a.ao, b.ao, m); r.lon = mix(a.lon, b.lon, m);
        return r;
    }
    // Haute / Ultra : pavage hexagonal (grille triangulaire ; 3 sommets, poids barycentriques)
    vec2 g = st * 3.4641;   // 2·√3 : une cellule ≈ un demi-scan
    vec2 sk = mat2(1.0, 0.0, -0.57735027, 1.15470054) * g;
    vec2 b0 = floor(sk); vec3 t = vec3(fract(sk), 0.0); t.z = 1.0 - t.x - t.y;
    vec3 w; vec2 v1, v2, v3;
    if (t.z > 0.0) { w = vec3(t.z, t.y, t.x); v1 = b0; v2 = b0 + vec2(0, 1); v3 = b0 + vec2(1, 0); }
    else { w = vec3(-t.z, 1.0 - t.y, 1.0 - t.x); v1 = b0 + vec2(1, 1); v2 = b0 + vec2(1, 0); v3 = b0 + vec2(0, 1); }
    w = pow(w, vec3(5.0)); w /= dot(w, vec3(1.0));   // poids accentués : le mélange ne délave pas le contraste
    Revet r; r.albedo = vec3(0.0); r.n = vec3(0.0); r.rough = 0.0; r.ao = 0.0; r.lon = 0.0;
    vec2 vs[3] = vec2[3](v1, v2, v3);
    for (int k = 0; k < 3; k++) {
        vec2 h = hash22(vs[k]);
        mat2 R = rotation(h.x * 6.2831853);
        Revet e = lire1(R * st + h * 17.0, R * dx, R * dy, R);
        r.albedo += e.albedo * w[k]; r.n += e.n * w[k]; r.rough += e.rough * w[k]; r.ao += e.ao * w[k]; r.lon += e.lon * w[k];
    }
    r.n = normalize(r.n);
    return r;
}
// macro-variations (albédo × 1 ± 12 %, rugosité ± 0,04) en coordonnées monde : taches, zones refaites, sans motif à 100 m
vec2 macroRevetement(vec2 w) {
    float m = rnoise(w / 23.0) * 0.6 + rnoise(w / 47.0 + 5.3) * 0.4;   // 0..1
    return vec2(1.0 + (m - 0.5) * 0.24, (m - 0.5) * 0.08);
}

// chaussée (F_ROAD) : uv = mètres (x latéral, y le long de la route)
void surfaceRoute(inout vec4 base, inout float rough, inout float metal, inout vec3 N, inout bool wetRoad, inout float flaque) {
    vec2 p = vUv;
    uint fl = uint(pc.params.w + 0.5);
    bool scan = (fl & F_HASMR) != 0u;   // WFS-205 : cartes du revêtement présentes (sinon repli procédural, RG-04)
    vec3 asphalt; float roughSec; float lonN = 1.0;
    if (scan) {
        Revet rv = lireRevetement(p, pc.params.z, false);
        vec2 mac = macroRevetement(vWorld.xz);
        asphalt = rv.albedo * mac.x;                                              // albédo recalibré à la conversion (RG-02)
        roughSec = clamp(rv.rough + mac.y, 0.0, 1.0);
        N = perturb(normalize(vNrm), vWorld, p, normalize(rv.n));
        lonN = rv.lon;
    } else {
        // WFS-205 (RG-02, RG-04) : repli procédural recalibré sur l'enrobé usé (0,10-0,18 ; avant : 0,045-0,115, moyenne ≈ 0,07)
        float grain = fbm(p * 3.1) * 0.6 + fbm(p * 23.0) * 0.4;
        asphalt = mix(vec3(0.085, 0.088, 0.095), vec3(0.19, 0.19, 0.198), grain);
        asphalt *= 0.85 + 0.3 * fbm(p * vec2(0.08, 0.05));                     // zones usées / rapiécées
        roughSec = 0.9 - grain * 0.15;
    }
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
    rough = roughToksvig(mix(roughSec, 0.55, paint), mix(lonN, 1.0, paint)); metal = 0.0;
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
        rough = roughToksvig(rough, mix(lonN, 1.0, puddle * 0.7));   // WFS-205 : flaque plus plane (comme N)
        metal = 0.0;
        flaque = puddle;
        N = normalize(mix(N, normalize(vNrm), puddle * 0.7));                       // flaque : surface plus plane, pas un miroir
    }
}
// trottoirs (F_WALK)
void surfaceTrottoir(inout vec4 base, inout float rough, inout float metal, inout vec3 N) {
    vec2 p = vUv;
    if ((uint(pc.params.w + 0.5) & F_HASMR) != 0u) {   // WFS-205 : béton de trottoir (scan à dalles : lu sans rotation)
        Revet rv = lireRevetement(p, pc.params.z, true);
        vec2 mac = macroRevetement(vWorld.xz);
        base.rgb = rv.albedo * mac.x * (0.94 + 0.12 * hash(floor(p / COTE_SCAN)));   // teinte par carreau : pas de motif répété
        rough = roughToksvig(clamp(rv.rough + mac.y, 0.0, 1.0), rv.lon); metal = 0.0;
        N = perturb(normalize(vNrm), vWorld, p, normalize(rv.n));
        return;
    }
    vec2 tile = fract(p / 0.6);
    float joint = 1.0 - smoothstep(0.0, 0.04, min(min(tile.x, 1.0 - tile.x), min(tile.y, 1.0 - tile.y)));
    float stone = 0.32 + 0.1 * hash(floor(p / 0.6)) + 0.06 * fbm(p * 8.0);
    base.rgb = vec3(stone) * vec3(1.0, 0.98, 0.95) * (1.0 - joint * 0.45);
    rough = 0.85; metal = 0.0;
}
