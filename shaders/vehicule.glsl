// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E10 Véhicules. Fondu des versions allégées, ombre de contact, tache des phares, vernis, vitres, fantôme.

// WFS-134 (RG-12) : fondu entre la voiture complète et sa version allégée (tramage écran 4×4 complémentaire)
void fonduLod() {
    ivec2 q = ivec2(gl_FragCoord.xy) & 3;
    const float B[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);
    float seuil = (B[q.y * 4 + q.x] + 0.5) / 16.0, w = -pc.emissive.w;
    if (!SANS_DISCARD && (w < 2.5 ? seuil >= w - 1.0 : seuil < w - 3.0)) discard;   // WFS-222
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
// WFS-270 (RG-01, RG-02) : éclairement moyen de la chaussée autour de la voiture la nuit (phares, lampadaires), en lux
// pré-exposés : 3 points au sol (4 m, 6,5 m et 10 m devant le centre de la voiture), lumières de l'image (E13, lecture seule).
// Même valeur pour tous les pixels d'une voiture (branche cohérente) ; 0 de jour (aucun projecteur, et appel réservé à la nuit).
vec3 eclairementRouteAutour() {
    vec3 o = pc.model[3].xyz;
    vec3 av = vec3(pc.model[2].x, 0.0, pc.model[2].z);   // avant du modèle = −Z local (phares en −L/2, nuit.cpp)
    av = dot(av, av) > 1e-8 ? -normalize(av) : vec3(0.0, 0.0, 1.0);
    int n = int(F.spotN.x + 0.5);
    vec3 E = vec3(0.0);
    const float DEVANT[3] = float[3](4.0, 6.5, 10.0);   // m devant le centre de la voiture
    for (int j = 0; j < 3; j++) {
        vec3 p = o + av * DEVANT[j] + vec3(0.0, 0.02, 0.0);
        for (int i = 0; i < 12; i++) {
            if (i >= n) break;
            vec3 Lv = F.spotPos[i].xyz - p;
            float d = length(Lv);
            vec3 L = Lv / max(d, 1e-4);
            float cosT = dot(L, -F.spotDir[i].xyz), NoL = max(L.y, 0.0);
            bool croisement = F.spotCone[i].z > 0.5;
            if ((!croisement && cosT <= F.spotCone[i].x) || (croisement && (d > F.spotPos[i].w || cosT < 0.17)) || NoL <= 0.0) continue;
            if (d < 2.0) continue;   // champ proche d'une optique (1/d² non borné) : pas un éclairement de la chaussée voisine
            float range = F.spotPos[i].w, decay = F.spotDir[i].w;
            float att = (range > 0.0 && decay > 0.0) ? pow(clamp(1.0 - d / range, 0.0, 1.0), decay) : 1.0;
            E += F.spotCol[i].rgb * (croisement ? feuCroisement(-L, F.spotDir[i].xyz, d, range) : smoothstep(F.spotCone[i].x, F.spotCone[i].y, cosT) * att) * NoL;
        }
    }
    return E / 3.0;
}
// peinture : couche de vernis brillante par-dessus (reflets nets du ciel et du soleil)
void vernisPeinture(inout vec3 col, vec3 R, vec3 sun, float NoV, float NoH, float NoL, float aaKernel, float specK) {
    float Fc = 0.04 + 0.96 * pow(1.0 - NoV, 5.0);
    vec3 solNuit = vec3(0.0);   // WFS-270 : luminance de la chaussée éclairée autour de la voiture (0 de jour)
    if (F.misc.y > 0.5) {
        // WFS-270 (RG-02) : la nuit, la peinture reçoit aussi la lumière de la chaussée éclairée qui l'entoure : rebond du sol
        // (asphalte usé ρ = 0,14, DA-realisme § 3 ; facteur de vue du sol (1 − N.y) / 2) et lumière diffusée par l'air humide
        // et le décor proche (part isotrope K_DIFFUS, calée sur RG-02 : carrosserie / route entre 0,4 et 1,5, journal).
        // Hors des lumières (aucun projecteur proche) : 0, la peinture garde le seul ciel de lune (iblDiffus), comme la route.
        // Luminance de la chaussée compressée au-delà de L_SAT (pré-exposée) : sous plusieurs faisceaux proches (grille de
        // départ en ville), la peinture ne sature pas en blanc ; elle garde sa teinte.
        const float RHO_ROUTE = 0.14, K_DIFFUS = 2.0, L_SAT = 0.5;
        vec3 Nc = normalize(R + normalize(F.camPos.xyz - vWorld));   // normale de la pièce (R = reflect(−V, N))
        // WFS-423 (RG-02) : en Basse, valeur calculée une fois par voiture sur le CPU (src/render/eclairement_voiture.h, même formule),
        // posée dans pc.nuit (w = 1) ; sinon (Moyenne, Haute, pièces hors voiture) : calcul d'avant, ici
        solNuit = RHO_ROUTE * (pc.nuit.w > 0.5 ? pc.nuit.rgb : eclairementRouteAutour()) / PI;
        solNuit /= 1.0 + dot(solNuit, vec3(0.2126, 0.7152, 0.0722)) / L_SAT;
        col += pc.color.rgb * solNuit * (0.5 * (1.0 - Nc.y) + K_DIFFUS) * specK;
    }
    float acA = sqrt(0.03 * 0.03 * 0.03 * 0.03 + aaKernel);          // vernis : alpha 0,0009, élargi sur les arêtes
    float sunKc = clamp(1.0 - sqrt(aaKernel) * 4.0, 0.0, 1.0);        // disque du soleil reflété : seulement si net
    // WFS-134 (RG-06, ANO-67) : sous l'horizon, le vernis reflète la chaussée sombre, pas le sol clair du ciel (taches
    // beiges des tranches de portes et des bas de caisse vus en rasant)
    // WFS-270 (RG-03) : la nuit, sous l'horizon, le vernis reflète la chaussée éclairée autour (bas de caisse, flancs)
    vec3 ciel = mix(skyColor(R, sunKc), max(F.groundCol.rgb * 0.3, solNuit), smoothstep(0.0, -0.3, R.y));
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
