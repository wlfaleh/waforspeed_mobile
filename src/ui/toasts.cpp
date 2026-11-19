// WaForSpeed — bandeaux de notification pendant la course (WFS-47) : voir toasts.h.
#include "toasts.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>   // setenv (banc de nuit)
#include <deque>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/commandes_tactiles.h"   // WFS-567 : dessin2d (biseau, polygone)
#include "../game/game.h"
#include "../game/poursuite.h"   // WFS-550 : police de la Poursuite (lecture seule)
#include "../game/track.h"       // M (mètres par unité)
#include "../render/renderer.h"
#include "text.h"
#include "langue.h"   // WFS-506 : bandeaux en français ou en anglais

namespace wfs {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }

// WFS-567 (RG-01) : genre du message → repère de couleur (filet) : checkpoint et points (quasi-contact, combo, feu grillé) en
// orange, police et mauvais coups (accident, combo perdu) en rouge, information (nouveaux tracés) en blanc. Plus de cyan.
enum Genre { NORMAL, CP, BAD, INFO };
messages::Repere repere(Genre g) { return g == BAD ? messages::Repere::Danger : g == INFO ? messages::Repere::Info : messages::Repere::Gain; }

// serie : message des quasi-contacts, regroupés (RG-03) ; frais : remis à neuf par un regroupement (vie reprise, sans
// nouvelle entrée)
struct Toast { std::string titre, valeur; Genre genre; double naissance = -1; bool serie = false, frais = false; };

std::deque<Toast> g_pile;   // le plus récent à la fin (en bas)

class Toasts : public System {
public:
    const char* name() const override { return "Bandeaux"; }

    // après les règles de course (ordre 210) : compteurs de cette mise à jour comparés à ceux de la précédente
    // WFS-506 (RG-01) : textes pris dans la langue active au moment de l'événement (une fois par bandeau, pas à chaque image)
    void update(double dt, const GameView& g) override {
        const Course* c = activeCourse();
        // RG-07 : pas de course, ou nouvelle course (Rejouer → décompte) : pile vidée, compteurs repris
        if (!c || c->state == RaceState::Count || c->timeLeft > 1e6) { g_pile.clear(); garder(c); return; }
        if (c->level > level_) {   // RG-02 : checkpoint (web : toast(`Checkpoint ${level}`, `+${add.toFixed(1)} s`, 'cp'))
            for (int n = level_ + 1; n <= c->level; n++)
                pousser({ui::tr("toast.checkpoint", std::to_string(n)), "+" + secondes(Course::checkpointBonus(n)) + " s", CP});   // maquette 30 : « +30 s »
        }
        // RG-03 (WFS-47) : quasi-contact (web : « Quasi-contact ×N » si combo > 1, « +250×combo ») ; WFS-567 (RG-03) : la série se
        // regroupe en un seul message (« QUASI-CONTACT ×3 +750 ») au lieu de s'empiler : le combo de chacun = combo final − ceux qui suivent
        for (int i = 0, d = c->nearCount - near_; i < d; i++) {
            const int combo = std::max(1, c->combo - (d - 1 - i));
            const std::string titre = combo > 1 ? ui::tr("toast.quasi_combo", std::to_string(combo)) : ui::tr("toast.quasi");
            const std::string valeur = "+" + std::to_string(250 * combo);
            auto it = std::find_if(g_pile.begin(), g_pile.end(), [](const Toast& t) { return t.serie; });
            if (it != g_pile.end() && combo > 1) {   // même série : le message existant prend le nouveau combo, en dernier
                Toast t = std::move(*it); g_pile.erase(it);
                t.titre = titre; t.valeur = valeur; t.frais = true;
                g_pile.push_back(std::move(t));
            } else {
                if (it != g_pile.end()) g_pile.erase(it);   // nouvelle série : l'ancienne s'efface
                Toast t{titre, valeur, NORMAL}; t.serie = true;
                pousser(std::move(t));
            }
        }
        if (c->accidents > accidents_) pousser({ui::tr("toast.accident"), "", BAD});   // RG-04 : playerAccident
        // RG-04 : combo perdu sur un choc (bump) : combo > 1 remis à 0 avant la fin de son délai ; le délai écoulé
        // (comboTimer ≤ dt) remet aussi le combo à 0 mais sans bandeau, comme le jeu web
        else if (combo_ > 1 && c->combo == 0 && timer_ > dt + 1e-9) pousser({ui::tr("toast.combo_perdu"), "", BAD});
        policeEnApproche(dt, g);
        garder(c);
    }

    void drawUi(Renderer& r, const GameView& g) override {
        const Course* c = activeCourse();
        // mêmes conditions que le HUD (WFS-27) : en course seulement
        if (!c || !g.player || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        const float H = g.uiHeight;
        // WFS-567 (RG-02, RG-04) : en Poursuite, le message de la poursuite (surprise, arrestation) vient en premier, puis le
        // plus récent de la pile ; ailleurs, les 2 plus récents. Placés en haut, sous le HUD, jamais sur la route.
        const bool poursuite = poursuiteMessage(msgPoursuite_);
        const float encartBas = poursuite ? poursuiteEncartBas(H, g.marges.haut) : 0, encartDemi = poursuite ? poursuiteEncartDemi(H) : 0;
        int rang = 0;
        if (!msgPoursuite_.empty())
            messages::dessiner(r, g, encartBas, encartDemi, rang++, ui::majuscules(msgPoursuite_), "", messages::Repere::Danger, 1.f);
        // vies, retraits, puis les plus récents
        for (auto it = g_pile.begin(); it != g_pile.end();) {
            if (it->naissance < 0) it->naissance = g.time;
            if (it->frais) { it->naissance = g.time - messages::ENTREE; it->frais = false; }   // regroupé : déjà visible, vie reprise
            if (g.time - it->naissance >= messages::VIE) it = g_pile.erase(it); else ++it;
        }
        const int n = std::min((int)g_pile.size(), messages::MAX_VISIBLES - rang);
        for (int i = (int)g_pile.size() - n; i < (int)g_pile.size(); i++) {
            const Toast& t = g_pile[i];
            const double a = g.time - t.naissance;
            // RG-03 : apparition et disparition en fondu (≤ 0,2 s)
            float op = 1;
            if (a < messages::ENTREE) op = (float)(a / messages::ENTREE);
            if (a > messages::VIE - messages::SORTIE) op = std::min(op, (float)((messages::VIE - a) / messages::SORTIE));
            messages::dessiner(r, g, encartBas, encartDemi, rang++, ui::majuscules(t.titre), t.valeur, repere(t.genre), std::clamp(op, 0.f, 1.f));
        }
    }

    void shutdown(Renderer&) override { g_pile.clear(); }

private:
    // « +30 s » quand le bonus est entier, sinon « +12,5 s » (maquette 30)
    static std::string secondes(double v) { return std::fabs(v - std::round(v)) < 1e-6 ? std::to_string((long long)std::llround(v)) : ui::decimal(v, 1); }
    // WFS-550 (maquette 30) : « POLICE EN APPROCHE » (rouge) dans la Poursuite, quand une voiture de police qui roule passe à moins
    // de 60 m du joueur ; au plus un bandeau toutes les 8 s (lecture seule de la poursuite, aucune règle de jeu changée)
    void policeEnApproche(double dt, const GameView& g) {
        attentePolice_ = std::max(0.0, attentePolice_ - dt);
        double rd[8]; bool fixe[8];
        const int n = g.player ? poursuitePolice(rd, fixe, 8) : 0;
        double d = 1e9;
        const double tour = g.track ? g.track->length() : 0;   // revue du lead dev (PR #424) : écart pris sur le tour du circuit
        for (int i = 0; i < n; i++) if (!fixe[i]) {
            double e = std::fabs(rd[i] - g.player->dist);
            if (tour > 0) { e = std::fmod(e, tour); e = std::min(e, tour - e); }
            d = std::min(d, e * M);
        }
        if (d < 60 && dPolice_ >= 60 && attentePolice_ <= 0) { pousser({ui::tr("toast.police_approche"), "", BAD}); attentePolice_ = 8; }
        dPolice_ = d;
    }
    double dPolice_ = 1e9, attentePolice_ = 0;
    void pousser(Toast t) {
        g_pile.push_back(std::move(t));   // le plus récent à la fin ; WFS-567 (RG-02) : le plus ancien laisse sa place
        while (g_pile.size() > (size_t)messages::MAX_VISIBLES) g_pile.pop_front();
    }
    std::string msgPoursuite_;   // tampon réutilisé (aucune allocation par image une fois rempli)
    void garder(const Course* c) {
        level_ = c ? c->level : 0; near_ = c ? c->nearCount : 0; accidents_ = c ? c->accidents : 0;
        combo_ = c ? c->combo : 0; timer_ = c ? c->comboTimer : 0;
    }
    int level_ = 0, near_ = 0, accidents_ = 0, combo_ = 0;
    double timer_ = 0;
};

} // namespace

// WFS-567 (RG-01) : un message : fond sombre translucide au bord gauche incliné, filet de couleur sur ce bord, titre et valeur
// clairs en capitales italiques sur une ligne. Trop large pour sa place : police réduite jusqu'à 3,5 % de H, puis valeur retirée.
void messages::fond(Renderer& r, const Rect& q, float H, Repere rep, float op) {
    const float bi = biais(H), f = 4 * (H / 720.f);   // filet de couleur sur le bord gauche incliné
    dessin2d::biseau(r, q.x, q.y, q.w, q.h, bi, dessin2d::uni(hex(SOMBRE, FOND_OPACITE * op)));
    const vec2 filet[4] = {{q.x + bi, q.y}, {q.x + bi + f, q.y}, {q.x + f, q.y + q.h}, {q.x, q.y + q.h}};
    dessin2d::polygone(r, filet, 4, dessin2d::uni(hex(couleurRepere(rep), op)));
}

void messages::dessiner(Renderer& r, const GameView& g, float encartBas, float encartDemi, int rang, const std::string& titre,
                        const std::string& valeur, Repere rep, float op) {
    const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f, mg = marge(H), bi = biais(H), ec = 10 * k;
    const float wmax = largeurMax(W, H, g.marges, encartDemi);
    float tp = taillePolice(H);
    auto largeur = [&](float px, bool avecValeur) {
        const float wt = ui::width(px, titre, 0.06f), wv = avecValeur && !valeur.empty() ? ui::width(px, valeur, 0.04f) : 0;
        return wt + (wv > 0 ? ec + wv : 0) + 2 * mg + bi;
    };
    bool avecValeur = true;
    float w = largeur(tp, true);
    if (w > wmax) { tp = std::max(H * 0.035f, tp * (wmax - 2 * mg - bi) / (w - 2 * mg - bi)); w = largeur(tp, true); }
    if (w > wmax && !valeur.empty()) { avecValeur = false; w = largeur(tp, false); }
    const Rect q = place(W, H, g.marges, encartBas, encartDemi, rang, std::min(w, std::max(wmax, 0.f)));
    fond(r, q, H, rep, op);
    const float ty = std::round(q.y + (q.h - tp) / 2), tx = std::round(q.x + bi + mg);
    ui::text(r, tx, ty, tp, titre, hex(TEXTE, op), ui::GAUCHE, 0.06f, false, true);
    if (avecValeur && !valeur.empty())
        ui::text(r, std::round(tx + ui::width(tp, titre, 0.06f) + ec), ty, tp, valeur, hex(TEXTE, op), ui::GAUCHE, 0.04f, false, true);
}

// WFS-136 (accroche A4) : bandeau demandé par un autre module (feu grillé, nouveaux tracés) ; même pile, même style
void toasts::pousser(const std::string& titre, const std::string& valeur, int genre) {
    g_pile.push_back({titre, valeur, genre == 1 ? CP : genre == 2 ? BAD : genre == 3 ? INFO : NORMAL});
    while (g_pile.size() > (size_t)messages::MAX_VISIBLES) g_pile.pop_front();
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
    // WFS-550 (maquette 30) : checkpoint, quasi-contact, combo ×2, police en approche (bandeau rouge poussé comme par la Poursuite)
    if (Course* c = activeCourseMut()) c->nearMiss();   // 1er quasi-contact (combo ×1), retiré de la pile : seul le combo ×2 est montré
    rouler(5);
    g_pile.clear();
    checkpoint();
    rouler(10);
    if (Course* c = activeCourseMut()) c->nearMiss();   // 2e : « QUASI-CONTACT +250 » et « COMBO ×2 +250 »
    rouler(5);
    toasts::pousser(ui::tr("toast.police_approche"), "", 2);
    rouler(15);
    capture("toast-maquette");
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
