// WaForSpeed — WFS-370 : détails d'architecture du kit (voir archi_details.h).
//
// Mesures (sources) :
// - garde-corps : hauteur 1,00 m, vide entre barreaux ≤ 0,11 m (NF P01-012, archi.garde_corps.hauteur du référentiel) ;
//   main courante 50 × 40 mm, barreaux de 16 mm (fer carré courant des garde-corps de serrurerie) ;
// - comble à la Mansart : brisis presque vertical (60 à 75°, ici 70°) haut d'un niveau habitable, terrasson à faible pente
//   (15 à 25°, ici 20°) couvert de zinc ; lucarnes à fronton dans le brisis, une par travée, dans l'axe des fenêtres ;
// - souches de cheminée en maçonnerie sur les murs pignons, couronnement en débord de 5 cm, mitrons de terre cuite ;
// - corniche : larmier en débord égal à la saillie du style, moulures en ressauts au-dessous (doucine, cordon) ;
// - soubassement de pierre de 0,6 m en saillie de 3 cm, cordon de 18 cm sous le plancher du 1er étage ;
// - couvertine d'acrotère : tôle pliée débordant de 3 cm de chaque côté.
// Les valeurs « de conception » (sans mesure publiée accessible, réseau limité) sont dites comme telles.
#include "archi_details.h"
#include <algorithm>
#include <cmath>

namespace wfs::archi::details {
namespace {

constexpr float GC_H = 1.0f;                       // NF P01-012
constexpr float MAIN_L = 0.05f, MAIN_H = 0.04f;    // main courante
constexpr float BARREAU = 0.016f, PAS_BARREAU = 0.12f;   // vide 0,104 m ≤ 0,11 m
constexpr float LISSE = 0.03f;
constexpr float BRISIS_DEG = 70.f, TERRASSON_DEG = 20.f;
constexpr float LUC_RETRAIT = 0.15f;               // nu de la lucarne derrière le nu de la façade
constexpr float LUC_ALLEGE = 0.30f, LUC_TABLEAU = 0.08f, LUC_LINTEAU = 0.20f, LUC_PENTE_DEG = 40.f, LUC_DEBORD = 0.08f;
constexpr float SOUCHE_L = 0.5f, SOUCHE_P = 1.8f, SOUCHE_HAUT = 1.0f;   // souche : 0,5 × 1,8 m, 1,0 m au-dessus du faîtage
constexpr float COUVERTINE = 0.03f;

// uv d'une face plane : mètres, projetés sur le plan le plus proche (murs : x ou z le long, y en hauteur)
vec2 uvPlan(vec3 p, vec3 n) {
    if (std::fabs(n.y) > 0.7f) return {p.x, p.z};
    if (std::fabs(n.x) > std::fabs(n.z)) return {n.x > 0 ? -p.z : p.z, p.y};
    return {n.z > 0 ? p.x : -p.x, p.y};
}
// quadrilatère plan a b c d ; la normale est prise du côté de « dehors » (l'ordre est corrigé si besoin)
void tri(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 dehors);
void face(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 d, vec3 dehors) {
    // sommets confondus (croupe réduite à une pointe…) : un seul triangle, jamais de triangle vide
    const auto pres = [](vec3 u, vec3 v) { return length(u - v) < 1e-5f; };
    if (pres(c, d) || pres(d, a)) { tri(m, a, b, c, dehors); return; }
    if (pres(a, b) || pres(b, c)) { tri(m, a, c, d, dehors); return; }
    vec3 n = cross(b - a, c - a);
    if (length(n) < 1e-9f) n = cross(c - a, d - a);
    if (length(n) < 1e-9f) return;
    n = normalize(n);
    if (dot(n, dehors) < 0) { std::swap(b, d); n = n * -1.f; }
    const uint32_t k = (uint32_t)m.v.size();
    for (vec3 p : {a, b, c, d}) m.v.push_back({p, n, uvPlan(p, n)});
    m.i.insert(m.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}
void tri(Maillage& m, vec3 a, vec3 b, vec3 c, vec3 dehors) {
    vec3 n = cross(b - a, c - a);
    if (length(n) < 1e-9f) return;
    n = normalize(n);
    if (dot(n, dehors) < 0) { std::swap(b, c); n = n * -1.f; }
    const uint32_t k = (uint32_t)m.v.size();
    for (vec3 p : {a, b, c}) m.v.push_back({p, n, uvPlan(p, n)});
    m.i.insert(m.i.end(), {k, k + 1, k + 2});
}
// pavé aligné : bits de faces (1 avant +z, 2 arrière −z, 4 gauche −x, 8 droite +x, 16 dessus, 32 dessous)
void pave(Maillage& m, float x0, float x1, float y0, float y1, float z0, float z1, int faces = 63) {
    if (x1 - x0 < 1e-4f || y1 - y0 < 1e-4f || std::fabs(z1 - z0) < 1e-4f) return;
    if (z0 > z1) std::swap(z0, z1);
    if (faces & 1) face(m, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1});
    if (faces & 2) face(m, {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1});
    if (faces & 4) face(m, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0});
    if (faces & 8) face(m, {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0});
    if (faces & 16) face(m, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0, 1, 0});
    if (faces & 32) face(m, {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}, {0, -1, 0});
}

// barreaux verticaux d'une file, de a à b (dans le plan y), plancher y, hauteur h
void barreaux(Maillage& m, vec3 a, vec3 b, float y, float h) {
    const vec3 d = b - a; const float L = length(vec3{d.x, 0, d.z});
    if (L < 0.05f) return;
    const int n = std::max(1, (int)std::ceil((L - 0.06f) / PAS_BARREAU));
    const float e = BARREAU / 2;
    const int faces = std::fabs(d.x) > std::fabs(d.z) ? (1 | 2) : (4 | 8);   // les deux faces vues de la rue et de la pièce (le chant de 16 mm ne se voit pas)
    for (int k = 0; k < n; k++) {
        const float t = (0.03f + (L - 0.06f) * (k + 0.5f) / n) / L;
        const vec3 c = a + d * t;
        pave(m, c.x - e, c.x + e, y, y + h, c.z - e, c.z + e, faces);
    }
}

// mitron de terre cuite : prisme à 8 pans légèrement conique (r0 en bas, r1 en haut), collerette en haut
void mitron(Maillage& m, vec3 c, float r0, float r1, float h) {
    constexpr int N = 8;
    auto anneau = [&](float ya, float yb, float ra, float rb) {
        for (int k = 0; k < N; k++) {
            const float a0 = 2 * PI * (k + 0.5f) / N, a1 = 2 * PI * (k + 1.5f) / N, am = (a0 + a1) / 2;
            const vec3 p0{c.x + std::cos(a0) * ra, ya, c.z + std::sin(a0) * ra}, p1{c.x + std::cos(a1) * ra, ya, c.z + std::sin(a1) * ra};
            const vec3 q1{c.x + std::cos(a1) * rb, yb, c.z + std::sin(a1) * rb}, q0{c.x + std::cos(a0) * rb, yb, c.z + std::sin(a0) * rb};
            face(m, p0, p1, q1, q0, {std::cos(am), 0, std::sin(am)});
        }
    };
    anneau(c.y, c.y + h, r0, r1);
    anneau(c.y + h - 0.06f, c.y + h, r1 + 0.025f, r1 + 0.025f);           // collerette
    for (int k = 0; k < N; k++) {                                          // dessus de la collerette (conduit sombre au centre : 0,6 r)
        const float a0 = 2 * PI * (k + 0.5f) / N, a1 = 2 * PI * (k + 1.5f) / N;
        const float ro = r1 + 0.025f, ri = r1 * 0.6f, y = c.y + h;
        face(m, {c.x + std::cos(a0) * ri, y, c.z + std::sin(a0) * ri}, {c.x + std::cos(a1) * ri, y, c.z + std::sin(a1) * ri},
             {c.x + std::cos(a1) * ro, y, c.z + std::sin(a1) * ro}, {c.x + std::cos(a0) * ro, y, c.z + std::sin(a0) * ro}, {0, 1, 0});
    }
}

// uv d'une vitre pour la pièce simulée derrière (decor.glsl, interieurFenetre) : même codage que archi_geo.cpp (uvVitre)
void uvVitre(Maillage& m, uint32_t k, float x0, float y0, float l, float h, float allege, int graine) {
    const float G = 100.f * (float)(1 + graine), L = std::round(std::min(l, 9.9f) * 10), H = std::round(std::min(h, 9.9f) * 10), A = std::round(std::clamp(allege, 0.f, 9.9f) * 10);
    for (uint32_t j = k; j < m.v.size(); j++) {
        const float u = std::clamp((m.v[j].p.x - x0) / l, 0.f, 1.f) * 0.999f, v = std::clamp((m.v[j].p.y - y0) / h, 0.f, 1.f) * 0.999f;
        m.v[j].uv = {L * 100 + A + u, G + H + v};
    }
}

}  // namespace

void gardeCorps(Maillage& m, Maillage& fins, float x0, float x1, float y, float z0, float z1) {
    const float yh = y + GC_H, ym = yh - MAIN_H, yl = y + 0.06f;
    const float zf = z1 - MAIN_L / 2;   // axe de la file avant
    pave(m, x0, x1, ym, yh, z1 - MAIN_L, z1);                                   // main courante
    pave(m, x0, x1, yl, yl + LISSE, zf - LISSE / 2, zf + LISSE / 2);            // lisse basse
    barreaux(fins, {x0, 0, zf}, {x1, 0, zf}, y, ym - y);
    if (z1 - z0 < 0.2f) return;
    for (float x : {x0, x1}) {                                                   // retours de côté, scellés au mur
        const float xa = x == x0 ? x0 : x1 - MAIN_L, xc = xa + MAIN_L / 2;
        pave(m, xa, xa + MAIN_L, ym, yh, z0, z1 - MAIN_L, 63 & ~1);
        pave(m, xc - LISSE / 2, xc + LISSE / 2, yl, yl + LISSE, z0, zf - LISSE / 2, 63 & ~1);
        barreaux(fins, {xc, 0, z0}, {xc, 0, zf - MAIN_L / 2}, y, ym - y);
    }
}

void corniche(Maillage& m, float L, float D, float H, float h, float s) {
    // profil, de bas en haut : (haut de la moulure en fraction de h, saillie en fraction de s)
    static constexpr float PROFIL[][2] = {{0.16f, 0.22f}, {0.30f, 0.40f}, {0.42f, 0.58f}, {0.52f, 0.74f}, {0.82f, 1.0f}, {1.0f, 0.86f}};
    float y = H - h;
    for (const auto& q : PROFIL) {
        const float yb = H - h + h * q[0], sa = s * q[1];
        pave(m, -sa, L + sa, y, yb, -D, sa, 63 & ~2);   // dessous visible (ressaut), dos contre le mur
        y = yb;
    }
}

// WFS-455 : lucarnes à fronton du brisis avant de p (code de WFS-370 déplacé tel quel) ; y1 : haut du brisis
void lucarnes(const Plan& p, int lod, float y1, Batiment& out) {
    const float H = p.hauteur(), tb = std::tan(BRISIS_DEG * PI / 180.f);
    Maillage& t = out.part[TOIT];
    // lucarnes à fronton, une par travée, dans l'axe des fenêtres d'étage
    float fenL = 1.1f;
    for (const Baie& b : p.baies) if (b.niveau == 1) { fenL = b.l; break; }
    const float w = std::clamp(fenL + 0.3f, 0.9f, p.travee - 0.4f), lw = w - 0.3f;
    const float zf = -LUC_RETRAIT, y0 = H + LUC_RETRAIT * tb, yA = y0 + LUC_ALLEGE;
    const float pente = std::tan(LUC_PENTE_DEG * PI / 180.f), monte = (w / 2 + LUC_DEBORD) * pente;
    const float lh = std::min(1.3f, y1 - 0.08f - yA - LUC_LINTEAU - monte);
    if (lh > 0.6f && p.travee >= 1.2f) {
        Maillage& pierre = out.part[ENCADREMENT];
        Maillage& vitre = out.part[VITRE];
        const float ytop = yA + lh + LUC_LINTEAU, yf = ytop + monte, zb = -(yf - H) / tb - 0.05f;
        for (int k = 0; k < p.travees; k++) {
            const float cx = p.axe(k), xl = cx - w / 2, xr = cx + w / 2, wl = cx - lw / 2, wr = cx + lw / 2;
            // nu de la lucarne percé de la fenêtre, tableaux, vitre au fond
            face(pierre, {xl, y0, zf}, {wl, y0, zf}, {wl, ytop, zf}, {xl, ytop, zf}, {0, 0, 1});
            face(pierre, {wr, y0, zf}, {xr, y0, zf}, {xr, ytop, zf}, {wr, ytop, zf}, {0, 0, 1});
            face(pierre, {wl, y0, zf}, {wr, y0, zf}, {wr, yA, zf}, {wl, yA, zf}, {0, 0, 1});
            face(pierre, {wl, yA + lh, zf}, {wr, yA + lh, zf}, {wr, ytop, zf}, {wl, ytop, zf}, {0, 0, 1});
            tri(pierre, {xl - LUC_DEBORD / 2, ytop, zf}, {xr + LUC_DEBORD / 2, ytop, zf}, {cx, ytop + (w / 2) * pente, zf}, {0, 0, 1});   // fronton
            const float zv = zf - LUC_TABLEAU;
            face(pierre, {wl, yA, zf}, {wl, yA, zv}, {wl, yA + lh, zv}, {wl, yA + lh, zf}, {1, 0, 0});
            face(pierre, {wr, yA, zv}, {wr, yA, zf}, {wr, yA + lh, zf}, {wr, yA + lh, zv}, {-1, 0, 0});
            face(pierre, {wl, yA + lh, zf}, {wr, yA + lh, zf}, {wr, yA + lh, zv}, {wl, yA + lh, zv}, {0, -1, 0});
            face(pierre, {wl, yA, zv}, {wr, yA, zv}, {wr, yA, zf}, {wl, yA, zf}, {0, 1, 0});
            const uint32_t kv = (uint32_t)vitre.v.size();
            face(vitre, {wl, yA, zv}, {wr, yA, zv}, {wr, yA + lh, zv}, {wl, yA + lh, zv}, {0, 0, 1});
            uvVitre(vitre, kv, wl, yA, lw, lh, LUC_ALLEGE + 0.6f, graineBaie(p, p.etages + 1, k));
            if (lod == 0) {
                pave(pierre, wl - 0.04f, wr + 0.04f, yA - 0.05f, yA, zf - 0.02f, zf + 0.05f);                   // appui
                pave(pierre, xl - 0.06f, xl + 0.06f, y0, ytop, zf, zf + 0.04f, 63 & ~2);                      // pilastres du nu
                pave(pierre, xr - 0.06f, xr + 0.06f, y0, ytop, zf, zf + 0.04f, 63 & ~2);
            }
            // jouées (zinc) : du nu de la lucarne au brisis
            tri(t, {xl, y0, zf}, {xl, ytop, zf}, {xl, ytop, -(ytop - H) / tb}, {-1, 0, 0});
            tri(t, {xr, y0, zf}, {xr, ytop, -(ytop - H) / tb}, {xr, ytop, zf}, {1, 0, 0});
            // toit à deux pans de la lucarne, en débord sur le fronton, rentré dans le brisis
            const float xe0 = xl - LUC_DEBORD, xe1 = xr + LUC_DEBORD, ze = zf + LUC_DEBORD, ye = ytop - LUC_DEBORD * pente * 0.f;
            face(t, {xe0, ye, ze}, {cx, yf, ze}, {cx, yf, zb}, {xe0, ye, zb}, {-pente, 1, 0});
            face(t, {cx, yf, ze}, {xe1, ye, ze}, {xe1, ye, zb}, {cx, yf, zb}, {pente, 1, 0});
            face(t, {xe0, ye - 0.04f, ze}, {xe0, ye, ze}, {cx, yf, ze}, {cx, yf - 0.04f, ze}, {0, 0, 1});      // rive avant
            face(t, {cx, yf - 0.04f, ze}, {cx, yf, ze}, {xe1, ye, ze}, {xe1, ye - 0.04f, ze}, {0, 0, 1});
            face(t, {xe0, ye - 0.04f, zb}, {cx, yf - 0.04f, zb}, {cx, yf - 0.04f, ze}, {xe0, ye - 0.04f, ze}, {pente, -1, 0});   // sous-face
            face(t, {cx, yf - 0.04f, zb}, {xe1, ye - 0.04f, zb}, {xe1, ye - 0.04f, ze}, {cx, yf - 0.04f, ze}, {-pente, -1, 0});
        }
    }
}

void mansart(const Plan& p, float D, int lod, Batiment& out) {
    const float H = p.hauteur(), L = p.largeur;
    const float tb = std::tan(BRISIS_DEG * PI / 180.f), tt = std::tan(TERRASSON_DEG * PI / 180.f);
    float hb = p.hEtage, e = hb / tb;
    if (2 * e > std::min(L, D) - 1.f) { e = std::max(0.2f, (std::min(L, D) - 1.f) / 2); hb = e * tb; }
    const float y1 = H + hb, zr = -D / 2, hr = tt * (D / 2 - e), yr = y1 + hr;
    float xa = e + (D / 2 - e), xb = L - e - (D / 2 - e);
    if (xa > xb) xa = xb = L / 2;
    Maillage& t = out.part[TOIT];
    // brisis (4 pans) puis terrasson (2 longs pans et 2 croupes)
    face(t, {0, H, 0}, {L, H, 0}, {L - e, y1, -e}, {e, y1, -e}, {0, 1, 1});
    face(t, {L, H, -D}, {0, H, -D}, {e, y1, -D + e}, {L - e, y1, -D + e}, {0, 1, -1});
    face(t, {0, H, -D}, {0, H, 0}, {e, y1, -e}, {e, y1, -D + e}, {-1, 1, 0});
    face(t, {L, H, 0}, {L, H, -D}, {L - e, y1, -D + e}, {L - e, y1, -e}, {1, 1, 0});
    face(t, {e, y1, -e}, {L - e, y1, -e}, {xb, yr, zr}, {xa, yr, zr}, {0, 1, 1});
    face(t, {L - e, y1, -D + e}, {e, y1, -D + e}, {xa, yr, zr}, {xb, yr, zr}, {0, 1, -1});
    tri(t, {e, y1, -D + e}, {e, y1, -e}, {xa, yr, zr}, {-1, 1, 0});
    tri(t, {L - e, y1, -e}, {L - e, y1, -D + e}, {xb, yr, zr}, {1, 1, 0});
    if (lod < 2) {
        lucarnes(p, lod, y1, out);
        // WFS-455 (ANO-254) : lucarnes aussi sur le brisis arrière, dans l'axe des fenêtres de la façade sur cour, tournées de 180°
        // (niveau 0 seulement, RG-04 ; au loin le brisis arrière garde son zinc, comme vu depuis la cour à plus de 120 m)
        if (lod == 0) {
            Batiment r;
            lucarnes(facadeSecondaire(p, L, 1), lod, y1, r);
            for (int k = 0; k < NB_PARTS; k++) {
                Maillage& dst = out.part[k];
                const uint32_t base = (uint32_t)dst.v.size();
                for (const Sommet& v : r.part[k].v) dst.v.push_back({{L - v.p.x, v.p.y, -D - v.p.z}, {-v.n.x, v.n.y, -v.n.z}, v.uv});
                for (uint32_t i : r.part[k].i) dst.i.push_back(base + i);
            }
        }
    }
    // souches de cheminée sur les deux murs pignons (maçonnerie, couronnement, mitrons)
    if (L < 5.f || D < 4.f) return;
    for (int c = 0; c < 2; c++) {
        const float xa0 = c ? L - 0.1f - SOUCHE_L : 0.1f, xa1 = xa0 + SOUCHE_L, xm = (xa0 + xa1) / 2;
        const float pz = std::min(SOUCHE_P, D - 2 * e), za = zr - pz / 2, zc = zr + pz / 2, yh = yr + SOUCHE_HAUT;
        pave(out.part[MUR], xa0, xa1, H + 0.5f, yh, za, zc, 63 & ~32);
        pave(out.part[ENCADREMENT], xa0 - 0.05f, xa1 + 0.05f, yh, yh + 0.1f, za - 0.05f, zc + 0.05f);
        if (lod == 0) {
            const int n = std::max(1, (int)std::floor(pz / 0.45f));
            for (int k = 0; k < n; k++) mitron(out.part[ENCADREMENT], {xm, yh + 0.1f, za + pz * (k + 0.5f) / n}, 0.12f, 0.10f, 0.4f);
        }
    }
}

void terrasse(const Plan& p, float D, int lod, Batiment& out) {
    constexpr float ACRO = 0.5f, ACRO_EP = 0.2f;   // acrotère (archi_geo.cpp)
    const float H = p.hauteur(), L = p.largeur;
    Maillage& toit = out.part[TOIT];
    Maillage& mur = out.part[MUR];
    face(toit, {0, H, 0}, {L, H, 0}, {L, H, -D}, {0, H, -D}, {0, 1, 0});
    pave(mur, 0, L, H, H + ACRO, -ACRO_EP, 0, 63 & ~32);
    pave(mur, 0, L, H, H + ACRO, -D, -D + ACRO_EP, 63 & ~32);
    pave(mur, 0, ACRO_EP, H, H + ACRO, -D, 0, 63 & ~32);
    pave(mur, L - ACRO_EP, L, H, H + ACRO, -D, 0, 63 & ~32);
    // WFS-455 (ANO-254) : l'édicule se voit aussi de loin (volume et dalle à tous les niveaux ; porte et ventilation de près)
    const bool edicule = p.edicules && L > 6 && D > 6;
    const float cx = L * 0.5f, cz = -D * 0.6f, x0 = cx - 1.6f, x1 = cx + 1.6f, z0 = cz - 1.4f, z1 = cz + 1.4f, yh = H + 2.6f;
    if (edicule && lod >= 1) {
        pave(mur, x0, x1, H, yh, z0, z1, 63 & ~(16 | 32));
        pave(out.part[ENCADREMENT], x0 - 0.15f, x1 + 0.15f, yh, yh + 0.2f, z0 - 0.15f, z1 + 0.15f);
        face(out.part[MENUISERIE], {cx - 0.45f, H + 0.05f, z1 + 0.005f}, {cx + 0.45f, H + 0.05f, z1 + 0.005f}, {cx + 0.45f, H + 2.15f, z1 + 0.005f},
             {cx - 0.45f, H + 2.15f, z1 + 0.005f}, {0, 0, 1});
    }
    if (lod >= 2) return;
    Maillage& met = out.part[METAL];   // couvertines (tôle pliée) sur les 4 côtés
    const float c = COUVERTINE, y = H + ACRO;
    pave(met, -c, L + c, y, y + 0.02f, -ACRO_EP - c, c);
    pave(met, -c, L + c, y, y + 0.02f, -D - c, -D + ACRO_EP + c);
    pave(met, -c, ACRO_EP + c, y, y + 0.02f, -D + ACRO_EP, -ACRO_EP);
    pave(met, L - ACRO_EP - c, L + c, y, y + 0.02f, -D + ACRO_EP, -ACRO_EP);
    if (!(edicule && lod == 0)) return;
    // édicule (cage d'escalier et machinerie) : 3,2 × 2,8 m, 2,6 m ; dalle en débord, porte métallique, grille de ventilation
    pave(mur, x0, x1, H, yh, z0, z1, 63 & ~(16 | 32));
    pave(out.part[ENCADREMENT], x0 - 0.15f, x1 + 0.15f, yh, yh + 0.2f, z0 - 0.15f, z1 + 0.15f);
    face(out.part[MENUISERIE], {cx - 0.45f, H + 0.05f, z1 + 0.005f}, {cx + 0.45f, H + 0.05f, z1 + 0.005f}, {cx + 0.45f, H + 2.15f, z1 + 0.005f},
         {cx - 0.45f, H + 2.15f, z1 + 0.005f}, {0, 0, 1});
    pave(met, cx - 0.5f, cx + 0.5f, H + 2.15f, H + 2.2f, z1, z1 + 0.04f);                       // linteau métallique
    pave(met, cx + 0.33f, cx + 0.39f, H + 1.0f, H + 1.06f, z1 + 0.005f, z1 + 0.05f);            // poignée
    for (int k = 0; k < 6; k++) {                                                                 // lames de la ventilation
        const float yb = H + 1.5f + k * 0.12f;
        face(met, {x1 + 0.005f, yb, cz - 0.4f}, {x1 + 0.005f, yb, cz + 0.4f}, {x1 + 0.07f, yb + 0.09f, cz + 0.4f}, {x1 + 0.07f, yb + 0.09f, cz - 0.4f}, {1, 1, 0});
    }
    pave(met, x1, x1 + 0.08f, H + 1.45f, H + 2.27f, cz - 0.44f, cz + 0.44f, 1 | 2 | 16 | 32);   // cadre de la grille
}

void facade(const Plan& p, Batiment& out) {
    const float L = p.largeur;
    Maillage& pierre = out.part[ENCADREMENT];
    if (p.style == Style::Classique || p.style == Style::Moderne) {   // soubassement : 0,6 m, saillie 3 cm, coupé par les baies basses
        constexpr float HS = 0.6f, S = 0.03f;
        std::vector<std::pair<float, float>> trous;
        for (const Baie& b : p.baies) if (b.niveau == 0 && b.y < HS) trous.push_back({b.x - b.l / 2, b.x + b.l / 2});
        std::sort(trous.begin(), trous.end());
        float prev = 0;
        for (auto [a, b] : trous) { pave(pierre, prev, a, -0.2f, HS, 0, S, 63 & ~(2 | 32)); prev = std::max(prev, b); }
        pave(pierre, prev, L, -0.2f, HS, 0, S, 63 & ~(2 | 32));
    }
    if (p.style == Style::Classique)   // cordon sous le 1er étage (18 cm, saillie 6 cm), au-dessus des linteaux du rez-de-chaussée
        pave(pierre, 0, L, p.hRdc - 0.2f, p.hRdc - 0.02f, 0, 0.06f, 63 & ~2);
    // vitrines : dormant métallique (60 mm), meneaux tous les 2 m au plus, imposte au-dessus de 2,4 m
    Maillage& met = out.part[METAL];
    const float d = regles(p.style).retrait;
    for (const Baie& b : p.baies) if (b.type == Ouverture::Vitrine) {
        const float x0 = b.x - b.l / 2, x1 = b.x + b.l / 2, y0 = b.y, y1 = b.y + b.h, z0 = -d, z1 = -d + 0.06f, e = 0.06f;
        pave(met, x0, x1, y0, y0 + 0.12f, z0, z1, 1 | 16);
        pave(met, x0, x1, y1 - e, y1, z0, z1, 1 | 32);
        pave(met, x0, x0 + e, y0, y1, z0, z1, 1 | 8);
        pave(met, x1 - e, x1, y0, y1, z0, z1, 1 | 4);
        const int n = std::max(1, (int)std::ceil(b.l / 2.f));
        for (int k = 1; k < n; k++) { const float x = x0 + b.l * k / n; pave(met, x - 0.03f, x + 0.03f, y0, y1, z0, z1, 1 | 4 | 8); }
        if (b.h > 2.6f) pave(met, x0, x1, y0 + 2.3f, y0 + 2.36f, z0, z1, 1 | 16 | 32);
    }
}

}  // namespace wfs::archi::details
