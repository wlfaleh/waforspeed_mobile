// WaForSpeed — bandeaux de notification pendant la course (WFS-47) : voir toasts.h.
#include "toasts.h"
#include "echelle.h"   // WFS-102 : échelle en dp
#include <algorithm>
#include <cmath>
#include <deque>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../game/track.h"
#include "../world/zones.h"   // WFS-95 (RG-04) : nom du quartier (lecture seule)
#include <cstdlib>
#include "text.h"

namespace wfs {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, DANGER = 0xFF1F4B, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};   // --glass rgba(20,22,31,.72)

// genre du bandeau (.toast, .toast.cp, .toast.bad) : couleur de la bordure gauche et de la valeur
enum Genre { NORMAL, CP, BAD };
unsigned couleur(Genre g) { return g == CP ? ACCENT : g == BAD ? DANGER : ACCENT2; }

struct Toast { std::string titre, valeur; Genre genre; double naissance = -1; };   // naissance : temps de la 1re image

// durées du jeu web : toast-in 240 ms, toast-out 260 ms à 1,5 s, retiré à 1,9 s (setTimeout)
constexpr double ENTREE = 0.24, SORTIE_DEBUT = 1.5, SORTIE = 0.26, VIE = 1.9;
constexpr size_t PILE_MAX = 3;

std::deque<Toast> g_pile;   // le plus récent devant (prepend)

class Toasts : public System {
public:
    const char* name() const override { return "Bandeaux"; }

    // après les règles de course (ordre 210) : compteurs de cette mise à jour comparés à ceux de la précédente
    void update(double dt, const GameView& g) override {
        const Course* c = activeCourse();
        // RG-07 : pas de course, ou nouvelle course (Rejouer → décompte) : pile vidée, compteurs repris
        if (!c || c->state == RaceState::Count || c->timeLeft > 1e6) { g_pile.clear(); garder(c); zone_ = -1; return; }
        // WFS-95 (RG-04) : nouveau quartier (update.js l. 240, zoneSeen) : un bandeau par changement de zone, pas au départ
        if (g.map && g.track && g.player && g.ecran == Ecran::Course && c->state != RaceState::Over) {
            const int64_t L = (int64_t)g.track->segmentCount();
            const int64_t a = L > 0 ? (((int64_t)std::floor(g.player->dist / SEG)) % L + L) % L : 0;
            const int z = L > 0 ? zoneIndex(*g.map, a, L) : -1;
            if (zone_ >= 0 && z >= 0 && z != zone_) {
                const char* nom = zoneName(*g.map, a, L);
                pousser({nom ? nom : "", "", CP});
                WFS_LOG("bandeau : quartier « %s »", nom ? nom : "");
            }
            zone_ = z;
        }
        if (c->level > level_) {   // RG-02 : checkpoint (web : toast(`Checkpoint ${level}`, `+${add.toFixed(1)} s`, 'cp'))
            for (int n = level_ + 1; n <= c->level; n++)
                pousser({"Checkpoint " + std::to_string(n), "+" + ui::decimal(Course::checkpointBonus(n), 1) + " s", CP});
        }
        // RG-03 : quasi-contact (web : « Quasi-contact ×N » si combo > 1, « +250×combo ») ; un bandeau par quasi-contact,
        // même si plusieurs tombent dans la même mise à jour (le combo de chacun : combo final − ceux qui suivent)
        for (int i = 0, d = c->nearCount - near_; i < d; i++) {
            const int combo = std::max(1, c->combo - (d - 1 - i));
            pousser({combo > 1 ? "Quasi-contact ×" + std::to_string(combo) : "Quasi-contact", "+" + std::to_string(250 * combo), NORMAL});
        }
        if (c->accidents > accidents_) pousser({"Accident !", "", BAD});   // RG-04 : playerAccident
        // RG-04 : combo perdu sur un choc (bump) : combo > 1 remis à 0 avant la fin de son délai ; le délai écoulé
        // (comboTimer ≤ dt) remet aussi le combo à 0 mais sans bandeau, comme le jeu web
        else if (combo_ > 1 && c->combo == 0 && timer_ > dt + 1e-9) pousser({"Combo perdu", "", BAD});
        garder(c);
    }

    void drawUi(Renderer& r, const GameView& g) override {
        const Course* c = activeCourse();
        if (g_pile.empty()) return;
        // mêmes conditions que le HUD (WFS-27) : en course seulement
        if (!c || !g.player || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = ui::echelle(W, H), u = H / 100.f;
        // RG-05 : pile à droite, sous le score (top = gouttière + 110 px) ; bord droit à gauche du bouton Nitro
        // (cercle de 6,5 % de H en (0,90 W − marge droite ; 0,32 H), Game::drawControls) pour ne jamais le recouvrir
        const float xr = W * 0.90f - g.marges.droite - 6.5f * u - 12 * k;
        // WFS-102 (RG-02) : petit écran : pile juste sous le panneau du score (3 bandeaux au-dessus du bouton Nitro)
        float y = 14 * k + g.marges.haut + (ui::compact(W, H) ? 76 : 110) * k;
        const float tp = 18 * k, vp = 14 * k, padL = 14 * k, padR = 18 * k, padV = 8 * k, gap = 12 * k, h = padV * 2 + tp * 1.15f;
        for (auto it = g_pile.begin(); it != g_pile.end();) {
            if (it->naissance < 0) it->naissance = g.time;
            const double a = g.time - it->naissance;
            if (a >= VIE) { it = g_pile.erase(it); continue; }
            // entrée : de +60 px à 0, opacité 0 → 1 (var(--ease) ≈ sortie douce) ; sortie : vers +40 px, opacité → 0 (ease-in)
            float op = 1, dx = 0;
            if (a < ENTREE) { const float e = 1 - std::pow(1 - (float)(a / ENTREE), 3.f); op = e; dx = 60 * k * (1 - e); }
            if (a > SORTIE_DEBUT) {
                const float s = std::min(1.f, (float)((a - SORTIE_DEBUT) / SORTIE)), e = s * s;
                op = 1 - e; dx = 40 * k * e;
            }
            const std::string titre = ui::majuscules(it->titre);
            const float wt = ui::width(tp, titre, 0.06f), wv = it->valeur.empty() ? 0 : ui::width(vp, it->valeur);
            const float w = padL + wt + (wv > 0 ? gap + wv : 0) + padR, x = xr - w + dx;
            const vec4 bord = hex(couleur(it->genre), op);
            r.uiRect(x, y, w, h, {GLASS.x, GLASS.y, GLASS.z, GLASS.w * op});
            r.uiRect(x, y, 4 * k, h, bord);   // border-left 4 px
            // titre 18 px en capitales (letter-spacing .06em) ; valeur 14 px alignée sur la ligne de base du titre
            // WFS-48 : titre (.toast b) en italique
            ui::text(r, std::round(x + padL), std::round(y + padV), tp, titre, hex(TEXTE, op), ui::GAUCHE, 0.06f, false, true);
            if (wv > 0) ui::text(r, std::round(x + padL + wt + gap), std::round(y + padV + (tp - vp) * 0.8f), vp, it->valeur, bord, ui::GAUCHE, 0, false);
            y += h + 8 * k;   // gap 8 px
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
    int level_ = 0, near_ = 0, accidents_ = 0, combo_ = 0, zone_ = -1;   // zone_ : quartier vu (WFS-95)
    double timer_ = 0;
};

} // namespace

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
    // WFS-95 (RG-04) / WFS-102 (RG-19) : nouveau quartier ; départ (WFS_DEPART) 30 tronçons avant la fin du 1er quartier
    {
        Track t; t.build(mapById("ville"));
        const int64_t L = (int64_t)t.segmentCount();
        int64_t fin = 1;
        while (fin < L && zoneIndex(*t.map(), fin, L) == zoneIndex(*t.map(), 0, L)) fin++;
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, fin - 30)).c_str(), 1);
    }
    const bool ok = g.init(r, "golf", "ville");
    unsetenv("WFS_DEPART");
    if (!ok) { courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    rouler(60 * 3);   // décompte : aucun bandeau de quartier (premier quartier)
    bool vu = false;
    for (int i = 0; i < 60 * 12 && !vu; i++) {
        rouler(1);
        for (const Toast& b : g_pile) if (b.genre == CP && b.titre.rfind("Checkpoint", 0) != 0) vu = true;
    }
    rouler(20);
    capture("toast-quartier");
    g.shutdown();
    courseBenchRules(false);
}

} // namespace wfs
