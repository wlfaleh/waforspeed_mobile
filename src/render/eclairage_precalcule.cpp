// WaForSpeed — WFS-269 : occlusion ambiante du décor fixe précalculée au chargement (voir eclairage_precalcule.h).
#include "eclairage_precalcule.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include "../core/jobs.h"

namespace wfs::precalcul {
namespace {

// grille d'occupation creuse : table de hachage à adressage ouvert (clés 64 bits, sondage linéaire), remplie une fois puis
// lue par tous les fils de travail (aucune écriture pendant la lecture)
class Grille {
public:
    explicit Grille(size_t prevu) {
        size_t n = 1024;
        while (n < prevu * 2) n <<= 1;
        cles_.assign(n, VIDE); masque_ = n - 1;
    }
    static uint64_t cle(int x, int y, int z) {
        return ((uint64_t)(uint32_t)(x + (1 << 20)) << 42) | ((uint64_t)(uint32_t)(y + (1 << 20)) << 21) | (uint64_t)(uint32_t)(z + (1 << 20));
    }
    void ajouter(uint64_t k) {
        if (n_ * 2 >= cles_.size()) agrandir();
        size_t h = melange(k) & masque_;
        while (cles_[h] != VIDE) { if (cles_[h] == k) return; h = (h + 1) & masque_; }
        cles_[h] = k; n_++;
    }
    bool contient(uint64_t k) const {
        size_t h = melange(k) & masque_;
        while (cles_[h] != VIDE) { if (cles_[h] == k) return true; h = (h + 1) & masque_; }
        return false;
    }
    size_t taille() const { return n_; }

private:
    static constexpr uint64_t VIDE = ~0ull;
    static size_t melange(uint64_t k) { k ^= k >> 33; k *= 0xff51afd7ed558ccdull; k ^= k >> 33; return (size_t)k; }
    void agrandir() {
        std::vector<uint64_t> anciennes;
        anciennes.swap(cles_);
        cles_.assign(anciennes.size() * 2, VIDE); masque_ = cles_.size() - 1; n_ = 0;
        for (uint64_t k : anciennes) if (k != VIDE) ajouter(k);
    }
    std::vector<uint64_t> cles_;
    size_t masque_ = 0, n_ = 0;
};

inline int cellule(float c) { return (int)std::floor(c / VOXEL_M); }

// marque les voxels touchés par un triangle (points du triangle échantillonnés à un demi-voxel)
void rasteriser(Grille& g, const vec3& a, const vec3& b, const vec3& c) {
    const float l = std::max({length(b - a), length(c - a), length(c - b)});
    const int n = std::max(1, (int)std::ceil(l / (VOXEL_M * 0.5f)));
    for (int i = 0; i <= n; i++)
        for (int j = 0; j <= n - i; j++) {
            const float u = (float)i / n, w = (float)j / n;
            const vec3 p = a + (b - a) * u + (c - a) * w;
            g.ajouter(Grille::cle(cellule(p.x), cellule(p.y), cellule(p.z)));
        }
}

// directions en cosinus autour de +Z (Malley : disque uniforme projeté sur l'hémisphère), fixes : résultat reproductible
struct Directions { vec3 d[RAYONS]; };
const Directions& directions() {
    static const Directions D = [] {
        Directions r{};
        const float or_ = 2.39996323f;   // angle d'or : points bien répartis sur le disque
        for (int k = 0; k < RAYONS; k++) {
            const float rr = std::sqrt((k + 0.5f) / RAYONS), a = or_ * k;
            const float x = rr * std::cos(a), y = rr * std::sin(a);
            r.d[k] = {x, y, std::sqrt(std::max(0.f, 1 - x * x - y * y))};
        }
        return r;
    }();
    return D;
}

// part du ciel vue depuis p (normale n unitaire) : moyenne des rayons qui ne rencontrent aucun voxel occupé dans PORTEE_M
float ciel(const Grille& g, const vec3& p, const vec3& n) {
    const vec3 t = normalize(std::fabs(n.y) < 0.9f ? cross(n, vec3{0, 1, 0}) : cross(n, vec3{1, 0, 0})), b = cross(n, t);
    const vec3 o = p + n * DECALAGE_M;
    const int pas = (int)(PORTEE_M / (VOXEL_M * 0.5f));
    int libres = 0;
    for (const vec3& dl : directions().d) {
        const vec3 d = t * dl.x + b * dl.y + n * dl.z;
        bool touche = false;
        for (int s = 1; s <= pas && !touche; s++) {
            const vec3 q = o + d * (s * VOXEL_M * 0.5f);
            touche = g.contient(Grille::cle(cellule(q.x), cellule(q.y), cellule(q.z)));
        }
        libres += !touche;
    }
    return (float)libres / RAYONS;
}

} // namespace

Bilan occlusionDecor(std::vector<Lot>& lots) {
    const auto t0 = std::chrono::steady_clock::now();
    Bilan bil;
    size_t tri = 0;
    for (const Lot& l : lots) if (l.occulte && l.i) tri += l.i->size() / 3;
    Grille g(tri * 4 + 1024);
    for (const Lot& l : lots) {
        if (!l.occulte || !l.v || !l.i) continue;
        const std::vector<Vertex>& v = *l.v;
        for (size_t k = 0; k + 2 < l.i->size(); k += 3) {
            const Vertex &a = v[(*l.i)[k]], &b = v[(*l.i)[k + 1]], &c = v[(*l.i)[k + 2]];
            rasteriser(g, {a.px, a.py, a.pz}, {b.px, b.py, b.pz}, {c.px, c.py, c.pz});
        }
    }
    bil.voxels = g.taille();
    std::vector<Lot*> recoivent;
    for (Lot& l : lots) if (l.recoit && l.v) recoivent.push_back(&l);
    std::atomic<size_t> sommets{0};
    std::atomic<long long> somme{0};
    jobs().parallelFor((int)recoivent.size(), 1, [&](int b0, int e0) {
        size_t n = 0; long long s = 0;
        for (int k = b0; k < e0; k++) {
            const float force = std::clamp(recoivent[(size_t)k]->force, 0.f, 1.f);
            for (Vertex& sv : *recoivent[(size_t)k]->v) {
                vec3 nn{sv.nx, sv.ny, sv.nz};
                const float ln = length(nn);
                if (!(ln > 1e-6f)) continue;
                nn = nn * (1.f / ln);
                const float ao = 1 - force * (1 - OCCLUSION_MIN) * (1 - ciel(g, {sv.px, sv.py, sv.pz}, nn));
                sv.nx *= ao; sv.ny *= ao; sv.nz *= ao;   // longueur d'avant × occlusion
                n++; s += (long long)std::lround(ao * 1000);
            }
        }
        sommets += n; somme += s;
    });
    bil.sommets = sommets;
    bil.moyenne = bil.sommets ? (double)somme / 1000.0 / (double)bil.sommets : 1.0;
    bil.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return bil;
}

} // namespace wfs::precalcul
