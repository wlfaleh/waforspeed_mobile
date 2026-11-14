// WaForSpeed — WFS-537 (épic E22) : bloc « Score » de l'écran de fin des 4 modes ; voir score_fin.h.
// Style design-street-racing, comme les panneaux de fin existants : surface #14161F, liseré orange, surtitre cyan espacé,
// valeurs blanches, total et « +N ★ » en orange (un seul accent), ★ dessinée en polygones (la police Rajdhani n'a pas le glyphe).
#include "score_fin.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "../audio/sfx.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/langue.h"
#include "../ui/text.h"
#include "commandes_tactiles.h"   // dessin2d::polygone (★)
#include "score_fin_calc.h"

namespace wfs::score_fin {

namespace {
Etat g_etat;
bool g_pose = false;                  // panneau posé par un écran de fin à cette image
float g_px = 0, g_py = 0, g_pw = 0, g_ph = 0;
float g_bloc[4] = {0, 0, 0, 0};   // banc : bloc dessiné (x, y, largeur, hauteur)
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 1}, TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1},
           TRAIT{1, 1, 1, 0.08f}, SOMBRE{10 / 255.f, 11 / 255.f, 16 / 255.f, 1};
}

void publier(const score::Score* sc, int etoiles, long long solde) {
    g_etat = Etat{};
    g_etat.pret = true;
    if (sc) { g_etat.detail = true; g_etat.lignes = sc->lignes; g_etat.total = sc->total; g_etat.nouveauRecord = sc->nouveauRecord; }
    g_etat.etoiles = etoiles;
    g_etat.solde = solde;
}
void effacer() { g_etat = Etat{}; g_bloc[2] = 0; }
const Etat& etat() { return g_etat; }
bool rectBloc(float& x, float& y, float& w, float& h) { if (g_bloc[2] <= 0) return false; x = g_bloc[0]; y = g_bloc[1]; w = g_bloc[2]; h = g_bloc[3]; return true; }
void poser(float x, float y, float w, float h) { g_pose = true; g_px = x; g_py = y; g_pw = w; g_ph = h; }

// ★ pleine à 5 branches, centrée en (cx, cy), rayon extérieur re : pentagone central + 5 pointes (polygones convexes)
void dessinerEtoile(Renderer& r, float cx, float cy, float re, const vec4& c) {
    const float ri = re * 0.382f;   // rayon intérieur d'une étoile régulière (sin 18° / sin 54°)
    vec2 ext[5], in[5];
    for (int i = 0; i < 5; i++) {
        const float a = -PI / 2 + i * 2 * PI / 5, b = a + PI / 5;
        ext[i] = {cx + re * std::cos(a), cy + re * std::sin(a)};
        in[i] = {cx + ri * std::cos(b), cy + ri * std::sin(b)};
    }
    const dessin2d::Teinte t = dessin2d::uni(c);
    dessin2d::polygone(r, in, 5, t);
    for (int i = 0; i < 5; i++) { const vec2 p[3] = {in[(i + 4) % 5], ext[i], in[i]}; dessin2d::polygone(r, p, 3, t); }
}

namespace {

class ScoreFin final : public System {
public:
    const char* name() const override { return "score-fin"; }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        if (nouveau != Ecran::Course && nouveau != Ecran::Pause) effacer();   // RG-03 : rien ne reste hors de la course
        t0_ = -1; sonJoue_ = false;
    }
    void drawUi(Renderer& r, const GameView& g) override {
        const bool pose = g_pose;
        g_pose = false;
        if (!pose || g.ecran != Ecran::Course || !g_etat.pret || !ui::fontReady()) { if (!g_etat.pret) { t0_ = -1; sonJoue_ = false; } return; }
        if (!g_etat.detail && g_etat.etoiles < 0) return;   // ni détail ni étoiles : rien à montrer
        if (t0_ < 0 || g.time < t0_) t0_ = g.time;
        dessiner(r, g, g.time - t0_);
    }

private:
    double t0_ = -1;
    bool sonJoue_ = false;

    void dessiner(Renderer& r, const GameView& g, double ecoule) {
        const Etat& e = g_etat;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        // RG-04 : à droite du panneau (à gauche s'il y a plus de place), jamais par-dessus ; trop étroit : tout est réduit (70 % au plus)
        const float gap = 2.5f * u, bord = 1.5f * u;
        const float droite = W - g.marges.droite - bord - (g_px + g_pw + gap), gauche = g_px - gap - g.marges.gauche - bord;
        const bool aDroite = droite + 1 * u >= gauche;   // à droite par défaut (à gauche : classement et minicarte de la course)
        const float place = std::max(droite, gauche), k = std::clamp(place / (44 * u), 0.7f, 1.f), cw = 44 * u * k, s = u * k;
        const float x = aDroite ? g_px + g_pw + gap : g_px - gap - cw;
        const int n = e.detail ? (int)e.lignes.size() : 0;
        const bool etoiles = e.etoiles >= 0;
        const float ligneH = 4.4f * s;
        float ch = 3 * s + 2.8f * s + 2.4f * s;                       // marge haute, surtitre « SCORE »
        if (e.detail) ch += n * ligneH + 1.8f * s + 5.2f * s;        // lignes, trait, total
        if (etoiles) ch += 2.2f * s + 8 * s;                          // « +N ★ »
        if (etoiles && e.solde >= 0) ch += 4 * s;                     // nouveau solde
        if (e.nouveauRecord) ch += 6 * s;                             // « Nouveau record ! »
        ch += 2.6f * s;                                               // marge basse
        const float y = std::max(g.marges.haut + bord, std::min(g_py, H - g.marges.bas - bord - ch));
        r.uiRect(x, y, cw, ch, SURFACE);
        g_bloc[0] = x; g_bloc[1] = y; g_bloc[2] = cw; g_bloc[3] = ch;
        r.uiRect(x, y, cw, 0.6f * u, ui::ORANGE);   // liseré néon des panneaux de fin
        const float xg = x + 3 * s, xd = x + cw - 3 * s, lw = cw - 6 * s;
        float yy = y + 3 * s;
        ui::text(r, xg, yy, 2.8f * s, ui::majuscules(ui::tr("fin.score.titre")), ui::CYAN, ui::GAUCHE, 0.22f, false);
        yy += 2.8f * s + 2.4f * s;
        if (e.detail) {
            const float pts = ui::width(3.2f * s, "+000");
            for (const score::Ligne& l : e.lignes) {   // RG-01 : libellé à gauche, points à droite
                ui::textBoite(r, xg, yy, lw - pts - 1.5f * s, 3.2f * s, 3.2f * s, ui::tr(l.cle), ui::GRIS, ui::GAUCHE, 0.f, false);
                ui::text(r, xd, yy, 3.2f * s, "+" + std::to_string(l.points), TEXTE, ui::DROITE, 0.f, false, true);
                yy += ligneH;
            }
            r.uiRect(xg, yy + 0.2f * s, lw, std::max(1.f, 0.25f * s), TRAIT);
            yy += 1.8f * s;
            ui::textBoite(r, xg, yy, lw * 0.6f, 3.6f * s, 3.6f * s, ui::majuscules(ui::tr("fin.score.total")), TEXTE, ui::GAUCHE, 0.16f, false);
            ui::text(r, xd, yy - 0.3f * s, 4.2f * s, std::to_string(e.total), ui::ORANGE, ui::DROITE, 0.f, false, true);
            yy += 5.2f * s;
        }
        if (etoiles) {   // RG-02 : « +N ★ » compté de 0 à N en 0,8 s, son court existant à la fin du compte
            yy += 2.2f * s;
            const int vu = etoilesAffichees(e.etoiles, ecoule);
            if (vu == e.etoiles && !sonJoue_) { sonJoue_ = true; if (e.etoiles > 0) sfxInterface(SfxId::BipCheckpoint); }
            const bool en = ui::langue::active() == ui::langue::Langue::Anglais;
            const std::string t = "+" + nombreEtoiles(vu, en);
            const float px = 7 * s, re = px * 0.42f, tw = ui::width(px, t), total = tw + 1.2f * s + 2 * re;
            const float x0 = x + (cw - total) / 2;
            ui::text(r, x0, yy, px, t, ui::ORANGE, ui::GAUCHE, 0.f, false, true);
            dessinerEtoile(r, x0 + tw + 1.2f * s + re, yy + px * 0.5f, re, ui::ORANGE);
            yy += 8 * s;
            if (e.solde >= 0) {   // nouveau solde du portefeuille (ce qu'il a vraiment reçu)
                ui::textBoite(r, x + cw / 2, yy, lw, 3 * s, 3 * s, ui::tr(singulier(e.solde, en) ? "fin.score.solde.un" : "fin.score.solde", nombreEtoiles(e.solde, en)), ui::GRIS, ui::CENTRE, 0.f, false);
                yy += 4 * s;
            }
        }
        if (e.nouveauRecord) {   // RG-02 : badge orange, texte sombre, comme « NOUVEAU RECORD » des écrans de fin
            const std::string t = ui::tr("fin.score.record");
            const float px = 3.2f * s, bw = std::min(lw, ui::width(px, t, 0.06f) + 4 * s), bh = 4.8f * s, bx = x + (cw - bw) / 2, by = yy + 0.6f * s;
            r.uiRect(bx, by, bw, bh, ui::ORANGE);
            ui::textBoite(r, bx + bw / 2, by, bw - 2 * s, bh, px, t, SOMBRE, ui::CENTRE, 0.06f, false);
        }
    }
};

} // namespace

WFS_SYSTEM(ScoreFin, 405)   // interface (400-499) : après les écrans de fin des modes (210 à 224), avant le HUD (420)

} // namespace wfs::score_fin
