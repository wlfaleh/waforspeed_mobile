// WaForSpeed — WFS-380 : vraie eau (mer de la Corniche, bassin de la marina, rivière et port de la Ville, sillages).
// Une seule eau pour tout le jeu (RG-05) ; les lieux diffèrent par des PARAMÈTRES portés par les sommets :
//   uv.x = distance au bord de l'eau (m) ; uv.y = pente du fond (m de profondeur par m, sable ≈ 0,03, roche ≈ 0,3, quai ≈ 1) ;
//   longueur de la normale = agitation (1 pleine mer, ≈ 0,3 port, ≈ 0,15 rivière) ; voir src/render/eau_parametres.h.
// Matériau MAT_EAU ; cutoff (pc.params.z) = mode : 0 surface d'eau, 1 sillage (uv : x latéral −1..1, y 0 à l'arrière du bateau
// → 1 au bout), 2 éclaboussure (uv : position sur le disque de rayon 1). Tout est calculé ici : aucune texture téléchargée (RG-06).
const uint F_EAU = 8388608u;

// RG-01 : houle et mer du vent de la Méditerranée par beau temps (vent ≈ 4 m/s). xy : direction (unitaire), z : longueur d'onde (m),
// w : amplitude (m). Dispersion en eau profonde ω² = g k : 28 m → 4,2 s, 14,5 m → 3,0 s ; hauteur significative 4 √(Σ A² / 2) ≈ 0,46 m.
// Mêmes valeurs dans src/render/eau_parametres.h (test_eau compare les deux).
const int EAU_NB = 8;
const vec4 EAU_VAGUES[8] = vec4[8](
    vec4( 0.9659,  0.2588, 28.0, 0.110),
    vec4( 0.8090,  0.5878, 19.0, 0.085),
    vec4( 0.9945, -0.1045, 14.5, 0.070),
    vec4( 0.5592,  0.8290,  9.3, 0.040),
    vec4( 0.8746, -0.4848,  6.1, 0.034),
    vec4( 0.3420,  0.9397,  3.7, 0.026),
    vec4( 0.6947, -0.7193,  2.3, 0.019),
    vec4( 0.4226,  0.9063,  1.4, 0.012));
const float EAU_G = 9.81;

// bruit de valeur 2D (0..1), lisse : dentelle de l'écume
float eauHash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float eauBruit(vec2 p) {
    vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
    return mix(mix(eauHash(i), eauHash(i + vec2(1, 0)), u.x), mix(eauHash(i + vec2(0, 1)), eauHash(i + vec2(1, 1)), u.x), u.y);
}
// écume en dentelle : cellules de bulles qui dérivent et se défont (deux octaves), 0..1
float eauDentelle(vec2 p, float t) {
    float a = eauBruit(p * 1.7 + vec2(0.13 * t, -0.07 * t));
    float b = eauBruit(p * 4.3 - vec2(0.05 * t, 0.11 * t));
    return a * 0.65 + b * 0.35;
}

// pente de la surface (dh/dx, dh/dz), hauteur (m) et variance de pente perdue par les vagues trop petites pour le pixel (elle passe
// dans la rugosité : le scintillement du soleil s'étale au loin comme en vrai, sans crénelage ni motif répété jusqu'à l'horizon)
void eauVagues(vec2 p, float t, float agit, int n, float empreinte, out vec2 pente, out float h, out float varPerdue) {
    pente = vec2(0.0); h = 0.0; varPerdue = 0.0;
    for (int i = 0; i < EAU_NB; i++) {
        if (i >= n) break;
        vec4 w = EAU_VAGUES[i];
        float k = 6.2831853 / w.z, om = sqrt(EAU_G * k), A = w.w * agit;
        float garde = 1.0 - smoothstep(0.18, 0.45, empreinte / w.z);   // au moins ≈ 3 pixels par longueur d'onde
        float th = k * dot(w.xy, p) - om * t + float(i) * 1.7;
        h += A * sin(th) * garde;
        pente += w.xy * (A * k * cos(th) * garde);
        varPerdue += (1.0 - garde * garde) * 0.5 * (A * k) * (A * k);
    }
}

// RG-03 : couleur lue à travers l'eau (Beer-Lambert) : fond de sable ou de roche atténué sur le trajet de la lumière, plus la lumière
// diffusée par l'eau (Méditerranée claire, Jerlov I : absorption ≈ 0,45 / 0,065 / 0,018 m⁻¹ en rouge / vert / bleu)
vec3 eauCorps(float prof, float NoV, float pente) {
    const vec3 ABS = vec3(0.45, 0.065, 0.018), RETRO = vec3(0.0035, 0.0045, 0.006);
    // fond : sable mouillé près du bord, roche calcaire (pente forte), herbier de posidonies au-delà de 6 à 12 m (vert très sombre)
    vec3 fond = mix(vec3(0.46, 0.40, 0.29), vec3(0.20, 0.19, 0.16), smoothstep(0.08, 0.25, pente));
    fond = mix(fond, vec3(0.035, 0.05, 0.035), smoothstep(6.0, 12.0, prof));
    float cosT = sqrt(1.0 - (1.0 - NoV * NoV) / 1.77);   // angle réfracté (n = 1,33)
    vec3 T = exp(-(ABS + RETRO) * prof * (1.0 + 1.0 / max(cosT, 0.3)));
    vec3 profond = 0.33 * RETRO / (ABS + RETRO);          // réflectance de l'eau infiniment profonde (Gordon)
    // sortie de l'eau : (1 − F)² / n² ≈ 0,54 (Fresnel à l'entrée et à la sortie, compression du cône de lumière à la surface)
    return (fond * T + profond * (1.0 - T)) * 0.54;
}

// RG-02 (nuit) : reflet des lampadaires et des feux des bateaux, en traînées allongées par les vagues (lobe GGX de chaque lumière,
// sans le cône du projecteur : on voit la lampe elle-même dans l'eau). Les phares des voitures (feux de croisement) n'y sont pas.
vec3 eauRefletsLampes(vec3 p, vec3 N, vec3 V, float a) {
    vec3 s = vec3(0.0);
    int n = int(F.spotN.x + 0.5);
    float NoV = max(dot(N, V), 1e-3);
    for (int i = 0; i < 12; i++) {
        if (i >= n) break;
        if (F.spotCone[i].z > 0.5) continue;
        vec3 Lv = F.spotPos[i].xyz - p;
        float d2 = max(dot(Lv, Lv), 1.0);
        vec3 L = Lv * inversesqrt(d2);
        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) continue;
        vec3 H = normalize(V + L);
        float NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
        float spec = D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * F_Schlick(vec3(0.02), VoH).x;
        s += min(F.spotCol[i].rgb * (spec * NoL * 6.0 / d2), vec3(SPEC_MAX));
    }
    return s;
}

// surface de l'eau : normale des vagues, couleur du corps de l'eau, rugosité, écume du bord (RG-01, 03, 04) ; renvoie la part
// d'écume (0..1) et l'opacité (sillage, éclaboussure)
float surfaceEau(inout vec4 base, inout float rough, inout float metal, inout vec3 N, vec3 V, out float alpha) {
    float t = F.camPos.w;
    int mode = int(pc.params.z + 0.5);
    vec2 p = vWorld.xz;
    metal = 0.0; alpha = 1.0;
    bool basse = F.spotN.z < 6.0;   // RG-07 : qualité Basse (4 lectures du reflet de la route au lieu de 12)
    if (mode != 0) {   // RG-04 : sillage derrière un bateau, éclaboussure d'un dauphin : écume en dentelle, bords fondus
        float cov;
        if (mode == 1) {
            float u = abs(vUv.x), v = clamp(vUv.y, 0.0, 1.0);
            float bras = exp(-pow((1.0 - u) / 0.16, 2.0));            // deux bras du V (ondes de Kelvin qui déferlent)
            float axe = exp(-pow(u / mix(0.18, 0.45, v), 2.0)) * 1.2;  // eau brassée par l'hélice, qui s'élargit
            cov = max(bras * (1.0 - v * 0.6), axe * (1.0 - v)) * (1.0 - smoothstep(0.7, 1.0, v)) * smoothstep(0.0, 0.08, v);
        } else {
            float r = length(vUv);
            cov = (1.0 - smoothstep(0.55, 1.0, r)) * (0.6 + 0.4 * smoothstep(0.0, 0.5, r));
        }
        float dentelle = eauDentelle(p * 1.4, t);
        alpha = clamp(smoothstep(0.25, 0.75, dentelle * cov * 1.6), 0.0, 1.0) * base.a;
        base.rgb = vec3(0.80, 0.82, 0.84);
        rough = 0.85; N = vec3(0.0, 1.0, 0.0);
        return 1.0;
    }
    float agit = clamp(length(vNrm), 0.05, 1.0);
    float x = max(vUv.x, 0.0), pente = vUv.y;
    float empreinte = max(length(fwidth(vWorld.xz)), 1e-4);   // taille du pixel sur l'eau (m)
    vec2 sl; float h, varPerdue;
    eauVagues(p, t, agit, basse ? 4 : EAU_NB, empreinte, sl, h, varPerdue);
    N = normalize(vec3(-sl.x, 1.0, -sl.y));
    // rugosité : petite ride sous le pixel (Cox-Munk, vent faible) + vagues perdues au loin ; Basse : 4 vagues, le reste en rugosité
    float vmPetites = basse ? 0.0024 : 0.0006;
    rough = clamp(sqrt(sqrt(0.0004 + 2.0 * (varPerdue + vmPetites * agit))), 0.08, 0.6);
    float NoV = max(dot(N, V), 1e-3);
    // profondeur : pente du bord sur 40 m, puis le tombant du plateau (≈ 12 %, petits fonds méditerranéens), 60 m au plus
    float prof = min(0.12 + x * pente + max(x - 40.0, 0.0) * 0.12, 60.0);
    base.rgb = eauCorps(prof, NoV, pente);
    // RG-04 : écume du bord. Sable : ressac (la nappe d'écume monte et redescend, 8 s, décalée le long de la côte) et petite vague
    // qui déferle 4 à 10 m avant le bord puis roule vers lui ; roche et quai : plaques qui battent avec la vague qui arrive
    float ecume = 0.0;
    if (x < 12.0 && agit > 0.2) {
        float roche = smoothstep(0.1, 0.3, pente);
        float ph = t * 0.785 + dot(p, vec2(0.023, 0.017)) + 2.0 * eauBruit(p * 0.03);
        float front = mix(mix(0.5, 2.6, 0.5 + 0.5 * sin(ph)), 0.5 + 1.5 * clamp(h * 4.0 + 0.5, 0.0, 1.0), roche) * agit;
        float bande = 1.0 - smoothstep(front * 0.5, front, x);
        float dentelle = eauDentelle(p * mix(1.1, 1.6, roche), t);
        ecume = smoothstep(0.64, 0.86, dentelle + 0.15 * bande) * bande * 0.8;   // dentelle trouée, pas une nappe
        // déferlement (sable) : ligne qui naît à 10 m, roule vers le bord en s'effilochant (même période que le ressac)
        float cyc = fract(ph / 6.2831853 + 0.35), xb = mix(10.0, 2.0, cyc) * agit;
        float ligne = exp(-pow((x - xb) / mix(0.6, 1.6, cyc), 2.0)) * (1.0 - cyc) * (1.0 - roche);
        ecume = max(ecume, smoothstep(0.62, 0.85, dentelle + 0.2 * ligne) * ligne * 0.75);
        // ligne d'eau : liseré d'écume toujours présent contre le bord (l'eau qui lèche le sable ou la roche)
        ecume = max(ecume, (1.0 - smoothstep(0.0, 0.35, x)) * smoothstep(0.4, 0.65, dentelle) * 0.85);
    }
    base.rgb = mix(base.rgb, vec3(0.80, 0.82, 0.84), ecume);
    rough = mix(rough, 0.8, ecume);
    N = normalize(mix(N, vec3(0.0, 1.0, 0.0), ecume));
    return ecume;
}
