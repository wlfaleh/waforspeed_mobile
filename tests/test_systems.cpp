// Vérifie le mécanisme des systèmes enfichables (WFS-30) : enregistrement automatique et ordre d'appel.
#include <algorithm>
#include <cstdio>
#include <string>
#include "../src/core/system.h"
#include "../src/core/gpu_logiciel.h"   // WFS-59
using namespace wfs;
static std::string trace;
struct Deux : System { const char* name() const override { return "deux"; } void update(double, const GameView&) override { trace += "2"; } };
struct Un : System { const char* name() const override { return "un"; } void update(double, const GameView&) override { trace += "1"; } };
WFS_SYSTEM(Deux, 200)
WFS_SYSTEM(Un, 100)
int main() {
    auto e = systemRegistry();
    std::stable_sort(e.begin(), e.end(), [](const SystemEntry& a, const SystemEntry& b) { return a.order < b.order; });
    GameView v;
    for (auto& x : e) x.make()->update(0.01, v);
    std::printf("systèmes : %zu, ordre : %s\n", e.size(), trace.c_str());
    bool ok = e.size() == 2 && trace == "12";
    // WFS-59 : plafond de dt — 0,05 s hors mode (inchangé), 0,5 s en mode GPU logiciel ; 60 pas de 1/120 s au plus
    const bool horsMode = !modeGpuLogiciel() && plafondDt(3.0) == 0.05 && plafondDt(0.016) == 0.016;
    activerModeGpuLogiciel("test");
    const bool enMode = modeGpuLogiciel() && plafondDt(3.0) == 0.5 && plafondDt(0.2) == 0.2;
    double acc = 0; int total = 0; const double step = 1.0 / 120.0;
    for (int image = 0; image < 360; ++image) {   // 360 images à 0,3 i/s → 180 s de jeu attendues (temps réel)
        acc += plafondDt(1.0 / 0.3);
        for (int pas = 0; acc >= step && pas < gpu_logiciel::PAS_MAX; ++pas) { acc -= step; ++total; }
    }
    const double tempsJeu = total * step;
    desactiverModeGpuLogiciel();
    std::printf("mode GPU logiciel : hors mode %s, en mode %s, temps de jeu %.2f s pour 360 images\n",
                horsMode ? "OK" : "ÉCHEC", enMode ? "OK" : "ÉCHEC", tempsJeu);
    ok = ok && horsMode && enMode && tempsJeu > 179.9 && tempsJeu < 180.1;
    return ok ? 0 : 1;
}
