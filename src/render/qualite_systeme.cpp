// WaForSpeed — module « qualité automatique » (WFS-15, épic E4), branché par les systèmes enfichables (WFS-30).
// Mesure les images/s réelles en course, ajuste la résolution de la scène et rétrograde une fois si besoin
// (watchPerf du jeu web, logique dans qualite_auto.h) ; bandeau « Graphismes rapides — pour plus de fluidité ».
// Inactif hors écran (banc d'essai : captures inchangées) et en mode GPU logiciel (WFS-59 gère déjà l'échelle).
#include <cmath>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/text.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

qualite::Auto g_auto;   // gardé d'une partie à l'autre (Game::init est rappelé à chaque carte)

class QualiteSystem : public System {
public:
    const char* name() const override { return "qualité"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r; dernier_ = -1; effets_ = 0;
        actif_ = !r.vk().offscreen && !r.gpuLogiciel();
        static bool annonce = false;
        if (actif_ && !annonce) {
            annonce = true;
            const ecrans::Reglages& R = ecrans::reglages();
            WFS_LOG("qualité : niveau %d %s", R.qualite, R.qualiteChoisie ? "choisi par le joueur" :
                    "choisi au premier lancement (Haute ; Basse sur GPU logiciel)");
        }
    }

    // une mesure par image réellement affichée ; l'état « en course » = partie lancée (décompte fini), comme 'play'
    void render(Renderer&, const GameView& g) override {
        if (!actif_) return;
        const double t = nowSeconds();
        const double dt = dernier_ < 0 ? 0 : t - dernier_;
        dernier_ = t;
        if (dt <= 0 || dt > 1) return;   // reprise après une pause ou un chargement : pas de mesure
        const ecrans::Reglages& R = ecrans::reglages();
        if (R.qualite != g_auto.niveau) { g_auto.niveau = R.qualite; g_auto.echelle = 1; }   // changé dans les réglages
        g_auto.choixJoueur = R.qualiteChoisie;
        const Course* c = activeCourse();
        effets_ |= g_auto.image(dt, g.ecran == Ecran::Course && !g.sceneDediee && c && c->playing());
    }

    // changements appliqués hors de l'image (les cibles de rendu sont recréées)
    void update(double dt, const GameView&) override {
        if (banniere_ > 0) banniere_ -= dt;
        if (!actif_ || !effets_ || !r_) return;
        if (effets_ & qualite::Auto::RETRO) {
            ecrans::reglages().qualite = g_auto.niveau;   // non enregistré : ce n'est pas un choix du joueur
            ecrans::appliquerQualiteNiveau(*r_);
            banniere_ = 2.5;
            WFS_LOG("qualité : rétrogradation automatique au niveau %d (%.0f images/s à l'échelle 0,6)", g_auto.niveau, g_auto.fpsMesure);
        }
        const float avant = r_->vk().renderScale;
        r_->vk().renderScale = qualite::echelleRendu(g_auto.niveau, g_auto.echelle);   // WFS-104 : jamais sous la netteté du garage
        if (r_->vk().renderScale != avant) r_->resize();
        // WFS-104 : téléphone lent → bloom puis ombres coupés, la résolution de la voiture ne bouge pas
        const qualite::Allegement al = qualite::allegement(g_auto.niveau, g_auto.echelle);
        r_->bloomForce = al.bloom ? qualite::bloomNiveau(g_auto.niveau) : 0.f;
        r_->forceNoShadows = !al.ombres;
        WFS_LOG("qualité : %.0f images/s → facteur dynamique %.2f (scène à %.0f %%, MSAA ×%d, bloom %s, ombres %s)", g_auto.fpsMesure,
                g_auto.echelle, r_->vk().renderScale * 100, (int)r_->vk().msaa, al.bloom ? "oui" : "coupé", al.ombres ? "oui" : "coupées");
        effets_ = 0;
    }

    // toast('Graphismes rapides', 'pour plus de fluidité', 'cp') du jeu web : bandeau en haut, liseré cyan
    void drawUi(Renderer& r, const GameView& g) override {
        if (banniere_ <= 0 || !ui::fontReady() || g.ecran != Ecran::Course) return;
        const float u = g.uiHeight / 100.f, w = 46 * u, h = 10 * u, x = (g.uiWidth - w) / 2, y = g.marges.haut + 18 * u;
        const float a = (float)std::min(1.0, banniere_ / 0.3);
        r.uiRect(x, y, w, h, {20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f * a});
        r.uiRect(x, y, 0.6f * u, h, {ui::CYAN.x, ui::CYAN.y, ui::CYAN.z, a});
        ui::text(r, x + w / 2, y + 1.6f * u, 3.6f * u, "GRAPHISMES RAPIDES", {0.95f, 0.96f, 0.97f, a}, ui::CENTRE, 0.08f, false, true);
        ui::text(r, x + w / 2, y + 6.0f * u, 2.6f * u, "pour plus de fluidité", {ui::CYAN.x, ui::CYAN.y, ui::CYAN.z, a}, ui::CENTRE, 0.f, false);
    }

private:
    Renderer* r_ = nullptr;
    bool actif_ = false;
    double dernier_ = -1, banniere_ = 0;
    int effets_ = 0;
};
WFS_SYSTEM(QualiteSystem, 360)

} // namespace
} // namespace wfs
