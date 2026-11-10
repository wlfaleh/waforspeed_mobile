// WaForSpeed — banc d'essai de WFS-404 (voitures retirées de l'application, licence non commerciale) : captures demandées par le
// lead dev (revue de la PR #337) qu'aucun banc existant ne donne :
//   face-<id>                 chaque voiture permise vue de face en vitrine (calandre, logos : RG-06)
//   ville-sens-inverse-<n>    autoroute de la Ville, voies opposées cadrées au téléobjectif, de jour et de nuit (RG-03 : Compass et 208)
// Le tour complet du garage (garage-tour-NN) est dans src/ui/garage.cpp (benchGarageTour), le camion de la Poursuite dans
// benchPoursuite (WFS_MOMENT=jour). Lancement : WFS_BANC=voitures-retirees (desktop_main.cpp).
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "cars.h"
#include "course_system.h"
#include "game.h"
#include "trace_cartes.h"
#include "track.h"

namespace wfs {

namespace {
// caméra fixe du banc (crochet ambiance, comme la vue haute de cartes_banc.cpp) : inactive hors de ce banc
struct VueFixe { bool actif = false; vec3 oeil{}, cible{}; float fov = 50; } g_vue;
class VueFixeBanc : public System {
public:
    const char* name() const override { return "vue-fixe-wfs404"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_vue.actif) return;
        fp.view = lookAt(g_vue.oeil, g_vue.cible, {0, 1, 0});
        fp.proj = perspectiveVk(g_vue.fov * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
        fp.camPos = g_vue.oeil;
    }
};
} // namespace
WFS_SYSTEM(VueFixeBanc, 996)

void benchVoituresFace(Game& g, Renderer& r, const std::string& out) {
    for (int p = 0; p < voituresPermises().n; p++) {
        const CarDef& c = CARS[voituresPermises().idx[p]];
        if (!g.init(r, c.id, "ville")) { WFS_ERR("face : %s impossible à charger", c.id); continue; }
        g.setAutopilot(false); g.camMode = 2; g.orbit = PI; g.orbitHauteur = 1.1f; g.orbitRecul = 5.5f;   // devant la voiture (avant vers −Z)
        for (int k = 0; k < 3; k++) { g.update(1.0 / 60); g.render(2100 + k / 60.0); }
        g.render(2101); g.render(2101.01);
        r.capturePng(out + "/face-" + c.id + ".png");
        WFS_LOG("capture face-%s : %s %s vue de face", c.id, marque(c), modele(c));
        g.orbitHauteur = 1.7f; g.orbitRecul = 6.5f;
        g.shutdown();
    }
}

void benchSensInverseVille(Game& g, Renderer& r, const std::string& out) {
    if (!trace::carteActive("ville")) return;
    Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    int64_t debut = -1;
    for (const trace::Section& s : trace::sections(t)) if (s.type == trace::Type::Autoroute) { debut = s.debut + (int64_t)(80 / SEG_M); break; }
    if (debut < 0) { WFS_ERR("sens inverse : pas d'autoroute en Ville"); return; }
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        setenv("WFS_DEPART", std::to_string(debut).c_str(), 1);
        courseBenchRules(true);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("sens inverse : chargement impossible"); continue; }
        g.setAutopilot(false); g.camMode = 0; g.allerA(Ecran::Course);
        for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(600 + i / 60.0); }
        // œil au-dessus du terre-plein central (3 m), téléobjectif (28°), visée 70 m plus loin sur la voie opposée du milieu (sensInverse, ville.cpp)
        const double s = g.player().dist;
        const TrackPoint o = t.point(s, -(RW + 4.0)), v = t.point(s + 70 / M, -(RW + 11.5));
        g_vue = {true, {(float)o.x, (float)o.y + 3.0f, (float)o.z}, {(float)v.x, (float)v.y + 0.8f, (float)v.z}, 28};
        for (int n = 0; n < 12; n++) {   // 12 instants espacés de 0,75 s : les voitures passent à horaire fixe (30 m/s environ)
            for (int i = 0; i < 45; i++) { g.update(1.0 / 60); if (i % 45 == 0) g.render(610 + n * 2 + i / 60.0); }
            g.render(620 + n); g.render(620.02 + n);
            const std::string nom = std::string("ville-sens-inverse-") + moment + "-" + (n < 9 ? "0" : "") + std::to_string(n + 1);
            r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s", nom.c_str());
        }
        g_vue.actif = false;
        g.shutdown();
    }
    courseBenchRules(false);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
