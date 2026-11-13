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
// WFS-488 (RG-04, RG-05) : couleur de la peinture au pixel. Couleur 2 (toit, coques de rétroviseurs) posée par la partie dans
// pc.color.w = −(0xRRGGBB + 1) (src/render/vehicule_peinture2.h ; ≥ 0 : une seule couleur, aspect d'avant) ; pièces en couleur 2
// repérées au chargement par une normale de longueur 4 (sommets dédoublés au bord : limite nette sur le joint). Même vernis, même
// rugosité et même métal que la couleur principale ; sRGB → linéaire comme hexColor (game.cpp) : puissance 2,2.
bool marquePeinture2() { return length(vNrm) >= 2.0 * length(pc.model[1].xyz); }
vec4 couleurPeinture() {
    if (pc.color.w > -0.5) return pc.color;
    if (!marquePeinture2()) return vec4(pc.color.rgb, 1.0);
    uint h = uint(-pc.color.w) - 1u;   // entier exact (−0,5 serait perdu au-delà de 2²³)
    vec3 c = vec3(float((h >> 16) & 255u), float((h >> 8) & 255u), float(h & 255u)) / 255.0;
    return vec4(pow(c, vec3(2.2)), 1.0);
}
// couleur d'un matériau non peint : alpha remis à 1 quand pc.color.w porte une couleur 2 (pièce opaque, voir peinture2Piece)
vec4 couleurMateriau() { return pc.color.w < -0.5 ? vec4(pc.color.rgb, 1.0) : pc.color; }
// WFS-488 : pièce d'origine NON peinte (toit noir ou carbone pris dans les garnitures noires : Corsa, M4, Range Rover Sport, Brezza) :
// avec une couleur 2, ses pixels marqués deviennent de la peinture (couleur 2, vernis), sans carte de normales ; finition de la
// carrosserie dans pc.params.z = 4 + métal × 100 (entier) + rugosité (< 1) ; pixels non marqués : aspect d'origine
bool peinture2Piece(uint flags) { return (flags & F_PAINT) == 0u && pc.color.w < -0.5 && marquePeinture2(); }
void finitionPeinture2(inout float metal, inout float rough) { float v = pc.params.z - 4.0; metal = floor(v) / 100.0; rough = fract(v); }
// peinture : couche de vernis par-dessus (reflets du ciel et du soleil). WFS-490 : rugosité du vernis rV (0,03 = brillant, aspect
// d'avant), force du vernis kV (0 = pas de vernis : mat, chromé ; la lumière de nuit de la chaussée reste reçue) et couleur qui
// reçoit cette lumière de nuit (cNuit : couleur de la peinture, ou celle de la finition)
void vernisCouche(inout vec3 col, vec3 R, vec3 sun, float NoV, float NoH, float NoL, float aaKernel, float specK, float rV, float kV, vec3 cNuit) {
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
        col += cNuit * solNuit * (0.5 * (1.0 - Nc.y) + K_DIFFUS) * specK;   // WFS-488 : couleur 2 sur le toit (cNuit)
    }
    if (kV <= 0.0) return;   // WFS-490 : finition sans vernis (mat, chromé)
    float acA = sqrt(rV * rV * rV * rV + aaKernel);                   // vernis : alpha 0,0009 (brillant), élargi sur les arêtes
    float sunKc = clamp(1.0 - sqrt(aaKernel) * 4.0, 0.0, 1.0);        // disque du soleil reflété : seulement si net
    if (rV > 0.05) sunKc *= pow(1.0 - rV, 6.0);                       // WFS-490 : vernis satiné : pas de disque net
    // WFS-134 (RG-06, ANO-67) : sous l'horizon, le vernis reflète la chaussée sombre, pas le sol clair du ciel (taches
    // beiges des tranches de portes et des bas de caisse vus en rasant)
    // WFS-270 (RG-03) : la nuit, sous l'horizon, le vernis reflète la chaussée éclairée autour (bas de caisse, flancs)
    vec3 cielR = skyColor(R, sunKc);
    if (rV > 0.05) cielR = mix(cielR, mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5), clamp(rV * 1.6, 0.0, 1.0));   // WFS-490 : ciel flou (satiné)
    vec3 ciel = mix(cielR, max(F.groundCol.rgb * 0.3, solNuit), smoothstep(0.0, -0.3, R.y));
    vec3 vernis = (ciel * F.misc.z * F.misc.w + D_GGX(NoH, acA) * V_Smith(NoV, NoL, acA) * sun * NoL) * Fc;
    col = col * (1.0 - Fc * kV) + min(vernis, vec3(SPEC_MAX)) * specK * kV;   // WFS-100 RG-01 / RG-02 ; WFS-490 : × kV (1 = brillant)
}
void vernisPeinture(inout vec3 col, vec3 R, vec3 sun, float NoV, float NoH, float NoL, float aaKernel, float specK) {
    vernisCouche(col, R, sun, NoV, NoH, NoL, aaKernel, specK, 0.03, 1.0, couleurPeinture().rgb);   // brillant : aspect d'avant
}
// ---------------------------------------------------------------------------------------------------------------------------
// WFS-490 (épic E18) : finitions de la peinture du joueur. Un seul shader : la finition voyage dans le métal des pièces peintes
// (pc.params.x = métal + 2 × finition, src/render/vehicule_finitions.h ; 0 = brillant, valeur d'avant, aucun changement).
// Valeurs sourcées : references/reel/voitures/peintures.md (rugosités, réflectances, tailles de paillettes, flop).
const int FIN_BRILLANT = 0, FIN_METALLISE = 1, FIN_NACRE = 2, FIN_MAT = 3, FIN_SATINE = 4, FIN_CHROME = 5, FIN_CAMELEON = 6, FIN_CANDY = 7;
const float FIN_VERNIS_BRILLANT = 0.03;   // rugosité du vernis 2K brillant (≥ 90 GU à 20°) : valeur d'avant (alpha 0,0009)
const float FIN_VERNIS_SATINE = 0.18;     // vernis satiné (30 à 60 GU à 60°)
const float FIN_MAT_RUGOSITE = 0.62;      // vernis mat (< 10 GU à 60°) : reflet du soleil large et faible, aucun reflet net
const float FIN_SATINE_RUGOSITE = 0.45;   // base sous le vernis satiné
const float FIN_METAL_RUGOSITE = 0.35;    // base métallisée (paillettes d'aluminium orientées ± 15°)
const float FIN_METAL_METAL = 0.65;       // part « métal » de la base métallisée (aluminium à 15-20 % du film, réflectance 0,91)
const float FIN_NACRE_RUGOSITE = 0.30, FIN_NACRE_METAL = 0.25;   // mica enrobé d'oxyde de titane : reflet doux et teinté
const float FIN_CHROME_F0 = 0.60;         // chrome (chrome métal, n et k de Rakić 1998) : réflectance 0,55 à 0,65 dans le visible
const float FIN_CHROME_RUGOSITE = 0.06;   // chrome poli (DA-realisme § 3 : 0,05 à 0,15, métal 1)
const float FIN_CHROME_TEINTE = 0.30;     // part de la couleur choisie dans la teinte du chrome
const float FIN_CAMELEON_TOUR = 1.5;      // rotation de teinte de face → rasant (rad) : ex. rouge → orange → or → jaune-vert (décalage vers le bleu)
const float FIN_CANDY_RUGOSITE = 0.20, FIN_CANDY_METAL = 0.85;   // base argent sous le vernis teinté
const float FIN_PAILLETTE = 0.0004;       // taille d'une cellule de paillette au plus près (m) : 0,4 mm (amas de paillettes de 20-40 µm)

int lireFinition(inout float metal) {
    float f = floor(metal * 0.5);
    metal -= 2.0 * f;
    return int(clamp(f, 0.0, 7.0));
}
// position du pixel dans le repère de la voiture (les paillettes suivent la carrosserie, sans glisser ni fourmiller)
vec3 positionCarrosserie() {
    mat3 M = mat3(pc.model);
    return (transpose(M) * (vWorld - pc.model[3].xyz)) / max(dot(M[0], M[0]), 1e-8);
}
vec3 hachage33(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}
// éclat d'une paillette pour une taille de cellule t (m) : une cellule sur 6 porte une paillette plane, inclinée au hasard de ±8°
// (paillettes de série orientées dans le film, Kirchner et al. 2007) : elles ne brillent qu'autour du reflet du soleil
float eclatCellule(vec3 q, float t, vec3 N, vec3 H, float NoL) {
    vec3 h = hachage33(floor(q / t) + vec3(17.0, 59.0, 113.0));
    if (h.z > 0.16) return 0.0;
    vec3 T = normalize(cross(N, abs(N.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0))), B = cross(N, T);
    vec3 Nf = normalize(N + (T * (h.x * 2.0 - 1.0) + B * (h.y * 2.0 - 1.0)) * 0.14);   // tan 8° = 0,14
    float NfH = max(dot(Nf, H), 0.0);
    return D_GGX(NfH, 0.05) * 0.91 * NoL * 0.04;   // aluminium 0,91 ; part de la cellule couverte ≈ 0,04 (point plus fin qu'un pixel)
}
// paillettes : taille de cellule choisie d'après la taille du pixel (jamais plus petite que lui : pas de fourmillement), deux
// tailles voisines mêlées en fondu (aucun saut quand la caméra avance) ; au loin, les paillettes se fondent dans la couleur
// moyenne (base métallisée). Basse : aucune paillette (RG-10), même couleur moyenne.
float paillettes(vec3 N, vec3 H, float NoL) {
    float px = length(fwidth(vWorld));   // dérivée calculée avant tout retour anticipé (contrôle uniforme, résultat défini)
    if (F.spotN.z > 0.5 && F.spotN.z < 8.0) return 0.0;   // Basse (WFS-263 : 4 lectures en Basse, 12 sinon)
    if (NoL <= 0.0) return 0.0;
    float niv = log2(max(px / FIN_PAILLETTE, 1.0));
    float n0 = floor(niv), fr = niv - n0;
    float t0 = FIN_PAILLETTE * exp2(n0);
    if (t0 > 0.02) return 0.0;   // cellules de plus de 2 cm : paillettes invisibles à cette distance
    vec3 q = positionCarrosserie();
    float e = eclatCellule(q, t0, N, H, NoL) * (1.0 - fr) + eclatCellule(q, t0 * 2.0, N, H, NoL) * fr;
    return e * (1.0 - smoothstep(0.006, 0.02, t0));
}
// teinte tournée de a radians en TSV (teinte, saturation, valeur), sur la couleur affichée (sRGB approché, puissance 1/2,2) :
// saturation (au moins 0,85) et clarté gardées, couleurs vives (une rotation à luminance constante donnait des olives boueuses)
vec3 tournerTeinte(vec3 c, float a) {
    vec3 s = pow(max(c, vec3(0.0)), vec3(1.0 / 2.2));
    float mx = max(s.r, max(s.g, s.b)), mn = min(s.r, min(s.g, s.b)), d = mx - mn;
    float h = 0.0;
    if (d > 1e-5) h = mx == s.r ? mod((s.g - s.b) / d, 6.0) : mx == s.g ? (s.b - s.r) / d + 2.0 : (s.r - s.g) / d + 4.0;
    h = fract(h / 6.0 + a / 6.2831853);
    float sat = max(mx > 1e-5 ? d / mx : 0.0, 0.85);
    vec3 k = clamp(abs(fract(h + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
    return pow(mx * mix(vec3(1.0), k, sat), vec3(2.2));
}
// teinte normalisée (canal le plus fort = 1) : couleur d'un vernis teinté ou d'un chrome teinté
vec3 teinteNormee(vec3 c) { return c / max(max(c.r, max(c.g, c.b)), 1e-3); }

// surface sous le vernis : couleur (albedo), métal et rugosité selon la finition ; NoV = cosinus de la vue (flop, angle)
void finitionSurface(int fin, inout vec3 albedo, inout float metal, inout float rough) {
    // angle pris sur la normale lissée du maillage (sans la carte de normales) : variation continue sur la carrosserie, sans
    // taches dues au relief fin (RG-07 : pas de bandes)
    float NoVg = clamp(abs(dot(normalize(vNrm), normalize(F.camPos.xyz - vWorld))), 0.0, 1.0);
    float rasant = 1.0 - NoVg;
    if (fin == FIN_METALLISE) {
        albedo *= mix(0.15, 1.6, pow(NoVg, 0.9));   // flop : plus clair de face, plus sombre de biais (moyenne ≈ 1)
        metal = FIN_METAL_METAL; rough = FIN_METAL_RUGOSITE;
    } else if (fin == FIN_NACRE) {
        // mica : reflet légèrement froid de face, doré / rosé en rasant (interférence du TiO2)
        // lustre laiteux : ≤ 0,03 sur une couleur saturée (pas de rouge délavé), jusqu'à 0,075 sur les blancs et les gris
        float k = smoothstep(0.15, 0.85, rasant), mx = max(albedo.r, max(albedo.g, albedo.b));
        float chroma = mx > 1e-4 ? (mx - min(albedo.r, min(albedo.g, albedo.b))) / mx : 0.0;
        albedo = albedo * mix(vec3(0.97, 0.99, 1.03), vec3(1.10, 0.88, 0.70), k) + vec3(1.0, 0.83, 0.67) * mix(0.075, 0.03, chroma) * (1.0 - 0.6 * k)
               + vec3(0.09, 0.055, 0.01) * k;   // reflet doré d'interférence du TiO₂ en rasant
        metal = FIN_NACRE_METAL; rough = FIN_NACRE_RUGOSITE;
    } else if (fin == FIN_MAT) {
        metal = 0.0; rough = FIN_MAT_RUGOSITE;
    } else if (fin == FIN_SATINE) {
        metal = 0.0; rough = FIN_SATINE_RUGOSITE;
    } else if (fin == FIN_CHROME) {
        // chrome teinté : réflectance 0,60 × teinte (30 % de la couleur choisie : au-delà, un miroir teinté sous un ciel bleu
        // devient sombre et ne se lit plus comme du chrome) ; noir → chrome fumé (≈ 0,30)
        float force = max(albedo.r, max(albedo.g, albedo.b));
        albedo = FIN_CHROME_F0 * mix(vec3(1.0), teinteNormee(albedo), FIN_CHROME_TEINTE) * mix(0.5, 1.0, smoothstep(0.0, 0.2, force));
        metal = 1.0; rough = FIN_CHROME_RUGOSITE;
    } else if (fin == FIN_CAMELEON) {
        // pigment à interférence sur base sombre : la teinte tourne avec l'angle de vue ; couleur sans teinte (gris, blanc,
        // noir) : base violette (violet → rouge → or en rasant)
        float chroma = max(albedo.r, max(albedo.g, albedo.b)) - min(albedo.r, min(albedo.g, albedo.b));
        vec3 c = mix(vec3(0.20, 0.03, 0.42), albedo, smoothstep(0.05, 0.2, chroma));
        // décalage vers le bleu en rasant (interférence en couche mince : la longueur d'onde renforcée raccourcit avec l'angle)
        albedo = tournerTeinte(c, FIN_CAMELEON_TOUR * rasant);
        metal = 0.75; rough = 0.20;
    } else if (fin == FIN_CANDY) {
        // vernis teinté transparent sur base argent : le chemin dans le vernis s'allonge en biais (Beer-Lambert) → plus saturé
        // et plus sombre de biais, lumineux de face
        vec3 t = teinteNormee(albedo);
        albedo = 0.9 * pow(max(t, vec3(1e-4)), vec3(mix(1.0, 2.8, smoothstep(0.0, 1.0, rasant)))) * mix(1.0, 0.10, rasant);
        metal = FIN_CANDY_METAL; rough = FIN_CANDY_RUGOSITE;
    }
}
// vernis et paillettes de la finition, à la place de vernisPeinture (brillant : vernisPeinture, aspect d'avant)
void vernisFinition(int fin, inout vec3 col, vec3 R, vec3 sun, float NoV, float NoH, float NoL, float aaKernel, float specK,
                    vec3 N, vec3 H, float ombre, vec3 albedo, vec3 f0) {
    if (fin == FIN_BRILLANT) { vernisPeinture(col, R, sun, NoV, NoH, NoL, aaKernel, specK); return; }
    // paillettes sous le vernis (métallisé, nacré doux, caméléon, base argent du candy), au soleil seulement
    float kP = fin == FIN_METALLISE ? 1.0 : fin == FIN_CANDY ? 0.8 : fin == FIN_CAMELEON ? 0.5 : fin == FIN_NACRE ? 0.3 : 0.0;
    if (kP > 0.0) {
        vec3 teinte = fin == FIN_METALLISE ? mix(vec3(1.0), teinteNormee(albedo), 0.3) : teinteNormee(albedo);
        col += min(teinte * paillettes(N, H, NoL) * kP * sun * ombre, vec3(SPEC_MAX * 0.5)) * specK;
    }
    if (fin == FIN_CHROME) {
        // chrome : miroir, réflectance teintée f0 (Schlick). Même image du monde pour toutes les voitures et toutes les scènes :
        // au-dessus de l'horizon le ciel, en dessous la chaussée éclairée (asphalte ρ = 0,14 au soleil et sous le ciel, DA-realisme
        // § 3), horizon net (−0,05 à 0,02) ; la sonde d'environnement (décor réel, déjà ajoutée par scene.frag) garde 30 %
        vec3 Fm = f0 + (1.0 - f0) * pow(1.0 - NoV, 5.0);
        float sunKc = clamp(1.0 - sqrt(aaKernel) * 4.0, 0.0, 1.0) * pow(1.0 - FIN_CHROME_RUGOSITE, 6.0);
        float aC = sqrt(pow(FIN_CHROME_RUGOSITE, 4.0) + aaKernel);
        vec3 cielMoyen = mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5) * F.misc.z;
        vec3 chaussee = 0.14 * (sun * max(F.sunDir.y, 0.0) / PI + cielMoyen);
        vec3 monde = mix(chaussee, skyColor(R, sunKc) * F.misc.z, smoothstep(-0.05, 0.02, R.y)) * F.misc.w;
        vec3 miroir = (monde + D_GGX(NoH, aC) * V_Smith(NoV, NoL, aC) * sun * NoL) * Fm;
        col = col * (1.0 - Fm * 0.7) + min(miroir, vec3(SPEC_MAX)) * specK * 0.7;
    }
    float rV = fin == FIN_SATINE ? FIN_VERNIS_SATINE : FIN_VERNIS_BRILLANT;
    float kV = (fin == FIN_MAT || fin == FIN_CHROME) ? 0.0 : fin == FIN_SATINE ? 0.85 : 1.0;   // satiné : vernis moins réfléchissant
    vec3 cNuit = fin == FIN_CHROME ? f0 * 0.5 : albedo;
    vernisCouche(col, R, sun, NoV, NoH, NoL, aaKernel, specK, rV, kV, cNuit);
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
