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
#include "profil.h"   // WFS-482 : écran Profil (compteur caché dessous)
#include "market_ecran.h"   // WFS-513 : écran Market (compteur caché dessous)
#include "../../game/course5.h"   // choixModeOuvert
#include "../verre.h"   // WFS-548 : rien sous une fenêtre de verre (verre::couvert)
#include "../../game/commandes_tactiles.h"   // WFS-550 : dessin2d (pilule arrondie)

namespace wfs::ecrans::compteur_etoiles {

namespace {
struct Zone { float x = 0, y = 0, w = 0, h = 0; };
Zone g_zone;          // dessin à la dernière image (w = 0 : pas dessiné)
float g_dp = 1;       // pixels par dp (cible de 48 dp)
bool g_presse = false;   // état appuyé (posé par le menu)
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f}, TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1},
           VOILE{0.f, 229 / 255.f, 1.f, 0.22f};

long long solde() { return boutique::solde(sauvegarde()); }
// WFS-550 (maquettes 26 et 29) : pilule arrondie du solde (remplissage, puis contour de `ep` pixels si ep > 0)
void pilule(Renderer& r, float x, float y, float w, float h, const vec4& c, float ep = 0) {
    vec2 p[verre::NP_CONTOUR];
    verre::contourArrondi(x, y, w, h, h / 2, p);
    if (ep > 0) dessin2d::contour(r, p, verre::NP_CONTOUR, ep, c);
    else dessin2d::polygone(r, p, verre::NP_CONTOUR, dessin2d::uni(c));
}

// WFS-550 (RG-05, maquette 29) : animation d'un gain, affichage seulement (le solde enregistré n'est jamais touché ici).
// Le compteur garde le dernier solde montré ; quand le solde enregistré monte (cadeau, fin de course, Market), au retour du
// compteur à l'écran : des étoiles volent de la voiture vers la pilule, « +N » orange (gain réel), la pilule s'allume et compte
// jusqu'au nouveau total en ≈ 1 s. Une baisse (achat) s'affiche tout de suite, sans animation.
struct Gain {
    long long vu = -1;            // dernier solde connu (−1 : rien vu depuis le lancement : pas d'animation au 1er affichage)
    long long depart = 0, gain = 0;
    double debut = -1;            // temps de la partie au début de l'animation (−1 : aucune)
};
Gain g_gain;
constexpr double VOL = 0.65, COMPTE_DEBUT = 0.35, COMPTE = 1.0, FIN = 1.6;   // secondes
constexpr int NB_ETOILES = 7;
vec4 orange(float a) { vec4 c = ui::ORANGE; c.w = a; return c; }   // --accent, accent de la monnaie
bool anglais() { return ui::langue::active() == ui::langue::Langue::Anglais; }
} // namespace

void appuye(bool oui) { g_presse = oui; }

namespace {

// solde à montrer à l'instant `t` (comptage rapide du départ au nouveau total, sortie douce)
long long soldeMontre(double t) {
    const long long s = solde();
    if (g_gain.vu < 0) g_gain.vu = s;
    if (s > g_gain.vu) { g_gain.depart = g_gain.vu; g_gain.gain = s - g_gain.vu; g_gain.debut = t; }   // nouveau gain (ou cumul)
    else if (s < g_gain.vu) g_gain.debut = -1;                                                       // dépense : tout de suite
    g_gain.vu = s;
    if (g_gain.debut < 0 || t < g_gain.debut || t - g_gain.debut >= FIN) { g_gain.debut = -1; return s; }
    const float u = std::clamp((float)((t - g_gain.debut - COMPTE_DEBUT) / COMPTE), 0.f, 1.f), e = 1 - (1 - u) * (1 - u) * (1 - u);
    return g_gain.depart + (long long)std::llround((double)(s - g_gain.depart) * e);
}

// RG-07 : dessiné juste après « Réglages » (même image) ; a : opacité du fondu d'entrée du menu
void dessiner(Renderer& r, const GameView& g, float bx, float by, float bh, float a) {
    const float W = g.uiWidth, H = g.uiHeight, kb = bh / 44.f;    // même unité que le bouton (44 px en 720 p)
    const float k = ui::petitEcran(W, H) ? ui::unitePetitEcran(W, H) : H / 720.f;
    g_dp = ui::pixelsParDp(W);
    const float tp = 18 * k, pad = 22 * kb, re = tp * 0.50f, ecart = 10 * kb;   // WFS-557 (maquette 01 : pilule 102 × 45, « 1 250 » de 18 px)
    const std::string t = score_fin::nombreEtoiles(soldeMontre(g.time), anglais());
    const float tw = ui::width(tp, t, 0.06f), w = pad + 2 * re + ecart + tw + pad;
    const float x = bx - std::max(12 * kb, 10 * g_dp) - w, y = by, h = bh;   // ≥ 10 dp : les cibles élargies (4 dp) ne se touchent pas
    g_zone = {x, y, w, h};
    auto op = [a](vec4 c) { c.w *= a; return c; };
    // maquettes 26 et 29 : pilule de verre arrondie, liseré clair
    pilule(r, x, y, w, h, op(GLASS));
    pilule(r, x, y, w, h, op(BORD), std::max(1.f, kb));
    if (g_presse) pilule(r, x, y, w, h, op(VOILE));   // appuyé : voile cyan, comme « Réglages »
    if (g_gain.debut >= 0) {   // WFS-550 (RG-05) : la pilule s'allume (liseré orange) pendant le comptage, puis s'éteint
        const double age = g.time - g_gain.debut;
        const float l = (float)std::clamp(std::min((age - VOL * 0.6) / 0.15, (FIN - age) / 0.3), 0.0, 1.0);
        if (l > 0) {
            vec2 p[verre::NP_CONTOUR];   // maquette 29 : lueur orange autour de la pilule, liseré orange
            verre::contourArrondi(x, y, w, h, h / 2, p);
            dessin2d::halo(r, p, verre::NP_CONTOUR, 10 * kb, op(orange(0.45f * l)));
            pilule(r, x, y, w, h, op(orange(0.95f * l)), 1.8f * kb);
        }
    }
    score_fin::dessinerEtoile(r, x + pad + re, y + h / 2, re, op(ui::ORANGE));
    ui::text(r, std::round(x + pad + 2 * re + ecart), std::round(y + (h - tp) / 2), tp, t, op(TEXTE), ui::GAUCHE, 0.06f, false, true);   // maquette : italique
    if (g_gain.debut >= 0) {
        const double age = g.time - g_gain.debut;
        // étoiles en vol : de la voiture (centre de l'écran) vers l'étoile de la pilule, en courbe, décalées de 50 ms
        const vec2 de{W * 0.62f, H * 0.52f}, vers{x + pad + re, y + h / 2};
        for (int i = 0; i < NB_ETOILES; i++) {
            const double u0 = (age - 0.05 * i) / VOL;
            if (u0 <= 0 || u0 >= 1) continue;
            const float u = (float)u0, e = u * u * (3 - 2 * u);
            const float cx = de.x + (vers.x - de.x) * e - std::sin(u * 3.14159f) * (40 + 12 * i) * k;   // arc vers la gauche
            const float cy = de.y + (vers.y - de.y) * e;
            const float rr = re * (0.7f + 0.5f * std::sin(u * 3.14159f)) * (1 - 0.08f * (i % 3));
            score_fin::dessinerEtoile(r, cx, cy, rr, op(orange(std::min(1.f, 4 * (1 - u)))));
        }
        // « +N » orange sous la pilule (valeur réelle du gain), qui apparaît puis s'efface à la fin
        const float ap = (float)std::clamp(std::min(age / 0.15, (FIN - age) / 0.35), 0.0, 1.0);
        if (ap > 0) {
            const std::string plus = "+" + score_fin::nombreEtoiles(g_gain.gain, anglais());
            ui::text(r, x + w / 2, y + h + 8 * kb - 6 * kb * (1 - ap), 20 * k, plus, op(orange(ap)), ui::CENTRE, 0.02f, false, true);
        }
    }
}

class CompteurEtoiles final : public System {
public:
    const char* name() const override { return "compteur-etoiles"; }
    void onEcran(Ecran, Ecran, const GameView&) override { t0_ = -1; g_zone = {}; }
    void drawUi(Renderer& r, const GameView& g) override {
        float bx, by, bw, bh;
        const bool bouton = boutonReglages(bx, by, bw, bh);   // lu à chaque image : « Réglages » dessiné à cette image ?
        if (!bouton || g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady() || choixModeOuvert() || credits::ouvert() || creationProfilOuverte() || cadeau_jour::ouvert() || profil_ecran::ouvert() || market_ecran::ouvert() || verre::couvert()) {   // WFS-548 : fenêtre de verre par-dessus   // WFS-482, WFS-513
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
