// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E9 Routes. Bruits (aussi lus par decor.glsl : sol), chaussée sèche et mouillée, trottoirs.

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p); vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p) { float s = 0.0, a = 0.5; for (int i = 0; i < 4; i++) { s += a * noise(p); p *= 2.03; a *= 0.5; } return s; }
// WFS-16 : bruit de la route mouillée (rh12 / rnoise de roadShader, js/world/renderer.js), en coordonnées monde
float rh12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float rnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(rh12(i), rh12(i + vec2(1.0, 0.0)), f.x), mix(rh12(i + vec2(0.0, 1.0)), rh12(i + vec2(1.0, 1.0)), f.x), f.y);
}
// ---------- WFS-205 (RG-01, RG-03) : revêtements réels (scans CC0 calibrés, src/render/route_revetements.*) ----------
// Cartes du matériau : texColor (albédo sRGB), texNormal (normales, vert = +v), texMR (occlusion, rugosité, métal).
// Lecture sans répétition visible (avis de l'expert-realisme-3d, journal de WFS-205) ; mode = pc.params.z (qualité) :
//   2 (Ultra, Haute) : pavage hexagonal (Mikkelsen, JCGT 2022 ; grille triangulaire de Heitz et Neyret 2018) : 3 lectures par
//     carte, rotation et décalage aléatoires par cellule, poids accentués (contraste gardé), dérivées tournées (pas de couture
//     de mipmap) ; normales tournées avec la cellule ;
//   1 (Moyenne) : deux échelles (1 et × 0,41 tournée de 37°) mélangées par la macro-variation : 2 lectures (albédo, normale), 1 (ORM) ;
//   0 (Basse) : une lecture.
// Toujours : macro-variations procédurales à 23 et 47 m (périodes sans multiple commun) : albédo ± 12 %, rugosité ± 0,04.
const float COTE_SCAN = 2.0;   // côté d'un scan en mètres (Poly Haven, E : references/reel/routes/revetements.json)
struct Revet { vec3 albedo; vec3 n; float rough; float ao; float lon; };   // lon : longueur moyenne des normales filtrées (Toksvig)
vec2 hash22(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.xx + p3.yz) * p3.zy); }
// Filtrage des normales (Toksvig 2005, LEAN) : les mipmaps de tools/textures/convertir.py sont RENORMALISÉES (WFS-149) : une
// mipmap grossière garde des pentes aussi fortes que le niveau 0, et la chaussée vue de loin (ou dans la capture d'environnement
// 256 px de WFS-150) brille autrement qu'au plus près. Ici, l'empreinte du pixel en texels (ρ) dit quelle part du relief est
// « sous le pixel » : le relief s'aplatit (facteur 1 / (1 + (ρ − 1) / 2) au-delà d'un texel) et la pente perdue passe dans la
// rugosité (α² += σ² × (1 − facteur²), σ² = variance des pentes d'un scan d'asphalte, E). Même reflet moyen à toutes les distances.
const float SIGMA2_PENTES = 0.08;
float facteurRelief(vec2 dx, vec2 dy) {
    float rho = max(length(dx), length(dy)) * float(textureSize(texNormal, 0).x);
    return 1.0 / (1.0 + max(rho - 1.0, 0.0) * 0.5);
}
float roughToksvig(float rough, float lon) {   // lon : facteur de relief gardé (1 : tout le relief au pixel)
    float v = SIGMA2_PENTES * (1.0 - lon * lon);
    return clamp(pow(pow(rough, 4.0) + v, 0.25), 0.0, 1.0);
}
Revet lire1(vec2 st, vec2 dx, vec2 dy, mat2 R) {
    Revet r;
    r.albedo = textureGrad(texColor, st, dx, dy).rgb;
    vec3 n = textureGrad(texNormal, st, dx, dy).xyz * 2.0 - 1.0;
    r.n = vec3(transpose(R) * n.xy, n.z);   // texture lue tournée de R : la pente revient dans le repère de la route
    r.lon = facteurRelief(dx, dy);          // part du relief que le pixel voit (empreinte en texels)
    r.n.xy *= r.lon;
    vec3 orm = textureGrad(texMR, st, dx, dy).rgb;
    r.ao = orm.r; r.rough = orm.g;
    return r;
}
mat2 rotation(float a) { float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }
Revet lireRevetement(vec2 uvM, float mode, bool joints) {
    vec2 st = uvM / COTE_SCAN, dx = dFdx(st), dy = dFdy(st);
    if (joints || mode < 0.5) {   // Basse, ou dalles à joints (trottoir) : pas de rotation (les joints resteraient tordus)
        return lire1(st, dx, dy, mat2(1.0));
    }
    if (mode < 1.5) {   // Moyenne : deux échelles
        mat2 R = rotation(0.6458);   // 37°
        Revet a = lire1(st, dx, dy, mat2(1.0));
        Revet b = lire1(R * st * 0.41 + 0.37, R * dx * 0.41, R * dy * 0.41, R);
        float m = smoothstep(0.3, 0.7, rnoise(uvM * 0.043 + 11.0));
        Revet r; r.albedo = mix(a.albedo, b.albedo, m); r.n = normalize(mix(a.n, b.n, m));
        r.rough = mix(a.rough, b.rough, m); r.ao = mix(a.ao, b.ao, m); r.lon = mix(a.lon, b.lon, m);
        return r;
    }
    // Haute / Ultra : pavage hexagonal (grille triangulaire ; 3 sommets, poids barycentriques)
    vec2 g = st * 3.4641;   // 2·√3 : une cellule ≈ un demi-scan
    vec2 sk = mat2(1.0, 0.0, -0.57735027, 1.15470054) * g;
    vec2 b0 = floor(sk); vec3 t = vec3(fract(sk), 0.0); t.z = 1.0 - t.x - t.y;
    vec3 w; vec2 v1, v2, v3;
    if (t.z > 0.0) { w = vec3(t.z, t.y, t.x); v1 = b0; v2 = b0 + vec2(0, 1); v3 = b0 + vec2(1, 0); }
    else { w = vec3(-t.z, 1.0 - t.y, 1.0 - t.x); v1 = b0 + vec2(1, 1); v2 = b0 + vec2(1, 0); v3 = b0 + vec2(0, 1); }
    w = pow(w, vec3(5.0)); w /= dot(w, vec3(1.0));   // poids accentués : le mélange ne délave pas le contraste
    Revet r; r.albedo = vec3(0.0); r.n = vec3(0.0); r.rough = 0.0; r.ao = 0.0; r.lon = 0.0;
    vec2 vs[3] = vec2[3](v1, v2, v3);
    for (int k = 0; k < 3; k++) {
        vec2 h = hash22(vs[k]);
        mat2 R = rotation(h.x * 6.2831853);
        Revet e = lire1(R * st + h * 17.0, R * dx, R * dy, R);
        r.albedo += e.albedo * w[k]; r.n += e.n * w[k]; r.rough += e.rough * w[k]; r.ao += e.ao * w[k]; r.lon += e.lon * w[k];
    }
    r.n = normalize(r.n);
    return r;
}
// macro-variations (albédo × 1 ± 12 %, rugosité ± 0,04) en coordonnées monde : taches, zones refaites, sans motif à 100 m
vec2 macroRevetement(vec2 w) {
    float m = rnoise(w / 23.0) * 0.6 + rnoise(w / 47.0 + 5.3) * 0.4;   // 0..1
    return vec2(1.0 + (m - 0.5) * 0.24, (m - 0.5) * 0.08);
}

// ---------- WFS-206 (RG-01, RG-02 ; M-03, M-04) : usure, raccords, rives et bordures propres à chaque carte ----------
// Constantes identiques à src/render/route_revetements.h (test_fidelite_routes les compare) :
const float BANDE_ROULEMENT_M = 0.9;     // M-03 : largeur d'une bande de roulement (0,8-1,0 m)
const float ECARTEMENT_ROUES_M = 1.572;  // M-03 : écartement des deux bandes d'une voie = voie moyenne des voitures (voitures.json)
const float BORDURE_LARGEUR_M = 0.30;    // M-04 : bloc de granit de la bordure (Ville), au bord du trottoir
const float DEMI_CHAUSSEE_M = 7.5;
const float LARGEUR_MARQUAGE_M = 0.15;   // WFS-594 (RG-03) : lignes de rive et de voie en ville (IISR : 2u, u = 7,5 cm) ; = route_revetements.h
const float MARQUAGE_ALBEDO = 0.68;      // WFS-594 (RG-03) : peinture routière blanche (0,6 à 0,75, DA-realisme.md § 3) ; = route_revetements.h
const float RACCORD_ALB_RECENT = 0.93;   // WFS-244 (RG-02) : enrobé repris récent / ancien, facteur d'albédo (voisin : 0,87 à 1,02)
const float RACCORD_ALB_ANCIEN = 0.97;   //   écart ≤ 8 % avec la moyenne d'une voie (test_fidelite_routes)       // RW (src/game/track.h) : le trottoir commence là (uv.x = latéral en mètres)
// pc.params.z = mode de lecture + 4 × carte + 16 × neige + 256 × tour (route::codeMateriau) ; carte : 0 Ville, 1 Campagne, 2 Corniche, 3 Grand Prix ;
// WFS-346 : neige au sol de la carte (saison du paquet), en huitièmes (route::NEIGE_PAS)
float modeLecture() { return mod(pc.params.z, 4.0); }
int carteRoute() { return int(mod(floor(pc.params.z / 4.0 + 0.01), 4.0)); }
float neigeRoute() { return mod(floor(pc.params.z / 16.0 + 0.01), 16.0) / 8.0; }
// WFS-346, WFS-392 (RG-01) : sous le toit de chaque tunnel la chaussée est sèche. pc.params.z porte aussi 256 × longueur du tour en
// segments (0 : aucun abri) ; pc.color.xyz (couleur inutilisée par la chaussée et le trottoir) : un abri par case, entier
// = début × 1024 + longueur, en segments le long de la route (route::coderAbris ; 0 : case vide) ; neige apportée par les roues
// sur les 8 premiers mètres
const float SEG_ROUTE_M = 1.2;           // SEG_M (src/game/track.h) ; même valeur dans route_revetements.h (test_cartes_v2_abris)
float abriRoute(float y) {
    float tour = floor(pc.params.z / 256.0 + 0.001) * SEG_ROUTE_M;
    if (tour <= 0.0) return 1.0;
    float d = mod(y, tour), sec = 0.0;
    for (int k = 0; k < 3; k++) {
        float c = pc.color[k], lon = mod(c, 1024.0);
        if (lon < 0.5) continue;
        float debut = floor(c / 1024.0 + 0.001) * SEG_ROUTE_M;
        sec = max(sec, smoothstep(-1.0, 8.0, min(d - debut, debut + lon * SEG_ROUTE_M - d)));   // m sous le toit (négatif : dehors)
    }
    return 1.0 - sec;
}

// ---------- WFS-346 (RG-01) : chaussée enneigée (neige tassée), lue de la saison de la carte ----------
// Réel (references/reel/cartes_v2/neige.md, avis expert-realisme-3d) : neige tassée et salie par la circulation, gris-blanc (albédo
// ≈ 0,4 à 0,6 : 0,45 retenu, phares de nuit compris ; jamais le blanc pur saturé de la neige fraîche, ≈ 0,85, réservé aux bas-côtés) ; traces des roues de chaque voie
// plus sombres (neige compactée en glace grise, asphalte qui apparaît par endroits : albédo ≈ 0,2 à 0,3) et plus lisses (rugosité
// 0,45 : glace polie ; neige tassée 0,75) ; bourrelets plus clairs entre les traces et en rive ; relief du scan d'asphalte effacé.
// Couvre aussi les lignes peintes (sous la neige). n : couverture 0 à 1 ; halfW : demi-largeur de la chaussée (m).
void neigeSurRoute(vec2 p, float halfW, float n, inout vec3 col, inout float rough, inout vec3 N) {
    float voie = 2.0 * halfW / 3.0;
    float k = clamp(floor((p.x + halfW) / voie), 0.0, 2.0);
    float c = -halfW + (k + 0.5) * voie;
    float derive = (rnoise(vec2(k * 7.1, p.y * 0.035)) - 0.5) * 0.3;               // trajectoires qui ondulent (± 15 cm)
    float dx = abs(abs(p.x - c - derive) - ECARTEMENT_ROUES_M * 0.5);
    float ornieres = 1.0 - smoothstep(0.16, 0.34, dx + (rnoise(p * vec2(2.3, 0.4)) - 0.5) * 0.12);   // traces de 0,35 à 0,6 m, bords irréguliers
    float g = fbm(p * vec2(1.3, 0.35)), fin = noise(p * 11.0);
    vec3 tassee = vec3(0.44, 0.46, 0.50) * (0.9 + 0.18 * g) * (0.96 + 0.08 * fin);   // neige tassée, légèrement bleutée
    vec3 trace = mix(vec3(0.22, 0.23, 0.25), col, 0.35 * smoothstep(0.55, 0.8, g));    // glace grise, asphalte qui perce par endroits
    float rive = 1.0 - smoothstep(0.0, 0.9, halfW - abs(p.x));                            // neige repoussée en rive : plus claire
    vec3 neige = mix(tassee, vec3(0.70, 0.72, 0.76), rive * 0.6);
    neige = mix(neige, trace, ornieres * 0.85);
    col = mix(col, neige, n);
    rough = mix(rough, mix(0.75, 0.45, ornieres), n);
    N = normalize(mix(N, normalize(vNrm), n * 0.85));
}
// usure de la chaussée au point p (uv en mètres) : x = facteur d'albédo, y = écart de rugosité, z = facteur de relief (normales)
//   - bandes de roulement (au pas des roues de chaque voie) : liant poli et gomme : plus sombres (− 13 %), plus lisses (− 0,05),
//     relief émoussé (80 % de la macrotexture) ; entre les bandes et en rive : granulats à nu, un peu plus clairs (+ 2 %) et
//     grenus ; albédo moyen d'une voie ≤ 1 × le revêtement (l'usure ne rend pas la route plus claire : avis expert-realisme-3d)
//   - raccords de chantier : rectangles d'enrobé repris sur une voie (3 à 15 m), plus sombres s'ils sont récents, joint de
//     bitume de 3 cm brillant ; Ville fréquents, Campagne parfois, Corniche rarement, Grand Prix jamais (piste resurfacée d'un bloc).
vec3 usureRoute(vec2 p, float halfW, int carte) {
    float voie = 2.0 * halfW / 3.0;                                    // 3 voies (marquage de surfaceRoute)
    float k = clamp(floor((p.x + halfW) / voie), 0.0, 2.0);
    float c = -halfW + (k + 0.5) * voie;                               // axe de la voie
    float derive = (rnoise(vec2(k * 7.1, p.y * 0.035)) - 0.5) * 0.25;   // trajectoires : les bandes ondulent un peu (± 12 cm)
    float dx = abs(abs(p.x - c - derive) - ECARTEMENT_ROUES_M * 0.5);
    float bande = 1.0 - smoothstep(BANDE_ROULEMENT_M * 0.5 - 0.18, BANDE_ROULEMENT_M * 0.5 + 0.08, dx);
    float force = carte == 3 ? 0.5 : 1.0;                              // circuit : gomme surtout sur la trajectoire, usure faible
    bande *= force * (0.75 + 0.25 * rnoise(p * vec2(0.6, 0.08)));
    float alb = mix(1.02, 0.87, bande), dr = mix(0.02, -0.05, bande), rel = mix(1.0, 0.8, bande);   // moyenne d'une voie ≤ 1 (avis expert)
    // raccords : une case de 37 m par voie
    // WFS-244 (ANO-116) : les raccords de WFS-206 se voyaient comme des plaques rectangulaires à bords nets (albédo × 0,72,
    // − 28 %, bords droits sans anticrénelage ; deux ou trois voies voisines dans la même case faisaient un rectangle sur toute
    // la chaussée). Aspect réel (RG-02) : écart ≤ 8 % avec l'asphalte voisin, bords irréguliers (découpe à la scie reprise,
    // bord effrité ± 12 cm), joint de bitume fondu à l'écran, une voie au plus. Voies 0 et 2 dans les cases paires, voie 1 dans
    // les impaires (jamais deux voies voisines côte à côte), probabilité doublée : même nombre de raccords qu'avant (RG-03).
    float proba = carte == 0 ? 0.45 : carte == 1 ? 0.25 : carte == 2 ? 0.12 : 0.0;
    float cy = floor(p.y / 37.0);
    vec2 h = hash22(vec2(cy * 3.7 + k * 11.3, k * 1.9 + 4.0));
    bool caseDeLaVoie = mod(cy + k, 2.0) < 0.5;
    if (caseDeLaVoie && h.x < proba * 2.0) {
        float hx = h.x / (proba * 2.0);                                       // 0..1 : tirages propres au raccord
        float y0 = cy * 37.0 + h.y * 18.0, lg = 3.0 + fract(hx * 91.7) * 12.0;
        float x0 = c - voie * 0.5 + 0.3 + fract(h.y * 37.1) * 0.6, x1 = c + voie * 0.5 - 0.2 - fract(h.y * 53.3) * 0.8;
        float bord = min(min(p.x - x0, x1 - p.x), min(p.y - y0, y0 + lg - p.y));   // > 0 : dans le raccord
        bord += (rnoise(p * 1.7 + h * 31.0) - 0.5) * 0.16 + (rnoise(p * 6.3 + 2.0) - 0.5) * 0.08;   // bord irrégulier (± 12 cm)
        float pix = max(fwidth(bord), 1e-4);                                  // taille du pixel en mètres : bords anticrénelés
        float dedans = smoothstep(-pix, pix, bord);
        if (dedans > 0.0) {
            bool recent = fract(hx * 13.1 + h.y * 5.3) < 0.5;
            float a = recent ? RACCORD_ALB_RECENT : RACCORD_ALB_ANCIEN, d = recent ? -0.03 : 0.0, rl = recent ? 0.85 : 1.0;   // enrobé repris : pas encore usé
            alb = mix(alb, a, dedans); dr = mix(dr, d, dedans); rel = mix(rel, rl, dedans);
            // joint de bitume coulé (3 cm, sombre et brillant) : couvert au prorata du pixel, il s'efface au loin (pas de liseré)
            float joint = (1.0 - smoothstep(0.03 - pix, 0.03 + pix, abs(bord))) * clamp(0.03 / pix, 0.0, 1.0);
            alb = mix(alb, 0.6, joint); dr = mix(dr, -0.25, joint); rel = mix(rel, 0.3, joint);
        }
    }
    return vec3(alb, dr, rel);
}
// ---------- WFS-594 (RG-02) : défauts de l'enrobé vus de près : fissures pontées, fissures fines, rustines, gouttes d'huile ----------
// Réel (catalogue des dégradations de surface des chaussées, LCPC 1998 ; avis expert-realisme-3d) : fissures transversales de retrait
// tous les 8 à 25 m, une sur deux pontée (bande de bitume coulé de 4 à 8 cm, noire et brillante : albédo × 0,55, rugosité − 0,25) ;
// fissure longitudinale au joint de deux bandes de répandage (axe des lignes de voie, ± 30 cm) ; rustines (nid-de-poule rebouché à
// l'enrobé à froid, 0,4 à 1,2 m, bord irrégulier, plus sombres si récentes) ; gouttes d'huile au milieu de la voie (albédo × 0,9).
// Ville fréquents, Corniche moins, Campagne entre les deux, Grand Prix jamais. Tout ce qui est plus fin que le pixel s'efface au
// prorata de sa couverture : aucun scintillement au loin (pix = taille du pixel en mètres).
const float PONTAGE_ALB = 0.55;     // bitume coulé / enrobé voisin (test_fidelite_routes : 0,45 à 0,65)
const float FISSURE_PAS_MIN_M = 8.0, FISSURE_PAS_MAX_M = 25.0;   // écart des fissures transversales
vec3 defautsChaussee(vec2 p, float halfW, int carte, float pix) {
    float alb = 1.0, dr = 0.0, rel = 1.0;
    if (carte == 3) return vec3(alb, dr, rel);
    float dens = carte == 0 ? 1.0 : carte == 1 ? 0.75 : 0.55;
    float voie = 2.0 * halfW / 3.0;
    // fissures transversales : au plus une par case de 25 m (écart de 8 à 25 m d'une fissure à l'autre en moyenne, RG-02)
    {
        float cy = floor(p.y / FISSURE_PAS_MAX_M);
        vec2 h = hash22(vec2(cy * 1.37 + 3.1, 7.7));
        if (h.x < dens * 0.85) {
            float y0 = cy * FISSURE_PAS_MAX_M + 1.0 + (FISSURE_PAS_MAX_M - 2.0) * h.y;
            float yc = y0 + (rnoise(vec2(p.x * 0.6, cy)) - 0.5) * 0.3 + (rnoise(vec2(p.x * 5.1, cy + 3.0)) - 0.5) * 0.05;   // tracé presque droit (retrait thermique), qui ondule de ± 15 cm
            float x0 = -halfW + fract(h.x * 17.3) * halfW * 0.8, x1 = halfW - fract(h.y * 23.9) * halfW * 0.8;   // pas toujours sur toute la largeur
            float dedansX = smoothstep(x0 - 0.3, x0 + 0.3, p.x) * (1.0 - smoothstep(x1 - 0.3, x1 + 0.3, p.x));
            float d = abs(p.y - yc);
            bool pontee = fract(h.y * 41.3) < 0.55;
            if (pontee) {   // bande de bitume coulé, largeur 4 à 8 cm qui varie le long de la fissure
                float lg = 0.02 + 0.02 * rnoise(vec2(p.x * 1.3, cy + 9.0));
                float c = (1.0 - smoothstep(lg - pix, lg + pix, d)) * dedansX * clamp(lg * 2.0 / pix, 0.0, 1.0);
                alb = mix(alb, PONTAGE_ALB, c); dr = mix(dr, -0.25, c); rel = mix(rel, 0.25, c);
            }
            // fissure ouverte (5 mm, sombre) au milieu du pontage ou seule ; couverte au prorata du pixel
            float f = (1.0 - smoothstep(0.0025, 0.0025 + pix, d)) * clamp(0.005 / pix, 0.0, 1.0) * dedansX;
            alb = mix(alb, 0.35, f * (pontee ? 0.5 : 0.9)); rel = mix(rel, 0.4, f);
        }
    }
    // fissure longitudinale au joint des bandes de répandage (près des lignes de voie), par tronçons de 6 à 30 m
    for (int s = -1; s <= 1; s += 2) {
        float lx = float(s) * halfW / 3.0 + 0.3 * float(s) * (rnoise(vec2(p.y * 0.02, float(s))) - 0.3);
        float tron = smoothstep(0.55, 0.62, rnoise(vec2(p.y * 0.045 + float(s) * 13.0, 2.0))) * dens;
        if (tron <= 0.0) continue;
        float d = abs(p.x - lx - (rnoise(vec2(p.y * 1.1, float(s) * 5.0)) - 0.5) * 0.18);
        float pont = step(0.5, rnoise(vec2(p.y * 0.011 + float(s) * 7.0, 4.0)));
        float lg = 0.025;
        float c = (1.0 - smoothstep(lg - pix, lg + pix, d)) * tron * pont * clamp(lg * 2.0 / pix, 0.0, 1.0);
        alb = mix(alb, PONTAGE_ALB, c); dr = mix(dr, -0.25, c); rel = mix(rel, 0.25, c);
        float f = (1.0 - smoothstep(0.002, 0.002 + pix, d)) * clamp(0.004 / pix, 0.0, 1.0) * tron * pont;   // fissure seulement sous son pontage
        alb = mix(alb, 0.4, f * 0.5);
    }
    // rustines : une case de 13 m par voie ; découpe à la scie presque rectangulaire de 0,4 à 1,2 m (coins arrondis, bord repris à la
    // main), enrobé à peine plus sombre (récent) ou plus clair (ancien, liant usé), joint de bitume brillant
    {
        float k = clamp(floor((p.x + halfW) / voie), 0.0, 2.0);
        float cy2 = floor(p.y / 13.0);
        vec2 h = hash22(vec2(cy2 * 2.9 + k * 5.1, k + 21.0));
        if (h.x < 0.3 * dens) {
            float c = -halfW + (k + 0.5) * voie + (fract(h.y * 7.3) - 0.5) * voie * 0.6;
            vec2 centre = vec2(c, cy2 * 13.0 + 2.0 + h.y * 9.0);
            vec2 demi = vec2(0.2 + 0.35 * fract(h.x * 31.7), 0.2 + 0.4 * fract(h.y * 19.1));
            float ang = (fract(h.x * 53.1) - 0.5) * 0.3;                       // découpe un peu de biais (± 9°)
            vec2 q = rotation(ang) * (p - centre);
            vec2 e = abs(q) - demi + 0.06;
            float bord = -(length(max(e, 0.0)) + min(max(e.x, e.y), 0.0) - 0.06);   // > 0 dedans (rectangle aux coins arrondis de 6 cm)
            bord += (rnoise(p * 7.0 + h * 40.0) - 0.5) * 0.04 + (rnoise(p * 23.0) - 0.5) * 0.015;
            float dedans = smoothstep(-pix, pix, bord);
            bool recente = fract(h.x * 9.7) < 0.5;
            alb = mix(alb, recente ? 0.9 : 1.04, dedans); dr = mix(dr, recente ? -0.04 : 0.03, dedans); rel = mix(rel, recente ? 0.75 : 1.0, dedans);
            float joint = (1.0 - smoothstep(0.012 - pix, 0.012 + pix, abs(bord))) * clamp(0.012 / pix, 0.0, 1.0);
            alb = mix(alb, 0.62, joint); dr = mix(dr, -0.2, joint);
        }
    }
    // gouttes d'huile au milieu de chaque voie (entre les deux bandes de roulement), taches de 10 à 40 cm
    {
        float k = clamp(floor((p.x + halfW) / voie), 0.0, 2.0);
        float c = -halfW + (k + 0.5) * voie;
        float milieu = 1.0 - smoothstep(0.2, 0.45, abs(p.x - c));
        float t = smoothstep(0.62, 0.8, rnoise(p * vec2(3.1, 1.3) + k * 3.0)) * milieu * clamp(0.1 / pix, 0.0, 1.0);
        alb *= 1.0 - 0.1 * t; dr -= 0.05 * t;
    }
    return vec3(alb, dr, rel);
}

// rives propres à la carte (d = distance au bord de la chaussée, m) : couleur, rugosité et part de la rive (0 : enrobé)
//   Campagne : bords effrités, la terre de l'accotement apparaît (albédo ≈ 0,15) ; Corniche : sable apporté par le vent
//   (albédo ≈ 0,35) ; Ville : caniveau plus sale au pied de la bordure ; Grand Prix : rien (vibreurs du circuit, circuit.cpp)
vec4 riveRoute(vec2 p, float halfW, int carte, out float part) {
    float d = halfW - abs(p.x);
    part = 0.0;
    if (carte == 1) {
        float e = fbm(p * vec2(1.7, 0.9) + 3.1);
        part = 1.0 - smoothstep(0.0, 0.06, d - (0.08 + 0.55 * e * e));
        return vec4(vec3(0.19, 0.155, 0.115) * (0.8 + 0.4 * fbm(p * 9.0)), 0.95);
    }
    if (carte == 2) {
        float e = smoothstep(0.35, 0.7, rnoise(p * vec2(0.9, 0.22) + 1.7));
        part = (1.0 - smoothstep(0.1, 1.1, d)) * e * 0.85;
        return vec4(vec3(0.43, 0.38, 0.29) * (0.9 + 0.2 * noise(p * 14.0)), 0.92);
    }
    if (carte == 0) {
        part = (1.0 - smoothstep(0.0, 0.45, d)) * 0.5;
        return vec4(vec3(0.08, 0.078, 0.075), 0.9);
    }
    return vec4(0.0);
}

// chaussée (F_ROAD) : uv = mètres (x latéral, y le long de la route)
void surfaceRoute(inout vec4 base, inout float rough, inout float metal, inout vec3 N, inout bool wetRoad, inout float flaque) {
    vec2 p = vUv;
    uint fl = uint(pc.params.w + 0.5);
    bool scan = (fl & F_HASMR) != 0u;   // WFS-205 : cartes du revêtement présentes (sinon repli procédural, RG-04)
    vec3 asphalt; float roughSec; float lonN = 1.0;
    if (scan) {
        Revet rv = lireRevetement(p, modeLecture(), false);
        vec2 mac = macroRevetement(vWorld.xz);
        asphalt = rv.albedo * mac.x;                                              // albédo recalibré à la conversion (RG-02)
        roughSec = clamp(rv.rough + mac.y, 0.0, 1.0);
        N = perturb(normalize(vNrm), vWorld, p, normalize(rv.n));
        lonN = rv.lon;
    } else {
        // WFS-205 (RG-02, RG-04) : repli procédural recalibré sur l'enrobé usé (0,10-0,18 ; avant : 0,045-0,115, moyenne ≈ 0,07)
        float grain = fbm(p * 3.1) * 0.6 + fbm(p * 23.0) * 0.4;
        asphalt = mix(vec3(0.085, 0.088, 0.095), vec3(0.19, 0.19, 0.198), grain);
        asphalt *= 0.85 + 0.3 * fbm(p * vec2(0.08, 0.05));                     // zones usées / rapiécées
        roughSec = 0.9 - grain * 0.15;
    }
    float halfW = pc.emissive.w;                                                 // demi-largeur de chaussée (m)
    {   // WFS-206 : usure, raccords et rives (chaussée de la course : demi-largeur RW ; rues transversales et places : hors)
        int carte = carteRoute();
        if (halfW > 1.0 && halfW < 50.0) {
            vec3 u = usureRoute(p, halfW, carte);
            vec3 df = defautsChaussee(p, halfW, carte, max(length(fwidth(p)), 1e-4));   // WFS-594 (RG-02)
            u.x *= df.x; u.y += df.y; u.z *= df.z;
            asphalt *= u.x; roughSec = clamp(roughSec + u.y, 0.0, 1.0);
            if (scan) { N = normalize(mix(normalize(vNrm), N, u.z)); lonN *= u.z; }
            float part; vec4 rive = riveRoute(p, halfW, carte, part);
            asphalt = mix(asphalt, rive.rgb, part); roughSec = mix(roughSec, rive.a, part);
            if (scan) N = normalize(mix(N, normalize(vNrm), part * 0.6));
        }
    }
    // WFS-594 (RG-03) : lignes de 15 cm (IISR, 7e partie : u = 7,5 cm, ligne de rive et de voie 2u en ville), bords anticrénelés au
    // pixel et légèrement irréguliers (± 6 mm : peinture appliquée à la machine, bords mangés) ; au loin, la ligne s'efface au prorata
    float pixM = max(length(fwidth(p)), 1e-4);
    float netM = 1.0 - smoothstep(0.004, 0.02, pixM);   // détails de la peinture (< 1 cm) seulement au plus près
    float bordM = (rnoise(p * vec2(23.0, 2.1)) - 0.5) * 0.012 * netM;
    float demiM = LARGEUR_MARQUAGE_M * 0.5;
    float edgeL = 1.0 - smoothstep(demiM - pixM, demiM + pixM, abs(abs(p.x) - (halfW - 0.45)) + bordM);  // lignes de rive continues
    float lane = 0.0;
    float usureTiret = 0.0;
    for (int k = -1; k <= 1; k += 2) {                                           // 3 voies : 2 lignes pointillées
        float lx = float(k) * halfW / 3.0;
        float dash = step(fract(p.y / 12.0), 0.45);
        float l = (1.0 - smoothstep(demiM - pixM, demiM + pixM, abs(p.x - lx) + bordM)) * dash;
        if (l > lane) { lane = l; usureTiret = hash(vec2(floor(p.y / 12.0), float(k) + 3.0)); }   // chaque tiret son âge
    }
    // WFS-223 (RG-02) : pas de tirets de voie dans un virage serré (anneau de giratoire) : uv.y croît de SEG_M par segment sur
    // toute la largeur, donc d(uv.y)/d(longueur réelle) = R / (R − x) : on le mesure par les dérivées écran (gradient de p.y
    // dans le plan de la chaussée) ; courbure κ = (1 − 1 / rapport) / x : rayon < 18 m (anneau de 14 m) : tirets retirés,
    // lignes de rive gardées ; épingles (22 m) et virages ordinaires inchangés
    vec3 wx = dFdx(vWorld), wy = dFdy(vWorld); vec2 sy = vec2(dFdx(p.y), dFdy(p.y));
    float aa = dot(wx, wx), ab = dot(wx, wy), bb = dot(wy, wy), det = aa * bb - ab * ab;
    float g2 = det > 1e-14 ? (bb * sy.x * sy.x - 2.0 * ab * sy.x * sy.y + aa * sy.y * sy.y) / det : 1.0;
    float kappa = abs(1.0 - inversesqrt(max(g2, 1e-6))) / max(abs(p.x), 0.5);
    lane *= 1.0 - smoothstep(0.047, 0.055, kappa);
    // WFS-594 (RG-03) : peinture usée : les roues l'arrachent par plaques (plus sur les tirets que l'on franchit en changeant de voie et
    // dans les bandes de roulement), le grain de l'enrobé perce ; au loin, la couverture moyenne (pas de scintillement)
    float usureP = 0.12 + 0.28 * usureTiret * step(edgeL, lane);
    {
        float voie = 2.0 * halfW / 3.0, kv = clamp(floor((p.x + halfW) / voie), 0.0, 2.0), cv = -halfW + (kv + 0.5) * voie;
        float dxr = abs(abs(p.x - cv) - ECARTEMENT_ROUES_M * 0.5);
        usureP += 0.35 * (1.0 - smoothstep(BANDE_ROULEMENT_M * 0.5 - 0.1, BANDE_ROULEMENT_M * 0.5 + 0.1, dxr));
    }
    float netF = 1.0 - smoothstep(0.002, 0.006, pixM);   // détails de ≈ 1 cm : effacés avant d'être plus fins que 4 pixels
    float trous = rnoise(p * vec2(31.0, 11.0)) * 0.55 + mix(0.5, rnoise(p * vec2(97.0, 61.0)), netF) * 0.45;
    float couvre = mix(1.0 - usureP * 0.6, 1.0 - smoothstep(1.0 - usureP - 0.08, 1.0 - usureP + 0.08, trous), netM);
    float paint = max(edgeL, lane) * clamp(couvre, 0.0, 1.0);
    vec3 peinture = vec3(MARQUAGE_ALBEDO) * vec3(1.0, 1.0, 0.97) * (0.93 + 0.07 * mix(0.5, noise(p * 60.0), netF))
                  * mix(1.0, 0.9, smoothstep(0.4, 0.8, rnoise(p * vec2(0.7, 0.15))));   // poussière et gomme par endroits
    base.rgb = mix(asphalt, peinture, paint);
    g_retroMarquage = paint;   // WFS-594 (RG-03) : la nuit, billes de verre de la peinture (eclairage.glsl : retroChaussee)
    // WFS-142 (RG-02) : asphalte sec, jour et nuit : mat (rugosité 0,75 à 0,9), lignes peintes 0,55, jamais métallique
    rough = roughToksvig(mix(roughSec, 0.55, paint), mix(lonN, 1.0, paint)); metal = 0.0;
    {   // WFS-346 : neige au sol (0 sur les cartes du code : rien ne change)
        float n = neigeRoute() * abriRoute(p.y);
        if (n > 0.0 && halfW > 1.0 && halfW < 50.0) { neigeSurRoute(p, halfW, n, base.rgb, rough, N); g_retroMarquage *= 1.0 - n; }   // WFS-594 : peinture sous la neige
    }
    if (F.nuitA.x > 0.5) {
        // WFS-142 (RG-03) : asphalte mouillé (pluie). Le roadShader du jeu web (rugosité 0,22, métal 0,4, flaques à 0,02 sur
        // ≈ 18 % de la chaussée) faisait un miroir : il n'est plus suivi. Ici : jamais métallique, rugosité 0,35 à 0,5, asphalte
        // assombri par l'eau (× 0,8), flaques rares aux bords doux (≈ 8 % de la chaussée, seuil 0,66 à 0,8 au lieu de 0,6 à 0,7), hors traces de roues et
        // hors peinture, rugosité ≥ 0,08
        wetRoad = true;
        g_retroMarquage *= 0.3;   // WFS-594 : film d'eau sur les billes : R_L mouillé ≈ 30 % du sec (EN 1436, classe RW)
        // WFS-205 : le film d'eau remplit la macrotexture : relief du scan retiré (grain de couleur gardé). Le reflet mouillé redevient
        // celui de WFS-142 ; sans cela, reflet des lampadaires non linéaire en N, différent dans la capture d'environnement (M-01 de
        // WFS-150, nuit, face −Y : − 2,1 % au run 806)
        if (scan) { N = normalize(vNrm); lonN = 1.0; }
        vec2 w = vWorld.xz;
        float rn = rnoise(w * 0.11) * 0.62 + rnoise(w * 0.43 + 7.0) * 0.38;      // deux échelles, pas de motif répété
        float lane = fract((p.x + halfW) / (2.0 * halfW) * 3.0);                   // position dans la voie (0..1)
        float trk = exp(-pow((lane - 0.3) * 13.0, 2.0)) + exp(-pow((lane - 0.7) * 13.0, 2.0));   // traces de roues
        float puddle = smoothstep(0.66, 0.8, rn) * (1.0 - min(trk, 1.0)) * (1.0 - paint) * F.nuitA.y;   // flaques (Haute)
        base.rgb *= (0.9 + 0.2 * rnoise(w * 0.031 + 3.0)) * (1.0 - trk * 0.07) * mix(0.8, 0.72, puddle);   // eau (flaque à peine plus sombre : pas de « trou » noir), réparations
        rough = clamp(mix(0.38, 0.5, paint) + (rn - 0.5) * 0.12 + trk * 0.04, 0.35, 0.5);   // traces de roues : un peu plus mates
        rough = mix(rough, 0.08, puddle);
        rough = roughToksvig(rough, mix(lonN, 1.0, puddle * 0.7));   // WFS-205 : flaque plus plane (comme N)
        metal = 0.0;
        flaque = puddle;
        N = normalize(mix(N, normalize(vNrm), puddle * 0.7));                       // flaque : surface plus plane, pas un miroir
    }
}
// WFS-206 (RG-02, M-04) : Ville : bordure de granit (15 cm de haut, bloc de 0,30 m, joints tous les 1,0 m) au bord du trottoir ;
// uv.x = latéral (m) : la bordure va de RW (face verticale) à RW + 0,30 ; granit gris moucheté (albédo ≈ 0,25-0,30, rugosité 0,7)
// WFS-594 (RG-04) : bordure T2 (NF EN 1340 : 15 × 25 cm) sur toutes les cartes : granit en Ville, béton gris ailleurs (albédo 0,30) ;
// face verticale lue en (long de la route, hauteur) : grain non étiré ; arête du chanfrein (2 cm) plus claire et usée ; joints au
// mortier tous les JOINT_BORDURE_M, un peu ouverts ; éclats sur l'arête par endroits
const float JOINT_BORDURE_M = 1.0;   // = route_revetements.h (test_fidelite_routes)
void bordureGranit(vec2 p, inout vec4 base, inout float rough, inout vec3 N) {
    if (carteRoute() == 3 || abs(p.x) > DEMI_CHAUSSEE_M + BORDURE_LARGEUR_M) return;
    bool granit = carteRoute() == 0;
    vec3 Ng = normalize(vNrm);
    bool face = abs(Ng.y) < 0.5;                                        // face verticale côté chaussée
    vec2 q = face ? vec2(p.y, vWorld.y) : p;                     // grain : sur la face, le long de la route et en hauteur
    float net = clamp(1.0 - length(fwidth(q)) * 60.0, 0.0, 1.0);       // grain effacé quand il devient plus petit que le pixel
    float grain = noise(q * vec2(70.0, 45.0)), tache = noise(p * vec2(9.0, 3.0));
    vec3 g;
    if (granit) {
        g = vec3(0.27, 0.27, 0.28) * (1.0 + (grain - 0.5) * 0.3 * net) * (0.92 + 0.16 * tache);
        g = mix(g, vec3(0.08), step(0.975, hash(floor(q * 90.0))) * net * 0.8);   // grains de mica noir (au plus près)
    } else {
        g = vec3(0.31, 0.305, 0.29) * (1.0 + (grain - 0.5) * 0.16 * net) * (0.9 + 0.18 * tache);   // béton vibré : grain plus fin
        g = mix(g, vec3(0.2, 0.195, 0.185), step(0.985, hash(floor(q * 140.0))) * net * 0.6);    // bulles d'air (bullage)
    }
    float bloc = floor(p.y / JOINT_BORDURE_M + 0.5);
    g *= 0.94 + 0.12 * hash(vec2(bloc, granit ? 1.0 : 2.0));         // chaque bloc son ton
    float pixJ = max(fwidth(p.y), 1e-4);
    float dj = abs(fract(p.y / JOINT_BORDURE_M + 0.5) - 0.5) * JOINT_BORDURE_M;   // m au joint le plus proche
    float joint = (1.0 - smoothstep(0.004, 0.006 + pixJ, dj)) * clamp(0.01 / pixJ, 0.0, 1.0);
    float arete = 0.0;
    if (!face) {   // chanfrein (RW à RW + 0,12 : pente de 2 cm) : arête polie par les pneus, éclats par endroits
        float ax = abs(p.x) - DEMI_CHAUSSEE_M;
        arete = (1.0 - smoothstep(0.0, 0.05, ax)) * (0.6 + 0.4 * noise(vec2(p.y * 3.0, 1.0)));
        float eclat = smoothstep(0.8, 0.9, noise(vec2(p.y * 2.3, 7.0))) * (1.0 - smoothstep(0.0, 0.03, ax));
        g = mix(g * (1.0 + 0.12 * arete), g * 0.7, eclat);
    }
    base.rgb = mix(g, vec3(0.1), joint * 0.7);
    rough = mix(granit ? 0.7 : 0.8, 0.55, arete * 0.6);
    N = Ng;
}
// WFS-346 : trottoir enneigé : neige piétinée, plus claire que la chaussée (peu de circulation), tassée par endroits
void neigeTrottoir(vec2 p, inout vec4 base, inout float rough, inout vec3 N) {
    float n = neigeRoute() * abriRoute(p.y);
    if (n <= 0.0) return;
    float g = fbm(p * vec2(0.9, 0.6));
    vec3 neige = mix(vec3(0.62, 0.64, 0.68), vec3(0.78, 0.80, 0.84), smoothstep(0.35, 0.7, g));
    base.rgb = mix(base.rgb, neige, n);
    rough = mix(rough, 0.8, n);
    N = normalize(mix(N, normalize(vNrm), n * 0.85));
}
// trottoirs (F_WALK)
void surfaceTrottoir(inout vec4 base, inout float rough, inout float metal, inout vec3 N) {
    vec2 p = vUv;
    if ((uint(pc.params.w + 0.5) & F_HASMR) != 0u) {   // WFS-205 : béton de trottoir (scan à dalles : lu sans rotation)
        Revet rv = lireRevetement(p, modeLecture(), true);
        vec2 mac = macroRevetement(vWorld.xz);
        base.rgb = rv.albedo * mac.x * (0.94 + 0.12 * hash(floor(p / COTE_SCAN)));   // teinte par carreau : pas de motif répété
        {   // WFS-594 (RG-04) : taches du trottoir : coulures et taches sombres (≈ 0,5 à 2 m), chewing-gums écrasés en Ville (1 à 2 cm)
            float pixT = max(length(fwidth(p)), 1e-4);
            float tache = smoothstep(0.6, 0.85, rnoise(p * 0.9 + 17.0) * 0.7 + rnoise(p * 3.1) * 0.3);
            base.rgb *= 1.0 - 0.18 * tache;
            if (carteRoute() == 0 && pixT < 0.02) {
                vec2 cg = floor(p / 0.35); vec2 hg = hash22(cg + 5.0);
                float gomme = (1.0 - smoothstep(0.006, 0.009 + pixT, length(p - (cg + 0.2 + 0.6 * hg) * 0.35))) * step(0.86, hg.x);
                base.rgb = mix(base.rgb, base.rgb * 0.55, gomme * (1.0 - smoothstep(0.01, 0.02, pixT)));
            }
        }
        rough = roughToksvig(clamp(rv.rough + mac.y, 0.0, 1.0), rv.lon); metal = 0.0;
        N = perturb(normalize(vNrm), vWorld, p, normalize(rv.n));
        bordureGranit(p, base, rough, N);
        neigeTrottoir(p, base, rough, N);
        return;
    }
    vec2 tile = fract(p / 0.6);
    float joint = 1.0 - smoothstep(0.0, 0.04, min(min(tile.x, 1.0 - tile.x), min(tile.y, 1.0 - tile.y)));
    float stone = 0.32 + 0.1 * hash(floor(p / 0.6)) + 0.06 * fbm(p * 8.0);
    base.rgb = vec3(stone) * vec3(1.0, 0.98, 0.95) * (1.0 - joint * 0.45);
    rough = 0.85; metal = 0.0;
    bordureGranit(p, base, rough, N);
    neigeTrottoir(p, base, rough, N);
}
