// WaForSpeed — garage : panneau « Skin » (WFS-534, RG-01 et RG-03) : emplacement VIDE, prêt pour WFS-494 (skins, skin offerte par voiture, prix).
// Il ne dessine rien et ne pose aucune zone d'appui : l'écran du garage ne change pas tant que son ticket n'est pas fait.
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage {

class SkinGarage : public PanneauGarage {
public:
    void dessiner(Renderer&, const MiseEnPage&, const EtatGarage&) override {}
    int appui(float, float) const override { return -1; }
    void viderZones() override {}
};

} // namespace wfs::ecrans::garage
