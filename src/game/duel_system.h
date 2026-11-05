// WaForSpeed — module « duel » (WFS-21) branché sur le moteur par les systèmes enfichables (WFS-30).
// Règles et protocole : duel.h. Ce module démarre la manche (graine commune du trafic, course jusqu'à FINISH),
// envoie la présence, applique les contacts, dessine les adversaires avec leur nom, le classement en direct et
// l'écran de résultat (« Revanche » / « Menu »).
#pragma once
#include <string>
#include "duel.h"

namespace wfs {

class Game; class Renderer;

duel::Session& duelSession();   // session du joueur (un seul salon à la fois)
// scènes du banc d'essai (captures duel-course.png, duel-resultat.png), appelées par src/platform/desktop_main.cpp :
// salon en mémoire avec un adversaire simulé qui rejoue des messages de présence
void benchDuelScene(Game& g, Renderer& r, const std::string& outDir);
// WFS-112 (banc du duel entre amis) : arrivée forcée dès le départ ; centre des boutons « revanche » et « retour » de
// l'écran de fin (faux tant qu'il n'est pas dessiné)
void duelArriveeForcee(bool oui);
bool duelBouton(const char* nom, float& x, float& y);

} // namespace wfs
