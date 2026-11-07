// WaForSpeed — banc de conduite dans la partie (WFS-241, WFS_BANC=conduite) : tableau du banc sans GPU (conduite_banc.h)
// écrit dans conduite.txt, puis « vidéos » (suites de captures) dans la vraie partie (Game, trafic, décor, caméra de
// poursuite, HUD) avec le nouveau modèle de conduite :
//  - conduite-epingle-aide-NN / conduite-epingle-sans-aide-NN : épingle du Grand Prix, volant lâché (le joueur simulé ne
//    tient que la vitesse du virage), aide activée puis désactivée ;
//  - conduite-rond-point-aide-NN / conduite-rond-point-sans-aide-NN : rond-point de la Ville, même chose ;
//  - conduite-rg07-NN : RG-07, aide désactivée, aucun appui à 240 km/h avant le rond-point de la Ville (SF90 lancée à fond
//    puis tout lâché) : la voiture va tout droit, sans freinage ni nitro automatiques.
// Vitesse, volant, angle des roues, écart et cap de chaque capture au journal du banc.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "conduite_banc.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

int benchConduite(Game& g, Renderer& r, const std::string& out) {
    std::string rapport;
    int ecarts = banc_conduite::mesurer(rapport);
    { std::ofstream f(out + "/conduite.txt"); f << rapport; }
    WFS_LOG("banc de conduite (WFS-241) : tableau dans conduite.txt, %d écart(s)", ecarts);
    ui::chargerPolice(r);
    const bool aideAvant = reglagesConduite().aideDirection, reglesAvant = courseBenchRulesOn();
    const std::vector<banc_conduite::Virage> vs = banc_conduite::virages();
    double temps = 7000;
    struct Video { const char* nom; const char* carte; const char* voiture; bool aide, rg07; };
    const Video VIDEOS[] = {{"epingle-aide", "circuit", "golf", true, false}, {"epingle-sans-aide", "circuit", "golf", false, false},
                            {"rond-point-aide", "ville", "golf", true, false}, {"rond-point-sans-aide", "ville", "golf", false, false},
                            {"rg07", "ville", "sf90", false, true}};
    for (const Video& vd : VIDEOS) {
        const banc_conduite::Virage* vg = nullptr;
        for (const auto& v : vs) if (!std::strcmp(v.carte, vd.carte) && (std::strstr(v.nom, "épingle") || std::strstr(v.nom, "rond-point de la Ville"))) { vg = &v; break; }
        if (!vg) { WFS_ERR("banc de conduite : virage de %s introuvable", vd.carte); ecarts++; continue; }
        Track& t = banc_conduite::piste(vd.carte);
        const double avantM = vd.rg07 ? 700 : 60;   // RG-07 : lancée de 700 m pour atteindre 240 km/h
        const int64_t depart = std::max<int64_t>(0, (int64_t)((vg->debutU - avantM / M) / SEG));
        reglagesConduite().aideDirection = vd.aide;
        setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
        courseBenchRules(true);
        const bool ok = g.init(r, vd.voiture, vd.carte);
        unsetenv("WFS_DEPART");
        if (!ok) { WFS_ERR("banc de conduite : chargement de %s impossible", vd.carte); ecarts++; continue; }
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
        g.allerA(Ecran::Course);
        for (int i = 0; i < 60 * 4; i++) { g.update(1.0 / 60); temps += 1.0 / 60; if (i % 20 == 0) g.render(temps); }   // décompte
        const Player& p = g.player();
        // captures : 10 images de 20 m avant le virage à 30 m après (RG-07 : 8 images du lâcher jusqu'à 3 s après)
        const double d0 = vg->debutU - 20 / M, d1 = vg->finU + 30 / M;
        int pris = 0, n = 0; const int nb = vd.rg07 ? 8 : 10;
        bool lache = false; double tLache = 0;
        for (; n < 60 * 90 && pris < nb; n++) {
            DriveInput in = joueurLigneIdeale(p, t, false);   // volant lâché : vitesse du virage tenue seulement
            if (vd.rg07) { in = DriveInput{}; if (!lache) { in.gas = true; if (p.kmh() >= 240 || p.dist >= vg->debutU - 150 / M) { lache = true; tLache = temps; } } }
            g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
            bool prendre;
            if (vd.rg07) prendre = lache && temps - tLache >= pris * 3.0 / (nb - 1);
            else prendre = p.dist >= d0 + (d1 - d0) * pris / (nb - 1) || (n > 60 * 60);
            if (n % 10 == 0 || prendre) g.render(temps);
            if (!prendre) continue;
            g.render(temps + 0.005);
            char nom[96]; std::snprintf(nom, sizeof nom, "conduite-%s-%02d.png", vd.nom, pris);
            r.capturePng(out + "/" + nom);
            WFS_LOG("capture %s : %+6.1f m du virage, %3.0f km/h, volant %+5.2f, roues %+6.3f rad (aide %+6.3f), écart %+5.2f m, cap %+5.2f rad%s%s",
                    nom, (p.dist - vg->debutU) * M, p.kmh(), p.steer, p.braquageRoues, p.aideRoues, p.playerX * RW, p.heading,
                    p.braking ? ", FREIN" : "", p.nitroOn ? ", NITRO" : "");
            pris++;
        }
        if (pris < nb) { WFS_ERR("banc de conduite : %s, %d capture(s) sur %d", vd.nom, pris, nb); ecarts++; }
        g.manette = DriveInput{}; g.modeManette = false; g.shutdown(); courseBenchRules(reglesAvant);
    }
    reglagesConduite().aideDirection = aideAvant;
    WFS_LOG("banc de conduite (WFS-241) : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
