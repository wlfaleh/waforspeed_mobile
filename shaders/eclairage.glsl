// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E13 Lumière et image. GGX, Fresnel, ombre du soleil, projecteurs, brouillard, halos des lumières.

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

// WFS-09 : halo des feux et des lampadaires (TEXS.radial du jeu web, mélange additif) ; bloc F_ADD de main
vec4 haloLumiere() {
    // dégradé radial 1 → 0,45 (35 % du rayon) → 0 ; couleur × intensité portée par la normale du sommet
    float r = length(vUv * 2.0 - 1.0);
    float k = r < 0.35 ? mix(1.0, 0.45, r / 0.35) : mix(0.45, 0.0, clamp((r - 0.35) / 0.65, 0.0, 1.0));
    return vec4(max(vNrm, vec3(0.0)) * k * (1.0 - fogAt(vWorld)) * F.iblB.w, 1.0);   // WFS-151 (RG-06) : suit l'exposition
}

// WFS-149 (RG-04) : carte de normales dans le repère tangent des sommets (MikkTSpace, src/render/mesh.h), drapeau
// MAT_TANGENTE seulement (aucun matériau existant ne l'a : image inchangée). Bitangente = signe × N × T (glTF).
vec3 perturbTangente(vec3 N, vec4 t, vec3 tn) {
    vec3 T = normalize(t.xyz - N * dot(N, t.xyz));
    vec3 B = cross(N, T) * (t.w < 0.0 ? -1.0 : 1.0);
    return normalize(mat3(T, B, N) * tn);
}

// ---------------------------------------------------------------- WFS-151 : éclairage d'image (IBL, RG-03 et RG-04)
// irradiance de la sonde : harmoniques sphériques d'ordre 2 (Ramamoorthi et Hanrahan 2001), coefficients déjà convolués
// par le cosinus, divisés par π et pré-exposés sur le CPU (eclairage_physique.cpp) : albédo × iblDiffus(N) = diffus
vec3 iblDiffus(vec3 n) {
    float x = n.x, y = n.y, z = n.z;
    vec3 r = F.iblSH[0].rgb * 0.282095
           + F.iblSH[1].rgb * (0.488603 * y) + F.iblSH[2].rgb * (0.488603 * z) + F.iblSH[3].rgb * (0.488603 * x)
           + F.iblSH[4].rgb * (1.092548 * x * y) + F.iblSH[5].rgb * (1.092548 * y * z) + F.iblSH[6].rgb * (0.315392 * (3.0 * z * z - 1.0))
           + F.iblSH[7].rgb * (1.092548 * x * z) + F.iblSH[8].rgb * (0.546274 * (x * x - y * y));
    return max(r, vec3(0.0));
}
// RG-04 : en mode physique, ambiance (skyAmb) et reflets (env × Fe) viennent de la sonde : cube préfiltré GGX lu au niveau
// de la rugosité, Fresnel intégré par la table BRDF du split-sum (Karis 2013). Plus de facteur d'ambiance ni d'occlusion
// spéculaire arbitraires ; le plafond du Fresnel de la chaussée (WFS-142, RG-04 : jamais de route miroir) est gardé.
void iblRemplacer(inout vec3 skyAmb, inout vec3 env, inout vec3 Fe, inout float specOcc, vec3 N, vec3 R, float NoV, float rough,
                  vec3 f0, uint flags, float flaque) {
    if (F.iblA.x < 0.5) return;
    skyAmb = iblDiffus(N);
    specOcc = 1.0;
    if ((flags & F_VTINT) != 0u) { Fe = vec3(0.0); env = vec3(0.0); return; }   // feuillage : Lambert sans reflet (pas de lecture)   // aussi dans les captures : rebonds de la lumière d'une capture à la suivante
    env = iblPrefiltre(R, clamp(rough, 0.0, 1.0));
    vec2 ab = texture(iblBrdf, vec2(clamp(NoV, 0.0, 1.0), clamp(rough, 0.0, 1.0))).rg;
    Fe = f0 * ab.x + ab.y;
    if ((flags & F_ROAD) != 0u) Fe = min(Fe, vec3(mix(0.25, 0.4, flaque)));
}
