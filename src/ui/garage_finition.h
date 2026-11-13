// WaForSpeed — garage : panneau « Finition » (WFS-534, RG-01 et RG-03) : emplacement VIDE, prêt pour WFS-491 (choix de la finition, prix, aperçu).
// Il ne dessine rien et ne pose aucune zone d'appui : l'écran du garage ne change pas tant que son ticket n'est pas fait.
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage {

class FinitionGarage : public PanneauGarage {
public:
    void dessiner(Renderer&, const MiseEnPage&, const EtatGarage&) override {}
    int appui(float, float) const override { return -1; }
    void viderZones() override {}
};

} // namespace wfs::ecrans::garage
