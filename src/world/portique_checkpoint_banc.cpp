// WaForSpeed — banc « portique-checkpoint » (WFS-562, RG-05 bis, RG-05 ter, captures demandées n° 3) : course Checkpoint neuve
// sur la Ville et la Corniche, de jour et de nuit, pilote automatique jusqu'au 1er checkpoint ; captures
// pc-<carte>-<jour|nuit>-approche (caméra de course, 60 m avant), -34 (trois quarts arrière, 12 m avant), -dessous (sous la bâche,
// vue de face depuis l'avant) et -ligne (ligne de chronométrage peinte, de près). Écart si le portique n'est pas dessiné
// (bâche absente de la scène) sur les 3 premiers checkpoints.
#include <cmath>
#include <string>
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"

namespace wfs {

bool portiqueCheckpointVisible();   // portique.cpp

int benchPortiqueCheckpoint(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    double t = 0;
    auto image = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const std::string& nom) { g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s", nom.c_str()); };
    // caméra de vitrine posée par rapport à la voiture : avantM devant, droiteM à droite, à hauteur m (comme le banc police-540)
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.camMode = 2; g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);
    const ModeCourse modeAvant = modeCourse();
    const VueCamera vueAvant = vueCamera();
    modeCourse() = ModeCourse::Checkpoint; choisirVue(VueCamera::Poursuite);
    if (!g.init(r, "golf", "ville")) { courseBenchRules(regles); WFS_ERR("portique-checkpoint : Ville impossible à charger"); return 1; }
    for (const char* carte : {"ville", "plage"})
        for (bool nuit : {false, true}) {
            const std::string base = std::string("pc-") + carte + (nuit ? "-nuit" : "-jour");
            g.allerA(Ecran::Menu); image(2);
            Game::demandes().carte = carte; Game::demandes().nuit = nuit;
            image(1);
            g.allerA(Ecran::Course); image(1);
            g.camMode = 1; g.setAutopilot(true);
            image(60 * 4);   // décompte
            const Course* c = activeCourse();
            auto avancer = [&](double resteM) {
                for (int i = 0; i < 60 * 90 && c && (c->nextCp - g.player().dist) * M > resteM; i++) {
                    g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 0) g.render(t);
                    courseBenchSetTime(30); c = activeCourse();
                }
            };
            avancer(60);
            if (!c) { ecarts++; WFS_ERR("portique-checkpoint : %s : pas de course", base.c_str()); continue; }
            // checkpoint en courbe serrée ou en tunnel (ligne peinte seule, WFS-139) : on passe au suivant (3 au plus)
            for (int k = 0; k < 3 && !portiqueCheckpointVisible(); k++) {
                g.render(t);
                if (portiqueCheckpointVisible()) break;
                WFS_LOG("portique-checkpoint : %s : checkpoint %d en ligne peinte seule, au suivant", base.c_str(), c->level + 1);
                const int niveau = c->level;   // passer ce checkpoint (niveau suivant), puis 60 m avant le prochain
                for (int i = 0; i < 60 * 30 && c && c->level == niveau; i++) { g.update(1.0 / 60); t += 1.0 / 60; courseBenchSetTime(30); c = activeCourse(); }
                avancer(60);
            }
            g.camMode = 1; capture(base + "-approche");
            const bool vu = portiqueCheckpointVisible();
            if (!vu) ecarts++;
            WFS_LOG("portique-checkpoint (WFS-562) : %s : portique dessiné à %.0f m : %s", base.c_str(), (c->nextCp - g.player().dist) * M, vu ? "OK" : "ÉCART (ligne seule ou absent)");
            avancer(12);
            g.setAutopilot(false); g.manette = DriveInput{}; g.modeManette = true;
            viser(-7.5, -3.2, 2.6f); capture(base + "-34");
            viser(26, 1.5, 3.2f); capture(base + "-dessous");
            avancer(4);
            viser(7, 5, 2.2f); capture(base + "-ligne");   // devant, à droite, tournée vers la voiture : la ligne peinte entre les deux
            g.modeManette = false; g.camMode = 0;
        }
    g.shutdown();
    modeCourse() = modeAvant; choisirVue(vueAvant);
    courseBenchRules(regles);
    return ecarts;
}

} // namespace wfs
