// WaForSpeed — WFS-537 (RG-07 à RG-09) : compteur d'étoiles du menu principal ; voir compteur_etoiles.h.
// Style du bouton « Réglages » (.cam-btn de design-street-racing) : vitré, bordure 1 px, biais 8 px, même hauteur, texte de
// 15 px ; ★ orange (accent de la monnaie), solde en blanc. Il est posé à 12 px à gauche du bouton « Réglages ».
#include "compteur_etoiles.h"
#include <algorithm>
#include <cmath>
#include "../../core/system.h"
#include "../../game/boutique.h"
#include "../../game/sauvegarde.h"
#include "../../game/score_fin.h"        // dessinerEtoile
#include "../../game/score_fin_calc.h"   // nombreEtoiles
#include "../../render/renderer.h"
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "reglages.h"
#include "reglages_appui.h"
#include "creation_profil.h"   // WFS-481 : écran « Crée ton pilote »
#include "../credits.h"
#include "cadeau_jour.h"   // WFS-501 : écran du cadeau du jour (compteur caché dessous)
#include "../../game/course5.h"   // choixModeOuvert

namespace wfs::ecrans::compteur_etoiles {

namespace {
struct Zone { float x = 0, y = 0, w = 0, h = 0; };
Zone g_zone;          // dessin à la dernière image (w = 0 : pas dessiné)
float g_dp = 1;       // pixels par dp (cible de 48 dp)
bool g_presse = false;   // état appuyé (posé par le menu)
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f}, TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1},
           VOILE{0.f, 229 / 255.f, 1.f, 0.22f};

// parallélogramme du bouton « Réglages » (haut décalé à droite de `biais`)
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    const UiVertex q[4] = {{x + biais, y, -1, -1, c.x, c.y, c.z, c.w}, {x + w, y, -1, -1, c.x, c.y, c.z, c.w},
                           {x + w - biais, y + h, -1, -1, c.x, c.y, c.z, c.w}, {x, y + h, -1, -1, c.x, c.y, c.z, c.w}};
    r.uiQuad(q);
}
long long solde() { return boutique::solde(sauvegarde()); }
bool anglais() { return ui::langue::active() == ui::langue::Langue::Anglais; }
} // namespace

void appuye(bool oui) { g_presse = oui; }

namespace {

// RG-07 : dessiné juste après « Réglages » (même image) ; a : opacité du fondu d'entrée du menu
void dessiner(Renderer& r, const GameView& g, float bx, float by, float bh, float a) {
    const float W = g.uiWidth, H = g.uiHeight, kb = bh / 44.f;    // même unité que le bouton (44 px en 720 p)
    const float k = ui::petitEcran(W, H) ? ui::unitePetitEcran(W, H) : H / 720.f;
    g_dp = ui::pixelsParDp(W);
    const float tp = 15 * k, pad = 16 * kb, re = tp * 0.48f, ecart = 8 * kb;
    const std::string t = score_fin::nombreEtoiles(solde(), anglais());
    const float tw = ui::width(tp, t, 0.06f), w = pad + 2 * re + ecart + tw + pad;
    const float x = bx - std::max(12 * kb, 10 * g_dp) - w, y = by, h = bh;   // ≥ 10 dp : les cibles élargies (4 dp) ne se touchent pas
    g_zone = {x, y, w, h};
    auto op = [a](vec4 c) { c.w *= a; return c; };
    biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, op(BORD));
    biseau(r, x, y, w, h, 8 * kb, op(GLASS));
    if (g_presse) biseau(r, x, y, w, h, 8 * kb, op(VOILE));   // appuyé : voile cyan, comme « Réglages »
    score_fin::dessinerEtoile(r, x + pad + re, y + h / 2, re, op(ui::ORANGE));
    ui::text(r, std::round(x + pad + 2 * re + ecart), std::round(y + (h - tp) / 2), tp, t, op(TEXTE), ui::GAUCHE, 0.06f, false);
}

class CompteurEtoiles final : public System {
public:
    const char* name() const override { return "compteur-etoiles"; }
    void onEcran(Ecran, Ecran, const GameView&) override { t0_ = -1; g_zone = {}; }
    void drawUi(Renderer& r, const GameView& g) override {
        float bx, by, bw, bh;
        const bool bouton = boutonReglages(bx, by, bw, bh);   // lu à chaque image : « Réglages » dessiné à cette image ?
        if (!bouton || g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady() || choixModeOuvert() || credits::ouvert() || creationProfilOuverte() || cadeau_jour::ouvert()) {
            g_zone = {}; t0_ = -1; return;   // rien sans « Réglages », ni sous un panneau du menu ; fondu au retour
        }
        if (t0_ < 0 || g.time < t0_) t0_ = g.time;
        const float t = std::clamp((float)(g.time - t0_) / 0.3f, 0.f, 1.f);   // fondu d'entrée du menu (300 ms)
        dessiner(r, g, bx, by, bh, t * t * (3 - 2 * t));
    }
private:
    double t0_ = -1;
};

} // namespace

bool dans(float x, float y) { return appui::dansCible(g_zone.x, g_zone.y, g_zone.w, g_zone.h, g_dp, x, y); }

std::string texteLecteur() { const long long n = solde(); return ui::tr(score_fin::singulier(n, anglais()) ? "menu.etoiles.lecteur.un" : "menu.etoiles.lecteur", score_fin::nombreEtoiles(n, anglais())); }

bool centre(float& x, float& y) {
    if (g_zone.w <= 0) return false;
    x = g_zone.x + g_zone.w / 2; y = g_zone.y + g_zone.h / 2; return true;
}

} // namespace wfs::ecrans::compteur_etoiles

namespace wfs { WFS_SYSTEM(ecrans::compteur_etoiles::CompteurEtoiles, 461) }   // interface : juste après les Réglages (460), avant les Crédits (470)
