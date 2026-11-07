// WaForSpeed — giratoires reconstruits (WFS-248) : voir giratoire.h.
#include "giratoire.h"
#include <algorithm>
#include <cmath>

namespace wfs::giratoire {

namespace {
vec3 pointAxe(Track& t, int64_t a) { const TrackPoint p = t.point((double)a * SEG, 0); return {(float)p.x, (float)p.y, (float)p.z}; }
// centre du cercle passant par trois points (plan xz) ; faux si alignés
bool cercle(vec3 p1, vec3 p2, vec3 p3, vec3& c) {
    const float ax = p1.x, az = p1.z, bx = p2.x, bz = p2.z, cx = p3.x, cz = p3.z;
    const float d = 2 * (ax * (bz - cz) + bx * (cz - az) + cx * (az - bz));
    if (std::fabs(d) < 1e-4f) return false;
    const float a2 = ax * ax + az * az, b2 = bx * bx + bz * bz, c2 = cx * cx + cz * cz;
    c = {(a2 * (bz - cz) + b2 * (cz - az) + c2 * (az - bz)) / d, (p1.y + p2.y + p3.y) / 3, (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d};
    return true;
}
float angle(const vec3& c, const vec3& q) { return std::atan2(q.z - c.z, q.x - c.x); }
vec3 polaire(const vec3& c, float r, float a, float y) { return {c.x + r * std::cos(a), c.y + y, c.z + r * std::sin(a)}; }
// angle ramené dans [−π, π]
float norme(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }

// triangle orienté vers le haut (normale +y quel que soit l'ordre donné)
void triHaut(std::vector<Tri>& o, vec3 a, vec3 b, vec3 c, Surface s) {
    const vec3 n = cross(b - a, c - a);
    if (n.y < 0) std::swap(b, c);
    o.push_back({a, b, c, s});
}
void quadHaut(std::vector<Tri>& o, vec3 a, vec3 b, vec3 c, vec3 d, Surface s) { triHaut(o, a, b, c, s); triHaut(o, a, c, d, s); }
// anneau plat de r0 à r1, de l'angle a0 à a1 (n morceaux), à la hauteur y
void anneau(std::vector<Tri>& o, const vec3& c, float r0, float r1, float a0, float a1, int n, float y, Surface s) {
    for (int k = 0; k < n; k++) {
        const float t0 = a0 + (a1 - a0) * k / n, t1 = a0 + (a1 - a0) * (k + 1) / n;
        quadHaut(o, polaire(c, r0, t0, y), polaire(c, r1, t0, y), polaire(c, r1, t1, y), polaire(c, r0, t1, y), s);
    }
}
} // namespace

Giratoire calculer(Track& t, const trace::Section& s) {
    Giratoire g;
    if (s.type != trace::Type::RondPoint || s.fin - s.debut < 20) return g;
    g.debut = s.debut; g.fin = s.fin;
    // anneau : segments dont la courbure a le signe opposé à celle de la courbe d'entrée (rondPoint : 90°, −270°, 90°)
    const double c0 = t.segAt(s.debut + 2).curve;
    int64_t e0 = -1, e1 = -1;
    for (int64_t a = s.debut + 1; a < s.fin; a++) if (t.segAt(a).curve * c0 < 0) { if (e0 < 0) e0 = a; e1 = a; }
    if (e0 < 0 || e1 - e0 < 12) return g;
    g.entree = e0; g.sortie = e1 + 1;
    const int64_t m = (e0 + e1) / 2, q = (e1 - e0) / 4;
    if (!cercle(pointAxe(t, e0 + q), pointAxe(t, m), pointAxe(t, e1 - q), g.c)) return g;
    for (int64_t a = e0; a <= e1; a++) {   // contrôle : l'axe de l'anneau suit bien le cercle du tracé (rayon de la section)
        const vec3 p = pointAxe(t, a);
        g.ecartAxe = std::max(g.ecartAxe, std::fabs(distanceCentre(g, p) - (float)s.rayon));
    }
    g.c.y = pointAxe(t, m).y;
    // branches : là où l'axe du tracé traverse le bord extérieur, avant l'entrée et après la sortie
    auto traverse = [&](int64_t depuis, int dir) {
        for (int64_t a = depuis, k = 0; k < 200; a += dir, k++) if (distanceCentre(g, pointAxe(t, a)) > R_EXT) return angle(g.c, pointAxe(t, a));
        return angle(g.c, pointAxe(t, depuis));
    };
    g.angEntree = traverse(e0, -1); g.angSortie = traverse(e1, 1);
    g.ok = g.ecartAxe < 1.5f;
    return g;
}

void geometrie(const Giratoire& g, Track& t, std::vector<Tri>& o) {
    if (!g.ok) return;
    const vec3& c = g.c;
    const int N = 96;   // 3,75° par morceau : bord de l'anneau lisse à 20 m (écart à l'arc < 2 cm)
    // ---------- îlot central : bordure (flanc + dessus de 25 cm), gazon ----------
    const float yI = H_BORDURE;
    for (int k = 0; k < N; k++) {
        const float t0 = 2 * PI * k / N, t1 = 2 * PI * (k + 1) / N;
        const vec3 b0 = polaire(c, R_ILOT, t0, Y_COURONNE), b1 = polaire(c, R_ILOT, t1, Y_COURONNE);
        const vec3 h0 = polaire(c, R_ILOT, t0, yI), h1 = polaire(c, R_ILOT, t1, yI);
        // flanc vertical, face vers l'extérieur (normale calculée par la carte : ordre extérieur)
        o.push_back({b0, b1, h1, BORDURE}); o.push_back({b0, h1, h0, BORDURE});
    }
    anneau(o, c, R_ILOT - 0.25f, R_ILOT, 0, 2 * PI, N, yI, BORDURE);
    anneau(o, c, 0.0f, R_ILOT - 0.25f, 0, 2 * PI, N, yI - 0.01f, PELOUSE);
    // ---------- couronne franchissable (pavés), anneau d'asphalte ----------
    anneau(o, c, R_ILOT, R_COURONNE, 0, 2 * PI, N, Y_COURONNE, PAVES);
    anneau(o, c, R_COURONNE, R_EXT, 0, 2 * PI, N, Y_ANNEAU, ASPHALTE);
    // ---------- lignes de rive (12 cm, blanches) : intérieure continue, extérieure ouverte aux deux branches ----------
    anneau(o, c, R_COURONNE + 0.2f, R_COURONNE + 0.32f, 0, 2 * PI, N, Y_MARQUAGE, MARQUAGE);
    const float ouverture = (float)(RW + 1.5) / R_EXT;   // demi-ouverture angulaire d'une branche (15 m de chaussée + évasement)
    auto ouvert = [&](float a) {
        bool o = std::fabs(norme(a - g.angEntree)) < ouverture || std::fabs(norme(a - g.angSortie)) < ouverture;
        for (float b : g.autresBranches) o = o || std::fabs(norme(a - b)) < ouverture;
        return o;
    };
    for (int k = 0; k < N; k++) {
        const float t0 = 2 * PI * k / N, t1 = 2 * PI * (k + 1) / N;
        if (ouvert(t0) || ouvert(t1)) continue;
        anneau(o, c, R_EXT - 0.32f, R_EXT - 0.2f, t0, t1, 1, Y_MARQUAGE, MARQUAGE);
    }
    // ---------- cédez-le-passage (T'2) en travers de la voie entrante, au bord de l'anneau ----------
    // la voie entrante est la moitié droite de la branche d'entrée (circulation à droite) : de l'axe au bord droit
    int64_t k = g.entree;
    for (int i = 0; i < 200 && distanceCentre(g, pointAxe(t, k)) < R_EXT + 0.6f; i++) k--;
    {
        const TrackPoint p0 = t.point((double)k * SEG, 0), p1 = t.point((double)(k + 1) * SEG, 0);
        const vec3 a{(float)p0.x, c.y, (float)p0.z}, b{(float)p1.x, c.y, (float)p1.z};
        const vec3 f = normalize(b - a), rr{-f.z, 0, f.x};   // sens de la route, droite de la route (Track : r = (cos θ, 0, sin θ))
        for (float x = -0.25f; x + 0.5f <= (float)RW - 0.3f; x += 1.0f) {
            const vec3 q0 = a + rr * x + vec3{0, Y_MARQUAGE + 0.002f, 0}, q1 = a + rr * (x + 0.5f) + vec3{0, Y_MARQUAGE + 0.002f, 0};
            quadHaut(o, q0, q1, q1 + f * 0.5f, q0 + f * 0.5f, MARQUAGE);
        }
    }
    // ---------- raccord des deux branches et îlot séparateur ----------
    // Les deux branches du tracé touchent l'anneau presque au même endroit (courbes d'entrée et de sortie de 90°), puis
    // s'écartent à 90° : leurs chaussées de 15 m se CHEVAUCHENT d'abord (deux surfaces et deux jeux de marquages croisés), puis
    // laissent entre elles un coin de terrain. Raccord : asphalte uni (sans marquage) sur toute la largeur des deux chaussées
    // tant qu'elles se touchent ; ensuite, îlot séparateur surélevé (bordure de 15 cm, gazon) entre leurs bords intérieurs,
    // sur 12 m, effilé vers l'anneau (nez arrondi par le pas de 1,2 m).
    auto axeA = [&](int64_t depuis, int dir, std::vector<std::pair<vec3, vec3>>& pts) {   // (point de l'axe, sens de la route)
        int64_t a = depuis;
        for (int i = 0; i < 200 && distanceCentre(g, pointAxe(t, a)) < R_EXT - 2.0f; i++) a += dir;
        for (int i = 0; i < (int)(45 / SEG_M); i++, a += dir) {
            const vec3 p0 = pointAxe(t, a), p1 = pointAxe(t, a + 1);
            pts.push_back({vec3{p0.x, c.y, p0.z}, normalize(vec3{p1.x - p0.x, 0, p1.z - p0.z})});
        }
    };
    std::vector<std::pair<vec3, vec3>> E, X;
    axeA(g.entree, -1, E); axeA(g.sortie, 1, X);
    const size_t n = std::min(E.size(), X.size());
    const float R = (float)RW;
    float ilot = 0;   // longueur d'îlot déjà posée (m)
    for (size_t i = 0; i + 1 < n && ilot < 12.f; i++) {
        auto bords = [&](const std::vector<std::pair<vec3, vec3>>& A, const std::vector<std::pair<vec3, vec3>>& B, size_t j, vec3& in, vec3& out) {
            const vec3 o = A[j].first, f = A[j].second, dr{-f.z, 0, f.x};
            const float s = dot(B[j].first - o, dr) > 0 ? 1.f : -1.f;   // côté tourné vers l'autre branche
            in = o + dr * (s * R); out = o - dr * (s * R);
        };
        vec3 ei0, eo0, xi0, xo0, ei1, eo1, xi1, xo1;
        bords(E, X, i, ei0, eo0); bords(X, E, i, xi0, xo0); bords(E, X, i + 1, ei1, eo1); bords(X, E, i + 1, xi1, xo1);
        const vec3 dy{0, Y_ANNEAU, 0};
        const float gap0 = length(xi0 - ei0) * (dot(xi0 - ei0, E[i].first - X[i].first) < 0 ? 1.f : -1.f);   // > 0 : bords séparés
        if (gap0 < 0.5f) {   // chaussées qui se touchent : asphalte uni d'un bord extérieur à l'autre
            quadHaut(o, eo0 + dy, ei0 + dy, ei1 + dy, eo1 + dy, ASPHALTE);
            quadHaut(o, xo0 + dy, xi0 + dy, xi1 + dy, xo1 + dy, ASPHALTE);
            quadHaut(o, ei0 + dy, xi0 + dy, xi1 + dy, ei1 + dy, ASPHALTE);
            continue;
        }
        // îlot séparateur entre les bords intérieurs (retrait de 0,5 m de chaque chaussée)
        const vec3 a0 = ei0 + normalize(xi0 - ei0) * 0.5f, b0 = xi0 + normalize(ei0 - xi0) * 0.5f;
        const vec3 a1 = ei1 + normalize(xi1 - ei1) * 0.5f, b1 = xi1 + normalize(ei1 - xi1) * 0.5f;
        const vec3 h{0, H_BORDURE, 0};
        quadHaut(o, a0 + h, b0 + h, b1 + h, a1 + h, ilot < 0.6f ? BORDURE : PELOUSE);
        o.push_back({a0 + dy, a1 + dy, a1 + h, BORDURE}); o.push_back({a0 + dy, a1 + h, a0 + h, BORDURE});   // flancs
        o.push_back({b1 + dy, b0 + dy, b0 + h, BORDURE}); o.push_back({b1 + dy, b0 + h, b1 + h, BORDURE});
        if (ilot < 0.01f) { o.push_back({a0 + dy, b0 + dy, b0 + h, BORDURE}); o.push_back({a0 + dy, b0 + h, a0 + h, BORDURE}); }   // nez
        ilot += (float)SEG_M;
    }
}

} // namespace wfs::giratoire
