// WaForSpeed — banc « messages » (WFS-567, épic E8, ANO-315 et ANO-290) : les messages affichés pendant la course, dans des
// scènes fixes, pour les captures avant / après de la PR (WFS_BANC=messages, en 1280x720 et en 2400x1080).
//   - Checkpoint (Ville, jour) : conseil de départ pendant le décompte ; passage d'un checkpoint ; série de quasi-contacts (combo) puis accident ; même série de nuit ;
//   - Poursuite (Ville, jour) : conseil de départ pendant le décompte ; annonce de renfort et perte de points ; bandeau de
//     surprise (barrage) avec « Police en approche » ; même scène de nuit.
// Le banc n'utilise que des fonctions qui existent déjà sur main (captures « avant » faites avec ce même fichier).
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/course.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/poursuite.h"
#include "../game/track.h"   // M (mètres par unité)
#include "../render/renderer.h"
#include "toasts.h"
#include "langue.h"

namespace wfs {

int benchMessages(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    double t = 5000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t += 0.01); g.render(t += 0.01);
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, %.2f km", nom, g.player().kmh(), g.player().dist * M / 1000);
    };
    auto demarrer = [&](ModeCourse m, const char* moment) {
        modeCourse() = m;
        setenv("WFS_MOMENT", moment, 1);
        if (!g.init(r, "golf", "ville")) { ecarts++; WFS_ERR("messages : chargement impossible (%s)", moment); return false; }
        g.setAutopilot(true); g.camMode = 0;
        g.allerA(Ecran::Course);
        return true;
    };
    auto checkpoint = [&]() { if (Course* c = activeCourseMut()) c->nextCp = g.player().dist; };   // franchi à la mise à jour suivante
    auto quasi = [&]() { if (Course* c = activeCourseMut()) c->nearMiss(); };
    courseBenchRules(true);

    // ---------- Checkpoint, jour puis nuit ----------
    for (const char* moment : {"jour", "nuit"}) {
        if (!demarrer(ModeCourse::Checkpoint, moment)) continue;
        const bool jour = moment[0] == 'j';
        images(90);                // 1,5 s du décompte : conseil de départ affiché
        if (jour) capture("messages-checkpoint-depart");
        images(90 + 3 * 60);       // fin du décompte puis 3 s de course
        if (jour) {
            images(150);           // messages du départ effacés
            checkpoint(); images(30);
            capture("messages-checkpoint-jour");
            images(150);
        }
        // série : checkpoint, 3 quasi-contacts (combo ×2, ×3), puis accident
        checkpoint(); images(10);
        quasi(); images(8); quasi(); images(8); quasi(); images(8);
        if (Course* c = activeCourseMut()) c->accident();
        images(30);
        capture(jour ? "messages-serie-jour" : "messages-serie-nuit");
        g.shutdown();
    }

    // ---------- Poursuite : départ, renfort, surprise ; jour puis nuit ----------
    poursuiteBancFixe(true);
    for (const char* moment : {"jour", "nuit"}) {
        if (!demarrer(ModeCourse::Poursuite, moment)) continue;
        const bool jour = moment[0] == 'j';
        images(90);   // 1,5 s du décompte : conseil de départ affiché
        if (jour) capture("messages-poursuite-depart");
        images(90 + 60);
        if (jour) {
            poursuiteBancMessages();   // « RENFORT DE POLICE » et perte de points (choc avec le trafic)
            images(20);
            capture("messages-poursuite-renfort");
        }
        // surprise du banc (barrage annoncé près du départ) ; « Police en approche » poussé comme par la poursuite
        for (int i = 0; i < 60 * 60 && g.player().dist * M / 1000 < 0.21; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 12 == 0) g.render(t); }
        toasts::pousser(ui::tr("toast.police_approche"), "", 2);
        images(15);
        capture(jour ? "messages-poursuite-surprise-jour" : "messages-poursuite-surprise-nuit");
        g.shutdown();
    }
    poursuiteBancFixe(false);
    modeCourse() = ModeCourse::Checkpoint;
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);
    WFS_LOG("messages (WFS-567) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs
