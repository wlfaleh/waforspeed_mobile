// WaForSpeed — WFS-376 : bateaux et équipements de la marina, modélisés (voir marina_bateaux.h).
#include "marina_bateaux.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace wfs {
namespace marina {

// ---------------------------------------------------------------- outils
void loft(Maille& m, const std::vector<std::vector<vec3>>& an, int couleur, bool ferme, vec3 a, vec3 b) {
    if (an.size() < 2 || an[0].size() < 2) return;
    const size_t n = an[0].size();
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const auto& r : an) for (const vec3& p : r) m.v.push_back({p, {0, 0, 0}, couleur});
    auto id = [&](size_t i, size_t j) { return k0 + (uint32_t)(i * n + j % n); };
    const vec3 ab = b - a;
    const float l2 = dot(ab, ab);
    auto dehors = [&](vec3 c) { const float u = l2 > 1e-9f ? std::clamp(dot(c - a, ab) / l2, 0.f, 1.f) : 0.f; return c - (a + ab * u); };
    const size_t i0 = m.i.size(), nj = ferme ? n : n - 1;
    float signe = 0;   // sens majoritaire : tout le loft est retourné d'un bloc (les triangles d'un loft ont le même sens)
    auto ajoute = [&](uint32_t p, uint32_t q, uint32_t r) {
        const vec3 A = m.v[p].p, B = m.v[q].p, C = m.v[r].p, nn = cross(B - A, C - A);
        if (dot(nn, nn) < 1e-12f) return;   // triangle dégénéré (pointe de l'étrave, bout d'un fuseau) : ignoré
        signe += dot(normalize(nn), dehors((A + B + C) * (1.f / 3)));
        m.i.insert(m.i.end(), {p, q, r});
    };
    for (size_t i = 0; i + 1 < an.size(); i++) for (size_t j = 0; j < nj; j++) {
        ajoute(id(i, j), id(i + 1, j), id(i + 1, j + 1));
        ajoute(id(i, j), id(i + 1, j + 1), id(i, j + 1));
    }
    if (signe < 0) for (size_t k = i0; k < m.i.size(); k += 3) std::swap(m.i[k + 1], m.i[k + 2]);
    for (size_t k = i0; k < m.i.size(); k += 3) {   // normales lissées : moyenne des faces, pondérée par l'aire
        const vec3 nn = cross(m.v[m.i[k + 1]].p - m.v[m.i[k]].p, m.v[m.i[k + 2]].p - m.v[m.i[k]].p);
        for (int e = 0; e < 3; e++) m.v[m.i[k + e]].n += nn;
    }
    for (size_t k = k0; k < m.v.size(); k++) {
        vec3 nn = normalize(m.v[k].n);
        if (length(nn) < 0.5f) nn = normalize(dehors(m.v[k].p));
        m.v[k].n = nn;
    }
}

void eventail(Maille& m, const std::vector<vec3>& pts, int couleur, vec3 dehors) {
    if (pts.size() < 3) return;
    vec3 c{0, 0, 0}, nn{0, 0, 0};
    for (size_t k = 0; k < pts.size(); k++) { c += pts[k]; nn += cross(pts[k], pts[(k + 1) % pts.size()]); }   // normale de Newell
    c = c * (1.f / (float)pts.size());
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    m.v.push_back({c, nn, couleur});
    for (const vec3& p : pts) m.v.push_back({p, nn, couleur});
    for (uint32_t k = 0; k < (uint32_t)pts.size(); k++) {
        const uint32_t a = k0 + 1 + k, b = k0 + 1 + (k + 1) % (uint32_t)pts.size();
        if (inverse) m.i.insert(m.i.end(), {k0, b, a}); else m.i.insert(m.i.end(), {k0, a, b});
    }
}

void ligne(Maille& m, vec3 a, vec3 b, float l, int couleur) {
    const vec3 d = normalize(b - a);
    vec3 p1 = cross(d, vec3{0, 1, 0});
    if (length(p1) < 0.2f) p1 = cross(d, vec3{1, 0, 0});
    p1 = normalize(p1);
    const vec3 p2 = normalize(cross(d, p1));
    for (const vec3& p : {p1, p2}) {
        const vec3 e = p * (l * 0.5f), nn = normalize(cross(p, d));   // sens des triangles (a − e, a + e, b + e)
        const uint32_t k0 = (uint32_t)m.v.size();
        m.v.push_back({a - e, nn, couleur}); m.v.push_back({a + e, nn, couleur});
        m.v.push_back({b + e, nn, couleur}); m.v.push_back({b - e, nn, couleur});
        m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
    }
}

namespace {

// quadrilatère plan (2 triangles), tourné vers dehors
void quadPlat(Maille& m, vec3 a, vec3 b, vec3 c, vec3 d, int couleur, vec3 dehors) {
    vec3 nn = cross(b - a, c - a);
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const vec3& p : {a, b, c, d}) m.v.push_back({p, nn, couleur});
    if (inverse) m.i.insert(m.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
    else m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
}
void triPlat(Maille& m, vec3 a, vec3 b, vec3 c, int couleur, vec3 dehors) {
    vec3 nn = cross(b - a, c - a);
    const bool inverse = dot(nn, dehors) < 0;
    nn = normalize(inverse ? -nn : nn);
    const uint32_t k0 = (uint32_t)m.v.size();
    for (const vec3& p : {a, b, c}) m.v.push_back({p, nn, couleur});
    if (inverse) m.i.insert(m.i.end(), {k0, k0 + 2, k0 + 1}); else m.i.insert(m.i.end(), {k0, k0 + 1, k0 + 2});
}
// anneau ellipse de n points autour du centre c, dans le plan (e1, e2)
std::vector<vec3> ellipse(vec3 c, vec3 e1, vec3 e2, float r1, float r2, int n) {
    std::vector<vec3> r;
    for (int k = 0; k < n; k++) { const float a = 2 * PI * (float)k / (float)n; r.push_back(c + e1 * (std::cos(a) * r1) + e2 * (std::sin(a) * r2)); }
    return r;
}
// tour (pièce de révolution verticale) : profils (rayon, hauteur) de bas en haut, n côtés, dessus fermé
void tour(Maille& m, vec3 base, const std::vector<std::pair<float, float>>& prof, int n, int couleur) {
    std::vector<std::vector<vec3>> an;
    for (auto [r, y] : prof) an.push_back(ellipse(base + vec3{0, y, 0}, {1, 0, 0}, {0, 0, 1}, r, r, n));
    loft(m, an, couleur, true, base, base + vec3{0, prof.back().second, 0});
    eventail(m, an.back(), couleur, {0, 1, 0});
}

// ---------------------------------------------------------------- coque lissée
// u : 0 au tableau arrière, 1 à l'étrave. Plan de forme : quasi parallèle à l'arrière (0,86 du bau au tableau), maître-bau à
// 40-46 % de la longueur, étrave en arc d'ellipse ; tonture (+14 % de franc-bord à l'étrave) ; bouge du pont (3,5 % du bau) ;
// étrave inclinée (7 % de L pour un voilier, 12 % pour un bateau à moteur, de la flottaison au pont)
struct Forme {
    float Lh, B, F, T, z0; bool moteur;
    float demi(float u) const {
        const float um = moteur ? 0.40f : 0.46f, h = B * 0.5f;
        if (u <= um) return h * (0.86f + 0.14f * std::sin(u / um * PI * 0.5f));
        const float v = (u - um) / (1 - um);
        return h * std::sqrt(std::max(0.f, 1 - v * v));
    }
    float franc(float u) const { return F * (1 + 0.14f * u * u); }
    float zDe(float u) const { return z0 + Lh * u; }
    float pontY(float x, float u) const {
        const float w = demi(u), k = w > 1e-3f ? std::clamp(x / w, -1.f, 1.f) : 0.f;
        return franc(u) + 0.035f * B * (w / (B * 0.5f)) * (1 - k * k);
    }
    float zA(float u, float y) const {
        const float rake = (moteur ? 0.12f : 0.07f) * Lh, w = std::clamp((u - 0.6f) / 0.4f, 0.f, 1.f);
        const float prof = std::clamp((franc(u) - y) / (franc(u) + T), 0.f, 1.f);
        return zDe(u) - rake * w * w * w * prof;
    }
    float uDe(float z) const { return std::clamp((z - z0) / Lh, 0.f, 1.f); }
    // point du bordé : demi-largeur relative kx, hauteur y
    vec3 bord(float u, float kx, float y) const { return {demi(u) * kx, y, zA(u, y)}; }
};

void coque(Maille& m, const Forme& f, int coulPont, int N) {
    std::vector<std::vector<vec3>> an, pont;
    for (int i = 0; i < N; i++) {
        // couples plus serrés vers l'étrave (là où la forme tourne), aucun à l'arrière quasi droit
        const float u = 1 - std::pow(1 - (float)i / (float)(N - 1), 1.35f), fb = f.franc(u);
        const float k1 = f.moteur ? 0.985f : 0.95f, k2 = f.moteur ? 0.90f : 0.80f;
        // bordé jusqu'à 25 cm sous la flottaison (la carène plus bas ne se voit pas : la mer est opaque), fond plat
        const vec3 p0 = f.bord(u, 1, fb), p1 = f.bord(u, k1, fb * 0.5f), p2 = f.bord(u, k2 * 0.97f, -0.25f);
        auto sym = [](vec3 p) { return vec3{-p.x, p.y, p.z}; };
        an.push_back({p0, p1, p2, sym(p2), sym(p1), sym(p0)});
        pont.push_back({p0, {0, f.pontY(0, u), f.zDe(u)}, sym(p0)});
    }
    loft(m, an, COUL_COQUE, false, {0, f.F * 0.4f, f.z0 - f.Lh}, {0, f.F * 0.4f, f.z0 + 2 * f.Lh});
    loft(m, pont, coulPont, false, {0, -20, f.z0 - f.Lh}, {0, -20, f.z0 + 2 * f.Lh});
    // tableau arrière (au-dessus de l'eau)
    const std::vector<vec3>& r = an.front();
    eventail(m, {r[0], r[1], r[2], r[3], r[4], r[5], {0, f.pontY(0, 0), f.z0}}, COUL_COQUE, {0, 0, -1});
}

// coque allégée (bloc au-delà de 200 m) : 11 triangles
void coqueLoin(Maille& m, const Forme& f) {
    const float um = 0.45f;
    const vec3 S = f.bord(0, 1, f.franc(0)), M = f.bord(um, 1, f.franc(um)), Bt{0, f.franc(1), f.zDe(1)};
    const vec3 Sb = f.bord(0, 0.8f, 0), Mb = f.bord(um, 0.85f, 0), Bb{0, 0, f.zA(1, 0)};
    for (float sd : {-1.f, 1.f}) {
        auto X = [sd](vec3 p) { return vec3{p.x * sd, p.y, p.z}; };
        quadPlat(m, X(S), X(M), X(Mb), X(Sb), COUL_COQUE, {sd, 0, 0});
        quadPlat(m, X(M), Bt, Bb, X(Mb), COUL_COQUE, {sd, 0, 0.3f});
    }
    triPlat(m, S, {-S.x, S.y, S.z}, Bt, COUL_COQUE, {0, 1, 0});
    quadPlat(m, S, {-S.x, S.y, S.z}, {-Sb.x, 0, Sb.z}, Sb, COUL_COQUE, {0, 0, -1});
}

// ---------------------------------------------------------------- voilier de croisière au port (Oceanis 30.1 : 9,6 m)
constexpr float V_L = 9.6f, V_B = 3.4f, V_F = 1.0f, V_T = 0.5f, V_ZMAT = 0.55f;
constexpr float V_HMAT = 1.38f * V_L;   // RG-04 : mât ≈ 1,35 × L au-dessus de l'eau (13 à 14 m) : 13,25 m

Bateau voilier(const Couleurs& c) {
    Bateau b; const Forme f{V_L, V_B, V_F, V_T, -V_L / 2, false};
    b.L = V_L; b.B = V_B; b.T = V_T; b.francPoupe = f.pontY(0, 0); b.francEtrave = f.franc(1); b.demiPoupe = f.demi(0); b.hautMat = V_HMAT;
    Maille& m = b.pres;
    coque(m, f, c.gelcoat, 6);
    // rouf : flancs inclinés, dessus bombé, avant qui plonge vers le pont
    const float zs[4] = {-1.5f, -0.3f, 0.9f, 2.0f}, hs[4] = {0.52f, 0.55f, 0.50f, 0.08f}, ks[4] = {1, 1, 0.95f, 0.78f};
    auto wb = [&](int i) { return std::min(1.12f, 0.70f * f.demi(f.uDe(zs[i]))) * ks[i]; };
    std::vector<std::vector<vec3>> rouf;
    for (int i = 0; i < 4; i++) {
        const float w = wb(i), wt = w * 0.86f, y0 = f.pontY(w, f.uDe(zs[i])) - 0.02f, z = zs[i];
        rouf.push_back({{w, y0, z}, {wt, y0 + hs[i], z}, {0, y0 + hs[i] + 0.05f, z}, {-wt, y0 + hs[i], z}, {-w, y0, z}});
    }
    loft(m, rouf, c.gelcoat, false, {0, 0.6f, -20}, {0, 0.6f, 20});
    eventail(m, rouf.front(), c.gelcoat, {0, 0, -1});   // cloison de la descente
    // vitrages du rouf : bande sombre sur chaque flanc (de 35 à 80 % de la hauteur), 1,2 cm devant le flanc
    auto flanc = [&](float z, float fr, float sd) {   // point du flanc tribord / bâbord, interpolé entre les anneaux
        int i = 0; while (i < 2 && z > zs[i + 1]) i++;
        const float t = std::clamp((z - zs[i]) / (zs[i + 1] - zs[i]), 0.f, 1.f);
        const vec3 a = rouf[(size_t)i][0] + (rouf[(size_t)i + 1][0] - rouf[(size_t)i][0]) * t, h = rouf[(size_t)i][1] + (rouf[(size_t)i + 1][1] - rouf[(size_t)i][1]) * t;
        const vec3 p = a + (h - a) * fr;
        return vec3{sd * (p.x + 0.012f), p.y, p.z};
    };
    constexpr std::array<std::array<float, 2>, 2> PANS{{{-1.1f, -0.3f}, {-0.3f, 0.7f}}};   // deux pans, coupés à l'anneau du rouf
    for (float sd : {-1.f, 1.f}) for (const auto& z : PANS)
        quadPlat(m, flanc(z[0], 0.35f, sd), flanc(z[1], 0.35f, sd), flanc(z[1], 0.8f, sd), flanc(z[0], 0.8f, sd), c.vitre, {sd, 0.4f, 0});
    // cockpit : banquettes et fond en teck entre deux hiloires de 0,30 m (de la cloison de la descente à 0,6 m du tableau)
    auto cockpit = [&](Maille& mc) {
        const float za = -V_L / 2 + 0.6f, zb = zs[0], wc = 0.62f * f.demi(f.uDe(za)), y = f.pontY(0, f.uDe(za)) + 0.01f;
        quadPlat(mc, {wc, y, za}, {-wc, y, za}, {-wc, y, zb}, {wc, y, zb}, c.pont, {0, 1, 0});
        for (float sd : {-1.f, 1.f}) quadPlat(mc, {sd * wc, y - 0.01f, za}, {sd * wc, y - 0.01f, zb}, {sd * wc, y + 0.30f, zb}, {sd * wc, y + 0.30f, za}, c.gelcoat, {sd, 0, 0});
    };
    cockpit(m);
    // filières inox à 0,6 m du pont, du balcon avant au balcon arrière, sur un chandelier au milieu de chaque bord
    for (float sd : {-1.f, 1.f}) {
        const float um = 0.5f, ua = 0.03f, xm = sd * 0.92f * f.demi(um), xa = sd * 0.85f * f.demi(ua);
        const vec3 avant{0, f.pontY(0, 0.97f) + 0.6f, V_L / 2 - 0.35f}, milieu{xm, f.pontY(xm, um) + 0.6f, f.zDe(um)}, arriere{xa, f.pontY(xa, ua) + 0.6f, f.zDe(ua)};
        ligne(m, avant, milieu, 0.02f, c.cable);
        ligne(m, milieu, arriere, 0.02f, c.cable);
        ligne(m, milieu, milieu - vec3{0, 0.6f, 0}, 0.025f, c.cable);
    }
    // mât à section ovale (17 × 10 cm au pied, 12 × 7 cm en tête), posé sur le rouf
    const float yPied = rouf[1][2].y + (rouf[2][2].y - rouf[1][2].y) * ((V_ZMAT - zs[1]) / (zs[2] - zs[1]));
    const std::vector<vec3> pied = ellipse({0, yPied, V_ZMAT}, {1, 0, 0}, {0, 0, 1}, 0.05f, 0.085f, 6);
    const std::vector<vec3> tete = ellipse({0, V_HMAT, V_ZMAT}, {1, 0, 0}, {0, 0, 1}, 0.035f, 0.06f, 6);
    loft(m, {pied, tete}, c.mat, true, {0, yPied, V_ZMAT}, {0, V_HMAT, V_ZMAT});
    eventail(m, tete, c.mat, {0, 1, 0});
    // barres de flèche (56 % de la hauteur, 1,2 m, en flèche de 0,3 m), haubans par les barres jusqu'aux cadènes, pataras
    const float ys = 0.56f * V_HMAT, uc = f.uDe(V_ZMAT - 0.1f);
    for (float sd : {-1.f, 1.f}) {
        const vec3 bout{sd * 1.2f, ys, V_ZMAT - 0.3f}, cadene{sd * 0.92f * f.demi(uc), f.pontY(sd * 0.92f * f.demi(uc), uc), V_ZMAT - 0.1f};
        ligne(m, {sd * 0.04f, ys, V_ZMAT}, bout, 0.045f, c.mat);
        ligne(m, {sd * 0.03f, V_HMAT - 0.35f, V_ZMAT}, bout, 0.02f, c.cable);
        ligne(m, bout, cadene, 0.02f, c.cable);
    }
    ligne(m, {0, V_HMAT - 0.05f, V_ZMAT - 0.07f}, {0, f.pontY(0, 0) + 0.05f, -V_L / 2 + 0.12f}, 0.02f, c.cable);
    // génois enroulé sur l'étai (Ø 0,15 m au plus gros, bande anti-UV de la couleur des toiles), étai au-dessus de la têtière
    const vec3 amure{0, f.pontY(0, 0.97f) + 0.25f, V_L / 2 - 0.3f}, tetiere{0, 0.9f * V_HMAT, V_ZMAT + 0.1f};
    const vec3 ax = normalize(tetiere - amure), e1{1, 0, 0}, e2 = normalize(cross(ax, e1));
    std::vector<std::vector<vec3>> genois;
    const float ts[4] = {0, 0.18f, 0.6f, 1}, rs[4] = {0.05f, 0.075f, 0.06f, 0.022f};
    for (int i = 0; i < 4; i++) genois.push_back(ellipse(amure + (tetiere - amure) * ts[i], e1, e2, rs[i], rs[i], 5));
    loft(m, genois, COUL_TOILE, true, amure, tetiere);
    ligne(m, tetiere, {0, V_HMAT - 0.3f, V_ZMAT + 0.07f}, 0.02f, c.cable);
    // bôme (14 × 9 cm, 3,85 m), à 2,0 m au-dessus du pont (RG-04), légèrement relevée au bout
    const float yb = f.pontY(0, f.uDe(V_ZMAT)) + 2.0f;
    const vec3 vit{0, yb, V_ZMAT - 0.12f}, bout{0, yb + 0.12f, V_ZMAT - 0.12f - 3.85f}, db = normalize(bout - vit);
    const vec3 eh = normalize(cross(db, vec3{1, 0, 0})) * (cross(db, vec3{1, 0, 0}).y < 0 ? -1.f : 1.f);
    loft(m, {ellipse(vit, {1, 0, 0}, eh, 0.045f, 0.07f, 6), ellipse(bout, {1, 0, 0}, eh, 0.045f, 0.07f, 6)}, c.mat, true, vit, bout);
    eventail(m, ellipse(bout, {1, 0, 0}, eh, 0.045f, 0.07f, 6), c.mat, db);
    // sac de grand-voile (lazy-bag) fermé sur la bôme : 3,0 m × 0,36 m × 0,40 m, plus bas vers l'arrière
    const float sec[7][2] = {{0.17f, -0.02f}, {0.18f, 0.20f}, {0.10f, 0.36f}, {0, 0.40f}, {-0.10f, 0.36f}, {-0.18f, 0.20f}, {-0.17f, -0.02f}};
    std::vector<std::vector<vec3>> sac;
    const float ss[2] = {0.25f, 3.25f}, ks2[2] = {1.0f, 0.75f};
    for (int i = 0; i < 2; i++) {
        const vec3 o = vit + db * ss[i] + eh * 0.07f;
        std::vector<vec3> r;
        for (const auto& p : sec) r.push_back(o + vec3{p[0] * ks2[i], 0, 0} + eh * (p[1] * ks2[i]));
        sac.push_back(r);
    }
    loft(m, sac, COUL_TOILE, false, vit + eh * 0.1f, bout + eh * 0.1f);
    eventail(m, sac.back(), COUL_TOILE, db);
    // version moyenne : coque à 4 couples, rouf, cockpit, mât, bôme et sac, génois enroulé (sans filières ni haubans : décision du PO)
    {
        Maille& mm = b.moyen;
        coque(mm, f, c.gelcoat, 4);
        loft(mm, {rouf[0], rouf[3]}, c.gelcoat, false, {0, 0.6f, -20}, {0, 0.6f, 20});
        eventail(mm, rouf.front(), c.gelcoat, {0, 0, -1});
        cockpit(mm);
        loft(mm, {pied, tete}, c.mat, true, {0, yPied, V_ZMAT}, {0, V_HMAT, V_ZMAT});
        loft(mm, {ellipse(vit, {1, 0, 0}, eh, 0.045f, 0.07f, 4), ellipse(bout, {1, 0, 0}, eh, 0.045f, 0.07f, 4)}, c.mat, true, vit, bout);
        const float sec2[5][2] = {{0.17f, -0.02f}, {0.15f, 0.30f}, {0, 0.40f}, {-0.15f, 0.30f}, {-0.17f, -0.02f}};
        std::vector<std::vector<vec3>> sac2;
        for (int i = 0; i < 2; i++) {
            const vec3 o = vit + db * ss[i] + eh * 0.07f;
            std::vector<vec3> r;
            for (const auto& q : sec2) r.push_back(o + vec3{q[0] * ks2[i], 0, 0} + eh * (q[1] * ks2[i]));
            sac2.push_back(r);
        }
        loft(mm, sac2, COUL_TOILE, false, vit + eh * 0.1f, bout + eh * 0.1f);
        eventail(mm, sac2.back(), COUL_TOILE, db);
        loft(mm, {ellipse(amure, e1, e2, 0.07f, 0.07f, 4), ellipse(tetiere, e1, e2, 0.025f, 0.025f, 4)}, COUL_TOILE, true, amure, tetiere);
    }
    // version allégée : coque de 11 triangles, mât en 2 quads croisés
    coqueLoin(b.loin, f);
    ligne(b.loin, {0, f.franc(0.6f), V_ZMAT}, {0, V_HMAT, V_ZMAT}, 0.15f, c.mat);
    return b;
}

// ---------------------------------------------------------------- yacht à moteur de 18 m (flybridge) et vedette
constexpr float Y_L = 18.f, Y_B = 4.9f, Y_F = 1.9f, Y_T = 0.9f, Y_PLAT = 1.0f;

Bateau moteur(const Couleurs& c, bool flybridge) {
    Bateau b; const Forme f{Y_L - Y_PLAT, Y_B, Y_F, Y_T, -Y_L / 2 + Y_PLAT, true};
    b.L = Y_L; b.B = Y_B; b.T = Y_T; b.francPoupe = f.pontY(0, 0); b.francEtrave = f.franc(1); b.demiPoupe = f.demi(0); b.plateforme = Y_PLAT;
    Maille& m = b.pres;
    coque(m, f, c.pont, 6);
    // plateforme de bain (dessus en teck à 0,40 m, épaisseur 0,18 m)
    const float wp = 0.92f * f.demi(0), za = -Y_L / 2, zb = f.z0 + 0.05f;
    quadPlat(m, {wp, 0.40f, za}, {-wp, 0.40f, za}, {-wp, 0.40f, zb}, {wp, 0.40f, zb}, c.pont, {0, 1, 0});
    quadPlat(m, {wp, 0.40f, za}, {-wp, 0.40f, za}, {-wp, 0.22f, za}, {wp, 0.22f, za}, COUL_COQUE, {0, 0, -1});
    for (float sd : {-1.f, 1.f}) quadPlat(m, {sd * wp, 0.40f, za}, {sd * wp, 0.40f, zb}, {sd * wp, 0.22f, zb}, {sd * wp, 0.22f, za}, COUL_COQUE, {sd, 0, 0});
    // superstructure (salon) : murs à rentrée, bande vitrée, pare-brise incliné, toit bombé
    const float zA = f.z0 + 2.6f, zF = f.z0 + 0.66f * f.Lh, ts[4] = {0, 0.4f, 0.72f, 1};
    struct Anneau { float z, w, h, y0; };
    Anneau A[4];
    for (int i = 0; i < 4; i++) {
        const float t = ts[i], z = zA + (zF - zA) * t, u = f.uDe(z);
        const float w = 0.84f * f.demi(u) * (t < 0.72f ? 1.f : 1.f - 0.35f * (t - 0.72f) / 0.28f);
        const float h = t < 0.55f ? 2.05f : 2.05f - 1.25f * (t - 0.55f) / 0.45f;
        A[i] = {z, w, h, f.pontY(w, u) - 0.03f};
    }
    auto P = [&](const Anneau& a, float fr, float sd) { return vec3{sd * a.w * (1 - 0.06f * fr), a.y0 + a.h * fr, a.z}; };
    for (float sd : {-1.f, 1.f}) {
        std::vector<std::vector<vec3>> bas, vitre;
        for (const Anneau& a : A) { bas.push_back({P(a, 0, sd), P(a, 0.42f, sd)}); vitre.push_back({P(a, 0.42f, sd), P(a, 0.88f, sd)}); }
        loft(m, bas, c.gelcoat, false, {0, 2.5f, -20}, {0, 2.5f, 20});
        loft(m, vitre, c.vitre, false, {0, 2.5f, -20}, {0, 2.5f, 20});
    }
    std::vector<std::vector<vec3>> toit;
    for (const Anneau& a : A) toit.push_back({P(a, 0.88f, 1), {0, a.y0 + a.h + 0.08f, a.z}, P(a, 0.88f, -1)});
    loft(m, toit, c.gelcoat, false, {0, 0, -20}, {0, 0, 20});
    for (int e = 0; e < 2; e++) {   // faces arrière (portes vitrées du salon) et avant
        const Anneau& a = A[e ? 3 : 0];
        const vec3 d{0, 0, e ? 1.f : -1.f};
        quadPlat(m, P(a, 0, 1), P(a, 0.42f, 1), P(a, 0.42f, -1), P(a, 0, -1), c.gelcoat, d);
        quadPlat(m, P(a, 0.42f, 1), P(a, 0.88f, 1), P(a, 0.88f, -1), P(a, 0.42f, -1), c.vitre, d);
        triPlat(m, P(a, 0.88f, 1), {0, a.y0 + a.h + 0.08f, a.z}, P(a, 0.88f, -1), c.gelcoat, d);
    }
    // hublots de la coque : bande sombre en deux pans sur chaque bord (55 à 75 % du franc-bord)
    for (float sd : {-1.f, 1.f}) for (int k = 0; k < 2; k++) {
        const float u0 = 0.30f + 0.16f * (float)k, u1 = u0 + 0.16f;
        auto H = [&](float u, float fr) { const float y = f.franc(u) * fr; return vec3{sd * (f.demi(u) * (0.985f + 0.015f * (fr - 0.5f) / 0.5f) + 0.03f), y, f.zA(u, y)}; };
        quadPlat(m, H(u0, 0.55f), H(u1, 0.55f), H(u1, 0.75f), H(u0, 0.75f), c.vitre, {sd, 0, 0});
    }
    // main courante inox à l'avant (0,8 m au-dessus du pont)
    for (float sd : {-1.f, 1.f}) {
        const float u0 = 0.55f, u1 = 0.95f;
        ligne(m, {sd * 0.93f * f.demi(u0), f.pontY(0, u0) + 0.8f, f.zDe(u0)}, {sd * 0.9f * f.demi(u1) + sd * 0.05f, f.pontY(0, u1) + 0.8f, f.zDe(u1)}, 0.03f, c.cable);
    }
    if (flybridge) {   // flybridge sur le toit du salon : console, pare-brise fumé, arceau radar
        std::vector<std::vector<vec3>> fly;
        const float tf[3] = {0.08f, 0.35f, 0.6f};
        float zf[3], wf[3], yf[3];
        for (int i = 0; i < 3; i++) {
            const int k = tf[i] < 0.4f ? 0 : 1; const float t = (tf[i] - ts[k]) / (ts[k + 1] - ts[k]);
            zf[i] = A[k].z + (A[k + 1].z - A[k].z) * t; wf[i] = 0.8f * (A[k].w + (A[k + 1].w - A[k].w) * t);
            yf[i] = A[k].y0 + A[k].h + 0.08f + (A[k + 1].y0 + A[k + 1].h - A[k].y0 - A[k].h) * t;
            fly.push_back({{wf[i], yf[i], zf[i]}, {wf[i] * 0.92f, yf[i] + 0.95f, zf[i]}, {0, yf[i] + 1.0f, zf[i]}, {-wf[i] * 0.92f, yf[i] + 0.95f, zf[i]}, {-wf[i], yf[i], zf[i]}});
        }
        loft(m, fly, c.gelcoat, false, {0, 3.5f, -20}, {0, 3.5f, 20});
        eventail(m, fly.back(), c.gelcoat, {0, 0, 1});
        quadPlat(m, {wf[2] * 0.85f, yf[2] + 0.95f, zf[2] - 0.1f}, {-wf[2] * 0.85f, yf[2] + 0.95f, zf[2] - 0.1f},
                 {-wf[2] * 0.75f, yf[2] + 1.35f, zf[2] - 0.45f}, {wf[2] * 0.75f, yf[2] + 1.35f, zf[2] - 0.45f}, c.vitre, {0, 0.5f, 1});
        const float zr = zf[0] + 0.4f, yr = yf[0] + 0.9f;
        ligne(m, {wf[0] * 0.85f, yr, zr}, {wf[0] * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
        ligne(m, {-wf[0] * 0.85f, yr, zr}, {-wf[0] * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
        ligne(m, {wf[0] * 0.6f, yr + 1.1f, zr - 0.3f}, {-wf[0] * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
    }
    // version moyenne : coque à 4 couples, plateforme, salon à 2 anneaux (murs, vitrage, toit), flybridge à 2 anneaux
    {
        Maille& mm = b.moyen;
        coque(mm, f, c.pont, 4);
        quadPlat(mm, {wp, 0.40f, za}, {-wp, 0.40f, za}, {-wp, 0.40f, zb}, {wp, 0.40f, zb}, c.pont, {0, 1, 0});
        quadPlat(mm, {wp, 0.40f, za}, {-wp, 0.40f, za}, {-wp, 0.22f, za}, {wp, 0.22f, za}, COUL_COQUE, {0, 0, -1});
        for (float sd : {-1.f, 1.f}) {
            loft(mm, {{P(A[0], 0, sd), P(A[0], 0.42f, sd)}, {P(A[3], 0, sd), P(A[3], 0.42f, sd)}}, c.gelcoat, false, {0, 2.5f, -20}, {0, 2.5f, 20});
            loft(mm, {{P(A[0], 0.42f, sd), P(A[0], 0.88f, sd)}, {P(A[3], 0.42f, sd), P(A[3], 0.88f, sd)}}, c.vitre, false, {0, 2.5f, -20}, {0, 2.5f, 20});
        }
        loft(mm, {toit[0], toit[3]}, c.gelcoat, false, {0, 0, -20}, {0, 0, 20});
        for (int e = 0; e < 2; e++) {
            const Anneau& a = A[e ? 3 : 0];
            const vec3 d{0, 0, e ? 1.f : -1.f};
            quadPlat(mm, P(a, 0, 1), P(a, 0.42f, 1), P(a, 0.42f, -1), P(a, 0, -1), c.gelcoat, d);
            quadPlat(mm, P(a, 0.42f, 1), P(a, 0.88f, 1), P(a, 0.88f, -1), P(a, 0.42f, -1), c.vitre, d);
        }
        if (flybridge) {
            const float t0 = 0.08f / ts[1], z0 = A[0].z + (A[1].z - A[0].z) * t0, w0 = 0.8f * (A[0].w + (A[1].w - A[0].w) * t0);
            const float y0 = A[0].y0 + A[0].h + 0.08f, t2 = (0.6f - ts[1]) / (ts[2] - ts[1]);
            const float z2 = A[1].z + (A[2].z - A[1].z) * t2, w2 = 0.8f * (A[1].w + (A[2].w - A[1].w) * t2);
            const float y2 = A[1].y0 + A[1].h + 0.08f + (A[2].y0 + A[2].h - A[1].y0 - A[1].h) * t2;
            auto an = [](float w, float y, float z) { return std::vector<vec3>{{w, y, z}, {w * 0.92f, y + 0.95f, z}, {0, y + 1.0f, z}, {-w * 0.92f, y + 0.95f, z}, {-w, y, z}}; };
            loft(mm, {an(w0, y0, z0), an(w2, y2, z2)}, c.gelcoat, false, {0, 3.5f, -20}, {0, 3.5f, 20});
            eventail(mm, an(w2, y2, z2), c.gelcoat, {0, 0, 1});
            const float zr = z0 + 0.4f, yr = y0 + 0.9f;   // arceau radar, comme le détail
            ligne(mm, {w0 * 0.85f, yr, zr}, {w0 * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
            ligne(mm, {-w0 * 0.85f, yr, zr}, {-w0 * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
            ligne(mm, {w0 * 0.6f, yr + 1.1f, zr - 0.3f}, {-w0 * 0.6f, yr + 1.1f, zr - 0.3f}, 0.14f, c.gelcoat);
        }
    }
    // version allégée : coque de 11 triangles, salon en prisme incliné
    coqueLoin(b.loin, f);
    const float h0 = A[0].y0, w0 = A[0].w * 0.9f, w1 = A[3].w * 0.9f, hT = A[0].h + (flybridge ? 1.0f : 0.f);
    const vec3 a0{w0, h0, A[0].z}, a1{-w0, h0, A[0].z}, a2{-w0, h0 + hT, A[0].z}, a3{w0, h0 + hT, A[0].z};
    const vec3 b0{w1, h0, A[3].z}, b1{-w1, h0, A[3].z}, b2{-w1 * 0.8f, h0 + A[3].h, A[3].z - 1.0f}, b3{w1 * 0.8f, h0 + A[3].h, A[3].z - 1.0f};
    quadPlat(b.loin, a3, a2, b2, b3, c.gelcoat, {0, 1, 0});
    quadPlat(b.loin, a0, a3, b3, b0, c.gelcoat, {1, 0, 0});
    quadPlat(b.loin, a1, a2, b2, b1, c.gelcoat, {-1, 0, 0});
    quadPlat(b.loin, b0, b3, b2, b1, c.vitre, {0, 0.4f, 1});
    quadPlat(b.loin, a0, a3, a2, a1, c.gelcoat, {0, 0, -1});
    return b;
}

} // namespace

Bateau construire(Modele m, const Couleurs& c) {
    switch (m) {
    case VOILIER: return voilier(c);
    case YACHT: return moteur(c, true);
    default: return moteur(c, false);   // vedette : posée à l'échelle 0,6 (10 à 12 m), sans flybridge (RG-05)
    }
}

int trianglesAmarrage(Modele m, bool aQuai) {
    // à quai : 2 amarres arrière (2 × 4) ; voilier : pendille à l'avant (4) ; moteur : passerelle (planche 2 + main courante 4)
    // au mouillage : la ligne de mouillage seulement (4)
    if (!aQuai) return 4;
    return m == VOILIER ? 12 : 14;
}

Maille amarrage(const Bateau& b, Modele m, float s, float ecart, float yQuai, const Couleurs& c) {
    Maille r;
    const float L = b.L * s, zp = -L / 2;
    const vec3 etrave{0, b.francEtrave * s, L / 2 - 0.1f * s};
    if (ecart < 0) { ligne(r, etrave, {0, -0.2f, L / 2 + 5.f}, 0.025f, c.amarre); return r; }   // ligne de mouillage
    // deux amarres croisées de la hanche au quai (cordage blanc), 0,3 m dans le quai
    const float xq = 0.85f * b.demiPoupe * s, yq = b.francPoupe * s, zq = zp + (b.plateforme + 0.15f) * s;
    for (float sd : {-1.f, 1.f}) ligne(r, {sd * xq, yq, zq}, {sd * (xq + 0.6f), yQuai + 0.05f, zp - ecart - 0.3f}, 0.025f, c.corde);
    if (m == VOILIER) { ligne(r, etrave, {0, -0.2f, L / 2 + 4.f}, 0.025f, c.amarre); return r; }   // pendille vers le corps-mort
    // passerelle de poupe (0,5 m de large) de la plateforme au quai, main courante d'un côté
    const vec3 p0{0, 0.42f * s, zp + 0.2f * s}, p1{0, yQuai + 0.03f, zp - ecart - 0.5f};
    quadPlat(r, p0 + vec3{0.25f, 0, 0}, p1 + vec3{0.25f, 0, 0}, p1 + vec3{-0.25f, 0, 0}, p0 + vec3{-0.25f, 0, 0}, c.mat, {0, 1, 0});
    ligne(r, p0 + vec3{0.25f, 0.85f, 0}, p1 + vec3{0.25f, 0.85f, 0}, 0.03f, c.mat);
    return r;
}

Maille bitte(const Couleurs& c) {
    Maille m;
    // bitte en fonte : semelle, fût étranglé, tête à rebord (profil d'une bitte de quai de plaisance, Ø 0,38 m à la base)
    tour(m, {0, 0, 0}, {{0.19f, 0}, {0.11f, 0.10f}, {0.12f, 0.38f}, {0.16f, 0.46f}}, 6, c.fonte);
    return m;
}

Maille ponton(float l, float hautQuai, const Couleurs& c, bool loin) {
    Maille m;
    const float W = PONTON_LARGEUR / 2, H = PONTON_HAUT;
    for (float sd : {-1.f, 1.f}) {   // lisses de bois (0,25 m), sous un bord chanfreiné
        loft(m, {{{sd * W, 0.20f, 0}, {sd * W, H - 0.05f, 0}}, {{sd * W, 0.20f, l}, {sd * W, H - 0.05f, l}}}, c.bois, false, {0, 0.3f, -20}, {0, 0.3f, 20});
        if (!loin) loft(m, {{{sd * (W - 0.1f), -0.2f, 0}, {sd * (W - 0.1f), 0.20f, 0}}, {{sd * (W - 0.1f), -0.2f, l}, {sd * (W - 0.1f), 0.20f, l}}}, c.flotteur, false, {0, 0, -20}, {0, 0, 20});
    }
    loft(m, {{{W, H - 0.05f, 0}, {W - 0.05f, H, 0}, {-W + 0.05f, H, 0}, {-W, H - 0.05f, 0}},
             {{W, H - 0.05f, l}, {W - 0.05f, H, l}, {-W + 0.05f, H, l}, {-W, H - 0.05f, l}}}, c.beton, false, {0, -5, -20}, {0, -5, 20});
    if (loin) return m;
    eventail(m, {{W, 0.20f, l}, {W, H - 0.05f, l}, {W - 0.05f, H, l}, {-W + 0.05f, H, l}, {-W, H - 0.05f, l}, {-W, 0.20f, l}}, c.bois, {0, 0, 1});
    // borne de distribution (eau, électricité) : colonne à angles abattus, tête plus large, au milieu du ponton
    int k = 0;
    for (float z = l * 0.5f; z < l - 3; z += 18, k++) {   // une borne, au milieu du ponton
        const float x = (k & 1 ? -1.f : 1.f) * 0.72f;
        std::vector<std::vector<vec3>> an;
        const float cotes[2] = {0.11f, 0.13f}, hs[2] = {H, H + 1.0f};   // fût qui s'évase vers la tête
        for (int i = 0; i < 2; i++) {
            const float a = cotes[i], ch = 0.035f;
            an.push_back({{x + a, hs[i], z - a + ch}, {x + a, hs[i], z + a - ch}, {x + a - ch, hs[i], z + a}, {x - a + ch, hs[i], z + a},
                          {x - a, hs[i], z + a - ch}, {x - a, hs[i], z - a + ch}, {x - a + ch, hs[i], z - a}, {x + a - ch, hs[i], z - a}});
        }
        loft(m, an, c.gelcoat, true, {x, H, z}, {x, H + 1.0f, z});
        eventail(m, an.back(), c.gelcoat, {0, 1, 0});
    }
    // pieu de guidage galvanisé (Ø 0,45 m, 2,4 m au-dessus de l'eau), chapeau conique, bride vers le ponton
    const vec3 pieu{W + 0.35f, 0, l - 0.6f};
    tour(m, pieu + vec3{0, -0.3f, 0}, {{0.22f, 0}, {0.22f, 2.55f}, {0.25f, 2.62f}, {0.06f, 2.85f}}, 6, c.cable);
    ligne(m, {W - 0.05f, 0.35f, l - 0.6f}, pieu + vec3{-0.2f, 0.35f, 0}, 0.12f, c.cable);
    // passerelle articulée (aluminium, 0,9 m de large) du bord du quai au ponton, mains courantes à 0,9 m
    const vec3 q0{0, hautQuai + 0.03f, -1.0f}, q1{0, H + 0.03f, 3.5f};
    quadPlat(m, q0 + vec3{0.45f, 0, 0}, q1 + vec3{0.45f, 0, 0}, q1 + vec3{-0.45f, 0, 0}, q0 + vec3{-0.45f, 0, 0}, c.mat, {0, 1, 0});
    for (float sd : {-1.f, 1.f}) ligne(m, q0 + vec3{sd * 0.45f, 0.9f, 0}, q1 + vec3{sd * 0.45f, 0.9f, 0}, 0.04f, c.mat);
    return m;
}

} // namespace marina
} // namespace wfs
