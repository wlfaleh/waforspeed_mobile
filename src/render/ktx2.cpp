// WaForSpeed — WFS-149 (RG-01) : lecture des textures KTX2 (voir ktx2.h) ; sans Vulkan ni fichier (testé par test_ktx2).
#include "ktx2.h"
#include <cstring>

namespace wfs::ktx2 {

namespace {
// identifiant du conteneur (spécification KTX 2.0, § 3.1)
constexpr uint8_t IDENTIFIANT[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
constexpr size_t TETE = 80, NIVEAU = 24;   // en-tête + index (§ 3.2 à 3.6), entrée de l'index des niveaux (§ 3.9)
template <class T> T lireLE(const uint8_t* p) { T v; std::memcpy(&v, p, sizeof v); return v; }
}

bool decrireFormat(VkFormat f, Bloc& b) {
    switch (f) {
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:      b = {4, 4, 16, true, true}; return true;
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:     b = {4, 4, 16, false, true}; return true;
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:      b = {6, 6, 16, true, true}; return true;
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:     b = {6, 6, 16, false, true}; return true;
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:  b = {4, 4, 16, true, true}; return true;
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: b = {4, 4, 16, false, true}; return true;
        case VK_FORMAT_R8G8B8A8_SRGB:            b = {1, 1, 4, true, false}; return true;
        case VK_FORMAT_R8G8B8A8_UNORM:           b = {1, 1, 4, false, false}; return true;
        default: return false;
    }
}

const char* nomFormat(VkFormat f) {
    switch (f) {
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "ASTC 4x4 sRGB";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "ASTC 4x4 linéaire";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "ASTC 6x6 sRGB";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "ASTC 6x6 linéaire";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "ETC2 RGBA sRGB";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "ETC2 RGBA linéaire";
        case VK_FORMAT_R8G8B8A8_SRGB: return "RGBA8 sRGB";
        case VK_FORMAT_R8G8B8A8_UNORM: return "RGBA8 linéaire";
        default: return "inconnu";
    }
}

bool lire(const uint8_t* p, size_t n, Fichier& f, std::string& erreur) {
    f = Fichier{};
    if (!p || n < TETE) { erreur = "fichier trop court"; return false; }
    if (std::memcmp(p, IDENTIFIANT, sizeof IDENTIFIANT) != 0) { erreur = "pas un fichier KTX2"; return false; }
    const uint32_t vkFormat = lireLE<uint32_t>(p + 12), typeSize = lireLE<uint32_t>(p + 16);
    const uint32_t w = lireLE<uint32_t>(p + 20), h = lireLE<uint32_t>(p + 24), d = lireLE<uint32_t>(p + 28);
    const uint32_t couches = lireLE<uint32_t>(p + 32), faces = lireLE<uint32_t>(p + 36), nNiveaux = lireLE<uint32_t>(p + 40);
    const uint32_t superCompression = lireLE<uint32_t>(p + 44);
    f.format = (VkFormat)vkFormat;
    if (!decrireFormat(f.format, f.bloc)) { erreur = "format " + std::to_string(vkFormat) + " non pris en charge"; return false; }
    if (typeSize != 1) { erreur = "typeSize " + std::to_string(typeSize) + " (1 attendu)"; return false; }
    if (w == 0 || h == 0 || w > 16384 || h > 16384) { erreur = "taille invalide"; return false; }
    if (d != 0 || couches != 0 || faces != 1) { erreur = "texture 2D simple attendue (ni volume, ni tableau, ni cube)"; return false; }
    if (superCompression != 0) { erreur = "supercompression non prise en charge"; return false; }
    uint32_t chaine = 1; for (uint32_t m = w > h ? w : h; m > 1; m >>= 1) chaine++;
    if (nNiveaux != chaine) { erreur = std::to_string(nNiveaux) + " niveaux (chaîne complète de " + std::to_string(chaine) + " attendue)"; return false; }
    if (n < TETE + (size_t)nNiveaux * NIVEAU) { erreur = "index des niveaux tronqué"; return false; }
    uint32_t lw = w, lh = h;
    for (uint32_t i = 0; i < nNiveaux; i++) {
        const uint8_t* e = p + TETE + (size_t)i * NIVEAU;
        const uint64_t ofs = lireLE<uint64_t>(e), lon = lireLE<uint64_t>(e + 8);
        const uint64_t attendu = (uint64_t)((lw + f.bloc.w - 1) / f.bloc.w) * ((lh + f.bloc.h - 1) / f.bloc.h) * f.bloc.octets;
        if (lon != attendu) { erreur = "niveau " + std::to_string(i) + " : " + std::to_string(lon) + " octets (" + std::to_string(attendu) + " attendus)"; return false; }
        if (ofs > n || lon > n - ofs) { erreur = "niveau " + std::to_string(i) + " hors du fichier"; return false; }
        f.niveaux.push_back({(size_t)ofs, (size_t)lon, lw, lh});
        lw = lw > 1 ? lw / 2 : 1; lh = lh > 1 ? lh / 2 : 1;
    }
    f.w = w; f.h = h; f.donnees = p; f.taille = n;
    return true;
}

} // namespace wfs::ktx2
