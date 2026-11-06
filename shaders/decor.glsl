// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E12 Environnements. Sol des bas-côtés, végétation (lit fbm de route.glsl).

// bas-côtés (F_GROUND)
void surfaceSol(inout vec4 base, inout float rough, inout float metal) {
    vec2 p = vUv;
    float g = fbm(p * 0.35) * 0.6 + fbm(p * 4.0) * 0.4;
    base.rgb = mix(vec3(0.05, 0.09, 0.03), vec3(0.16, 0.22, 0.07), g);
    rough = 0.95; metal = 0.0;
}
// végétation (WFS-34, F_VTINT) : luminosité du sommet, couverture de l'alpha découpé dans les mipmaps
void teinteVegetation(inout vec4 base, bool vtint, uint flags) {
    if (vtint) base.rgb *= length(vNrm);
    // alpha découpé : la couverture est conservée dans les mipmaps lointains (le feuillage ne s'éclaircit pas au loin)
    if (vtint && (flags & F_MASK) != 0u) base.a *= 1.0 + max(textureQueryLod(texColor, vUv).x, 0.0) * 0.25;
}
