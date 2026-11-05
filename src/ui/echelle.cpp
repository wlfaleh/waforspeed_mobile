// WaForSpeed — échelle de l'interface (WFS-102) : voir echelle.h.
#include "echelle.h"
#include <algorithm>
#include "../game/commandes_tactiles.h"   // densiteCommandes (WFS-108) et dessin2d

namespace wfs {
namespace ui {

float densite(float W, float) {
    if (densiteCommandes() > 0) return densiteCommandes();   // Android : lue une seule fois par WFS-108 (RG-18)
    return W >= 2000.f ? 2.625f : 1.f;                         // banc PC : 2400x1080 = téléphone 420 dpi ; 1280x720 = k 1
}

float hauteurDp(float W, float H) { return std::clamp(H / densite(W, H), 360.f, 720.f); }

float echelle(float W, float H) { return H / hauteurDp(W, H); }

void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}

} // namespace ui
} // namespace wfs
