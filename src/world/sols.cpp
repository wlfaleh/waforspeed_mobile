// WaForSpeed — WFS-383 : sols des cartes (voir sols.h).
#include "sols.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include "cartes_v2/carte_donnees.h"

namespace wfs {
namespace sols {

Material materiau(Climat c, bool teinte, bool rocheux) {
    Material m;
    m.flags = MAT_GROUND; m.rough = 0.95f;
    m.cutoff = code(c, teinte, rocheux);
    return m;
}

void lisserNormales(std::vector<Vertex>& v, std::vector<uint32_t>& idx) {
    if (v.empty() || idx.size() < 3) return;
    // position au centimètre : même normale pour tous les sommets d'un point (aucune arête vive entre deux bandes) ;
    // position + uv au millimètre : sommets fusionnés (deux bandes de couleur différente gardent chacune leur sommet)
    struct Cle { int64_t x, y, z, u, w; bool operator==(const Cle& o) const { return x == o.x && y == o.y && z == o.z && u == o.u && w == o.w; } };
    struct H { size_t operator()(const Cle& k) const { uint64_t h = 1469598103934665603ull; for (int64_t q : {k.x, k.y, k.z, k.u, k.w}) { h ^= (uint64_t)q; h *= 1099511628211ull; } return (size_t)h; } };
    std::unordered_map<Cle, uint32_t, H> parPoint, parSommet; parPoint.reserve(v.size() / 2 + 1); parSommet.reserve(v.size() / 2 + 1);
    std::vector<Vertex> sortie; sortie.reserve(v.size() / 2 + 1);
    std::vector<uint32_t> nouveau(v.size()), point;   // point : indice de la normale commune de chaque sommet de sortie
    std::vector<vec3> normales;
    for (size_t k = 0; k < v.size(); k++) {
        const Vertex& s = v[k];
        const int64_t x = std::llround(s.px * 100.0), y = std::llround(s.py * 100.0), z = std::llround(s.pz * 100.0);
        auto [ip, np] = parPoint.try_emplace(Cle{x, y, z, 0, 0}, (uint32_t)normales.size());
        if (np) normales.push_back({0, 0, 0});
        auto [is, ns] = parSommet.try_emplace(Cle{x, y, z, std::llround(s.u * 1000.0), std::llround(s.v * 1000.0)}, (uint32_t)sortie.size());
        if (ns) { sortie.push_back(s); point.push_back(ip->second); }
        nouveau[k] = is->second;
    }
    for (uint32_t& i : idx) i = nouveau[i];
    // normale unitaire de chaque triangle, tournée vers le haut, ajoutée à ses 3 points pondérée par l'angle du coin (Thürmer et
    // Wüthrich 1998 : le résultat ne dépend pas du découpage du quad en deux triangles)
    auto P = [&](uint32_t k) { return vec3{sortie[k].px, sortie[k].py, sortie[k].pz}; };
    for (size_t t = 0; t + 2 < idx.size(); t += 3) {
        const vec3 q[3] = {P(idx[t]), P(idx[t + 1]), P(idx[t + 2])};
        vec3 n = cross(q[1] - q[0], q[2] - q[0]);
        const float l = length(n);
        if (l < 1e-10f) continue;   // triangle plat (bande rabotée à rien)
        n = n * (1.f / l);
        if (n.y < 0) n = -n;
        for (int j = 0; j < 3; j++) {
            const vec3 e1 = q[(j + 1) % 3] - q[j], e2 = q[(j + 2) % 3] - q[j];
            const float l1 = length(e1), l2 = length(e2);
            if (l1 < 1e-6f || l2 < 1e-6f) continue;
            const float angle = std::acos(std::clamp(dot(e1, e2) / (l1 * l2), -1.f, 1.f));
            normales[point[idx[t + j]]] = normales[point[idx[t + j]]] + n * angle;
        }
    }
    for (size_t k = 0; k < sortie.size(); k++) {
        const vec3 n = normales[point[k]];
        const float l = length(n);
        const vec3 u = l > 1e-8f ? n * (1.f / l) : vec3{0, 1, 0};
        sortie[k].nx = u.x; sortie[k].ny = u.y; sortie[k].nz = u.z;
    }
    v = std::move(sortie);
}

bool climatCarte(const char* carte, Climat& c) {
    if (!carte) return false;
    if (!std::strcmp(carte, "plage")) { c = Climat::Mediterraneen; return true; }
    if (!std::strcmp(carte, "ville") || !std::strcmp(carte, "campagne") || !std::strcmp(carte, "circuit")) { c = Climat::Tempere; return true; }
    const auto d = cartes_v2::donnees(carte);
    if (!d || d->climatSol < 1 || d->climatSol > 3) return false;
    c = (Climat)d->climatSol;
    return true;
}

} // namespace sols
} // namespace wfs
