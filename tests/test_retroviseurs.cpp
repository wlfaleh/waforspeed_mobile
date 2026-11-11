// WFS-324 : rétroviseurs de la vue Cockpit (src/render/retroviseurs.h), par le calcul :
//   - réflexion par un plan (involution, points du plan fixes) ;
//   - projection du miroir plan (Kooima) : les 4 coins de la glace tombent aux 4 coins de l'image du reflet (uv de la glace) ;
//     un point vu dans la glace (rayon de l'œil réfléchi par la loi de la réflexion) tombe au texel de son point de la glace ;
//     ce qui est devant la glace (côté non réfléchissant) est coupé ; profondeur inversée dans ]0, 1] ;
//   - les 3 glaces des 20 intérieurs : devant l'œil, tournées vers lui, l'intérieure sur l'axe en haut du pare-brise, les
//     extérieures hors de la portière ; reflet du centre vers l'arrière ; tailles réglementaires (ONU R46) ;
//   - mesure des rétroviseurs sur un modèle (boîte de caisse + coques qui dépassent) et sans coque.
// usage : test_retroviseurs
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/render/retroviseurs.h"
#include "../src/render/vehicule_cockpit.h"
using namespace wfs;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("rétroviseurs : %-92s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }
static vec4 projeter(const mat4& M, vec3 p) {
    vec4 q{};
    q.x = M.at(0, 0) * p.x + M.at(0, 1) * p.y + M.at(0, 2) * p.z + M.at(0, 3);
    q.y = M.at(1, 0) * p.x + M.at(1, 1) * p.y + M.at(1, 2) * p.z + M.at(1, 3);
    q.z = M.at(2, 0) * p.x + M.at(2, 1) * p.y + M.at(2, 2) * p.z + M.at(2, 3);
    q.w = M.at(3, 0) * p.x + M.at(3, 1) * p.y + M.at(3, 2) * p.z + M.at(3, 3);
    return q;
}
static bool proche(float a, float b, float e = 1e-3f) { return std::fabs(a - b) <= e; }

int main() {
    // ---- réflexion
    {
        const vec3 p{0.3f, 1.2f, -0.5f}, n = normalize(vec3{0.2f, -0.1f, 1.f});
        const mat4 M = retro::reflexion(p, n);
        const vec3 a{2.f, -1.f, 7.f}, aa = M.transformPoint(M.transformPoint(a)), q = p + vec3{0.1f, 0.05f, 0} - n * dot(vec3{0.1f, 0.05f, 0}, n);
        cas("réflexion deux fois = identité", length(aa - a) < 1e-4f);
        cas("point du plan inchangé", length(M.transformPoint(q) - q) < 1e-4f);
        cas("normale retournée", length(M.transformDir(n) + n) < 1e-4f);
    }
    // ---- projection du miroir plan : glace inclinée quelconque devant un œil
    {
        retro::Miroir m;
        m.w = 0.24f; m.h = 0.06f;
        const vec3 c{0.35f, 0.15f, -0.55f}, oeil{0.f, 0.04f, 0.f};
        const vec3 n = normalize(normalize(oeil - c) + vec3{0, 0, 1});
        const vec3 x = normalize(cross(vec3{0, 1, 0}, n)), y = cross(n, x);
        mat4 G = mat4::identity();
        G.at(0, 0) = x.x; G.at(1, 0) = x.y; G.at(2, 0) = x.z; G.at(0, 1) = y.x; G.at(1, 1) = y.y; G.at(2, 1) = y.z;
        G.at(0, 2) = n.x; G.at(1, 2) = n.y; G.at(2, 2) = n.z; G.at(0, 3) = c.x; G.at(1, 3) = c.y; G.at(2, 3) = c.z;
        m.repere = G;
        const mat4 R = translate({100.f, 2.f, -40.f}) * rotateY(0.7f);   // repère de l'intérieur quelconque dans le monde
        const retro::Fenetre f = retro::fenetre(m, R);
        const vec3 oeilM = R.transformPoint(oeil);
        mat4 vp; vec3 oeilR;
        const bool ok = retro::vueProjMiroir(f, oeilM, 0.f, vp, oeilR);
        cas("projection calculée (œil devant la glace)", ok);
        // coins : bas gauche (−1, +1), bas droit (+1, +1), haut gauche (−1, −1) en coordonnées normalisées (y vers le bas), profondeur 1
        auto ndc = [&](vec3 p) { const vec4 q = projeter(vp, p); return vec3{q.x / q.w, q.y / q.w, q.z / q.w}; };
        const vec3 a = ndc(f.pa), b = ndc(f.pb), cc = ndc(f.pc);
        cas("coin bas gauche de la glace → coin bas gauche de l'image", proche(a.x, -1) && proche(a.y, 1) && proche(a.z, 1));
        cas("coin bas droit de la glace → coin bas droit de l'image", proche(b.x, 1) && proche(b.y, 1));
        cas("coin haut gauche de la glace → coin haut gauche de l'image", proche(cc.x, -1) && proche(cc.y, -1));
        // uv de la glace (glaceRetro : u = 0,5 + x / w, v = 0,5 − y / h) = texel de l'image (u = (x_ndc + 1) / 2, v = (y_ndc + 1) / 2)
        const float lx = 0.07f, ly = -0.012f;
        const vec3 X = R.transformPoint(G.transformPoint({lx, ly, 0}));
        const vec3 nx = ndc(X);
        cas("uv d'un point de la glace = texel de l'image du reflet", proche((nx.x + 1) * 0.5f, 0.5f + lx / m.w) && proche((nx.y + 1) * 0.5f, 0.5f - ly / m.h));
        // loi de la réflexion : le point vu en X (rayon de l'œil réfléchi sur la glace), à 12 m, tombe au même texel que X
        const vec3 nM = normalize(R.transformDir(n)), d = normalize(X - oeilM), rr = d - nM * (2 * dot(d, nM));
        const vec3 P = X + rr * 12.f, np = ndc(P);
        cas("point vu dans la glace (12 m) → même texel que son point de la glace", proche(np.x, nx.x, 2e-3f) && proche(np.y, nx.y, 2e-3f));
        cas("profondeur inversée dans ]0, 1[ (point à 12 m : < 0,1)", np.z > 0 && np.z < 0.1f && np.z < nx.z);
        // côté non réfléchissant : un point devant la glace vu dans sa direction (derrière elle pour l'œil) est coupé (profondeur > 1)
        const vec3 Q = X - nM * 0.5f + d * 0.05f;
        const vec4 qq = projeter(vp, Q);
        cas("objet derrière la glace (côté non réfléchissant) : coupé", qq.w <= 0 || qq.z / qq.w > 1.f);
        cas("œil réfléchi de l'autre côté de la glace", dot(oeilR - f.pa, nM) < 0 && proche(dot(oeilR - f.pa, nM), -dot(oeilM - f.pa, nM)));
        // recul : même cadrage, plan proche plus loin
        mat4 vp2; vec3 o2;
        retro::vueProjMiroir(f, oeilM, 0.3f, vp2, o2);
        const vec4 a2 = projeter(vp2, f.pa), p2 = projeter(vp2, P);
        cas("plan proche reculé : même cadrage, glace devant le plan proche (coupée)", proche(p2.x / p2.w, np.x, 2e-3f) && a2.z / a2.w > 1.f);
        // œil derrière la glace : rien
        mat4 vp3; vec3 o3;
        cas("œil derrière la glace : aucune vue", !retro::vueProjMiroir(f, R.transformPoint(c - n * 0.2f), 0.f, vp3, o3));
    }
    // ---- les 3 glaces des 20 intérieurs
    {
        int malPlacees = 0, malReglees = 0, tailles = 0;
        for (int i = 0; i < cockpit::nombreInterieurs(); i++) {
            const cockpit::InterieurDef& d = cockpit::interieur(i);
            const cockpit::Largeur l = cockpit::largeurPour(d);
            const float cx = l.oeil, demi = l.coudes * 0.5f;
            const auto m = retro::miroirs(d, cx, demi, {1.8f, 4.4f, nullptr});
            const vec3 oeil{0, cockpit::OEIL_CAMERA_PLUS_M, 0};
            for (int k = 0; k < retro::NB_GLACES; k++) {
                const mat4& G = m[(size_t)k].repere;
                const vec3 c{G.at(0, 3), G.at(1, 3), G.at(2, 3)}, n{G.at(0, 2), G.at(1, 2), G.at(2, 2)};
                bool place = c.z < -0.25f && dot(oeil - c, n) > 0;
                if (k == retro::INTERIEUR) place = place && std::fabs(c.x - cx) < 0.05f && c.y > 0.0f && c.y < cockpit::HAUT_PARE_BRISE_Y;
                else place = place && std::fabs(c.x - cx) > cockpit::faceVitre(demi) && (k == retro::GAUCHE) == (c.x < cx);
                if (!place) { malPlacees++; std::printf("  glace %d de %s mal placée (%.3f ; %.3f ; %.3f)\n", k, d.car, c.x, c.y, c.z); }
                // rayon de l'œil vers le centre, réfléchi : vers l'arrière (≥ 0,95 de +z), à moins de 6° de l'horizontale ; extérieur : vers
                // le dehors de moins de 8°
                const vec3 dd = normalize(c - oeil), rr = dd - n * (2 * dot(dd, n));
                bool regle = rr.z > 0.95f && std::fabs(rr.y) < std::sin(6 * PI / 180);
                if (k != retro::INTERIEUR) regle = regle && std::fabs(rr.x) < std::sin(8 * PI / 180);
                if (!regle) { malReglees++; std::printf("  glace %d de %s mal réglée (reflet du centre %.3f ; %.3f ; %.3f)\n", k, d.car, rr.x, rr.y, rr.z); }
                // ONU R46 : glace intérieure ≥ 40 × 130 mm (rectangle de 40 mm de haut, segment de 130 mm), extérieure (classe III) idem
                if (m[(size_t)k].h < 0.040f || m[(size_t)k].w < 0.130f || m[(size_t)k].reflexion < 0.4f) tailles++;
            }
        }
        cas("20 intérieurs : glaces devant l'œil, tournées vers lui, à leur place (axe, hors de la portière)", malPlacees == 0);
        cas("20 intérieurs : reflet du centre vers l'arrière (intérieure : droit derrière ; extérieures : dehors < 8°)", malReglees == 0);
        cas("20 intérieurs : glaces ≥ 40 × 130 mm, réflexion ≥ 40 % (ONU R46)", tailles == 0);
    }
    // ---- mesure des rétroviseurs sur un modèle : caisse de 1,8 × 1,4 × 4,4 m, coques de 20 × 12 × 10 cm qui dépassent de 12 cm
    {
        Model mo; mo.bmin = {-1.02f, 0, -2.2f}; mo.bmax = {1.02f, 1.4f, 2.2f};
        std::vector<float> v;
        auto boite = [&](vec3 a, vec3 b, int n, int nz) {
            for (int i = 0; i <= n; i++) for (int j = 0; j <= n; j++) for (int k = 0; k <= nz; k++)
                v.insert(v.end(), {a.x + (b.x - a.x) * i / n, a.y + (b.y - a.y) * j / n, a.z + (b.z - a.z) * k / nz});
        };
        boite({-0.9f, 0.f, -2.2f}, {0.9f, 1.4f, 2.2f}, 8, 8);   // caisse : sommets tous les 55 cm en long (tranches vides comblées)
        const size_t caisse = v.size();
        boite({-1.02f, 0.95f, -0.75f}, {-0.86f, 1.07f, -0.65f}, 4, 4);
        boite({0.86f, 0.95f, -0.75f}, {1.02f, 1.07f, -0.65f}, 4, 4);
        retro::mesurerRetroviseurs(mo, v.data(), 3, v.size() / 3);
        cas("modèle : rétroviseurs gauche et droit trouvés", mo.retroMesure[0] && mo.retroMesure[1]);
        cas("modèle : boîte du gauche à sa place (± 1 cm)", proche(mo.retroMin[0].x, -1.02f, 0.01f) && proche(mo.retroMax[0].z, -0.65f, 0.01f) && proche(mo.retroMin[0].y, 0.95f, 0.01f));
        Model sans; sans.bmin = {-0.9f, 0, -2.2f}; sans.bmax = {0.9f, 1.4f, 2.2f};
        std::vector<float> w(v.begin(), v.begin() + (long)caisse);
        retro::mesurerRetroviseurs(sans, w.data(), 3, w.size() / 3);
        cas("modèle sans rétroviseur : rien trouvé (place estimée)", !sans.retroMesure[0] && !sans.retroMesure[1]);
        // glaces posées avec le modèle : reflet vu depuis la glace du modèle (décalage), plan proche derrière toute la coque
        const cockpit::InterieurDef& d = *cockpit::interieurPour("golf");
        const cockpit::Largeur l = cockpit::largeurPour(d);
        const auto m = retro::miroirs(d, l.oeil, l.coudes * 0.5f, {1.8f, 4.4f, &mo});
        cas("glaces extérieures : reflet vu depuis le rétroviseur du modèle, plan proche derrière sa coque",
            m[retro::GAUCHE].mesure && m[retro::DROITE].mesure && m[retro::GAUCHE].recul >= 0.10f && length(m[retro::GAUCHE].decalageVue) > 0.f);
    }
    std::printf("rétroviseurs : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
