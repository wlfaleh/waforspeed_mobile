// WaForSpeed — WFS-324 : place des glaces et projection du miroir plan (fonctions pures, sans Vulkan) : voir retroviseurs.h.
#include <algorithm>
#include <cmath>
#include <cstring>
#include "retroviseurs.h"
#include "vehicule_cockpit.h"

namespace wfs::retro {

namespace {
// glace extérieure par modèle (m, largeur × hauteur du rectangle englobant). Statut : E (estimé sur photos de presse, proportion de
// la glace à la largeur de la portière ; aucune fiche constructeur ouverte par le réseau des agents) ; les glaces de série vont de
// 165 × 85 mm (supercar) à 245 × 150 mm (grand SUV) ; règlement ONU R46 (classe III) : la glace contient un rectangle de 40 mm de
// haut et une ligne de 130 mm (minimum, toujours tenu ici)
struct CoteDef { const char* car; CotesExterieur c; };
const CoteDef COTES[] = {
    {"golf", {0.200f, 0.118f}}, {"m4", {0.190f, 0.108f}}, {"p911", {0.175f, 0.100f}}, {"sf90", {0.170f, 0.088f}},
    {"s3", {0.198f, 0.115f}}, {"giulia", {0.185f, 0.105f}}, {"p208", {0.195f, 0.115f}}, {"gallardo", {0.165f, 0.090f}},
    {"mustang", {0.178f, 0.096f}}, {"model3", {0.200f, 0.120f}}, {"abarth", {0.172f, 0.110f}}, {"compass", {0.215f, 0.135f}},
    {"corsa", {0.195f, 0.115f}}, {"gt86", {0.185f, 0.105f}}, {"rrsport", {0.245f, 0.150f}}, {"tcross", {0.210f, 0.130f}},
    {"v60", {0.210f, 0.125f}}, {"a45", {0.195f, 0.112f}}, {"elantra", {0.198f, 0.115f}}, {"x5", {0.235f, 0.145f}},
};
// repère d'une glace : centre c, normale n (vers le conducteur), x horizontal (à droite vu de l'œil), y = n × x
mat4 repereGlace(vec3 c, vec3 n) {
    const vec3 x = normalize(cross(vec3{0, 1, 0}, n)), y = cross(n, x);
    mat4 R = mat4::identity();
    R.at(0, 0) = x.x; R.at(1, 0) = x.y; R.at(2, 0) = x.z;
    R.at(0, 1) = y.x; R.at(1, 1) = y.y; R.at(2, 1) = y.z;
    R.at(0, 2) = n.x; R.at(1, 2) = n.y; R.at(2, 2) = n.z;
    R.at(0, 3) = c.x; R.at(1, 3) = c.y; R.at(2, 3) = c.z;
    return R;
}
// normale d'une glace plane qui renvoie vers `vise` (direction regardée dans la glace) le regard venu de l'œil : bissectrice
vec3 normaleReglee(vec3 centre, vec3 oeil, vec3 vise) { return normalize(normalize(oeil - centre) + normalize(vise)); }
} // namespace

CotesExterieur cotesExterieur(const char* car) {
    if (car) for (const CoteDef& c : COTES) if (std::strcmp(c.car, car) == 0) return c.c;
    return {0.200f, 0.118f};   // berline compacte (E)
}

void mesurerRetroviseurs(Model& m, const float* xyz, size_t pas, size_t n) {
    m.retroMesure[0] = m.retroMesure[1] = false;
    const float H = m.bmax.y - m.bmin.y, L = m.bmax.z - m.bmin.z, cx = (m.bmin.x + m.bmax.x) * 0.5f, cz = (m.bmin.z + m.bmax.z) * 0.5f;
    if (H <= 0.1f || L <= 0.5f || n == 0) return;
    const float y0 = m.bmin.y + 0.40f * H, y1 = m.bmin.y + 0.90f * H;
    // largeur de l'arrière (moitié arrière, même hauteur) de chaque côté
    float arriere[2] = {0, 0};
    for (size_t i = 0; i < n; i++) {
        const float* p = xyz + i * pas;
        if (p[1] < y0 || p[1] > y1 || p[2] < cz) continue;
        const int c = p[0] < cx ? 0 : 1;
        arriere[c] = std::max(arriere[c], std::fabs(p[0] - cx));
    }
    for (int c = 0; c < 2; c++) {
        if (arriere[c] <= 0.3f) continue;
        vec3 a{1e9f, 1e9f, 1e9f}, b{-1e9f, -1e9f, -1e9f}; size_t vus = 0;
        for (size_t i = 0; i < n; i++) {
            const float* p = xyz + i * pas;
            if (p[1] < y0 || p[1] > y1 || p[2] > cz || p[2] < m.bmin.z + 0.15f * L || (p[0] < cx) != (c == 0)) continue;
            if (std::fabs(p[0] - cx) < arriere[c] + 0.02f) continue;
            a = {std::min(a.x, p[0]), std::min(a.y, p[1]), std::min(a.z, p[2])}; b = {std::max(b.x, p[0]), std::max(b.y, p[1]), std::max(b.z, p[2])};
            vus++;
        }
        // une vraie coque : au moins 20 sommets, 8 à 40 cm de long et de haut (sinon : pli de la carrosserie, poignée : place estimée)
        const vec3 t = b - a;
        if (vus >= 20 && t.z > 0.05f && t.z < 0.45f && t.y > 0.05f && t.y < 0.40f) { m.retroMin[c] = a; m.retroMax[c] = b; m.retroMesure[c] = true; }
    }
}

std::array<Miroir, NB_GLACES> miroirs(const cockpit::InterieurDef& d, float cx, float demi, const Carrosserie& car) {
    const float largeurHorsTout = car.largeur;
    using namespace cockpit;
    std::array<Miroir, NB_GLACES> m{};
    const vec3 oeil{0, OEIL_CAMERA_PLUS_M, 0};   // œil de la caméra au repos (le réglage du conducteur)
    // ---- intérieur (≈ 250 × 70 mm, glace 236 × 56 mm, cadre de 7 mm) : accroché au pare-brise sous la traverse, sur l'axe de la
    // voiture (place de WFS-323, inchangée) ; glace réglée pour voir droit derrière, la lunette arrière au milieu de la glace
    {
        const float yh = HAUT_PARE_BRISE_Y, zh = HAUT_PARE_BRISE_Z, Yb = d.pareBriseY, Zb = d.pareBriseZ;
        const vec3 eu = normalize(vec3{0, yh - Yb, zh - Zb}), en = normalize(cross(vec3{1, 0, 0}, eu));
        const vec3 pied = vec3{cx, Yb, Zb} + eu * (length(vec3{0, yh - Yb, zh - Zb}) - 0.05f) + en * 0.004f;
        const vec3 c{cx - 0.01f, pied.y - 0.062f, pied.z + 0.06f};   // centre de la coque
        const vec3 n = normaleReglee(c, oeil, {0, -std::sin(2.5f * PI / 180.f), std::cos(2.5f * PI / 180.f)});   // lunette arrière : 2,5° sous l'horizontale
        m[INTERIEUR].repere = repereGlace(c + n * 0.0205f, n);
        m[INTERIEUR].w = 0.236f; m[INTERIEUR].h = 0.056f;
        m[INTERIEUR].reflexion = 0.80f;   // glace argentée, position jour (ONU R46 : ≥ 40 %)
        m[INTERIEUR].recul = 0.04f;       // le rétroviseur intérieur du modèle de la voiture, à quelques cm
    }
    // ---- extérieurs : sur l'aile de la portière, à l'angle avant de la vitre latérale (E) : centre de la glace 2,5 cm hors de la
    // carrosserie (la coque dépasse de ≈ 14 cm : largeur avec rétroviseurs ≈ largeur + 0,28 m, Golf 8 : 2,07 m pour 1,79 m),
    // 10 cm au-dessus de la ligne de ceinture, 22 cm derrière le pied du pare-brise (aile de fixation 8 cm derrière lui). Réglage du
    // conducteur : la carrosserie occupe le cinquième intérieur de la glace, l'horizon un peu au-dessus du milieu (regard 1,5° vers le bas)
    const CotesExterieur ce = cotesExterieur(d.car);
    const float W = largeurHorsTout > 0.5f ? largeurHorsTout : 2.f * (demi + 0.17f);
    // œil du siège dans le repère du modèle (camera_vue.cpp : conducteur à gauche, à −cx de l'axe, 2 % de la longueur devant le milieu)
    const vec3 oeilModele{-cx, hauteurOeil(d), -car.longueur * 0.02f};
    for (int k : {GAUCHE, DROITE}) {
        const float sx = k == GAUCHE ? -1.f : 1.f;
        const vec3 c{cx + sx * (W * 0.5f + 0.025f), ligneCeinture(d) + 0.10f, d.pareBriseZ + 0.22f};
        float recul = 0.30f;
        vec3 decal{};
        const int cote = k == GAUCHE ? 0 : 1;
        const bool mesure = car.modele && car.modele->retroMesure[cote];
        if (mesure) {   // glace du modèle : face arrière de sa coque, côté extérieur (la coque déborde de 1,5 cm autour de la glace)
            const vec3 a = car.modele->retroMin[cote] - oeilModele, b = car.modele->retroMax[cote] - oeilModele;
            const float dehors = k == GAUCHE ? a.x : b.x;
            decal = vec3{dehors - sx * (0.015f + ce.w * 0.5f), a.y + 0.55f * (b.y - a.y), b.z - 0.005f} - c;
            recul = std::clamp(b.z - a.z + 0.02f, 0.08f, 0.30f);   // toute la coque du modèle devant le plan proche
        }
        const float D = length(c - oeil), demiAngle = std::atan(ce.w * 0.5f / D);
        // flanc de la voiture vu de la glace : du centre de la glace vers l'arrière de la carrosserie (≈ 2,2 m derrière l'œil)
        const float flanc = std::atan2(std::fabs(c.x - (cx + sx * W * 0.5f)), 2.2f - c.z);
        const float dehors = 0.6f * demiAngle - flanc, bas = 1.5f * PI / 180.f;
        const vec3 vise{sx * std::sin(dehors) * std::cos(bas), -std::sin(bas), std::cos(dehors) * std::cos(bas)};
        m[(size_t)k].repere = repereGlace(c, normaleReglee(c, oeil, vise));
        m[(size_t)k].w = ce.w; m[(size_t)k].h = ce.h;
        m[(size_t)k].reflexion = 0.50f;   // glace chromée (ONU R46 : ≥ 40 % ; glaces bleutées antireflet ≈ 45 à 55 %)
        // plan proche derrière la glace : la coque du rétroviseur du modèle de la voiture ne remplit pas la glace (vu au banc avant la
        // mesure : coque bleue sur 70 % de la glace) ; place estimée : 30 cm (coque du modèle à une place inconnue)
        m[(size_t)k].recul = recul;
        m[(size_t)k].mesure = mesure; m[(size_t)k].decalageVue = decal;
        m[(size_t)k].rayonExterieur = ce.h * 0.45f;
    }
    return m;
}

Fenetre fenetre(const Miroir& m, const mat4& R, bool avecDecalage) {
    const mat4 G = R * (avecDecalage ? translate(m.decalageVue) : mat4::identity()) * m.repere;
    return {G.transformPoint({-m.w * 0.5f, -m.h * 0.5f, 0}), G.transformPoint({m.w * 0.5f, -m.h * 0.5f, 0}),
            G.transformPoint({-m.w * 0.5f, m.h * 0.5f, 0})};
}

mat4 reflexion(vec3 p, vec3 n) {
    mat4 M = mat4::identity();
    const float nn[3] = {n.x, n.y, n.z}, d = dot(p, n);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) M.at(i, j) = (i == j ? 1.f : 0.f) - 2.f * nn[i] * nn[j];
        M.at(i, 3) = 2.f * d * nn[i];
    }
    return M;
}

bool vueProjMiroir(const Fenetre& f, vec3 oeil, float recul, mat4& vp, vec3& oeilReflet) {
    const vec3 vr = normalize(f.pb - f.pa), vu = normalize(f.pc - f.pa), vn = normalize(cross(vr, vu));
    const vec3 va = f.pa - oeil, vb = f.pb - oeil, vc = f.pc - oeil;
    const float dist = -dot(va, vn);   // distance de l'œil au plan de la glace (> 0 : œil devant la glace)
    const mat4 M = reflexion(f.pa, vn);
    oeilReflet = M.transformPoint(oeil);
    if (dist < 1e-3f) return false;   // œil derrière la glace : rien à voir
    const float n = dist + std::max(0.f, recul), s = n / dist;
    const float l = dot(vr, va) * s, r = dot(vr, vb) * s, b = dot(vu, va) * s, t = dot(vu, vc) * s;
    mat4 P{};
    P.at(0, 0) = 2 * n / (r - l); P.at(0, 2) = (r + l) / (r - l);
    P.at(1, 1) = -2 * n / (t - b); P.at(1, 2) = -(t + b) / (t - b);   // y de l'écran vers le bas (perspectiveVk)
    P.at(2, 3) = n;                                                   // profondeur inversée, plan lointain à l'infini
    P.at(3, 2) = -1;
    mat4 V = mat4::identity();
    const vec3 ax[3] = {vr, vu, vn};
    for (int i = 0; i < 3; i++) { V.at(i, 0) = ax[i].x; V.at(i, 1) = ax[i].y; V.at(i, 2) = ax[i].z; V.at(i, 3) = -dot(ax[i], oeil); }
    vp = P * V * M;
    return true;
}

} // namespace wfs::retro
