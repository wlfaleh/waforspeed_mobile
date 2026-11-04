// WaForSpeed — caméras de course : vues poursuite, capot et cockpit (ticket WFS-66, épic E3).
// Port de js/world/scene.js du jeu web : CAM_MODES, cycleCamera, updateCamera (suivi, champ de vision) et buildCockpit
// (tableau de bord, montants, volant et compteur dessinés par code, attachés à la caméra).
//
// - Poursuite (vue par défaut) : caméra de poursuite de Game (inchangée, image identique), plus « jamais sous la route ».
// - Capot : œil au-dessus du capot, lié à la carrosserie, champ de vision 72° (hood du jeu web).
// - Cockpit : œil du conducteur (à gauche), voiture du joueur cachée, tableau de bord, montants de pare-brise, volant qui
//   tourne de −braquage × 2,3 (lissé) et aiguille de 0 à 300 km/h (2,2 − v / 300 × 4,4 rad), champ de vision 72°.
// La vue choisie est mémorisée (elle survit à Game::init : nouvelle course, changement de voiture) ; l'écran des
// réglages (WFS-41) et la sauvegarde (WFS-24) passent par vueCamera() / choisirVue().
#pragma once
#include <algorithm>
#include <cmath>
#include "../core/ecran.h"
#include "../core/math.h"

namespace wfs {

class Track;
struct Player;
struct CarDef;

enum class VueCamera { Poursuite = 0, Capot = 1, Cockpit = 2 };
constexpr int VUE_NOMBRE = 3;

// CAM_MODES du jeu web : 'proche' (poursuite, 58°), vue capot (hood), 'capot' / « Cockpit » (cockpit: true, 72°)
struct VueDef { const char* id; const char* nom; float fov; bool capot; bool cockpit; };
inline constexpr VueDef VUES[VUE_NOMBRE] = {
    {"poursuite", "Poursuite", 58.f, false, false},
    {"capot", "Capot", 72.f, true, false},
    {"cockpit", "Cockpit", 72.f, true, true},
};

// vue choisie (mémorisée pour toute la session) ; vueSuivante = cycleCamera du jeu web
VueCamera vueCamera();
void choisirVue(VueCamera v);
VueCamera vueSuivante();

// ---------- fonctions pures (vérifiées par tests/test_systems.cpp) ----------
inline VueCamera vueApres(VueCamera v) { return (VueCamera)(((int)v + 1) % VUE_NOMBRE); }
// lissage d'une image : a += (b − a) × min(1, dt × vitesse)  (wheel.rotation.z du jeu web)
inline float lisser(float a, float b, float dt, float vitesse) { return a + (b - a) * std::min(1.f, dt * vitesse); }
// angle visé du volant : −braquage × 2,3
inline float volantCible(double braquage) { return (float)(-braquage * 2.3); }
// aiguille du compteur : 2,2 rad à l'arrêt, −2,2 rad à 300 km/h et au-delà
inline float aiguilleAngle(double kmh) { return 2.2f - (float)(std::min(std::fabs(kmh), 300.0) / 300.0 * 4.4); }
// champ de vision visé des vues capot et cockpit : C.fov + min(p, 1,3) × 7 + 6 en nitro (p = |vitesse| / MAX_SPEED)
inline float fovVue(VueCamera v, double p, bool nitro) {
    return VUES[(int)v].fov + (float)std::min(std::fabs(p), 1.3) * 7.f + (nitro ? 6.f : 0.f);
}

// ---------- accroche de Game::render (WFS-66) ----------
// Appelée après la caméra de poursuite de Game (eye, look, fov déjà calculés). camMode 2 (vitrine) et écrans hors course
// et pause : rien ne change. Poursuite : eye jamais sous la route. Capot / cockpit : eye, look et fov remplacés.
// fdt : temps écoulé depuis l'image précédente (s, borné). Renvoie true si la voiture du joueur ne doit pas être dessinée.
bool cameraVue(Track& track, const Player& pl, const CarDef& car, const mat4& carM, int camMode, Ecran ecran, float fdt,
               vec3& eye, vec3& look, float& fov);

// état du cockpit de l'image en cours (lu par le système « camera » pour dessiner le cockpit)
// (volant et aiguille en radians, fov = champ de vision vertical de l'image, en degrés)
struct EtatCockpit { bool actif = false; float volant = 0, aiguille = 2.2f, fov = 58.f; };
const EtatCockpit& etatCockpit();
// centre du bouton de vue posé à la dernière image (banc d'essai : appui simulé) ; false s'il n'est pas affiché
bool boutonVue(float& cx, float& cy);

} // namespace wfs
