// WaForSpeed — garage : colonne d'action du panneau (WFS-534, sortie de garage.cpp sans changement) : bouton « Choisir »
// (« Chargement… » tant que le modèle 3D n'est pas prêt) et bouton « Retour ». Emplacement prêt pour l'achat : cadenas,
// prix, bouton « Acheter » à la place de « Choisir » pour une voiture ou une couleur verrouillée, message « Il te manque
// N ★ » (WFS-489, WFS-499, WFS-502). Aujourd'hui, rien n'est à acheter : seuls « Choisir » et « Retour » sont dessinés.
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage {

class AchatGarage : public PanneauGarage {
public:
    enum Bouton { AUCUN = 0, CHOISIR = 1, RETOUR = 2 };   // WFS-115 : mêmes numéros que le bouton appuyé de garage.cpp
    void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) override;   // zones g_z.choisir et g_z.retour
    int appui(float x, float y) const override;   // Bouton touché (CHOISIR, RETOUR), -1 si aucun
    void viderZones() override;
};

} // namespace wfs::ecrans::garage
