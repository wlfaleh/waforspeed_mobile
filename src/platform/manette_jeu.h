// WaForSpeed — manettes de jeu (WFS-22 / WFS-23) : branchement sur la partie (voir manette.h pour la correspondance).
#pragma once
#include <string>
#if defined(__ANDROID__)
struct AInputEvent;
#endif

namespace wfs {
class Game;
class Renderer;
namespace manette {

// une fois par image, avant Game::update : commandes de conduite (Game::manette), vue, pause, focus des menus,
// pause automatique si la manette part en course (RG-05)
void appliquer(Game& g);
int focusCourant();   // bouton qui a le focus dans les zones de l'image (−1 : aucun)

// banc d'essai (WFS_BANC=manette) : course pilotée par des entrées de manette injectées, focus des menus, pause
void benchManette(Game& g, Renderer& r, const std::string& out);

#if defined(__ANDROID__)
// manette_android.cpp : événement de manette → état (true : gardé, ne va pas au tactile) ; un toucher de l'écran rend la
// main au tactile ; verifierDeconnexion : manette retirée (JeuActivity), à appeler à chaque image
bool evenementAndroid(const AInputEvent* e);
void verifierDeconnexion();
#endif

} // namespace manette
} // namespace wfs
