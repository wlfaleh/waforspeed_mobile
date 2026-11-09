#version 450
// WaForSpeed — passe finale : exposition, tonemapping ACES, vignette, gamma, tramage.
// WFS-100 : le même shader sert aussi aux passes du bloom (pc.bloom.x ≠ 0), dessinées hors écran avant la passe finale.
layout(set = 0, binding = 0) uniform sampler2D hdr;        // passe finale : scène HDR ; passes du bloom : image source
layout(set = 0, binding = 1) uniform sampler2D bloomTex;   // passe finale : bloom remonté ; remontée : niveau inférieur
// params : x exposition, y sortie sRGB matérielle, z force du bloom, w exposition physique k la nuit (WFS-230 : vision de nuit ;
//          0 de jour : image inchangée ; WFS-276 : négative dans la Ville = sans relèvement des basses lumières)
// bloom (WFS-100) : x mode (0 passe finale, 1 seuil + réduction, 2 flou horizontal, 3 flou vertical, 4 remontée),
//                   y poids du niveau (remontée) ; passe finale : luminance d'adaptation de nuit en cd/m² (WFS-214, 0 de jour), z pas du flou en texels de la source / niveau inférieur présent (remontée)
layout(push_constant) uniform P { vec4 rot; vec4 params; vec4 bloom; } pc;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 outColor;
// WFS-230 (RG-04) : vision de nuit (effet Purkinje), sur la luminance absolue L = pixel / k (cd/m²), avant la courbe de tons.
//  - luminance scotopique V = Y [1,33 (1 + (Y + Z) / X) − 1,68] (Larson, Rushmeier et Piatko 1997, « A Visibility Matching Tone
//    Reproduction Operator for High Dynamic Range Scenes », IEEE TVCG 3(4)), ramenée au blanc D65 (÷ 2,573) : un gris garde
//    sa clarté, les rouges s'assombrissent, les verts et les bleus restent lisibles ;
//  - poids scotopique s : courbe en S sur log10 L entre 0,005 et 5 cd/m² (vision mésopique, CIE 191:2010), plafonné à 0,9
//    (choix de jeu : l'herbe au clair de lune garde un peu de couleur) ;
//  - teinte des bâtonnets : chromaticité (0,25 ; 0,25) (Jensen et coll. 2001, « A Physically-Based Night Sky Model ») à 40 % seulement
//    (choix de jeu, pas de « nuit américaine ») ; aucun grain, aucune teinte globale. Avis expert-realisme-3d du 2026-11-07.
// WFS-214 (RG-05) : seulement LOIN des lumières : le poids suit la luminance d'adaptation de l'œil La (cd/m², mesurée par l'exposition
// automatique autour du joueur, pc.bloom.y) ou celle du pixel si elle est plus forte ; sous un lampadaire (La ≥ 1 cd/m²), une peinture
// foncée garde sa couleur ; en rase campagne (La ≈ 0,01), rien ne change. Copie C++ et test : eclairage_exposition.h, test_post_wfs214.
const float SCOTO_LOG_BAS = -2.3, SCOTO_LOG_PLAGE = 3.0, SCOTO_MAX = 0.9;
vec3 visionNuit(vec3 c, float k, float La) {
    if (k <= 0.0) return c;
    vec3 rgb = max(c, vec3(0.0));
    float X = dot(rgb, vec3(0.4124, 0.3576, 0.1805)), Y = dot(rgb, vec3(0.2126, 0.7152, 0.0722)), Z = dot(rgb, vec3(0.0193, 0.1192, 0.9505));
    if (Y <= 1e-8 || X <= 1e-8) return c;
    float V = max(Y * (1.33 * (1.0 + (Y + Z) / X) - 1.68), 0.0) / 2.573;
    float t = clamp((log(max(Y / k, La)) / log(10.0) - SCOTO_LOG_BAS) / SCOTO_LOG_PLAGE, 0.0, 1.0);
    float s = SCOTO_MAX * (1.0 - t * t * (3.0 - 2.0 * t));
    const vec3 BATONNETS = vec3(0.706, 0.990, 1.966);   // (0,25 ; 0,25) en sRGB linéaire, Y = 1
    vec3 teinte = mix(vec3(1.0), BATONNETS, 0.4);
    return mix(c, V * teinte, s);
}
// WFS-214 (RG-01) : fonctions de transfert sRGB exactes (IEC 61966-2-1:1999), les mêmes que l'encodeur matériel de l'écran
vec3 srgbCode(vec3 l) { l = clamp(l, 0.0, 1.0); return mix(l * 12.92, 1.055 * pow(l, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, l)); }
vec3 srgbLineaire(vec3 s) { return mix(s / 12.92, pow((s + 0.055) / 1.055, vec3(2.4)), step(0.04045, s)); }
// bruit triangulaire de −1 à +1 (densité en triangle), fixe à l'écran : deux hachages indépendants du pixel
float tramageTriangulaire(vec2 p) {
    float a = fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    float b = fract(sin(dot(p, vec2(39.3468, 11.1353))) * 24634.6345);
    return a + b - 1.0;
}
vec3 aces(vec3 x) { return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); }

// WFS-276 (RG-01 à RG-03) : la nuit, adaptation de chaque pixel à sa propre luminance dans les basses lumières. Près d'une lampe,
// l'exposition globale montre la flaque (≈ 15 lx) mais le paysage sous la lune (≈ 0,1 lx, 7 IL plus bas) sortait noir (péage :
// 52 % de pixels noirs). Sous le seuil SEUIL_NUIT (0,08 : ≈ 1,17 IL sous le gris moyen 0,18, après exposition), la luminance est relevée
// en puissance PUISSANCE_NUIT (0,45) : clarté perçue ∝ L^0,33 à 0,5 aux faibles adaptations (S. S. Stevens, « To Honor Fechner and Repeal His Law »,
// Science 133, 1961 ; J. C. Stevens et S. S. Stevens 1963, exposant 0,33 à 0,5 dans l'obscurité). 7 IL d'écart deviennent
// ≈ 3,8 IL (test_fidelite_lumiere) : la flaque reste nettement plus claire (RG-02 : ≥ 2 IL) ; au-dessus du seuil (flaques, rues éclairées, phares),
// rien ne change ; continu au seuil (même valeur), teinte gardée ; aucun halo (rien de spatial) ; de jour (k = 0) : rien.
const float SEUIL_NUIT = 0.08, PUISSANCE_NUIT = 0.45;
vec3 basseLumiereNuit(vec3 c, float k) {
    if (k <= 0.0) return c;
    float Y = dot(max(c, vec3(0.0)), vec3(0.2126, 0.7152, 0.0722));
    if (Y >= SEUIL_NUIT || Y <= 1e-9) return c;
    return c * (SEUIL_NUIT * pow(Y / SEUIL_NUIT, PUISSANCE_NUIT) / Y);
}

// WFS-100 (RG-04) : seuil 1,0 avec genou doux 0,5 (courbe quadratique entre 0,5 et 1,5, puis linéaire)
vec3 seuilBloom(vec3 c) {
    const float t = 1.0, k = 0.5;
    float br = max(c.r, max(c.g, c.b));
    float rq = clamp(br - t + k, 0.0, 2.0 * k);
    rq = rq * rq / (4.0 * k);
    return c * (max(rq, br - t) / max(br, 1e-4));
}
// flou gaussien séparable à 9 prises contiguës (UnrealBloomPass : getSeperableBlurMaterial), poids normalisés
const float G9[5] = float[](0.2270270, 0.1945946, 0.1216216, 0.0540540, 0.0162162);

vec4 passeBloom(int mode) {
    vec2 px = 1.0 / vec2(textureSize(hdr, 0));
    if (mode == 1) {
        // seuil + réduction de moitié : 4 lectures bilinéaires à cheval sur 2 × 2 pixels → carré de 4 × 4 pixels sans trou ;
        // chaque lecture est plafonnée et seuillée avant la moyenne (un pixel isolé très brillant ne fait pas de tache)
        vec3 s = vec3(0.0);
        for (int i = 0; i < 4; i++) {
            vec2 o = vec2((i & 1) == 0 ? -1.0 : 1.0, (i & 2) == 0 ? -1.0 : 1.0) * px;
            s += seuilBloom(min(texture(hdr, vUv * pc.bloom.w + o).rgb * pc.params.x, vec3(24.0)));   // WFS-246 : bloom.w = part de la scène dessinée
        }
        return vec4(s * 0.25, 1.0);
    }
    if (mode == 2 || mode == 3) {
        vec2 d = (mode == 2 ? vec2(px.x, 0.0) : vec2(0.0, px.y)) * pc.bloom.z;
        vec3 s = texture(hdr, vUv).rgb * G9[0];
        for (int i = 1; i < 5; i++) s += (texture(hdr, vUv + d * float(i)).rgb + texture(hdr, vUv - d * float(i)).rgb) * G9[i];
        return vec4(s, 1.0);
    }
    // remontée : niveau flou × poids + niveau inférieur agrandi par un filtre en tente 3 × 3 (pas de marches ni de losanges)
    vec3 s = texture(hdr, vUv).rgb * pc.bloom.y;
    if (pc.bloom.z > 0.5) {
        vec2 q = 1.0 / vec2(textureSize(bloomTex, 0));
        vec3 u = vec3(0.0);
        for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++)
            u += texture(bloomTex, vUv + vec2(x, y) * q).rgb * float((2 - abs(x)) * (2 - abs(y)));
        s += u / 16.0;
    }
    return vec4(s, 1.0);
}

void main() {
    int mode = int(pc.bloom.x + 0.5);
    if (mode != 0) { outColor = passeBloom(mode); return; }
    vec2 uvS = vUv * (pc.bloom.w > 0.0 ? pc.bloom.w : 1.0);   // WFS-246 (RG-07) : part de la cible de scène dessinée
    vec3 c = visionNuit(texture(hdr, uvS).rgb, abs(pc.params.w), pc.bloom.y) * pc.params.x;   // WFS-230 : vision de nuit (rien de jour)
    c = basseLumiereNuit(c, pc.params.w);   // WFS-276 : basses lumières de la nuit relevées (rien de jour ; params.w < 0 : Ville, rien)
    // léger halo autour des zones très lumineuses (soleil, reflets).
    // 4 × 4 lectures bilinéaires, chacune à cheval sur 2 × 2 pixels, espacées de 2 pixels : elles couvrent tout le
    // carré de 8 × 8 pixels sans trou (WFS-32 : l'ancienne grille de 5 × 5 lectures espacées de 3 pixels transformait
    // chaque pixel très brillant en une grille de points blancs). Chaque lecture est plafonnée pour qu'un seul pixel
    // extrême ne fasse pas une tache.
    vec2 px = 1.0 / vec2(textureSize(hdr, 0));
    vec3 glow = vec3(0.0);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
        vec3 s = texture(hdr, uvS + (vec2(i, j) * 2.0 - 3.5) * px).rgb * pc.params.x;   // -3,5 : à cheval entre 2 pixels
        glow += max(min(s, vec3(24.0)) - 2.5, 0.0);
    }
    c += glow / 16.0 * 0.35;
    // bloom (WFS-15, WFS-100 : UnrealBloomPass du jeu web) : chaîne « dual filter » de 5 niveaux calculée avant cette passe
    // (Renderer::passesBloom) ; force 0 (qualité basse, GPU logiciel) : rien n'est lu, image identique
    if (pc.params.z > 0.0) c += texture(bloomTex, vUv).rgb * pc.params.z;
    c = aces(c);
    vec2 q = vUv - 0.5;
    c *= 1.0 - dot(q, q) * 0.35;
    // WFS-214 (RG-01, ANO-109) : tramage AVANT la conversion 8 bits, dans l'espace des codes de l'écran. Avant : ±0,5 / 255 ajouté
    // en lumière linéaire même quand l'écran encode en sRGB (sortie matérielle, cas du téléphone et du banc) : dans un ciel clair
    // (code sRGB ≈ 0,7), un pas de code vaut ≈ 1,6 / 255 en linéaire, le bruit ne couvrait qu'un tiers de pas → paliers (anneaux
    // autour du soleil). Maintenant : bruit triangulaire (somme de deux bruits uniformes) de ±1 code, ajouté au code sRGB puis
    // ramené en linéaire si l'écran réencode : la marche disparaît à toute clarté, sans biais de teinte (moyenne nulle).
    // Réf. : R. A. Wannamaker, S. P. Lipshitz, J. Vanderkooy, « A Theory of Nonsubtractive Dither », IEEE Trans. Signal Proc. 48(2), 2000.
    c = pc.params.y < 0.5 ? pow(c, vec3(1.0 / 2.2)) : srgbCode(c);
    c += tramageTriangulaire(gl_FragCoord.xy) / 255.0;
    if (pc.params.y >= 0.5) c = srgbLineaire(clamp(c, 0.0, 1.0));
    outColor = vec4(c, 1.0);
}
