// WaForSpeed — garage : rangée des couleurs (WFS-534) : voir garage_couleur.h. Code repris tel quel de garage.cpp.
#include "garage_couleur.h"
#include <algorithm>
#include "../platform/manette.h"   // WFS-80 : pastille au focus de la manette
#include "text.h"

namespace wfs::ecrans::garage {

void CouleurGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float k = mp.k, op = mp.op, paintH = mp.paintH, bx = mp.bx;
    // .paint-line : « Couleur », 16 pastilles (30 × 30, skewX(-10deg)), nom de la peinture
    const float yy = mp.y1 + mp.statH + 14 * k;
    float cx = mp.px0 + mp.padX;
    ui::text(r, cx, yy + 9 * k, 13 * k, ui::majuscules("Couleur"), hex(MUTED, op), ui::GAUCHE, 0.16f, false);
    cx += ui::width(13 * k, "COULEUR", 0.16f) + 16 * k;
    const float s = std::min(30 * k, (bx - 32 * k - cx - 140 * k) / NB_PEINTURES - 8 * k);
    for (int i = 0; i < NB_PEINTURES; i++) {
        const bool on = i == e.peinture;
        if (on) biseau(r, cx - 4 * k, yy + (paintH - s) / 2 - 4 * k, s + 8 * k, s + 8 * k, s * 0.18f, hex(ACCENT2, 0.35f * op));   // halo
        biseau(r, cx - 2 * k, yy + (paintH - s) / 2 - 2 * k, s + 4 * k, s + 4 * k, s * 0.18f, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
        biseau(r, cx, yy + (paintH - s) / 2, s, s, s * 0.18f, hex(PAINTS[i].hex, op));
        g_z.peintures[i] = {cx - 4 * k, yy - 4 * k, s + 8 * k, paintH + 8 * k};
        manette::focus(g_z.peintures[i]);   // WFS-80 : pastille au focus de la manette (A = toucher)
        cx += s + 8 * k;
    }
    ui::text(r, cx + 6 * k, yy + 6 * k, 15 * k, PAINTS[e.peinture].nom, hex(TEXTE, op), ui::GAUCHE, 0, false);
}

int CouleurGarage::appui(float x, float y) const {
    for (int i = 0; i < NB_PEINTURES; i++) if (g_z.peintures[i].contient(x, y)) return i;
    return -1;
}

void CouleurGarage::viderZones() { for (Zone& z : g_z.peintures) z = Zone{}; }

} // namespace wfs::ecrans::garage
