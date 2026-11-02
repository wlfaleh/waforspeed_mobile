// WaForSpeed — chargement d'une voiture .wfm (voir tools/convert_car.mjs) : les textures JPEG/PNG sont
// décodées en parallèle sur tous les cœurs, puis envoyées au GPU depuis le thread principal.
#include "model_loader.h"
#include <algorithm>
#include <cstring>
#include "../core/jobs.h"
#include "../core/platform.h"

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
    }
    // vitres et pièces transparentes dessinées après les pièces opaques
    out.vb = r.vk().uploadBuffer(verts.data(), verts.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    out.ib = r.vk().uploadBuffer(idx.data(), idx.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    out.textures.resize(nTex);
    for (uint32_t i = 0; i < nTex; i++)
        if (dec[i].w) out.textures[i] = r.vk().uploadTexture(dec[i].rgba.data(), (uint32_t)dec[i].w, (uint32_t)dec[i].h, dec[i].srgb);
    auto texAt = [&](int32_t t) -> const Image* { return (t >= 0 && t < (int32_t)nTex && out.textures[t].view) ? &out.textures[t] : nullptr; };
    for (auto& m : raw) {
        Material mt;
        mt.color = {m.col[0], m.col[1], m.col[2], m.col[3]};
        mt.metal = m.metal; mt.rough = m.rough; mt.cutoff = m.cut;
        mt.emissive = {m.em[0], m.em[1], m.em[2]};
        mt.flags = m.flags & ~(uint32_t)(MAT_HASNORMAL | MAT_HASMR);
        if (mt.flags & MAT_GLASS) mt.flags |= MAT_BLEND;
        if (mt.flags & MAT_PAINT) { mt.rough = std::min(mt.rough, 0.45f); }
        out.mats.push_back(r.makeMaterial(mt, texAt(m.tC), texAt(m.tN), texAt(m.tMR)));
    }
    out.ok = true;
    WFS_LOG("%s : %u matériaux, %u textures, %zu triangles", path.c_str(), nMat, nTex, idx.size() / 3);
    return true;
}

} // namespace wfs
