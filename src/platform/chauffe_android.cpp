// WaForSpeed — chauffe et fréquence d'affichage sur Android (WFS-123, RG-01 / RG-04), compilé avec android_main.cpp.
//   - RG-01 : état thermique d'Android (android/thermal.h : AThermal_acquireManager, AThermal_getCurrentThermalStatus,
//     AThermal_registerThermalStatusListener, API 30+). Les fonctions sont chargées par dlsym (minSdk 26) : sous l'API 30,
//     aucun symbole, statut −1 → niveau 0, rien d'autre. Le rappel d'Android (thread binder) écrit un atomique ; la boucle
//     de jeu le lit par chauffeNiveau() (source de chauffe.h).
//   - RG-04 : ANativeWindow_setFrameRate(fenêtre, 60, défaut) à chaque création de la fenêtre (API 30+, par dlsym) : les
//     écrans 90 / 120 Hz affichent 60 images/s stables, la présentation FIFO de vk.cpp ne change pas.
#if defined(__ANDROID__)
#include <android/native_window.h>
#include <dlfcn.h>
#include <atomic>
#include "../core/log.h"
#include "chauffe.h"

namespace wfs {
namespace {
// android/thermal.h (API 30) : types et signatures recopiés pour l'appel par dlsym (en-tête absent sous l'API 30)
struct AThermalManager;
using FnAcquerir = AThermalManager* (*)();
using FnStatut = int (*)(AThermalManager*);
using FnRappel = void (*)(void* donnees, int statut);
using FnEcouter = int (*)(AThermalManager*, FnRappel, void*);
using FnFrequence = int32_t (*)(ANativeWindow*, float, int8_t);

std::atomic<int> g_statutAndroid{-1};
AThermalManager* g_thermique = nullptr;
FnFrequence g_setFrameRate = nullptr;
bool g_chauffeCharge = false;

void* libAndroid() { static void* lib = dlopen("libandroid.so", RTLD_NOW); return lib; }

// thread binder d'Android : on ne fait qu'écrire le statut (lu par la boucle de jeu)
void rappelThermique(void*, int statut) { g_statutAndroid = statut; }
int statutAndroid() { return g_statutAndroid; }
} // namespace

// au démarrage (android_main) : gestionnaire thermique, statut actuel et écoute des changements
void chauffeAndroidDemarrer() {
    if (g_chauffeCharge) return;
    g_chauffeCharge = true;
    chauffe::definirSource(statutAndroid);   // toujours installée : plafond 30 / 60 images/s même sous l'API 30
    void* lib = libAndroid();
    auto acquerir = lib ? (FnAcquerir)dlsym(lib, "AThermal_acquireManager") : nullptr;
    auto lire = lib ? (FnStatut)dlsym(lib, "AThermal_getCurrentThermalStatus") : nullptr;
    auto ecouter = lib ? (FnEcouter)dlsym(lib, "AThermal_registerThermalStatusListener") : nullptr;
    g_setFrameRate = lib ? (FnFrequence)dlsym(lib, "ANativeWindow_setFrameRate") : nullptr;
    if (!acquerir || !lire || !ecouter) { WFS_LOG("CHAUFFE état thermique absent (API < 30) : niveau 0"); return; }
    g_thermique = acquerir();
    if (!g_thermique) { WFS_WARN("CHAUFFE gestionnaire thermique indisponible : niveau 0"); return; }
    g_statutAndroid = lire(g_thermique);
    const int code = ecouter(g_thermique, rappelThermique, nullptr);
    WFS_LOG("CHAUFFE écoute de l'état thermique (statut Android %d, code %d)", g_statutAndroid.load(), code);
}

// RG-04 : à chaque création de la fenêtre (APP_CMD_INIT_WINDOW)
void chauffeAndroidFenetre(ANativeWindow* fenetre) {
    if (!fenetre) return;
    if (!g_setFrameRate) { WFS_LOG("setFrameRate absent (API < 30) : fréquence de l'écran inchangée"); return; }
    const int32_t code = g_setFrameRate(fenetre, 60.f, 0 /* ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT */);
    WFS_LOG("setFrameRate 60 : %s (code %d)", code == 0 ? "ok" : "refusé", (int)code);
}

} // namespace wfs
#endif
