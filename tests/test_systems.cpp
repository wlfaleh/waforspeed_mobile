// Vérifie le mécanisme des systèmes enfichables (WFS-30) : enregistrement automatique et ordre d'appel.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/core/system.h"
#include "../src/core/gpu_logiciel.h"   // WFS-59
#include "../src/game/camera_vue.h"     // WFS-66 : fonctions pures des caméras (en-tête seul)
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
    // WFS-66 : caméras du jeu web (CAM_MODES, cycleCamera, volant −braquage × 2,3, aiguille 2,2 − v / 300 × 4,4, fov)
    auto proche = [](double a, double b) { return std::fabs(a - b) < 1e-5; };
    const bool cycle = vueApres(VueCamera::Poursuite) == VueCamera::Capot && vueApres(VueCamera::Capot) == VueCamera::Cockpit &&
                       vueApres(VueCamera::Cockpit) == VueCamera::Poursuite;
    const bool vues = proche(VUES[0].fov, 58) && proche(VUES[2].fov, 72) && VUES[2].cockpit && VUES[1].capot && !VUES[0].capot;
    const bool aiguille = proche(aiguilleAngle(0), 2.2) && proche(aiguilleAngle(150), 0) && proche(aiguilleAngle(300), -2.2) &&
                          proche(aiguilleAngle(420), -2.2) && proche(aiguilleAngle(-150), 0);
    float vol = 0; for (int i = 0; i < 120; i++) vol = lisser(vol, volantCible(0.5), 1.f / 60, 14.f);   // 2 s de braquage à 0,5
    const bool volant = proche(volantCible(1), -2.3) && proche(lisser(0, 1, 1.f / 60, 14.f), 14.0 / 60) && proche(vol, -1.15) &&
                        proche(lisser(0, 1, 0.5f, 14.f), 1);
    const bool fov = proche(fovVue(VueCamera::Cockpit, 0, false), 72) && proche(fovVue(VueCamera::Cockpit, 1, false), 79) &&
                     proche(fovVue(VueCamera::Cockpit, 2, true), 72 + 1.3 * 7 + 6) && proche(fovVue(VueCamera::Capot, 0.5, false), 75.5);
    std::printf("caméras : cycle %s, vues %s, aiguille %s, volant %s, champ de vision %s\n", cycle ? "OK" : "ÉCHEC", vues ? "OK" : "ÉCHEC",
                aiguille ? "OK" : "ÉCHEC", volant ? "OK" : "ÉCHEC", fov ? "OK" : "ÉCHEC");
    ok = ok && cycle && vues && aiguille && volant && fov;
    return ok ? 0 : 1;
}
