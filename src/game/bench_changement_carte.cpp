// WaForSpeed — banc d'essai du changement de carte (WFS-74) : écran de chargement, course neuve, course figée hors de l'écran
// Course. Captures : terrain-fige-1 / terrain-fige-2 (2 s d'écart sur l'écran Terrain, même chrono), terrain-chargement
// (barre partielle et étape), terrain-arrivee (nouvelle carte, décompte, chrono et score à zéro, peinture du garage gardée).
#include "bench_changement_carte.h"
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/platform.h"
#include "../render/renderer.h"
#include "../ui/ecrans/chargement.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

void benchChangementCarte(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    unsetenv("WFS_MOMENT");
    if (!g.init(r, "golf", "ville")) { WFS_ERR("changement de carte : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 1200;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto etat = [&]() {
        const Course* c = activeCourse();
        char b[160];
        std::snprintf(b, sizeof b, "chrono %.2f s, temps de course %.2f s, score %lld, checkpoint %d", c ? c->timeLeft : 0.0,
                      c ? c->raceTime : 0.0, c ? c->score(g.player()) : 0, c ? c->level + 1 : 0);
        return std::string(b);
    };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s — %s", nom, nomEcran(g.ecran()), quoi, etat().c_str());
    };
    // ---------- RG-05 : course figée derrière l'écran Terrain ----------
    for (int i = 0; i < 60 * 6; i++) image();   // décompte (3 s) puis 3 s de course
    g.allerA(Ecran::Terrain);
    for (int i = 0; i < 6; i++) image();
    capture("terrain-fige-1", "écran Terrain");
    const std::string avant = etat();
    for (int i = 0; i < 120; i++) image();      // 2 s plus tard
    capture("terrain-fige-2", "écran Terrain, 2 s plus tard");
    WFS_LOG("changement de carte : course figée %s (%s / %s)", etat() == avant ? "OUI, identique" : "NON, DIFFÉRENTE", avant.c_str(), etat().c_str());
    // ---------- RG-01 / RG-04 / RG-06 : « Rouler » vers le circuit de jour, écran de chargement branché comme sur Android ----------
    ecrans::Chargement& ch = ecrans::chargement();
    Game::Demandes& d = Game::demandes();
    d.debutChargement = [&r] { ecrans::chargement().debut(r, nowSeconds()); };
    d.progression = ecrans::Chargement::progression;
    d.finChargement = [](bool ok) { if (ok) ecrans::chargement().fin(); else ecrans::chargement().erreur(); };
    ch.apresImage = [&](int etape) {   // étape 2 : « Chargement du modèle 3D… » (barre partielle)
        if (etape != 2) return;
        r.capturePng(out + "/terrain-chargement.png");
        WFS_LOG("capture terrain-chargement : %s %d %%", ch.libelle().c_str(), (int)std::lround(ch.pourcent() * 100));
    };
    d.carte = "circuit"; d.nuit = false;   // ce que fait « Rouler » de l'écran Terrain (terrain.cpp)
    image();
    ch.apresImage = nullptr;
    for (int i = 0; i < 60; i++) image();       // 1 s : décompte « 2 »
    capture("terrain-arrivee", "circuit de jour, course neuve");
    // ---------- RG-02 : même carte, même moment → pas de reconstruction, course neuve ----------
    for (int i = 0; i < 60 * 4; i++) image();
    g.allerA(Ecran::Terrain); image();
    d.carte = "circuit"; d.nuit = false;
    image(); image();
    WFS_LOG("changement de carte : même carte → écran %s, %s (course neuve attendue : décompte, chrono 30 s)", nomEcran(g.ecran()), etat().c_str());
    // ---------- pause puis reprise : pas de remise à zéro ----------
    for (int i = 0; i < 60 * 5; i++) image();
    const std::string p0 = etat();
    g.allerA(Ecran::Pause);
    for (int i = 0; i < 90; i++) image();
    g.allerA(Ecran::Course); image();
    WFS_LOG("changement de carte : pause puis reprise : avant %s / après %s", p0.c_str(), etat().c_str());
    d.debutChargement = nullptr; d.progression = nullptr; d.finChargement = nullptr;
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);
}

} // namespace wfs
