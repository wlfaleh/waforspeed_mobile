// WaForSpeed — WFS-420 (RG-03) : mémoire de la carte graphique par type. Voir memoire_gpu.h.
#include "memoire_gpu.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>

namespace wfs::memoire_gpu {
namespace {
std::atomic<int64_t> g_octets[N_GENRES];
struct Format { VkFormat f = VK_FORMAT_UNDEFINED; int64_t octets = 0; int images = 0; };
constexpr int N_FORMATS = 32;   // formats de textures distincts suivis (au-delà : comptés dans « autres formats »)
Format g_formats[N_FORMATS]; int64_t g_autresFormats = 0;
std::mutex g_verrou;
std::string mo1(double o) { char b[24]; std::snprintf(b, sizeof b, "%.0f", o / (1024.0 * 1024.0)); return b; }
}

std::string nomFormat(VkFormat f) {
    switch (f) {
    case VK_FORMAT_R8G8B8A8_UNORM: return "RGBA8";
    case VK_FORMAT_R8G8B8A8_SRGB: return "RGBA8 sRGB";
    case VK_FORMAT_B8G8R8A8_UNORM: return "BGRA8";
    case VK_FORMAT_B8G8R8A8_SRGB: return "BGRA8 sRGB";
    case VK_FORMAT_R8_UNORM: return "R8";
    case VK_FORMAT_R8G8_UNORM: return "RG8";
    case VK_FORMAT_R16G16B16A16_SFLOAT: return "RGBA16F";
    case VK_FORMAT_R16G16_SFLOAT: return "RG16F";
    case VK_FORMAT_R16_SFLOAT: return "R16F";
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "R11G11B10F";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "RGB10A2";
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "BC1";
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "BC1 sRGB";
    case VK_FORMAT_BC3_UNORM_BLOCK: return "BC3";
    case VK_FORMAT_BC3_SRGB_BLOCK: return "BC3 sRGB";
    case VK_FORMAT_BC4_UNORM_BLOCK: return "BC4";
    case VK_FORMAT_BC5_UNORM_BLOCK: return "BC5";
    case VK_FORMAT_BC7_UNORM_BLOCK: return "BC7";
    case VK_FORMAT_BC7_SRGB_BLOCK: return "BC7 sRGB";
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "ETC2";
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "ETC2 sRGB";
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "ETC2 A8";
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "ETC2 A8 sRGB";
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "EAC RG11";
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "ASTC 4x4";
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "ASTC 4x4 sRGB";
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "ASTC 6x6";
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "ASTC 6x6 sRGB";
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "ASTC 8x8";
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "ASTC 8x8 sRGB";
    case VK_FORMAT_D32_SFLOAT: return "D32F";
    case VK_FORMAT_D16_UNORM: return "D16";
    case VK_FORMAT_D24_UNORM_S8_UINT: return "D24S8";
    default: return "format " + std::to_string((int)f);
    }
}

void compter(Genre g, int64_t octets, VkFormat fmt) {
    if (g >= N_GENRES || octets == 0) return;
    g_octets[g] += octets;
    if (g != TEXTURES) return;
    std::lock_guard<std::mutex> v(g_verrou);
    for (Format& e : g_formats) if (e.f == fmt && e.images > 0) { e.octets += octets; e.images += octets > 0 ? 1 : -1; return; }
    if (octets < 0) { g_autresFormats += octets; return; }   // format d'avant la table pleine
    for (Format& e : g_formats) if (e.images <= 0) { e = {fmt, octets, 1}; return; }
    g_autresFormats += octets;
}

double mo(Genre g) { return g < N_GENRES ? g_octets[g].load() / (1024.0 * 1024.0) : 0; }

std::string texte() {
    Format f[N_FORMATS]; int64_t autres;
    { std::lock_guard<std::mutex> v(g_verrou); std::copy(g_formats, g_formats + N_FORMATS, f); autres = g_autresFormats; }
    std::sort(f, f + N_FORMATS, [](const Format& a, const Format& b) { return a.octets > b.octets; });
    std::string t;
    for (int g = 0; g < N_GENRES; g++) {
        t += (g ? " · " : "") + std::string(NOMS[g]) + " " + mo1((double)g_octets[g].load()) + " Mo";
        if (g != TEXTURES) continue;
        std::string d;
        for (const Format& e : f) if (e.images > 0 && e.octets > 0) d += (d.empty() ? "" : ", ") + nomFormat(e.f) + " " + mo1((double)e.octets) + " (" + std::to_string(e.images) + ")";
        if (autres > 0) d += (d.empty() ? "" : ", ") + std::string("autres formats ") + mo1((double)autres);
        if (!d.empty()) t += " [" + d + "]";
    }
    return t;
}

} // namespace wfs::memoire_gpu
