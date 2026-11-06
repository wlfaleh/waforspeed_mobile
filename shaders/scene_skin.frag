#version 450
// WaForSpeed — personnages (WFS-72) : copie de scene.frag (même éclairage PBR, ombres, brouillard, projecteurs)
// + vêtements peints de pedMaterial du jeu web (js/world/people.js l. 54-110), repris tels quels : bloc « WFS-72 » plus bas.
// Toute modification de scene.frag doit être reportée ici (seuls les blocs marqués WFS-72 diffèrent).
#include "common.glsl"
layout(location = 0) in vec3 vWorld;
layout(location = 1) in vec3 vNrm;
layout(location = 2) in vec2 vUv;
layout(location = 3) in vec4 vCloth;   // WFS-72 : _cloth (bras, hauteur, jambe, partie)
layout(location = 4) in vec3 vBind;    // WFS-72 : position en pose de repos (col rond)
// WFS-72 : palette d'os puis apparence de l'instance : look[0..5] = peau, cheveux, haut, bas, chaussures, maillot (rgb,
// codes sRGB ; look[0].w = 1 : apparence active), look[6] = uCloth, look[7] = uBody
layout(set = 2, binding = 0) uniform Os { mat4 os[64]; vec4 look[8]; } B;
layout(set = 1, binding = 0) uniform sampler2D texColor;
layout(set = 1, binding = 1) uniform sampler2D texNormal;
layout(set = 1, binding = 2) uniform sampler2D texMR;
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;
const uint F_PAINT = 1u, F_BLEND = 2u, F_TAIL = 4u, F_HEAD = 8u, F_MASK = 32u, F_GLASS = 64u,
           F_ROAD = 256u, F_WALK = 512u, F_GROUND = 1024u, F_SHADOW = 2048u, F_HASNORMAL = 4096u, F_HASMR = 8192u, F_VTINT = 16384u,
           F_POOL = 32768u, F_ADD = 65536u,   // WFS-09 : tache des phares (multiplicative), halo (additif)
           F_WHEEL = 128u, F_FANTOME = 131072u, F_EMTEX = 262144u;   // WFS-134 : jantes et pneus (ambiante minimale), fantôme du record (translucide)

// WFS-149 (RG-05) : code découpé par domaine (déplacé tel quel) : une épic Réalisme par fichier
#include "eclairage.glsl"   // E13 Lumière et image
#include "route.glsl"       // E9 Routes
#include "decor.glsl"       // E12 Environnements
#include "vehicule.glsl"    // E10 Véhicules

void main() {
    uint flags = uint(pc.params.w + 0.5);
    // finesse de la pièce (WFS-42) : seulement pour les voitures (emissive.w sert à la demi-largeur de route sinon)
    bool fine = pc.emissive.w > 0.0 && (flags & (F_ROAD | F_WALK | F_GROUND)) == 0u;
    // WFS-134 (RG-12) : fondu entre la voiture complète et sa version allégée (56 à 64 m) : tramage écran 4×4 complémentaire
    // (emissive.w = −(1 + t) : pixel gardé si seuil < t ; −(3 + t) : gardé si seuil ≥ t)
    if (pc.emissive.w < -0.5 && (flags & (F_ROAD | F_WALK | F_GROUND)) == 0u) {
        fonduLod();
    }
    // WFS-16 : passe de reflet (vue miroir de la chaussée) : rien de ce qui est sous le plan
    if (F.nuitA.w > 0.5 && vWorld.y < F.nuitB.x - 0.05) discard;
    if ((flags & F_ADD) != 0u) {   // WFS-09 : halo des feux et des lampadaires (TEXS.radial du jeu web, mélange additif)
        outColor = haloLumiere();
        return;
    }
    if ((flags & F_POOL) != 0u) {   // WFS-09 : tache des phares sur la route (beamTex('pool') de js/cars/models.js)
        outColor = tachePhares();
        return;
    }
    if ((flags & F_SHADOW) != 0u) {   // ombre de contact : rectangle aux bords très doux
        outColor = ombreContact();
        return;
    }
    vec3 N = normalize(vNrm);
    vec3 V = normalize(F.camPos.xyz - vWorld);
    bool vtint = (flags & F_VTINT) != 0u;   // végétation (WFS-34) : |normale| = luminosité, même éclairage des deux faces
    // pièces double face. Route, trottoirs et bas-côtés : normales déjà orientées vers le haut à la construction, mais
    // triangles enroulés dans l'autre sens → ne pas retourner (sinon le sol ne recevait ni soleil ni ombre, WFS-07)
    // passe de reflet (WFS-16) : la vue miroir inverse le sens des faces
    bool face = gl_FrontFacing != (F.nuitA.w > 0.5);
    if (!face && !vtint && (flags & (F_ROAD | F_WALK | F_GROUND)) == 0u) N = -N;
    vec4 base = texture(texColor, vUv) * pc.color;
    // WFS-100 (RG-03, ANO-28) : carrosserie = couleur de peinture choisie × 1, sans la texture de couleur du modèle
    // (js/cars/models.js : color.set(paint), carte de couleur retirée) ; vitres, feux, jantes et logos gardent leur texture
    if ((flags & F_PAINT) != 0u) base = pc.color;
    teinteVegetation(base, vtint, flags);
    float metal = pc.params.x, rough = pc.params.y;
    if ((flags & F_HASMR) != 0u) { vec4 mr = texture(texMR, vUv); rough *= mr.g; metal *= mr.b; }
    if ((flags & F_HASNORMAL) != 0u) N = perturb(N, vWorld, vUv, normalize(texture(texNormal, vUv).xyz * 2.0 - 1.0));
    if (B.look[0].w > 0.5) {   // ---------- WFS-72 : pedMaterial du jeu web (map_fragment, roughness, normal) ----------
        // le jeu web (three r128 sans décodage sRGB) travaille sur les codes sRGB de la texture et des couleurs : idem ici
        vec3 uSkin = B.look[0].rgb, uHair = B.look[1].rgb, uTop = B.look[2].rgb, uBottom = B.look[3].rgb,
             uShoe = B.look[4].rgb, uSwim = B.look[5].rgb;
        vec4 uCloth = B.look[6], uBody = B.look[7];
        vec3 tex = pow(max(base.rgb, vec3(0.0)), vec3(1.0 / 2.2)), col = tex;
        float part = vCloth.w, cm = 0.0;
        float lum = dot(tex, vec3(0.299, 0.587, 0.114));
        if (part < 0.5) {
            // corps : la peau garde la texture (visage, mains), teinte selon la personne
            float sat = max(tex.r, max(tex.g, tex.b)) - min(tex.r, min(tex.g, tex.b));
            float under = 1.0 - smoothstep(0.05, 0.13, sat);            // sous-vêtements gris du modèle d'origine
            vec3 skin = tex * uSkin;
            float arm = vCloth.x, y = vCloth.y, leg = vCloth.z;
            float isArm = step(-0.5, arm), isLeg = step(-0.5, leg), dressed = uCloth.z;
            // col rond : on découvre le cou (distance à l'axe du cou, en pose de repos)
            float hd = length(vec2(vBind.x, (vBind.z - uBody.z) * 1.15)), ny = y - uBody.x;
            float collar = 1.0 - step(0.0, ny + 0.05) * step(hd, 0.085 + max(0.0, -ny) * 0.4);
            float top = dressed * collar * step(ny, 0.05) * max((1.0 - isArm) * (1.0 - isLeg) * step(uBody.y, y), isArm * step(arm, uCloth.x));
            float bot = dressed * (1.0 - isArm) * step(y, uBody.y + 0.001) * (1.0 - isLeg * step(uCloth.y, leg));
            float shoe = isLeg * step(1.93, leg);
            vec3 baseC = mix(skin, uSwim * (0.25 + lum * 1.6), under * (1.0 - dressed));
            // vêtements : couleur unie, un peu du relief de la texture pour les plis et l'ombre
            float shade = mix(1.0, clamp(lum / 0.5, 0.55, 1.2), 0.3);
            float hem = 1.0 - 0.25 * (1.0 - smoothstep(0.0, 0.035, abs(arm - uCloth.x))) * isArm * dressed;
            vec3 cloth = shoe > 0.5 ? uShoe : (top > 0.5 ? uTop : uBottom);
            cm = max(max(top, bot), shoe);
            col = mix(baseC, cloth * shade * hem, cm);
        } else if (part > 1.5) {
            col = uHair * (0.35 + lum * 1.25);                          // sourcils et cheveux : texture grise teintée
        }
        base.rgb = pow(max(col, vec3(0.0)), vec3(2.2));
        rough = part < 0.5 ? mix(0.52, 0.9, cm) : (part < 1.5 ? 0.15 : 0.72);
        vec3 geo = normalize(vNrm) * (gl_FrontFacing ? 1.0 : -1.0);
        N = normalize(mix(N, geo, cm * 0.85));                            // tissus : pas les muscles du modèle nu
    }
    if ((flags & F_MASK) != 0u && base.a < pc.params.z) discard;

    // ---------- surfaces procédurales (uv = mètres : x latéral, y le long de la route) ----------
    bool wetRoad = false;   // WFS-16 : chaussée mouillée ; WFS-142 (RG-01) : seulement quand il pleut
    float flaque = 0.0;     // WFS-142 : part de flaque d'eau au pixel (0 à 1)
    if ((flags & F_ROAD) != 0u) {
        surfaceRoute(base, rough, metal, N, wetRoad, flaque);
    } else if ((flags & F_WALK) != 0u) {
        surfaceTrottoir(base, rough, metal);
    } else if ((flags & F_GROUND) != 0u) {
        surfaceSol(base, rough, metal);
    }

    vec3 L = normalize(F.sunDir.xyz);
    vec3 H = normalize(V + L);
    float NoL = max(dot(N, L), 0.0), NoV = max(dot(N, V), 1e-3), NoH = max(dot(N, H), 0.0), VoH = max(dot(V, H), 0.0);
    // anticrénelage des reflets (Tokuyoshi et Kaplanyan 2019, WFS-32) : là où la normale change beaucoup d'un pixel à
    // l'autre (arêtes, petits triangles vus en biais), le reflet est un peu élargi : plus d'éclat isolé sur un pixel.
    // Les grandes surfaces (normale régulière) gardent leur reflet net.
    vec3 dndx = dFdx(N), dndy = dFdy(N);
    float aaKernel = min(2.0 * 0.25 * (dot(dndx, dndx) + dot(dndy, dndy)), 0.18);
    // pièces très fines (WFS-42 : fils de la lunette, rayons de jante) : triangles plus petits qu'un pixel, que les
    // dérivées ne voient pas. WFS-100 (RG-01, ANO-21) : plus d'élargissement du reflet (il étalait le soleil en traits
    // brillants dans le vide) ; les opaques reçoivent le même facteur de couverture du pixel que les transparents
    float detailK = 1.0;   // part du pixel réellement couverte par la pièce (1 = pièce plus large qu'un pixel)
    if (fine) detailK = clamp(pc.emissive.w / max(length(fwidth(vWorld)), 1e-6), 0.0, 1.0);
    // opaque : pas d'alpha → les reflets (soleil, ciel, vernis) sont atténués par la couverture ; transparent : alpha × couverture
    float specK = (flags & F_BLEND) == 0u ? detailK : 1.0;
    float a = sqrt(max(rough * rough, 0.002) * max(rough * rough, 0.002) + aaKernel);
    vec3 albedo = base.rgb;
    vec3 f0 = mix(vec3(0.04), albedo, metal);
    if (wetRoad) f0 = vec3(mix(0.04, 0.02, flaque));   // WFS-142 (RG-03) : eau des flaques f0 = 0,02
    vec3 Fs = F_Schlick(f0, VoH);
    vec3 spec = vtint ? vec3(0.0) : D_GGX(NoH, a) * V_Smith(NoV, NoL, a) * Fs;   // feuillage : Lambert sans reflet (jeu web)
    vec3 diff = (1.0 - Fs) * (1.0 - metal) * albedo / PI;
    vec3 sun = F.sunColor.rgb * F.sunDir.w;
    float ombre = sunShadow(vWorld, N);
    vec3 col = diff * sun * NoL * ombre + min(spec * sun * NoL * ombre, vec3(SPEC_MAX)) * specK;   // WFS-100 RG-01 / RG-02
    col += spotLights(vWorld, N, V, albedo, f0, metal, a, vtint);   // WFS-45 (aucun si spotN = 0)

    // lumière ambiante : ciel (hémisphère) + reflets de l'environnement
    vec3 R = reflect(-V, N);
    vec3 skyAmb = mix(F.groundCol.rgb, F.skyTop.rgb, N.y * 0.5 + 0.5) * F.misc.z;
    // WFS-45 : HemisphereLight de three (irradiance mix(bas, haut, 0,5·N.y + 0,5), × PI puis Lambert 1/PI → albedo × mix)
    if (F.hemiSky.w > 0.5) skyAmb = mix(F.hemiGround.rgb, F.hemiSky.rgb, N.y * 0.5 + 0.5);
    vec3 Fe = f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - NoV, 5.0);
    if (vtint) Fe = vec3(0.0);   // pas de reflet du ciel sur les cartes vues en rasant ou de dos
    // WFS-142 (RG-04) : chaussée : Fresnel plafonné à 0,25 (0,4 dans les flaques), même en vue rasante (caméra basse de course :
    // NoV ≈ 0,05 au loin donnait Fe ≈ 0,78, un reflet presque total sur toute la route lointaine)
    if ((flags & F_ROAD) != 0u) Fe = min(Fe, vec3(mix(0.25, 0.4, flaque)));
    // le disque du soleil ne se reflète nettement que sur les surfaces lisses (sinon tache blanche sur la route)
    // pièces fines (WFS-42) : le ciel reflété, et surtout le disque du soleil (60 x l'intensité), suivent la rugosité
    // effective (élargie par l'anticrénelage) : sinon chaque petite facette en miroir renvoie le soleil → points brillants
    float roughEnv = fine ? max(rough, sqrt(a)) : rough;
    float sunK = pow(1.0 - clamp(roughEnv, 0.0, 1.0), 6.0);
    vec3 env = mix(skyColor(R, sunK), mix(F.skyHorizon.rgb, F.skyTop.rgb, 0.5), clamp(roughEnv * 1.2, 0.0, 1.0)) * F.misc.z;
    // WFS-134 (ANO-67) : carrosserie : sous l'horizon, reflet de la chaussée sombre plutôt que du sol clair du ciel
    if ((flags & F_PAINT) != 0u) env = mix(env, F.groundCol.rgb * 0.3 * F.misc.z, smoothstep(0.0, -0.3, R.y));
    float specOcc = mix(1.0, 0.18, clamp(rough, 0.0, 1.0));   // les surfaces rugueuses reflètent peu le ciel
    iblRemplacer(skyAmb, env, Fe, specOcc, N, R, NoV, roughEnv, f0, flags, flaque);   // WFS-151 (RG-04) : sonde IBL
    if (wetRoad && F.nuitA.z > 0.5) {
        // WFS-16 : reflet temps réel de la scène (passe miroir, demi-résolution), au même point de l'écran.
        // WFS-142 (RG-04) : il ne REMPLACE plus le reflet du ciel (avant : env = reflet, l'image nette de la rue sur toute la
        // chaussée) : il s'y mélange, au plus 0,35 (0,45 dans les flaques) × la force réglée (configgame), et moitié moins au
        // loin (20 à 80 m). Étalement vertical 8 % de la hauteur d'écran (2 % dans les flaques) au lieu de ≈ 1 % : traînées
        // floues des lumières, aucun contour d'immeuble lisible. 12 lectures en tente, décalées d'un pixel à l'autre (bruit
        // fixe à l'écran, aucun scintillement d'une image à l'autre) pour ne pas voir 12 copies.
        vec2 suv = gl_FragCoord.xy * F.nuitB.yz + N.xz * 0.01;
        float spread = mix(0.08, 0.02, flaque);
        float j0 = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));   // bruit entrelacé (IGN)
        vec3 rs = vec3(0.0); float ws = 0.0;
        for (int i = 0; i < 12; i++) {
            float t = (float(i) + j0) / 12.0 - 0.5;                    // −0,5 .. 0,5
            float wt = 1.0 - abs(t) * 1.6;                              // tente : le centre compte plus
            vec2 o = vec2((fract(float(i) * 0.618) - 0.5) * spread * 0.15, t * spread);
            rs += texture(reflTex, suv + o).rgb * wt; ws += wt;
        }
        rs /= ws;
        float dist = length(F.camPos.xyz - vWorld);
        float wR = clamp(max(F.nuitB.w, 0.0) * (1.0 - rough), 0.0, mix(0.35, 0.45, flaque)) * mix(1.0, 0.5, smoothstep(20.0, 80.0, dist));
        env = mix(env, rs, wR);
    }
    float kMesure = ((flags & F_ROAD) != 0u && F.nuitB.w < 0.0) ? 0.0 : 1.0;   // WFS-142 (RG-08) : WFS_REFLETS=0, route sans reflet
    col += albedo * (1.0 - metal) * skyAmb + min(env * Fe * F.misc.w * specOcc, vec3(SPEC_MAX)) * specK * kMesure;   // WFS-100 RG-01 / RG-02

    // peinture : couche de vernis brillante par-dessus (reflets nets du ciel et du soleil)
    if ((flags & F_PAINT) != 0u) {
        vernisPeinture(col, R, sun, NoV, NoH, NoL, aaKernel, specK);
    }
    float alpha = base.a * detailK;   // pièce transparente plus fine qu'un pixel : couverture partielle (WFS-42)
    if ((flags & F_GLASS) != 0u) {
        vitre(col, alpha, albedo, skyAmb, R, sun, NoV, NoH, NoL);
    }
    col += pc.emissive.rgb * (((flags & F_EMTEX) != 0u) ? base.rgb : vec3(1.0)) * F.iblB.w;   // WFS-134 : émission × texture ; WFS-151 (RG-06)
    // WFS-134 (RG-05) : jantes et pneus jamais noyés dans le noir (ambiante minimale 0,08)
    if ((flags & F_WHEEL) != 0u) col = max(col, albedo * 0.08);

    // brouillard (perspective atmosphérique)
    float d = length(F.camPos.xyz - vWorld);
    float fogK = 1.0 - exp(-d * F.fog.a);
    // WFS-45 : brouillard linéaire de three r128 (fog_fragment : smoothstep(near, far, profondeur de vue))
    if (F.fogLin.y > 0.0) fogK = smoothstep(F.fogLin.x, F.fogLin.y, dot(vWorld - F.camPos.xyz, F.camFwd.xyz));
    col = mix(col, F.fog.rgb, fogK);
    // pas de pixel aberrant (WFS-32) : un reflet du soleil sur un minuscule triangle du vernis peut donner une valeur
    // énorme, voire NaN (normale nulle) ; on plafonne à 64 et on remplace NaN / infini par 0
    col = clamp(col, vec3(0.0), vec3(64.0));
    if (any(isnan(col)) || any(isinf(col))) col = vec3(0.0);
    // WFS-134 (RG-07, ANO-60) : fantôme du record : teinte cyan #00E5FF à 30 %, translucide (α 0,45), un peu lumineux
    // pour rester visible de nuit
    if ((flags & F_FANTOME) != 0u) {
        outColor = fantome(col, alpha);
        return;
    }
    outColor = vec4(col, ((flags & F_BLEND) != 0u) ? alpha : 1.0);
}
