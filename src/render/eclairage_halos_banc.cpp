// WaForSpeed — WFS-337 (épics E4 / E13, ANO-189) : banc des halos des têtes de lampe de nuit (WFS_BANC=halos).
// Ville de nuit, voie rapide (autoroute du tracé 2) : candélabres de 12 m du terre-plein central (ville.cpp, LAMPE_AR).
// Captures : halos-160-0 à -5 (suite d'images à 160 km/h, une image tous les 1/10 s), halos-50m (lampadaire à ≈ 50 m),
// halos-pres (lampadaire de près), halos-rue (rue de la Ville, lampadaire de trottoir) et halos-poursuite (Poursuite police, ANO-189).
// Au journal : pour chaque capture, la distance du lampadaire le plus proche devant la voiture.
#include <cmath>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../game/ressenti.h"
#include "../ui/ecrans/reglages.h"
#include "eclairage.h"
#include "renderer.h"

namespace wfs {

int benchHalos(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    unsetenv("WFS_METEO"); setenv("WFS_METEO", "sec", 1);   // pas de pluie : seul le halo change d'une capture à l'autre
    const VueCamera vue0 = vueCamera();
    const bool reduit = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;
    ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
    Track tr; tr.build(mapById("ville"), trace::versionCarte("ville"));
    int64_t au = -1, auFin = -1;
    for (const trace::Section& s : trace::sections(tr)) if (s.autoroute && s.type != trace::Type::Tunnel && au < 0) { au = s.debut; auFin = s.fin; }
    WFS_LOG("halos : autoroute de la Ville, segments %lld à %lld", (long long)au, (long long)auFin);
    double t = 900;
    int erreurs = 0;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    // lampadaire le plus proche devant la voiture (m), sur la piste qui se répète
    auto prochain = [&]() {
        const int64_t len = (int64_t)tr.segmentCount(), aP = (int64_t)std::floor(g.player().dist / SEG);
        double best = 1e9;
        for (const Lampadaire& l : lampadaires()) {
            const int64_t d = ((l.seg - aP) % len + len) % len;
            best = std::min(best, (double)d * SEG * M - std::fmod(g.player().dist, SEG) * M);
        }
        return best;
    };
    auto capture = [&](const std::string& nom) {
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, %.0f m, lampadaire suivant à %.1f m", nom.c_str(), g.player().kmh(), g.player().dist * M, prochain());
    };
    auto partir = [&](int64_t seg, VueCamera v) {
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, seg)).c_str(), 1);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("halos : chargement de la Ville impossible"); erreurs++; return false; }
        choisirVue(v); g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
        return true;
    };
    // avance (rendu une image sur 8) jusqu'à ce que le lampadaire suivant soit à « cible » m (± 1 m), 40 s au plus
    auto jusquA = [&](double cible) {
        for (int i = 0; i < 60 * 40; i++) { const double d = prochain(); if (d <= cible + 1 && d >= cible - 6) return true; image(i % 8 == 0); }
        return false;
    };
    if (au >= 0 && partir(au + 20, VueCamera::Poursuite)) {
        for (int i = 0; i < 60 * 4; i++) image(i % 8 == 0);   // décompte et départ
        // RG-01 : chaque candélabre double du terre-plein (2 lanternes à 5 m, ville.cpp) a ses 2 têtes inscrites
        int doubles = 0, seules = 0;
        for (const Lampadaire& l : lampadaires()) {
            if (l.style != StyleLampadaire::Ville || l.y < 11.f) continue;
            bool deux = false;
            for (const Lampadaire& m : lampadaires()) deux = deux || (m.style == StyleLampadaire::Tete && m.seg == l.seg && std::fabs(std::fabs(m.lat - l.lat) - 5.f) < 0.01f);
            (deux ? doubles : seules)++;
        }
        if (seules || !doubles) erreurs++;
        WFS_LOG("halos : candélabres doubles de la voie rapide : %d avec leurs 2 têtes, %d avec une seule %s", doubles, seules, seules || !doubles ? "ÉCART" : "OK");
        // suite d'images à 160 km/h (en accélérant, le pilote automatique monte ensuite à ≈ 210 km/h) : une image tous les 1/10 s
        for (int i = 0; i < 60 * 30 && g.player().kmh() < 158; i++) image(i % 8 == 0);
        for (int k = 0; k < 6; k++) { capture("halos-160-" + std::to_string(k)); for (int i = 0; i < 6; i++) image(false); }
        if (jusquA(50)) capture("halos-50m");
        if (jusquA(14)) capture("halos-pres");
    }
    // rue de la Ville : lampadaire de trottoir (ville.cpp, objet LAMPE), au départ
    if (partir(0, VueCamera::Poursuite)) {
        for (int i = 0; i < 60 * 4; i++) image(i % 8 == 0);
        if (jusquA(20)) capture("halos-rue");
    }
    // Poursuite police (scène d'ANO-189), sur la voie rapide
    modeCourse() = ModeCourse::Poursuite; kmCourse5() = 2; courseBenchRules(true);
    if (au >= 0 && partir(au + 20, VueCamera::Poursuite)) {
        for (int i = 0; i < 60 * 8; i++) image(i % 8 == 0);
        if (jusquA(40)) capture("halos-poursuite");
    }
    modeCourse() = ModeCourse::Checkpoint; courseBenchRules(false);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT"); unsetenv("WFS_METEO");
    choisirVue(vue0); reglagesRessenti().reduireMouvements = reduit;
    return erreurs;
}

} // namespace wfs
