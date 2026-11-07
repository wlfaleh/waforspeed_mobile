// WaForSpeed — aide à la direction du joueur (ticket WFS-217, épic E3) : pose sur la voiture du joueur le choix de la ligne
// « Aide à la direction » des Réglages (reglagesConduite(), Activée par défaut). Seule la voiture du joueur reçoit ce choix :
// les autres Player (ordinateur, références du jeu web : simulateCourse) gardent Aide::Web (RG-05).
#include "../core/system.h"
#include "player.h"

namespace wfs {

class AideDirection : public System {
public:
    const char* name() const override { return "aide-direction"; }
    // appelé à chaque pas de 1/120 s avec la voiture du joueur (Game::update) : le choix s'applique tout de suite
    void collide(double, Player& pl, const GameView&) override {
        pl.aide = reglagesConduite().aideDirection ? Aide::Activee : Aide::Desactivee;
    }
};

} // namespace wfs

WFS_SYSTEM(wfs::AideDirection, 202)   // gameplay (200-299)
