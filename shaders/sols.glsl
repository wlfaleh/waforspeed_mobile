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

void surfaceTerrain(inout vec4 base, inout float rough, inout float metal, vec3 Ng) {
    const int code = int(pc.params.z + 0.5), climat = code % 4;
    const bool teinte = (code & 4) != 0, rocheux = (code & 8) != 0;
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
    vec3 bas = mix(pre, tache, smoothstep(0.3, 0.8, macro) * (teinte ? 0.25 : 0.4));
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

    base.rgb = mix(mix(bas, talus, tTerre), roc, tRoche);
    rough = mix(0.95, 0.85, tRoche); metal = 0.0;
}
