// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E10 Véhicules. Fondu des versions allégées, ombre de contact, tache des phares, vernis, vitres, fantôme.

// WFS-134 (RG-12) : fondu entre la voiture complète et sa version allégée (tramage écran 4×4 complémentaire)
void fonduLod() {
    ivec2 q = ivec2(gl_FragCoord.xy) & 3;
    const float B[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);
    float seuil = (B[q.y * 4 + q.x] + 0.5) / 16.0, w = -pc.emissive.w;
    if (w < 2.5 ? seuil >= w - 1.0 : seuil < w - 3.0) discard;
}
// WFS-09 : tache des phares sur la route (beamTex('pool') de js/cars/models.js) ; bloc F_POOL de main
vec4 tachePhares() {
    // uv.x = travers (0..1 sur BEAM.wid = 26 m), uv.y = distance devant le pare-chocs / BEAM.len (110 m) ;
    // mélange multiplicatif : sol × (1 + faisceau), comme MAT.headPool (DstColor + One)
    float dm = vUv.y * 110.0, xm = (vUv.x - 0.5) * 26.0;
    float rise = min(1.0, dm / 2.5);
    float fall = 1.0 / (1.0 + pow(dm / 32.0, 2.0)) * (1.0 - max(0.0, (dm - 110.0 * 0.82) / (110.0 * 0.18)));
    float spread = 0.9 + dm * 0.32, spotW = 0.8 + dm * 0.06;
    float wide = exp(-0.5 * pow(xm / spread, 4.0));
    float hot = 0.3 * exp(-0.5 * pow((dm - 36.0) / 22.0, 2.0)) * exp(-0.5 * pow(xm / spotW, 2.0));
    float a = clamp(rise * (fall * wide * 0.7 + hot), 0.0, 1.0);
    return vec4(pc.color.rgb * a * (1.0 - fogAt(vWorld)), 1.0);
}
// ombre de contact (F_SHADOW) : rectangle aux bords très doux
vec4 ombreContact() {
    vec2 q = abs(vUv * 2.0 - 1.0);
    float a = (1.0 - smoothstep(0.55, 1.0, q.x)) * (1.0 - smoothstep(0.6, 1.0, q.y));
    return vec4(0.0, 0.0, 0.0, a * pc.color.a);
}
// peinture : couche de vernis brillante par-dessus (reflets nets du ciel et du soleil)
void vernisPeinture(inout vec3 col, vec3 R, vec3 sun, float NoV, float NoH, float NoL, float aaKernel, float specK) {
    float Fc = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
    float acA = sqrt(0.03 * 0.03 * 0.03 * 0.03 + aaKernel);          // vernis : alpha 0,0009, élargi sur les arêtes
    float sunKc = clamp(1.0 - sqrt(aaKernel) * 4.0, 0.0, 1.0);        // disque du soleil reflété : seulement si net
    // WFS-134 (RG-06, ANO-67) : sous l'horizon, le vernis reflète la chaussée sombre, pas le sol clair du ciel (taches
    // beiges des tranches de portes et des bas de caisse vus en rasant)
    vec3 ciel = mix(skyColor(R, sunKc), F.groundCol.rgb * 0.3, smoothstep(0.0, -0.3, R.y));
    vec3 vernis = (ciel * F.misc.z * F.misc.w + D_GGX(NoH, acA) * V_Smith(NoV, NoL, acA) * sun * NoL) * Fc;
    col = col * (1.0 - Fc) + min(vernis, vec3(SPEC_MAX)) * specK;   // WFS-100 RG-01 / RG-02
}
// vitres (F_GLASS)
void vitre(inout vec3 col, inout float alpha, vec3 albedo, vec3 skyAmb, vec3 R, vec3 sun, float NoV, float NoH, float NoL) {
    float Fg = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
    col = albedo * 0.08 * skyAmb + min(skyColor(R) * F.misc.z * Fg + D_GGX(NoH, 0.0004) * 0.25 * sun * NoL, vec3(SPEC_MAX));   // WFS-100 RG-02
    alpha = clamp(0.55 + Fg, 0.0, 1.0);
}
// WFS-134 (RG-07, ANO-60) : fantôme du record ; bloc F_FANTOME de main
vec4 fantome(vec3 col, float alpha) {
    const vec3 CYAN = vec3(0.0, 0.7835, 1.0);   // #00E5FF en linéaire
    col = mix(col, CYAN * max(dot(col, vec3(0.2126, 0.7152, 0.0722)), 0.05) * 2.0, 0.3) + CYAN * 0.06;
    return vec4(col, clamp(alpha, 0.0, 1.0) * 0.45);
}
