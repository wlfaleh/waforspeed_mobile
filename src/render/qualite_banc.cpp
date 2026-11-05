// WaForSpeed — banc d'essai de la qualité (WFS-15) : voir qualite_banc.h.
#include "qualite_banc.h"
#include <cstdlib>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/course5.h"          // WFS-134 : poursuite en Basse (aucune voiture en cubes)
#include "../game/course_system.h"
#include "../game/traffic_system.h"   // WFS-134 : voiture-course-trafic (3 voitures à 10, 25 et 50 m)
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
    // WFS-86 : échelle réelle de la scène sur un téléphone lent (résolution dynamique au plancher 0,6) : avant le
    // correctif, échelle du niveau × 0,6 (Haute 60 %, Moyenne 45 %, Basse 30 %) ; après : plancher 60 %, Basse fixe 50 %
    WFS_LOG("qualité (WFS-86) : échelle réelle à 0,6 de résolution dynamique : Haute %.0f %%, Moyenne %.0f %%, Basse %.0f %% (avant : 60 / 45 / 30 %%)",
            qualite::echelleRendu(qualite::HAUTE, 0.6f) * 100, qualite::echelleRendu(qualite::MOYENNE, 0.6f) * 100,
            qualite::echelleRendu(qualite::BASSE, 0.6f) * 100);

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
        WFS_LOG("qualité %s (WFS-134 RG-09) : %s", noms[q], qualite::ligneRendu().c_str());
        if (q >= 1) {   // WFS-104 : la Golf en course, vue de près (comme garage-golf), qualités Moyenne et Haute
            g.camMode = 2; g.orbit = 2.4f;
            for (int i = 0; i < 4; i++) g.render(905 + i / 60.0);
            r.capturePng(out + "/voiture-course-" + noms[q] + ".png");
            WFS_LOG("qualité (WFS-104) : voiture-course-%s : scène à %.0f %%, MSAA ×%d, niveau %s (garage : même échelle, même MSAA)",
                    noms[q], r.vk().renderScale * 100, (int)r.vk().msaa, noms[q]);
            g.camMode = 1;
        }
        WFS_LOG("qualité %s : scène à %.0f %%, ombres %s, bloom %.1f, %u appels de dessin, %u objets masqués (WFS-82), %.1f ms par image (banc, CPU)",
                noms[q], r.vk().renderScale * 100, r.forceNoShadows ? "coupées" : "actives", r.bloomForce, r.appelsDessin(), r.objetsMasques(), ms);
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");

    // WFS-134 (RG-09) : la voiture en course, de jour, vue par le joueur (caméra de poursuite) : Haute de près, Haute avec
    // 3 voitures du trafic à 10, 25 et 50 m devant, Basse (75 %) ; à comparer avec garage-golf
    struct Prise { const char* nom; int niveau; bool trafic; };
    for (const Prise& pr : {Prise{"voiture-course-proche", qualite::HAUTE, false}, Prise{"voiture-course-trafic", qualite::HAUTE, true},
                            Prise{"voiture-course-basse", qualite::BASSE, false}}) {
        R.qualite = pr.niveau; ecrans::appliquerQualiteNiveau(r);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("qualité : chargement impossible"); break; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(910 + i / 60.0); }
        if (pr.trafic) if (Traffic* tr = activeTrafficMut()) {
            const double pos = g.player().position, D[3] = {10, 25, 50};
            for (size_t i = 0; i < tr->cars.size(); i++) {   // 3 voitures placées devant, les autres loin derrière (hors champ)
                TrafficCar& c = tr->cars[i];
                c.crashed = false; c.speed = c.want = g.player().speed;
                if (i < 3) { c.lane = c.target = (int)(i % LANES); c.offset = c.prevOff = LANE_X[(i + 2) % LANES]; c.z = pos + D[i] / M; }
                else c.z = pos - (200 + 5.0 * (double)i) / M;
            }
        }
        // de près (orbite 2,4 m comme voiture-course-haute) ; trafic et Basse : caméra de poursuite haute du joueur
        if (!pr.trafic && pr.niveau == qualite::HAUTE) { g.camMode = 2; g.orbit = 2.4f; } else g.camMode = 1;
        for (int i = 0; i < 4; i++) g.render(914 + i / 60.0);
        r.capturePng(out + "/" + pr.nom + ".png");
        WFS_LOG("capture %s (WFS-134) : %s, %.0f km/h", pr.nom, qualite::ligneRendu().c_str(), g.player().kmh());
        g.shutdown();
    }
    // WFS-134 (demande de l'utilisateur : « jamais de cubes ») : poursuite en qualité Basse, trafic dense placé de 8 à 150 m
    // devant, police et trafic : vrais modèles à toute distance ; appels de dessin au journal
    {
        R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
        courseBenchRules(true); modeCourse() = ModeCourse::Poursuite;
        if (g.init(r, "golf", "ville")) {
            g.setAutopilot(true); g.camMode = 1;
            for (int i = 0; i < 8 * 60; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(920 + i / 60.0); }
            size_t places = 0;
            if (Traffic* tr = activeTrafficMut()) {
                const double pos = g.player().position;
                for (size_t i = 0; i < tr->cars.size(); i++) {
                    TrafficCar& c = tr->cars[i];
                    c.crashed = false; c.speed = c.want = g.player().speed;
                    if (i < 14) { c.offset = c.prevOff = LANE_X[i % LANES]; c.z = pos + (8 + 11.0 * (double)i) / M; places++; }
                    else c.z = pos - (200 + 5.0 * (double)i) / M;
                }
            }
            unsigned appels = 0;
            for (int i = 0; i < 4; i++) { g.render(930 + i / 60.0); appels += r.appelsDessin(); }
            r.capturePng(out + "/voiture-basse-poursuite.png");
            WFS_LOG("capture voiture-basse-poursuite (WFS-134) : %s, %zu voitures du trafic de 8 à 151 m, vrais modèles (aucun cube), "
                    "%u appels de dessin par image", qualite::ligneRendu().c_str(), places, appels / 4);
            g.shutdown();
        }
        courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint;
    }
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
