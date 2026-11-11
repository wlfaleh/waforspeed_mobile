// WaForSpeed — WFS-383 (E12 / E15, ANO-224) : sols des cartes. Inclus par scene.frag après route.glsl (bruits rnoise, rh12).
// Mélange de 3 matériaux selon la pente du terrain (normale lissée du sommet, vNrm) :
//   pente < 25° : pré ou herbe sèche, avec taches d'herbe d'une autre teinte et de terre (et feuilles mortes en automne) ;
//   25 à 40°    : terre et maquis (talus) ;
//   > 40°       : roche (calcaire de la Corniche, roche grise ailleurs), veines et fissures.
// Les limites varient de ± 4° avec une tache de 25 m (jamais une ligne droite) ; fondu de 8° à chaque passage.
// Matériau MAT_GROUND ; cutoff (pc.params.z) = code du sol (src/world/sols.h) : climat (1 méditerranéen, 2 tempéré, 3 automne)
// + 4 × « teinte » (le pré prend la couleur du sommet : champs cultivés de la Campagne) + 8 × « rocheux » (pente lue + 20°).
// Couleurs : albédo en sRGB linéaire (le shader travaille en linéaire), mesures et sources dans src/world/sols.h.
// Coût (RG-05) : 2 bruits de 4 octaves (2 en lecture légère) + 3 lectures de bruit simples, sans texture.

float bruitSol(vec2 p, int oct) {   // bruit fractal normé (0 à 1)
    float s = 0.0, a = 0.5, t = 0.0;
    for (int i = 0; i < 4; i++) { if (i >= oct) break; s += a * rnoise(p); t += a; p = p * 2.03 + vec2(17.3, 9.1); a *= 0.5; }
    return s / t;
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
    const float deg = degrees(acos(clamp(normalize(Ng).y, 0.0, 1.0))) + (rocheux ? 20.0 : 0.0) + (macro - 0.5) * 8.0;
    const float tTerre = smoothstep(21.0, 29.0, deg), tRoche = smoothstep(36.0, 44.0, deg);

    vec3 pre, tache, terre, maquis, roche, roche2;
    if (climat == 1) {          // méditerranéen (Corniche) : garrigue basse, taches d'herbe sèche, terre rouge, calcaire blanc-gris
        pre = vec3(0.159, 0.178, 0.059); tache = vec3(0.323, 0.287, 0.122); terre = vec3(0.262, 0.153, 0.076);
        maquis = vec3(0.068, 0.087, 0.030); roche = vec3(0.539, 0.503, 0.423); roche2 = vec3(0.356, 0.328, 0.270);
    } else if (climat == 3) {   // automne : pré fané, terre sombre, feuilles mortes (plus bas), roche grise
        pre = vec3(0.209, 0.202, 0.065); tache = vec3(0.147, 0.212, 0.042); terre = vec3(0.112, 0.068, 0.037);
        maquis = vec3(0.112, 0.061, 0.030); roche = vec3(0.270, 0.254, 0.216); roche2 = vec3(0.178, 0.165, 0.144);
    } else {                    // tempéré (Ville, Campagne, Grand Prix) : pré 0x6B7F3A, herbe sèche, terre brune, roche grise
        pre = vec3(0.147, 0.212, 0.042); tache = vec3(0.323, 0.275, 0.102); terre = vec3(0.156, 0.102, 0.051);
        maquis = vec3(0.090, 0.110, 0.035); roche = vec3(0.270, 0.254, 0.216); roche2 = vec3(0.178, 0.165, 0.144);
    }
    // champ cultivé : sa couleur (blé, labour…) garde la parcelle lisible, ramenée à mi-chemin du pré mesuré (plus d'aplat saturé)
    if (teinte) pre = mix(base.rgb, pre, 0.45);

    // pente faible : pré, taches d'une autre herbe (≈ 25 m) et de terre nue (≈ 3,5 m), grain de 40 cm
    vec3 bas = mix(pre, tache, smoothstep(0.45, 0.75, macro) * (teinte ? 0.35 : 0.7));
    bas = mix(bas, terre, smoothstep(0.66, 0.86, meso) * 0.45);
    if (climat == 3) {   // feuilles mortes variées (brun, roux, jaune, brun foncé) par carrés de 25 cm ; au loin : leur moyenne
        const vec2 c = floor(w * 4.0);
        const float h = rh12(c), couvre = smoothstep(0.35, 0.75, meso * 0.6 + rh12(c + 31.7) * 0.4);
        const vec3 f = h < 0.25 ? vec3(0.195, 0.076, 0.023) : h < 0.5 ? vec3(0.332, 0.144, 0.030)
                     : h < 0.75 ? vec3(0.392, 0.254, 0.042) : vec3(0.112, 0.061, 0.030);
        const vec3 moyenne = vec3(0.258, 0.134, 0.031);
        bas = mix(bas, mix(moyenne, f, detail), mix(0.35, couvre * 0.85, detail));
    }
    bas *= 0.82 + 0.36 * fin;
    // talus : terre et maquis en touffes
    vec3 talus = mix(terre, maquis, smoothstep(0.38, 0.62, meso + (fin - 0.5) * 0.3)) * (0.85 + 0.3 * fin);
    // roche : bancs clairs et foncés, fissures sombres (fines, effacées au loin)
    const float fis = (1.0 - smoothstep(0.0, 0.035, abs(bruitSol(w * vec2(0.9, 0.35) + 3.0, oct) - 0.5))) * detail;
    vec3 roc = mix(roche, roche2, smoothstep(0.3, 0.8, meso)) * (0.85 + 0.3 * fin) * (1.0 - 0.45 * fis);

    base.rgb = mix(mix(bas, talus, tTerre), roc, tRoche);
    rough = mix(0.95, 0.85, tRoche); metal = 0.0;
}
