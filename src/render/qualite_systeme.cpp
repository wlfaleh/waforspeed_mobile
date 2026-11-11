// WaForSpeed — module « qualité automatique » (WFS-15, épic E4), branché par les systèmes enfichables (WFS-30).
// Mesure les images/s réelles en course, ajuste la résolution de la scène et rétrograde une fois si besoin
// (watchPerf du jeu web, logique dans qualite_auto.h) ; bandeau « Graphismes rapides — pour plus de fluidité ».
// Inactif hors écran (banc d'essai : captures inchangées) et en mode GPU logiciel (WFS-59 gère déjà l'échelle).
#include "diagnostic_perf.h"   // WFS-419
#include <algorithm>
#include <cstdio>
#include <cstdlib>
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
qualite::MesureImages g_mesure;   // WFS-246 (RG-06) : 20 dernières secondes de course
qualite::Cadence g_cadence; qualite::MesureImages::Stats g_stats1s; double g_statsT = 0;   // WFS-262 (RG-04) : statistiques recalculées chaque seconde
bool g_enCoursePrec = false; double g_fichierT = 0;   // WFS-246 : fin de course (journal), fichier écrit toutes les 5 s

class QualiteSystem : public System {
public:
    const char* name() const override { return "qualité"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r; g_r = &r; dernier_ = -1; effets_ = 0;
        actif_ = !r.vk().offscreen && !r.gpuLogiciel();
        if (std::getenv("WFS_MESURE_ESSAI") && !g_mesure.n) {   // WFS-246 : banc PC, ligne de la mesure dans les captures des Réglages
            static float v[1200]; for (int i = 0; i < 1200; i++) v[i] = i % 100 == 0 ? 52.f : i % 25 == 0 ? 36.f : 16.7f; qualite::mesureEssai(v, 1200);
        }
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
    void render(Renderer& r, const GameView& g) override {
        pipelinesCourse(r, g.ecran == Ecran::Course && !g.sceneDediee);   // WFS-262 (RG-02) : aussi au banc
        if (!actif_ && !mesure_) return;
        const double t = nowSeconds();
        const double dt = dernier_ < 0 ? 0 : t - dernier_;
        dernier_ = t;
        if (dt <= 0 || dt > 1) return;   // reprise après une pause ou un chargement : pas de mesure
        g_ipsT += dt; g_ipsN++;   // WFS-134 (RG-01) : images/s de la ligne « Rendu », tous écrans, sur 1 s
        if (g_ipsT >= 1) { g_ips = (float)(g_ipsN / g_ipsT); g_ipsT = 0; g_ipsN = 0; }
        if (!actif_) return;
        const ecrans::Reglages& R = ecrans::reglages();
        if (R.qualite != g_auto.niveau) { g_auto.niveau = R.qualite; g_auto.echelle = 1; g_auto.palier = 0; r_->echelleDynamique(1.f); g_cadence = {}; qualite::cadenceCourse = 60; }   // WFS-262 : cadence remise à 60   // changé dans les réglages (WFS-246 : palier, zone dessinée)
        g_auto.choixJoueur = R.qualiteChoisie;
        const Course* c = activeCourse();
        const bool enCourse = g.ecran == Ecran::Course && !g.sceneDediee && c && c->playing();
        effets_ |= g_auto.imageChauffe(dt, enCourse, chauffeNiveau(), r_->gpuImageMs());   // WFS-246 : marge du GPU
        mesurer(dt, enCourse);
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
        // WFS-246 (RG-07) : cibles à la capacité du niveau de netteté (recréées seulement si elle change : jamais pour la
        // résolution dynamique) ; la résolution dynamique ne change que la zone dessinée, sans attente du GPU
        const float cap = qualite::echelleScene(g_auto.niveau, 1.f), voulu = qualite::echelleScene(g_auto.niveau, g_auto.echelle);
        if (r_->vk().renderScale != cap) { r_->vk().renderScale = cap; r_->resize(); }
        r_->echelleDynamique(voulu / cap);
        // WFS-246 : effets coupés seulement une fois la scène à 60 % (paliers de qualite_auto.h)
        const qualite::Allegement al = qualite::allegement(g_auto.niveau, g_auto.palier);
        r_->bloomForce = al.bloom ? qualite::bloomNiveau(g_auto.niveau) : 0.f;
        r_->forceNoShadows = !al.ombres;
        r_->forceSansReflets = !al.reflets; eclairage::suspendreCaptures(!al.captures);   // WFS-222 (RG-03)
        WFS_LOG("qualité : %.0f images/s, GPU %.1f ms → facteur dynamique %.2f, palier %d (scène à %.0f %%, MSAA ×%d, bloom %s, ombres %s, reflets %s, captures %s)", g_auto.fpsMesure,
                r_->gpuImageMs(), g_auto.echelle, g_auto.palier, r_->vk().renderScale * r_->echelleDynamiqueActuelle() * 100, (int)r_->vk().msaa, al.bloom ? "oui" : "coupé", al.ombres ? "oui" : "coupées",
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
    // WFS-262 (RG-02) : pipelines créés pendant la course (décompte compris), au journal en sortant de la course (0 attendu) ;
    // le cache de pipelines est enregistré à ce moment (hors course : aucun à-coup d'écriture)
    void pipelinesCourse(Renderer& r, bool scene) {
        if (r.pipelinesCrees() != pipelinesVus_) {   // hors course (menu, pause) : changement de qualité demandé par le joueur
            pipelinesVus_ = r.pipelinesCrees();
        }
        if (scene && !sceneCourse_) {
            sceneCourse_ = true; pipelinesDebut_ = r.pipelinesCrees();
            WFS_LOG("pipelines (WFS-262) : course affichée, %u pipelines créés depuis le lancement", pipelinesDebut_);
        } else if (!scene && sceneCourse_) {
            sceneCourse_ = false;
            const uint32_t n = r.pipelinesCrees() - pipelinesDebut_;
            if (n) WFS_ERR("pipelines (WFS-262 RG-02) : %u pipeline(s) créé(s) pendant la course (0 attendu)", n);
            else WFS_LOG("pipelines (WFS-262 RG-02) : 0 pipeline créé pendant la course : OK");
            r.enregistrerCachePipelines();
        }
    }
    bool sceneCourse_ = false; uint32_t pipelinesDebut_ = 0, pipelinesVus_ = 0;
    // WFS-246 (RG-06) : durées d'image de la course ; fichier rendu-mesure.txt toutes les 5 s de course, ligne au journal à la fin
    void mesurer(double dt, bool enCourse) {
        if (enCourse && !g_enCoursePrec) { g_cadence = {}; qualite::cadenceCourse = 60; }   // WFS-262 : chaque course part à 60
        if (enCourse) {
            g_mesure.ajouter((float)(dt * 1000));
            if ((g_statsT += dt) >= 1) { g_statsT = 0; g_stats1s = g_mesure.stats(); }
            // WFS-262 (RG-04) : cadence 60 / 30 ; « au plancher » : scène déjà à 60 % et effets allégés
            const bool plancher = g_auto.echelle <= g_auto.echelleMin() + 1e-4f && g_auto.palier >= 1;
            if (qualite::cadenceAuto && g_cadence.image(dt, true, plancher, g_stats1s.p95, r_ ? r_->gpuImageMs() : -1)) {
                qualite::cadenceCourse = g_cadence.ips;
                WFS_LOG("cadence (WFS-262) : %d images/s (95 %% des images à %.1f ms, GPU %.1f ms)", g_cadence.ips, g_stats1s.p95, r_ ? r_->gpuImageMs() : -1.0);
            }
            g_fichierT += dt;
            if (g_fichierT >= 5) { g_fichierT = 0; ecrireFichier(); }
        } else if (g_enCoursePrec && g_mesure.n) {
            WFS_LOG("mesure du rendu (WFS-246, RG-06) : %s", qualite::ligneMesure().c_str());
            ecrireFichier();
        }
        g_enCoursePrec = enCourse;
    }
    static void ecrireFichier() {
        if (qualite::dossierMesure.empty()) return;
        if (FILE* f = std::fopen((qualite::dossierMesure + "/rendu-mesure.txt").c_str(), "w")) { std::fputs(qualite::texteMesure().c_str(), f); std::fclose(f); }
    }
    // WFS-134 (RG-01) : la ligne « Rendu » (sans les images/s, qui bougent sans cesse) écrite au journal quand elle change ;
    // le journal est celui du rapport partagé de WFS-126
    void journalRendu() {
        if (!r_) return;
        const std::string l = qualite::texteRendu(qualite::niveauAffiche(ecrans::reglages().qualite), r_->vk().renderScale * r_->echelleDynamiqueActuelle(), (int)r_->vk().msaa, -1, chauffeNiveau());
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
    const std::string l = texteRendu(niveauAffiche(ecrans::reglages().qualite), g_r->vk().renderScale * g_r->echelleDynamiqueActuelle(), (int)g_r->vk().msaa, g_ips, chauffeNiveau());
    const std::string d = texteDessin(false);   // WFS-317 (I9) : appels et triangles de l'image, part de chaque famille
    std::string ld = d.empty() ? l : l + " · " + d;
    if (g_r->sceneFigee()) ld = l + " · 0 appel · 0 k triangles · fond figé";   // WFS-329 : menu, choix du terrain : scène dessinée une fois, relue ensuite
    // WFS-419 : interrupteurs de diagnostic allumés (rien s'ils sont tous éteints) ; après le menu caché : appareil (RG-03)
    const std::string da = diag::texteActifs();
    if (!da.empty()) ld += " · " + da;
    if (diag::g_menuVu) ld += " · " + diag::texteAppareil(g_r->vk());
    return modeDiagnostic() ? ld + " · " + passes::texteCouts(passes::coutsFenetre()) : ld;   // WFS-150 (RG-05) : coût de chaque passe
}
// WFS-317 (I9) : comptes du Renderer (flushDraw, toutes passes : ombres, reflet, capture, scène), par famille
std::string qualite::texteDessin(bool detail) {
    if (!g_r) return "";
    static_assert(Renderer::FAM_N == 3, "texteComptes : 3 familles (décor, voitures, personnes)");
    const Renderer::ComptesImage& c = g_r->comptesImage();
    return texteComptes(c.appels, c.triangles, detail);
}
// WFS-246 (RG-06) : chiffres de la mesure du téléphone
namespace {
double coutCapture() {   // passe enfichable de capture d'environnement (horodatages des passes, WFS-150)
    for (const passes::Cout& c : passes::coutsFenetre()) if (c.nom.rfind("capture", 0) == 0) return c.ms;
    return 0;
}
std::string virgule(double v) { char b[24]; std::snprintf(b, sizeof b, "%.1f", v); for (char* p = b; *p; p++) if (*p == '.') *p = ','; return b; }
}
std::string qualite::ligneMesure() {
    const MesureImages::Stats st = g_mesure.stats();
    if (!st.images) return "";
    std::string l = "Images (20 s) : méd. " + virgule(st.mediane) + " · 95 % " + virgule(st.p95) + " · 99 % " + virgule(st.p99) + " · écart " + virgule(st.ecartType) + " ms · cadence " + std::to_string(qualite::cadenceCourse) + " · > 33 ms : " +
                    std::to_string(st.plus33) + " · > 50 ms : " + std::to_string(st.plus50);
    if (g_r && g_r->gpuPasses().mesuree) {
        const Renderer::GpuPasses& p = g_r->gpuPasses();
        l += " · GPU ombre " + virgule(p.ombre) + " reflet " + virgule(p.reflet) + " capture " + virgule(coutCapture()) + " scène " + virgule(p.scene) +
             " post " + virgule(p.post) + " ms";
    }
    return l;
}
std::string qualite::texteMesure() {
    const MesureImages::Stats st = g_mesure.stats();
    if (!st.images) return "";
    std::string t = "WaForSpeed : mesure du rendu sur le téléphone (WFS-246, RG-06)\n" + ligneRendu() + "\n";
    t += "fenêtre : " + virgule(st.secondes) + " s de course, " + std::to_string(st.images) + " images\n";
    t += "durée d'image : médiane " + virgule(st.mediane) + " ms, 95 % " + virgule(st.p95) + " ms, 99 % " + virgule(st.p99) + " ms, écart type " + virgule(st.ecartType) + " ms\n";
    t += "cadence de la course (WFS-262) : " + std::to_string(qualite::cadenceCourse) + " images/s\n";
    t += texteDessin(true) + "\n";   // WFS-317 (I9)
    t += "images de plus de 33 ms : " + std::to_string(st.plus33) + " ; de plus de 50 ms : " + std::to_string(st.plus50) + "\n";
    if (g_r && g_r->gpuPasses().mesuree) {
        const Renderer::GpuPasses& p = g_r->gpuPasses();
        t += "temps GPU (ms, lissé) : ombre " + virgule(p.ombre) + ", reflet " + virgule(p.reflet) + ", capture " + virgule(coutCapture()) +
             ", scène " + virgule(p.scene) + ", post (bloom, image finale, interface) " + virgule(p.post) + "\n";
    } else t += "temps GPU : horodatages non disponibles sur ce téléphone\n";
    char e[64]; std::snprintf(e, sizeof e, "échelle dynamique %.2f, palier d'allègement %d\n", g_r ? g_r->echelleDynamiqueActuelle() : 1.f, g_auto.palier);
    return t + e;
}
void qualite::mesureEssai(const float* ms, int n) { g_mesure.vider(); for (int i = 0; i < n; i++) g_mesure.ajouter(ms[i]); }
} // namespace wfs
