// WaForSpeed — WFS-325 (lot 1) : pièces propres au modèle de la Golf 8 GTI, de la M4, de la 911 (992.1) et de la SF90 : voir
// vehicule_cockpit_details.h. Toutes les pièces sont modelées (arrondis, chanfreins, lunettes, lames, cannelures : règle de
// l'utilisateur du 2026-11-09 19:41, aucune forme nue) avec les formes de vehicule_cockpit_formes.h et la pièce tournée tour().
// Repère de l'œil : x à droite, y en haut, −z devant ; le conducteur est à gauche, le centre de la voiture en x = cx.
#include "vehicule_cockpit_details.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "vehicule_cockpit_formes.h"

namespace wfs::cockpit {

namespace {
using namespace formes;
using M = MatDetail;

// ---------------------------------------------------------------- formes de base
// pièce tournée (molette, bouton rond, bague, pommeau) : profil (rayon, hauteur) tourné autour de l'axe y du repère, en n pas.
// Le profil se parcourt « matière à gauche » : la face visible est à droite du sens de parcours (normale (dy, −dr) du plan r, y) ;
// un point de rayon nul ferme la pièce. Bande cannelée (molette) : `stries` cannelures entre les hauteurs y0 et y1, creux = part
// du rayon ; normales calculées sur la surface (cannelures comprises), orientées comme le profil
void tour(Geo& g, const mat4& Mx, const std::vector<vec2>& prof, int n, int stries = 0, float creux = 0, float y0 = 0, float y1 = 0) {
    const size_t K = prof.size();
    if (K < 2) return;
    auto rayon = [&](float r, float y, float a) {
        if (stries > 0 && y > y0 && y < y1) return r * (1 - creux * (0.5f + 0.5f * std::cos(stries * a)));
        return r;
    };
    auto point = [&](size_t k, float a) { const float r = rayon(prof[k].x, prof[k].y, a); return vec3{r * std::cos(a), prof[k].y, r * std::sin(a)}; };
    const uint32_t b = (uint32_t)g.v.size();
    for (int i = 0; i <= n; i++) {
        const float a = 2 * PI * i / n;
        for (size_t k = 0; k < K; k++) {
            // normale du profil (moyenne des deux segments voisins), puis normale de la surface (différences finies) orientée pareil
            const vec2 pa = prof[k > 0 ? k - 1 : k], pb = prof[std::min(k + 1, K - 1)];
            const vec2 dp{pb.x - pa.x, pb.y - pa.y};
            const vec2 np{dp.y, -dp.x};
            vec3 nA = normalize(vec3{np.x * std::cos(a), np.y, np.x * std::sin(a)});
            const float e = 0.002f;
            const vec3 dA = point(k, a + e) - point(k, a - e), dT = point(std::min(k + 1, K - 1), a) - point(k > 0 ? k - 1 : k, a);
            vec3 nS = cross(dT, dA);
            if (length(nS) > 1e-9f && prof[k].x > 1e-5f) { nS = normalize(nS); if (dot(nS, nA) < 0) nS = nS * -1.f; } else nS = nA;
            ajouterUV(g, Mx, point(k, a), nS, a * std::max(prof[k].x, 0.005f) * GRAIN, prof[k].y * GRAIN);
        }
    }
    for (int i = 0; i < n; i++) for (size_t k = 0; k + 1 < K; k++) {
        const uint32_t p0 = b + (uint32_t)(i * K + k), p1 = b + (uint32_t)((i + 1) * K + k);
        quadOriente(g, p0, p1, p1 + 1, p0 + 1);
    }
}

// orientation d'une pièce posée sur la face verticale de la planche (face +z vers le conducteur), légèrement levée vers l'œil
mat4 surFace(float x, float y, float z, float leve = 0) { return translate({x, y, z}) * rotateX(-leve); }

// ---------- aérateurs
// aérateur rectangulaire : lunette moulée (cadre), conduit sombre (parois et fond), lames profilées inclinées vers l'œil, curseur
// d'orientation au milieu, molette de fermeture (BMW) ; module de 16 mm en saillie de la face (la face de la planche n'est pas percée) ;
// lames en plastique grainé gris foncé devant un conduit noir (comme les vraies : sans ce contraste, elles ne se voyaient pas)
struct Aer { float w, h, r, ep; int lames; bool verticales, curseur, molette; M cadre; };
constexpr float AER_PROF = 0.016f;
void aerateur(PiecesDetails& p, const mat4& A, const Aer& a) {
    cadreArrondi(p[a.cadre], A, a.w, a.h, a.r, a.ep, AER_PROF);
    const float wi = a.w - 2 * a.ep, hi = a.h - 2 * a.ep, ri = std::max(0.002f, a.r - a.ep);
    cadreArrondi(p[M::Lames], A * translate({0, 0, -0.002f}), wi + 0.001f, hi + 0.001f, ri, 0.002f, AER_PROF - 0.003f);   // parois du conduit
    dalleFace(p[M::Lames], A * translate({0, 0, -AER_PROF + 0.002f}), wi, hi, 0.002f, ri, 0.0006f);                     // fond du conduit
    const int n = std::max(2, a.lames);
    for (int k = 0; k < n; k++) {
        if (!a.verticales) {
            const float y = -hi / 2 + hi * (k + 0.5f) / n, lh = hi / n * 0.62f;
            dalleFace(p[M::PlancheBas], A * translate({0, y, -0.007f}) * rotateX(0.50f), wi - 0.002f, 0.0030f, lh, 0.0014f, 0.0007f);
        } else {
            const float x = -wi / 2 + wi * (k + 0.5f) / n, lw = wi / n * 0.62f;
            dalleFace(p[M::PlancheBas], A * translate({x, 0, -0.007f}) * rotateY(-0.30f), 0.0030f, hi - 0.002f, lw, 0.0014f, 0.0007f);
        }
    }
    // curseur d'orientation : petite pièce moulée devant les lames, au milieu (aérateurs VW, BMW, Porsche)
    if (a.curseur) dalleFace(p[a.cadre], A * translate({0, 0, -0.0015f}), a.verticales ? 0.010f : 0.018f, a.verticales ? 0.018f : 0.010f, 0.006f, 0.0042f, 0.0016f);
    // molette de fermeture (BMW) : molette crantée dans la lunette du bas, à droite, axe le long de x
    if (a.molette) {
        const std::vector<vec2> m{{0.f, -0.006f}, {0.0055f, -0.006f}, {0.007f, -0.0045f}, {0.007f, 0.0045f}, {0.0055f, 0.006f}, {0.f, 0.006f}};
        tour(p[M::Lames], A * translate({a.w * 0.28f, -a.h / 2 + a.ep * 0.5f, -0.003f}) * rotateZ(-PI / 2), m, 40, 20, 0.12f, -0.0044f, 0.0044f);
    }
}
// aérateur rond « turbine » (SF90) : bague moulée en saillie, conduit sombre, moyeu bombé et 6 aubes vrillées
void aerateurRond(PiecesDetails& p, const mat4& A, float D, M bague) {
    const float R = D * 0.5f, Ri = R - 0.009f, pr = 0.018f;
    const mat4 T = A * rotateX(PI / 2);   // axe du tour (y) vers +z : face vers le conducteur
    // bague : de l'arrière extérieur vers l'avant, lèvre arrondie, puis l'intérieur (matière à gauche du parcours)
    tour(p[bague], T, {{R, -pr}, {R, -0.003f}, {R - 0.0008f, -0.0008f}, {R - 0.003f, 0.0015f}, {Ri + 0.004f, 0.0015f}, {Ri + 0.0012f, 0.f},
                       {Ri, -0.002f}}, 64);
    tour(p[M::Lames], T, {{Ri, -0.002f}, {Ri - 0.0005f, -pr + 0.003f}, {Ri - 0.004f, -pr + 0.002f}, {0.f, -pr + 0.002f}}, 48);   // conduit et fond
    const float rh = Ri * 0.30f;
    tour(p[bague], T, {{rh, -0.012f}, {rh, -0.004f}, {rh * 0.85f, 0.f}, {rh * 0.5f, 0.0016f}, {0.f, 0.0021f}}, 40);              // moyeu
    for (int k = 0; k < 6; k++) {
        const float a = k * PI / 3 + PI / 6, l = Ri - rh - 0.001f;
        dalleFace(p[M::PlancheBas], A * rotateZ(a) * translate({rh + l * 0.5f, 0, -0.008f}) * rotateX(0.45f), l, 0.0030f, 0.011f, 0.0014f, 0.0007f);
    }
}
// aérateur en stade (911) : même aérateur, coins au demi-côté
Aer stade(float w, float h, int lames, M cadre) { return {w, h, h * 0.5f, 0.007f, lames, false, true, false, cadre}; }

// ---------- commandes
// rangée de n touches moulées (w × h, pas `pas`), en relief de 3 mm sur leur panneau
void touches(PiecesDetails& p, const mat4& A, int n, float w, float h, float pas, M m) {
    for (int k = 0; k < n; k++)
        dalleFace(p[m], A * translate({(k - (n - 1) * 0.5f) * pas, 0, 0.0015f}), w, h, 0.003f, std::min(w, h) * 0.28f, 0.0009f);
}
// interrupteur à bascule (Porsche, BMW) : embase sombre et levier profilé incliné vers le haut
void bascule(PiecesDetails& p, const mat4& A, M levier) {
    dalleFace(p[M::Lames], A, 0.015f, 0.026f, 0.004f, 0.0045f, 0.0012f);
    profile(p[levier], A, {0, -0.003f, 0.001f}, {0, 0.005f, 0.021f}, 0.010f, 0.0055f, 0.0027f, 0.0045f, 0.0085f);
}
// bouton rond tourné (démarreur, sélecteur rotatif) : bague, corps, face légèrement bombée ; axe +z du repère A
void boutonRond(PiecesDetails& p, const mat4& A, float r, float h, M bague, M corps) {
    const mat4 T = A * rotateX(PI / 2);
    tour(p[bague], T, {{r + 0.0035f, 0.f}, {r + 0.0035f, 0.0025f}, {r + 0.0022f, 0.0040f}, {r + 0.0004f, 0.0040f}, {r + 0.0004f, 0.0015f}}, 48);
    tour(p[corps], T, {{r, 0.f}, {r, h - 0.002f}, {r - 0.0015f, h - 0.0002f}, {r * 0.5f, h + 0.0008f}, {0.f, h + 0.001f}}, 48);
}
// commande rotative moletée (iDrive de la M4, molette du 911) : base chromée, flanc cannelé, dessus bombé
void molette(PiecesDetails& p, const mat4& A, float r, float h, M base, M corps) {
    const mat4 T = A * rotateX(PI / 2);
    tour(p[base], T, {{r + 0.004f, 0.f}, {r + 0.004f, 0.002f}, {r + 0.0025f, 0.0035f}, {r + 0.0005f, 0.0035f}}, 64);
    tour(p[corps], T, {{r, 0.0035f}, {r, h - 0.004f}, {r - 0.0015f, h - 0.0012f}, {r * 0.72f, h}, {0.f, h + 0.0015f}}, 120, 60, 0.05f, 0.005f, h - 0.0045f);
}

// ---------- console centrale : repère de son dessus (x à droite, y en haut, z vers l'arrière), centre du dessus en (0, 0, 0)
struct Console { float cx, yDessus, z0, z1, w, pente; };
// pente : le dessus remonte vers l'arrière (rotateX d'un angle négatif : +z monte)
mat4 repereConsole(const Console& c) { return translate({c.cx, c.yDessus, (c.z0 + c.z1) * 0.5f}) * rotateX(-c.pente); }
// corps de la console (tunnel compris, jusqu'au plancher) : bloc moulé à arêtes arrondies, et panneau du bas de la planche qui le
// raccorde à la face de la planche (centre du bas de la planche)
void corpsConsole(PiecesDetails& p, const InterieurDef& d, const Console& c, M matiere) {
    const float e = c.yDessus - PLANCHER_Y - 0.03f, L = c.z1 - c.z0;   // 3 cm au-dessus du plancher : la pente ne le fait pas traverser
    dalle(p[matiere], repereConsole(c) * translate({0, -e * 0.5f, 0}), c.w, L, e, 0.045f, 0.02f, 0.97f);
    // panneau du bas de la planche (devant les genoux, au milieu) : de la face de la planche au dessus de la console
    const float zF = zFacePlanche(d), yF = d.plancheY - 0.42f;
    dalleFace(p[M::PlancheBas], translate({c.cx, (yF + c.yDessus) * 0.5f, zF + 0.006f}), c.w + 0.02f, yF - c.yDessus + 0.05f, 0.012f, 0.02f, 0.006f);
}
// accoudoir rembourré (cuir), sur le dessus de la console, de za à zb (repère de la console)
void accoudoir(PiecesDetails& p, const Console& c, float za, float zb, M m) {
    dalle(p[m], repereConsole(c) * translate({0, 0.035f, (za + zb) * 0.5f}), c.w - 0.012f, zb - za, 0.06f, 0.035f, 0.018f, 0.96f);
}
// bac de rangement (chargeur sans fil) : cadre noir laqué et fond caoutchouté, de za à zb
void bac(PiecesDetails& p, const Console& c, float za, float zb, float w) {
    const mat4 C = repereConsole(c);
    dalle(p[M::Laque], C * translate({0, 0.004f, (za + zb) * 0.5f}), w, zb - za, 0.008f, 0.02f, 0.003f);
    dalle(p[M::Lames], C * translate({0, 0.009f, (za + zb) * 0.5f}), w - 0.02f, zb - za - 0.02f, 0.003f, 0.012f, 0.001f);
}
// deux porte-gobelets côte à côte (cadre moulé, fond sombre), centrés en z
void gobelets(PiecesDetails& p, const Console& c, float z, M cadre) {
    const mat4 C = repereConsole(c);
    for (float s : {-1.f, 1.f}) {
        const mat4 G = C * translate({s * 0.047f, 0.006f, z}) * rotateX(-PI / 2);   // face +z du cadre vers le haut
        cadreArrondi(p[cadre], G, 0.080f, 0.080f, 0.032f, 0.008f, 0.010f);
        dalleFace(p[M::Lames], G * translate({0, 0, -0.008f}), 0.066f, 0.066f, 0.002f, 0.026f, 0.0006f);
    }
}

// ---------- portières : repère de la face intérieure (x le long de la voiture, y en haut, +z vers l'habitacle)
struct Porte {
    float accY, accZ0, accZ1, accW;   // accoudoir : dessus (sous la ligne de ceinture), avant, arrière, saillie
    float poigY, poigZ;               // niche de la poignée d'ouverture (centre)
    float hpY, hpZ, hpW, hpH;         // haut-parleur (centre, taille)
    bool tweeter, ambiance;
    M decor, insert, grille, poignee, accMat;
    int vitres;                       // lève-vitres sur l'accoudoir, côté conducteur
};
mat4 repereFace(float x, float y, float z, float sx) { return translate({x, y, z}) * rotateY(-sx * PI / 2); }
void portiere(PiecesDetails& p, const InterieurDef& d, float cx, float demi, float sx, const Porte& s, bool conducteur) {
    const float xi = cx + sx * (demi - 0.01f), dans = -sx;   // face intérieure (WFS-315, inchangée) ; dans : vers l'habitacle
    const float yc = ligneCeinture(d), zAv = d.pareBriseZ - 0.06f, zAr = 0.60f;
    // décor sous l'appui de vitre, sur toute la longueur, et guide de lumière d'ambiance juste dessous
    profile(p[s.decor], mat4::identity(), {xi + dans * 0.004f, yc - 0.075f, zAv + 0.20f}, {xi + dans * 0.004f, yc - 0.075f, zAr - 0.12f}, 0.034f, 0.008f, 0.003f);
    if (s.ambiance)
        profile(p[M::Ambiance], mat4::identity(), {xi + dans * 0.003f, yc - 0.097f, zAv + 0.22f}, {xi + dans * 0.003f, yc - 0.097f, zAr - 0.14f}, 0.0035f, 0.004f, 0.0015f);
    // insert gainé (tissu, cuir ou Alcantara) entre le décor et l'accoudoir
    const float yi = (yc - 0.10f + s.accY + 0.02f) * 0.5f, hi = (yc - 0.10f) - (s.accY + 0.02f);
    if (hi > 0.03f) dalleFace(p[s.insert], repereFace(xi + dans * 0.003f, yi, (zAv + 0.30f + zAr - 0.20f) * 0.5f, sx), zAr - 0.20f - zAv - 0.30f, hi, 0.006f, 0.025f, 0.003f);
    // niche de la poignée d'ouverture (noir laqué) et levier chromé, galbé vers l'habitacle
    dalleFace(p[M::Laque], repereFace(xi + dans * 0.006f, s.poigY, s.poigZ, sx), 0.14f, 0.048f, 0.006f, 0.02f, 0.003f);   // devant l'insert (pas de faces confondues)
    lisse(p[s.poignee], mat4::identity(), {{xi + dans * 0.010f, s.poigY, s.poigZ - 0.052f}, {xi + dans * 0.019f, s.poigY, s.poigZ - 0.022f},
                                           {xi + dans * 0.021f, s.poigY, s.poigZ + 0.018f}, {xi + dans * 0.013f, s.poigY, s.poigZ + 0.050f}}, 0.011f, 0.016f, 0.005f);
    // accoudoir rembourré, qui remonte un peu vers l'arrière ; poignée de tirage creusée dans son dessus, à l'avant
    {
        const float xa = xi + dans * s.accW * 0.5f;
        lisse(p[s.accMat], mat4::identity(), {{xa, s.accY - 0.025f, s.accZ0}, {xa, s.accY - 0.022f, s.accZ0 + 0.10f}, {xa, s.accY - 0.016f, (s.accZ0 + s.accZ1) * 0.5f},
                                              {xa, s.accY - 0.010f, s.accZ1}}, s.accW, 0.05f, 0.02f);
        dalle(p[M::Lames], translate({xa, s.accY + 0.0005f, s.accZ0 + 0.20f}), s.accW * 0.45f, 0.13f, 0.004f, 0.012f, 0.0012f);
        if (conducteur && s.vitres > 0) {   // platine des lève-vitres (noir laqué), devant la poignée de tirage, touches en relief
            const mat4 V = translate({xa, s.accY + 0.002f, s.accZ0 + 0.075f}) * rotateX(-PI / 2) * rotateZ(sx < 0 ? -PI / 2 : PI / 2);
            dalleFace(p[M::Laque], V, 0.10f, 0.048f, 0.004f, 0.012f, 0.0015f);
            for (int k = 0; k < s.vitres; k++) {
                const float u = (k % 2 ? 0.022f : -0.022f), v = s.vitres > 2 ? (k < 2 ? 0.012f : -0.012f) : 0.f;
                dalleFace(p[M::Lames], V * translate({u, v, 0.003f}), 0.024f, 0.016f, 0.006f, 0.005f, 0.0018f);
            }
        }
    }
    // haut-parleur de porte : lunette, membrane sombre, grille à fentes horizontales
    {
        const mat4 H = repereFace(xi + dans * 0.003f, s.hpY, s.hpZ, sx);
        cadreArrondi(p[s.grille], H * translate({0, 0, 0.010f}), s.hpW, s.hpH, std::min(s.hpW, s.hpH) * 0.42f, 0.010f, 0.010f);
        dalleFace(p[M::Lames], H, s.hpW - 0.016f, s.hpH - 0.016f, 0.003f, std::min(s.hpW, s.hpH) * 0.38f, 0.001f);
        const int n = (int)((s.hpH - 0.03f) / 0.009f);
        for (int k = 0; k < n; k++) {
            const float y = -(s.hpH - 0.03f) * 0.5f + (s.hpH - 0.03f) * (k + 0.5f) / n, rr = std::min(s.hpW, s.hpH) * 0.5f - 0.012f;
            const float dy = std::fabs(y) - (s.hpH * 0.5f - 0.012f - rr), l = dy > 0 ? (s.hpW - 0.024f) - 2 * (rr - std::sqrt(std::max(0.f, rr * rr - dy * dy))) : s.hpW - 0.024f;
            if (l > 0.01f) dalleFace(p[s.grille], H * translate({0, y, 0.006f}), l, 0.0042f, 0.003f, 0.0019f, 0.0008f);
        }
    }
    // tweeter dans l'angle du rétroviseur (haut de la portière, à l'avant)
    if (s.tweeter) {
        const mat4 T = repereFace(xi + dans * 0.004f, yc - 0.045f, zAv + 0.12f, sx);
        cadreArrondi(p[s.grille], T, 0.060f, 0.045f, 0.018f, 0.007f, 0.008f);
        dalleFace(p[M::Lames], T * translate({0, 0, -0.006f}), 0.048f, 0.033f, 0.002f, 0.013f, 0.0006f);
        for (int k = -1; k <= 1; k++) dalleFace(p[s.grille], T * translate({0, k * 0.0085f, -0.002f}), 0.040f, 0.0035f, 0.0025f, 0.0016f, 0.0007f);
    }
    // vide-poche : lèvre moulée en bas de la portière
    profile(p[M::PlancheBas], mat4::identity(), {xi + dans * 0.018f, -0.80f, zAv + 0.20f}, {xi + dans * 0.018f, -0.80f, zAv + 0.80f}, 0.035f, 0.032f, 0.012f);
}

// ---------- plafonnier (liseuses, touches), sous le ciel de toit derrière le pied du rétroviseur intérieur
void plafonnier(PiecesDetails& p, float cx, float z, bool sos) {
    // suit la pente du ciel de toit (dessus collé au ciel de toit, 3 mm dedans)
    const float pente = std::atan2(hauteurCielDeToit(z + 0.08f) - hauteurCielDeToit(z - 0.08f), 0.16f);
    const mat4 C = translate({cx, (hauteurCielDeToit(z + 0.08f) + hauteurCielDeToit(z - 0.08f)) * 0.5f - 0.014f, z}) * rotateX(-pente);   // +z (arrière) monte
    dalle(p[M::PlancheBas], C, 0.23f, 0.16f, 0.034f, 0.045f, 0.010f, 0.92f);
    for (float s : {-1.f, 1.f}) {   // liseuses : lentilles givrées, touche devant chacune
        dalle(p[M::Lentille], C * translate({s * 0.062f, -0.0175f, 0.010f}), 0.058f, 0.042f, 0.003f, 0.012f, 0.001f);
        dalle(p[M::Lames], C * translate({s * 0.062f, -0.0175f, -0.040f}), 0.026f, 0.014f, 0.004f, 0.005f, 0.0013f);
    }
    if (sos) dalle(p[M::Repere], C * translate({0, -0.0175f, -0.035f}), 0.030f, 0.020f, 0.004f, 0.007f, 0.0013f);   // appel d'urgence
}

// ---------------------------------------------------------------- les 4 voitures
// Golf 8 GTI : aérateurs rectangulaires aux deux bouts, à la hauteur des écrans ; deux aérateurs plats sous l'écran central, feux de
// détresse entre eux ; barre tactile de climatisation dessous ; commande d'éclairage tactile à gauche du volant ; décor laqué sous
// le liseré rouge ; console : bac de recharge, petit sélecteur DSG, démarreur, porte-gobelets, accoudoir ; portière : décor,
// lumière d'ambiance, poignée chromée, accoudoir et lève-vitres, haut-parleur et tweeter
void golf(PiecesDetails& p, const InterieurDef& d, float cx, float demi) {
    const float Y = d.plancheY, zF = zFacePlanche(d) + AER_PROF, x0 = cx - demi, x1 = cx + demi;
    for (float x : {x0 + 0.10f, x1 - 0.10f}) aerateur(p, surFace(x, Y - 0.140f, zF, 0.10f), {0.115f, 0.075f, 0.020f, 0.0085f, 5, false, true, false, M::Chrome});
    // sous l'écran central : vu de l'œil, l'écran (17 cm devant la face) cache la face jusqu'à 0,36 m sous l'œil : aérateurs juste dessous
    const float xe = d.ecranPos.x;
    for (float s : {-1.f, 1.f}) aerateur(p, surFace(xe + s * 0.088f, Y - 0.218f, zF, 0.10f), {0.150f, 0.044f, 0.014f, 0.0070f, 3, false, true, false, M::Chrome});
    dalleFace(p[M::Laque], surFace(xe, Y - 0.218f, zF - 0.010f), 0.030f, 0.026f, 0.008f, 0.006f, 0.002f);           // feux de détresse
    cadreArrondi(p[M::Repere], surFace(xe, Y - 0.218f, zF - 0.005f), 0.016f, 0.014f, 0.003f, 0.0028f, 0.002f);
    // barre de climatisation : panneau laqué, deux curseurs tactiles creusés, touches aux bouts
    dalleFace(p[M::Laque], surFace(xe, Y - 0.268f, zF - 0.012f, 0.25f), 0.30f, 0.036f, 0.006f, 0.010f, 0.002f);
    for (float s : {-1.f, 1.f}) dalleFace(p[M::Lames], surFace(xe + s * 0.085f, Y - 0.268f, zF - 0.008f, 0.25f), 0.090f, 0.006f, 0.002f, 0.003f, 0.0008f);
    touches(p, surFace(xe, Y - 0.268f, zF - 0.008f, 0.25f), 3, 0.022f, 0.016f, 0.035f, M::Lames);
    // commande d'éclairage tactile, à gauche du volant, sous l'aérateur
    dalleFace(p[M::Laque], surFace(x0 + 0.10f, Y - 0.235f, zF - 0.012f), 0.10f, 0.055f, 0.006f, 0.010f, 0.002f);
    touches(p, surFace(x0 + 0.10f, Y - 0.235f, zF - 0.008f), 3, 0.022f, 0.018f, 0.030f, M::Lames);
    // décor laqué sous le liseré rouge, côté passager, jusqu'à l'aérateur de droite ; lumière d'ambiance dessous
    profile(p[M::Laque], mat4::identity(), {cx + 0.18f, Y - 0.135f, zF - 0.012f}, {x1 - 0.17f, Y - 0.135f, zF - 0.012f}, 0.045f, 0.008f, 0.004f);
    profile(p[M::Ambiance], mat4::identity(), {cx + 0.19f, Y - 0.162f, zF - 0.013f}, {x1 - 0.18f, Y - 0.162f, zF - 0.013f}, 0.0035f, 0.004f, 0.0015f);
    // console
    const Console c{cx, -0.575f, zFacePlanche(d) + 0.02f, 0.16f, 0.24f, 0.05f};
    corpsConsole(p, d, c, M::PlancheBas);
    const float zc0 = c.z0 - (c.z0 + c.z1) * 0.5f;   // avant de la console, repère de la console
    bac(p, c, zc0 + 0.03f, zc0 + 0.19f, 0.18f);
    {   // platine du sélecteur (noir laqué) : sélecteur DSG à bascule, démarreur à gauche, touches P et mode à droite
        const mat4 C = repereConsole(c) * translate({0, 0.004f, zc0 + 0.34f});
        dalle(p[M::Laque], C, 0.17f, 0.15f, 0.008f, 0.02f, 0.003f);
        dalle(p[M::Lames], C * translate({0, 0.006f, 0.012f}), 0.042f, 0.072f, 0.006f, 0.016f, 0.003f);                       // embase
        profile(p[M::Lames], C, {0, 0.008f, 0.020f}, {0, 0.042f, 0.004f}, 0.030f, 0.040f, 0.012f, 0.034f, 0.026f);          // corps
        dalle(p[M::Cuir], C * translate({0, 0.046f, 0.002f}) * rotateX(0.35f), 0.034f, 0.050f, 0.010f, 0.015f, 0.004f);       // dessus
        boutonRond(p, C * translate({-0.058f, 0.006f, -0.020f}) * rotateX(-PI / 2), 0.012f, 0.006f, M::Repere, M::Laque);    // démarreur
        for (float z : {-0.020f, 0.020f}) dalleFace(p[M::Lames], C * translate({0.058f, 0.006f, z}) * rotateX(-PI / 2), 0.024f, 0.018f, 0.006f, 0.005f, 0.0018f);   // P, mode
    }
    gobelets(p, c, zc0 + 0.51f, M::Laque);
    accoudoir(p, c, zc0 + 0.67f, c.z1 - (c.z0 + c.z1) * 0.5f, M::Cuir);
    const Porte porte{-0.415f, -0.62f, 0.20f, 0.085f, -0.300f, -0.62f, -0.70f, -0.90f, 0.17f, 0.17f, true, true, M::Laque, M::Tissu, M::PlancheBas,
                      M::Chrome, M::Cuir, 4};
    for (float sx : {-1.f, 1.f}) portiere(p, d, cx, demi, sx, porte, sx < 0);
    plafonnier(p, cx, HAUT_PARE_BRISE_Z + 0.13f, true);
}

// M4 Competition (G82) : aérateurs à lunette chromée et molette de fermeture, aux bouts et deux sous l'écran central posé sur la
// planche, feux de détresse entre eux ; panneau de climatisation à touches et bascules chromées ; commande d'éclairage à gauche
// (molette et touches) ; carbone (jonc existant) ; console : bac, sélecteur M gainé, commande rotative et ses touches, accoudoir ;
// portière : décor carbone, lumière d'ambiance blanche, poignée chromée, haut-parleur à grille alu
void m4(PiecesDetails& p, const InterieurDef& d, float cx, float demi) {
    const float Y = d.plancheY, zF = zFacePlanche(d) + AER_PROF, x0 = cx - demi, x1 = cx + demi;
    for (float x : {x0 + 0.075f, x1 - 0.075f}) aerateur(p, surFace(x, Y - 0.165f, zF, 0.10f), {0.105f, 0.075f, 0.010f, 0.0080f, 4, false, true, true, M::Chrome});
    const float xe = d.ecranPos.x;
    for (float s : {-1.f, 1.f}) aerateur(p, surFace(xe + s * 0.072f, Y - 0.160f, zF, 0.10f), {0.118f, 0.058f, 0.010f, 0.0075f, 4, false, true, true, M::Chrome});
    dalleFace(p[M::Laque], surFace(xe, Y - 0.160f, zF - 0.010f), 0.022f, 0.044f, 0.008f, 0.005f, 0.002f);           // feux de détresse
    cadreArrondi(p[M::Repere], surFace(xe, Y - 0.160f, zF - 0.005f), 0.012f, 0.018f, 0.003f, 0.0025f, 0.002f);
    // climatisation : panneau laqué, rangée de touches, deux bascules chromées de température aux bouts
    dalleFace(p[M::Laque], surFace(xe, Y - 0.235f, zF - 0.012f, 0.25f), 0.27f, 0.050f, 0.006f, 0.010f, 0.002f);
    touches(p, surFace(xe, Y - 0.235f, zF - 0.008f, 0.25f), 6, 0.024f, 0.020f, 0.031f, M::Lames);
    for (float s : {-1.f, 1.f}) bascule(p, surFace(xe + s * 0.120f, Y - 0.238f, zF - 0.008f, 0.25f), M::Chrome);
    // commande d'éclairage, à gauche du volant : molette chromée et deux touches
    dalleFace(p[M::Laque], surFace(x0 + 0.105f, Y - 0.235f, zF - 0.012f), 0.10f, 0.060f, 0.006f, 0.012f, 0.002f);
    molette(p, surFace(x0 + 0.088f, Y - 0.235f, zF - 0.009f), 0.018f, 0.012f, M::Chrome, M::Lames);
    touches(p, surFace(x0 + 0.135f, Y - 0.235f, zF - 0.008f) * rotateZ(PI / 2), 2, 0.018f, 0.016f, 0.022f, M::Lames);
    profile(p[M::Ambiance], mat4::identity(), {cx + 0.17f, Y - 0.128f, zF - 0.013f}, {x1 - 0.17f, Y - 0.128f, zF - 0.013f}, 0.0035f, 0.004f, 0.0015f);
    // console
    const Console c{cx, -0.580f, zFacePlanche(d) + 0.02f, 0.16f, 0.25f, 0.05f};
    corpsConsole(p, d, c, M::PlancheBas);
    const float zc0 = c.z0 - (c.z0 + c.z1) * 0.5f;
    bac(p, c, zc0 + 0.03f, zc0 + 0.19f, 0.19f);
    {   // sélecteur M : soufflet et poignée gainée de cuir, touche P sur le dessus ; commande rotative et ses touches derrière, à droite
        const mat4 C = repereConsole(c) * translate({0, 0.004f, zc0 + 0.33f});
        dalle(p[M::Laque], C, 0.20f, 0.20f, 0.008f, 0.025f, 0.003f);
        dalle(p[M::Cuir], C * translate({-0.012f, 0.008f, 0.0f}), 0.070f, 0.090f, 0.010f, 0.030f, 0.005f, 0.9f);              // soufflet
        profile(p[M::Cuir], C, {-0.012f, 0.012f, 0.010f}, {-0.012f, 0.078f, -0.012f}, 0.040f, 0.058f, 0.018f, 0.050f, 0.036f);  // poignée
        dalle(p[M::Chrome], C * translate({-0.012f, 0.081f, -0.016f}) * rotateX(0.40f), 0.030f, 0.026f, 0.006f, 0.010f, 0.002f); // touche P
        molette(p, C * translate({0.050f, 0.006f, 0.070f}) * rotateX(-PI / 2), 0.026f, 0.020f, M::Chrome, M::Lames);              // commande rotative
        for (int k = 0; k < 4; k++) {   // touches autour de la commande rotative
            const float a = PI * (0.65f + 0.23f * k);
            dalleFace(p[M::Lames], C * translate({0.050f + std::cos(a) * 0.048f, 0.007f, 0.070f - std::sin(a) * 0.048f}) * rotateX(-PI / 2), 0.020f, 0.012f, 0.005f, 0.004f, 0.0015f);
        }
        touches(p, C * translate({-0.072f, 0.006f, 0.020f}) * rotateX(-PI / 2) * rotateZ(PI / 2), 3, 0.020f, 0.016f, 0.026f, M::Lames);   // modes de conduite
    }
    accoudoir(p, c, zc0 + 0.62f, c.z1 - (c.z0 + c.z1) * 0.5f, M::Cuir);
    const Porte porte{-0.420f, -0.64f, 0.20f, 0.090f, -0.310f, -0.66f, -0.70f, -0.90f, 0.18f, 0.15f, false, true, M::Accent, M::Cuir, M::Alu,
                      M::Chrome, M::Cuir, 4};
    for (float sx : {-1.f, 1.f}) portiere(p, d, cx, demi, sx, porte, sx < 0);
    plafonnier(p, cx, HAUT_PARE_BRISE_Z + 0.13f, true);
}

// 911 Carrera 4S (992.1) : planche plate et basse ; aérateurs en stade aux deux bouts et deux aérateurs plats sous l'écran encastré ;
// rangée de 5 interrupteurs à bascule sous les aérateurs ; commande de démarrage tournante à gauche du volant ; alu brossé (jonc
// existant) ; console : petit sélecteur électrique, touches, couvercle de rangement en cuir, accoudoir ; portière : poignée alu,
// accoudoir, haut-parleur à grille alu
void p911(PiecesDetails& p, const InterieurDef& d, float cx, float demi) {
    const float Y = d.plancheY, zF = zFacePlanche(d) + AER_PROF, x0 = cx - demi, x1 = cx + demi;
    for (float x : {x0 + 0.11f, x1 - 0.11f}) aerateur(p, surFace(x, Y - 0.150f, zF, 0.10f), stade(0.130f, 0.052f, 3, M::Chrome));
    const float xe = d.ecranPos.x, yb = d.ecranPos.y - d.ecranH * 0.5f;   // bas de l'écran encastré
    // vu de l'œil, l'écran (6 cm devant la face) cache la face jusqu'à 0,42 m sous l'œil : aérateurs juste dessous, puis les bascules
    for (float s : {-1.f, 1.f}) aerateur(p, surFace(xe + s * 0.075f, yb - 0.052f, zF, 0.10f), stade(0.135f, 0.034f, 2, M::Chrome));
    for (int k = 0; k < 5; k++) bascule(p, surFace(xe + (k - 2) * 0.034f, yb - 0.098f, zF - 0.008f, 0.20f), M::Chrome);
    dalleFace(p[M::Laque], surFace(xe, yb - 0.098f, zF - 0.012f, 0.20f), 0.20f, 0.040f, 0.006f, 0.010f, 0.002f);
    {   // démarrage : bague chromée et poignée tournante en forme de clé, à gauche du volant
        const mat4 S = surFace(x0 + 0.13f, Y - 0.235f, zF - 0.012f);
        boutonRond(p, S, 0.020f, 0.010f, M::Chrome, M::Lames);
        profile(p[M::Lames], S, {0, -0.014f, 0.012f}, {0, 0.014f, 0.012f}, 0.010f, 0.016f, 0.005f);
    }
    // console
    const Console c{cx, -0.585f, zFacePlanche(d) + 0.02f, 0.16f, 0.23f, 0.06f};
    corpsConsole(p, d, c, M::PlancheBas);
    const float zc0 = c.z0 - (c.z0 + c.z1) * 0.5f;
    {   // platine laquée : petit sélecteur électrique (corps, tête chromée), touches de chaque côté, bac devant
        const mat4 C = repereConsole(c) * translate({0, 0.004f, zc0 + 0.32f});
        dalle(p[M::Laque], C, 0.19f, 0.24f, 0.008f, 0.025f, 0.003f);
        profile(p[M::Lames], C, {0, 0.006f, 0.014f}, {0, 0.040f, -0.004f}, 0.026f, 0.044f, 0.011f, 0.036f, 0.022f);
        dalle(p[M::Chrome], C * translate({0, 0.042f, -0.006f}) * rotateX(0.30f), 0.024f, 0.040f, 0.008f, 0.010f, 0.003f);
        for (float s : {-1.f, 1.f}) touches(p, C * translate({s * 0.066f, 0.006f, 0.005f}) * rotateX(-PI / 2) * rotateZ(PI / 2), 3, 0.024f, 0.020f, 0.030f, M::Lames);
        bac(p, c, zc0 + 0.03f, zc0 + 0.11f, 0.16f);
    }
    accoudoir(p, c, zc0 + 0.60f, c.z1 - (c.z0 + c.z1) * 0.5f, M::Cuir);
    const Porte porte{-0.400f, -0.60f, 0.20f, 0.085f, -0.290f, -0.60f, -0.70f, -0.88f, 0.17f, 0.15f, false, false, M::Accent, M::Cuir, M::Alu,
                      M::Alu, M::Cuir, 2};
    for (float sx : {-1.f, 1.f}) portiere(p, d, cx, demi, sx, porte, sx < 0);
    plafonnier(p, cx, HAUT_PARE_BRISE_Z + 0.12f, false);
}

// SF90 Stradale : pas d'écran central ; aérateurs ronds « turbine » à bague alu, deux aux bouts et deux au milieu ; console : platine
// alu en forme de grille de boîte manuelle (R, mode, départ arrêté), accoudoir ; portière : décor carbone, insert Alcantara,
// levier d'ouverture alu, haut-parleur à grille alu
void sf90(PiecesDetails& p, const InterieurDef& d, float cx, float demi) {
    const float Y = d.plancheY, zF = zFacePlanche(d) + 0.018f, x0 = cx - demi, x1 = cx + demi;
    for (float x : {x0 + 0.06f, x1 - 0.06f}) aerateurRond(p, surFace(x, Y - 0.175f, zF, 0.10f), 0.085f, M::Alu);
    for (float s : {-1.f, 1.f}) aerateurRond(p, surFace(cx + s * 0.070f, Y - 0.175f, zF, 0.10f), 0.080f, M::Alu);
    // panneau de touches (carbone) entre les aérateurs du milieu et la console
    dalleFace(p[M::Accent], surFace(cx, Y - 0.250f, zF - 0.012f, 0.25f), 0.22f, 0.040f, 0.006f, 0.010f, 0.002f);
    touches(p, surFace(cx, Y - 0.250f, zF - 0.008f, 0.25f), 5, 0.026f, 0.018f, 0.036f, M::Lames);
    // console
    const Console c{cx, -0.600f, zFacePlanche(d) + 0.02f, 0.16f, 0.22f, 0.06f};
    corpsConsole(p, d, c, M::Accent);
    const float zc0 = c.z0 - (c.z0 + c.z1) * 0.5f;
    {   // grille : platine alu, trois fentes sombres en H (une longue, deux branches), un levier dans chaque branche
        const mat4 C = repereConsole(c) * translate({0, 0.004f, zc0 + 0.32f});
        dalle(p[M::Alu], C, 0.15f, 0.20f, 0.012f, 0.030f, 0.004f);
        dalle(p[M::Lames], C * translate({0, 0.0065f, 0}), 0.014f, 0.12f, 0.002f, 0.007f, 0.0008f);
        for (float s : {-1.f, 1.f}) dalle(p[M::Lames], C * translate({s * 0.035f, 0.0065f, -0.03f}), 0.056f, 0.014f, 0.002f, 0.007f, 0.0008f);
        for (float s : {-1.f, 0.f, 1.f}) {
            const vec3 b{s * 0.058f, 0.007f, s == 0 ? 0.045f : -0.03f};
            profile(p[s == 0 ? M::Repere : M::Alu], C, b, b + vec3{0, 0.026f, -0.006f}, 0.010f, 0.007f, 0.0035f, 0.006f, 0.014f);
        }
    }
    accoudoir(p, c, zc0 + 0.60f, c.z1 - (c.z0 + c.z1) * 0.5f, M::Tissu);
    const Porte porte{-0.420f, -0.62f, 0.15f, 0.075f, -0.270f, -0.70f, -0.68f, -0.86f, 0.15f, 0.12f, false, false, M::Accent, M::Tissu, M::Alu,
                      M::Alu, M::Tissu, 2};
    for (float sx : {-1.f, 1.f}) portiere(p, d, cx, demi, sx, porte, sx < 0);
    plafonnier(p, cx, HAUT_PARE_BRISE_Z + 0.12f, false);
}

struct DetailsDef { const char* car; void (*construire)(PiecesDetails&, const InterieurDef&, float, float); unsigned ambiance; };
// lumière d'ambiance la nuit : Golf GTI rouge (couleur du liseré), M4 blanche ; 911 et SF90 : sans (E)
const DetailsDef DETAILS[] = {{"golf", golf, 0xd0141e}, {"m4", m4, 0xe8eef6}, {"p911", p911, 0}, {"sf90", sf90, 0}};
const DetailsDef* detailsDef(const InterieurDef& d) {
    for (const DetailsDef& k : DETAILS) if (std::strcmp(k.car, d.car) == 0) return &k;
    return nullptr;
}
} // namespace

bool detailsPropres(const InterieurDef& d) { return detailsDef(d) != nullptr; }
unsigned couleurAmbiance(const InterieurDef& d) { const DetailsDef* k = detailsDef(d); return k ? k->ambiance : 0; }

void construireDetails(const InterieurDef& d, PiecesDetails& p) {
    p = PiecesDetails{};
    const DetailsDef* k = detailsDef(d);
    if (!k) return;
    const Largeur l = largeurPour(d);
    k->construire(p, d, l.oeil, l.coudes * 0.5f);
}

} // namespace wfs::cockpit
