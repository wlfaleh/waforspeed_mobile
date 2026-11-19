// WaForSpeed — WFS-562 : images qui disent le mode (voir modes_photos.h).
#include "modes_photos.h"
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/commandes_tactiles.h"   // dessin2d : voile en dégradé
#include "../render/renderer.h"
#include "../../third_party/stb/stb_image.h"   // implémentation dans src/render/model_loader.cpp

namespace wfs {
namespace modes_photos {

const char* const FICHIERS[NB] = {"ui/modes/checkpoint.jpg", "ui/modes/course5.jpg", "ui/modes/poursuite.jpg"};

namespace {
Image g_planche{};
bool g_pretes = false, g_essaye = false, g_refusBanc = false;
}

bool charger(Renderer& r) {
    if (g_pretes || g_essaye) return g_pretes;
    if (g_refusBanc) { g_essaye = true; WFS_WARN("modes (WFS-562) : photos refusées par le banc, scène floutée"); return false; }
    g_essaye = true;   // un seul essai par démarrage du moteur : pas de relecture à chaque image si une photo manque
    // planche de 3 photos côte à côte (2160 × 546 RGBA, 4,7 Mo) : une seule texture, un seul jeu de descripteurs
    const int W = LARGEUR * NB, H = HAUTEUR;
    std::vector<uint8_t> rgba;
    try { rgba.assign((size_t)W * H * 4, 0); } catch (const std::bad_alloc&) { WFS_WARN("modes (WFS-562) : mémoire refusée, scène floutée"); return false; }
    for (int i = 0; i < NB; i++) {
        std::vector<uint8_t> jpg;
        if (!readAsset(FICHIERS[i], jpg)) { WFS_WARN("modes (WFS-562) : photo %s introuvable, scène floutée", FICHIERS[i]); return false; }
        int w = 0, h = 0, n = 0;
        uint8_t* px = stbi_load_from_memory(jpg.data(), (int)jpg.size(), &w, &h, &n, 4);
        if (!px || w != LARGEUR || h != HAUTEUR) {
            if (px) stbi_image_free(px);
            WFS_WARN("modes (WFS-562) : photo %s illisible ou pas en %d × %d, scène floutée", FICHIERS[i], LARGEUR, HAUTEUR);
            return false;
        }
        for (int y = 0; y < H; y++) std::memcpy(&rgba[((size_t)y * W + (size_t)i * LARGEUR) * 4], px + (size_t)y * LARGEUR * 4, (size_t)LARGEUR * 4);
        stbi_image_free(px);
    }
    g_planche = r.vk().uploadTexture(rgba.data(), (uint32_t)W, (uint32_t)H, false);   // valeurs sRGB recopiées telles quelles (comme l'atlas)
    if (!g_planche.view) { WFS_WARN("modes (WFS-562) : planche des photos refusée par la carte graphique, scène floutée"); return false; }
    r.uiSetPhotos(g_planche);
    g_pretes = true;
    WFS_LOG("modes (WFS-562) : 3 photos des modes chargées (planche %d × %d, %.1f Mo)", W, H, W * H * 4 / 1048576.0);
    return true;
}

bool pretes() { return g_pretes; }
void refuserBanc(bool oui) { g_refusBanc = oui; }

void liberer(Renderer& r) {
    if (g_planche.view) { r.uiSetPhotos(Image{}); r.vk().destroy(g_planche); }
    g_planche = Image{}; g_pretes = g_essaye = false;
}

void dessiner(Renderer& r, int i, const vec2 q[4], float x, float y, float w, float h, float yVoile0, float yVoile1, float voile) {
    if (!g_pretes || i < 0 || i >= NB) return;
    // uv de la boîte de la carte (aucune déformation : le parallélogramme découpe la photo), demi-texel à l'intérieur de la photo i
    const float du = 0.5f / (LARGEUR * NB), dv = 0.5f / HAUTEUR;
    const float u0 = (float)i / NB + du, u1 = (float)(i + 1) / NB - du;
    vec2 uv[4]; vec4 c[4];
    for (int k = 0; k < 4; k++) {
        const float s = w > 0 ? (q[k].x - x) / w : 0, t = h > 0 ? (q[k].y - y) / h : 0;
        uv[k] = {u0 + (u1 - u0) * s, dv + (1 - 2 * dv) * t};
        c[k] = {1, 1, 1, 1};
    }
    r.uiPhoto(q, uv, c);
    // voile sombre pour le nom du mode et la phrase (RG-04) : transparent au-dessus de yVoile0, opaque sous yVoile1
    dessin2d::polygone(r, q, 4, {{0.04f, 0.045f, 0.06f, 0.f}, {0.04f, 0.045f, 0.06f, voile}, yVoile0, yVoile1});
}

// libère la planche à l'arrêt du moteur (changement de carte compris) ; rechargée à la prochaine ouverture de l'écran
class ModesPhotos : public System {
public:
    const char* name() const override { return "modes : photos"; }
    void shutdown(Renderer& r) override { r.vk().waitIdle(); liberer(r); }
};

} // namespace modes_photos

WFS_SYSTEM(modes_photos::ModesPhotos, 455)   // interface (400-499)

} // namespace wfs
