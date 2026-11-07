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

// ---------- ombre du soleil : WFS-227 (lot 1 de WFS-153) : cascades stabilisées, pénombre physique ----------
// Cascades (src/render/eclairage_ombres.h) : F.ombreCasc[k] (monde → [-1, 1]² × [0, 1]), fin le long du regard F.ombreFin[k],
// demi-côté F.ombreDemi[k] ; atlas de 2 × 2 tuiles (une cascade : image entière). Profondeur de chaque cascade :
// 2 × demi + RECUL_SOLEIL_M (même constante que le C++, vérifiée par tests/fidelite_lumiere_ombres.cpp).
// Biais (acné, décollement) en texels de la cascade : décalage le long de la normale 1,5 texel, profondeur 1 texel (pente
// plafonnée par le décalage normal) ; une cascade fine (près) garde donc un biais fin : l'ombre reste collée au pied.
// Pénombre (RG-02) : largeur = distance occultant → récepteur × tan(0,53°) (F.ombreP.w) : distance de l'occultant estimée par
// des comparaisons à des profondeurs décalées (0,3 à 30 m, dichotomie : 3 essais ; carte de comparaison : pas de lecture brute) ;
// filtre sur un disque de Poisson de cette largeur (au moins un texel), tourné par pixel (pas de bandes) : 8 lectures en Haute et
// Ultra (4 pour la recherche), 4 en Moyenne (1 pour la recherche) : moins de lectures, même physique. Point éclairé : 4 lectures.
const float RECUL_SOLEIL_M = 120.0;
const vec2 POISSON[12] = vec2[12](vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457), vec2(-0.203, 0.621),
    vec2(0.962, -0.195), vec2(0.473, -0.480), vec2(0.519, 0.767), vec2(0.185, -0.893), vec2(0.507, 0.064), vec2(0.896, 0.412),
    vec2(-0.322, -0.933), vec2(-0.792, -0.598));
const float DELTAS_M[5] = float[5](0.3, 1.0, 3.0, 10.0, 30.0);
// ombre de la cascade k (1 éclairé, 0 à l'ombre) ; −1 : point hors de la cascade
float ombreCascade(int k, vec3 p, vec3 n) {
    float demi = F.ombreDemi[k], tuile = F.ombreP.z;
    float texelM = 2.0 * demi / tuile, profM = 2.0 * demi + RECUL_SOLEIL_M;
    vec4 c = F.ombreCasc[k] * vec4(p + n * (1.5 * texelM), 1.0);
    vec2 uv = c.xy * 0.5 + 0.5;
    if (uv.x <= 0.0 || uv.y <= 0.0 || uv.x >= 1.0 || uv.y >= 1.0 || c.z >= 1.0) return -1.0;
    float sc = F.ombreP.x > 1.5 ? 0.5 : 1.0;
    vec2 o0 = F.ombreP.x > 1.5 ? vec2(float(k % 2), float(k / 2)) * 0.5 : vec2(0.0);
    vec2 auv = o0 + uv * sc;
    float txUv = sc / tuile, mParUv = 2.0 * demi / sc;
    vec2 lo = o0 + vec2(txUv * 0.5), hi = o0 + vec2(sc - txUv * 0.5);   // jamais de lecture dans la tuile voisine
    float z = c.z - texelM / profM;
    bool fine = F.ombreP.y > 1.5;                                        // Haute, Ultra
    // 1) distance de l'occultant : part ombrée (occ) quand on rapproche la profondeur comparée de Δ vers le soleil
    float rs = max(0.5 * DELTAS_M[4] * F.ombreP.w, 2.0 * texelM) / mParUv;
    int nS = fine ? 4 : 1;
    vec2 dS[4] = vec2[4](vec2(0.0), vec2(0.7, 0.7), vec2(-0.7, 0.7), vec2(0.0, -1.0));
    if (fine) { dS[0] = vec2(-0.7, -0.7); }
    float occ0 = 0.0;
    for (int i = 0; i < nS; i++) occ0 += 1.0 - texture(shadowMap, vec3(clamp(auv + dS[i] * rs, lo, hi), z));
    occ0 /= float(nS);
    if (occ0 <= 0.0) return 1.0;                                         // aucun occultant autour : éclairé
    // dichotomie sur les 5 profondeurs (3 comparaisons) : plus grand Δ où au moins la moitié de l'occultation reste
    int lo2 = -1, hi2 = 4;   // occ(Δ[lo2]) > ½ occ0 (lo2 = −1 : Δ = 0) ; occ(Δ[hi2]) ≤ ½ occ0 supposé (au-delà : 30 m)
    for (int it = 0; it < 3; it++) {
        int mid = (lo2 + hi2 + 1) / 2;
        float o = 0.0;
        for (int i = 0; i < nS; i++) o += 1.0 - texture(shadowMap, vec3(clamp(auv + dS[i] * rs, lo, hi), z - DELTAS_M[mid] / profM));
        if (o / float(nS) > 0.5 * occ0) lo2 = mid; else hi2 = mid;
    }
    float dB = sqrt(max(lo2 >= 0 ? DELTAS_M[lo2] : 0.15, 0.15) * DELTAS_M[hi2]);   // milieu géométrique de l'intervalle trouvé
    // 2) filtre sur la largeur de la pénombre (demi-largeur en rayon), au moins un texel
    float r = max(0.5 * dB * F.ombreP.w, texelM) / mParUv;
    float ang = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
    mat2 R = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
    int nF = fine ? 8 : 4;
    float sum = 0.0;
    for (int i = 0; i < 12; i++) {
        if (i >= nF) break;
        sum += texture(shadowMap, vec3(clamp(auv + R * POISSON[i] * r, lo, hi), z));
    }
    return sum / float(nF);
}
float sunShadow(vec3 p, vec3 n) {
    if (F.shadowP.x < 0.5) return 1.0;
    int nc = int(F.ombreP.x + 0.5);
    float d = dot(p - F.camPos.xyz, F.camFwd.xyz);
    if (d >= F.ombreFin[nc - 1]) return 1.0;
    int k = 0;
    while (k < nc - 1 && d > F.ombreFin[k]) k++;
    float o = ombreCascade(k, p, n);
    if (o < 0.0) { o = k + 1 < nc ? ombreCascade(k + 1, p, n) : 1.0; return o < 0.0 ? 1.0 : o; }
    // fondu vers la cascade suivante sur les derniers 15 % (la dernière s'efface vers « éclairé ») : pas de couture visible
    float debut = k > 0 ? F.ombreFin[k - 1] : 0.0, fin = F.ombreFin[k];
    float t = smoothstep(mix(debut, fin, 0.85), fin, d);
    if (t > 0.0) {
        float o2 = k + 1 < nc ? ombreCascade(k + 1, p, n) : 1.0;
        o = mix(o, o2 < 0.0 ? 1.0 : o2, t);
    }
    return o;
}

// projecteurs (WFS-45) : SpotLight de three.js r128 sans lumières physiques (lights_pars_begin / lights_fragment_begin) :
// cône smoothstep(coneCos, penumbraCos, cosθ), distance pow(saturate(1 − d / portée), décroissance), même GGX que le soleil.
// Pas d'ombre portée des projecteurs.
// WFS-218 : feu de croisement (spotCone.z = 1) : répartition et constantes de src/render/phares.h (à garder identiques),
// éclairement physique 1/d² ; w : direction phare → point, D : axe du phare (avant de la voiture, horizontal)
float feuCroisement(vec3 w, vec3 D, float d, float range) {
    vec3 R = normalize(cross(D, vec3(0.0, 1.0, 0.0))), U = cross(R, D);
    float el = degrees(asin(clamp(dot(w, U), -1.0, 1.0))), az = degrees(atan(dot(w, R), dot(w, D)));
    float lim = -0.57 + min(max(az, 0.0) * 0.26795, 1.57), t = (-el - 1.2) / 2.5, ah = az / 18.0;   // WFS-226 : étalement en (az / 18)⁴
    float v = (el > -1.2 ? 1.0 : 1.0 / (1.0 + t * t)) * (1.0 - 0.985 * smoothstep(lim - 0.3, lim + 0.3, el));
    return v * (1.0 - smoothstep(60.0, 80.0, abs(az))) / (1.0 + ah * ah * ah * ah) * (1.0 - smoothstep(0.8 * range, range, d)) / max(d * d, 0.25);
}
vec3 spotLights(vec3 p, vec3 N, vec3 V, vec3 albedo, vec3 f0, float metal, float a, bool noSpec) {
    vec3 sum = vec3(0.0);
    int n = int(F.spotN.x + 0.5);
    float NoV = max(dot(N, V), 1e-3);
    for (int i = 0; i < 12; i++) {   // WFS-226 : 12 projecteurs au plus
        if (i >= n) break;
        vec3 Lv = F.spotPos[i].xyz - p;
        float d = length(Lv);
        vec3 L = Lv / max(d, 1e-4);
        float cosT = dot(L, -F.spotDir[i].xyz);
        float NoL = max(dot(N, L), 0.0);
        bool croisement = F.spotCone[i].z > 0.5;   // WFS-218
        if ((!croisement && cosT <= F.spotCone[i].x) || (croisement && (d > F.spotPos[i].w || cosT < 0.17)) || NoL <= 0.0) continue;   // WFS-226 : hors faisceau
        float range = F.spotPos[i].w, decay = F.spotDir[i].w;
        float att = (range > 0.0 && decay > 0.0) ? pow(clamp(1.0 - d / range, 0.0, 1.0), decay) : 1.0;
        float k = (croisement ? feuCroisement(-L, F.spotDir[i].xyz, d, range) : smoothstep(F.spotCone[i].x, F.spotCone[i].y, cosT) * att) * NoL;
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
