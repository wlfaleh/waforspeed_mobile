// WaForSpeed — WFS-380 : vraie eau (mer de la Corniche, bassin de la marina, rivière et port de la Ville, sillages).
// Une seule eau pour tout le jeu (RG-05) ; les lieux diffèrent par des PARAMÈTRES portés par les sommets (src/render/eau_parametres.h) :
//   uv.x = distance au bord de l'eau (m, négative sous le sable) ; uv.y = abscisse le long du bord (m) ;
//   normale = (pente du fond, ±agitation, 0) : |xz| = pente (m de profondeur par m, sable 0,03, roche 0,3, quai 1) ; |y| = agitation
//   (1 pleine mer, 0,3 port, 0,15 rivière) ; y < 0 : eau douce trouble (rivière : 4 m de fond au plus, vert-brun). Une rotation autour
//   de y (tours suivants de la piste) ne change ni |xz| ni y.
// Matériau MAT_EAU ; cutoff (pc.params.z) = mode : 0 surface d'eau, 1 sillage (uv : x latéral −1..1, y 0 à l'arrière du bateau
// → 1 au bout), 2 éclaboussure (uv : position sur la nappe de rayon 1, ou uv.y ≥ 2 : couronne soulevée, WFS-401), 3 nappe de ressac sur le sable (uv.x : remontée sur la plage
// depuis le bord, m ; uv.y : abscisse le long du bord). Tout est calculé ici : aucune texture téléchargée (RG-06).
const uint F_EAU = 8388608u;

// RG-01 : houle et mer du vent de la Méditerranée par beau temps (vent ≈ 4 m/s). xy : direction (unitaire), z : longueur d'onde (m),
// w : amplitude (m). Dispersion en eau profonde ω² = g k : 28 m → 4,2 s, 14,5 m → 3,0 s ; hauteur significative 4 √(Σ A² / 2) ≈ 0,46 m.
// Mêmes valeurs dans src/render/eau_parametres.h (test_eau compare les deux).
const int EAU_NB = 8;
const vec4 EAU_VAGUES[8] = vec4[8](
    vec4( 0.9781,  0.2079, 28.0, 0.110),
    vec4( 0.6157,  0.7880, 19.0, 0.085),
    vec4( 0.8290, -0.5592, 14.5, 0.070),
    vec4( 0.1045,  0.9945,  9.3, 0.040),
    vec4( 0.4540, -0.8910,  6.1, 0.034),
    vec4( 0.8910,  0.4540,  3.7, 0.026),
    vec4(-0.3907,  0.9205,  2.3, 0.019),
    vec4(-0.1392, -0.9903,  1.4, 0.012));
const float EAU_G = 9.81;

// bruit de valeur 2D (0..1), lisse : dentelle de l'écume
float eauHash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float eauBruit(vec2 p) {
    vec2 i = floor(p), f = fract(p), u = f * f * (3.0 - 2.0 * f);
    return mix(mix(eauHash(i), eauHash(i + vec2(1, 0)), u.x), mix(eauHash(i + vec2(0, 1)), eauHash(i + vec2(1, 1)), u.x), u.y);
}
// écume en dentelle : cellules de bulles qui dérivent et se défont (deux octaves), 0..1
float eauDentelle(vec2 p, float t) {
    float a = eauBruit(p * 1.7 + vec2(0.13 * t, -0.07 * t));
    float b = eauBruit(p * 4.3 - vec2(0.05 * t, 0.11 * t));
    return a * 0.65 + b * 0.35;
}
// WFS-401 (RG-01) : réseau de bulles de l'écume (cellules de Voronoï) : distance au centre de la cellule la plus proche, 0 (centre) à
// ≈ 0,75 (bord) ; les centres bougent lentement (bulles qui crèvent et se reforment) : 9 cellules lues
vec2 eauHash2(vec2 p) { return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453); }
float eauCellules(vec2 p, float t) {
    vec2 i = floor(p), f = fract(p);
    float d = 8.0;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++) {
            vec2 o = vec2(float(x), float(y)), h = eauHash2(i + o);
            vec2 c = o + 0.5 + 0.38 * sin(6.2831853 * h + t * (0.35 + 0.3 * h.yx)) - f;
            d = min(d, dot(c, c));
        }
    return sqrt(d);
}
// WFS-401 (RG-01) : écume réelle d'une part de couverture cov (0..1), rendue en OPACITÉ (0..1) : une mousse qui se défait en dentelle,
// pas une tache unie ni un trait filé, ni un aplat découpé (avis 3D B1). Cœur dense là où les bulles se serrent (bords des grosses
// cellules), bords minces et translucides en dégradé vers les trous (centres des cellules, rayon tiré par cellule : trous de tailles
// et de formes irrégulières), filaments fins (bords des petites cellules) qui relient les plaques par-dessus les trous. Les trous
// grandissent quand la couverture baisse ; une couverture faible reste translucide. Deux tailles (42 cm et 15 cm), isotropes : rapport
// longueur / largeur ≈ 1 (RG-01 : 4 au plus). « fin » (0..1) : part de la petite échelle (0 au loin : rien sous le pixel)
float eauMousse(vec2 p, float t, float cov, float fin) {
    if (cov <= 0.01) return 0.0;
    // bords des cellules très ondulés (deux échelles de déformation : trous allongés, en virgule, jamais ronds ni alignés)
    vec2 q = p + 0.55 * vec2(eauBruit(p * 1.3 + 3.1), eauBruit(p * 1.3 - 1.7)) + 0.2 * vec2(eauBruit(p * 4.1 + 8.2), eauBruit(p * 4.1 - 6.4));
    float c1 = eauCellules(q * 2.4, t), c2 = fin > 0.01 ? eauCellules(q * 6.5 + 7.3, t * 1.3) : 0.6;
    float var = eauBruit(q * 0.8 + 5.3);                                           // plaques plus ou moins serrées
    float trou = mix(0.62, 0.12, cov) + 0.4 * (var - 0.5);                         // rayon du trou, très variable d'un endroit à l'autre
    float corps = smoothstep(trou - 0.04, trou + 0.38, c1);                       // dégradé : mince au bord du trou, dense au cœur
    float m = corps * (0.3 + 0.7 * corps) * (0.65 + 0.35 * eauBruit(q * 3.3 - 2.1));   // épaisseur inégale (ombrage, translucidité)
    float fil = smoothstep(0.38, 0.6, c2) * fin * (1.0 - smoothstep(trou - 0.05, trou + 0.1, c1)) * 0.45;   // filaments dans les trous
    return max(m, fil) * (0.45 + 0.55 * smoothstep(0.0, 0.6, cov));
}

// pente de la surface (dh/dx, dh/dz), hauteur (m) et variance de pente perdue par les vagues trop petites pour le pixel (elle passe
// dans la rugosité : le scintillement du soleil s'étale au loin comme en vrai, sans crénelage ni motif répété jusqu'à l'horizon)
// WFS-401 (RG-01, RG-02) : « groupes » de vagues : hors Basse, l'amplitude de chaque vague varie lentement dans l'espace (bruit à
// 1,5 longueur d'onde le long de la crête, 1,2 en travers, qui dérive) : les crêtes infinies d'une sinusoïde deviennent des crêtes
// courtes qui se croisent (mer du vent réelle : crêtes de 1 à 3 longueurs d'onde), et l'interférence des 8 vagues ne se répète plus
// (aucune période commune). Moyenne de l'amplitude inchangée (bruit de moyenne 0,5 × 2) : même hauteur significative (test_eau).
// WFS-401 (R7) : hors Basse, chaque vague est gardée selon la taille du pixel MESURÉE DANS SA DIRECTION (dp = dérivées écran de la
// position, dFdx puis dFdy) : à angle rasant, le pixel est très long vers l'horizon et étroit en travers ; avant, la plus grande
// des deux longueurs effaçait aussi les vagues qui vont en travers (pourtant bien vues) : il ne restait que des bandes lisses
// parallèles à l'horizon (mer « filée »). Basse : taille du pixel isotrope (empreinte), comme avant.
void eauVagues(vec2 p, float t, float agit, int n, float empreinte, vec4 dp, out vec2 pente, out float h, out float varPerdue) {
    pente = vec2(0.0); h = 0.0; varPerdue = 0.0;
    bool groupes = n > 4;
    for (int i = 0; i < EAU_NB; i++) {
        if (i >= n) break;
        vec4 w = EAU_VAGUES[i];
        float k = 6.2831853 / w.z, om = sqrt(EAU_G * k), A = w.w * agit;
        float dphi = 0.0;
        if (groupes) {
            vec2 cr = vec2(-w.y, w.x);   // le long de la crête
            vec2 g = vec2(dot(p, cr) / (1.5 * w.z), dot(p, w.xy) / (1.2 * w.z)) + vec2(float(i) * 7.31, float(i) * 3.17 - 0.05 * t);
            A *= 2.0 * eauBruit(g);
            // crêtes qui se courbent et espacement qui varie (± 1,5 rad sur 3 longueurs d'onde) : vues au téléobjectif, les vagues
            // longues ne font plus des rayures régulières jusqu'à l'horizon (RG-02) ; la pente de ce déphasage (≈ 8 % de k) est négligée
            dphi = 3.0 * (eauBruit(g * 0.45 + vec2(11.3, -4.7)) - 0.5);
        }
        float ew = groupes ? abs(dot(dp.xy, w.xy)) + abs(dot(dp.zw, w.xy)) : empreinte;
        // au moins ≈ 3 pixels par longueur d'onde (WFS-401 : hors Basse, effacée plus tôt, de 4 à 8 pixels par longueur d'onde :
        // les vagues proches de la limite du pixel faisaient de fines rayures régulières au loin, RG-02 ; leur pente passe dans la rugosité)
        float garde = groupes ? 1.0 - smoothstep(0.12, 0.25, ew / w.z) : 1.0 - smoothstep(0.18, 0.45, ew / w.z);
        float th = k * dot(w.xy, p) - om * t + float(i) * 1.7 + dphi;
        h += A * sin(th) * garde;
        pente += w.xy * (A * k * cos(th) * garde);
        varPerdue += (1.0 - garde * garde) * 0.5 * (A * k) * (A * k);
    }
}

// RG-03 : couleur lue à travers l'eau (Beer-Lambert) : fond de sable ou de roche atténué sur le trajet de la lumière, plus la lumière
// diffusée par l'eau (Méditerranée claire, Jerlov I : absorption ≈ 0,45 / 0,065 / 0,018 m⁻¹ en rouge / vert / bleu)
vec3 eauCorps(float prof, float NoV, float pente, bool trouble) {
    // mer : absorption de l'eau pure + peu de particules ; rivière : matières dissoutes (le bleu absorbé) et limon en suspension
    vec3 ABS = trouble ? vec3(0.55, 0.32, 0.75) : vec3(0.45, 0.065, 0.018);
    vec3 RETRO = trouble ? vec3(0.030, 0.034, 0.018) : vec3(0.0035, 0.0045, 0.006);
    // fond : sable mouillé près du bord, roche calcaire (pente forte), herbier de posidonies au-delà de 6 à 12 m (vert très sombre) ;
    // rivière : vase
    vec3 fond = mix(vec3(0.46, 0.40, 0.29), vec3(0.20, 0.19, 0.16), smoothstep(0.08, 0.25, pente));
    fond = trouble ? vec3(0.14, 0.12, 0.09) : mix(fond, vec3(0.035, 0.05, 0.035), smoothstep(6.0, 12.0, prof));
    float cosT = sqrt(1.0 - (1.0 - NoV * NoV) / 1.77);   // angle réfracté (n = 1,33)
    vec3 T = exp(-(ABS + RETRO) * prof * (1.0 + 1.0 / max(cosT, 0.3)));
    vec3 profond = 0.33 * RETRO / (ABS + RETRO);          // réflectance de l'eau infiniment profonde (Gordon)
    // sortie de l'eau : (1 − F)² / n² ≈ 0,54 (Fresnel à l'entrée et à la sortie, compression du cône de lumière à la surface)
    return (fond * T + profond * (1.0 - T)) * 0.54;
}

// RG-02 (nuit) : reflet des lampadaires et des feux des bateaux, en traînées allongées par les vagues (lobe GGX de chaque lumière,
// sans le cône du projecteur : on voit la lampe elle-même dans l'eau). Les phares des voitures (feux de croisement) n'y sont pas.
vec3 eauRefletsLampes(vec3 p, vec3 N, vec3 V, float a) {
    vec3 s = vec3(0.0);
    int n = min(int(F.spotN.x + 0.5), 12);   // WFS-423 (RG-01) : vrai nombre de lumières de l'image (12 au plus : taille des tableaux)
    float NoV = max(dot(N, V), 1e-3);
    for (int i = 0; i < n; i++) {   // WFS-423 (RG-01) : plus de tours vides jusqu'à 12
        if (F.spotCone[i].z > 0.5) continue;
        vec3 Lv = F.spotPos[i].xyz - p;
        float d2 = max(dot(Lv, Lv), 1.0);
        vec3 L = Lv * inversesqrt(d2);
        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) continue;
        vec3 H = normalize(V + L);
        float NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
        float spec = D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * F_Schlick(vec3(0.02), VoH).x;
        s += min(F.spotCol[i].rgb * (spec * NoL * 2.5 / d2), vec3(SPEC_MAX));
    }
    return s;
}

// surface de l'eau : normale des vagues, couleur du corps de l'eau, rugosité, écume du bord (RG-01, 03, 04) ; renvoie la part
// d'écume (0..1) et l'opacité (sillage, éclaboussure)
float surfaceEau(inout vec4 base, inout float rough, inout float metal, inout vec3 N, vec3 V, out float alpha) {
    float t = F.camPos.w;
    int mode = int(pc.params.z + 0.5);
    vec2 p = vWorld.xz;
    metal = 0.0; alpha = 1.0;
    // RG-07 : qualité Basse. Le moteur ne passe pas la qualité aux shaders : F.spotN.z vaut 4 en Basse et 12 sinon (lectures du
    // reflet de la route, renderer.cpp, WFS-263) ; si cette valeur change, revoir ce seuil
    bool basse = F.spotN.z < 6.0;
    if (mode == 3) {   // D5 (avis 3D) : nappe du ressac sur le sable : l'eau monte, redescend, laisse le sable mouillé ; bord irrégulier
        float r = vUv.x, le = vUv.y;
        float ph = t * 0.785 + le * 0.021 + 2.0 * eauBruit(vec2(le * 0.03, 0.5));   // même cycle que l'écume dans l'eau
        float front = mix(-0.4, 2.6, 0.5 + 0.5 * sin(ph)) + 0.7 * (eauBruit(vec2(le * 0.45, 3.1)) - 0.5);
        float mouille = 3.0 + 0.8 * (eauBruit(vec2(le * 0.2, 7.3)) - 0.5);           // limite du sable mouillé (marée du ressac)
        float dentelle = eauDentelle(vec2(le * 1.1, r * 2.0) + vec2(0.0, -0.5 * t), t);
        float nappe = 1.0 - smoothstep(front - 0.15, front + 0.05, r);
        float lisere = exp(-pow((r - front) / 0.22, 2.0)) * smoothstep(0.35, 0.65, dentelle);
        float ecumeN = clamp(max(lisere, nappe * smoothstep(0.6, 0.85, dentelle) * 0.6), 0.0, 1.0);
        if (!basse) {   // WFS-401 (RG-01) : de près, écume de la nappe en mousse à trous (comme dans l'eau), plus de traits filés
            float dN = length(F.camPos.xyz - vWorld), presN = 1.0 - smoothstep(30.0, 45.0, dN);
            if (presN > 0.0) {
                float fin = 1.0 - smoothstep(0.03, 0.08, length(fwidth(vWorld.xz)));
                ecumeN = mix(ecumeN, 0.9 * eauMousse(vec2(le, r - 0.3 * t), t, clamp(ecumeN * 1.4, 0.0, 1.0), fin), presN);
            }
        }
        float sec = smoothstep(mouille - 0.7, mouille, r);
        // nappe : film d'eau sur le sable (sable sombre, lisse, le ciel s'y reflète) ; derrière : sable mouillé mat qui sèche
        base.rgb = mix(mix(vec3(0.20, 0.17, 0.12), vec3(0.24, 0.21, 0.15), 1.0 - nappe), basse ? vec3(0.80, 0.82, 0.84) : vec3(0.60, 0.63, 0.65), ecumeN);
        rough = mix(mix(0.32, 0.07, nappe), 0.8, ecumeN);
        float d = length(F.camPos.xyz - vWorld);
        alpha = clamp(max(nappe * 0.85, 0.75 * (1.0 - sec)), 0.0, 1.0) * (1.0 - smoothstep(60.0, 90.0, d)) * smoothstep(-0.5, -0.2, r);
        N = vec3(0.0, 1.0, 0.0);
        return ecumeN;
    }
    if (mode != 0) {   // RG-04 : sillage derrière un bateau, éclaboussure d'un dauphin : écume en dentelle, bords fondus
        float cov;
        if (mode == 1) {
            float u = abs(vUv.x), v = clamp(vUv.y, 0.0, 1.0);
            float bras = exp(-pow((1.0 - u) / 0.16, 2.0));            // deux bras du V (ondes de Kelvin qui déferlent)
            float axe = exp(-pow(u / mix(0.18, 0.45, v), 2.0)) * 1.2;  // eau brassée par l'hélice, qui s'élargit
            cov = max(bras * (1.0 - v * 0.6), axe * (1.0 - v)) * (1.0 - smoothstep(0.7, 1.0, v)) * smoothstep(0.0, 0.08, v);
        } else if (vUv.y > 1.5) {
            // WFS-401 (RG-04) : couronne d'eau soulevée par le dauphin : en bas une nappe d'eau laiteuse translucide, plus haut des
            // doigts (jets) qui se brisent en gouttes de 1 à 4 cm ; bord du haut déchiré, pied fondu dans l'eau ; jamais une paroi pleine
            float u = vUv.x, v = clamp(vUv.y - 2.0, 0.0, 1.0);
            // avis 3D R3 : jets inégaux et penchés (pas une palissade) : le tour est décalé selon la hauteur (jet incliné, sens et
            // angle propres à chaque jet), trois fréquences mêlées (largeurs variées), hauteur tirée par jet, base irrégulière
            float pen = (eauBruit(vec2(u * 9.0, 5.1)) - 0.5) * 0.06;                                 // inclinaison locale
            float uj = u + pen * v;
            float doigt = eauBruit(vec2(uj * 11.0, 0.7)) * 0.5 + eauBruit(vec2(uj * 27.0, 2.3)) * 0.3 + eauBruit(vec2(uj * 63.0, 4.9)) * 0.2;
            float haut = 0.15 + 0.85 * pow(eauBruit(vec2(uj * 17.0, 8.3)), 1.5) * (0.5 + doigt);   // hauteur du jet, très variable
            float vh = v / max(haut, 0.05);
            float pied = 0.04 + 0.22 * eauBruit(vec2(u * 13.0, 6.6)) + 0.14 * eauBruit(vec2(u * 47.0, 1.9));   // haut de la nappe déchiré
            float nappeC = (1.0 - smoothstep(pied, pied + 0.18, v)) * smoothstep(0.0, 0.08, v) * 0.26;
            // jet : s'amincit en montant (le seuil monte avec la hauteur), se défait en gouttes avant son sommet
            float jet = smoothstep(0.48 + 0.25 * vh, 0.6 + 0.25 * vh, doigt) * (1.0 - smoothstep(0.45, 0.85, vh)) * smoothstep(0.0, 0.12, v) * 0.6;
            // gouttes : une par cellule au plus (36 autour, 9 en hauteur), au-dessus du tiers de la gerbe et jusqu'au-dessus des jets
            // (gouttes lancées plus haut), rayon tiré, un peu étirées en hauteur (chute)
            vec2 cg = vec2(u * 36.0, v * 9.0), ic = floor(cg), fc = fract(cg);
            vec2 hg = eauHash2(ic + 31.7);
            vec2 dg = (fc - (0.25 + 0.5 * hg)) * vec2(1.0, 0.75);
            float rg = 0.14 + 0.18 * eauHash(ic + 5.1);
            float goutte = (1.0 - smoothstep(rg * 0.55, rg, length(dg))) * smoothstep(0.3, 0.5, vh) * (1.0 - smoothstep(1.1, 1.35, vh))
                         * step(0.45, eauHash(ic - 2.9));
            float aC = clamp(max(max(nappeC, jet), goutte * 0.9), 0.0, 1.0);
            alpha = aC * base.a;
            // eau éclairée : gouttes et jets blancs (bulles, lumière diffusée), nappe vert-bleu clair ; normale de la paroi (extérieur)
            base.rgb = mix(vec3(0.42, 0.55, 0.58), vec3(0.80, 0.82, 0.84), clamp(max(jet * 1.6, goutte) + vh * 0.3, 0.0, 1.0));
            rough = 0.3; metal = 0.0;
            N = normalize(vec3(vNrm.x, 0.5 * length(vNrm.xz), vNrm.z));
            return 1.0;
        } else {
            // WFS-401 (RG-04) : nappe d'écume posée autour de la gerbe : bord déchiré (rayon qui varie avec l'angle), mousse en dentelle
            float r = length(vUv), ang = atan(vUv.y, vUv.x);
            float bord = 0.75 + 0.25 * eauBruit(vec2(ang * 2.2 + 9.0, 1.3)) + 0.12 * (eauBruit(vec2(ang * 7.0, 4.1)) - 0.5);
            cov = (1.0 - smoothstep(0.3 * bord, bord, r)) * 0.6;
            float m = eauMousse(vWorld.xz * 2.2, t, cov, 1.0);
            alpha = clamp(m * 0.75, 0.0, 1.0) * base.a;
            base.rgb = vec3(0.74, 0.77, 0.79);
            rough = 0.85; N = vec3(0.0, 1.0, 0.0);
            return 1.0;
        }
        float dentelle = eauDentelle(p * 1.4, t);
        alpha = clamp(smoothstep(0.25, 0.75, dentelle * cov * 1.6), 0.0, 1.0) * base.a;
        base.rgb = vec3(0.80, 0.82, 0.84);
        rough = 0.85; N = vec3(0.0, 1.0, 0.0);
        return 1.0;
    }
    float agit = clamp(abs(vNrm.y), 0.05, 1.0), pente = length(vNrm.xz);
    bool trouble = vNrm.y < 0.0;
    float x = max(vUv.x, 0.0), le = vUv.y;   // distance au bord, abscisse le long du bord (m)
    float empreinte = max(length(fwidth(vWorld.xz)), 1e-4);   // taille du pixel sur l'eau (m)
    vec4 dp = vec4(dFdx(vWorld.xz), dFdy(vWorld.xz));          // WFS-401 (R7) : pixel dans chaque direction
    vec2 sl; float h, varPerdue;
    eauVagues(p, t, agit, basse ? 4 : EAU_NB, empreinte, dp, sl, h, varPerdue);
    // D2 (avis 3D) : petite mer du vent sans période (bruit qui dérive, deux échelles) : aucune rayure qui se répète ; s'efface quand
    // le pixel devient plus grand que ses rides (la rugosité prend le relais)
    if (!basse) {
        float garde = 1.0 - smoothstep(0.15, 0.45, empreinte / 1.2);   // bruit sans direction : filtré sur le côté long du pixel
        if (garde > 0.0) {
            vec2 q = p * 0.85 + vec2(0.31 * t, -0.17 * t), r2 = p * 2.3 - vec2(0.22 * t, 0.37 * t);
            const float e = 0.15;
            float b0 = eauBruit(q), b1 = eauBruit(r2);
            vec2 g = vec2(eauBruit(q + vec2(e, 0.0)) - b0, eauBruit(q + vec2(0.0, e)) - b0) / e * 0.045
                   + vec2(eauBruit(r2 + vec2(e, 0.0)) - b1, eauBruit(r2 + vec2(0.0, e)) - b1) / e * 0.025;
            sl += g * garde * agit;
        }
    }
    N = normalize(vec3(-sl.x, 1.0, -sl.y));
    // rugosité : petite ride sous le pixel (Cox-Munk, vent faible) + vagues perdues au loin ; Basse : 4 vagues, le reste en rugosité.
    // Plafonnée à 0,35 : au-delà, le ciel reflété au loin s'assombrissait (bande sombre sous l'horizon, D1 de l'avis 3D)
    float vmPetites = basse ? 0.0024 : 0.0006;
    rough = clamp(sqrt(sqrt(0.0004 + 2.0 * (varPerdue + vmPetites * agit))), 0.08, 0.35);
    // WFS-401 (RG-03) : traînée du soleil (ou de la lune) faite de reflets séparés. Les rides plus petites que le pixel ne floutent pas
    // le soleil en tache : chacune est une petite facette qui renvoie le soleil vers l'œil ou non (« glitter », Cox et Munk 1954).
    // Hors Basse ; soleil ou lune levé.
    if (!basse && F.sunDir.w > 0.0 && F.sunDir.y > -0.02) {
        float sig2 = varPerdue + vmPetites * agit + 0.0008;
        // poids : probabilité relative que la facette qui renverrait le soleil (demi-vecteur H entre l'œil et le soleil) existe là,
        // loi gaussienne des pentes : exp(−tan²θ / 2σ²), tan²θ ≈ 2 (1 − N·H). Mesurée sur H et non sur le reflet : à angle rasant, une
        // pente de côté ne dévie presque pas le reflet (2 sin(élévation) × pente) : la traînée reste une colonne étroite sous le soleil,
        // comme en vrai ; elle s'élargit au loin avec les pentes perdues. Queue un peu plus large que la loi : bords doux (avis 3D B2).
        vec3 Hs = normalize(V + normalize(F.sunDir.xyz));
        float poids = exp(-(1.0 - dot(N, Hs)) / (1.6 * sig2));
        if (poids > 0.01) {
            // avis 3D B3 : les paillettes sont tirées SUR L'EAU (cellules du monde), pas sur la grille de l'écran : elles restent à
            // leur place sur la mer quand la caméra bouge. Taille de la cellule : 0,2 m au moins et 2 pixels au moins, par paliers
            // (puissances de 2 : la cellule ne change pas d'une image à l'autre). Chaque paillette a sa propre vie (≈ 0,4 s, début
            // décalé d'une cellule à l'autre) : elles naissent et s'éteignent une à une, sans fourmillement de tout l'écran.
            float taille = exp2(ceil(log2(max(0.2, 2.0 * empreinte))));
            vec2 cw = floor(p / taille);
            vec2 cel = cw + floor(t * 2.5 + eauHash(cw + 1.3)) * vec2(17.31, 9.13);
            // une paillette tirée est une facette qui renvoie vraiment le soleil (normale = demi-vecteur, à peine bougée) : plus de
            // pixel noir dans la traînée ; ailleurs la normale et le lobe moyens restent (au plus 35 % des cellules : paillettes séparées)
            // la facette est un petit point rond à une place tirée dans la cellule (rayon : 1,5 cm ou un demi-pixel au moins, bord
            // fondu), pas la cellule entière : aucun carré ; vu en rasant, il s'aplatit en petit trait horizontal comme en vrai
            if (eauHash(cel + 3.7) < 0.35 * poids) {
                vec2 hh = eauHash2(cel);
                vec2 c = (cw + 0.2 + 0.6 * hh) * taille;
                float rg = max(0.015, 0.5 * empreinte);
                float w = 1.0 - smoothstep(0.5 * rg, rg, length(p - c));
                if (w > 0.0) {
                    N = normalize(mix(N, normalize(Hs + vec3(hh.x - 0.5, 0.0, hh.y - 0.5) * 0.02), w));
                    rough = mix(rough, 0.06, w);
                }
            }
        }
    }
    float NoV = max(dot(N, V), 1e-3);
    // profondeur : pente du bord sur 40 m, puis le tombant du plateau (≈ 12 %, petits fonds méditerranéens), 60 m au plus ;
    // rivière : 4 m au plus
    float prof = trouble ? min(0.12 + x * pente, 4.0) : min(0.12 + x * pente + max(x - 40.0, 0.0) * 0.12, 60.0);
    base.rgb = eauCorps(prof, NoV, pente, trouble);
    // RG-04 : écume du bord, en LIGNES parallèles au rivage (bruit étiré le long du bord : abscisse « le ») qui avancent vers lui.
    // Sable : nappe du ressac (se prolonge sur la plage : mode 3), petite vague qui déferle à 10 m puis roule jusqu'au bord, traînées
    // laissées derrière. Roche et quai : la vague bat contre le bord, l'écume monte sur 1 à 4 m et redescend, plaques qui dérivent.
    float ecume = 0.0;
    if (x < 12.0 && agit > 0.2 && !trouble) {
        float roche = smoothstep(0.1, 0.3, pente);
        float ph = t * 0.785 + le * 0.021 + 2.0 * eauBruit(vec2(le * 0.03, 0.5));
        float cyc = fract(ph / 6.2831853 + 0.35);
        // traînées : taches de 2,2 m le long du bord sur 0,7 m en travers, dentelle fine par-dessus (WFS-401, RG-01 : avant 6 m sur
        // 0,4 m, rapport 15 : l'écume paraissait « filée » le long du rivage ; maintenant 3 au plus)
        float trainee = eauBruit(vec2(le * 0.45, (x + 1.2 * t) * 1.45));
        float fin = eauBruit(vec2(le * 1.3 + 0.4 * t, x * 3.2 - 0.9 * t));
        // ligne de déferlement (sable) : naît à 10 m, roule vers le bord à ≈ 1 m/s en s'élargissant, s'efface au bord
        float xb = mix(10.0, 0.5, cyc) * agit;
        float ligne = exp(-pow((x - xb) / mix(0.35, 1.3, cyc), 2.0)) * (1.0 - cyc * 0.6);
        // reste d'écume derrière la ligne (entre la ligne et le large, 3 m), qui se défait
        float reste = smoothstep(xb, xb + 0.3, x) * (1.0 - smoothstep(xb + 0.3, xb + 3.0, x)) * 0.5;
        // WFS-401 : seuil plus doux (0,3 à 0,8 au lieu de 0,42 à 0,72) : de près, c'est la mousse (eauMousse) qui découpe les bords
        float sable = clamp(ligne + reste, 0.0, 1.0) * smoothstep(basse ? 0.42 : 0.3, basse ? 0.72 : 0.8, trainee * 0.7 + fin * 0.45);
        // ressac au bord (0 à 2 m) : nappe qui suit le cycle
        float ressac = (1.0 - smoothstep(0.0, mix(0.8, 2.2, 0.5 + 0.5 * sin(ph)), x)) * smoothstep(0.38, 0.62, fin * 0.6 + trainee * 0.5);
        // roche : la vague (hauteur h) monte contre le bord ; plaques qui dérivent vers le large
        float monte = mix(1.8, 5.0, clamp(h * 3.0 + 0.5, 0.0, 1.0));
        float rocheE = (1.0 - smoothstep(monte * 0.5, monte, x)) * smoothstep(0.22, 0.5, eauDentelle(p * 1.5, t) * 0.8 + 0.25)
                     // liseré au pied de la roche : largeur qui varie le long du bord et avec la vague (WFS-401 : plus une bande droite)
                     // (Basse : bande fixe de 0,9 m, comme avant : même coût que main, RG-06)
                     + (1.0 - smoothstep(0.0, basse ? 0.9 : 0.3 + 1.1 * eauBruit(vec2(le * 0.7, 0.25 * t)) + 0.6 * clamp(h * 3.0, 0.0, 1.0), x)) * 0.8
                     + (1.0 - smoothstep(3.0, 10.0, x)) * smoothstep(0.58, 0.78, eauBruit(vec2(le * 0.4, x * 1.1 - 0.4 * t)));
        // B1 (lead dev) : l'écume du bord vient des vagues : aucune dans l'eau calme (port 0,3, rivière 0,15), entière en pleine mer
        ecume = mix(max(sable, ressac), clamp(rocheE, 0.0, 1.0), roche) * 0.85 * smoothstep(0.5, 0.9, agit);
        // WFS-401 (RG-01) : de près (moins de 45 m, hors Basse), la couverture devient une mousse en dentelle (réseau de bulles, trous
        // qui s'ouvrent quand elle se défait) ; au-delà, le pixel couvre plusieurs bulles : la couverture moyenne suffit
        float dCam = length(F.camPos.xyz - vWorld);
        float pres = basse ? 0.0 : 1.0 - smoothstep(30.0, 45.0, dCam);
        if (pres > 0.0) {
            vec2 pm = vec2(le, x + 0.35 * t);   // repère du bord : la mousse avance avec le front vers le rivage
            float m = eauMousse(pm, t, clamp(ecume * 1.25, 0.0, 1.0), 1.0 - smoothstep(0.03, 0.08, empreinte));
            m *= smoothstep(0.03, 0.3, ecume) * (1.0 - 0.6 * smoothstep(4.0, 9.0, x));   // avis 3D B1 : au large du bord, plaques minces
            ecume = mix(ecume, 0.9 * m, pres);
        }
    }
    // avis 3D B1 : hors Basse, écume blanc cassé (albédo ≈ 0,63, mousse humide qui laisse passer un peu l'eau) au lieu de 0,82
    base.rgb = mix(base.rgb, basse ? vec3(0.80, 0.82, 0.84) : vec3(0.60, 0.63, 0.65), ecume);
    rough = mix(rough, 0.8, ecume);
    N = normalize(mix(N, vec3(0.0, 1.0, 0.0), ecume));
    return ecume;
}
