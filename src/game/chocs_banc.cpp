// WaForSpeed — banc des chocs avec le décor (WFS-256, WFS_BANC=chocs ; RG-06). Pour chaque carte (Ville, Corniche, Campagne,
// Grand Prix), la vraie partie est chargée (les cartes déclarent leurs objets solides : chocs_cartes.cpp, plage.cpp,
// decor_carte.cpp), puis :
//  - mesures sans GPU (chocs_banc.h) : tours complets au pilote sans choc (aucun mur invisible) ; chaque genre d'objet à portée
//    arrêté à 30, 80 et 150 km/h ; tableau dans chocs.txt ;
//  - captures chocs-<carte>-0 / -1 / -2 : la voiture lancée à 60 km/h sur le premier poteau (ou mur) à portée : avant le choc,
//    au choc, 1 s après (dans la partie : décor, caméra de poursuite, HUD).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "chocs_banc.h"
#include "conduite_banc.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

int benchChocs(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0; std::string rapport;
    const bool reglesAvant = courseBenchRulesOn(), aideAvant = reglagesConduite().aideDirection;
    double temps = 8000;
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        courseBenchRules(true);
        reglagesConduite().aideDirection = false;
        // 1) chargement réel de la carte (registre rempli), mesures sans GPU sur la même piste
        if (!g.init(r, "golf", carte)) { WFS_ERR("chocs : chargement de %s impossible", carte); ecarts++; continue; }
        Track& t = banc_conduite::piste(carte);
        ecarts += banc_chocs::mesurer(t, mapById(carte), rapport);
        // 2) obstacle visé pour les captures : premier poteau (sinon mur) à portée, assez loin du départ
        const auto& ps = chocs::registre().parSegment();
        int64_t seg = -1; chocs::ObstacleRoute cible{};
        for (chocs::Genre gv : {chocs::Genre::Poteau, chocs::Genre::Mur})
            for (size_t a = 200; a < ps.size() && seg < 0; a++)
                for (const chocs::ObstacleRoute& o : ps[a])
                    if (o.genre == gv && std::abs(o.lat) < mapById(carte).edge * RW - 0.5) { seg = (int64_t)a; cible = o; break; }
        g.shutdown();
        if (seg < 0) { WFS_LOG("chocs : %s : aucun poteau ni mur à portée pour les captures", carte); continue; }
        const int64_t depart = seg - (int64_t)(70 / SEG_M);
        setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
        const bool ok = g.init(r, "golf", carte);
        unsetenv("WFS_DEPART");
        if (!ok) { ecarts++; continue; }
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
        g.allerA(Ecran::Course);
        for (int i = 0; i < 60 * 4; i++) { g.update(1.0 / 60); temps += 1.0 / 60; if (i % 20 == 0) g.render(temps); }   // décompte
        const Player& p = g.player();
        const TrackPoint B = t.pointLisse(seg * SEG + cible.along / M, cible.lat);
        int pris = 0, chocs0 = -1; double tChoc = 0;
        auto compte = [] { const Course* c = activeCourse(); return c ? c->bumps + c->accidents + c->scrapes : 0; };   // chocs de la course
        const int k0 = compte();
        for (int n = 0; n < 60 * 25 && pris < 3; n++) {
            // pilote du banc : droit sur l'obstacle, 60 km/h
            DriveInput in; in.gas = p.kmh() < 60; in.brake = false;
            const double dx = B.x - p.monde.x, dz = B.z - p.monde.z, sn = std::sin(p.monde.cap), cs = std::cos(p.monde.cap);
            const double av = dx * sn - dz * cs, dr = dx * cs + dz * sn;
            in.steer = (float)p.braquagePour(2 * dr / std::max(1.0, av * av + dr * dr), std::max(1.0, std::abs(p.monde.u())));
            g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
            if (compte() > k0 && chocs0 < 0) { chocs0 = n; tChoc = temps; }
            bool prendre = false; const char* quand = "";
            if (pris == 0 && av < 12 && av > 0) { prendre = true; quand = "avant le choc"; }
            else if (pris == 1 && chocs0 >= 0) { prendre = true; quand = "au choc"; }
            else if (pris == 2 && chocs0 >= 0 && temps - tChoc >= 1.0) { prendre = true; quand = "1 s après"; }
            if (n % 6 == 0 || prendre) g.render(temps);
            if (!prendre) continue;
            g.render(temps + 0.005);
            char nom[64]; std::snprintf(nom, sizeof nom, "chocs-%s-%d.png", carte, pris);
            r.capturePng(out + "/" + nom);
            WFS_LOG("capture %s (%s) : %s à %.1f m, %3.0f km/h, écart %+.2f m, cap %+.2f rad", nom, quand, chocs::nomGenre(cible.genre),
                    std::max(0.0, av), p.kmh(), p.playerX * RW, p.heading);
            pris++;
        }
        if (pris < 3) { WFS_ERR("chocs : %s : %d capture(s) sur 3 (le choc n'a pas eu lieu ?)", carte, pris); ecarts++; }
        g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    }
    courseBenchRules(reglesAvant); reglagesConduite().aideDirection = aideAvant;
    { std::ofstream f(out + "/chocs.txt"); f << rapport; }
    WFS_LOG("banc des chocs (WFS-256) : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
