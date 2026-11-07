// WaForSpeed — caméras de course : vues poursuite, capot et cockpit (ticket WFS-66, épic E3).
// Port de js/world/scene.js du jeu web : CAM_MODES, cycleCamera, updateCamera (suivi, champ de vision) et buildCockpit
// (tableau de bord, montants, volant et compteur dessinés par code, attachés à la caméra).
//
// - Poursuite (vue par défaut) et Proche (WFS-85) : updateCamera du jeu web ; l'œil suit le cap de la voiture avec retard
//   (camHd, WFS-98), recule en accélérant (camDistDyn), jamais sous la route ; demi-tour : recul ≥ 6 m, œil à 3 m au moins
//   et regard relevé (voiture dans le tiers bas).
// - Capot : œil au-dessus du capot, lié à la carrosserie, champ de vision 72° (hood du jeu web).
// - Cockpit : œil du conducteur (à gauche), voiture du joueur cachée, tableau de bord, montants de pare-brise, volant qui
//   tourne de −braquage × 2,3 (lissé) et aiguille de 0 à 300 km/h (2,2 − v / 300 × 4,4 rad) ; WFS-235 : champ de vision
//   horizontal de 85° (vertical selon l'écran), regard un peu plongeant (réglage « Regard cockpit ») qui suit le virage,
//   mouvement de tête doux (sauf « Réduire les mouvements »).
// La vue choisie est mémorisée (elle survit à Game::init : nouvelle course, changement de voiture) ; l'écran des
// réglages (WFS-41) et la sauvegarde (WFS-24) passent par vueCamera() / choisirVue().
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "../core/ecran.h"
#include "../core/math.h"
#include "../core/configgame.h"   // WFS-128

namespace wfs {

class Track;
struct Player;
struct CarDef;

// WFS-85 : Proche ajoutée à la fin (les indices 0 à 2 des sauvegardes et des réglages restent valables)
enum class VueCamera { Poursuite = 0, Capot = 1, Cockpit = 2, Proche = 3 };
constexpr int VUE_NOMBRE = 4;

// CAM_MODES du jeu web : 'proche' (poursuite, 58°), vue capot (hood), 'capot' / « Cockpit » (cockpit: true, 72°)
// champ de vision de chaque vue : configgame.json (camera.*.zoom, ConfigJeu::Camera::fov, même ordre), lu par fovVue
struct VueDef { const char* id; const char* nom; bool capot; bool cockpit; };
inline constexpr VueDef VUES[VUE_NOMBRE] = {
    {"poursuite", "Arrière", false, false},   // WFS-135 (ANO-58) : affichée « Arrière », identifiant enregistré inchangé ; WFS-85 : vue « loin » du jeu web (62°) ; WFS-105 : caméra haute, 66°
    {"capot", "Capot", true, false},
    {"cockpit", "Cockpit", true, true},
    {"proche", "Proche", false, false},         // WFS-85 : « proche » du jeu web (58°) ; WFS-105 : un peu plus large, 60°
};

// WFS-85 : caméras de poursuite (CAM_MODES du jeu web, js/world/scene.js) : distance derrière la voiture, hauteur, regard
// devant et hauteur du point visé (m). Constantes à ajuster après un essai sur téléphone (RG-05).
// WFS-105 : caméras relevées à la demande de l'utilisateur (jeu web : 8 m × 2,5 m et 4,6 m × 1,28 m, ≈ 17° et 15°).
//   Poursuite : 9 m derrière, 6,3 m de haut, soit 35° d'élévation à l'arrêt et ≈ 28° en course (la caméra recule avec la
//   vitesse). 40° est écarté : l'horizon sortirait par le haut de l'image (RG-01).
//   Proche : 6 m derrière, 2,67 m de haut (24°) : la rue se voit par-dessus le toit.
//   Point visé SOUS la route (lookH < 0) : le regard reste tourné vers le virage qui arrive (ahead), mais la caméra
//   s'incline vers le bas ; l'arrière de la voiture reste ainsi au-dessus du compteur (72 % de la hauteur), même à l'arrêt.
// WFS-128 : valeurs de configgame.json (section camera) ; défauts du code dans ConfigJeu::Camera (src/core/configgame.h)
using Poursuite = CamPoursuite;   // { dist, h, ahead, lookH }
inline const Poursuite &POURSUITE_LOIN = g_configJeu.camera.loin, &POURSUITE_PROCHE = g_configJeu.camera.proche;
inline bool g_cameraFiltreExact = true;   // WFS-221 : banc « fluidite » seulement : false = anciennes formules de lissage (mesure « avant »)
inline const float &SUIVI_OEIL = g_configJeu.camera.suiviOeil, &SUIVI_REGARD = g_configJeu.camera.suiviRegard;   // lissage du suivi (/s), comme updateCamera du jeu web

// vue choisie (mémorisée pour toute la session) ; vueSuivante = cycleCamera du jeu web
VueCamera vueCamera();
void choisirVue(VueCamera v);
VueCamera vueSuivante();

// ---------- fonctions pures (vérifiées par tests/test_systems.cpp) ----------
// cycle : Poursuite → Proche → Capot → Cockpit → Poursuite (WFS-85)
inline VueCamera vueApres(VueCamera v) {
    switch (v) { case VueCamera::Poursuite: return VueCamera::Proche; case VueCamera::Proche: return VueCamera::Capot;
                 case VueCamera::Capot: return VueCamera::Cockpit; default: return VueCamera::Poursuite; }
}
// lissage d'une image : a += (b − a) × min(1, dt × vitesse)  (wheel.rotation.z du jeu web)
inline float lisser(float a, float b, float dt, float vitesse) { return a + (b - a) * std::min(1.f, dt * vitesse); }
// angle visé du volant : −braquage × 2,3
inline float volantCible(double braquage) { return (float)(-braquage * 2.3); }
// aiguille du compteur : 2,2 rad à l'arrêt, −2,2 rad à 300 km/h et au-delà
inline float aiguilleAngle(double kmh) { return 2.2f - (float)(std::min(std::fabs(kmh), 300.0) / 300.0 * 4.4); }
// champ de vision visé des vues capot et cockpit : C.fov + min(p, 1,3) × 7 + 6 en nitro (p = |vitesse| / MAX_SPEED)
inline float fovVue(VueCamera v, double p, bool nitro) {
    const auto& c = config().camera;   // WFS-128 : zoom de chaque vue et élargissements (vitesse, nitro) : configgame.json
    return c.fov[(int)v] + (float)std::min(std::fabs(p), 1.3) * c.elargVitesse + (nitro ? c.elargNitro : 0.f);
}

// ---------- WFS-235 : regard du conducteur en vue Cockpit (RG-02, RG-03, RG-04) ----------
// « Regard cockpit » (Réglages) : la caméra regarde un peu plus bas ou plus haut ; Normal par défaut ; enregistré par l'écran
// des réglages (clé regard du fichier des réglages, comme « Réduire les mouvements »)
enum class RegardCockpit { Bas = 0, Normal = 1, Haut = 2 };
RegardCockpit regardCockpit();
void choisirRegard(RegardCockpit r);
// champ de vision HORIZONTAL de la vue Cockpit (RG-02 : 75 à 90°, vision d'un conducteur) : 85° à l'arrêt, + 4° à pleine vitesse,
// + 2° en nitro, 90° au plus ; le champ vertical en découle selon la forme de l'écran (2400 × 1080 : ≈ 45°)
constexpr float COCKPIT_FOV_H = 85.f, COCKPIT_FOV_H_VITESSE = 4.f, COCKPIT_FOV_H_NITRO = 2.f, COCKPIT_FOV_H_MAX = 90.f;
inline float fovHorizontalCockpit(double p, bool nitro) {
    return std::min(COCKPIT_FOV_H_MAX, COCKPIT_FOV_H + (float)std::min(std::fabs(p), 1.0) * COCKPIT_FOV_H_VITESSE + (nitro ? COCKPIT_FOV_H_NITRO : 0.f));
}
// champ vertical (°) d'un champ horizontal donné (°) sur un écran de rapport largeur / hauteur ; borné de 30 à 75°
inline float fovVerticalDepuisHorizontal(float horizontalDeg, float aspect) {
    const float a = std::max(0.3f, aspect), h = horizontalDeg * 3.14159265f / 180.f;
    return std::clamp(2.f * std::atan(std::tan(h * 0.5f) / a) * 180.f / 3.14159265f, 30.f, 75.f);
}
// inclinaison du regard sous l'horizontale (rad, négatif = vers le bas) : un conducteur regarde la route à 20-40 m, quelques
// degrés sous l'horizon (RG-02 « légèrement plongeant ») ; Bas −7°, Normal −3,5°, Haut 0°
inline float inclinaisonRegard(RegardCockpit r) {
    return r == RegardCockpit::Bas ? -7.f * 3.14159265f / 180.f : r == RegardCockpit::Haut ? 0.f : -3.5f * 3.14159265f / 180.f;
}
// regard qui suit le virage : la tête tourne vers le point de la route visé à ≈ 1,5 s (12 à 40 m), à 60 % de son angle,
// ±12° au plus (la tête d'un conducteur suit le virage sans regarder tout le point de corde) ; angle en rad (+ = à droite)
constexpr float COCKPIT_SUIVI_VIRAGE = 0.6f, COCKPIT_LACET_MAX = 12.f * 3.14159265f / 180.f;
inline float distanceRegard(double kmh) { return (float)std::clamp(std::fabs(kmh) / 3.6 * 1.5, 12.0, 40.0); }
inline float lacetVirage(float angleVersRoute) { return std::clamp(angleVersRoute * COCKPIT_SUIVI_VIRAGE, -COCKPIT_LACET_MAX, COCKPIT_LACET_MAX); }
// RG-04 : mouvement de tête (m, repère de la voiture : x à droite, z vers l'arrière) : vers l'avant au freinage, vers
// l'arrière à l'accélération (3 mm par m/s², 3 cm au plus), vers l'extérieur du virage (2 mm par m/s², 2 cm au plus)
inline vec3 mouvementTete(double lonAcc, double latAcc) {
    return {(float)std::clamp(-latAcc * 0.002, -0.02, 0.02), 0.f, (float)std::clamp(lonAcc * 0.003, -0.03, 0.03)};
}

// ---------- accroche de Game::render (WFS-66) ----------
// Appelée après la caméra de poursuite de Game (eye, look, fov déjà calculés). camMode 2 (vitrine) et écrans hors course
// et pause : rien ne change. Poursuite et proche : eye, look et fov remplacés (updateCamera du jeu web, WFS-85 / WFS-98) ;
// vue haute du banc (camMode 1) : eye jamais sous la route. Capot / cockpit : eye, look et fov remplacés.
// fdt : temps écoulé depuis l'image précédente (s, borné). Renvoie true si la voiture du joueur ne doit pas être dessinée.
bool cameraVue(Track& track, const Player& pl, const CarDef& car, const mat4& carM, int camMode, Ecran ecran, float fdt,
               vec3& eye, vec3& look, float& fov);

// état du cockpit de l'image en cours (lu par le système « camera » pour dessiner le cockpit)
// (volant et aiguille en radians, fov = champ de vision vertical de l'image, en degrés)
// WFS-235 : repere = repère de l'intérieur (monde ← œil au repos, attaché à la voiture : la tête tourne et bouge dedans)
struct EtatCockpit { bool actif = false; float volant = 0, aiguille = 2.2f, fov = 58.f; mat4 repere = mat4::identity(); };
const EtatCockpit& etatCockpit();
// WFS-197 : banc d'essai des intérieurs par modèle (captures cockpit-*.png dans outDir), puis g.shutdown()
class Game; class Renderer;
// WFS-201 : `voitures` : liste des voitures (lot 1 au job partie2c, lot 2 réparti entre les jobs nuit et partie1a, lot 3 entre partie1b et partie2b ; cockpit : toutes)
void benchCockpit(Game& g, Renderer& r, const std::string& outDir, const std::vector<std::string>& voitures);
// voitures des scènes du banc : lot 1 et la Corsa (cockpit commun), puis le lot 2 en deux moitiés
inline const std::vector<std::string> COCKPIT_LOT1{"golf", "m4", "p911", "sf90", "corsa"}, COCKPIT_LOT2A{"a45", "s3", "giulia", "p208"},
                                      COCKPIT_LOT2B{"gallardo", "mustang", "model3", "x5"},
                                      COCKPIT_LOT3A{"abarth", "compass", "elantra", "gt86"}, COCKPIT_LOT3B{"rrsport", "tcross", "v60"};   // WFS-202 (Corsa : lot 1)
// centre du bouton de vue posé à la dernière image (banc d'essai : appui simulé) ; false s'il n'est pas affiché
bool boutonVue(float& cx, float& cy);

} // namespace wfs
