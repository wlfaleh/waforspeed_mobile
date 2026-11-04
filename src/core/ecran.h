// WaForSpeed — écrans de l'application (point d'extension WFS-36).
// Un seul écran est affiché à la fois ; `Game` le connaît et les systèmes le lisent dans `GameView::ecran`
// (équivalent de `state` du jeu web : 'menu' | 'garage' | 'count' | 'play' | 'pause' | 'over').
// Un système demande un autre écran par `GameView::allerA(e)` : le changement est appliqué entre deux images, puis tous
// les systèmes sont prévenus par `System::onEcran(ancien, nouveau, vue)`.
//   - hors `Course` : la conduite ne reçoit aucune commande, les commandes tactiles ne sont pas dessinées, la voiture reste
//     immobile, la scène continue d'être dessinée derrière les menus ;
//   - `Pause` : la simulation est figée (aucun `update`), la scène reste affichée.
#pragma once

namespace wfs {

enum class Ecran { Chargement, Menu, Garage, Terrain, Course, Pause, Fin, Salon };   // Salon : duel en ligne (WFS-83)

inline const char* nomEcran(Ecran e) {
    switch (e) {
        case Ecran::Chargement: return "chargement";
        case Ecran::Menu: return "menu";
        case Ecran::Garage: return "garage";
        case Ecran::Terrain: return "terrain";
        case Ecran::Salon: return "salon";
        case Ecran::Course: return "course";
        case Ecran::Pause: return "pause";
        case Ecran::Fin: return "fin";
    }
    return "?";
}

} // namespace wfs
