// WaForSpeed — garage : rangée des couleurs (WFS-534, sortie de garage.cpp sans changement) : .paint-line « Couleur »,
// 16 pastilles inclinées et nom de la peinture. Ticket suivant : WFS-489 (28 couleurs, 2e rangée « toit et rétros »).
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage {

class CouleurGarage : public PanneauGarage {
public:
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.peintures
    int appui(float x, float y) const override;   // indice dans PAINTS de la pastille touchée, -1 si aucune
    void viderZones() override;
};

} // namespace wfs::ecrans::garage
