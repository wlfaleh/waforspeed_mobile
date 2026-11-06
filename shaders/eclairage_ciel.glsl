// WaForSpeed — WFS-151 (épic E13, RG-01, M-04) : ciel en luminance physique (inclus par sky.frag après common.glsl).
// Distribution relative : ciel clair normalisé CIE type 12 (CIE S 011/E:2003, a −1, b −0,32, c 10, d −3, e 0,45),
// niveau absolu et couleurs calculés sur le CPU (eclairage_physique.cpp, même modèle que eclairage_unites.h) :
//   cielP.x = K × k (jour : L = K f(γ) φ(Z), cd/m² pré-exposées) ; cielP.y = luminance uniforme × k (nuit, 0 le jour) ;
//   cielP.z = luminance du disque du soleil × k (éclairement normal / angle solide du soleil) ; cielP.w inutilisé.
// La teinte reste le dégradé du jeu (skyTop, skyHorizon), ramené à une luminance relative de 1 ; sous l'horizon : sol
// (groundCol, luminance pré-exposée). Disque du soleil absent des captures et des reflets (soleil analytique, pas compté deux fois).
const float COS_RAYON_SOLEIL = 0.99998918;   // cos(0,2665°) : demi-angle apparent du soleil (NASA NSSDC, « Sun Fact Sheet »)
float cieF(float chi) { return 1.0 + 10.0 * (exp(-3.0 * chi) - exp(-3.0 * 1.5707963)) + 0.45 * cos(chi) * cos(chi); }
float ciePhi(float cz) { return cz <= 1e-6 ? 1.0 : 1.0 - exp(-0.32 / cz); }
vec3 cielPhysique(vec3 d) {
    vec3 s = normalize(F.sunDir.xyz);
    float h = d.y;
    vec3 teinte = mix(F.skyHorizon.rgb, F.skyTop.rgb, pow(clamp(h, 0.0, 1.0), 0.45));
    teinte /= max(dot(teinte, vec3(0.2126729, 0.7151522, 0.0721750)), 1e-6);
    float L = F.cielP.y > 0.0 ? F.cielP.y : F.cielP.x * cieF(acos(clamp(dot(d, s), -1.0, 1.0))) * ciePhi(max(h, 0.0));
    vec3 col = mix(teinte * L, F.groundCol.rgb, smoothstep(0.0, -0.25, h));
    if (F.nuitA.w < 0.5 && dot(d, s) > COS_RAYON_SOLEIL) col += F.sunColor.rgb * F.cielP.z;
    return col;
}
