// WaForSpeed — système de test du point d'extension « scène dédiée » (WFS-44), pour le banc d'essai.
// Inactif par défaut (aucun effet sur le jeu) ; activerSceneEssai(true) lui fait prendre la scène :
// fond uni #07070c, caméra FOV 36°, un cube de repère, rien d'autre (ni route, ni ville, ni trafic, ni voiture du joueur).
#pragma once

namespace wfs {
void activerSceneEssai(bool on);
}
