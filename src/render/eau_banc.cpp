// WaForSpeed — WFS-380 : caméra libre du banc de l'eau (voir eau_banc.h). Même principe que la caméra haute du banc des cartes
// (cartes_banc.cpp, WFS-212) : le crochet ambiance remplace la vue, rien d'autre ne change.
#include "eau_banc.h"
#include <algorithm>
#include "../core/system.h"
#include "renderer.h"

namespace wfs {
namespace {
struct VueEau { bool actif = false; vec3 oeil{}, cible{}; float fov = 55; } g_vue;
class EauVueBanc : public System {
public:
    const char* name() const override { return "eau-vue-banc"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_vue.actif) return;
        fp.view = lookAt(g_vue.oeil, g_vue.cible, {0, 1, 0});
        fp.proj = perspectiveVk(g_vue.fov * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
        fp.camPos = g_vue.oeil;
    }
};
} // namespace
WFS_SYSTEM(EauVueBanc, 996)

void eau::vueBanc(bool actif, vec3 oeil, vec3 cible, float fov) { g_vue = {actif, oeil, cible, fov}; }
} // namespace wfs
