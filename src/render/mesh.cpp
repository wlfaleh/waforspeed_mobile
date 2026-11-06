// WaForSpeed — WFS-149 (RG-04) : tangentes des sommets (voir mesh.h).
#include "mesh.h"
#include <cmath>
#include <cstring>

namespace wfs::tangentes {

namespace {
struct D3 { double x = 0, y = 0, z = 0; };
inline D3 operator-(const D3& a, const D3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline D3 operator+(const D3& a, const D3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline D3 operator*(const D3& a, double k) { return {a.x * k, a.y * k, a.z * k}; }
inline double dot(const D3& a, const D3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline D3 cross(const D3& a, const D3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline double len(const D3& a) { return std::sqrt(dot(a, a)); }
inline D3 unit(const D3& a) { const double l = len(a); return l > 1e-30 ? a * (1.0 / l) : D3{}; }
inline D3 pos(const Vertex& v) { return {v.px, v.py, v.pz}; }
// normale unitaire du sommet (la végétation porte une luminosité dans sa longueur : seule la direction compte)
inline D3 nrm(const Vertex& v) { const D3 n = unit({v.nx, v.ny, v.nz}); return len(n) > 0.5 ? n : D3{0, 1, 0}; }
// vecteur projeté dans le plan de normale n, normalisé (nul si parallèle à n)
inline D3 plan(const D3& a, const D3& n) { return unit(a - n * dot(n, a)); }
}

uint32_t calculer(Vertex* v, uint32_t nv, const uint32_t* idx, uint32_t ni) {
    if (!v || nv == 0) return 0;
    // tampons réutilisés (pas d'allocation à chaque morceau de route) ; normales unitaires calculées une fois par sommet
    static thread_local std::vector<D3> T, B, Nu;
    T.assign(nv, D3{}); B.assign(nv, D3{}); Nu.resize(nv);
    for (uint32_t i = 0; i < nv; i++) Nu[i] = nrm(v[i]);
    for (uint32_t t = 0; t + 2 < ni; t += 3) {
        const uint32_t c[3] = {idx[t], idx[t + 1], idx[t + 2]};
        if (c[0] >= nv || c[1] >= nv || c[2] >= nv) continue;
        const D3 p[3] = {pos(v[c[0]]), pos(v[c[1]]), pos(v[c[2]])};
        const D3 e1 = p[1] - p[0], e2 = p[2] - p[0];
        const double s1 = (double)v[c[1]].u - v[c[0]].u, t1 = (double)v[c[1]].v - v[c[0]].v;
        const double s2 = (double)v[c[2]].u - v[c[0]].u, t2 = (double)v[c[2]].v - v[c[0]].v;
        const double r = s1 * t2 - s2 * t1;
        if (std::fabs(r) < 1e-20 || len(cross(e1, e2)) < 1e-20) continue;   // uv ou triangle dégénérés : ignoré (comme mikktspace.c)
        const D3 sdir = (e1 * t2 - e2 * t1) * (1.0 / r), tdir = (e2 * s1 - e1 * s2) * (1.0 / r);   // ∂p/∂u, ∂p/∂v
        // arêtes unitaires (une fois par triangle) : a[k] = de p[k] vers p[k + 1]
        const D3 a[3] = {unit(p[1] - p[0]), unit(p[2] - p[1]), unit(p[0] - p[2])};
        for (int k = 0; k < 3; k++) {
            const uint32_t i = c[k];
            // angle du coin k entre les arêtes vers p[k + 1] et vers p[k + 2] (poids du coin, mikktspace.c : fAngle)
            const double angle = std::acos(std::fmax(-1.0, std::fmin(1.0, -dot(a[k], a[(k + 2) % 3]))));
            const D3& n = Nu[i];
            T[i] = T[i] + plan(sdir, n) * angle;
            B[i] = B[i] + plan(tdir, n) * angle;
        }
    }
    uint32_t sans = 0;
    for (uint32_t i = 0; i < nv; i++) {
        const D3& n = Nu[i];
        D3 t = plan(T[i], n);
        double w = 1;
        if (len(t) < 0.5) {   // aucun triangle exploitable : repère arbitraire, perpendiculaire à la normale
            t = plan(std::fabs(n.x) < 0.9 ? D3{1, 0, 0} : D3{0, 0, 1}, n);
            sans++;
        } else if (dot(cross(n, t), B[i]) < 0) w = -1;   // bitangente opposée à n × t : uv en miroir
        v[i].tx = versOctet((float)t.x); v[i].ty = versOctet((float)t.y); v[i].tz = versOctet((float)t.z);
        v[i].tw = w > 0 ? 127 : -127;
    }
    return sans;
}

bool auMoinsUne(const Vertex* v, size_t nv) {
    for (size_t i = 0; i < nv; i++) if (presente(v[i])) return true;
    return false;
}

void copierSommetsWfm(const uint8_t* src, uint32_t nv, Vertex* dst) {
    for (uint32_t i = 0; i < nv; i++) {
        float f[8];
        std::memcpy(f, src + (size_t)i * WFM_SOMMET_OCTETS, WFM_SOMMET_OCTETS);   // position, normale, uv : 8 flottants
        dst[i] = Vertex{f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]};
    }
}

bool lireSectionTangentes(const uint8_t* p, const uint8_t* fin, const std::vector<size_t>& premiers,
                          const std::vector<uint32_t>& nombres, Vertex* v) {
    if (!p || !fin || fin - p < 4 || std::memcmp(p, "TAN1", 4) != 0 || premiers.size() != nombres.size()) return false;
    p += 4;
    size_t total = 0;
    for (uint32_t n : nombres) total += (size_t)n * 4;
    if ((size_t)(fin - p) < total) return false;
    for (size_t m = 0; m < nombres.size(); m++)
        for (uint32_t k = 0; k < nombres[m]; k++, p += 4) {
            Vertex& q = v[premiers[m] + k];
            q.tx = (int8_t)p[0]; q.ty = (int8_t)p[1]; q.tz = (int8_t)p[2]; q.tw = (int8_t)p[3];
        }
    return true;
}

} // namespace wfs::tangentes
