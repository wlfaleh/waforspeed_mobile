// WaForSpeed — WFS-483 (RG-01, RG-04) : géométrie de la plaque 3D du joueur, sur le processeur (sans Vulkan, testée).
// Repère de la plaque : x vers la droite vue de l'extérieur, y vers le haut, z vers l'extérieur ; mètres. Chaque pièce est un
// profil balayé le long d'un rectangle à coins arrondis (contour fermé) : chaque bande du profil a ses propres sommets (arêtes
// vives entre deux bandes, chanfreins nets), lissés le long du contour (coins arrondis sans facettes visibles).
//   - porte-plaque : jupe arrière qui descend jusqu'à la caisse (jours de la grille, vehicule_plaque.h), flanc, chanfrein,
//     dessus plat, chanfrein intérieur, pente, lèvre qui recouvre 2 mm du bord de la plaque, puis le fond du logement ;
//   - plaque : tôle de 1 mm, bord roulé (quart de rond de 0,6 mm), face avant texturée (u de gauche à droite, v de haut en bas) ;
//   - vis : tête bombée de 9 mm, collerette, fente de 1,2 mm × 0,9 mm creusée dans le dôme (révolution de 20 secteurs).
#include <algorithm>
#include <cmath>
#include "vehicule_plaque.h"

namespace wfs::plaque_joueur {
namespace {

constexpr float MM = 0.001f;
constexpr float PI_F = 3.14159265358979f;

// contour à coins arrondis (demi-largeur a, demi-hauteur b, rayon r), sens trigonométrique vu de +z, en mm, départ en haut du
// côté droit ; nLong / nCourt : points ajoutés sur les côtés longs (haut, bas) et courts (gauche, droite), pour que la jupe du
// porte-plaque suive la caisse entre les coins (même nombre pour tous les anneaux d'une pièce)
std::vector<vec3> contour(float a, float b, float r, int seg, int nLong = 0, int nCourt = 0) {
    std::vector<vec3> c;
    r = std::max(0.3f, std::min(r, std::min(a, b) - 0.01f));
    const float cx[4] = {a - r, -a + r, -a + r, a - r}, cy[4] = {b - r, b - r, -b + r, -b + r};
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i <= seg; i++) {
            const float t = (k + (float)i / seg) * PI_F / 2;
            c.push_back({cx[k] + r * std::cos(t), cy[k] + r * std::sin(t), 0});
        }
        // côté droit jusqu'au coin suivant
        const int n = (k % 2 == 0) ? nLong : nCourt, k1 = (k + 1) % 4;
        const float t1 = (k + 1) * PI_F / 2;
        const vec3 p0{cx[k] + r * std::cos(t1), cy[k] + r * std::sin(t1), 0}, p1{cx[k1] + r * std::cos(t1), cy[k1] + r * std::sin(t1), 0};
        for (int i = 1; i <= n; i++) c.push_back(p0 + (p1 - p0) * ((float)i / (n + 1)));
    }
    return c;
}

// abscisse curviligne (0 au premier point, longueur totale = 1) de chaque point d'un contour fermé
std::vector<float> abscisses(const std::vector<vec3>& c) {
    std::vector<float> s(c.size(), 0.f);
    for (size_t i = 1; i < c.size(); i++) s[i] = s[i - 1] + length(c[i] - c[i - 1]);
    const float total = s.back() + length(c.front() - c.back());
    for (float& x : s) x /= total;
    return s;
}

// ajoute un triangle tourné vers la normale voulue (sinon retourné) : enroulement toujours vers l'extérieur
void triangle(Maillage& m, uint32_t a, uint32_t b, uint32_t c) {
    const vec3 &A = m.sommets[a].p, &B = m.sommets[b].p, &C = m.sommets[c].p;
    const vec3 n = cross(B - A, C - A), voulu = m.sommets[a].n + m.sommets[b].n + m.sommets[c].n;
    if (dot(n, voulu) >= 0) m.indices.insert(m.indices.end(), {a, b, c});
    else m.indices.insert(m.indices.end(), {a, c, b});
}

// anneau du profil : retrait (mm, vers l'intérieur du contour) et hauteur z (mm) ; jupe : z remplacé par −jour du point
struct Anneau { float retrait, z; bool jupe = false; };

// pièce balayée : contour (a, b, r) en mm, anneaux du profil ; chaque paire d'anneaux est une bande à normales propres
void balayer(Maillage& m, float a, float b, float r, int seg, int nLong, int nCourt, const std::vector<Anneau>& profil, const Pose* pose) {
    std::vector<std::vector<vec3>> P;
    for (const Anneau& an : profil) {
        std::vector<vec3> c = contour(a - an.retrait, b - an.retrait, r - an.retrait, seg, nLong, nCourt);
        const std::vector<float> s = abscisses(c);
        for (size_t i = 0; i < c.size(); i++) c[i].z = an.jupe ? -(pose ? jourContour(*pose, s[i]) : 0.f) : an.z;
        P.push_back(std::move(c));
    }
    const int n = (int)P[0].size();
    for (size_t k = 0; k + 1 < P.size(); k++) {
        const uint32_t base = (uint32_t)m.sommets.size();
        // normale de chaque point de la bande : tangente du contour × direction du profil, lissée le long du contour
        for (int ligne = 0; ligne < 2; ligne++)
            for (int i = 0; i < n; i++) {
                const vec3& q = P[k + ligne][i];
                const vec3 t = P[k][(i + 1) % n] - P[k][(i + n - 1) % n];
                const vec3 t2 = P[k + 1][(i + 1) % n] - P[k + 1][(i + n - 1) % n];
                const vec3 prof = P[k + 1][i] - P[k][i];
                // contour dans le sens trigonométrique : tangente × profil sort de la pièce (flanc vers l'extérieur, dessus vers +z,
                // dessous de la lèvre vers −z, paroi du logement vers le centre)
                const vec3 nn = normalize(cross(normalize(t + t2), prof));
                m.sommets.push_back({q * MM, nn, 0, 0});
            }
        for (int i = 0; i < n; i++) {
            const uint32_t i0 = base + i, i1 = base + (i + 1) % n, j0 = base + n + i, j1 = base + n + (i + 1) % n;
            triangle(m, i0, i1, j1); triangle(m, i0, j1, j0);
        }
    }
}

// face plane fermant un anneau du contour (éventail depuis le centre) ; uv à plat sur le rectangle L × H (texture de la plaque)
void face(Maillage& m, const std::vector<vec3>& c, float z, float sens, bool uv, float L, float H) {
    const uint32_t base = (uint32_t)m.sommets.size();
    const vec3 n{0, 0, sens};
    auto ajout = [&](float x, float y) {
        const float u = uv ? (x + L / 2) / L : 0, v = uv ? (H / 2 - y) / H : 0;
        m.sommets.push_back({vec3{x, y, z} * MM, n, u, v});
    };
    ajout(0, 0);
    for (const vec3& q : c) ajout(q.x, q.y);
    const uint32_t nc = (uint32_t)c.size();
    for (uint32_t i = 0; i < nc; i++) triangle(m, base, base + 1 + i, base + 1 + (i + 1) % nc);
}

// une vis : collerette puis dôme, fente le long de x creusée dans le dôme ; centre (cx, cy), posée sur z0 (mm)
void vis(Maillage& m, float cx, float cy, float z0) {
    constexpr int SECT = 20, ANN = 6;
    const float R = VIS_D_MM / 2, FENTE = 0.6f, PROF = 0.9f;
    // profil (rayon, hauteur) : collerette de 0,4 mm, dôme jusqu'à 2,2 mm
    auto hauteur = [&](float rr) {
        if (rr >= R) return 0.f;
        const float d = rr / R;
        return 0.4f + (VIS_H_MM - 0.4f) * std::sqrt(std::max(0.f, 1 - d * d * 1.05f));
    };
    const uint32_t base = (uint32_t)m.sommets.size();
    auto point = [&](float x, float y) {
        const float rr = std::sqrt(x * x + y * y);
        float h = hauteur(std::min(rr, R * 0.97f));
        if (std::fabs(y) < FENTE && rr < R * 0.86f) h = std::max(0.4f, h - PROF);   // fente
        return vec3{cx + x, cy + y, z0 + h};
    };
    // grille polaire : centre + ANN anneaux × SECT secteurs, puis anneau du bord à z0
    m.sommets.push_back({point(0, 0) * MM, {0, 0, 1}, 0, 0});
    for (int a = 1; a <= ANN; a++)
        for (int s = 0; s < SECT; s++) {
            const float rr = R * a / ANN, t = 2 * PI_F * s / SECT;
            m.sommets.push_back({point(rr * std::cos(t), rr * std::sin(t)) * MM, {0, 0, 1}, 0, 0});
        }
    for (int s = 0; s < SECT; s++) {   // bord : descend sur la plaque
        const float t = 2 * PI_F * s / SECT;
        m.sommets.push_back({vec3{cx + R * std::cos(t), cy + R * std::sin(t), z0} * MM, {std::cos(t), std::sin(t), 0}, 0, 0});
    }
    auto id = [&](int a, int s) { return a == 0 ? base : base + 1 + (uint32_t)((a - 1) * SECT + (s % SECT)); };
    std::vector<uint32_t> tri;
    for (int s = 0; s < SECT; s++) { tri.insert(tri.end(), {id(0, 0), id(1, s), id(1, s + 1)}); }
    for (int a = 1; a <= ANN; a++)
        for (int s = 0; s < SECT; s++) tri.insert(tri.end(), {id(a, s), id(a + 1, s), id(a + 1, s + 1), id(a, s), id(a + 1, s + 1), id(a, s + 1)});
    // normales des sommets : moyenne des faces (le dôme et la fente lissés, la collerette nette)
    std::vector<vec3> acc(m.sommets.size() - base, vec3{0, 0, 0});
    for (size_t k = 0; k + 2 < tri.size(); k += 3) {
        const vec3 &A = m.sommets[tri[k]].p, &B = m.sommets[tri[k + 1]].p, &C = m.sommets[tri[k + 2]].p;
        vec3 fn = cross(B - A, C - A); if (fn.z < 0) fn = fn * -1.f;
        for (int q = 0; q < 3; q++) acc[tri[k + q] - base] = acc[tri[k + q] - base] + fn;
    }
    for (size_t i = 0; i < acc.size(); i++) if (length(acc[i]) > 0) m.sommets[base + i].n = normalize(acc[i]);
    for (size_t k = 0; k + 2 < tri.size(); k += 3) triangle(m, tri[k], tri[k + 1], tri[k + 2]);
}

} // namespace

float jourContour(const Pose& p, float s) {
    const float t = (s - std::floor(s)) * JOURS;
    const int i0 = (int)t % JOURS, i1 = (i0 + 1) % JOURS;
    const float f = t - std::floor(t);
    return std::clamp(p.jours[i0] * (1 - f) + p.jours[i1] * f, 0.f, JOUR_MAX_MM);
}

vec3 pointContour(float s) {
    // contour fin (64 segments par coin) parcouru à l'abscisse s : même départ et même sens que contour()
    static const std::vector<vec3> c = contour(SUPPORT_L_MM / 2, SUPPORT_H_MM / 2, SUPPORT_RAYON_MM, 64);
    static const std::vector<float> a = abscisses(c);
    s -= std::floor(s);
    size_t i = std::upper_bound(a.begin(), a.end(), s) - a.begin();
    const size_t i0 = i - 1, i1 = i % c.size();
    const float s1 = i < a.size() ? a[i] : 1.f, f = (s - a[i0]) / std::max(1e-9f, s1 - a[i0]);
    return c[i0] + (c[i1] - c[i0]) * f;
}

mat4 repere(const Pose& p) {
    const vec3 n = normalize(p.normale);
    const vec3 U = normalize(vec3{0, 1, 0} - n * n.y), X = cross(U, n);
    mat4 m = mat4::identity();
    m.at(0, 0) = X.x; m.at(1, 0) = X.y; m.at(2, 0) = X.z;
    m.at(0, 1) = U.x; m.at(1, 1) = U.y; m.at(2, 1) = U.z;
    m.at(0, 2) = n.x; m.at(1, 2) = n.y; m.at(2, 2) = n.z;
    const vec3 o = p.centre + n * DECALAGE_M;
    m.at(0, 3) = o.x; m.at(1, 3) = o.y; m.at(2, 3) = o.z;
    return m;
}

Pieces construire(bool complet, const Pose* pose) {
    Pieces P;
    const int seg = complet ? 8 : 3;
    // ---- porte-plaque (mm) : contour 540 × 130, rayon 9 ; la plaque (520 × 110) est à 10 mm du bord, la lèvre va jusqu'à 12 mm
    {
        const float a = SUPPORT_L_MM / 2, b = SUPPORT_H_MM / 2, r = SUPPORT_RAYON_MM, e = SUPPORT_EP_MM;
        const float bordPlaque = (SUPPORT_L_MM - PLAQUE_L_MM) / 2, avantPlaque = PLAQUE_Z_MM + PLAQUE_EP_MM;
        std::vector<Anneau> profil = {
            {0, 0, true},                         // pied de la jupe, sur la caisse
            {0, e - 2.2f},                        // flanc
            {1.6f, e - 0.6f}, {2.4f, e},          // chanfrein extérieur (1,6 mm) et arrondi
            {bordPlaque - 4.f, e},                // dessus plat
            {bordPlaque - 2.4f, e - 0.8f},        // chanfrein intérieur
            {bordPlaque - 1.0f, avantPlaque + 1.2f},
            {bordPlaque + LEVRE_MM, avantPlaque + 0.25f},   // lèvre sur la plaque
            {bordPlaque + LEVRE_MM, avantPlaque + 0.05f},
            {bordPlaque, avantPlaque + 0.05f},    // dessous de la lèvre
            {bordPlaque, 0},                      // paroi du logement
        };
        if (!complet) profil = {{0, 0, true}, {0, e - 2.2f}, {2.4f, e}, {bordPlaque - 4.f, e}, {bordPlaque - 1.0f, avantPlaque + 1.2f},
                                {bordPlaque + LEVRE_MM, avantPlaque + 0.25f}, {bordPlaque, avantPlaque + 0.05f}, {bordPlaque, 0}};
        // côtés subdivisés : un point tous les ≈ 20 mm (complet) ou 45 mm (simple), comme les 64 jours du tableau (≈ 21 mm)
        const float pas = complet ? 20.f : 45.f;
        const int nL = (int)std::ceil((2 * (a - r)) / pas) - 1, nC = (int)std::ceil((2 * (b - r)) / pas) - 1;
        balayer(P.support, a, b, r, seg, nL, nC, profil, pose);
        face(P.support, contour(a - bordPlaque, b - bordPlaque, r - bordPlaque, seg), 0, 1, false, 0, 0);   // fond du logement
        // dos : ferme la jupe (caché par la caisse ; évite de voir l'intérieur par un jour de la calandre)
        {
            const std::vector<vec3> c = contour(a, b, r, seg, nL, nC);
            const std::vector<float> s = abscisses(c);
            float moyen = 0;
            if (pose) { for (float j : pose->jours) moyen += std::clamp(j, 0.f, JOUR_MAX_MM); moyen /= JOURS; }
            const uint32_t base = (uint32_t)P.support.sommets.size();
            P.support.sommets.push_back({vec3{0, 0, -moyen} * MM, {0, 0, -1}, 0, 0});
            for (size_t i = 0; i < c.size(); i++) P.support.sommets.push_back({vec3{c[i].x, c[i].y, -(pose ? jourContour(*pose, s[i]) : 0.f)} * MM, {0, 0, -1}, 0, 0});
            for (uint32_t i = 0; i < c.size(); i++) triangle(P.support, base, base + 1 + i, base + 1 + (i + 1) % (uint32_t)c.size());
        }
    }
    // ---- plaque : tôle de 1 mm posée au fond du logement surélevé (PLAQUE_Z_MM), bord roulé
    {
        const float a = PLAQUE_L_MM / 2, b = PLAQUE_H_MM / 2, r = PLAQUE_RAYON_MM, z0 = PLAQUE_Z_MM, z1 = PLAQUE_Z_MM + PLAQUE_EP_MM;
        std::vector<Anneau> profil = {{0, z0}, {0, z1 - 0.6f}, {0.18f, z1 - 0.18f}, {0.6f, z1}};
        if (!complet) profil = {{0, z0}, {0, z1}};
        balayer(P.plaque, a, b, r, seg, 0, 0, profil, nullptr);
        const float rf = profil.back().retrait;
        face(P.plaque, contour(a - rf, b - rf, r - rf, seg), z1, 1, true, PLAQUE_L_MM, PLAQUE_H_MM);
        face(P.plaque, contour(a, b, r, seg), z0, -1, false, 0, 0);
    }
    // ---- vis : à gauche, et au milieu de la bande de droite, à mi-hauteur
    if (complet) {
        const float z = PLAQUE_Z_MM + PLAQUE_EP_MM;
        vis(P.vis, -PLAQUE_L_MM / 2 + VIS_X_GAUCHE_MM, 0, z);
        vis(P.vis, PLAQUE_L_MM / 2 - 6.5f - BANDE_L_MM / 2, 0, z);
    }
    return P;
}

} // namespace wfs::plaque_joueur
