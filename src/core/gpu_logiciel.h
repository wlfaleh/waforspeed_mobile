// WaForSpeed — mode GPU logiciel (WFS-59, SIM-MVP-1) : point d'extension du noyau.
// Sur l'émulateur de la QA (GPU logiciel SwiftShader / llvmpipe) le jeu tourne à moins d'une image par seconde :
// avec le plafond normal de dt (0,05 s) le temps du jeu avance de ≈ 0,015 s par seconde réelle. En mode actif :
//   - temps de jeu au temps réel : plafond de dt porté à 0,5 s (pas fixes de 1/120 s inchangés, 60 pas au plus par image) ;
//   - rendu allégé : scène 3D à 25 % (src/render/vk.cpp) et ombres du soleil coupées (Renderer::forceNoShadows, Game::init).
// Activation : GPU logiciel détecté par VkCtx::init (appareil seulement, jamais le banc d'essai hors écran)
// ou extra d'intent `qa_simu=true` (src/platform/android_main.cpp). Hors mode : rien ne change.
#pragma once
#include <algorithm>
#include "log.h"

namespace wfs {
namespace gpu_logiciel {
constexpr double DT_MAX = 0.05;            // plafond de dt normal (Game::update)
constexpr double DT_MAX_LOGICIEL = 0.5;    // plafond de dt en mode GPU logiciel
constexpr int PAS_MAX = 60;                // pas de 1/120 s au plus par image (= 0,5 s)
constexpr float RENDU = 0.25f;             // échelle de la scène 3D en mode GPU logiciel
inline bool& etat() { static bool actif = false; return actif; }
}

inline bool modeGpuLogiciel() { return gpu_logiciel::etat(); }

// active le mode (une seule fois) ; raison : nom du GPU ou « intent qa_simu=true »
inline void activerModeGpuLogiciel(const char* raison) {
    if (gpu_logiciel::etat()) return;
    gpu_logiciel::etat() = true;
    WFS_LOG("mode GPU logiciel demandé (%s)", raison ? raison : "?");
}

// remet le mode à l'état inactif (tests)
inline void desactiverModeGpuLogiciel() { gpu_logiciel::etat() = false; }

// plafond de dt de Game::update selon le mode
inline double plafondDt(double dt) { return std::min(dt, modeGpuLogiciel() ? gpu_logiciel::DT_MAX_LOGICIEL : gpu_logiciel::DT_MAX); }

#if defined(__ANDROID__)
// lit l'extra `qa_simu` de l'intent de l'activité (booléen `--ez qa_simu true` ou texte `--es qa_simu true`)
// vm : JavaVM*, activite : jobject de l'activité (ANativeActivity::vm / clazz)
bool lireExtraQaSimu(void* vm, void* activite);
#endif
}
