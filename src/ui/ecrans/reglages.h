// WaForSpeed — écran de réglages (WFS-68, absorbé par WFS-41, épic E8 « Écrans et menus »).
// Panneau « Réglages » ouvert depuis le menu principal et depuis la pause (bouton en haut à droite, dans la zone sûre),
// comme la section #pause .settings du jeu web (renderSettings, #setCam, #setGfx, #setSnd) :
//   - Caméra : Poursuite / Capot / Cockpit (vues de WFS-66, choisirVue) ;
//   - Graphismes : Basse / Moyenne / Haute (échelle de la scène 3D et ombres du soleil) ;
//   - Son : Activé / Coupé (setMuted du web : la sortie audio passe à zéro).
// Les choix s'appliquent tout de suite et sont enregistrés (fichier texte simple), relus au démarrage.
//   - Commandes : Droitier / Gaucher (WFS-75 : miroir horizontal des commandes tactiles, miroirX ci-dessous) ;
//   - Vibrations : Activées / Coupées (WFS-75 : vibrer() de WFS-70 n'envoie plus rien, « vibration ignorée (réglage) »).
#pragma once
#include <atomic>
#include <string>
#include "../../core/system.h"   // Marges

namespace wfs {
class Game;
class Renderer;
namespace ecrans {

enum { CAM_POURSUITE = 0, CAM_CAPOT = 1, CAM_COCKPIT = 2 };   // = VueCamera (src/game/camera_vue.h)
enum { QUALITE_BASSE = 0, QUALITE_MOYENNE = 1, QUALITE_HAUTE = 2 };

struct Reglages {
    int camera = CAM_POURSUITE;
    int qualite = QUALITE_HAUTE;
    bool qualiteChoisie = false;   // WFS-15 : niveau choisi par le joueur (lu ou touché) → pas de rétrogradation automatique
    std::atomic<bool> son{true};   // lu par le thread audio (src/audio/audio_system.cpp)
    std::atomic<bool> musique{true};   // WFS-76 : musique seule (l'ambiance reste)
    bool gaucher = false;      // WFS-75 : commandes en miroir (volant à droite, pédales et nitro à gauche)
    bool vibrations = true;    // WFS-75 : vibrations du téléphone (motifs de WFS-70)
};
Reglages& reglages();

// WFS-75 : miroir des commandes tactiles, seul point de bascule, appliqué au dessin comme au toucher (game.cpp,
// direction.cpp). Les positions sont calculées pour un droitier avec margesCommandes(), puis x passe par miroirX.
inline float miroirX(float x, float W) { return reglages().gaucher ? W - x : x; }
// marges de sécurité (WFS-51) vues par les commandes : gauche et droite échangées pour un gaucher
inline Marges margesCommandes(const Marges& m) { return reglages().gaucher ? Marges{m.droite, m.haut, m.gauche, m.bas} : m; }
void appliquerQualiteNiveau(Renderer& r);   // WFS-15 : applique reglages().qualite (qualité automatique)

// panneau ouvert : le menu et la pause ne prennent plus les touchers
bool reglagesOuverts();
// toucher pour les réglages (panneau ouvert, ou bouton « Réglages » visé) : le menu et la pause le laissent passer
bool toucheReglages(float x, float y);

// banc d'essai : reglages-menu, reglages-pause, reglages-basse (qualité basse en course)
void benchReglages(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans
} // namespace wfs
