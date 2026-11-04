// WaForSpeed — ressenti de conduite (ticket WFS-70, épic E3) : secousse de caméra, flash, ralenti, vibration du
// téléphone et option « réduire les mouvements ». Port de js/game/update.js du jeu web (playerAccident, bump, hitCar,
// hitGhosts, bordure : `shake`, `flash`, `timeScale`, `slowmoT`, `navigator.vibrate`), js/world/scene.js (secousse
// appliquée à la caméra), js/world/fx.js (flash) et js/core/config.js (`reduceMotion`).
//
// - Choc fort (accident) : secousse 0,9, flash 1, ralenti à 35 % pendant 0,5 s (Course::accident), vibration 90-40-120 ms.
// - Choc léger (bump) : secousse 0,25 + impact × 1,5, flash 0,35 (valeurs du jeu web).
// - Frottement : secousse 0,3, flash 0,25, vibration 40 ms. Bordure de la route (update.js, ligne 214) : secousse 0,12.
// - « Réduire les mouvements » : secousse et ralenti à 0, flash et vibrations gardés.
// Les réglages (mode de direction, réduire les mouvements) sont publics : l'écran des réglages (WFS-41) les modifie,
// la sauvegarde (WFS-24) les mémorisera. Partie pure (en-tête seul) : vérifiée par tests/test_course.cpp.
#pragma once
#include <algorithm>
#include <string>
#include "../core/math.h"

namespace wfs {

class Game;
class Renderer;

// modes de direction (steerMode du jeu web : 'glisser' / 'fleches')
enum class ModeDirection { Glisser = 0, Fleches = 1 };

// RG-09 : réglages publics, lus et modifiés par les écrans (WFS-41) et le banc d'essai (WFS_BANC=ressenti)
struct ReglagesRessenti {
    ModeDirection direction = ModeDirection::Glisser;   // par défaut : zone de glissement (comportement d'avant WFS-70)
    bool reduireMouvements = false;                     // reduceMotion du jeu web
};
inline ReglagesRessenti& reglagesRessenti() { static ReglagesRessenti r; return r; }

// secousse et flash (shake / flash du jeu web), en temps de jeu
struct EtatRessenti {
    float secousse = 0, flash = 0;
    // playerAccident : shake = reduceMotion ? 0 : 0,9 ; flash = 1
    void chocFort(bool reduit) { secousse = reduit ? 0.f : 0.9f; flash = 1.f; }
    // bump(impact) : shake = max(shake, 0,25 + impact × 1,5) ; flash = max(flash, 0,35)
    void chocLeger(double impact, bool reduit) {
        secousse = std::max(secousse, reduit ? 0.f : (float)(0.25 + impact * 1.5)); flash = std::max(flash, 0.35f);
    }
    // frottement (hitGhosts du jeu web, RG-03) : shake 0,3 ; flash 0,25
    void frottement(bool reduit) { secousse = std::max(secousse, reduit ? 0.f : 0.3f); flash = std::max(flash, 0.25f); }
    // bordure de la route, en course (update.js) : shake 0,12
    void bordure(bool reduit) { secousse = std::max(secousse, reduit ? 0.f : 0.12f); }
    // fin de update() : shake −= dt ; flash −= dt × 3
    void avancer(double dt) { secousse = std::max(0.f, secousse - (float)dt); flash = std::max(0.f, flash - (float)dt * 3.f); }
    // décalage de la caméra (scene.js) : ((rand − 0,5) × shake × 0,6 ; (rand − 0,5) × shake × 0,4 ; 0), axes du monde
    vec3 decalage(Mulberry32& rng, bool reduit) const {
        if (reduit || secousse <= 0) return {0, 0, 0};
        const float x = (float)(rng.next() - 0.5) * secousse * 0.6f, y = (float)(rng.next() - 0.5) * secousse * 0.4f;
        return {x, y, 0};
    }
    // couleur du flash (fx.js : rgba(255, 31, 75, flash × 0,28), codes sRGB comme ui::ROUGE)
    vec4 couleurFlash() const { return {1.f, 31 / 255.f, 75 / 255.f, flash * 0.28f}; }
};

// ---------- vibration du téléphone ----------
// motif = navigator.vibrate([...]) : durées en ms (vibre, pause, vibre…). Sur PC : seulement écrite au journal.
using Vibreur = void (*)(const int* ms, int n);
void installerVibreur(Vibreur v);   // Android : vibration_android.cpp (JNI Vibrator)
void vibrer(const int* ms, int n);   // journal « vibration 90-40-120 » + vibreur installé

// ---------- accroches ----------
const EtatRessenti& etatRessenti();
vec3 ressentiDecalageCamera();   // appelé par cameraVue (camera_vue.cpp) à chaque image de course
// banc d'essai (WFS_BANC=ressenti) : captures ressenti-choc, ressenti-fleches, ressenti-reduit
void benchRessenti(Game& g, Renderer& r, const std::string& outDir);

} // namespace wfs
