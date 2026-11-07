// WaForSpeed — cache de pipelines Vulkan (WFS-262, RG-01) : voir cache_pipelines.h.
#include "cache_pipelines.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace wfs::cache_pipelines {

static uint32_t lire32(const uint8_t* p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

bool enteteValide(const uint8_t* d, size_t n, const Identite& id) {
    // VkPipelineCacheHeaderVersionOne : taille de l'en-tête (32), version (1), fournisseur, appareil, UUID (16 octets)
    if (!d || n < 32) return false;
    const uint32_t taille = lire32(d), version = lire32(d + 4);
    if (taille < 32 || taille > n || version != 1) return false;
    if (lire32(d + 8) != id.fournisseur || lire32(d + 12) != id.appareil) return false;
    return std::memcmp(d + 16, id.uuid, 16) == 0;
}

std::vector<uint8_t> lire(const std::string& c) {
    std::vector<uint8_t> v;
    if (c.empty()) return v;
    FILE* f = std::fopen(c.c_str(), "rb");
    if (!f) return v;
    if (std::fseek(f, 0, SEEK_END) == 0) {
        const long n = std::ftell(f);
        if (n > 0 && n < 64L * 1024 * 1024) {   // plus de 64 Mo : fichier anormal, ignoré
            v.resize((size_t)n); std::fseek(f, 0, SEEK_SET);
            if (std::fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
        }
    }
    std::fclose(f);
    return v;
}

bool ecrire(const std::string& c, const std::vector<uint8_t>& d) {
    if (c.empty() || d.empty()) return false;
    const std::string t = c + ".tmp";
    FILE* f = std::fopen(t.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(d.data(), 1, d.size(), f) == d.size();
    if (std::fclose(f) != 0 || !ok) { std::remove(t.c_str()); return false; }
    return std::rename(t.c_str(), c.c_str()) == 0;
}

std::string chemin() {
    std::string dos = dossier;
    if (dos.empty()) if (const char* e = std::getenv("WFS_CACHE_PIPELINES")) dos = e;
    return dos.empty() ? std::string() : dos + "/pipelines.cache";
}

} // namespace wfs::cache_pipelines
