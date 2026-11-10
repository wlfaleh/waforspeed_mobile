// WaForSpeed — WFS-380 (RG-09) : rochers et galets du rivage (voir eau_rivage.h).
#include "eau_rivage.h"
#include <algorithm>
#include <cmath>
#include <map>
#include "../core/math.h"

namespace wfs::rivage {
namespace {

// petit générateur déterministe (même graine, même pierre)
struct Alea {
    uint32_t s;
    float operator()() { s += 0x6D2B79F5u; uint32_t t = s; t = (t ^ (t >> 15)) * (t | 1u); t ^= t + (t ^ (t >> 7)) * (t | 61u); return ((t ^ (t >> 14)) & 0xFFFFFF) / 16777216.f; }
};

// bruit de valeur 3D lisse (bosses de la pierre)
float hash3(int x, int y, int z, uint32_t g) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u ^ g * 2654435761u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (h & 0xFFFF) / 65535.f;
}
float bruit(vec3 p, uint32_t g) {
    const int x = (int)std::floor(p.x), y = (int)std::floor(p.y), z = (int)std::floor(p.z);
    const float fx = p.x - x, fy = p.y - y, fz = p.z - z;
    auto L = [](float a, float b, float t) { return a + (b - a) * t; };
    const float ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy), uz = fz * fz * (3 - 2 * fz);
    float c[2][2][2];
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) for (int k = 0; k < 2; k++) c[i][j][k] = hash3(x + i, y + j, z + k, g);
    return L(L(L(c[0][0][0], c[1][0][0], ux), L(c[0][1][0], c[1][1][0], ux), uy), L(L(c[0][0][1], c[1][0][1], ux), L(c[0][1][1], c[1][1][1], ux), uy), uz);
}

} // namespace

Maille rocher(uint32_t graine, int subdiv) {
    // icosaèdre
    const float t = (1.f + std::sqrt(5.f)) / 2.f;
    std::vector<vec3> P = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t},
                           {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    for (vec3& p : P) p = normalize(p);
    std::vector<uint32_t> I = {0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                               3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1};
    for (int s = 0; s < subdiv; s++) {   // subdivision : chaque triangle en 4, milieux partagés
        std::map<uint64_t, uint32_t> mil;
        auto milieu = [&](uint32_t a, uint32_t b) {
            const uint64_t k = a < b ? ((uint64_t)a << 32 | b) : ((uint64_t)b << 32 | a);
            auto it = mil.find(k);
            if (it != mil.end()) return it->second;
            P.push_back(normalize((P[a] + P[b]) * 0.5f));
            return mil[k] = (uint32_t)P.size() - 1;
        };
        std::vector<uint32_t> J;
        for (size_t k = 0; k < I.size(); k += 3) {
            const uint32_t a = I[k], b = I[k + 1], c = I[k + 2], ab = milieu(a, b), bc = milieu(b, c), ca = milieu(c, a);
            J.insert(J.end(), {a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca});
        }
        I.swap(J);
    }
    Alea D{graine * 2654435761u + 12345u};
    // proportions d'un bloc d'enrochement, un peu différentes pour chaque pierre
    vec3 echelle{1.f, 0.5f + 0.15f * D(), 0.68f + 0.15f * D()};
    if (subdiv == 0) echelle.y *= 0.72f;   // galet roulé par la mer : plus plat (hauteur ≈ 0,4 à 0,55 × longueur)
    // plans de fracture : la pierre est taillée (tout point au-delà du plan est ramené dessus)
    struct Plan { vec3 n; float d; };
    std::vector<Plan> plans;
    const int nPlans = 4 + (int)(D() * 3);
    for (int k = 0; k < nPlans; k++) {
        const float az = D() * 6.2831853f, el = (D() - 0.3f) * 1.4f;
        plans.push_back({normalize(vec3{std::cos(az) * std::cos(el), std::sin(el), std::sin(az) * std::cos(el)}), 0.48f + 0.22f * D()});
    }
    const uint32_t gb = graine * 7919u + 3;
    for (vec3& p : P) {
        vec3 q = p * (1.f + 0.10f * (bruit(p * 2.1f, gb) - 0.5f) + 0.05f * (bruit(p * 5.3f, gb + 1) - 0.5f));   // bosses
        for (const Plan& pl : plans) { const float e = dot(q, pl.n) - pl.d; if (e > 0) q = q - pl.n * e; }   // fractures (faces planes)
        q = {q.x * echelle.x, q.y * echelle.y, q.z * echelle.z};
        q.y = std::max(q.y, -0.8f * echelle.y);   // base aplatie : posée sur le fond
        p = q;
    }
    // triangles tournés vers l'extérieur (sens direct vu de dehors)
    for (size_t k = 0; k < I.size(); k += 3) {
        const vec3 a = P[I[k]], b = P[I[k + 1]], c = P[I[k + 2]];
        if (dot(cross(b - a, c - a), a + b + c) < 0) std::swap(I[k + 1], I[k + 2]);
    }
    // normales : lissées entre faces voisines proches (bosses), arête vive entre deux faces à plus de 35° (cassure de la pierre) :
    // chaque coin de triangle prend la moyenne des faces de son sommet qui sont à moins de 35° de la sienne
    std::vector<vec3> Fn(I.size() / 3);
    std::vector<std::vector<uint32_t>> parSommet(P.size());
    for (size_t k = 0; k < I.size(); k += 3) {
        Fn[k / 3] = cross(P[I[k + 1]] - P[I[k]], P[I[k + 2]] - P[I[k]]);
        for (int j = 0; j < 3; j++) parSommet[I[k + j]].push_back((uint32_t)(k / 3));
    }
    Maille m;
    const float cosMax = std::cos(35.f * 3.14159265f / 180.f);
    for (size_t k = 0; k < I.size(); k++) {
        const uint32_t f = (uint32_t)(k / 3), s = I[k];
        const vec3 nf = length(Fn[f]) > 1e-10f ? normalize(Fn[f]) : vec3{0, 1, 0};
        vec3 n{0, 0, 0};
        for (uint32_t g : parSommet[s]) if (length(Fn[g]) > 1e-10f && dot(normalize(Fn[g]), nf) >= cosMax) n = n + Fn[g];
        n = length(n) > 1e-10f ? normalize(n) : nf;
        m.v.push_back({P[s].x, P[s].y, P[s].z, n.x, n.y, n.z, 0, 0});
        m.i.push_back((uint32_t)k);
    }
    return m;
}

void poser(const Maille& m, vec3 c, vec3 ax, vec3 ay, vec3 az, float tuile, std::vector<Vertex>& v, std::vector<uint32_t>& i) {
    const uint32_t k0 = (uint32_t)v.size();
    // normales : transformée inverse-transposée d'axes orthogonaux (échelles différentes) = axe / longueur²
    const vec3 nx = ax * (1.f / std::max(dot(ax, ax), 1e-8f)), ny = ay * (1.f / std::max(dot(ay, ay), 1e-8f)), nz = az * (1.f / std::max(dot(az, az), 1e-8f));
    for (const Vertex& s : m.v) {
        const vec3 p = c + ax * s.px + ay * s.py + az * s.pz;
        const vec3 n = normalize(nx * s.nx + ny * s.ny + nz * s.nz);
        // uv en mètres : projection oblique (perpendiculaire à (1, 1, 1)) : aucune couture d'un triangle à l'autre
        const float u = (p.x - p.z) * 0.7071f, w = (p.x + p.z) * 0.4082f - p.y * 0.8165f;
        v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u / tuile, w / tuile});
    }
    for (uint32_t x : m.i) i.push_back(k0 + x);
}

} // namespace wfs::rivage
