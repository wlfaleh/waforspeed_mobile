// WaForSpeed — garage : cadre de l'écran (WFS-534) : voir garage_cadre.h. Code repris tel quel de garage.cpp.
#include "garage_cadre.h"
#include "../game/commandes_tactiles.h"   // WFS-99 : dessin2d (biseaux d'un seul tenant, traits lissés)
#include "../render/renderer.h"
#include "langue.h"   // WFS-507 : textes par ui::tr
#include "text.h"

namespace wfs::ecrans::garage::cadre {

void voiles(Renderer& r, const MiseEnPage& mp) {
    const float W = mp.W, H = mp.H, op = mp.op;
    // .screen--show : voile du bas (linear-gradient 0deg, .92 → 0 à 42 %), un seul dégradé (WFS-99 : sans bandes)
    degrade(r, 0, H * 0.58f, W, H, hex(BG, 0), hex(BG, 0.92f * op), false);
    // WFS-99 (RG-01, ANO-22) : voile .screen--garage à gauche (linear-gradient 100deg, rendu horizontal) :
    // .92 à 0 %, .70 à 34 %, 0 à 62 % de la largeur, sous le titre, l'accroche et la barre d'index
    degrade(r, 0, 0, W * 0.34f, H, hex(BG, 0.92f * op), hex(BG, 0.70f * op), true);
    degrade(r, W * 0.34f, 0, W * 0.62f, H, hex(BG, 0.70f * op), hex(BG, 0), true);
}

void fleches(Renderer& r, const MiseEnPage& mp) {
    const float W = mp.W, H = mp.H, k = mp.k, op = mp.op, gl = mp.gl, gr = mp.gr;
    // ---------- flèches .show-arrow (64 × 88, à 42 % de la hauteur) ----------
    for (int s = 0; s < 2; s++) {
        const float w = 64 * k, h = 88 * k, x = s == 0 ? gl : W - gr - w, y = H * 0.42f - h / 2;
        biseau(r, x, y, w, h, 12 * k, hex(0x14161F, 0.72f * op));
        const float cx = x + w / 2, cy = y + h / 2, d = s == 0 ? 1.f : -1.f, ep = 3.2f * k;
        const vec2 chevron[3] = {{cx + 6 * k * d, cy - 14 * k}, {cx - 8 * k * d, cy}, {cx + 6 * k * d, cy + 14 * k}};
        dessin2d::trait(r, chevron, 3, ep, hex(TEXTE, op));   // WFS-99 : trait lissé d'un seul tenant
        (s == 0 ? g_z.prev : g_z.next) = {x, y, w, h};
    }
}

void fondPanneau(Renderer& r, const MiseEnPage& mp) {
    const float k = mp.k, op = mp.op;
    {   // fond vitré d'un seul tenant, coin haut droit coupé (WFS-99, RG-06 : 14 px), bord supérieur
        const float coin = 14 * k, px0 = mp.px0, px1 = mp.px1, py0 = mp.py0;
        const vec2 p[5] = {{px0, py0}, {px1 - coin, py0}, {px1, py0 + coin}, {px1, py0 + mp.panelH}, {px0, py0 + mp.panelH}};
        dessin2d::polygone(r, p, 5, dessin2d::uni({GLASS.x, GLASS.y, GLASS.z, GLASS.w * op}));
        r.uiRect(px0, py0, px1 - px0 - coin, 1 * k, {BORD.x, BORD.y, BORD.z, BORD.w * op});
    }
}

void pied(Renderer& r, const MiseEnPage& mp, const CarDef& car, bool voiturePrete) {
    const float W = mp.W, H = mp.H, k = mp.k, op = mp.op, gr = mp.gr, gb = mp.gb;
    // .show-credit : crédit du modèle (licence obligatoire), en bas à droite ; WFS-99 (RG-06, ANO-33) : capitales de
    // 10 px à 720p (taille 14 = capitales + jambages), #8A90A6, sous le panneau
    ui::text(r, W - gr, H - gb - 17 * k, 14 * k, ui::tr("garage.credit", auteurDuCredit(car)), hex(MUTED, op), ui::DROITE, 0, false);   // WFS-191 (RG-03)
    if (!voiturePrete) {   // .show-loading
        const std::string& l = ui::tr("chargement.etape.modele");   // WFS-507 : même clé que l'écran de chargement
        const float lw = ui::width(14 * k, l, 0.08f) + 36 * k;
        r.uiRect(W / 2 - lw / 2, H * 0.44f - 22 * k, lw, 44 * k, {GLASS.x, GLASS.y, GLASS.z, GLASS.w * op});
        ui::text(r, W / 2, H * 0.44f - 8 * k, 14 * k, l, hex(TEXTE, op), ui::CENTRE, 0.08f, false);
    }
}

} // namespace wfs::ecrans::garage::cadre
