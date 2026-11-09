// WaForSpeed — choix du terrain et du moment (WFS-41, épic E8 « Écrans et menus »).
// Étape 2 du jeu web (section #mapSel, screens.js : renderMapUI, mapCard, mapArt, stepMap, pickMap) : carrousel des
// 4 cartes (nom, description, longueur ≈ km), index « 01 / 04 », boutons Jour / Nuit (nuit par défaut), « Retour » vers le
// menu et « Rouler » : la partie est reconstruite sur la carte choisie (Game::demandes), puis la course démarre.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
struct MapDef;
namespace ecrans {

// mapLengthKm du jeu web : longueur d'une carte, même tirage que buildTrack, sans construire la piste
double longueurKm(const MapDef& m);

// banc d'essai : centre du bouton « Rouler » posé à la dernière image (false si l'écran n'est pas affiché)
bool centreRouler(float& x, float& y);
void choisirCarte(const std::string& id, bool nuit);   // banc (WFS-307) : carte et moment affichés dans le choix du terrain
// banc d'essai : terrain-0 (ville), terrain-1 (circuit), terrain-rouler (course lancée sur le circuit)
void benchTerrain(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans
} // namespace wfs
