// WaForSpeed — bandeaux de notification pendant la course (WFS-47) : voir toasts.h.
#include "toasts.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>   // setenv (banc de nuit)
#include <deque>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "text.h"
#include "langue.h"   // WFS-506 : bandeaux en français ou en anglais

namespace wfs {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, DANGER = 0xFF1F4B, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SOMBRE = 0x10131C;   // titre sombre des bandeaux (maquette 30)

// genre du bandeau : WFS-550 (RG-06, maquette 30) : checkpoint en cyan, points (quasi-contact, combo, feu grillé) en orange,
// police et mauvais coups (accident, combo perdu) en rouge
enum Genre { NORMAL, CP, BAD };
unsigned couleur(Genre g) { return g == CP ? ACCENT2 : g == BAD ? DANGER : ACCENT; }
// titre sombre sur le cyan et l'orange (maquette 30), clair sur le rouge ; la valeur est toujours blanche
unsigned couleurTexte(Genre g) { return g == BAD ? TEXTE : SOMBRE; }

struct Toast { std::string titre, valeur; Genre genre; double naissance = -1; };   // naissance : temps de la 1re image

// durées du jeu web : toast-in 240 ms, toast-out 260 ms à 1,5 s, retiré à 1,9 s (setTimeout)
constexpr double ENTREE = 0.24, SORTIE_DEBUT = 1.5, SORTIE = 0.26, VIE = 1.9;
constexpr size_t PILE_MAX = 3;

std::deque<Toast> g_pile;   // le plus récent devant (prepend)

class Toasts : public System {
public:
    const char* name() const override { return "Bandeaux"; }

    // après les règles de course (ordre 210) : compteurs de cette mise à jour comparés à ceux de la précédente
    // WFS-506 (RG-01) : textes pris dans la langue active au moment de l'événement (une fois par bandeau, pas à chaque image)
    void update(double dt, const GameView&) override {
        const Course* c = activeCourse();
        // RG-07 : pas de course, ou nouvelle course (Rejouer → décompte) : pile vidée, compteurs repris
        if (!c || c->state == RaceState::Count || c->timeLeft > 1e6) { g_pile.clear(); garder(c); return; }
        if (c->level > level_) {   // RG-02 : checkpoint (web : toast(`Checkpoint ${level}`, `+${add.toFixed(1)} s`, 'cp'))
            for (int n = level_ + 1; n <= c->level; n++)
                pousser({ui::tr("toast.checkpoint", std::to_string(n)), "+" + ui::decimal(Course::checkpointBonus(n), 1) + " s", CP});
        }
        // RG-03 : quasi-contact (web : « Quasi-contact ×N » si combo > 1, « +250×combo ») ; un bandeau par quasi-contact,
        // même si plusieurs tombent dans la même mise à jour (le combo de chacun : combo final − ceux qui suivent)
        for (int i = 0, d = c->nearCount - near_; i < d; i++) {
            const int combo = std::max(1, c->combo - (d - 1 - i));
            pousser({combo > 1 ? ui::tr("toast.quasi_combo", std::to_string(combo)) : ui::tr("toast.quasi"), "+" + std::to_string(250 * combo), NORMAL});
        }
        if (c->accidents > accidents_) pousser({ui::tr("toast.accident"), "", BAD});   // RG-04 : playerAccident
        // RG-04 : combo perdu sur un choc (bump) : combo > 1 remis à 0 avant la fin de son délai ; le délai écoulé
        // (comboTimer ≤ dt) remet aussi le combo à 0 mais sans bandeau, comme le jeu web
        else if (combo_ > 1 && c->combo == 0 && timer_ > dt + 1e-9) pousser({ui::tr("toast.combo_perdu"), "", BAD});
        garder(c);
    }

    void drawUi(Renderer& r, const GameView& g) override {
        const Course* c = activeCourse();
        if (g_pile.empty()) return;
        // mêmes conditions que le HUD (WFS-27) : en course seulement
        if (!c || !g.player || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        // WFS-550 (RG-06, maquette 30 ; ordre de l'utilisateur du 2026-11-15 : la maquette à 100 %) : pile centrée sous la rangée
        // du haut ; chaque bandeau est une bande de couleur droite, pleine au centre et fondue vers la gauche et la droite, titre
        // sombre (blanc sur le rouge) et valeur blanche, en capitales italiques de même taille ; ils glissent du haut et s'effacent ;
        // aucun flou (aucun coût de rendu ajouté en course). Mesures de la maquette (1600 × 720) : 42 px de haut, 7 px d'écart.
        const float xc = W * 0.5f + (g.marges.gauche - g.marges.droite) / 2;
        float y = bandeauxHaut(H, g.marges);
        const float tp = 21 * k, h = 42 * k, plein = 40 * k, fondu = 34 * k, gap = 9 * k;
        auto sommet = [](float x, float yy, const vec4& c) { return UiVertex{x, yy, -1, -1, c.x, c.y, c.z, c.w}; };
        for (auto it = g_pile.begin(); it != g_pile.end();) {
            if (it->naissance < 0) it->naissance = g.time;
            const double a = g.time - it->naissance;
            if (a >= VIE) { it = g_pile.erase(it); continue; }
            // entrée : glisse du haut (−24 px → 0), opacité 0 → 1 (sortie douce) ; sortie : s'efface sur place (ease-in)
            float op = 1, dy = 0;
            if (a < ENTREE) { const float e = 1 - std::pow(1 - (float)(a / ENTREE), 3.f); op = e; dy = -24 * k * (1 - e); }
            if (a > SORTIE_DEBUT) { const float s2 = std::min(1.f, (float)((a - SORTIE_DEBUT) / SORTIE)); op = 1 - s2 * s2; }
            const std::string titre = ui::majuscules(it->titre), valeur = it->valeur;   // maquette 30 : « +30 s » (unité en minuscule)
            const float wt = ui::width(tp, titre, 0.06f), wv = valeur.empty() ? 0 : ui::width(tp, valeur, 0.04f), ecart = 10 * k;
            const float wtexte = wt + (wv > 0 ? ecart + wv : 0), w = wtexte + 2 * plein, x = xc - w / 2, yy = y + dy;
            const vec4 c1 = hex(couleur(it->genre), 0.92f * op), c0 = hex(couleur(it->genre), 0);
            // bande : fondu gauche, plein, fondu droit (un quadrilatère à couleurs par sommet chacun)
            const float xa = x - fondu, xb = x + plein * 0.6f, xc2 = x + w - plein * 0.6f, xd = x + w + fondu;
            const UiVertex q1[4] = {sommet(xa, yy, c0), sommet(xb, yy, c1), sommet(xb, yy + h, c1), sommet(xa, yy + h, c0)};
            const UiVertex q2[4] = {sommet(xb, yy, c1), sommet(xc2, yy, c1), sommet(xc2, yy + h, c1), sommet(xb, yy + h, c1)};
            const UiVertex q3[4] = {sommet(xc2, yy, c1), sommet(xd, yy, c0), sommet(xd, yy + h, c0), sommet(xc2, yy + h, c1)};
            r.uiQuad(q1); r.uiQuad(q2); r.uiQuad(q3);
            const float ty = std::round(yy + (h - tp) / 2), tx = std::round(xc - wtexte / 2);
            ui::text(r, tx, ty, tp, titre, hex(couleurTexte(it->genre), op), ui::GAUCHE, 0.06f, false, true);
            if (wv > 0) ui::text(r, std::round(tx + wt + ecart), ty, tp, valeur, hex(TEXTE, op), ui::GAUCHE, 0.04f, false, true);
            y += h + gap;
            ++it;
        }
    }

    void shutdown(Renderer&) override { g_pile.clear(); }

private:
    void pousser(Toast t) {
        g_pile.push_front(std::move(t));
        while (g_pile.size() > PILE_MAX) g_pile.pop_back();
    }
    void garder(const Course* c) {
        level_ = c ? c->level : 0; near_ = c ? c->nearCount : 0; accidents_ = c ? c->accidents : 0;
        combo_ = c ? c->combo : 0; timer_ = c ? c->comboTimer : 0;
    }
    int level_ = 0, near_ = 0, accidents_ = 0, combo_ = 0;
    double timer_ = 0;
};

} // namespace

// WFS-136 (accroche A4) : bandeau demandé par un autre module (feu grillé, nouveaux tracés) ; même pile, même style
void toasts::pousser(const std::string& titre, const std::string& valeur, int genre) {
    g_pile.push_front({titre, valeur, genre == 1 ? CP : genre == 2 ? BAD : NORMAL});
    while (g_pile.size() > PILE_MAX) g_pile.pop_back();
}

WFS_SYSTEM(Toasts, 425)   // interface : après le HUD (420), avant le fondu du chargement (490)

// banc d'essai : checkpoint seul, puis pile de 3 (accident, checkpoint, quasi-contact : le plus récent en haut)
void benchToasts(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("Bandeaux : chargement impossible"); courseBenchRules(false); return; }
    double t = 700;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 5 == 0) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t); g.render(t + 0.01);
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %zu bandeau(x)", nom, g_pile.size());
        for (const Toast& b : g_pile) WFS_LOG("  « %s » « %s »", ui::majuscules(b.titre).c_str(), b.valeur.c_str());
    };
    auto checkpoint = [&]() { if (Course* c = activeCourseMut()) c->nextCp = g.player().dist; };   // franchi à la mise à jour suivante
    g.setAutopilot(true); g.camMode = 0;
    rouler(60 * 3 + 30);   // décompte (3 s) puis 0,5 s de course
    g_pile.clear();        // un quasi-contact du pilote automatique ne doit pas brouiller la capture
    checkpoint();
    rouler(30);
    capture("toast-checkpoint");
    rouler(150);           // 2,5 s : le bandeau du checkpoint est retiré
    g_pile.clear();
    if (Course* c = activeCourseMut()) c->accident();
    rouler(20);
    checkpoint();
    rouler(20);
    if (Course* c = activeCourseMut()) c->nearMiss();
    rouler(20);
    capture("toast-pile");
    g.shutdown();
    // WFS-550 (RG-06, RG-02) : même pile de nuit, avec un combo ×2 (jauge du combo sous le score) : checkpoint, 2 quasi-contacts
    setenv("WFS_MOMENT", "nuit", 1);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true); g.camMode = 0;
        rouler(60 * 3 + 30);
        g_pile.clear();
        checkpoint();
        rouler(20);
        if (Course* c = activeCourseMut()) c->nearMiss();
        rouler(20);
        if (Course* c = activeCourseMut()) c->nearMiss();
        rouler(10);
        capture("toast-pile-nuit");
        g.shutdown();
    } else WFS_ERR("Bandeaux : chargement de nuit impossible");
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);
}

} // namespace wfs
