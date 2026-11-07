// WaForSpeed — WFS-151 (épic E13, RG-01, M-04) : ciel en luminance physique (inclus par sky.frag après common.glsl).
// Distribution relative : ciel clair normalisé CIE type 12 (CIE S 011/E:2003, a −1, b −0,32, c 10, d −3, e 0,45),
// niveau absolu et couleurs calculés sur le CPU (eclairage_physique.cpp, même modèle que eclairage_unites.h) :
//   cielP.x = K × k (jour : L = K f(γ) φ(Z), cd/m² pré-exposées) ; cielP.y = luminance uniforme × k (nuit, 0 le jour) ;
//   cielP.z = luminance du disque du soleil × k (éclairement normal / angle solide du soleil) ; cielP.w = angle de phase de
//   la lune (rad, WFS-207 : la nuit, F.sunDir est la direction de la lune calculée, atmosphere_physique.h).
// La teinte reste le dégradé du jeu (skyTop, skyHorizon), ramené à une luminance relative de 1 ; sous l'horizon : sol
// (groundCol, luminance pré-exposée). Disque du soleil absent des captures et des reflets (soleil analytique, pas compté deux fois).
const float COS_RAYON_SOLEIL = 0.99998918;   // cos(0,2665°) : demi-angle apparent du soleil (NASA NSSDC, « Sun Fact Sheet »)
float cieF(float chi) { return 1.0 + 10.0 * (exp(-3.0 * chi) - exp(-3.0 * 1.5707963)) + 0.45 * cos(chi) * cos(chi); }
float ciePhi(float cz) { return cz <= 1e-6 ? 1.0 : 1.0 - exp(-0.32 / cz); }
// WFS-208 (RG-04) : nuages : couche plane à 2 km (projection d.xz / d.y), bruit de valeur sur 4 octaves ; couverture de la
// carte dans cielP.w le jour (la nuit, cielP.w est la phase de la lune et il n'y a pas de nuages) ; éclairés par le soleil
// (albédo 0,6, diffusion lambertienne) : coût de quelques opérations par pixel de ciel, aucune texture, aucun appel de dessin
float hashN(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float bruit(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hashN(i), hashN(i + vec2(1, 0)), f.x), mix(hashN(i + vec2(0, 1)), hashN(i + vec2(1, 1)), f.x), f.y); }
float nuages(vec3 d, float couverture) {
    if (couverture <= 0.0 || d.y <= 0.02) return 0.0;
    vec2 p = d.xz / d.y * 0.6; float v = 0.0, a = 0.5;
    for (int o = 0; o < 4; o++) { v += a * bruit(p); p *= 2.03; a *= 0.5; }
    return smoothstep(1.0 - couverture, 1.0 - couverture + 0.25, v) * smoothstep(0.02, 0.15, d.y);
}
vec3 cielPhysique(vec3 d) {
    vec3 s = normalize(F.sunDir.xyz);
    float h = d.y;
    vec3 teinte = mix(F.skyHorizon.rgb, F.skyTop.rgb, pow(clamp(h, 0.0, 1.0), 0.45));
    teinte /= max(dot(teinte, vec3(0.2126729, 0.7151522, 0.0721750)), 1e-6);
    float L = F.cielP.y > 0.0 ? F.cielP.y : F.cielP.x * cieF(acos(clamp(dot(d, s), -1.0, 1.0))) * ciePhi(max(h, 0.0));
    vec3 col = mix(teinte * L, F.groundCol.rgb, smoothstep(0.0, -0.25, h));
    if (F.nuitA.w < 0.5 && dot(d, s) > COS_RAYON_SOLEIL) {
        float eclaire = 1.0;
        if (F.cielP.y > 0.0) {   // WFS-207 : lune, partie éclairée selon l'angle de phase (terminateur vertical)
            vec3 ax = normalize(cross(s, vec3(0.0, 1.0, 0.0))), ay = cross(ax, s);
            vec2 q = vec2(dot(d - s, ax), dot(d - s, ay)) / sqrt(1.0 - COS_RAYON_SOLEIL * COS_RAYON_SOLEIL);
            vec3 n = vec3(q, sqrt(max(0.0, 1.0 - dot(q, q))));
            eclaire = step(0.0, dot(n, vec3(sin(F.cielP.w), 0.0, cos(F.cielP.w))));
        }
        col += F.sunColor.rgb * F.cielP.z * eclaire;
    }
    if (F.cielP.y <= 0.0) {   // WFS-208 : nuages de jour (devant le soleil)
        float n = nuages(d, F.cielP.w);
        vec3 nuage = F.sunColor.rgb * F.sunDir.w * (0.6 / 3.14159265) * (max(s.y, 0.05) + 0.35);
        col = mix(col, nuage, n * 0.9);
    }
    return col;
}
