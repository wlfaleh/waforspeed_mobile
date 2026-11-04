// WaForSpeed — banc d'essai de la qualité (WFS-15) : voir qualite_banc.h.
#include "qualite_banc.h"
#include <cstdlib>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {

void benchQualite(Game& g, Renderer& r, const std::string& out) {
    // images/s simulées (watchPerf) : 25 images/s pendant 10 s, puis 60 images/s
    qualite::Auto a;
    for (int s = 0; s < 6; s++) {
        int e = 0; for (int i = 0; i < 50; i++) e |= a.image(1.0 / 25, true);
        WFS_LOG("qualité simulée : 25 images/s, mesure %d → échelle %.2f, niveau %d%s", s + 1, a.echelle, a.niveau,
                (e & qualite::Auto::RETRO) ? " — rétrogradation : « Graphismes rapides pour plus de fluidité »" : "");
    }
    for (int s = 0; s < 3; s++) { for (int i = 0; i < 120; i++) a.image(1.0 / 60, true); }
    WFS_LOG("qualité simulée : 60 images/s pendant 6 s → échelle %.2f, niveau %d (pas de remontée de niveau)", a.echelle, a.niveau);

    // les trois niveaux, de nuit (bloom des feux, néons et phares), même scène
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    const char* noms[3] = {"basse", "moyenne", "haute"};
    setenv("WFS_MOMENT", "nuit", 1);
    for (int q = 0; q < 3; q++) {
        R.qualite = q; ecrans::appliquerQualiteNiveau(r);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("qualité : chargement impossible"); break; }
        g.setAutopilot(true); g.camMode = 1;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(900 + i / 60.0); }
        const double t0 = nowSeconds();
        for (int i = 0; i < 20; i++) { g.update(1.0 / 60); g.render(904 + i / 60.0); }
        const double ms = (nowSeconds() - t0) / 20 * 1000;
        r.capturePng(out + "/qualite-" + noms[q] + ".png");
        WFS_LOG("qualité %s : scène à %.0f %%, ombres %s, bloom %.1f, %u appels de dessin, %u objets masqués (WFS-82), %.1f ms par image (banc, CPU)",
                noms[q], r.vk().renderScale * 100, r.forceNoShadows ? "coupées" : "actives", r.bloomForce, r.appelsDessin(), r.objetsMasques(), ms);
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
