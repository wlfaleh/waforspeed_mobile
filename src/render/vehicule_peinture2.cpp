// WaForSpeed — WFS-488 : couleur secondaire d'une voiture (toit et coques de rétroviseurs). Voir vehicule_peinture2.h.
// Domaine : E10 Véhicules.
#include "vehicule_peinture2.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include "../core/log.h"
#include "../core/platform.h"

namespace wfs::vehicule_peinture2 {

namespace {
uint32_t u32(const uint8_t* p) { uint32_t x; std::memcpy(&x, p, 4); return x; }
// clé d'un triangle : ses 3 sommets (indices globaux) triés, 21 bits chacun (2 097 151 sommets au plus)
constexpr uint32_t MAX_SOMMETS = 1u << 21;
uint64_t cle(uint32_t a, uint32_t b, uint32_t c) {
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
    return ((uint64_t)a << 42) | ((uint64_t)b << 21) | c;
}
}

uint32_t empreinte(const uint8_t* sommets, uint32_t nSommets, const uint8_t* indices, uint32_t nIndices) {
    uint32_t h = 0x811c9dc5u;
    auto pas = [&](uint8_t o) { h ^= o; h *= 0x01000193u; };
    for (size_t k = 0; k < (size_t)nIndices * 4; k++) pas(indices[k]);
    for (uint32_t s = 0; s < nSommets; s++) for (int k = 0; k < 12; k++) pas(sommets[(size_t)s * 32 + k]);
    return h;
}

bool lire(const uint8_t* p, size_t n, std::vector<Bloc>& blocs) {
    blocs.clear();
    if (!p || n < 12 || std::memcmp(p, "WPT2", 4) != 0 || u32(p + 4) != 1) return false;
    const uint32_t nb = u32(p + 8);
    size_t o = 12;
    for (uint32_t i = 0; i < nb; i++) {
        if (o + 20 > n) return false;
        Bloc b{u32(p + o), u32(p + o + 4), u32(p + o + 8), u32(p + o + 12), u32(p + o + 16), {}};
        o += 20;
        const size_t octets = ((size_t)b.triangles + 7) / 8;
        if (b.triangles > (1u << 26) || o + octets > n) return false;
        b.bits.assign(p + o, p + o + octets);
        o += octets;
        blocs.push_back(std::move(b));
    }
    return o == n;
}

Bilan marquer(const std::vector<Bloc>& blocs, const std::vector<MaillageWfm>& maillages, std::vector<Vertex>& verts,
              std::vector<uint32_t>& idx, const std::vector<std::vector<ModelPart>*>& listes) {
    Bilan bl;
    if (verts.size() >= MAX_SOMMETS || listes.empty()) return bl;
    // triangles du toit et des rétros, par leurs 3 sommets (les étapes du chargement réordonnent les indices, jamais les sommets)
    std::unordered_set<uint64_t> zone;
    std::vector<uint8_t> sommetZone(verts.size(), 0);
    for (const Bloc& b : blocs) {
        if (b.maillage >= maillages.size()) continue;
        const MaillageWfm& m = maillages[b.maillage];
        if (m.nIndices / 3 != b.triangles || empreinte(m.sommets, m.nSommets, m.indices, m.nIndices) != b.empreinte) {
            WFS_ERR("couleur 2 : maillage %u changé depuis tools/peinture2_voitures.mjs (empreinte) : ignoré (WFS-488)", b.maillage);
            continue;
        }
        if (!m.peint && std::find(bl.piecesNonPeintes.begin(), bl.piecesNonPeintes.end(), m.materiau) == bl.piecesNonPeintes.end())
            bl.piecesNonPeintes.push_back(m.materiau);   // pièce d'origine non peinte (noterMateriaux)
        for (uint32_t t = 0; t < b.triangles; t++) {
            if (!(b.bits[t >> 3] & (1u << (t & 7)))) continue;
            uint32_t g[3];
            for (int j = 0; j < 3; j++) g[j] = m.premierSommet + u32(m.indices + ((size_t)t * 3 + j) * 4);
            if (g[0] >= verts.size() || g[1] >= verts.size() || g[2] >= verts.size()) continue;
            zone.insert(cle(g[0], g[1], g[2]));
            for (uint32_t s : g) sommetZone[s] = 1;
        }
        bl.toit += b.toit; bl.retro += b.retro; bl.lu = true;
    }
    if (zone.empty()) return bl;
    // 1. triangles en couleur 2 de chaque liste ; sommets utilisés en couleur 2 et en couleur 1
    // (liste 0 = modèle complet : triangles du .pt2 seulement ; autres listes : triangles dont les 3 sommets sont de la zone)
    std::vector<uint8_t> en2(verts.size(), 0), en1(verts.size(), 0);
    std::vector<std::vector<uint8_t>> marque(listes.size());
    std::unordered_set<uint64_t> plagesVues;   // une plage d'indices partagée par deux listes n'est traitée qu'une fois
    for (size_t l = 0; l < listes.size(); l++) {
        if (!listes[l]) continue;
        for (const ModelPart& p : *listes[l]) {
            const bool nouvelle = plagesVues.insert(((uint64_t)p.firstIndex << 32) | p.indexCount).second;
            for (uint32_t t = 0; t + 2 < p.indexCount; t += 3) {
                uint32_t g[3];
                for (int j = 0; j < 3; j++) g[j] = (uint32_t)((int64_t)p.vertexOffset + idx[p.firstIndex + t + j]);
                if (g[0] >= verts.size() || g[1] >= verts.size() || g[2] >= verts.size()) { marque[l].push_back(0); continue; }
                const bool z = l == 0 ? zone.count(cle(g[0], g[1], g[2])) != 0 : (sommetZone[g[0]] && sommetZone[g[1]] && sommetZone[g[2]]);
                marque[l].push_back(nouvelle && z ? 1 : (nouvelle ? 0 : 2));   // 2 : plage déjà traitée
                if (!nouvelle) continue;
                for (uint32_t s : g) (z ? en2 : en1)[s] = 1;
                if (z) { bl.triangles += l == 0; bl.lod += l != 0; }
            }
        }
    }
    // 2. sommets : couleur 2 seulement → normale × MARQUE sur place ; des deux couleurs → copie marquée à la fin du tampon
    const size_t n0 = verts.size();
    std::vector<uint32_t> copie(n0, UINT32_MAX);
    auto marquerSommet = [](Vertex& v) {
        const float l = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
        if (l > 1e-12f) { const float k = MARQUE / l; v.nx *= k; v.ny *= k; v.nz *= k; }
    };
    for (size_t s = 0; s < n0; s++) {
        if (!en2[s]) continue;
        if (!en1[s]) { marquerSommet(verts[s]); continue; }
        copie[s] = (uint32_t)verts.size();
        Vertex v = verts[s]; marquerSommet(v); verts.push_back(v);
        bl.dedoubles++;
    }
    // 3. triangles en couleur 2 qui touchent un sommet dédoublé : indices vers la copie (relatifs à vertexOffset)
    for (size_t l = 0; l < listes.size(); l++) {
        if (!listes[l]) continue;
        size_t k = 0;
        for (const ModelPart& p : *listes[l])
            for (uint32_t t = 0; t + 2 < p.indexCount; t += 3, k++) {
                if (marque[l][k] != 1) continue;
                for (int j = 0; j < 3; j++) {
                    uint32_t& i = idx[p.firstIndex + t + j];
                    const uint32_t g = (uint32_t)((int64_t)p.vertexOffset + i);
                    if (copie[g] != UINT32_MAX) i = (uint32_t)((int64_t)copie[g] - p.vertexOffset);
                }
            }
    }
    return bl;
}

std::vector<uint32_t> appliquer(const std::string& cheminWfm, const std::vector<uint8_t>& f, std::vector<Vertex>& verts, std::vector<uint32_t>& idx, Model& m) {
    if (cheminWfm.size() < 4) return {};
    std::vector<uint8_t> pt2;
    const std::string chemin = cheminWfm.substr(0, cheminWfm.size() - 4) + ".pt2";
    if (!readAsset(chemin, pt2)) return {};   // voiture sans couleur 2 repérée (deux-roues…) : une seule couleur
    std::vector<Bloc> blocs;
    if (!lire(pt2.data(), pt2.size(), blocs)) { WFS_ERR("%s : illisible, voiture d'une seule couleur (WFS-488)", chemin.c_str()); return {}; }
    // table des maillages du .wfm (même lecture que loadCarModel : en-tête, textures, matériaux de 56 octets, maillages)
    std::vector<MaillageWfm> ms;
    size_t o = 8;
    auto lu = [&](size_t n) { return o + n <= f.size(); };
    if (!lu(12)) return {};
    const uint32_t nT = u32(&f[o]), nM = u32(&f[o + 4]), nMe = u32(&f[o + 8]);
    o += 12 + 24;
    for (uint32_t t = 0; t < nT; t++) { if (!lu(4)) return {}; o += 4 + (size_t)u32(&f[o]); }
    if (!lu((size_t)nM * 56)) return {};
    std::vector<uint32_t> drapeaux(nM);
    for (uint32_t k = 0; k < nM; k++) drapeaux[k] = u32(&f[o + (size_t)k * 56 + 52]);
    o += (size_t)nM * 56;
    uint32_t premier = 0;
    for (uint32_t k = 0; k < nMe; k++) {
        if (!lu(12)) return {};
        MaillageWfm w; w.materiau = u32(&f[o]); w.nSommets = u32(&f[o + 4]); w.nIndices = u32(&f[o + 8]); o += 12;
        w.peint = w.materiau < nM && (drapeaux[w.materiau] & MAT_PAINT);
        if (!lu((size_t)w.nSommets * 32 + (size_t)w.nIndices * 4)) return {};
        w.premierSommet = premier; w.sommets = &f[o]; o += (size_t)w.nSommets * 32; w.indices = &f[o]; o += (size_t)w.nIndices * 4;
        premier += w.nSommets;
        ms.push_back(w);
    }
    std::vector<std::vector<ModelPart>*> listes{&m.parts, &m.partsLod, &m.partsBasse};
    for (std::vector<ModelPart>& n : m.partsNiveaux) listes.push_back(&n);
    for (std::vector<ModelPart>& n : m.partsBasseNiveaux) listes.push_back(&n);
    const double t0 = nowSeconds();
    const Bilan b = marquer(blocs, ms, verts, idx, listes);
    if (!b.lu) return {};
    WFS_LOG("%s : couleur 2 : %u triangles (toit %u, rétroviseurs %u%s), %u sommets dédoublés au bord, %u triangles des versions allégées, %zu matériau(x) d'origine non peint(s) en partie repeint(s), %.1f ms (WFS-488)",
            cheminWfm.c_str(), b.triangles, b.toit, b.retro, b.toit ? "" : " : pas de pavillon distinct, rétroviseurs seuls", b.dedoubles, b.lod,
            b.piecesNonPeintes.size(), (nowSeconds() - t0) * 1000);
    return b.piecesNonPeintes;
}

void noterMateriaux(const std::vector<uint32_t>& pieces, Model& m) {
    if (pieces.empty()) return;
    const Material* peinture = nullptr;   // peinture de la carrosserie (premier matériau peint) : même finition (RG-04)
    for (const Material& mt : m.mats) if (mt.flags & MAT_PAINT) { peinture = &mt; break; }
    for (uint32_t k : pieces) {
        if (k >= m.mats.size()) continue;
        Material& mt = m.mats[k];
        // pièce opaque seulement (le code de la couleur 2 prend la place de l'alpha, la finition celle du seuil de découpe)
        if ((mt.flags & (MAT_BLEND | MAT_MASK | MAT_GLASS | MAT_TAIL | MAT_HEAD | MAT_EMTEX)) || mt.color.w < 0.99f) {
            WFS_ERR("couleur 2 : matériau %u non peint transparent, découpé ou lumineux : non repeint (WFS-488)", k);
            continue;
        }
        mt.peinture2Piece = true;
        mt.peinture2Finition = codeFinition(peinture ? peinture->metal : 0.f, peinture ? peinture->rough : 0.45f);
    }
}

} // namespace wfs::vehicule_peinture2
