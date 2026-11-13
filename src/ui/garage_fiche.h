// WaForSpeed — garage : fiche de la voiture (WFS-534, sortie de garage.cpp sans changement) : en-tête .show-head (marque ·
// place, nom, type, accroche, points des voitures), caractéristiques .specs et barres .statlist du panneau du bas.
// Textes de la fiche (FR / EN) : WFS-507.
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage {

class FicheGarage : public PanneauGarage {
public:
    // en-tête en haut à gauche et points des voitures permises (zones g_z.points)
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;
    // place (parmi les voitures permises) du point touché, -1 si aucun
    int appui(float x, float y) const override;
    void viderZones() override;
    // .specs (Puissance, 0 à 100, Vitesse max) en haut du panneau ; renvoie l'abscisse où commencent les barres
    float specs(Renderer& r, const MiseEnPage& mp, const CarDef& car) const;
    // .statlist : 4 barres sur 10, de `x` jusqu'à la colonne des boutons
    void stats(Renderer& r, const MiseEnPage& mp, const CarDef& car, float x) const;
};

} // namespace wfs::ecrans::garage
