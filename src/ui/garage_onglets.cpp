// WaForSpeed — garage : onglets, panneau de l'onglet, nom de l'objet montré et bouton rond (WFS-551) : voir garage_onglets.h.
#include "garage_onglets.h"
#include <algorithm>
#include <cmath>
#include "../core/log.h"
#include "../game/commandes_tactiles.h"   // dessin2d
#include "../platform/manette.h"
#include "../render/renderer.h"
#include "boutons.h"
#include "garage_achat.h"   // texteEtoiles
#include "langue.h"
#include "text.h"
#include "verre.h"

namespace wfs::ecrans::garage {
namespace {
int g_onglet = ONGLET_VOITURE;
const char* const CLES[NB_ONGLETS_GARAGE] = {"garage.onglet.voiture", "garage.onglet.peinture", "garage.onglet.finition", "garage.onglet.skins"};
// centres des onglets, en part de la largeur de l'écran (maquettes 06, 07, 13, 16 : 610, 785, 960, 1113 px sur 1400)
constexpr float CENTRES[NB_ONGLETS_GARAGE] = {0.436f, 0.561f, 0.686f, 0.795f};
} // namespace

int ongletGarage() { return g_onglet; }
void choisirOnglet(int o) {
    if (o < 0 || o >= NB_ONGLETS_GARAGE || o == g_onglet) return;
    g_onglet = o;
    WFS_LOG("garage : onglet %d (%s)", o, CLES[o]);
}

float panneauOnglet(Renderer& r, const MiseEnPage& mp, float w) {
    w = std::min(w, mp.px1 - mp.px0);
    const float x = std::clamp(mp.W / 2 - w / 2, mp.px0, mp.px1 - w);
    verre::panneau(r, x, mp.py0, w, mp.panelH, {12 * mp.k, 1.5f * mp.k, mp.op, true, 0, 1});
    return x;
}

void nomMontre(Renderer& r, const MiseEnPage& mp, const std::string& nom, bool apercu) {
    const float k = mp.k, op = mp.op;
    ui::text(r, mp.cxBouton, mp.yNom, 22 * k, ui::majuscules(nom), hex(TEXTE, op), ui::CENTRE, 0.02f, true, true);
    if (apercu) ui::text(r, mp.cxBouton, mp.yNom + 28 * k, 12 * k, ui::tr("garage.apercu_pas_a_toi"), hex(MUTED, op), ui::CENTRE, 0, false);
}

namespace onglets {

void dessiner(Renderer& r, const MiseEnPage& mp) {
    const float k = mp.k, op = mp.op, h = 26 * k, w = 112 * k, y = mp.gt + 34 * k;
    for (int o = 0; o < NB_ONGLETS_GARAGE; o++) {
        const float cx = mp.W * CENTRES[o];
        const std::string t = ui::majuscules(ui::tr(CLES[o]));
        if (o == g_onglet) {
            // onglet ouvert : titre en néon cyan (tube blanc, lueur cyan), lumière qui tombe dessous (maquettes)
            const float px = 30 * k;
            dessin2d::couronne(r, cx, y + h * 0.9f, 0, 46 * k, hex(ACCENT2, 0.30f * op), hex(ACCENT2, 0));
            r.uiCircle(cx, y + h * 0.95f, 7 * k, hex(0xBFFBFF, 0.85f * op));
            for (int i = 0; i < 3; i++)   // lueur : le titre redessiné un peu plus gros, très transparent
                ui::text(r, cx, y + h / 2 - px * 0.62f - i * 0.6f * k, px + i * 1.2f * k, t, hex(ACCENT2, 0.22f * op), ui::CENTRE, 0.02f, false, true);
            ui::text(r, cx, y + h / 2 - px * 0.6f, px, t, hex(0xC8FCFF, op), ui::CENTRE, 0.02f, false, true);
        } else {
            // onglet fermé : plaque claire, liseré orange à gauche, texte sombre en capitales grasses
            const float x = cx - w / 2;
            vec2 p[64]; const int n = ui::boutons::contourArrondi(p, x, y, w, h, 2.5f * k);
            dessin2d::halo(r, p, n, 6 * k, {1, 1, 1, 0.10f * op});
            dessin2d::polygone(r, p, n, dessin2d::uni(hex(0xE3E5EA, op)));
            r.uiRect(x + 2 * k, y + h * 0.55f, w - 4 * k, h * 0.45f - 2 * k, hex(0xC9CCD3, op));
            dessin2d::contour(r, p, n, 1.5f * k, hex(0x2A2D36, op));
            r.uiRect(x + 4 * k, y + 5 * k, 6 * k, h - 10 * k, hex(ACCENT, op));
            ui::textBoite(r, cx + 6 * k, y, w - 24 * k, h, 16 * k, t, hex(0x111318, op), ui::CENTRE, 0.04f, false, false);
        }
        g_z.ongletsGarage[o] = {cx - w / 2 - 6 * k, y - 8 * k, w + 12 * k, h + 16 * k};
        manette::focus(g_z.ongletsGarage[o]);
    }
}

int appui(float x, float y) {
    for (int o = 0; o < NB_ONGLETS_GARAGE; o++) if (g_z.ongletsGarage[o].contient(x, y)) return o;
    return -1;
}

void viderZones() { for (Zone& z : g_z.ongletsGarage) z = Zone{}; }

void boutonRond(Renderer& r, float cx, float cy, float R, const std::string& titre, const std::string& sousTitre, float op, bool grise) {
    const float d = 2 * R, x = cx - R, y = cy - R;
    // même dessin que le Start/Stop de WFS-549 en forme ronde ; le sous-titre (prix avec ★) est posé ici, l'étoile dessinée
    const float px = 0.2f * d;
    ui::boutons::startStop(r, x, y, d, d, titre, op, false, sousTitre.empty() ? "" : " ", px, grise);
    if (sousTitre.empty()) return;
    const float s = px * 0.75f, total = s + px * 0.25f + px + px * 0.8f, yt = y + d / 2 - total / 2 + s + px * 0.25f;
    texteEtoiles(r, cx, yt + px + px * 0.09f, px * 0.62f, sousTitre, hex(0xC9CDDA, op), ui::CENTRE, 0.04f);
}

} // namespace onglets
} // namespace wfs::ecrans::garage
