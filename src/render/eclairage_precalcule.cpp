// WaForSpeed — WFS-269 : occlusion ambiante du décor fixe précalculée au chargement (voir eclairage_precalcule.h).
#include "eclairage_precalcule.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include "../core/jobs.h"

namespace wfs::precalcul {

// grille d'occupation à deux niveaux : cellules de 16 × 16 × 16 voxels (8 m) repérées par une table de hachage à adressage ouvert,
// chacune un masque de 4096 bits (512 octets). Remplie une fois (une grille par tâche, puis fusion par OU des masques), puis lue
// par tous les fils (aucune écriture) ; chaque rayon garde sa dernière cellule : presque toujours un simple test de bit.
// (Essais au banc, Ville, 3,3 millions de voxels : table plate de clés 64 bits, 4,7 s puis 1,5 s en parallèle ; ici bien moins.)
class Grille {
public:
    struct Curseur { uint64_t cle = ~0ull; const uint64_t* bits = nullptr; };
    static uint64_t cleCellule(int cx, int cy, int cz) {
        return ((uint64_t)(uint32_t)(cx + (1 << 20)) << 42) | ((uint64_t)(uint32_t)(cy + (1 << 20)) << 21) | (uint64_t)(uint32_t)(cz + (1 << 20));
    }
    void reserver(size_t cellules) {
        size_t n = 1024;
        while (n < cellules * 2) n <<= 1;
        cles_.assign(n, VIDE); index_.assign(n, 0); masque_ = n - 1;
        blocs_.reserve(cellules);
    }
    void ajouter(int x, int y, int z) {
        const uint64_t c = cleCellule(x >> 4, y >> 4, z >> 4);
        if (c != derniere_) { derniere_ = c; courant_ = trouverOuCreer(c); }
        const int b = ((x & 15) << 8) | ((y & 15) << 4) | (z & 15);
        uint64_t& w = blocs_[courant_][(size_t)(b >> 6)];
        if (!(w >> (b & 63) & 1)) { w |= 1ull << (b & 63); n_++; }
    }
    bool contient(int x, int y, int z, Curseur& cu) const {
        const uint64_t c = cleCellule(x >> 4, y >> 4, z >> 4);
        if (c != cu.cle) { cu.cle = c; cu.bits = trouver(c); }
        if (!cu.bits) return false;
        const int b = ((x & 15) << 8) | ((y & 15) << 4) | (z & 15);
        return cu.bits[b >> 6] >> (b & 63) & 1;
    }
    size_t taille() const { return n_; }
    size_t cellules() const { return blocs_.size(); }
    // ajoute les voxels d'une autre grille (OU des masques), puis recompte
    void fusionner(const Grille& o) {
        for (size_t h = 0; h < o.cles_.size(); h++) if (o.cles_[h] != VIDE) {
            derniere_ = VIDE;
            auto& dst = blocs_[trouverOuCreer(o.cles_[h])];
            const auto& src = o.blocs_[o.index_[h]];
            for (size_t w = 0; w < 64; w++) dst[w] |= src[w];
        }
    }
    void recompter() { n_ = 0; for (const auto& b : blocs_) for (uint64_t w : b) n_ += (size_t)__builtin_popcountll(w); }

private:
    static constexpr uint64_t VIDE = ~0ull;
    static size_t melange(uint64_t k) { k ^= k >> 33; k *= 0xff51afd7ed558ccdull; k ^= k >> 33; return (size_t)k; }
    size_t trouverOuCreer(uint64_t c) {
        if ((blocs_.size() + 1) * 2 > cles_.size()) agrandir();
        size_t h = melange(c) & masque_;
        while (cles_[h] != VIDE) { if (cles_[h] == c) return index_[h]; h = (h + 1) & masque_; }
        cles_[h] = c; index_[h] = (uint32_t)blocs_.size(); blocs_.emplace_back(); blocs_.back().fill(0);
        return index_[h];
    }
    const uint64_t* trouver(uint64_t c) const {
        size_t h = melange(c) & masque_;
        while (cles_[h] != VIDE) { if (cles_[h] == c) return blocs_[index_[h]].data(); h = (h + 1) & masque_; }
        return nullptr;
    }
    void agrandir() {
        std::vector<uint64_t> anciennes; std::vector<uint32_t> idx;
        anciennes.swap(cles_); idx.swap(index_);
        const size_t n = std::max<size_t>(1024, anciennes.size() * 2);
        cles_.assign(n, VIDE); index_.assign(n, 0); masque_ = n - 1;
        for (size_t k = 0; k < anciennes.size(); k++) if (anciennes[k] != VIDE) {
            size_t h = melange(anciennes[k]) & masque_;
            while (cles_[h] != VIDE) h = (h + 1) & masque_;
            cles_[h] = anciennes[k]; index_[h] = idx[k];
        }
        derniere_ = VIDE;
    }
    std::vector<uint64_t> cles_;
    std::vector<uint32_t> index_;
    std::vector<std::array<uint64_t, 64>> blocs_;
    size_t masque_ = 0, n_ = 0, courant_ = 0;
    uint64_t derniere_ = VIDE;
};

namespace {
std::unique_ptr<Grille> g_grilleTour;   // WFS-161 : grille gardée pour les murs du kit (voir le .h)

inline int cellule(float c) { return (int)std::floor(c / VOXEL_M); }

// voxels touchés par un triangle (points du triangle échantillonnés à 0,9 voxel : aucun trou qu'un rayon, qui avance d'un
// demi-voxel, pourrait traverser sans toucher le voxel voisin)
void rasteriser(Grille& g, const vec3& a, const vec3& b, const vec3& c) {
    const float l = std::max({length(b - a), length(c - a), length(c - b)});
    const int n = std::max(1, (int)std::ceil(l / (VOXEL_M * 0.9f)));
    for (int i = 0; i <= n; i++)
        for (int j = 0; j <= n - i; j++) {
            const float u = (float)i / n, w = (float)j / n;
            const vec3 p = a + (b - a) * u + (c - a) * w;
            g.ajouter(cellule(p.x), cellule(p.y), cellule(p.z));
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
    int libres = 0;
    Grille::Curseur cu;
    for (const vec3& dl : directions().d) {
        const vec3 d = t * dl.x + b * dl.y + n * dl.z;
        // rayon vers le bas : il verrait le sol, dont la sonde d'éclairage (SH9, plus sombre en bas) tient déjà compte ; le
        // compter comme caché assombrirait deux fois toute façade dégagée (essai au banc : casino de la Corniche) : libre
        if (d.y < -0.05f) { libres++; continue; }
        // pas d'un voxel (un mur, coque d'un voxel d'épaisseur, n'est jamais sauté : les bâtiments sont creux dans la grille) ;
        // dans une cellule de 8 m vide, saut direct à sa sortie (le ciel au-dessus des toits : quelques pas seulement)
        bool touche = false;
        for (float t = VOXEL_M * 0.5f; t <= PORTEE_M;) {
            const vec3 q = o + d * t;
            const int ix = cellule(q.x), iy = cellule(q.y), iz = cellule(q.z);
            if (g.contient(ix, iy, iz, cu)) { touche = true; break; }
            if (cu.bits) { t += VOXEL_M; continue; }
            const float C = 16 * VOXEL_M;   // côté d'une cellule (m)
            float sortie = PORTEE_M;
            const float bornes[3][2] = {{(float)(ix >> 4) * C, (float)((ix >> 4) + 1) * C}, {(float)(iy >> 4) * C, (float)((iy >> 4) + 1) * C},
                                        {(float)(iz >> 4) * C, (float)((iz >> 4) + 1) * C}};
            const float qq[3] = {q.x, q.y, q.z}, dd[3] = {d.x, d.y, d.z};
            for (int a = 0; a < 3; a++)
                if (std::fabs(dd[a]) > 1e-6f) sortie = std::min(sortie, ((dd[a] > 0 ? bornes[a][1] : bornes[a][0]) - qq[a]) / dd[a]);
            t += std::max(sortie, 0.f) + VOXEL_M * 0.05f;
        }
        libres += !touche;
    }
    return (float)libres / RAYONS;
}

} // namespace

Bilan occlusionDecor(std::vector<Lot>& lots, bool garder) {
    const auto t0 = std::chrono::steady_clock::now();
    Bilan bil;
    // chaque tâche remplit sa propre grille pour une suite de lots voisins (morceaux consécutifs le long de la route : une région
    // compacte), sans garder de clés ; puis les grilles sont fusionnées (OU des masques). Essais au banc (Ville) : clés brutes
    // triées 1,2 s ; non triées 0,8 s mais 72 millions de clés (580 Mo) : écarté.
    std::vector<const Lot*> occultent;
    for (const Lot& l : lots) if (l.occulte && l.v && l.i && !l.i->empty()) occultent.push_back(&l);
    std::mutex verrou;
    std::vector<std::unique_ptr<Grille>> partielles;
    const int grain = std::max(1, (int)occultent.size() / 32);
    jobs().parallelFor((int)occultent.size(), grain, [&](int b0, int e0) {
        auto gp = std::make_unique<Grille>();
        gp->reserver(256);
        for (int k = b0; k < e0; k++) {
            const Lot& l = *occultent[(size_t)k];
            const std::vector<Vertex>& v = *l.v;
            for (size_t t = 0; t + 2 < l.i->size(); t += 3) {
                const Vertex &a = v[(*l.i)[t]], &b = v[(*l.i)[t + 1]], &c = v[(*l.i)[t + 2]];
                rasteriser(*gp, {a.px, a.py, a.pz}, {b.px, b.py, b.pz}, {c.px, c.py, c.pz});
            }
        }
        std::lock_guard<std::mutex> v(verrou);
        partielles.push_back(std::move(gp));
    });
    size_t cellules = 0;
    for (const auto& p : partielles) cellules += p->cellules();
    auto gt = std::make_unique<Grille>();
    Grille& g = *gt;
    g.reserver(cellules + 1024);
    for (auto& p : partielles) { g.fusionner(*p); p.reset(); }
    g.recompter();
    bil.memoireMo = (double)g.cellules() * 512 / 1e6;
    bil.voxels = g.taille();
    bil.msGrille = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
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
    g_grilleTour = garder ? std::move(gt) : nullptr;
    return bil;
}

const Grille* grilleDuTour() { return g_grilleTour.get(); }
void libererGrille() { g_grilleTour.reset(); }
float occlusionPoint(const Grille& g, const vec3& p, const vec3& n) { return 1 - (1 - OCCLUSION_MIN) * (1 - ciel(g, p, n)); }

} // namespace wfs::precalcul
