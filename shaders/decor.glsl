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

// ---------------------------------------------------------------- WFS-161 (ST-3, RG-03) : intérieurs derrière les vitres
// « Interior mapping » (J. van Dongen, « Interior Mapping: a new technique for rendering realistic buildings », CGI 2008) :
// le rayon du regard est prolongé derrière la vitre jusqu'au fond, aux murs, au sol ou au plafond d'une pièce virtuelle,
// sans aucun triangle de plus. Données de la baie dans ses uv (archi_geo.cpp) : uv.x = largeur (dm) × 100 + allège (dm)
// + u, uv.y = hauteur (dm) + v, u et v de 0 à 1 sur la vitre. La pièce : largeur de la baie, du sol (allège sous l'appui)
// au plafond (0,3 m au-dessus du linteau), 4 m de profondeur.
// Lumière : pc.emissive = couleur de la lampe × luminance des murs (cd/m², calculée par le C++) ; pc.params.z = part des
// fenêtres allumées (0 le jour). Le jour, la pièce ne reçoit que 2 % de la lumière du ciel (facteur de lumière du jour).
const uint F_INTERIEUR = 1048576u;
const float PIECE_PROF = 4.0, PIECE_PLAFOND = 0.3, JOUR_FACTEUR = 0.02;

float hashFenetre(vec3 p) { return fract(sin(dot(floor(p * 4.0), vec3(12.9898, 78.233, 37.719))) * 43758.5453); }

vec3 interieurFenetre(vec3 N, vec3 V, vec3 skyAmb) {
    const float l = floor(vUv.x / 100.0) * 0.1, al = mod(floor(vUv.x), 100.0) * 0.1, h = floor(vUv.y) * 0.1;
    if (l <= 0.0 || h <= 0.0) return vec3(0.0);
    const vec2 f = fract(vUv);
    const vec3 U = vec3(0.0, 1.0, 0.0), T = normalize(cross(U, N));
    // origine de la baie (coin bas gauche, dans le plan de la vitre) : graine de « allumée ou non »
    const vec3 origine = vWorld - T * (f.x * l) - U * (f.y * h);
    // rayon dans le repère de la pièce (mètres) : x le long de la baie, y vers le haut depuis l'appui, z vers le fond
    const vec3 o = vec3(f.x * l, f.y * h, 0.0), d = vec3(dot(-V, T), dot(-V, U), dot(V, N));
    if (d.z <= 1e-4) return vec3(0.0);
    const float tx = d.x > 0.0 ? (l - o.x) / d.x : (d.x < 0.0 ? -o.x / d.x : 1e9);
    const float ty = d.y > 0.0 ? (h + PIECE_PLAFOND - o.y) / d.y : (d.y < 0.0 ? (-al - o.y) / d.y : 1e9);
    const float tz = PIECE_PROF / d.z, t = min(tx, min(ty, tz));
    const vec3 p = o + d * t;
    // albédo des surfaces de la pièce : plafond blanc, murs clairs, fond un peu plus chaud, sol en bois
    float rho = 0.5;
    if (t == ty) rho = d.y > 0.0 ? 0.8 : 0.25;          // plafond / sol
    else if (t == tz) rho = 0.55 + 0.1 * step(0.6, fract(p.x * 0.7));   // fond : un meuble ou un cadre de temps en temps
    // un meuble bas devant le mur du fond (canapé, buffet), au hasard
    if (t == tz && p.y < 0.85 - al && hashFenetre(origine + 0.5) > 0.4) rho *= 0.45;
    const bool allumee = hashFenetre(origine) < pc.params.z;
    // exposition physique (WFS-151) ; sans éclairage physique : exposition de référence de la nuit (EV100 4) ou du jour (15)
    const float k = F.iblB.x > 0.0 ? F.iblB.x : (F.misc.y > 0.5 ? 1.0 / (1.2 * 16.0) : 1.0 / (1.2 * 32768.0));
    vec3 c = allumee ? pc.emissive.rgb * (rho / 0.5) * k : rho * skyAmb * JOUR_FACTEUR;
    // lampe au plafond : un peu plus de lumière vers le haut de la pièce, ombre douce vers le fond
    if (allumee) c *= mix(0.75, 1.15, clamp((p.y + al) / (h + al + PIECE_PLAFOND), 0.0, 1.0)) * mix(1.0, 0.7, p.z / PIECE_PROF);
    return c;
}

// ---------------------------------------------------------------- WFS-161 (ST-4, RG-04) : façades, repli procédural PROVISOIRE
// En attendant les textures scannées CC0 (Poly Haven, dépôt dunkean/magna-urbis : accès LFS à obtenir), l'appareil de la façade
// est dessiné ici. Règle de fidélité (M-03) : la moyenne de l'albédo sur la façade reste celle du référentiel (pc.color,
// references/reel/materiaux.json) : la teinte du mortier est fixée et celle des pierres / briques est calculée pour compenser
// la part des joints ; les variations d'une pierre à l'autre sont de moyenne nulle. uv en mètres (x le long du mur, y depuis
// le sol). Type d'appareil dans pc.params.z : 0 crépi, 1 pierre de taille, 2 brique, 3 moellons, 4 béton.
const uint F_FACADE = 2097152u;
const float MORTIER = 0.45;                   // albédo du mortier de chaux (gris clair)

// joints d'un appareil réglé (assises de hauteur h, éléments de longueur l, décalés d'une demi-longueur une assise sur deux) :
// rend 1 dans le joint, 0 dans l'élément ; id = identifiant de l'élément (variation de teinte)
float joint(vec2 p, float l, float h, float e, out vec2 id) {
    float rang = floor(p.y / h);
    float x = p.x / l + 0.5 * mod(rang, 2.0);
    id = vec2(floor(x), rang);
    vec2 f = vec2(fract(x) * l, fract(p.y / h) * h);
    vec2 aw = fwidth(p) + 1e-4;   // joint anticrénelé (largeur du pixel)
    float jx = 1.0 - smoothstep(e * 0.5 - aw.x, e * 0.5 + aw.x, min(f.x, l - f.x));
    float jy = 1.0 - smoothstep(e * 0.5 - aw.y, e * 0.5 + aw.y, min(f.y, h - f.y));
    return max(jx, jy);
}

void surfaceFacade(inout vec4 base, inout float rough) {
    const vec3 ref = base.rgb;                // albédo moyen du référentiel
    const vec2 p = vUv;
    const int type = int(pc.params.z + 0.5);
    vec3 c = ref;
    if (type == 1 || type == 2) {
        // pierre de taille : assises de 0,40 m, blocs de 0,80 m, joints de 8 mm ; brique (module 22 × 6,5 cm + joints de
        // 1 cm : pas de 0,23 × 0,075 m, appareil en panneresses)
        const float l = type == 1 ? 0.80 : 0.23, h = type == 1 ? 0.40 : 0.075, e = type == 1 ? 0.008 : 0.010;
        vec2 id; float j = joint(p, l, h, e, id);
        const float fj = 1.0 - (l - e) * (h - e) / (l * h);           // part de la surface occupée par les joints
        const vec3 element = max((ref - fj * MORTIER) / (1.0 - fj), vec3(0.0));   // compense : moyenne = référentiel
        const float var = (hash(id + 7.0) - 0.5) * (type == 1 ? 0.10 : 0.18);   // teinte d'un élément à l'autre (moyenne nulle)
        // M-03 à toute résolution : quand le pixel dépasse le joint, l'anticrénelage l'élargit et la moyenne glisse vers le
        // mortier (cubemap de 128 ou 256 px, façade lointaine) : le joint tend alors vers sa part moyenne fj (banc des passes,
        // M-01 de WFS-150 : cubemap contre scène à ± 1 %)
        const vec2 pw = fwidth(p);
        j = mix(j, fj, smoothstep(0.5 * e, 2.0 * e, max(pw.x, pw.y)));
        c = mix(element * (1.0 + var), vec3(MORTIER), j);
        rough = mix(rough, 0.95, j);
    } else if (type == 3) {   // moellons : cellules irrégulières de 0,25 à 0,4 m, joints épais
        vec2 g = p / 0.32, gi = floor(g); float d1 = 9.0, d2 = 9.0; vec2 idc = gi;
        for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) {
            vec2 o = vec2(x, y), q = o + hash22(gi + o) * 0.8 + 0.1 - fract(g);
            float d = dot(q, q);
            if (d < d1) { d2 = d1; d1 = d; idc = gi + o; } else if (d < d2) d2 = d;
        }
        float j = 1.0 - smoothstep(0.02, 0.06, sqrt(d2) - sqrt(d1));
        const float fj = 0.18;                                        // part des joints (mesurée sur ce motif, à ± 0,02)
        const vec3 pierre = max((ref - fj * MORTIER) / (1.0 - fj), vec3(0.0));
        c = mix(pierre * (1.0 + (hash(idc) - 0.5) * 0.3), vec3(MORTIER), j);
    } else {   // crépi (0) et béton (4) : grain fin, moyenne nulle
        const float grain = (fbm(p * (type == 0 ? 9.0 : 3.0)) - 0.47) * (type == 0 ? 0.16 : 0.08);
        c = ref * (1.0 + grain);
    }
    // RG-04 : salissure en pied de mur (rejaillissement de la pluie, 0 à 0,6 m) et coulures verticales (eau sous les appuis)
    const float pied = 1.0 - 0.22 * (1.0 - smoothstep(0.0, 0.6, p.y)) * (0.7 + 0.6 * fbm(p * vec2(3.0, 1.0)));
    const float coulure = 1.0 - 0.10 * smoothstep(0.55, 0.9, fbm(vec2(p.x * 6.0, p.y * 0.35)));
    base.rgb = c * pied * coulure;
}
