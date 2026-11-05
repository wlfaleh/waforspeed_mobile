#version 450
// WaForSpeed — personnages (WFS-72) : copie de scene.frag (même éclairage PBR, ombres, brouillard, projecteurs)
// + vêtements peints de pedMaterial du jeu web (js/world/people.js l. 54-110), repris tels quels : bloc « WFS-72 » plus bas.
// Toute modification de scene.frag doit être reportée ici (seuls les blocs marqués WFS-72 diffèrent).
#include "common.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNrm;
layout(location = 2) in vec2 vUv;
layout(location = 3) in vec4 vCloth;   // WFS-72 : _cloth (bras, hauteur, jambe, partie)
layout(location = 4) in vec3 vBind;    // WFS-72 : position en pose de repos (col rond)
// WFS-72 : palette d'os puis apparence de l'instance : look[0..5] = peau, cheveux, haut, bas, chaussures, maillot (rgb,
// codes sRGB ; look[0].w = 1 : apparence active), look[6] = uCloth, look[7] = uBody
layout(set = 2, binding = 0) uniform Os { mat4 os[64]; vec4 look[8]; } B;
layout(set = 1, binding = 0) uniform sampler2D texColor;
layout(set = 1, binding = 1) uniform sampler2D texNormal;
layout(set = 1, binding = 2) uniform sampler2D texMR;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;
const uint F_PAINT = 1u, F_BLEND = 2u, F_TAIL = 4u, F_HEAD = 8u, F_MASK = 32u, F_GLASS = 64u,
           F_ROAD = 256u, F_WALK = 512u, F_GROUND = 1024u, F_SHADOW = 2048u, F_HASNORMAL = 4096u, F_HASMR = 8192u, F_VTINT = 16384u,
           F_POOL = 32768u, F_ADD = 65536u;   // WFS-09 : tache des phares (multiplicative), halo (additif)

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
// part du brouillard au point p (même calcul que la fin de main)
float fogAt(vec3 p) {
    if (F.fogLin.y > 0.0) return smoothstep(F.fogLin.x, F.fogLin.y, dot(p - F.camPos.xyz, F.camFwd.xyz));
    return 1.0 - exp(-length(F.camPos.xyz - p) * F.fog.a);
}

// WFS-100 (RG-02) : plafond du spéculaire (valeur HDR) avant le bloom, pour toutes les surfaces
const float SPEC_MAX = 4.0;
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

// ombre du soleil (WFS-07) : 9 lectures comparées (filtre bilinéaire matériel → flou d'environ 3 texels, ombres douces).
// Le point est d'abord poussé le long de la normale (normalBias du jeu web) pour éviter l'acné d'ombre.
float sunShadow(vec3 p, vec3 n) {
    if (F.shadowP.x < 0.5) return 1.0;
    vec4 c = F.shadowMat * vec4(p + n * F.shadowP.z, 1.0);
    vec2 uv = c.xy * 0.5 + 0.5;
    if (uv.x <= 0.0 || uv.y <= 0.0 || uv.x >= 1.0 || uv.y >= 1.0 || c.z >= 1.0) return 1.0;
    float z = c.z + F.shadowP.w, t = F.shadowP.y * 1.5, sum = 0.0;
    for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) sum += texture(shadowMap, vec3(uv + vec2(x, y) * t, z));
    vec2 e = min(uv, 1.0 - uv);   // fondu au bord de la boîte d'ombre : pas de coupure nette
    return mix(1.0, sum / 9.0, smoothstep(0.0, 0.06, min(e.x, e.y)));
}

// projecteurs (WFS-45) : SpotLight de three.js r128 sans lumières physiques (lights_pars_begin / lights_fragment_begin) :
// cône smoothstep(coneCos, penumbraCos, cosθ), distance pow(saturate(1 − d / portée), décroissance), même GGX que le soleil.
// Pas d'ombre portée des projecteurs.
vec3 spotLights(vec3 p, vec3 N, vec3 V, vec3 albedo, vec3 f0, float metal, float a, bool noSpec) {
    vec3 sum = vec3(0.0);
    int n = int(F.spotN.x + 0.5);
    float NoV = max(dot(N, V), 1e-3);
    for (int i = 0; i < 8; i++) {
        if (i >= n) break;
        vec3 Lv = F.spotPos[i].xyz - p;
        float d = length(Lv);
        vec3 L = Lv / max(d, 1e-4);
        float cosT = dot(L, -F.spotDir[i].xyz);
        float NoL = max(dot(N, L), 0.0);
        if (cosT <= F.spotCone[i].x || NoL <= 0.0) continue;
        float range = F.spotPos[i].w, decay = F.spotDir[i].w;
        float att = (range > 0.0 && decay > 0.0) ? pow(clamp(1.0 - d / range, 0.0, 1.0), decay) : 1.0;
        float k = smoothstep(F.spotCone[i].x, F.spotCone[i].y, cosT) * att * NoL;
        vec3 H = normalize(V + L);
        float NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
        vec3 Fs = F_Schlick(f0, VoH);
        vec3 spec = noSpec ? vec3(0.0) : D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;
        sum += (1.0 - Fs) * (1.0 - metal) * albedo / PI * F.spotCol[i].rgb * k + min(spec * F.spotCol[i].rgb * k, vec3(SPEC_MAX));   // WFS-100 RG-02
    }
    return sum;
}

void main() {
    uint flags = uint(pc.params.w + 0.5);
    // finesse de la pièce (WFS-42) : seulement pour les voitures (emissive.w sert à la demi-largeur de route sinon)
    bool fine = pc.emissive.w > 0.0 && (flags & (F_ROAD | F_WALK | F_GROUND)) == 0u;
    // WFS-16 : passe de reflet (vue miroir de la chaussée) : rien de ce qui est sous le plan
    if (F.nuitA.w > 0.5 && vWorld.y < F.nuitB.x - 0.05) discard;
    if ((flags & F_ADD) != 0u) {   // WFS-09 : halo des feux et des lampadaires (TEXS.radial du jeu web, mélange additif)
        // dégradé radial 1 → 0,45 (35 % du rayon) → 0 ; couleur × intensité portée par la normale du sommet
        float r = length(vUv * 2.0 - 1.0);
        float k = r < 0.35 ? mix(1.0, 0.45, r / 0.35) : mix(0.45, 0.0, clamp((r - 0.35) / 0.65, 0.0, 1.0));
        outColor = vec4(max(vNrm, vec3(0.0)) * k * (1.0 - fogAt(vWorld)), 1.0);
        return;
    }
    if ((flags & F_POOL) != 0u) {   // WFS-09 : tache des phares sur la route (beamTex('pool') de js/cars/models.js)
        // uv.x = travers (0..1 sur BEAM.wid = 26 m), uv.y = distance devant le pare-chocs / BEAM.len (110 m) ;
        // mélange multiplicatif : sol × (1 + faisceau), comme MAT.headPool (DstColor + One)
        float dm = vUv.y * 110.0, xm = (vUv.x - 0.5) * 26.0;
        float rise = min(1.0, dm / 2.5);
        float fall = 1.0 / (1.0 + pow(dm / 32.0, 2.0)) * (1.0 - max(0.0, (dm - 110.0 * 0.82) / (110.0 * 0.18)));
        float spread = 0.9 + dm * 0.32, spotW = 0.8 + dm * 0.06;
        float wide = exp(-0.5 * pow(xm / spread, 4.0));
        float hot = 0.3 * exp(-0.5 * pow((dm - 36.0) / 22.0, 2.0)) * exp(-0.5 * pow(xm / spotW, 2.0));
        float a = clamp(rise * (fall * wide * 0.7 + hot), 0.0, 1.0);
        outColor = vec4(pc.color.rgb * a * (1.0 - fogAt(vWorld)), 1.0);
        return;
    }
    if ((flags & F_SHADOW) != 0u) {   // ombre de contact : rectangle aux bords très doux
        vec2 q = abs(vUv * 2.0 - 1.0);
        float a = (1.0 - smoothstep(0.55, 1.0, q.x)) * (1.0 - smoothstep(0.6, 1.0, q.y));
        outColor = vec4(0.0, 0.0, 0.0, a * pc.color.a);
        return;
    }
    vec3 N = normalize(vNrm);
    vec3 V = normalize(F.camPos.xyz - vWorld);
    bool vtint = (flags & F_VTINT) != 0u;   // végétation (WFS-34) : |normale| = luminosité, même éclairage des deux faces
    // pièces double face. Route, trottoirs et bas-côtés : normales déjà orientées vers le haut à la construction, mais
    // triangles enroulés dans l'autre sens → ne pas retourner (sinon le sol ne recevait ni soleil ni ombre, WFS-07)
    // passe de reflet (WFS-16) : la vue miroir inverse le sens des faces
    bool face = gl_FrontFacing != (F.nuitA.w > 0.5);
    if (!face && !vtint && (flags & (F_ROAD | F_WALK | F_GROUND)) == 0u) N = -N;
    vec4 base = texture(texColor, vUv) * pc.color;
    // WFS-100 (RG-03, ANO-28) : carrosserie = couleur de peinture choisie × 1, sans la texture de couleur du modèle
    // (js/cars/models.js : color.set(paint), carte de couleur retirée) ; vitres, feux, jantes et logos gardent leur texture
    if ((flags & F_PAINT) != 0u) base = pc.color;
    if (vtint) base.rgb *= length(vNrm);
    // alpha découpé : la couverture est conservée dans les mipmaps lointains (le feuillage ne s'éclaircit pas au loin)
    if (vtint && (flags & F_MASK) != 0u) base.a *= 1.0 + max(textureQueryLod(texColor, vUv).x, 0.0) * 0.25;
    float metal = pc.params.x, rough = pc.params.y;
    if ((flags & F_HASMR) != 0u) { vec4 mr = texture(texMR, vUv); rough *= mr.g; metal *= mr.b; }
    if ((flags & F_HASNORMAL) != 0u) N = perturb(N, vWorld, vUv, normalize(texture(texNormal, vUv).xyz * 2.0 - 1.0));
    if (B.look[0].w > 0.5) {   // ---------- WFS-72 : pedMaterial du jeu web (map_fragment, roughness, normal) ----------
        // le jeu web (three r128 sans décodage sRGB) travaille sur les codes sRGB de la texture et des couleurs : idem ici
        vec3 uSkin = B.look[0].rgb, uHair = B.look[1].rgb, uTop = B.look[2].rgb, uBottom = B.look[3].rgb,
             uShoe = B.look[4].rgb, uSwim = B.look[5].rgb;
        vec4 uCloth = B.look[6], uBody = B.look[7];
        vec3 tex = pow(max(base.rgb, vec3(0.0)), vec3(1.0 / 2.2)), col = tex;
        float part = vCloth.w, cm = 0.0;
        float lum = dot(tex, vec3(0.299, 0.587, 0.114));
        if (part < 0.5) {
            // corps : la peau garde la texture (visage, mains), teinte selon la personne
            float sat = max(tex.r, max(tex.g, tex.b)) - min(tex.r, min(tex.g, tex.b));
            float under = 1.0 - smoothstep(0.05, 0.13, sat);            // sous-vêtements gris du modèle d'origine
            vec3 skin = tex * uSkin;
            float arm = vCloth.x, y = vCloth.y, leg = vCloth.z;
            float isArm = step(-0.5, arm), isLeg = step(-0.5, leg), dressed = uCloth.z;
            // col rond : on découvre le cou (distance à l'axe du cou, en pose de repos)
            float hd = length(vec2(vBind.x, (vBind.z - uBody.z) * 1.15)), ny = y - uBody.x;
            float collar = 1.0 - step(0.0, ny + 0.05) * step(hd, 0.085 + max(0.0, -ny) * 0.4);
            float top = dressed * collar * step(ny, 0.05) * max((1.0 - isArm) * (1.0 - isLeg) * step(uBody.y, y), isArm * step(arm, uCloth.x));
            float bot = dressed * (1.0 - isArm) * step(y, uBody.y + 0.001) * (1.0 - isLeg * step(uCloth.y, leg));
            float shoe = isLeg * step(1.93, leg);
            vec3 baseC = mix(skin, uSwim * (0.25 + lum * 1.6), under * (1.0 - dressed));
            // vêtements : couleur unie, un peu du relief de la texture pour les plis et l'ombre
            float shade = mix(1.0, clamp(lum / 0.5, 0.55, 1.2), 0.3);
            float hem = 1.0 - 0.25 * (1.0 - smoothstep(0.0, 0.035, abs(arm - uCloth.x))) * isArm * dressed;
            vec3 cloth = shoe > 0.5 ? uShoe : (top > 0.5 ? uTop : uBottom);
            cm = max(max(top, bot), shoe);
            col = mix(baseC, cloth * shade * hem, cm);
        } else if (part > 1.5) {
            col = uHair * (0.35 + lum * 1.25);                          // sourcils et cheveux : texture grise teintée
        }
        base.rgb = pow(max(col, vec3(0.0)), vec3(2.2));
        rough = part < 0.5 ? mix(0.52, 0.9, cm) : (part < 1.5 ? 0.15 : 0.72);
        vec3 geo = normalize(vNrm) * (gl_FrontFacing ? 1.0 : -1.0);
        N = normalize(mix(N, geo, cm * 0.85));                            // tissus : pas les muscles du modèle nu
    }
    if ((flags & F_MASK) != 0u && base.a < pc.params.z) discard;

    // ---------- surfaces procédurales (uv = mètres : x latéral, y le long de la route) ----------
    bool wetRoad = false;   // WFS-16 : chaussée mouillée (nuit en ville et sur la corniche)
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
        if (F.nuitA.x > 0.5) {   // WFS-16 : roadShader du jeu web (rnoise, trk, puddle) ; RG-01 rugosité 0,22, métal 0,4
            wetRoad = true;
            vec2 w = vWorld.xz;
            float rn = rnoise(w * 0.11) * 0.62 + rnoise(w * 0.43 + 7.0) * 0.38;      // deux échelles, pas de motif répété
            float lane = fract((p.x + halfW) / (2.0 * halfW) * 3.0);                   // position dans la voie (0..1)
            float trk = exp(-pow((lane - 0.3) * 13.0, 2.0)) + exp(-pow((lane - 0.7) * 13.0, 2.0));   // traces de roues
            float puddle = smoothstep(0.6, 0.7, rn - trk * 0.08) * F.nuitA.y;          // flaques (qualité haute)
            base.rgb *= (0.9 + 0.2 * rnoise(w * 0.031 + 3.0)) * (1.0 - trk * 0.07) * mix(1.0, 0.5, puddle);   // réparations
            rough = clamp(mix(0.22, 0.45, paint) * (0.8 + rn * 0.4) - trk * 0.08, 0.04, 1.0);
            rough = mix(rough, 0.02, puddle);
            metal = 0.4 * (1.0 - paint);
            N = normalize(mix(N, normalize(vNrm), puddle));                             // flaque : surface miroir
        }
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
    // anticrénelage des reflets (Tokuyoshi et Kaplanyan 2019, WFS-32) : là où la normale change beaucoup d'un pixel à
    // l'autre (arêtes, petits triangles vus en biais), le reflet est un peu élargi : plus d'éclat isolé sur un pixel.
    // Les grandes surfaces (normale régulière) gardent leur reflet net.
    vec3 dndx = dFdx(N), dndy = dFdy(N);
    float aaKernel = min(2.0 * 0.25 * (dot(dndx, dndx) + dot(dndy, dndy)), 0.18);
    // pièces très fines (WFS-42 : fils de la lunette, rayons de jante) : triangles plus petits qu'un pixel, que les
    // dérivées ne voient pas. WFS-100 (RG-01, ANO-21) : plus d'élargissement du reflet (il étalait le soleil en traits
    // brillants dans le vide) ; les opaques reçoivent le même facteur de couverture du pixel que les transparents
    float detailK = 1.0;   // part du pixel réellement couverte par la pièce (1 = pièce plus large qu'un pixel)
    if (fine) detailK = clamp(pc.emissive.w / max(length(fwidth(vWorld)), 1e-6), 0.0, 1.0);
    // opaque : pas d'alpha → les reflets (soleil, ciel, vernis) sont atténués par la couverture ; transparent : alpha × couverture
    float specK = (flags & F_BLEND) == 0u ? detailK : 1.0;
    float a = sqrt(max(rough * rough, 0.002) * max(rough * rough, 0.002) + aaKernel);
    vec3 albedo = base.rgb;
    vec3 f0 = mix(vec3(0.04), albedo, metal);
    vec3 Fs = F_Schlick(f0, VoH);
    vec3 spec = vtint ? vec3(0.0) : D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;   // feuillage : Lambert sans reflet (jeu web)
    vec3 diff = (1.0 - Fs) * (1.0 - metal) * albedo / PI;
    vec3 sun = F.sunColor.rgb * F.sunDir.w;
    float ombre = sunShadow(vWorld, N);
    vec3 col = diff * sun * NoL * ombre + min(spec * sun * NoL * ombre, vec3(SPEC_MAX)) * specK;   // WFS-100 RG-01 / RG-02
    col += spotLights(vWorld, N, V, albedo, f0, metal, a, vtint);   // WFS-45 (aucun si spotN = 0)

    // lumière ambiante : ciel (hémisphère) + reflets de l'environnement
    vec3 R = reflect(-V, N);
    vec3 skyAmb = mix(F.groundCol.rgb, F.skyTop.rgb, N.y * 0.5 + 0.5) * F.misc.z;
    // WFS-45 : HemisphereLight de three (irradiance mix(bas, haut, 0,5·N.y + 0,5), × PI puis Lambert 1/PI → albedo × mix)
    if (F.hemiSky.w > 0.5) skyAmb = mix(F.hemiGround.rgb, F.hemiSky.rgb, N.y * 0.5 + 0.5);
    vec3 Fe = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
    if (vtint) Fe = vec3(0.0);   // pas de reflet du ciel sur les cartes vues en rasant ou de dos
    // le disque du soleil ne se reflète nettement que sur les surfaces lisses (sinon tache blanche sur la route)
    // pièces fines (WFS-42) : le ciel reflété, et surtout le disque du soleil (60 x l'intensité), suivent la rugosité
    // effective (élargie par l'anticrénelage) : sinon chaque petite facette en miroir renvoie le soleil → points brillants
    float roughEnv = fine ? max(rough, sqrt(a)) : rough;
    float sunK = pow(1.0 - clamp(roughEnv, 0.0, 1.0), 6.0);
    vec3 env = mix(skyColor(R, sunK), mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5), clamp(roughEnv * 1.2, 0.0, 1.0)) * F.misc.z;
    float specOcc = mix(1.0, 0.18, clamp(rough, 0.0, 1.0));   // les surfaces rugueuses reflètent peu le ciel
    if (wetRoad && F.nuitA.z > 0.5) {
        // WFS-16 : reflet temps réel de la scène (passe miroir, demi-résolution), au même point de l'écran ; étiré à la
        // verticale selon la rugosité (traînées des lumières sur l'asphalte mouillé), légèrement déformé par le relief
        vec2 suv = gl_FragCoord.xy * F.nuitB.yz + N.xz * 0.01;
        float spread = rough * 0.05;
        vec3 rs = vec3(0.0);
        for (int i = 0; i < 6; i++) rs += texture(reflTex, suv + vec2(0.0, (float(i) - 2.5) * 0.4 * spread)).rgb;
        env = rs / 6.0;
    }
    float kMesure = ((flags & F_ROAD) != 0u && F.nuitB.w < 0.0) ? 0.0 : 1.0;   // WFS-142 (RG-08) : WFS_REFLETS=0, route sans reflet
    col += albedo * (1.0 - metal) * skyAmb + min(env * Fe * F.misc.w * specOcc, vec3(SPEC_MAX)) * specK * kMesure;   // WFS-100 RG-01 / RG-02

    // peinture : couche de vernis brillante par-dessus (reflets nets du ciel et du soleil)
    if ((flags & F_PAINT) != 0u) {
        float Fc = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
        float acA = sqrt(0.03 * 0.03 * 0.03 * 0.03 + aaKernel);          // vernis : alpha 0,0009, élargi sur les arêtes
        float sunKc = clamp(1.0 - sqrt(aaKernel) * 4.0, 0.0, 1.0);        // disque du soleil reflété : seulement si net
        vec3 vernis = (skyColor(R, sunKc) * F.misc.z * F.misc.w + D_GGX(NoH, acA) * V_Smith(NoV, NoL, acA) * sun * NoL) * Fc;
        col = col * (1.0 - Fc) + min(vernis, vec3(SPEC_MAX)) * specK;   // WFS-100 RG-01 / RG-02
    }
    float alpha = base.a * detailK;   // pièce transparente plus fine qu'un pixel : couverture partielle (WFS-42)
    if ((flags & F_GLASS) != 0u) {
        float Fg = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
        col = albedo * 0.08 * skyAmb + min(skyColor(R) * F.misc.z * Fg + D_GGX(NoH, 0.0004) * 0.25 * sun * NoL, vec3(SPEC_MAX));   // WFS-100 RG-02
        alpha = clamp(0.55 + Fg, 0.0, 1.0);
    }
    col += pc.emissive.rgb;

    // brouillard (perspective atmosphérique)
    float d = length(F.camPos.xyz - vWorld);
    float fogK = 1.0 - exp(-d * F.fog.a);
    // WFS-45 : brouillard linéaire de three r128 (fog_fragment : smoothstep(near, far, profondeur de vue))
    if (F.fogLin.y > 0.0) fogK = smoothstep(F.fogLin.x, F.fogLin.y, dot(vWorld - F.camPos.xyz, F.camFwd.xyz));
    col = mix(col, F.fog.rgb, fogK);
    // pas de pixel aberrant (WFS-32) : un reflet du soleil sur un minuscule triangle du vernis peut donner une valeur
    // énorme, voire NaN (normale nulle) ; on plafonne à 64 et on remplace NaN / infini par 0
    col = clamp(col, vec3(0.0), vec3(64.0));
    if (any(isnan(col)) || any(isinf(col))) col = vec3(0.0);
    outColor = vec4(col, ((flags & F_BLEND) != 0u) ? alpha : 1.0);
}
