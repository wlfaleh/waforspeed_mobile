// WaForSpeed — garage : décor 3D de la scène dédiée (WFS-534, sorti de garage.cpp sans changement) : sol, plateau
// tournant, anneau néon, 9 barres lumineuses, hémisphère et 4 projecteurs (WFS-44, WFS-45, WFS-99). Les formes nues
// (disque, tore, boîte) relèvent des tickets « formes géométriques » (hors WFS-534, RG-03).
#pragma once
#include "../game/cars.h"
#include "../render/renderer.h"

namespace wfs {
struct GameView;
struct SceneDediee;
namespace ecrans::garage {

class DecorGarage {
public:
    void init(Renderer& r);
    void shutdown(Renderer& r);
    // scène dédiée (WFS-44) : caméra cadrée sur la voiture `car`, fond uni, brouillard, lumières
    void regler(FrameParams& fp, SceneDediee& s, const GameView& g, const CarDef& car) const;
    // sol, plateau, anneau de la couleur néon `glow` (0xRRGGBB) et barres lumineuses
    void dessiner(Renderer& r, unsigned glow);

private:
    Mesh sol_, plateau_, anneau_, barre_;
    Material matSol_, matPlateau_, matNeon_[3], matAnneau_;
};

} // namespace ecrans::garage
} // namespace wfs
