// WaForSpeed — module « qualité automatique » (WFS-15, épic E4), branché par les systèmes enfichables (WFS-30).
// Mesure les images/s réelles en course, ajuste la résolution de la scène et rétrograde une fois si besoin
// (watchPerf du jeu web, logique dans qualite_auto.h) ; bandeau « Graphismes rapides — pour plus de fluidité ».
// Inactif hors écran (banc d'essai : captures inchangées) et en mode GPU logiciel (WFS-59 gère déjà l'échelle).
#include <algorithm>
#include <cmath>
#include <string>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../platform/chauffe.h"   // WFS-123 : chauffe (RG-02, RG-03)
#include "../ui/ecrans/reglages.h"
#include "../ui/text.h"
#include "qualite_auto.h"
#include "renderer.h"
#include "passes.h"   // WFS-150 (RG-05)
#include "eclairage_physique.h"   // WFS-222 : captures suspendues

namespace wfs {
namespace {

qualite::Auto g_auto;   // gardé d'une partie à l'autre (Game::init est rappelé à chaque carte)
double g_banniere = 0;  // WFS-123 : bandeau partagé avec la chauffe (bandeauGraphismesRapides)
bool g_banniereChauffe = false;   // WFS-134 (RG-02) : bandeau « Qualité allégée (chauffe) » plutôt que « pour plus de fluidité »
// WFS-134 (RG-01) : rendu réellement utilisé (ligne des Réglages, journal) : moteur, images/s lissées sur 1 s
Renderer* g_r = nullptr;
float g_ips = -1;            // −1 : pas encore mesuré (banc d'essai hors écran)
double g_ipsT = 0; int g_ipsN = 0;
std::string g_ligneVue;      // dernière ligne écrite au journal (sans les images/s)

class QualiteSystem : public System {
public:
    const char* name() const override { return "qualité"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r; g_r = &r; dernier_ = -1; effets_ = 0;
        actif_ = !r.vk().offscreen && !r.gpuLogiciel();
        mesure_ = !r.vk().offscreen;   // WFS-134 (RG-01) : images/s mesurées sur le téléphone, même en GPU logiciel
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
        if (!actif_ && !mesure_) return;
        const double t = nowSeconds();
        const double dt = dernier_ < 0 ? 0 : t - dernier_;
        dernier_ = t;
        if (dt <= 0 || dt > 1) return;   // reprise après une pause ou un chargement : pas de mesure
        g_ipsT += dt; g_ipsN++;   // WFS-134 (RG-01) : images/s de la ligne « Rendu », tous écrans, sur 1 s
        if (g_ipsT >= 1) { g_ips = (float)(g_ipsN / g_ipsT); g_ipsT = 0; g_ipsN = 0; }
        if (!actif_) return;
        const ecrans::Reglages& R = ecrans::reglages();
        if (R.qualite != g_auto.niveau) { g_auto.niveau = R.qualite; g_auto.echelle = 1; g_auto.palier = 0; }   // changé dans les réglages (WFS-246 : palier)
        g_auto.choixJoueur = R.qualiteChoisie;
        const Course* c = activeCourse();
        effets_ |= g_auto.imageChauffe(dt, g.ecran == Ecran::Course && !g.sceneDediee && c && c->playing(), chauffeNiveau(), r_->gpuImageMs());   // WFS-246 : marge du GPU
        if (g_auto.mesures != mesuresVues_) {   // WFS-150 (RG-04) : la Haute tient-elle (Ultra proposé) ?
            mesuresVues_ = g_auto.mesures; const bool avant = qualite::tenueUltra.tenue;
            qualite::tenueUltra.mesure(g_auto.fpsMesure, R.qualite, chauffeNiveau());
            if (qualite::tenueUltra.tenue && !avant) WFS_LOG("qualité (WFS-150) : Haute tenue %d × 2 s à %.0f images/s ou plus : Ultra proposé", qualite::MESURES_TENUE, qualite::IPS_TENUE);
        }
    }

    // changements appliqués hors de l'image (les cibles de rendu sont recréées)
    void update(double dt, const GameView&) override {
        if (g_banniere > 0) g_banniere -= dt;
        journalRendu();   // WFS-134 (RG-01) : une ligne à chaque changement du rendu réel
        if (!actif_ || !effets_ || !r_) return;
        if (effets_ & qualite::Auto::RETRO) {
            // WFS-134 (RG-02) : la netteté du niveau quitté est retenue (résolution et MSAA inchangés), seuls les coûts baissent
            qualite::retenuAuto = std::max(qualite::retenuAuto, g_auto.niveau + 1);
            ecrans::reglages().qualite = g_auto.niveau;   // non enregistré : ce n'est pas un choix du joueur
            ecrans::appliquerQualiteNiveau(*r_);
            g_banniere = 2.5; g_banniereChauffe = false;
            WFS_LOG("qualité : rétrogradation automatique au niveau %d (%.0f images/s à l'échelle 0,6)", g_auto.niveau, g_auto.fpsMesure);
        }
        const float avant = r_->vk().renderScale;
        r_->vk().renderScale = qualite::echelleScene(g_auto.niveau, g_auto.echelle);   // WFS-246 : de 100 % (75 % en Basse) à 60 %
        if (r_->vk().renderScale != avant) r_->resize();
        // WFS-246 : effets coupés seulement une fois la scène à 60 % (paliers de qualite_auto.h)
        const qualite::Allegement al = qualite::allegement(g_auto.niveau, g_auto.palier);
        r_->bloomForce = al.bloom ? qualite::bloomNiveau(g_auto.niveau) : 0.f;
        r_->forceNoShadows = !al.ombres;
        r_->forceSansReflets = !al.reflets; eclairage::suspendreCaptures(!al.captures);   // WFS-222 (RG-03)
        WFS_LOG("qualité : %.0f images/s, GPU %.1f ms → facteur dynamique %.2f, palier %d (scène à %.0f %%, MSAA ×%d, bloom %s, ombres %s, reflets %s, captures %s)", g_auto.fpsMesure,
                r_->gpuImageMs(), g_auto.echelle, g_auto.palier, r_->vk().renderScale * 100, (int)r_->vk().msaa, al.bloom ? "oui" : "coupé", al.ombres ? "oui" : "coupées",
                al.reflets ? "oui" : "coupés", al.captures ? "oui" : "suspendues");
        effets_ = 0;
    }

    // toast('Graphismes rapides', 'pour plus de fluidité', 'cp') du jeu web : bandeau en haut, liseré cyan
    void drawUi(Renderer& r, const GameView& g) override {
        if (g_banniere <= 0 || !ui::fontReady() || g.ecran != Ecran::Course) return;
        const float u = g.uiHeight / 100.f, w = 46 * u, h = 10 * u, x = (g.uiWidth - w) / 2, y = g.marges.haut + 18 * u;
        const float a = (float)std::min(1.0, g_banniere / 0.3);
        r.uiRect(x, y, w, h, {20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f * a});
        r.uiRect(x, y, 0.6f * u, h, {ui::CYAN.x, ui::CYAN.y, ui::CYAN.z, a});
        // WFS-134 (RG-02) : « Qualité allégée » : ombres, bloom et décor lointain baissent, la netteté de la voiture reste
        ui::text(r, x + w / 2, y + 1.6f * u, 3.6f * u, "QUALITÉ ALLÉGÉE", {0.95f, 0.96f, 0.97f, a}, ui::CENTRE, 0.08f, false, true);
        ui::text(r, x + w / 2, y + 6.0f * u, 2.6f * u, g_banniereChauffe ? "(chauffe)" : "pour plus de fluidité", {ui::CYAN.x, ui::CYAN.y, ui::CYAN.z, a},
                 ui::CENTRE, 0.f, false);
    }

private:
    // WFS-134 (RG-01) : la ligne « Rendu » (sans les images/s, qui bougent sans cesse) écrite au journal quand elle change ;
    // le journal est celui du rapport partagé de WFS-126
    void journalRendu() {
        if (!r_) return;
        const std::string l = qualite::texteRendu(qualite::niveauAffiche(ecrans::reglages().qualite), r_->vk().renderScale, (int)r_->vk().msaa, -1, chauffeNiveau());
        if (l == g_ligneVue) return;
        g_ligneVue = l;
        WFS_LOG("%s (netteté du niveau %d, %.0f images/s)", qualite::ligneRendu().c_str(), qualite::niveauNettete(ecrans::reglages().qualite), g_ips);
    }
    Renderer* r_ = nullptr;
    bool mesure_ = false;
    bool actif_ = false;
    double dernier_ = -1;
    int effets_ = 0;
    int mesuresVues_ = 0;   // WFS-150
};
WFS_SYSTEM(QualiteSystem, 360)

} // namespace
void bandeauGraphismesRapides() { g_banniere = 2.5; g_banniereChauffe = true; }   // WFS-123 (RG-02) ; WFS-134 : « (chauffe) »
// WFS-134 (RG-01) : rendu réellement utilisé, lu en direct
std::string qualite::ligneRendu() {
    if (!g_r) return "Rendu : --";
    const std::string l = texteRendu(niveauAffiche(ecrans::reglages().qualite), g_r->vk().renderScale, (int)g_r->vk().msaa, g_ips, chauffeNiveau());
    return modeDiagnostic() ? l + " · " + passes::texteCouts(passes::coutsFenetre()) : l;   // WFS-150 (RG-05) : coût de chaque passe
}
} // namespace wfs
