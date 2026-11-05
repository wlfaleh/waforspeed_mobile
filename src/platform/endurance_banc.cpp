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
#include "../game/traffic_system.h"
#include "../ui/ecrans/chargement.h"
#if defined(__linux__)
#include <dlfcn.h>
#include <malloc.h>
#include <atomic>
#include <vulkan/vulkan.h>

// WFS-126 (RG-09) : compteurs Vulkan du banc, sans toucher src/render : wfs_bench appelle directement vkAllocateMemory,
// vkCreateBuffer… (pas de table de fonctions) ; ces définitions de l'exécutable passent donc avant celles du chargeur
// (libvulkan), qu'elles appellent ensuite (dlsym RTLD_NEXT). Compter seulement : aucun comportement ne change.
namespace {
std::atomic<long> g_vkMem{0}, g_vkOctets{0}, g_vkTampons{0}, g_vkImages{0}, g_vkPipelines{0};
template <class F> F suivante(const char* nom) { static_assert(sizeof(F) == sizeof(void*)); return reinterpret_cast<F>(dlsym(RTLD_NEXT, nom)); }
} // namespace
extern "C" {
VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice d, const VkMemoryAllocateInfo* i, const VkAllocationCallbacks* a, VkDeviceMemory* m) {
    static auto f = suivante<PFN_vkAllocateMemory>("vkAllocateMemory");
    const VkResult r = f(d, i, a, m);
    if (r == VK_SUCCESS) { g_vkMem++; g_vkOctets += (long)i->allocationSize; }
    return r;
}
VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice d, VkDeviceMemory m, const VkAllocationCallbacks* a) {
    static auto f = suivante<PFN_vkFreeMemory>("vkFreeMemory");
    if (m != VK_NULL_HANDLE) g_vkMem--;   // octets libérés inconnus ici : voir g_vkOctets (cumul alloué)
    f(d, m, a);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice d, const VkBufferCreateInfo* i, const VkAllocationCallbacks* a, VkBuffer* b) {
    static auto f = suivante<PFN_vkCreateBuffer>("vkCreateBuffer");
    const VkResult r = f(d, i, a, b); if (r == VK_SUCCESS) g_vkTampons++; return r;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice d, VkBuffer b, const VkAllocationCallbacks* a) {
    static auto f = suivante<PFN_vkDestroyBuffer>("vkDestroyBuffer"); if (b != VK_NULL_HANDLE) g_vkTampons--; f(d, b, a);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice d, const VkImageCreateInfo* i, const VkAllocationCallbacks* a, VkImage* im) {
    static auto f = suivante<PFN_vkCreateImage>("vkCreateImage");
    const VkResult r = f(d, i, a, im); if (r == VK_SUCCESS) g_vkImages++; return r;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice d, VkImage im, const VkAllocationCallbacks* a) {
    static auto f = suivante<PFN_vkDestroyImage>("vkDestroyImage"); if (im != VK_NULL_HANDLE) g_vkImages--; f(d, im, a);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice d, VkPipelineCache c, uint32_t n, const VkGraphicsPipelineCreateInfo* i,
                                                         const VkAllocationCallbacks* a, VkPipeline* p) {
    static auto f = suivante<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines");
    const VkResult r = f(d, c, n, i, a, p); if (r == VK_SUCCESS) g_vkPipelines += n; return r;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice d, VkPipeline p, const VkAllocationCallbacks* a) {
    static auto f = suivante<PFN_vkDestroyPipeline>("vkDestroyPipeline"); if (p != VK_NULL_HANDLE) g_vkPipelines--; f(d, p, a);
}
} // extern "C"
#endif

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

// WFS-126 (RG-09) : relevé d'un cycle : RSS (anonyme / fichiers), tas de malloc, objets Vulkan vivants, trafic
struct Releve { double rss = 0, anon = 0, fich = 0, tas = 0, tasReserve = 0, vkMo = 0; long vkMem = 0, tampons = 0, images = 0, pipelines = 0; int trafic = 0; };
Releve relever() {
    Releve m;
    if (FILE* f = std::fopen("/proc/self/status", "r")) {
        char l[256];
        while (std::fgets(l, sizeof l, f)) {
            if (!std::strncmp(l, "VmRSS:", 6)) m.rss = std::atof(l + 6) / 1024;
            else if (!std::strncmp(l, "RssAnon:", 8)) m.anon = std::atof(l + 8) / 1024;
            else if (!std::strncmp(l, "RssFile:", 8)) m.fich = std::atof(l + 8) / 1024;
        }
        std::fclose(f);
    }
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC_MINOR__ >= 33)
    const struct mallinfo2 mi = mallinfo2();
    m.tas = (double)(mi.uordblks + mi.hblkhd) / (1024.0 * 1024.0);   // octets demandés à malloc et encore utilisés
    m.tasReserve = (double)(mi.arena + mi.hblkhd) / (1024.0 * 1024.0);   // octets que malloc garde (libres compris)
#endif
#if defined(__linux__)
    m.vkMem = g_vkMem; m.vkMo = g_vkOctets / (1024.0 * 1024.0); m.tampons = g_vkTampons; m.images = g_vkImages; m.pipelines = g_vkPipelines;
#endif
    if (const Traffic* t = activeTraffic()) m.trafic = (int)t->cars.size();
    return m;
}
void journalReleve(int cy, const Releve& m) {
    WFS_LOG("ENDURANCE cycle %d : RSS %.1f Mo, anonyme %.1f Mo, fichiers %.1f Mo, tas utilisé %.1f Mo (gardé par malloc %.1f Mo), "
            "Vulkan %ld allocations (%.0f Mo alloués en tout), %ld tampons, %ld images, %ld pipelines, trafic %d voitures",
            cy, m.rss, m.anon, m.fich, m.tas, m.tasReserve, m.vkMem, m.vkMo, m.tampons, m.images, m.pipelines, m.trafic);
}
// pente (Mo par cycle) par moindres carrés sur les cycles [de, a]
double pente(const double* v, int de, int a) {
    double sx = 0, sy = 0, sxx = 0, sxy = 0; int n = 0;
    for (int c = de; c <= a; c++) { sx += c; sy += v[c]; sxx += (double)c * c; sxy += c * v[c]; n++; }
    const double d = n * sxx - sx * sx;
    return d != 0 ? (n * sxy - sx * sy) / d : 0;
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
    double vRss[CYCLES + 1] = {}, vTas[CYCLES + 1] = {}, vReserve[CYCLES + 1] = {}, vFich[CYCLES + 1] = {};   // RG-09 : un relevé par cycle
    for (int cy = 1; cy <= CYCLES; cy++) {
        carte = CARTES[(cy - 1) % 4]; nuit = cy > 4;   // 4 cartes de jour, puis les 4 de nuit
        const Releve rel = relever();
        journalReleve(cy, rel);
        vRss[cy] = rel.rss; vTas[cy] = rel.tas; vReserve[cy] = rel.tasReserve; vFich[cy] = rel.fich;
        const double rss = rel.rss;
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
    journalReleve(CYCLES + 1, relever());   // relevé de fin (« cycle 9 »)
    // RG-09 (b) : pente par moindres carrés sur les cycles 2 à 8 (RSS, tas utilisé, tas gardé par malloc, fichiers)
    WFS_LOG("endurance : pente (cycles 2 à 8) : RSS %+.2f Mo/cycle, tas utilisé %+.2f Mo/cycle, tas gardé %+.2f Mo/cycle, fichiers %+.2f Mo/cycle",
            pente(vRss, 2, CYCLES), pente(vTas, 2, CYCLES), pente(vReserve, 2, CYCLES), pente(vFich, 2, CYCLES));
    g.setAutopilot(false);
    courseBenchRules(false);
    g.shutdown();
    if (croissance > 10) { WFS_ERR("endurance : fuite de mémoire : +%.1f %% au cycle %d (seuil 10 %%)", croissance, pire); return 3; }
    return 0;
}

} // namespace wfs
