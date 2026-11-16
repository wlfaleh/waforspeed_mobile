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
Zone g_solde;   // cadre du solde (dernière image) : ouvre le Market (RG-12)
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
    ui::text(r, mp.cxBouton, mp.yNom, 26 * k, ui::majuscules(nom), hex(TEXTE, op), ui::CENTRE, 0.02f, true, true);   // WFS-557 : nom de 26
    if (apercu) { ui::AvecPolice inter(ui::Police::Inter); ui::text(r, mp.cxBouton, mp.yNom + 30 * k, 13 * k, ui::tr("garage.apercu_pas_a_toi"), hex(0xA3A8B5, op), ui::CENTRE, 0, false); }   // WFS-557 : Inter de 13 (maquettes 06, 07, 16)
}

namespace onglets {

void dessiner(Renderer& r, const MiseEnPage& mp) {
    const float k = mp.k, op = mp.op, h = 26 * k, w = 116 * k, y = mp.gt + 34 * k;
    for (int o = 0; o < NB_ONGLETS_GARAGE; o++) {
        const float cx = mp.W * CENTRES[o];
        const std::string t = ui::majuscules(ui::tr(CLES[o]));
        if (o == g_onglet) {
            // onglet ouvert : titre en néon cyan (tube blanc, lueur cyan), lumière qui tombe dessous (maquettes)
            const float px = 46 * k;   // WFS-557 : néon de 128 × 28 (maquettes 13, 15)
            dessin2d::couronne(r, cx, y + h * 0.9f, 0, 46 * k, hex(ACCENT2, 0.30f * op), hex(ACCENT2, 0));
            r.uiCircle(cx, y + h * 0.95f, 7 * k, hex(0xBFFBFF, 0.85f * op));
            // maquettes : lettres en tube de néon (contour creux), lueur cyan autour, carré de lumière dessous
            r.uiRect(cx - 9 * k, y + h + 2 * k, 18 * k, 14 * k, hex(0x9BF6FF, 0.55f * op));
            dessin2d::couronne(r, cx, y + h + 9 * k, 0, 34 * k, hex(ACCENT2, 0.55f * op), hex(ACCENT2, 0));
            for (int i = 0; i < 3; i++)   // lueur : le titre plein redessiné un peu plus gros, très transparent
                ui::text(r, cx, y + h / 2 - px * 0.62f - i * 0.6f * k, px + i * 1.2f * k, t, hex(ACCENT2, 0.24f * op), ui::CENTRE, 0.02f, false, false);
            ui::contourNeon(true);
            ui::text(r, cx, y + h / 2 - px * 0.6f, px, t, hex(0xA8FAFF, op), ui::CENTRE, 0.02f, false, false);
            ui::contourNeon(false);
        } else {
            // onglet fermé : plaque claire, liseré orange à gauche, texte sombre en capitales grasses
            const float x = cx - w / 2;
            vec2 p[64]; const int n = ui::boutons::contourArrondi(p, x, y, w, h, 2.5f * k);
            dessin2d::halo(r, p, n, 6 * k, {1, 1, 1, 0.10f * op});
            dessin2d::polygone(r, p, n, dessin2d::uni(hex(0xE3E5EA, op)));
            r.uiRect(x + 2 * k, y + h * 0.55f, w - 4 * k, h * 0.45f - 2 * k, hex(0xC9CCD3, op));
            dessin2d::contour(r, p, n, 1.5f * k, hex(0x2A2D36, op));
            r.uiRect(x + 4 * k, y + 5 * k, 6 * k, h - 10 * k, hex(ACCENT, op));
            ui::textBoite(r, cx + 6 * k, y, w - 22 * k, h, 26 * k, t, hex(0x111318, op), ui::CENTRE, 0.0f, false, false);   // WFS-557 : capitales de 16 de haut (maquettes)
        }
        g_z.ongletsGarage[o] = {cx - w / 2 - 6 * k, y - 8 * k, w + 12 * k, h + 16 * k};
        manette::focus(g_z.ongletsGarage[o]);
    }
}

int appui(float x, float y) {
    for (int o = 0; o < NB_ONGLETS_GARAGE; o++) if (g_z.ongletsGarage[o].contient(x, y)) return o;
    return -1;
}

void viderZones() { for (Zone& z : g_z.ongletsGarage) z = Zone{}; g_solde = Zone{}; }

bool dansSolde(float x, float y) { return g_solde.contient(x, y); }

void boutonRond(Renderer& r, float cx, float cy, float R, const std::string& titre, const std::string& sousTitre, float op, bool grise) {
    const float d = 2 * R, x = cx - R, y = cy - R;
    // même dessin que le Start/Stop de WFS-549 en forme ronde ; le sous-titre (prix avec ★) est posé ici, l'étoile dessinée
    // WFS-557 (maquettes 13 et 15 : « CHOISIR » / « ACHETER » au même chrome brillant que les boutons du menu, maquette 01)
    (void)d; (void)x; (void)y;
    const float kk = R / 117.f;
    ui::boutons::rondMenu(r, cx, cy, kk, titre, "", op, false, grise);
    if (sousTitre.empty()) return;
    texteEtoiles(r, cx, cy + 22 * kk, 22 * kk, sousTitre, hex(0xC6C7CC, op), ui::CENTRE, 0.04f);
}

void fleches(Renderer& r, const MiseEnPage& mp) {
    // maquettes 13 et 15 : grosses flèches pleines à 46 % de la hauteur ; précédente en bois sombre, suivante en orange vif
    const float k = mp.k, op = mp.op, w = 88 * k, h = 60 * k, y = mp.H * 0.46f - h / 2;   // WFS-557 : 88 × 60 (maquettes 13, 15)
    for (int s = 0; s < 2; s++) {
        const float x = s == 0 ? mp.gl + 14 * k : mp.W - mp.gr - 15 * k - w;
        auto X = [&](float u) { return s == 1 ? x + u * w : x + (1 - u) * w; };   // flèche gauche : miroir
        // WFS-557 (maquettes 13, 15) : flèche d'un seul tenant (tige de la moitié de la hauteur, pointe sur 45 %), dégradé vertical,
        // arête claire autour du contour entier (pas de trait entre la tige et la pointe)
        vec2 tige[4] = {{X(0), y + 0.27f * h}, {X(0.56f), y + 0.27f * h}, {X(0.56f), y + 0.73f * h}, {X(0), y + 0.73f * h}};
        vec2 tete[3] = {{X(0.55f), y}, {X(1), y + 0.5f * h}, {X(0.55f), y + h}};
        vec2 tour[7] = {{X(0), y + 0.27f * h}, {X(0.55f), y + 0.27f * h}, {X(0.55f), y}, {X(1), y + 0.5f * h}, {X(0.55f), y + h},
                        {X(0.55f), y + 0.73f * h}, {X(0), y + 0.73f * h}};
        if (s == 0) { std::swap(tige[1], tige[3]); std::swap(tete[0], tete[2]); for (int i = 0; i < 3; i++) std::swap(tour[i], tour[6 - i]); }   // miroir : sens horaire gardé
        const vec4 haut = s ? hex(0xFFC94F, op) : hex(0x7E5A3B, op), bas = s ? hex(0xFF8A00, op) : hex(0x3E2A1A, op);
        const dessin2d::Teinte t{haut, bas, y, y + h};
        if (s) dessin2d::halo(r, tour, 7, 12 * k, hex(ACCENT, 0.30f * op));
        else dessin2d::halo(r, tour, 7, 8 * k, {0, 0, 0, 0.45f * op});
        dessin2d::polygone(r, tige, 4, t);
        dessin2d::polygone(r, tete, 3, t);
        if (!s) for (int i = 0; i < 3; i++)   // veines du bois sur la tige
            r.uiRect(std::min(X(0.03f), X(0.5f)), y + (0.38f + 0.11f * i) * h, std::fabs(X(0.5f) - X(0.03f)), 1.2f * k, hex(0x2A1C10, 0.45f * op));
        dessin2d::contour(r, tour, 7, 1.6f * k, s ? hex(0xFFE2A0, 0.9f * op) : hex(0xE8DCCB, 0.85f * op));   // arête claire (maquettes)
        (s == 0 ? g_z.prev : g_z.next) = {x - 8 * k, y - 12 * k, w + 16 * k, h + 24 * k};
    }
}

void solde(Renderer& r, const MiseEnPage& mp, long long etoiles) {
    // maquettes : en haut à droite, cadre de verre sombre incliné, étoile orange et solde en italique
    const float k = mp.k, op = mp.op, px = 25 * k;   // WFS-557 (maquettes 13, 15) : cadre de 40 de haut, solde de 16 de haut
    const std::string t = ui::nombre(etoiles);
    const float w = ui::width(px, t) + 62 * k, h = 40 * k, x = mp.W - mp.gr - 16 * k - w, y = mp.gt + 12 * k;
    g_solde = {x - 6 * k, y - 6 * k, w + 12 * k, h + 12 * k};
    dessin2d::biseau(r, x, y, w, h, 8 * k, dessin2d::uni(hex(0x0E1016, 0.85f * op)));
    const vec2 c[4] = {{x + 8 * k, y}, {x + w, y}, {x + w - 8 * k, y + h}, {x, y + h}};
    dessin2d::contour(r, c, 4, 1.2f * k, {1, 1, 1, 0.22f * op});
    etoile(r, x + 26 * k, y + h / 2, 12 * k, hex(ACCENT, op));
    ui::text(r, x + 44 * k, y + h / 2 - px * 0.6f, px, t, hex(TEXTE, op), ui::GAUCHE, 0, false, true);
}

} // namespace onglets
} // namespace wfs::ecrans::garage
