// WaForSpeed — décor fixe préparé au chargement (WFS-268) : voir lots_decor.h.
#include "lots_decor.h"
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../game/track.h"
#include "lots_decor_tour.h"

namespace wfs::lots {

bool g_lotsDecor = [] { const char* e = std::getenv("WFS_LOTS"); return !(e && std::strcmp(e, "0") == 0); }();

bool DecorFixe::preparer(Renderer& r, Track& t, int parts, int blocChunks, const Construire& construire, const char* nom) {
    liberer(r);
    const auto t0 = std::chrono::steady_clock::now();
    parts_ = std::min(parts, MAX_PARTS);
    L_ = (int64_t)t.segmentCount();
    if (L_ <= 0 || parts_ <= 0) return false;
    const int64_t nChunks = (L_ + CH - 1) / CH;
    // passage d'un tour au suivant (lots_decor_tour.h) ; écart au journal
    t.pose(-CH - 2); t.pose(2 * L_ + CH + 40);   // poses calculées ici (thread principal) : les tâches ne font que lire la piste
    tour_ = mesurerTour(t);
    WFS_LOG("décor fixe (WFS-268) : %s : un tour = le précédent tourné de %.2f° (écart %.4f m)", nom, tour_.theta * 57.29578, tour_.ecart);
    for (Cache& c : cache_) c = Cache{};
    // construction de tous les morceaux du tour 0 (détail et allégé), en parallèle, avec les fonctions des cartes
    struct Morceau { std::array<std::vector<Vertex>, MAX_PARTS> v; std::array<std::vector<uint32_t>, MAX_PARTS> i; };
    std::vector<Morceau> cpu((size_t)nChunks * 2);
    jobs().parallelFor((int)cpu.size(), 1, [&](int b0, int e0) {
        for (int k = b0; k < e0; k++) construire(k / 2, (k & 1) != 0, cpu[(size_t)k].v.data(), cpu[(size_t)k].i.data());
    });
    // regroupement : bloc de blocChunks morceaux, un maillage par matériau et par niveau de détail
    const int64_t B = std::max(1, blocChunks);
    std::vector<Vertex> vv; std::vector<uint32_t> ii;
    for (int64_t c0 = 0; c0 < nChunks; c0 += B) {
        Bloc bl; bl.s0 = c0 * CH; bl.s1 = std::min(L_, (c0 + B) * CH);
        for (int lod = 0; lod < 2; lod++) for (int p = 0; p < parts_; p++) {
            vv.clear(); ii.clear();
            for (int64_t c = c0; c < std::min(nChunks, c0 + B); c++) {
                const Morceau& m = cpu[(size_t)(c * 2 + lod)];
                const uint32_t base = (uint32_t)vv.size();
                vv.insert(vv.end(), m.v[(size_t)p].begin(), m.v[(size_t)p].end());
                for (uint32_t x : m.i[(size_t)p]) ii.push_back(base + x);
            }
            if (ii.empty()) continue;
            bl.m[(size_t)lod][(size_t)p] = r.makeMesh(vv, ii, false);
            sommets_ += vv.size(); octets_ += vv.size() * sizeof(Vertex) + ii.size() * 4;
        }
        blocs_.push_back(bl);
    }
    dureeMs_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    pret_ = true;
    WFS_LOG("décor fixe (WFS-268) : %s : %lld morceaux → %zu blocs de %lld m, %zu sommets, %.1f Mo GPU, préparé en %.0f ms", nom,
            (long long)nChunks, blocs_.size(), (long long)std::llround((double)B * CH * SEG_M), sommets_, octets_ / 1048576.0, dureeMs_);
    return true;
}

void DecorFixe::liberer(Renderer& r) {
    for (Bloc& b : blocs_) for (auto& l : b.m) for (Mesh& m : l) if (m.vb.buf) r.destroy(m);
    blocs_.clear(); pret_ = false; sommets_ = octets_ = 0; L_ = 0; orig_[0] = orig_[1] = UINT64_MAX;
}

const mat4& DecorFixe::matriceTour(Track& t, int64_t k) {
    for (Cache& c : cache_) if (c.k == k) return c.m;
    Cache* libre = &cache_[(size_t)(((k % 8) + 8) % 8)];   // case de k (cache circulaire par k)
    libre->k = k;
    libre->m = matriceDuTour(t, tour_, k);
    return libre->m;
}

} // namespace wfs::lots
