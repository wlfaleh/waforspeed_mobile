// WaForSpeed — WFS-383 (E12 / E15, ANO-224) : sols des cartes. Inclus par scene.frag après route.glsl (bruits rnoise, rh12).
// Mélange de 3 matériaux selon la pente du terrain (normale lissée du sommet, vNrm) :
//   pente < 25° : pré ou herbe sèche, avec taches d'herbe d'une autre teinte et de terre (et feuilles mortes en automne) ;
//   25 à 40°    : terre et maquis (talus) ;
//   > 40°       : roche (calcaire gris-beige de la Corniche, roche grise ailleurs) en plaques aux bords nets (cellules de Voronoï de
//                 1,5 à 2,5 m) : fissures fines aux joints, joints assombris (relief), chaque plaque d'un ton un peu différent.
// Les limites varient de ± 4° avec une tache de 25 m (jamais une ligne droite) ; fondu de 8° à chaque passage.
// Matériau MAT_GROUND ; cutoff (pc.params.z) = code du sol (src/world/sols.h) : climat (1 méditerranéen, 2 tempéré, 3 automne)
// + 4 × « teinte » (le pré prend la couleur du sommet : champs cultivés de la Campagne) + 8 × « rocheux » (pente lue + jusqu'à 30°
// selon la part de roche du sommet, uv.x de 0 à 1, fondue par le bruit : aucune limite droite d'une bande à l'autre).
// Couleurs : albédo en sRGB linéaire (le shader travaille en linéaire), mesures et sources dans src/world/sols.h.
// Coût (RG-05) : 2 bruits de 4 octaves (2 en lecture légère) + 3 à 6 lectures de bruit simples, sans texture ; les cellules de la
// roche (9 cases) ne sont calculées que là où la pente lue dépasse 30°.

float bruitSol(vec2 p, int oct) {   // bruit fractal normé (0 à 1)
    float s = 0.0, a = 0.5, t = 0.0;
    for (int i = 0; i < 4; i++) { if (i >= oct) break; s += a * rnoise(p); t += a; p = p * 2.03 + vec2(17.3, 9.1); a *= 0.5; }
    return s / t;
}

// cellules de Voronoï : x = distance au joint le plus proche (F2 − F1, en unités de cellule), y = hasard de la cellule (0 à 1)
vec2 cellulesSol(vec2 p) {
    const vec2 i = floor(p), f = fract(p);
    float d1 = 8.0, d2 = 8.0, h = 0.0;
    for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) {
        const vec2 g = vec2(x, y), r = g + vec2(rh12(i + g), rh12(i + g + vec2(37.1, 11.7))) - f;
        const float d = dot(r, r);
        if (d < d1) { d2 = d1; d1 = d; h = rh12(i + g + vec2(5.3, 71.9)); } else if (d < d2) d2 = d;
    }
    return vec2(sqrt(d2) - sqrt(d1), h);
}

// ---------- WFS-594 (RG-05) : sols vus de près : relief et grain fins, sable, neige, bande de terre et de gravier au bord ----------
// Textures faites par le code (bruit multi-échelle, cellules), au niveau de détail d'une photo de près (brin, gravillon, ride de sable,
// croûte de neige) : aucune texture libre n'est atteignable depuis le réseau du projet (Poly Haven et ambientCG bloqués,
// regles-reseau.md). Relief : hauteur h (m) → normale par les dérivées écran (Mikkelsen 2010, « Bump mapping unparametrized
// surfaces on the GPU ») : aucune lecture de plus ; h ne contient que des détails plus grands que le pixel (fondu « detail »), donc
// aucun scintillement. Bits du code (src/world/sols.h) : 16 sable (couleur de la palette gardée), 32 neige (couche de neige fraîche),
// 64 bord de route (uv.x = latéral en m depuis l'axe : bande de terre et de gravier au pied du trottoir).
const float BORD_TROTTOIR_M = 11.5;       // RW + largeur du trottoir de Game (WALK_W = 4 m) : le sol commence là
const float BANDE_GRAVIER_MIN_M = 0.3, BANDE_GRAVIER_MAX_M = 0.8;   // RG-05 : bande de terre et de gravier au bord (30 à 80 cm)
vec3 bosseSol(vec3 N, float h) {   // relief h (m) → normale (dérivées écran)
    vec3 dpx = dFdx(vWorld), dpy = dFdy(vWorld);
    float hx = dFdx(h), hy = dFdy(h);
    vec3 r1 = cross(dpy, N), r2 = cross(N, dpx);
    float det = dot(dpx, r1);
    vec3 g = sign(det) * (hx * r1 + hy * r2);
    return normalize(abs(det) * N - g);
}

void surfaceTerrain(inout vec4 base, inout float rough, inout float metal, vec3 Ng, inout vec3 N) {
    const int code = int(pc.params.z + 0.5), climat = code % 4;
    const bool teinte = (code & 4) != 0, rocheux = (code & 8) != 0;
    const bool sable = (code & 16) != 0, neigeSol = (code & 32) != 0, bordRoute = (code & 64) != 0;
    const vec3 couleurBande = base.rgb;   // WFS-594 : couleur de la palette (sable sec ou mouillé), avant le mélange du pré
    const vec2 w = vWorld.xz;
    // finesse : les détails de moins de 0,5 m s'effacent quand un pixel couvre plus de 15 cm (pas de scintillement au loin)
    const float px = length(fwidth(w));
    const float detail = 1.0 - smoothstep(0.15, 0.6, px);
    const int oct = LEGER ? 2 : 4;
    const float macro = bruitSol(w * 0.04, oct);           // taches de ≈ 25 m
    const float meso = bruitSol(w * 0.29 + 7.0, oct);      // taches de ≈ 3,5 m
    const float fin = mix(0.5, rnoise(w * 2.6), detail);   // grain de ≈ 40 cm
    // pente en degrés (normale lissée : plus de facettes), limites irrégulières
    // zone de rochers : part de roche du sommet (uv.x), bord rendu irrégulier par les taches de 25 m et de 3,5 m
    const float bonus = rocheux ? 30.0 * smoothstep(0.25, 0.85, vUv.x + (macro - 0.5) * 0.4 + (meso - 0.5) * 0.25) : 0.0;
    const float deg = degrees(acos(clamp(normalize(Ng).y, 0.0, 1.0))) + bonus + (macro - 0.5) * 8.0;
    const float tTerre = smoothstep(21.0, 29.0, deg);

    vec3 pre, tache, terre, maquis, roche, roche2;
    if (climat == 1) {          // méditerranéen (Corniche) : garrigue basse, taches d'herbe sèche, terre rouge, calcaire blanc-gris
        pre = vec3(0.159, 0.178, 0.059); tache = vec3(0.323, 0.287, 0.122); terre = vec3(0.262, 0.153, 0.076);
        maquis = vec3(0.068, 0.087, 0.030); roche = vec3(0.479, 0.413, 0.296); roche2 = vec3(0.319, 0.270, 0.191);
    } else if (climat == 3) {   // automne : pré fané, terre sombre, feuilles mortes (plus bas), roche grise
        pre = vec3(0.209, 0.202, 0.065); tache = vec3(0.147, 0.212, 0.042); terre = vec3(0.112, 0.068, 0.037);
        maquis = vec3(0.112, 0.061, 0.030); roche = vec3(0.270, 0.254, 0.216); roche2 = vec3(0.178, 0.165, 0.144);
    } else {                    // tempéré (Ville, Campagne, Grand Prix) : pré 0x6B7F3A, herbe sèche, terre brune, roche grise
        pre = vec3(0.147, 0.212, 0.042); tache = vec3(0.323, 0.275, 0.102); terre = vec3(0.156, 0.102, 0.051);
        maquis = vec3(0.090, 0.110, 0.035); roche = vec3(0.270, 0.254, 0.216); roche2 = vec3(0.178, 0.165, 0.144);
    }
    // champ cultivé : sa couleur (blé, labour…) garde la parcelle lisible, ramenée à mi-chemin du pré mesuré (plus d'aplat saturé)
    if (teinte) pre = mix(base.rgb, pre, 0.45);

    // pente faible : pré, grandes taches d'une autre herbe (≈ 25 m, contraste 0,4, fondu large : jamais un motif de camouflage),
    // taches rondes de terre nue (≈ 3,5 m, fondu doux, couleur ramenée vers le pré), grain de 40 cm
    vec3 bas = mix(pre, tache, smoothstep(0.3, 0.8, macro) * (teinte || bordRoute ? 0.25 : 0.4));   // WFS-594 : bord de route arrosé (ville) : moins d'herbe sèche
    bas = mix(bas, mix(terre, pre, 0.35), smoothstep(0.62, 0.9, meso) * 0.35);
    if (climat == 1) {   // garrigue : pointillé de touffes de maquis sombre (romarin, chêne kermès) de 30 à 80 cm, effacé au loin
        const float touffe = smoothstep(0.62, 0.78, rnoise(w * 1.9 + 23.0) * 0.6 + rnoise(w * 4.1 + 5.0) * 0.4);
        bas = mix(bas, maquis, touffe * 0.7 * detail + 0.12 * (1.0 - detail));
    }
    if (climat == 3) {   // feuilles mortes variées (brun, roux, jaune, brun foncé) en taches irrégulières de 10 à 30 cm ; au loin : leur moyenne
        const float h = rnoise(w * 3.3 + 41.0), couvre = smoothstep(0.42, 0.6, rnoise(w * 5.7 + 13.0) * 0.65 + meso * 0.35);
        const vec3 f = mix(mix(vec3(0.112, 0.061, 0.030), vec3(0.195, 0.076, 0.023), smoothstep(0.25, 0.4, h)),
                           mix(vec3(0.332, 0.144, 0.030), vec3(0.392, 0.254, 0.042), smoothstep(0.6, 0.75, h)), smoothstep(0.42, 0.58, h));
        const vec3 moyenne = vec3(0.258, 0.134, 0.031);
        bas = mix(bas, mix(moyenne, f, detail), mix(0.35, couvre * 0.85, detail));
    }
    bas *= 0.82 + 0.36 * fin;
    // talus : terre claire et pierreuse (cailloux de calcaire), touffes de maquis sombre de 0,5 à 3 m aux bords doux ; au loin, leur moyenne
    const vec3 terreP = mix(terre, roche, 0.35 + 0.2 * smoothstep(0.55, 0.8, rnoise(w * 5.3 + 2.0)) * detail);
    const float touffeT = smoothstep(0.5, 0.64, rnoise(w * 0.55 + 11.0) * 0.55 + rnoise(w * 1.6 + 3.0) * 0.3 + mix(0.5, rnoise(w * 4.3), detail) * 0.15);
    const vec3 talus = mix(terreP * (0.85 + 0.3 * fin), maquis * (0.8 + 0.4 * fin), touffeT * 0.85);
    // roche : affleure par plaques entières (bords nets), fissures fines aux joints (effacées au loin), joints assombris (relief),
    // ton propre à chaque plaque ; cellules calculées seulement au-delà de 30°
    float tRoche = 0.0;
    vec3 roc = roche;
    if (deg > 30.0) {
        // dalles de ≈ 3 × 5 m, allongées comme des bancs ; le réseau de diaclases n'est pas complet (fissures seulement là où le
        // calcaire est fracturé), la surface de la dalle est grêlée (lapiaz) et tachée de lichen gris : pas un dallage
        const vec2 c = cellulesSol(w * vec2(0.2, 0.32) + 19.0);
        tRoche = smoothstep(39.0, 41.0, deg + (c.y - 0.5) * 12.0);
        const float fracture = smoothstep(0.5, 0.75, rnoise(w * 0.3 + 61.0) * 0.7 + rnoise(w * 0.9 + 5.0) * 0.3);
        const float fis = (1.0 - smoothstep(0.008, 0.03, c.x)) * detail * fracture, joint = mix(1.0, smoothstep(0.0, 0.2, c.x), fracture);
        const float lapiaz = mix(0.5, rnoise(w * 3.1 + 7.0) * 0.6 + rnoise(w * 9.0) * 0.4, detail);
        roc = mix(roche, roche2, smoothstep(0.2, 0.95, c.y * 0.45 + meso * 0.35 + macro * 0.2)) * (0.8 + 0.35 * lapiaz) * (0.75 + 0.25 * joint) * (1.0 - 0.6 * fis);
    }

    // WFS-594 (RG-05) : grain fin de près (fondu entre 2 et 8 cm de pixel) et relief de chaque matériau
    const float fin2 = 1.0 - smoothstep(0.02, 0.08, px);   // détails de 1 à 5 cm
    const float mi = 1.0 - smoothstep(0.04, 0.2, px);      // touffes et mottes de 10 à 30 cm (visibles jusqu'à ≈ 30 m)
    const float touffeM = LEGER ? 0.5 : rnoise(w * 6.7 + 31.0) * 0.55 + rnoise(w * 14.3 + 5.0) * 0.45;
    bas *= mix(1.0, 0.8 + 0.4 * touffeM, mi);                // herbe : touffes plus sombres et plus claires (pas un aplat)
    float h = 0.0;
    if (!LEGER && fin2 > 0.0) {
        // herbe : brins et touffes (2 à 4 cm), pointes plus claires, creux plus sombres (ombre entre les brins) ; tige plus fine au loin
        const float brin = rnoise(w * vec2(41.0, 37.0) + 3.0) * 0.55 + rnoise(w * vec2(97.0, 89.0)) * 0.45;
        const float touffeH = rnoise(w * 7.3 + 9.0);
        const float hHerbe = (brin * 0.7 + touffeH * 0.3) * 0.03;
        bas *= mix(1.0, 0.72 + 0.56 * (brin * 0.7 + touffeH * 0.3), fin2);
        // terre et talus : gravillons et cailloux (4 à 8 cm) plus clairs, en relief
        const vec2 cg = cellulesSol(w * 14.0 + 3.7);
        const float caillou = smoothstep(0.08, 0.25, cg.x) * step(0.55, cg.y);
        const vec3 talusFin = mix(talus, mix(roche, vec3(0.42, 0.40, 0.36), cg.y), caillou * 0.7 * fin2);
        const float hTerre = caillou * 0.012 + rnoise(w * 23.0) * 0.006;
        const float hRoche = (rnoise(w * 11.0 + 1.0) * 0.6 + rnoise(w * 31.0) * 0.4) * 0.025;
        base.rgb = mix(mix(bas, talusFin, tTerre), roc, tRoche);
        h = mix(mix(hHerbe, hTerre, tTerre), hRoche, tRoche) * fin2;
    }
    if (!LEGER) {
        h += touffeM * 0.03 * mi * (1.0 - tRoche);
        if (fin2 <= 0.0) base.rgb = mix(mix(bas, talus, tTerre), roc, tRoche);
    } else base.rgb = mix(mix(bas, talus, tTerre), roc, tRoche);
    rough = mix(0.95, 0.85, tRoche); metal = 0.0;

    // bord de route (sol de Game et de la Ville) : bande de terre battue et de gravier au pied du trottoir, de 30 à 80 cm (bord
    // irrégulier), l'herbe déborde par touffes ; au loin, sa couleur moyenne
    if (bordRoute) {
        const float d = abs(vUv.x) - BORD_TROTTOIR_M;
        const float larg = mix(BANDE_GRAVIER_MIN_M, BANDE_GRAVIER_MAX_M, rnoise(vec2(vUv.y * 0.15, sign(vUv.x) * 5.0)));
        const float bord = larg + (rnoise(w * 2.3) - 0.5) * 0.25 * detail;
        const float pixB = max(fwidth(d), 1e-4);
        const float bande = 1.0 - smoothstep(bord - pixB - 0.05, bord + pixB + 0.05, d);
        if (bande > 0.0) {
            const vec2 cg = cellulesSol(w * 30.0 + 11.0);                // gravillons de 1,5 à 4 cm (concassé 4/20)
            const float gr = smoothstep(0.05, 0.2, cg.x);
            const vec3 terreB = mix(terre, roche, 0.25) * (0.85 + 0.3 * rnoise(w * 9.0));
            const vec3 gravier = mix(vec3(0.24, 0.23, 0.21), vec3(0.36, 0.34, 0.30), cg.y) * mix(0.6, 1.0, gr);
            const vec3 bandeC = mix(mix(terreB, gravier, 0.55), mix(terreB, gravier, gr * 0.8 + 0.1), fin2);
            const float herbe = smoothstep(0.55, 0.75, rnoise(w * 3.1 + 2.0) * 0.6 + (d / max(bord, 0.1)) * 0.5);   // touffes qui débordent
            base.rgb = mix(base.rgb, bandeC, bande * (1.0 - herbe));
            rough = mix(rough, 0.9, bande);
            h = mix(h, gr * 0.008 * fin2, bande * (1.0 - herbe));
        }
    }

    // sable (Corniche) : couleur mesurée de la palette (WFS-371), rides de vent (8 à 15 cm, direction qui tourne lentement),
    // grain, coquillages et débris rares ; sable mouillé (palette plus sombre) plus lisse et plus brillant
    if (sable) {
        const vec3 c0 = couleurBande;   // couleur de la bande (sable sec ou mouillé)
        const float lum = dot(c0, vec3(0.2126, 0.7152, 0.0722));
        const float mouille = 1.0 - smoothstep(0.32, 0.45, lum);
        const float ang = (macro - 0.5) * 1.2 + (meso - 0.5) * 0.4;
        const vec2 dir = vec2(cos(ang), sin(ang));
        const float ph = dot(w, dir) / 0.11 + rnoise(w * 0.9) * 2.5 + rnoise(w * 3.7) * 0.6;
        const float ride = pow(abs(fract(ph) - 0.3) / 0.7, 1.4);           // profil dissymétrique (face au vent douce)
        const float pietine = smoothstep(0.55, 0.8, meso);                   // zones piétinées : rides effacées
        const float r = ride * (1.0 - pietine * 0.85) * (1.0 - mouille);
        const float g = rnoise(w * 57.0) * 0.6 + rnoise(w * 131.0) * 0.4;
        vec3 c = c0 * (0.92 + 0.12 * mix(0.5, g, fin2)) * (0.96 + 0.08 * mix(0.5, r, detail));
        const vec2 cd = cellulesSol(w * 9.0 + 41.0);                        // coquillages et débris (1 à 3 cm), rares
        c = mix(c, vec3(0.62, 0.58, 0.50), smoothstep(0.25, 0.4, cd.x) * step(0.93, cd.y) * fin2 * 0.6);
        c = mix(c, vec3(0.12, 0.10, 0.07), smoothstep(0.25, 0.4, cd.x) * step(0.985, fract(cd.y * 13.0)) * fin2 * 0.5);
        base.rgb = c;
        rough = mix(0.92, 0.55, mouille);
        h = (r * 0.012 * detail + g * 0.002 * fin2);
    }

    // neige fraîche au sol (cartes d'hiver) : albédo 0,80 à 0,88 (DA-realisme.md : neige fraîche ≈ 0,85), ombre bleutée des creux,
    // congères et croûte de vent (sastrugi, 1 à 4 m), grain de surface ; quelques herbes qui percent en bord de route
    if (neigeSol) {
        const float cong = bruitSol(w * 0.35 + 3.0, oct);
        const float vent = rnoise(w * vec2(2.1, 0.6) + 17.0) * 0.6 + rnoise(w * vec2(5.3, 1.7)) * 0.4;
        const float grain = rnoise(w * 43.0) * 0.6 + rnoise(w * 103.0) * 0.4;
        const float crete = pow(vent, 2.0);                                    // crêtes de vent plus nettes que les creux
        const float hN = cong * 0.3 * detail + crete * 0.06 * mi + grain * 0.004 * fin2;
        // creux plus bleutés (lumière du ciel seulement, neige plus tassée), bosses plus blanches ; albédo moyen ≈ 0,8 (neige fraîche)
        vec3 c = mix(vec3(0.70, 0.74, 0.82), vec3(0.86, 0.88, 0.91), smoothstep(0.25, 0.75, cong * 0.6 + crete * 0.4))
                 * (0.96 + 0.06 * mix(0.5, grain, fin2));
        base.rgb = mix(base.rgb, c, 0.97);
        rough = 0.7;
        h = hN;
    }

    if (!LEGER) N = bosseSol(N, h);
}
