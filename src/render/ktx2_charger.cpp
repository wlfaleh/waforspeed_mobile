// WaForSpeed — WFS-149 (RG-01) : choix de la variante KTX2 que le GPU sait lire, puis envoi au GPU (voir ktx2.h).
#include "ktx2.h"
#include "../core/log.h"
#include "../core/platform.h"

namespace wfs::ktx2 {

bool chargerTexture(VkCtx& vk, const std::string& base, Texture& out) {
    out = Texture{};
    std::string raisons;
    for (const char* variante : {"astc", "etc2", "rgba8"}) {
        std::vector<uint8_t> octets;
        const std::string chemin = base + "." + variante + ".ktx2";
        if (!readAsset(chemin, octets)) continue;
        Fichier f; std::string err;
        if (!lire(octets.data(), octets.size(), f, err)) { WFS_ERR("%s : KTX2 illisible : %s", chemin.c_str(), err.c_str()); continue; }
        if (!vk.formatEchantillonnable(f.format)) { raisons += std::string(raisons.empty() ? "" : ", ") + nomFormat(f.format) + " non lu par le GPU"; continue; }
        std::vector<const uint8_t*> niv; std::vector<size_t> tailles;
        for (const Niveau& n : f.niveaux) { niv.push_back(f.donnees + n.offset); tailles.push_back(n.taille); }
        out.image = vk.uploadNiveaux(f.format, f.w, f.h, niv.data(), tailles.data(), (uint32_t)niv.size());
        out.variante = variante; out.format = f.format; out.octets = f.octetsTexture(); out.ok = out.image.view != VK_NULL_HANDLE;
        WFS_LOG("texture %s : %s, %ux%u, %zu niveaux, %.1f Ko sur le GPU%s (WFS-149)", chemin.c_str(), nomFormat(f.format), f.w, f.h,
                f.niveaux.size(), out.octets / 1024.0, raisons.empty() ? "" : (" ; écartés : " + raisons).c_str());
        return out.ok;
    }
    WFS_ERR("%s : aucune variante KTX2 utilisable (%s)", base.c_str(), raisons.empty() ? "fichiers absents" : raisons.c_str());
    return false;
}

} // namespace wfs::ktx2
