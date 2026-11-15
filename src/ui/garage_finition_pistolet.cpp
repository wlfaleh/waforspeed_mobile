// WaForSpeed — garage : icône du pistolet de peinture de l'utilisateur (WFS-551, RG-03) : voir garage_finition_pistolet.h.
#include "garage_finition_pistolet.h"
#include <algorithm>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../render/renderer.h"
#include "../../third_party/stb/stb_image.h"   // implémentation dans src/render/model_loader.cpp

namespace wfs::ecrans::garage::pistolet {
namespace {
bool g_pret = false;
float g_u0 = 0, g_v0 = 0, g_u1 = 0, g_v1 = 0;   // place de l'icône dans l'atlas (0..1)
constexpr int MARGE = 8;                          // texels mis à zéro autour (les mipmaps ne mélangent pas les caractères voisins)
} // namespace

void ajouterAtlas(uint8_t* rgba, int cote, int yLibre) {
    g_pret = false;
    const int x0 = cote - COTE, y0 = cote - COTE;
    if (yLibre > y0 - MARGE) { WFS_WARN("Icône du pistolet : atlas de police trop plein (ligne libre %d)", yLibre); return; }
    std::vector<uint8_t> png;
    if (!readAsset("ui/icone-pistolet.png", png)) { WFS_WARN("Icône du pistolet introuvable (ui/icone-pistolet.png)"); return; }
    int w = 0, h = 0, n = 0;
    uint8_t* px = stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4);
    if (!px || w != COTE || h != COTE) { if (px) stbi_image_free(px); WFS_WARN("Icône du pistolet : image illisible ou pas en %d px", COTE); return; }
    for (int y = y0 - MARGE; y < cote; y++)       // marge et zone de l'icône : canaux G, B, A seulement (R = police)
        for (int x = x0 - MARGE; x < cote; x++) {
            uint8_t* d = rgba + ((size_t)y * cote + x) * 4;
            d[1] = d[2] = d[3] = 0;
            if (x < x0 || y < y0) continue;
            const uint8_t* s = px + ((size_t)(y - y0) * COTE + (x - x0)) * 4;
            const Masque m = masque(s[0], s[1], s[2], s[3]);
            d[1] = m.g; d[2] = m.b; d[3] = m.a;
        }
    stbi_image_free(px);
    // demi-texel à l'intérieur : le filtrage ne lit pas la marge
    g_u0 = (x0 + 0.5f) / cote; g_v0 = (y0 + 0.5f) / cote; g_u1 = (cote - 0.5f) / cote; g_v1 = (cote - 0.5f) / cote;
    g_pret = true;
    WFS_LOG("Icône du pistolet rangée dans l'atlas (%d px)", COTE);
}

bool pret() { return g_pret; }

void dessiner(Renderer& r, float x, float y, float s, const vec4 (&t)[4]) {
    if (!g_pret || s <= 0) return;
    // u décalé de +4 : le shader d'interface lit les masques de l'icône (shaders/ui.frag)
    const UiVertex q[4] = {{x, y, g_u0 + 4, g_v0, t[0].x, t[0].y, t[0].z, t[0].w},
                           {x + s, y, g_u1 + 4, g_v0, t[1].x, t[1].y, t[1].z, t[1].w},
                           {x + s, y + s, g_u1 + 4, g_v1, t[2].x, t[2].y, t[2].z, t[2].w},
                           {x, y + s, g_u0 + 4, g_v1, t[3].x, t[3].y, t[3].z, t[3].w}};
    r.uiQuad(q);
}

} // namespace wfs::ecrans::garage::pistolet
