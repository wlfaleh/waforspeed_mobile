// WaForSpeed — garage : colonne d'action (WFS-534) : voir garage_achat.h. Code repris tel quel de garage.cpp.
#include "garage_achat.h"
#include "langue.h"   // WFS-507 : textes par ui::tr
#include "text.h"

namespace wfs::ecrans::garage {

// boutons (colonne de droite) : Choisir (.btn orange), Retour (.btn--ghost)
void AchatGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float k = mp.k, op = mp.op, bx = mp.bx, bw = mp.bw;
    const float hb = 52 * k, hg = 46 * k, gap = 12 * k, tot = hb + gap + hg, by = mp.py0 + (mp.panelH - tot) / 2;
    const bool pret = e.voiturePrete;
    biseau(r, bx, by, bw, hb, 14 * k, pret ? hex(ACCENT, op) : hex(SURFACE2, op));
    ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr(pret ? "garage.choisir" : "garage.chargement")), pret ? hex(ON_ACCENT, op) : hex(MUTED, op),
             ui::CENTRE, 0.1f, false);
    g_z.choisir = {bx, by, bw, hb};
    const float gy = by + hb + gap;
    biseau(r, bx, gy, bw, hg, 14 * k, hex(SURFACE2, op));
    ui::text(r, bx + bw / 2, gy + 13 * k, 16 * k, ui::majuscules(ui::tr("garage.retour")), hex(TEXTE, op), ui::CENTRE, 0.1f, false);
    g_z.retour = {bx, gy, bw, hg};
}

int AchatGarage::appui(float x, float y) const {
    if (g_z.choisir.contient(x, y)) return CHOISIR;
    if (g_z.retour.contient(x, y)) return RETOUR;
    return -1;
}

void AchatGarage::viderZones() { g_z.choisir = g_z.retour = Zone{}; }

} // namespace wfs::ecrans::garage
