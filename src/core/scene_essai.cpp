// WaForSpeed — système de test de la scène dédiée (WFS-44) : voir scene_essai.h.
#include "scene_essai.h"
#include <cmath>
#include "../render/renderer.h"
#include "system.h"

namespace wfs {

static bool g_actif = false;
void activerSceneEssai(bool on) { g_actif = on; }

// couleur sRGB (0xRRGGBB) → valeur HDR qui ressort à l'écran avec cette couleur, pour une exposition donnée
// (ACES de la passe finale ≈ 0,214·x pour les très faibles valeurs, puis gamma)
static vec3 fondUni(unsigned hex, float exposure) {
    auto c = [&](int sh) { const float s = ((hex >> sh) & 255) / 255.f; return std::pow(s, 2.2f) / 0.214f / exposure; };
    return {c(16), c(8), c(0)};
}

class SceneEssai : public System {
public:
    const char* name() const override { return "scene-essai"; }

    void init(Renderer& r, const GameView&) override {
        // cube de 1 m posé au sol, 6 faces avec leurs normales
        std::vector<Vertex> v; std::vector<uint32_t> idx;
        const vec3 n[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (const vec3& f : n) {
            const vec3 a = std::abs(f.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, b = cross(f, a);
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i < 4; i++) {
                const float sa = (i == 1 || i == 2) ? 0.5f : -0.5f, sb = i >= 2 ? 0.5f : -0.5f;
                const vec3 p = f * 0.5f + a * sa + b * sb + vec3{0, 0.5f, 0};
                v.push_back({p.x, p.y, p.z, f.x, f.y, f.z, sa + 0.5f, sb + 0.5f});
            }
            idx.insert(idx.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        cube_ = r.makeMesh(v, idx);
        Material m; m.color = {1.0f, 0.42f, 0.0f, 1}; m.rough = 0.45f;   // orange de la charte (#FF6A00)
        mat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
        mat_.flags |= MAT_DOUBLE;
    }
    void shutdown(Renderer& r) override { r.destroy(cube_); }

    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (!g_actif) return false;
        const vec3 eye{3.4f, 2.0f, 3.4f}, look{0, 0.5f, 0};
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        fp.view = lookAt(eye, look, {0, 1, 0});
        fp.proj = perspectiveVk(36.f * 3.14159265f / 180.f, aspect, 0.1f);
        fp.camPos = eye;
        fp.exposure = 1.0f;
        const vec3 bg = fondUni(0x07070c, fp.exposure);
        fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = bg;   // fond uni
        fp.fogDensity = 0;
        fp.sunIntensity = 2.5f; fp.sunDir = {-0.5f, 0.8f, 0.3f};     // éclaire le cube (le disque du soleil n'est pas visible)
        fp.ambient = 0.6f; fp.shadows = false; fp.hasFocus = false;
        s.cible = look;                                               // route, joueur, commandes : false (défaut)
        return true;
    }

    void render(Renderer& r, const GameView& g) override {
        if (g_actif && g.sceneDediee) r.draw(cube_, mat_, mat4::identity());
    }

private:
    Mesh cube_; Material mat_;
};
WFS_SYSTEM(SceneEssai, 900)

} // namespace wfs
