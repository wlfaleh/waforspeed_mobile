// WaForSpeed — chargement d'une voiture .wfm (voir tools/convert_car.mjs) : les textures JPEG/PNG sont
// décodées en parallèle sur tous les cœurs, puis envoyées au GPU depuis le thread principal.
#include "model_loader.h"
#include <algorithm>
#include <cstring>
#include "../core/jobs.h"
#include "../core/platform.h"
#include "feux_modele.h"   // WFS-109 : découpe des feux arrière (RG-13)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "../../third_party/stb/stb_image.h"

namespace wfs {

namespace {
struct Reader {
    const uint8_t* p; const uint8_t* end; bool ok = true;
    template <class T> T get() { T v{}; if (p + sizeof(T) > end) { ok = false; return v; } std::memcpy(&v, p, sizeof(T)); p += sizeof(T); return v; }
    const uint8_t* take(size_t n) { if (p + n > end) { ok = false; return nullptr; } const uint8_t* r = p; p += n; return r; }
};
struct Decoded { std::vector<uint8_t> rgba; int w = 0, h = 0; bool srgb = true; };
}

bool loadCarModel(Renderer& r, const std::string& path, Model& out) {
    std::vector<uint8_t> file;
    if (!readAsset(path, file)) { WFS_ERR("Fichier introuvable : %s", path.c_str()); return false; }
    Reader rd{file.data(), file.data() + file.size()};
    const uint8_t* magic = rd.take(4);
    if (!magic || std::memcmp(magic, "WFM1", 4) != 0) { WFS_ERR("%s : format inconnu", path.c_str()); return false; }
    rd.get<uint32_t>();
    uint32_t nTex = rd.get<uint32_t>(), nMat = rd.get<uint32_t>(), nMesh = rd.get<uint32_t>();
    float bb[6]; for (float& f : bb) f = rd.get<float>();
    out.bmin = {bb[0], bb[1], bb[2]}; out.bmax = {bb[3], bb[4], bb[5]};

    // textures compressées : on garde un pointeur dans le fichier, décodage en parallèle
    std::vector<std::pair<const uint8_t*, uint32_t>> blobs(nTex);
    for (uint32_t i = 0; i < nTex; i++) { uint32_t n = rd.get<uint32_t>(); blobs[i] = {rd.take(n), n}; }

    struct RawMat { float col[4], metal, rough, em[3], cut; int32_t tC, tN, tMR; uint32_t flags; };
    std::vector<RawMat> raw(nMat);
    for (auto& m : raw) {
        for (float& f : m.col) f = rd.get<float>();
        m.metal = rd.get<float>(); m.rough = rd.get<float>();
        for (float& f : m.em) f = rd.get<float>();
        m.cut = rd.get<float>(); m.tC = rd.get<int32_t>(); m.tN = rd.get<int32_t>(); m.tMR = rd.get<int32_t>(); m.flags = rd.get<uint32_t>();
    }
    if (!rd.ok) { WFS_ERR("%s : fichier tronqué", path.c_str()); return false; }

    // espace colorimétrique : couleur = sRGB, normales / métal-rugosité = linéaire
    std::vector<Decoded> dec(nTex);
    for (auto& m : raw) { if (m.tN >= 0 && m.tN < (int)nTex) dec[m.tN].srgb = false; if (m.tMR >= 0 && m.tMR < (int)nTex) dec[m.tMR].srgb = false; }
    jobs().parallelFor((int)nTex, 1, [&](int b, int e) {
        for (int i = b; i < e; i++) {
            int w, h, c;
            uint8_t* px = stbi_load_from_memory(blobs[i].first, (int)blobs[i].second, &w, &h, &c, 4);
            if (!px) continue;
            dec[i].rgba.assign(px, px + (size_t)w * h * 4); dec[i].w = w; dec[i].h = h;
            stbi_image_free(px);
        }
    });

    // géométrie : tous les maillages dans un seul tampon de sommets et d'indices
    std::vector<Vertex> verts; std::vector<uint32_t> idx;
    std::vector<float> partDetail(nMat, 0.f);
    std::vector<char> partFil(nMat, 0);
    std::vector<vec3> matMin(nMat, vec3{1e9f, 1e9f, 1e9f}), matMax(nMat, vec3{-1e9f, -1e9f, -1e9f});   // WFS-09 : boîte de chaque matériau
    for (uint32_t i = 0; i < nMesh; i++) {
        uint32_t mat = rd.get<uint32_t>(), nv = rd.get<uint32_t>(), ni = rd.get<uint32_t>();
        const uint8_t* vp = rd.take((size_t)nv * sizeof(Vertex));
        const uint8_t* ip = rd.take((size_t)ni * 4);
        if (!rd.ok) { WFS_ERR("%s : maillage tronqué", path.c_str()); return false; }
        ModelPart part{(uint32_t)idx.size(), ni, (int32_t)verts.size(), mat};
        size_t v0 = verts.size(), i0 = idx.size();
        verts.resize(v0 + nv); std::memcpy(&verts[v0], vp, (size_t)nv * sizeof(Vertex));
        idx.resize(i0 + ni); std::memcpy(&idx[i0], ip, (size_t)ni * 4);
        out.parts.push_back(part);
        if (mat < nMat) for (uint32_t k = 0; k < nv; k++) {
            const Vertex& q = verts[v0 + k];
            matMin[mat] = {std::min(matMin[mat].x, q.px), std::min(matMin[mat].y, q.py), std::min(matMin[mat].z, q.pz)};
            matMax[mat] = {std::max(matMax[mat].x, q.px), std::max(matMax[mat].y, q.py), std::max(matMax[mat].z, q.pz)};
        }
        // WFS-42 : finesse de la pièce = hauteur médiane de ses triangles (fils de dégivrage, rayons de jante…)
        if (ni >= 3 && mat < nMat) {
            const Vertex* V = &verts[v0]; const uint32_t* I = &idx[i0];
            std::vector<float> alt, asp; alt.reserve(ni / 3); asp.reserve(ni / 3);
            for (uint32_t t = 0; t + 2 < ni; t += 3) {
                if (I[t] >= nv || I[t + 1] >= nv || I[t + 2] >= nv) continue;
                const Vertex &a = V[I[t]], &b = V[I[t + 1]], &c = V[I[t + 2]];
                vec3 A{a.px, a.py, a.pz}, B{b.px, b.py, b.pz}, C{c.px, c.py, c.pz};
                float e = std::max({length(B - A), length(C - B), length(A - C)});
                if (e > 1e-7f) { const float h = length(cross(B - A, C - A)) / e; alt.push_back(h); asp.push_back(e / std::max(h, 1e-7f)); }
            }
            if (!alt.empty()) {
                std::nth_element(alt.begin(), alt.begin() + alt.size() / 2, alt.end());
                const float med = alt[alt.size() / 2];
                if (med < 0.004f) partDetail[mat] = partDetail[mat] > 0 ? std::min(partDetail[mat], med) : med;
                // fils : triangles longs et étroits (allongement médian > 8), à la différence des surfaces faites de
                // petits triangles (jantes : 3,3)
                std::nth_element(asp.begin(), asp.begin() + asp.size() / 2, asp.end());
                if (med < 0.004f && asp[asp.size() / 2] > 8.f) partFil[mat] = true;
            }
        }
    }
    // WFS-60 : roues séparées (splitWheels du jeu web) : chaque matériau est redécoupé en une partie fixe et une partie
    // par roue (mêmes sommets, indices regroupés) ; sans 4 roues trouvées, le modèle reste tel quel
    {
        std::vector<roues::Maillage> ms;
        for (const ModelPart& p : out.parts) {
            const uint32_t nv = (uint32_t)((&p == &out.parts.back() ? verts.size() : (size_t)(&p + 1)->vertexOffset) - (size_t)p.vertexOffset);
            ms.push_back({&verts[(size_t)p.vertexOffset].px, 8, nv, &idx[p.firstIndex], p.indexCount,
                          p.material < nMat && (raw[p.material].flags & MAT_WHEEL) != 0});
        }
        const roues::Decoupe d = roues::decouper(ms);
        if (d.n == 4) {
            std::vector<uint32_t> idx2; idx2.reserve(idx.size());
            std::vector<ModelPart> parts2;
            size_t tournent = 0;
            for (size_t k = 0; k < out.parts.size(); k++) {
                const ModelPart& p = out.parts[k];
                for (int r = -1; r < 4; r++) {
                    const uint32_t first = (uint32_t)idx2.size();
                    for (uint32_t t = 0; t < p.indexCount / 3; t++)
                        if (d.roueDuTriangle[k][t] == r) for (int j = 0; j < 3; j++) idx2.push_back(idx[p.firstIndex + t * 3 + j]);
                    const uint32_t n = (uint32_t)idx2.size() - first;
                    if (n) parts2.push_back({first, n, p.vertexOffset, p.material, r});
                    if (r >= 0) tournent += n / 3;
                }
            }
            idx.swap(idx2); out.parts.swap(parts2);
            for (int k = 0; k < 4; k++) out.roues[k] = d.roues[k];
            out.nRoues = 4;
            WFS_LOG("%s : 4 roues séparées (rayon %.3f m, essieux z %.2f / %.2f), %zu triangles tournent (WFS-60)", path.c_str(),
                    d.roues[0].rayon, d.roues[0].centre.z, d.roues[2].centre.z, tournent);
        } else WFS_LOG("%s : roues non trouvées, modèle inchangé (WFS-60)", path.c_str());
    }
    // WFS-109 (RG-13, RG-03) : feux arrière découpés dans un matériau de feux avant + arrière ; 3e feu stop marqué
    std::vector<feux_modele::Materiau> fm(nMat);
    for (uint32_t mi = 0; mi < nMat; mi++) fm[mi] = {raw[mi].flags, {raw[mi].em[0], raw[mi].em[1], raw[mi].em[2]}, matMin[mi], matMax[mi]};
    const feux_modele::Resultat feuA = feux_modele::decouper(idx, out.parts, [&](size_t v) { return vec3{verts[v].px, verts[v].py, verts[v].pz}; }, fm, out.bmax.z - out.bmin.z);
    if (feuA.materiau >= 0) WFS_LOG("%s : feu arrière découpé dans le matériau %d, %u triangles (WFS-109)", path.c_str(), feuA.materiau, feuA.triangles);
    // vitres et pièces transparentes dessinées après les pièces opaques
    out.vb = r.vk().uploadBuffer(verts.data(), verts.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    out.ib = r.vk().uploadBuffer(idx.data(), idx.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    out.textures.resize(nTex);
    for (uint32_t i = 0; i < nTex; i++)
        if (dec[i].w) out.textures[i] = r.vk().uploadTexture(dec[i].rgba.data(), (uint32_t)dec[i].w, (uint32_t)dec[i].h, dec[i].srgb);
    auto texAt = [&](int32_t t) -> const Image* { return (t >= 0 && t < (int32_t)nTex && out.textures[t].view) ? &out.textures[t] : nullptr; };
    // WFS-09 : feux repérés par matériau (lampAt de js/cars/models.js). Un matériau n'est un feu que si toutes ses pièces
    // sont au bon endroit : feux arrière dans le dernier tiers (z > 0,3 L), phares dans le premier (z < −0,3 L), bloc
    // de moins de 0,5 m de haut ; sinon (étriers « brake », plastique partagé) le drapeau est retiré.
    const float L = out.bmax.z - out.bmin.z, H = out.bmax.y - out.bmin.y;
    vec3 tMin{1e9f, 1e9f, 1e9f}, tMax{-1e9f, -1e9f, -1e9f}, hMin = tMin, hMax = tMax;
    for (uint32_t mi = 0; mi < nMat; mi++) {
        if (matMax[mi].x < matMin[mi].x) { raw[mi].flags &= ~(uint32_t)(MAT_TAIL | MAT_HEAD); continue; }
        const bool basse = matMax[mi].y - matMin[mi].y < 0.5f;
        if ((raw[mi].flags & MAT_TAIL) && !(basse && matMin[mi].z > L * 0.3f)) raw[mi].flags &= ~(uint32_t)MAT_TAIL;
        if ((raw[mi].flags & MAT_HEAD) && !(basse && matMax[mi].z < -L * 0.3f)) raw[mi].flags &= ~(uint32_t)MAT_HEAD;
        vec3 &a = (raw[mi].flags & MAT_TAIL) ? tMin : hMin, &b = (raw[mi].flags & MAT_TAIL) ? tMax : hMax;
        if (raw[mi].flags & (MAT_TAIL | MAT_HEAD)) {
            a = {std::min(a.x, matMin[mi].x), std::min(a.y, matMin[mi].y), std::min(a.z, matMin[mi].z)};
            b = {std::max(b.x, matMax[mi].x), std::max(b.y, matMax[mi].y), std::max(b.z, matMax[mi].z)};
        }
    }
    if (feuA.materiau >= 0) { tMin = feuA.bmin; tMax = feuA.bmax; }   // WFS-109 : position mesurée du feu découpé
    // hauteurs des halos : pièces des feux, sinon proportions des voitures du jeu web (d.tail − 0,25 ≈ 0,53 H, d.nose − 0,12 ≈ 0,41 H)
    out.feux.largeur = out.bmax.x - out.bmin.x; out.feux.avant = out.bmin.z; out.feux.arriere = out.bmax.z;
    out.feux.pieceArriere = tMax.y > tMin.y; out.feux.arriereZ = out.feux.pieceArriere ? tMax.z : out.bmax.z;   // WFS-109 : lueur sur le feu
    out.feux.arriereY = tMax.y > tMin.y ? (tMin.y + tMax.y) * 0.5f : out.bmin.y + H * 0.53f;
    out.feux.avantY = hMax.y > hMin.y ? (hMin.y + hMax.y) * 0.5f : out.bmin.y + H * 0.41f;
    // centre latéral d'un feu : aux trois quarts de la demi-largeur des pièces des feux (bloc gauche + bloc droit)
    const float hw = out.feux.largeur * 0.5f;
    out.feux.arriereX = tMax.x > tMin.x ? std::clamp(std::max(tMax.x, -tMin.x) * 0.75f, 0.2f, hw - 0.1f) : hw - 0.18f;
    out.feux.avantX = hMax.x > hMin.x ? std::clamp(std::max(hMax.x, -hMin.x) * 0.75f, 0.2f, hw - 0.1f) : hw - 0.3f;
    for (size_t mi = 0; mi < raw.size(); mi++) {
        const RawMat& m = raw[mi];
        Material mt;
        mt.detail = partDetail[mi];
        mt.filament = partFil[mi] && !(m.flags & (MAT_PAINT | MAT_BLEND | MAT_GLASS));
        mt.color = {m.col[0], m.col[1], m.col[2], m.col[3]};
        mt.metal = m.metal; mt.rough = m.rough; mt.cutoff = m.cut;
        mt.emissive = {m.em[0], m.em[1], m.em[2]};
        mt.flags = m.flags & ~(uint32_t)(MAT_HASNORMAL | MAT_HASMR);
        if (mt.flags & MAT_GLASS) mt.flags |= MAT_BLEND;
        if (mt.flags & MAT_PAINT) { mt.rough = std::min(mt.rough, 0.45f); }
        out.mats.push_back(r.makeMaterial(mt, texAt(m.tC), texAt(m.tN), texAt(m.tMR)));
    }
    for (uint32_t mi = 0; mi < nMat; mi++)
        if (partDetail[mi] > 0) WFS_LOG("%s : matériau %u très fin (%.1f mm) → préfiltré%s (WFS-42)", path.c_str(), mi, partDetail[mi] * 1000.f,
                                        out.mats[mi].filament ? ", fils en couverture partielle" : "");
    out.ok = true;
    WFS_LOG("%s : %u matériaux, %u textures, %zu triangles", path.c_str(), nMat, nTex, idx.size() / 3);
    return true;
}

} // namespace wfs
