// WaForSpeed — banc « portique-depart » (WFS-559, RG-09 et captures demandées) : départ d'une course neuve sur la Ville et la
// Corniche, de jour et de nuit, en Checkpoint (voiture sur la ligne) et en Course à 5 (grille derrière la ligne), comme le
// joueur : menu, carte choisie, « Rouler ». Captures portique-<mode>-<carte>-<jour|nuit>-feux0 / feux3 / feux5 / go.png.
// Écart si les feux ne suivent pas le décompte (RG-06 : 0 avant le 1er « 3 », 3 puis 5 rouges, verts au départ) ou si le
// décompte change de durée (3 s, RG-01).
#include <cmath>
#include <string>
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"

namespace wfs {

int portiqueDepartLumiere();   // portique_depart.cpp

int benchPortiqueDepart(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) {
        if (!ok) ecarts++;
        WFS_LOG("portique-depart (WFS-559) : %s : %s", quoi.c_str(), ok ? "OK" : "ÉCART");
    };
    double t = 0;
    auto image = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto capture = [&](const std::string& nom) { g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s", nom.c_str()); };
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);   // décompte et chrono du jeu (le banc PC les coupe par défaut)
    const ModeCourse modeAvant = modeCourse();
    if (!g.init(r, "golf", "ville")) { courseBenchRules(regles); WFS_ERR("portique-depart : Ville impossible à charger"); return 1; }
    const char* modes[3] = {"checkpoint", "course5", "course5-proche"};   // vue « Arrière » (par défaut), puis « Proche » (cadrage de la maquette 09)
    const VueCamera vueAvant = vueCamera();
    for (int im = 0; im < 3; im++)
        for (const char* carte : {"ville", "plage"})
            for (bool nuit : {false, true}) {
                const std::string base = std::string("portique-") + modes[im] + "-" + carte + (nuit ? "-nuit" : "-jour");
                modeCourse() = im ? ModeCourse::Course5 : ModeCourse::Checkpoint;
                choisirVue(im == 2 ? VueCamera::Proche : VueCamera::Poursuite);
                if (im == 2 && nuit == false && std::string(carte) == "plage") continue;   // vue Proche : Ville jour et nuit, Corniche de nuit
                g.allerA(Ecran::Menu); image(2);
                Game::demandes().carte = carte; Game::demandes().nuit = nuit;
                image(1);
                g.allerA(Ecran::Course); image(1);
                bool decompte = false;
                for (int i = 0; i < 60 && !decompte; i++) { decompte = activeCourse() && activeCourse()->state == RaceState::Count; if (!decompte) image(1); }
                verifier(decompte, base + " : décompte au départ");
                if (!decompte) continue;
                const Course* c = activeCourse();
                if (c->lastCount < 0) { verifier(c->feuxRouges() == 0, base + " : aucun feu avant le 1er « 3 »"); capture(base + "-feux0"); }
                int n = 0;
                double debut = -1;
                float bx = 0, by = 0;
                bool vu3 = false, vu5 = false, vuGo = false;
                for (; n < 60 * 8 && !vuGo; n++) {
                    g.update(1.0 / 120); t += 1.0 / 120;
                    c = activeCourse();
                    if (!c) break;
                    if (debut < 0 && c->lastCount >= 0) debut = t;
                    if (!vu3 && c->feuxRouges() == 3) { vu3 = true; capture(base + "-feux3"); verifier(!boutonVue(bx, by), base + " : bouton de vue caché pendant le décompte (RG-10)");
                        verifier(portiqueDepartLumiere() == (nuit ? 1 : 0), base + (nuit ? " : lumière rouge des feux posée (RG-04)" : " : pas de lumière des feux de jour")); }
                    if (!vu5 && c->feuxRouges() == 5) { vu5 = true; capture(base + "-feux5"); }
                    if (c->feuxVerts()) {
                        vuGo = true; image(6); capture(base + "-go");
                        verifier(boutonVue(bx, by), base + " : bouton de vue revenu au départ (RG-10)");
                        verifier(portiqueDepartLumiere() == (nuit ? 2 : 0), base + (nuit ? " : lumière verte des feux posée au GO (RG-04)" : " : pas de lumière des feux de jour"));
                        verifier(debut > 0 && std::abs((t - 0.1) - debut - 3.0) < 0.1, base + " : décompte de 3 s");
                    }
                }
                verifier(vu3 && vu5 && vuGo, base + " : 3 puis 5 feux rouges, verts au départ");
                image(60);   // 1 s après le départ : feux éteints
                verifier(activeCourse() && activeCourse()->feuxRouges() == 0 && !activeCourse()->feuxVerts(), base + " : feux éteints après le GO");
            }
    modeCourse() = modeAvant; choisirVue(vueAvant);
    g.shutdown();
    courseBenchRules(regles);
    WFS_LOG("portique-depart (WFS-559) : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
