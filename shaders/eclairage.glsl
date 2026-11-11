// WaForSpeed — WFS-149 (RG-05) : code de scene.frag déplacé tel quel (rien de réécrit), inclus par scene.frag et
// scene_skin.frag après leurs entrées, uniformes et drapeaux F_* (il les lit directement).
// Domaine : E13 Lumière et image. GGX, Fresnel, ombre du soleil, projecteurs, brouillard, halos des lumières.

// part du brouillard au point p (même calcul que la fin de main)
// WFS-381 (RG-01) : brume exponentielle selon la distance ET la hauteur (référence testée : src/render/atmosphere_brume.h) :
// fog.a = β0 (m⁻¹) au niveau fogLin.w, fogLin.z = 1 / H (0 : sans hauteur, calcul d'avant à l'identique)
float brumeTau(float d, float yPoint) {   // épaisseur optique entre l'œil et un point à la distance d, à la hauteur yPoint
    float tau = d * F.fog.a;
    if (F.fogLin.z > 0.0) {
        float yo = max(F.camPos.y, F.fogLin.w), x = (max(yPoint, F.fogLin.w) - yo) * F.fogLin.z;
        tau *= exp(-(yo - F.fogLin.w) * F.fogLin.z) * (abs(x) < 1e-4 ? 1.0 - 0.5 * x : (1.0 - exp(-x)) / x);
    }
    return tau;
}
float fogAt(vec3 p) {
    if (F.fogLin.y > 0.0) return smoothstep(F.fogLin.x, F.fogLin.y, dot(p - F.camPos.xyz, F.camFwd.xyz));
    return 1.0 - exp(-brumeTau(length(F.camPos.xyz - p), p.y));
}
// WFS-381 : brume appliquée à la couleur d'un pixel au point p. RG-03 : couleur de l'horizon du moment (fog.rgb), plus claire et
// plus chaude du côté du soleil (diffusion vers l'avant des aérosols, lobe de Mie en cos⁸) ; la nuit et sous la pluie : fog.rgb
vec3 appliquerBrume(vec3 col, vec3 p) {
    if (F.fogLin.y > 0.0) return mix(col, F.fog.rgb, smoothstep(F.fogLin.x, F.fogLin.y, dot(p - F.camPos.xyz, F.camFwd.xyz)));
    vec3 v = p - F.camPos.xyz;
    float d2 = dot(v, v), inv = inversesqrt(max(d2, 1e-8));
    vec3 brume = F.fog.rgb;
    if (F.fogLin.z > 0.0 && F.misc.y < 0.5) {   // F.sunDir : normalisé sur le CPU (Renderer::beginFrame)
        float c = max(dot(v, F.sunDir.xyz) * inv, 0.0); c *= c; c *= c; c *= c;
        brume *= mix(vec3(1.0), F.sunColor.rgb * (1.6 / max(dot(F.sunColor.rgb, vec3(0.2126, 0.7152, 0.0722)), 1e-4)), 0.35 * c);
    }
    return mix(col, brume, 1.0 - exp(-brumeTau(d2 * inv, p.y)));
}

// WFS-100 (RG-02) : plafond du spéculaire (valeur HDR) avant le bloom, pour toutes les surfaces
const float SPEC_MAX = 4.0;
const float SPEC_MAX_CROISEMENT = 0.8;   // WFS-382 (RG-03) : reflet d'un feu de croisement sur la chaussée, borné (éclat brillant, jamais blanc brûlé)
float D_GGX(float NoH, float a) { float a2 = a * a; float d = NoH * NoH * (a2 - 1.0) + 1.0; return a2 / (PI * d * d); }
float V_Smith(float NoV, float NoL, float a) {
    float k = a * 0.5; return 0.25 / max((NoV * (1.0 - k) + k) * (NoL * (1.0 - k) + k), 1e-4);
}
vec3 F_Schlick(vec3 f0, float VoH) { return f0 + (1.0 - f0) * pow(1.0 - VoH, 5.0); }

// repère tangent sans tangentes : dérivées écran (normal maps)
vec3 perturb(vec3 N, vec3 p, vec2 uv, vec3 tn) {
    vec3 dp1 = dFdx(p), dp2 = dFdy(p); vec2 du1 = dFdx(uv), du2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, N), dp1perp = cross(N, dp1);
    vec3 T = dp2perp * du1.x + dp1perp * du2.x, B = dp2perp * du1.y + dp1perp * du2.y;
    float inv = inversesqrt(max(max(dot(T, T), dot(B, B)), 1e-12));
    return normalize(mat3(T * inv, B * inv, N) * tn);
}

// ---------- ombre du soleil : WFS-227 (lot 1 de WFS-153) : cascades stabilisées, pénombre physique ----------
// Cascades (src/render/eclairage_ombres.h) : F.ombreCasc[k] (monde → [-1, 1]² × [0, 1]), fin le long du regard F.ombreFin[k],
// demi-côté F.ombreDemi[k] ; atlas de 2 × 2 tuiles (une cascade : image entière). Profondeur de chaque cascade :
// 2 × demi + RECUL_SOLEIL_M (même constante que le C++, vérifiée par tests/fidelite_lumiere_ombres.cpp).
// Biais (acné, décollement) en texels de la cascade : décalage le long de la normale 1,5 texel, profondeur 1 texel (pente
// plafonnée par le décalage normal) ; une cascade fine (près) garde donc un biais fin : l'ombre reste collée au pied.
// Pénombre (RG-02) : largeur = distance occultant → récepteur × tan(0,53°) (F.ombreP.w) : distance de l'occultant estimée par
// des comparaisons à des profondeurs décalées (0,3 à 30 m, dichotomie : 3 essais ; carte de comparaison : pas de lecture brute) ;
// filtre sur un disque de Poisson de cette largeur (au moins un texel), tourné par pixel (pas de bandes) : 8 lectures en Haute et
// Ultra (4 pour la recherche), 4 en Moyenne (1 pour la recherche) : moins de lectures, même physique. Point éclairé : 4 lectures.
const float RECUL_SOLEIL_M = 120.0;
const vec2 POISSON[12] = vec2[12](vec2(-0.326, -0.406), vec2(-0.840, -0.074), vec2(-0.696, 0.457), vec2(-0.203, 0.621),
    vec2(0.962, -0.195), vec2(0.473, -0.480), vec2(0.519, 0.767), vec2(0.185, -0.893), vec2(0.507, 0.064), vec2(0.896, 0.412),
    vec2(-0.322, -0.933), vec2(-0.792, -0.598));
const float DELTAS_M[5] = float[5](0.3, 1.0, 3.0, 10.0, 30.0);
// ombre de la cascade k (1 éclairé, 0 à l'ombre) ; −1 : point hors de la cascade
float ombreCascade(int k, vec3 p, vec3 n) {
    float demi = F.ombreDemi[k], tuile = F.ombreP.z;
    float texelM = 2.0 * demi / tuile, profM = 2.0 * demi + RECUL_SOLEIL_M;
    vec4 c = F.ombreCasc[k] * vec4(p + n * (1.5 * texelM), 1.0);
    vec2 uv = c.xy * 0.5 + 0.5;
    if (uv.x <= 0.0 || uv.y <= 0.0 || uv.x >= 1.0 || uv.y >= 1.0 || c.z >= 1.0) return -1.0;
    float sc = F.ombreP.x > 1.5 ? 0.5 : 1.0;
    vec2 o0 = F.ombreP.x > 1.5 ? vec2(float(k % 2), float(k / 2)) * 0.5 : vec2(0.0);
    vec2 auv = o0 + uv * sc;
    float txUv = sc / tuile, mParUv = 2.0 * demi / sc;
    vec2 lo = o0 + vec2(txUv * 0.5), hi = o0 + vec2(sc - txUv * 0.5);   // jamais de lecture dans la tuile voisine
    float z = c.z - texelM / profM;
    bool fine = F.ombreP.y > 1.5;                                        // Haute, Ultra
    // 1) distance de l'occultant : part ombrée (occ) quand on rapproche la profondeur comparée de Δ vers le soleil
    float rs = max(0.5 * DELTAS_M[4] * F.ombreP.w, 2.0 * texelM) / mParUv;
    int nS = fine ? 4 : 1;
    vec2 dS[4] = vec2[4](vec2(0.0), vec2(0.7, 0.7), vec2(-0.7, 0.7), vec2(0.0, -1.0));
    if (fine) { dS[0] = vec2(-0.7, -0.7); }
    float occ0 = 0.0;
    for (int i = 0; i < nS; i++) occ0 += 1.0 - texture(shadowMap, vec3(clamp(auv + dS[i] * rs, lo, hi), z));
    occ0 /= float(nS);
    if (occ0 <= 0.0) return 1.0;                                         // aucun occultant autour : éclairé
    // dichotomie sur les 5 profondeurs (3 comparaisons) : plus grand Δ où au moins la moitié de l'occultation reste
    int lo2 = -1, hi2 = 4;   // occ(Δ[lo2]) > ½ occ0 (lo2 = −1 : Δ = 0) ; occ(Δ[hi2]) ≤ ½ occ0 supposé (au-delà : 30 m)
    for (int it = 0; it < 3; it++) {
        int mid = (lo2 + hi2 + 1) / 2;
        float o = 0.0;
        for (int i = 0; i < nS; i++) o += 1.0 - texture(shadowMap, vec3(clamp(auv + dS[i] * rs, lo, hi), z - DELTAS_M[mid] / profM));
        if (o / float(nS) > 0.5 * occ0) lo2 = mid; else hi2 = mid;
    }
    float dB = sqrt(max(lo2 >= 0 ? DELTAS_M[lo2] : 0.15, 0.15) * DELTAS_M[hi2]);   // milieu géométrique de l'intervalle trouvé
    // 2) filtre sur la largeur de la pénombre (demi-largeur en rayon), au moins un texel
    float r = max(0.5 * dB * F.ombreP.w, texelM) / mParUv;
    float ang = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
    mat2 R = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
    int nF = fine ? 8 : 4;
    float sum = 0.0;
    for (int i = 0; i < 12; i++) {
        if (i >= nF) break;
        sum += texture(shadowMap, vec3(clamp(auv + R * POISSON[i] * r, lo, hi), z));
    }
    return sum / float(nF);
}
float sunShadow(vec3 p, vec3 n) {
    if (F.shadowP.x < 0.5) return 1.0;
    int nc = int(F.ombreP.x + 0.5);
    float d = dot(p - F.camPos.xyz, F.camFwd.xyz);
    if (d >= F.ombreFin[nc - 1]) return 1.0;
    int k = 0;
    while (k < nc - 1 && d > F.ombreFin[k]) k++;
    float o = ombreCascade(k, p, n);
    if (o < 0.0) { o = k + 1 < nc ? ombreCascade(k + 1, p, n) : 1.0; return o < 0.0 ? 1.0 : o; }
    // fondu vers la cascade suivante sur les derniers 15 % (la dernière s'efface vers « éclairé ») : pas de couture visible
    float debut = k > 0 ? F.ombreFin[k - 1] : 0.0, fin = F.ombreFin[k];
    float t = smoothstep(mix(debut, fin, 0.85), fin, d);
    if (t > 0.0) {
        float o2 = k + 1 < nc ? ombreCascade(k + 1, p, n) : 1.0;
        o = mix(o, o2 < 0.0 ? 1.0 : o2, t);
    }
    return o;
}

// projecteurs (WFS-45) : SpotLight de three.js r128 sans lumières physiques (lights_pars_begin / lights_fragment_begin) :
// cône smoothstep(coneCos, penumbraCos, cosθ), distance pow(saturate(1 − d / portée), décroissance), même GGX que le soleil.
// Pas d'ombre portée des projecteurs.
// WFS-218 : feu de croisement (spotCone.z = 1) : répartition et constantes de src/render/phares.h (à garder identiques),
// éclairement physique 1/d² ; w : direction phare → point, D : axe du phare (avant de la voiture, horizontal)
float feuCroisement(vec3 w, vec3 D, float d, float range) {
    vec3 R = normalize(cross(D, vec3(0.0, 1.0, 0.0))), U = cross(R, D);
    float el = degrees(asin(clamp(dot(w, U), -1.0, 1.0))), az = degrees(atan(dot(w, R), dot(w, D)));
    float lim = -0.57 + min(max(az, 0.0) * 0.26795, 1.57), t = (-el - 1.5) / 0.85, ah = az / 18.0;   // WFS-382 : plateau à −1,5°   // WFS-226 : étalement en (az / 18)⁴
    float v = (el > -1.5 ? 1.0 : 1.0 / (1.0 + pow(t, 1.6))) * (1.0 - 0.985 * smoothstep(lim - 0.3, lim + 0.3, el));
    return v * (1.0 - smoothstep(60.0, 80.0, abs(az))) / (1.0 + ah * ah * ah * ah) * (1.0 - smoothstep(0.8 * range, range, d)) / max(d * d, 0.25)
         * smoothstep(0.40, 0.70, d * dot(w, D));   // WFS-413 : boîtier et bouclier cachent la lampe à moins de 40 cm devant la glace
}
// WFS-353 (RG-01, accord E13 du PO du 2026-11-09 09:12:58) : variante « 2 lumières par pixel » (spotN.y = 2), lancée par le
// banc seulement tant que le PO n'a pas tranché (qualite_auto.h, LUMIERES_PAR_PIXEL_BASSE : le jeu garde 4). Toutes les lumières
// qui atteignent le pixel l'éclairent encore (même faisceau, même portée, même 1/d² : flaques de lumière des lampadaires et
// des phares gardées, aucune lumière ne disparaît). Les 2 plus fortes AU POINT ÉCLAIRÉ (éclairement reçu × luminance de la
// couleur, pas l'ordre de la liste) reçoivent le calcul complet (coupure exacte du feu de croisement, Fresnel, GGX) ; les
// autres passent en approximation : diffus seul, Fresnel pris à l'incidence normale (1 − f0), sans reflet spéculaire, et
// coupure du feu de croisement approchée (feuCroisementRapide).
// atan2 approché (polynôme de degré 7, erreur ≤ 0,0015 rad), pour la seule approximation
float atanRapide(float y, float x) {
    float ax = abs(x), ay = abs(y), t = min(ax, ay) / max(max(ax, ay), 1e-6), s = t * t;
    float r = ((-0.0464964749 * s + 0.15931422) * s - 0.327622764) * s * t + t;
    if (ay > ax) r = 1.57079637 - r;
    if (x < 0.0) r = 3.14159274 - r;
    return y < 0.0 ? -r : r;
}
// même répartition que feuCroisement (phares.h), sans normalize / cross / asin / atan : axe D pris horizontal (U = haut),
// élévation asin(t) ≈ t (écart < 1,2 % sous 15°), azimut par atanRapide
float feuCroisementRapide(vec3 w, vec3 D, float d, float range) {
    float el = 57.2957795 * w.y, az = 57.2957795 * atanRapide(w.z * D.x - w.x * D.z, dot(w, D));
    float lim = -0.57 + min(max(az, 0.0) * 0.26795, 1.57), t = (-el - 1.5) / 0.85, ah = az / 18.0;   // WFS-382 : plateau à −1,5°
    float v = (el > -1.5 ? 1.0 : 1.0 / (1.0 + pow(t, 1.6))) * (1.0 - 0.985 * smoothstep(lim - 0.3, lim + 0.3, el));
    return v * (1.0 - smoothstep(60.0, 80.0, abs(az))) / (1.0 + ah * ah * ah * ah) * (1.0 - smoothstep(0.8 * range, range, d)) / max(d * d, 0.25)
         * smoothstep(0.40, 0.70, d * dot(w, D));   // WFS-413 : boîtier et bouclier cachent la lampe à moins de 40 cm devant la glace
}
// WFS-382 (RG-01, accord du PO du 2026-11-10) : rétroréflexion de l'asphalte vers un conducteur placé derrière ses phares (EN 1436, R_L ;
// avis expert-realisme-3d : k = 0,12 / sin α, de 1 à 7) ; feux du joueur seulement (spotCone.y = 1 : pénombre 1 posée par nuit.cpp), sol
// horizontal sombre (s'efface sur la neige et les surfaces claires, qui diffusent comme Lambert : RG-07)
float retroChaussee(int i, vec3 L, vec3 N, vec3 albedo) { return F.spotCone[i].z > 0.5 && F.spotCone[i].y > 0.99 && N.y > 0.9 ? mix(clamp(0.12 / max(L.y, 1e-3), 1.0, 7.0), 1.0, smoothstep(0.2, 0.4, dot(albedo, vec3(0.2126, 0.7152, 0.0722)))) : 1.0; }
vec3 spotLights2(int n, vec3 p, vec3 N, vec3 V, vec3 albedo, vec3 f0, float metal, float a, bool noSpec) {
    vec3 sum = vec3(0.0);
    float e0 = 0.0, e1 = 0.0, e2 = 0.0, k0 = 0.0, k1 = 0.0;   // 2 plus fortes : éclairement reçu (classement) et k approché déjà compté ; e2 : 3e
    int i0 = -1, i1 = -1;
    for (int i = 0; i < n; i++) {   // WFS-423 (RG-01) : bornée au vrai nombre de lumières (n ≤ 12, spotLights)
        vec3 Lv = F.spotPos[i].xyz - p;
        float d = length(Lv);
        vec3 L = Lv / max(d, 1e-4);
        float cosT = dot(L, -F.spotDir[i].xyz);
        float NoL = max(dot(N, L), 0.0);
        bool croisement = F.spotCone[i].z > 0.5;
        if ((!croisement && cosT <= F.spotCone[i].x) || (croisement && (d > F.spotPos[i].w || cosT < 0.17)) || NoL <= 0.0) continue;
        float range = F.spotPos[i].w, decay = F.spotDir[i].w;
        if (range > 0.0 && d >= range) continue;
        float att = (range > 0.0 && decay > 0.0) ? pow(clamp(1.0 - d / range, 0.0, 1.0), decay) : 1.0;
        float k = (croisement ? feuCroisementRapide(-L, F.spotDir[i].xyz, d, range) : smoothstep(F.spotCone[i].x, F.spotCone[i].y, cosT) * att) * NoL * retroChaussee(i, L, N, albedo);   // WFS-382
        sum += F.spotCol[i].rgb * k;   // approximation : diffus seul (multiplié par l'albédo à la fin)
        float e = dot(F.spotCol[i].rgb, vec3(0.2126, 0.7152, 0.0722)) * k;   // éclairement reçu (luminance)
        if (e > e0) { e2 = e1; e1 = e0; i1 = i0; k1 = k0; e0 = e; i0 = i; k0 = k; }
        else if (e > e1) { e2 = e1; e1 = e; i1 = i; k1 = k; }
        else e2 = max(e2, e);
    }
    vec3 diffus = (1.0 - metal) * albedo / PI;
    vec3 tot = (1.0 - f0) * diffus * sum;
    // les 2 plus fortes : calcul complet (comme spotLights), à la place de leur part approchée. RG-02 (aucune lumière qui
    // « saute ») : la 2e passe en fondu vers l'approximation quand la 3e la rattrape (e2 → e1) : à l'échange de rang, les deux
    // sont approchées, aucune frontière nette dans les reflets de la chaussée mouillée
    float NoV = max(dot(N, V), 1e-3), w1 = smoothstep(1.0, 1.6, e1 / max(e2, 1e-8));
    for (int j = 0; j < 2; j++) {
        int i = j == 0 ? i0 : i1;
        if (i < 0) break;
        vec3 Lv = F.spotPos[i].xyz - p;
        float d = length(Lv);
        vec3 L = Lv / max(d, 1e-4);
        float NoL = max(dot(N, L), 0.0), kA = j == 0 ? k0 : k1, k = kA;
        if (F.spotCone[i].z > 0.5) k = feuCroisement(-L, F.spotDir[i].xyz, d, F.spotPos[i].w) * NoL * retroChaussee(i, L, N, albedo);   // coupure exacte ; WFS-382
        vec3 H = normalize(V + L);
        float NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
        vec3 Fs = F_Schlick(f0, VoH);
        vec3 spec = noSpec ? vec3(0.0) : D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;
        tot += (((1.0 - Fs) * k - (1.0 - f0) * kA) * diffus * F.spotCol[i].rgb + min(spec * F.spotCol[i].rgb * k, vec3(F.spotCone[i].z > 0.5 ? SPEC_MAX_CROISEMENT : SPEC_MAX))) * (j == 0 ? 1.0 : w1);   // WFS-100 RG-02
    }
    return max(tot, vec3(0.0));
}
vec3 spotLights(vec3 p, vec3 N, vec3 V, vec3 albedo, vec3 f0, float metal, float a, bool noSpec) {
    vec3 sum = vec3(0.0);
    int n = min(int(F.spotN.x + 0.5), 12);   // WFS-423 (RG-01) : vrai nombre de lumières de l'image (12 au plus : taille des tableaux)
    int parPixel = F.spotN.y > 0.5 ? int(F.spotN.y + 0.5) : 12, vus = 0;   // WFS-263 (RG-02) : 4 lumières par pixel au plus en Basse
    if (parPixel == 2) return spotLights2(n, p, N, V, albedo, f0, metal, a, noSpec);   // WFS-353 : Basse (Moyenne / Haute : 12)
    float NoV = max(dot(N, V), 1e-3);
    // WFS-226 : 12 projecteurs au plus ; WFS-423 (RG-01) : la boucle s'arrête au vrai nombre de lumières de l'image (n) ou dès que
    // le plafond par pixel est atteint (4 en Basse) : plus aucun tour vide jusqu'à 12 (même résultat, mêmes lumières gardées)
    for (int i = 0; i < n && vus < parPixel; i++) {
        vec3 Lv = F.spotPos[i].xyz - p;
        float d = length(Lv);
        vec3 L = Lv / max(d, 1e-4);
        float cosT = dot(L, -F.spotDir[i].xyz);
        float NoL = max(dot(N, L), 0.0);
        bool croisement = F.spotCone[i].z > 0.5;   // WFS-218
        if ((!croisement && cosT <= F.spotCone[i].x) || (croisement && (d > F.spotPos[i].w || cosT < 0.17)) || NoL <= 0.0) continue;   // WFS-226 : hors faisceau
        float range = F.spotPos[i].w, decay = F.spotDir[i].w;
        if (range > 0.0 && d >= range) continue;   // WFS-263 : hors de portée, n'éclaire pas ce pixel (et ne compte pas)
        vus++;   // WFS-263 : lumière qui éclaire vraiment ce pixel (dans le faisceau et à portée)
        float att = (range > 0.0 && decay > 0.0) ? pow(clamp(1.0 - d / range, 0.0, 1.0), decay) : 1.0;
        float k = (croisement ? feuCroisement(-L, F.spotDir[i].xyz, d, range) : smoothstep(F.spotCone[i].x, F.spotCone[i].y, cosT) * att) * NoL * retroChaussee(i, L, N, albedo);   // WFS-382
        vec3 H = normalize(V + L);
        float NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
        vec3 Fs = F_Schlick(f0, VoH);
        vec3 spec = noSpec ? vec3(0.0) : D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;
        sum += (1.0 - Fs) * (1.0 - metal) * albedo / PI * F.spotCol[i].rgb * k + min(spec * F.spotCol[i].rgb * k, vec3(croisement ? SPEC_MAX_CROISEMENT : SPEC_MAX));   // WFS-100 RG-02 ; WFS-382
    }
    return sum;
}

// WFS-356 (RG-02, ANO-202) : adaptation locale de l'œil aux pièces éclairées vues de la rue (vitres et vitrines, decor.glsl).
// Une pièce à 200 lx (murs ≈ 32 cd/m², plafond ≈ 58) est 4 à 5 IL au-dessus de la chaussée éclairée (1 à 3 cd/m²) : à l'exposition
// de la rue (EV 3 à 5), elle sortait 1,5 à 3 fois au-dessus du blanc (blocs blancs sur toute la façade). L'œil, adapté localement,
// y lit encore les murs et le plafond (Thompson, Shirley et Ferwerda 2002 ; même principe que les lumières artificielles de
// WFS-230 RG-03) : sous le genou (luminance exposée 0,2), rien ne change ; au-dessus, épaule rationnelle (Reinhard) vers 0,6
// (≈ 213 / 255 après ACES : jamais saturé ; rouge ≤ 0,85, à peine au-dessus du genou du bloom à 0,5 : pas de grand halo autour des
// vitrines), continue et de pente 1 au genou ; teinte gardée. Mêmes valeurs dans eclairage_exposition.h.
const float PIECE_GENOU = 0.2, PIECE_PLAFOND_EXPO = 0.6;
vec3 pieceAdaptee(vec3 c) {
    float Y = dot(max(c, vec3(0.0)), vec3(0.2126, 0.7152, 0.0722));
    if (Y <= PIECE_GENOU) return c;
    const float m = PIECE_PLAFOND_EXPO - PIECE_GENOU;
    const float x = Y - PIECE_GENOU;
    return c * ((PIECE_GENOU + m * x / (x + m)) / Y);
}

// WFS-09 : halo des feux et des lampadaires (TEXS.radial du jeu web, mélange additif) ; bloc F_ADD de main
vec4 haloLumiere() {
    // dégradé radial 1 → 0,45 (35 % du rayon) → 0 ; couleur × intensité portée par la normale du sommet
    float r = length(vUv * 2.0 - 1.0);
    float k = r < 0.35 ? mix(1.0, 0.45, r / 0.35) : mix(0.45, 0.0, clamp((r - 0.35) / 0.65, 0.0, 1.0));
    return vec4(max(vNrm, vec3(0.0)) * k * (1.0 - fogAt(vWorld)) * F.iblB.w, 1.0);   // WFS-151 (RG-06) : suit l'exposition
}

// WFS-149 (RG-04) : carte de normales dans le repère tangent des sommets (MikkTSpace, src/render/mesh.h), drapeau
// MAT_TANGENTE seulement (aucun matériau existant ne l'a : image inchangée). Bitangente = signe × N × T (glTF).
vec3 perturbTangente(vec3 N, vec4 t, vec3 tn) {
    vec3 T = normalize(t.xyz - N * dot(N, t.xyz));
    vec3 B = cross(N, T) * (t.w < 0.0 ? -1.0 : 1.0);
    return normalize(mat3(T, B, N) * tn);
}

// ---------------------------------------------------------------- WFS-151 : éclairage d'image (IBL, RG-03 et RG-04)
// irradiance de la sonde : harmoniques sphériques d'ordre 2 (Ramamoorthi et Hanrahan 2001), coefficients déjà convolués
// par le cosinus, divisés par π et pré-exposés sur le CPU (eclairage_physique.cpp) : albédo × iblDiffus(N) = diffus
vec3 iblDiffus(vec3 n) {
    float x = n.x, y = n.y, z = n.z;
    vec3 r = F.iblSH[0].rgb * 0.282095
           + F.iblSH[1].rgb * (0.488603 * y) + F.iblSH[2].rgb * (0.488603 * z) + F.iblSH[3].rgb * (0.488603 * x)
           + F.iblSH[4].rgb * (1.092548 * x * y) + F.iblSH[5].rgb * (1.092548 * y * z) + F.iblSH[6].rgb * (0.315392 * (3.0 * z * z - 1.0))
           + F.iblSH[7].rgb * (1.092548 * x * z) + F.iblSH[8].rgb * (0.546274 * (x * x - y * y));
    return max(r, vec3(0.0));
}
// RG-04 : en mode physique, ambiance (skyAmb) et reflets (env × Fe) viennent de la sonde : cube préfiltré GGX lu au niveau
// de la rugosité, Fresnel intégré par la table BRDF du split-sum (Karis 2013). Plus de facteur d'ambiance ni d'occlusion
// spéculaire arbitraires ; le plafond du Fresnel de la chaussée (WFS-142, RG-04 : jamais de route miroir) est gardé.
void iblRemplacer(inout vec3 skyAmb, inout vec3 env, inout vec3 Fe, inout float specOcc, vec3 N, vec3 R, float NoV, float rough,
                  vec3 f0, uint flags, float flaque) {
    if (F.iblA.x < 0.5) return;
    skyAmb = iblDiffus(N);
    specOcc = 1.0;
    if ((flags & F_VTINT) != 0u) { Fe = vec3(0.0); env = vec3(0.0); return; }   // feuillage : Lambert sans reflet (pas de lecture)   // aussi dans les captures : rebonds de la lumière d'une capture à la suivante
    env = iblPrefiltre(R, clamp(rough, 0.0, 1.0));
    vec2 ab = texture(iblBrdf, vec2(clamp(NoV, 0.0, 1.0), clamp(rough, 0.0, 1.0))).rg;
    Fe = f0 * ab.x + ab.y;
    if ((flags & F_ROAD) != 0u) Fe = min(Fe, vec3(mix(0.25, 0.4, flaque)));
}

// WFS-263 (RG-01) : ambiance et reflet du shader léger (décor au-delà de 60 m) : mêmes formules que le complet (ciel analytique
// sans sonde ; avec sonde : harmoniques sphériques et table BRDF exacte), mais UNE seule lecture de la sonde (le cube dominant
// du mélange A / B) au lieu de trois (skyColor + iblPrefiltre lisent chacun les deux cubes). Seule autre différence avec le
// complet : pas de carte de normales (relief invisible au-delà de 60 m).
void iblLeger(inout vec3 skyAmb, out vec3 env, inout vec3 Fe, inout float specOcc, vec3 N, vec3 R, float NoV, float rough, float sunK, vec3 f0, bool vtint) {
    if (F.iblA.x > 0.5) {
        skyAmb = iblDiffus(N);
        specOcc = 1.0;
        float r = clamp(rough, 0.0, 1.0);
        env = F.iblA.y >= 0.5 ? textureLod(iblCubeB, R, r * F.iblA.w).rgb * F.iblB.z : textureLod(iblCubeA, R, r * F.iblA.z).rgb * F.iblB.y;
        vec2 ab = texture(iblBrdf, vec2(clamp(NoV, 0.0, 1.0), r)).rg;
        Fe = f0 * ab.x + ab.y;
    } else env = mix(skyColor(R, sunK), mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5), clamp(rough * 1.2, 0.0, 1.0)) * F.misc.z;
    if (vtint) { Fe = vec3(0.0); env = vec3(0.0); }   // feuillage : Lambert sans reflet (comme iblRemplacer)
}
