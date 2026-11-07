// WaForSpeed — données communes aux shaders de scène
// WFS-222 : SANS_DISCARD = vrai pour les pipelines des objets opaques sans découpe (ni alpha masqué, ni tramage de fondu,
// ni passe de reflet) : le pilote retire les « discard » du shader, et le test de profondeur se fait AVANT le shader
// (early-Z, LRZ des GPU mobiles) : les pixels cachés ne sont plus calculés. Faux par défaut : comportement d'avant.
layout(constant_id = 0) const bool SANS_DISCARD = false;
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
    mat4 shadowMat;   // monde → carte d'ombre du soleil (WFS-07)
    vec4 shadowP;     // x ombres actives (0/1), y taille d'un texel (uv), z décalage normal (m), w biais de profondeur
    // WFS-45 : projecteurs, lumière hémisphère, brouillard linéaire (three.js r128)
    vec4 camFwd;      // xyz direction regardée par la caméra (profondeur du brouillard linéaire)
    vec4 fogLin;      // x début, y fin du brouillard linéaire (three Fog) ; y = 0 : brouillard exponentiel
    vec4 hemiSky;     // rgb couleur du haut × intensité (HemisphereLight) ; w 1 = active, sinon ambiance du ciel
    vec4 hemiGround;  // rgb couleur du bas × intensité
    vec4 spotN;       // x nombre de projecteurs (0 à 8 : WFS-09, phares + lampadaires)
    vec4 spotPos[8];  // xyz position, w portée (0 = infinie)
    vec4 spotDir[8];  // xyz direction éclairée, w décroissance
    vec4 spotCol[8];  // rgb couleur × intensité × PI
    vec4 spotCone[8]; // x cos(angle), y cos(angle × (1 − pénombre))
    // WFS-09 / WFS-16 : nuit, route mouillée, reflets
    vec4 nuitA;       // x route mouillée (0/1), y flaques (0/1, qualité haute), z reflets temps réel lisibles (0/1),
                      // w 1 = passe de reflet (vue miroir : faces inversées, rien sous le plan)
    vec4 nuitB;       // x hauteur du plan de reflet (m), y 1 / largeur, z 1 / hauteur de la scène (pixels),
                      // w force du reflet de la route mouillée (WFS-142, configgame ; −1 = aucun reflet, mesure du banc)
    // WFS-151 (accroche E13) : SH9 d'irradiance / π pré-exposées ; iblA x actif, y poids de la sonde B, z/w niveau max A/B ;
    vec4 iblSH[9]; vec4 iblA; vec4 iblB; vec4 cielP;   // iblB x k, y/z k / échelle A/B, w lumières artistiques ; cielP : eclairage_ciel.glsl
    // WFS-227 (accroche E13) : cascades d'ombre du soleil (src/render/eclairage_ombres.h) : matrices monde → cascade, fin de chaque
    // cascade le long du regard (m), demi-côté (m) ; ombreP x nombre de cascades, y qualité (1 Moyenne, 2 Haute, 3 Ultra),
    // z côté d'une tuile de l'atlas (texels), w tan(0,53°) (pénombre)
    mat4 ombreCasc[4]; vec4 ombreFin; vec4 ombreDemi; vec4 ombreP;
} F;
layout(set = 0, binding = 1) uniform sampler2DShadow shadowMap;
layout(set = 0, binding = 2) uniform sampler2D reflTex;   // WFS-16 : reflet de la scène sur la chaussée mouillée
layout(set = 0, binding = 3) uniform samplerCube iblCubeA;   // WFS-151 : sondes préfiltrées GGX (A ancienne, B récente)
layout(set = 0, binding = 4) uniform samplerCube iblCubeB;
layout(set = 0, binding = 5) uniform sampler2D iblBrdf;      // WFS-151 : table BRDF du split-sum (NoV, rugosité)
vec3 iblPrefiltre(vec3 d, float r) {   // WFS-151 : luminance pré-exposée de l'environnement, rugosité r (une lecture hors mélange)
    if (F.iblA.y > 0.999) return textureLod(iblCubeB, d, r * F.iblA.w).rgb * F.iblB.z;
    return mix(textureLod(iblCubeA, d, r * F.iblA.z).rgb * F.iblB.y, textureLod(iblCubeB, d, r * F.iblA.w).rgb * F.iblB.z, F.iblA.y);
}

// ciel analytique : sert à la fois pour le fond et pour les reflets (voitures, vitres)
vec3 skyColor(vec3 d, float sunK) {
    if (F.iblA.x > 0.5) return iblPrefiltre(d, 0.0);   // WFS-151 (RG-04) : reflets de l'environnement réel (sonde)
    float h = clamp(d.y, -1.0, 1.0);
    vec3 col = mix(F.skyHorizon.rgb, F.skyTop.rgb, pow(clamp(h, 0.0, 1.0), 0.45));
    col = mix(col, F.groundCol.rgb * 0.6, smoothstep(0.0, -0.25, h));      // sous l'horizon : sol
    float s = max(dot(d, normalize(F.sunDir.xyz)), 0.0);
    col += F.sunColor.rgb * (pow(s, 900.0) * 60.0 * sunK + pow(s, 12.0) * 0.25) * F.sunDir.w;   // disque + halo
    return col;
}
vec3 skyColor(vec3 d) { return skyColor(d, 1.0); }
