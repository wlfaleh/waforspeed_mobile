// WaForSpeed — WFS-227 (lot 1 de WFS-153, épic E13) : ombres du soleil en CASCADES stabilisées, pénombre physique.
//
// Calcul seul (aucun appel Vulkan) : utilisé par Renderer (matrices des cascades dans FrameUBO, passe d'ombre par tuile de
// l'atlas) et testé par tests/fidelite_lumiere_ombres.cpp.
//   - RG-01 : 4 cascades en Ultra (250 m), 3 en Haute (200 m), 2 en Moyenne (100 m) ; en Basse, pas d'ombre du soleil (comme
//     avant : Reglages, RG-04 aucune perte d'images/s) ; découpage mélangé logarithmique (90 %) / linéaire ;
//   - stabilisées : chaque cascade est une sphère de RAYON FIXE (champ de vision maximal du jeu, format d'image le plus large) ;
//     centre calé sur la grille des texels dans le repère du soleil : une ombre ne « grésille » pas quand la caméra bouge ;
//   - atlas : une seule image (carte d'ombre de WFS-07), découpée en 2 × 2 tuiles (1 cascade : image entière) ;
//   - RG-02 : pénombre physique : largeur = distance occultant → récepteur × tan(0,53°) (diamètre apparent du soleil) ;
//     shaders/eclairage.glsl (sunShadow) estime la distance de l'occultant puis filtre sur cette largeur (même constante).
#pragma once
#include <cmath>
#include "../core/math.h"

namespace wfs::ombres {

constexpr int CASCADES_MAX = 4;
constexpr double DIAMETRE_SOLEIL_DEG = 0.53;   // diamètre apparent moyen du soleil (0,524° à 0,542° selon la saison)
inline double tanSoleil() { return std::tan(DIAMETRE_SOLEIL_DEG * PI / 180.0); }
// M-02 : largeur de la pénombre (m) à une distance d (m) entre l'occultant et le récepteur
inline double penombre(double d) { return d * tanSoleil(); }

constexpr float PROCHE_M = 2.f;              // début du découpage (la voiture est à ≈ 5 m de la caméra)
constexpr float LAMBDA = 0.9f;               // part logarithmique du découpage (1re cascade courte : ombres fines près de la voiture)
constexpr float RECUL_SOLEIL_M = 120.f;      // profondeur ajoutée côté soleil (arbres, immeubles hors du champ qui portent ombre)
constexpr float TAN_DEMI_V_MAX = 0.75f;      // champ vertical maximal (≈ 74° : cockpit 72°, poursuite 62° + 8° à grande vitesse)
constexpr float FORMAT_MAX = 2.2f;           // largeur / hauteur maximale de l'image (téléphone 2340 × 1080)

// nombre de cascades et distance couverte selon le niveau affiché (qualite::Niveau : 0 Basse … 3 Ultra)
inline int nombre(int niveau) { return niveau >= 3 ? 4 : niveau == 2 ? 3 : niveau == 1 ? 2 : 1; }
inline float portee(int niveau) { return niveau >= 3 ? 250.f : niveau == 2 ? 200.f : niveau == 1 ? 100.f : 40.f; }

struct Cascades {
    int n = 1;
    float fin[CASCADES_MAX] = {};      // distance (le long du regard) où finit chaque cascade
    float demi[CASCADES_MAX] = {};     // demi-côté (m) de chaque cascade (rayon de sa sphère)
    vec3 centre[CASCADES_MAX] = {};    // centre (monde) après calage sur la grille
    mat4 m[CASCADES_MAX];              // monde → [-1, 1]² × [0, 1] (profondeur) de la cascade
    mat4 union_;                       // boîte qui contient toutes les cascades (masquage des objets qui portent ombre)
    float profondeur[CASCADES_MAX] = {};   // profondeur couverte par chaque cascade (m) : 2 × demi + RECUL_SOLEIL_M
};

// fin de la cascade i (sur n) entre PROCHE_M et la portée
inline float finCascade(int i, int n, float portee) {
    const float u = (float)(i + 1) / (float)n;
    const float lg = PROCHE_M * std::pow(portee / PROCHE_M, u), li = PROCHE_M + (portee - PROCHE_M) * u;
    return LAMBDA * lg + (1 - LAMBDA) * li;
}
// sphère qui contient la tranche [a, b] du champ de vision (k² = tan²(demi-horizontal) + tan²(demi-vertical)), rayon FIXE
// (champ maximal) : centre à la distance c le long du regard, rayon r
inline void sphereTranche(float a, float b, float& c, float& r) {
    const float k2 = TAN_DEMI_V_MAX * TAN_DEMI_V_MAX * (1 + FORMAT_MAX * FORMAT_MAX);
    c = std::min(b, 0.5f * (a + b) * (1 + k2));
    r = std::sqrt((b - c) * (b - c) + b * b * k2);
    r = std::max(r, std::sqrt((c - a) * (c - a) + a * a * k2));
}

// matrice orthographique du soleil (repère V = lookAt(soleil → origine)) centrée sur c, demi-côté h, calée sur la grille
// des texels (taille de tuile « tuile » texels) ; profondeur : [centre − h − RECUL_SOLEIL_M, centre + h]
inline mat4 orthoSoleil(const mat4& V, const vec3& c, float h, uint32_t tuile, float& profondeur, vec3& centreCale) {
    const vec3 lc = V.transformPoint(c);
    const float texel = 2 * h / (float)tuile;
    const float cx = std::round(lc.x / texel) * texel, cy = std::round(lc.y / texel) * texel;
    profondeur = 2 * h + RECUL_SOLEIL_M;
    const float pres = -lc.z - h - RECUL_SOLEIL_M;   // distance le long du soleil (vue : −z)
    mat4 P{};
    P.at(0, 0) = 1 / h; P.at(0, 3) = -cx / h;
    P.at(1, 1) = 1 / h; P.at(1, 3) = -cy / h;
    P.at(2, 2) = -1 / profondeur; P.at(2, 3) = -pres / profondeur;
    P.at(3, 3) = 1;
    // centre réellement utilisé (monde) : axes x et y du repère du soleil = lignes 0 et 1 de V
    centreCale = c + vec3{V.at(0, 0), V.at(0, 1), V.at(0, 2)} * (cx - lc.x) + vec3{V.at(1, 0), V.at(1, 1), V.at(1, 2)} * (cy - lc.y);
    return P * V;
}

// cascades de l'image : soleil (direction VERS le soleil), caméra (position, regard horizontal ou non), niveau de qualité,
// côté de la carte d'ombre (texels) ; une cascade : boîte de WFS-07 autour de focus (40 m, 20 m devant)
inline Cascades calculer(const vec3& versSoleil, const vec3& camPos, const vec3& regard, int niveau, uint32_t cote) {
    Cascades C;
    C.n = nombre(niveau);
    const vec3 L = normalize(versSoleil), up = std::fabs(L.y) > 0.99f ? vec3{1, 0, 0} : vec3{0, 1, 0};
    const mat4 V = lookAt(L, {0, 0, 0}, up);
    const uint32_t tuile = C.n > 1 ? cote / 2 : cote;
    const float D = portee(niveau);
    vec3 f = regard; if (length(f) < 1e-4f) f = {0, 0, -1}; f = normalize(f);
    float a = 0;
    for (int i = 0; i < C.n; i++) {
        const float b = finCascade(i, C.n, D);
        float c, r; sphereTranche(a, b, c, r);
        C.fin[i] = b; C.demi[i] = r;
        C.m[i] = orthoSoleil(V, camPos + f * c, r, tuile, C.profondeur[i], C.centre[i]);
        a = b;
    }
    float c, r, p; vec3 cc; sphereTranche(0, D, c, r);
    C.union_ = orthoSoleil(V, camPos + f * c, r, cote, p, cc);
    return C;
}

} // namespace wfs::ombres
