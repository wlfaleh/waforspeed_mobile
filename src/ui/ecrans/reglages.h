// WaForSpeed — écran de réglages (WFS-68, absorbé par WFS-41, épic E8 « Écrans et menus »).
// Panneau « Réglages » ouvert depuis le menu principal et depuis la pause (bouton en haut à droite, dans la zone sûre),
// comme la section #pause .settings du jeu web (renderSettings, #setCam, #setGfx, #setSnd) :
//   - Caméra : Proche / Loin / Cockpit (camMode du web : 0, 1, 2 ; natif : Game::camMode 0, 1, 3) ;
//   - Graphismes : Basse / Moyenne / Haute (échelle de la scène 3D et ombres du soleil) ;
//   - Son : Activé / Coupé (setMuted du web : la sortie audio passe à zéro).
// Les choix s'appliquent tout de suite et sont enregistrés (fichier texte simple), relus au démarrage.
// Limites : « Vibrations » masquée (pas de vibreur côté Android) ; disposition gaucher des commandes : à venir (ticket E0).
#pragma once
#include <atomic>
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {

enum { CAM_PROCHE = 0, CAM_LOIN = 1, CAM_COCKPIT = 2 };
enum { QUALITE_BASSE = 0, QUALITE_MOYENNE = 1, QUALITE_HAUTE = 2 };

struct Reglages {
    int camera = CAM_PROCHE;
    int qualite = QUALITE_HAUTE;
    std::atomic<bool> son{true};   // lu par le thread audio (src/audio/audio_system.cpp)
};
Reglages& reglages();

// panneau ouvert : le menu et la pause ne prennent plus les touchers
bool reglagesOuverts();
// toucher pour les réglages (panneau ouvert, ou bouton « Réglages » visé) : le menu et la pause le laissent passer
bool toucheReglages(float x, float y);

// banc d'essai : reglages-menu, reglages-pause, reglages-basse (qualité basse en course)
void benchReglages(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans
} // namespace wfs
