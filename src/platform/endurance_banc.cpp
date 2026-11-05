// WaForSpeed — banc d'endurance (WFS-91) : voir endurance_banc.h.
#include "endurance_banc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/chargement.h"

namespace wfs {

namespace {

// garage du banc : « Choisir » sans toucher l'écran (GameView::choisirVoiture, comme garage.cpp), voiture et peinture
const char* g_voiture = nullptr;
vec3 g_peinture{};
class EnduranceGarage : public System {
public:
    const char* name() const override { return "endurance-garage"; }
    void update(double, const GameView& g) override {
        if (g_voiture && g.choisirVoiture) { g.choisirVoiture(g_voiture, g_peinture); g_voiture = nullptr; }
    }
};

// mémoire résidente du processus (Mo), VmRSS de /proc/self/status ; 0 si illisible
double rssMo() {
    FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return 0;
    char l[256]; double kb = 0;
    while (std::fgets(l, sizeof l, f)) if (!std::strncmp(l, "VmRSS:", 6)) { kb = std::atof(l + 6); break; }
    std::fclose(f);
    return kb / 1024.0;
}

const char* CARTES[4] = {"ville", "plage", "campagne", "circuit"};
const char* VOITURES[3] = {"m4", "p911", "golf"};
const vec3 PEINTURES[3] = {{0.60f, 0.01f, 0.02f}, {0.01f, 0.05f, 0.27f}, {0.92f, 0.40f, 0.0f}};
constexpr int CYCLES = 8;
constexpr double COURSE_MAX = 78;   // s de course au plus par cycle (puis fin forcée : temps écoulé)

} // namespace

WFS_SYSTEM(EnduranceGarage, 445)   // n'existe que dans wfs_bench (src/platform n'est pas dans l'APK)

int benchEndurance(VkCtx& vk, Renderer& r, Game& g, const std::string& out, unsigned largeur, unsigned hauteur) {
    courseBenchRules(true);
    unsetenv("WFS_MOMENT");
    std::string carte = "ville";
    bool nuit = false;
    if (!g.init(r, "golf", carte.c_str())) { WFS_ERR("endurance : chargement impossible"); courseBenchRules(false); return 4; }
    Game::Demandes& d = Game::demandes();
    auto brancherChargement = [&] {   // écran de chargement branché comme sur Android (initGraphics)
        d.debutChargement = [&r] { ecrans::chargement().debut(r, nowSeconds()); };
        d.progression = ecrans::Chargement::progression;
        d.finChargement = [](bool ok) { if (ok) ecrans::chargement().fin(); else ecrans::chargement().erreur(); };
    };
    brancherChargement();
    g.camMode = 0;
    double t = 3000, simule = 0;
    long images = 0;
    const double t0 = nowSeconds();
    // WFS-114 : une image rendue sur N (WFS_ENDURANCE_RENDU, 30 par défaut : deux par seconde de jeu) ; la simulation reste à 60 Hz.
    // Le rendu sous les couches de validation coûte l'essentiel du temps du job (≤ 15 min, RG-02).
    const char* rn = std::getenv("WFS_ENDURANCE_RENDU");
    const long tous = rn && std::atoi(rn) > 0 ? std::atoi(rn) : 30;
    long rendues = 0;
    auto pas = [&](int n) {
        for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; simule += 1.0 / 60; if (++images % tous == 0) { g.render(t); rendues++; } }
    };
    auto capture = [&](const std::string& nom) {
        g.render(t += 0.01); g.render(t += 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("endurance : capture %s (écran %s)", nom.c_str(), nomEcran(g.ecran()));
    };
    int retours = 0;
    // RG-02 / RG-09 : ce que fait Android sur APP_CMD_TERM_WINDOW puis APP_CMD_INIT_WINDOW (termGraphics / initGraphics)
    auto retourArrierePlan = [&](const char* ou) -> bool {
        const std::string voiture = g.player().car ? g.player().car->id : "golf";
        if (g.ecran() == Ecran::Course) { g.allerA(Ecran::Pause); pas(2); }   // APP_CMD_PAUSE en course → pause (WFS-39)
        const double av = rssMo();
        g.shutdown(); r.shutdown(); vk.shutdown();                              // termGraphics
        if (!vk.init(nullptr, largeur, hauteur)) { WFS_ERR("endurance : Vulkan non recréé après le retour %d", retours + 1); return false; }
        // ANO-52 : même Renderer réinitialisé, comme termGraphics d'android_main.cpp (WFS-100 RG-W1, contournement retiré : WFS-111)
        r.init(vk);                                                              // initGraphics
        ecrans::chargement().debut(r, nowSeconds());
        if (!g.init(r, voiture.c_str(), carte.c_str(), ecrans::Chargement::progression)) {
            ecrans::chargement().erreur(); WFS_ERR("endurance : partie non rechargée après le retour %d", retours + 1); return false;
        }
        ecrans::chargement().fin();
        brancherChargement();
        g.allerA(Ecran::Menu);   // WFS-38 : l'application repart sur le menu
        pas(40);
        retours++;
        capture("endurance-retour-" + std::to_string(retours));
        WFS_LOG("endurance : retour d'arrière-plan %d (%s), carte %s, %s, voiture %s, RSS %.1f → %.1f Mo", retours, ou, carte.c_str(),
                nuit ? "nuit" : "jour", voiture.c_str(), av, rssMo());
        return true;
    };
    double rss2 = 0, rssMax = 0;
    int pire = 0;
    for (int cy = 1; cy <= CYCLES; cy++) {
        carte = CARTES[(cy - 1) % 4]; nuit = cy > 4;   // 4 cartes de jour, puis les 4 de nuit
        const double rss = rssMo();
        if (cy == 2) rss2 = rss;
        if (cy >= 3 && rss > rssMax) { rssMax = rss; pire = cy; }
        WFS_LOG("endurance : cycle %d, carte %s, moment %s, RSS %.1f Mo", cy, carte.c_str(), nuit ? "nuit" : "jour", rss);
        // Menu → Terrain → « Rouler » (reconstruction de la partie sur la carte et le moment choisis)
        g.setAutopilot(false);
        g.allerA(Ecran::Menu); pas(30);
        g.allerA(Ecran::Terrain); pas(30);
        d.carte = carte; d.nuit = nuit;   // ce que fait « Rouler » (terrain.cpp)
        pas(1);
        if (g.ecran() != Ecran::Course) g.allerA(Ecran::Course);
        g.setAutopilot(true);
        // course en pilote automatique jusqu'à la fin (temps écoulé ou accident), au plus COURSE_MAX s
        double enCourse = 0;
        bool pause = false, retour = false;
        while (true) {
            pas(30); enCourse += 0.5;
            const Course* c = activeCourse();
            if (c && c->state == RaceState::Over) break;
            if (cy % 3 == 2 && !pause && enCourse >= 20) {   // un cycle sur trois : Pause puis reprise
                g.allerA(Ecran::Pause); pas(90); g.allerA(Ecran::Course); pas(2); pause = true;
                WFS_LOG("endurance : cycle %d, pause puis reprise", cy);
            }
            if (cy == 1 && !retour && enCourse >= 30) {   // RG-09 : retour d'arrière-plan en course
                if (!retourArrierePlan("en course")) return 4;
                retour = true;
                g.allerA(Ecran::Course); g.setAutopilot(true);   // la course repart (nouvelle course, comme sur téléphone)
            }
            if (enCourse >= COURSE_MAX) { courseBenchSetTime(0.01); WFS_LOG("endurance : cycle %d, fin forcée à %.0f s", cy, enCourse); }
            if (enCourse > COURSE_MAX + 10) break;
        }
        const Course* c = activeCourse();
        pas(70);   // écran de fin affiché
        capture("endurance-fin-" + carte + (nuit ? "-nuit" : "-jour"));
        WFS_LOG("endurance : cycle %d, fin de course (%s), %.0f s de course, %.0f km/h", cy, c && c->state == RaceState::Over ? "arrivée" : "pas finie",
                c ? c->raceTime : 0.0, g.player().kmh());
        g.setAutopilot(false);
        // un cycle sur trois : Garage (autre voiture, autre peinture)
        if (cy % 3 == 0) {
            g.allerA(Ecran::Garage); pas(40);
            const int k = (cy / 3 - 1) % 3;
            g_voiture = VOITURES[k]; g_peinture = PEINTURES[k];
            pas(2);
            WFS_LOG("endurance : cycle %d, garage : voiture %s", cy, g.player().car ? g.player().car->id : "?");
            if (cy == 3 && !retourArrierePlan("au garage")) return 4;
            if (g.ecran() == Ecran::Garage) { g.allerA(Ecran::Menu); pas(10); }
        }
        if (cy == 6 && !retourArrierePlan("au menu")) return 4;
    }
    const double rssFin = rssMo();
    if (rssFin > rssMax) { rssMax = rssFin; pire = CYCLES + 1; }
    const double croissance = rss2 > 0 ? (rssMax - rss2) / rss2 * 100 : 0;
    WFS_LOG("endurance : bilan : %d cycles, %d retours d'arrière-plan, %.1f min de jeu simulé, %ld pas, %.0f s réelles", CYCLES, retours,
            simule / 60, images, nowSeconds() - t0);
    WFS_LOG("endurance : durée : %.0f s réelles (%.1f min), %ld images rendues (une sur %ld)", nowSeconds() - t0, (nowSeconds() - t0) / 60, rendues, tous);
    WFS_LOG("endurance : bilan mémoire : RSS cycle 2 %.1f Mo, maximum %.1f Mo (cycle %d), fin %.1f Mo, croissance %+.1f %% (seuil 10 %%)",
            rss2, rssMax, pire, rssFin, croissance);
    g.setAutopilot(false);
    courseBenchRules(false);
    g.shutdown();
    if (croissance > 10) { WFS_ERR("endurance : fuite de mémoire : +%.1f %% au cycle %d (seuil 10 %%)", croissance, pire); return 3; }
    return 0;
}

} // namespace wfs
