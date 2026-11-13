// WaForSpeed — garage : cadre de l'écran (WFS-534, sorti de garage.cpp sans changement) : voiles sombres, flèches
// .show-arrow (zones g_z.prev / g_z.next), fond vitré du panneau du bas, crédit du modèle et « Chargement du modèle 3D… ».
#pragma once
#include "garage_commun.h"

namespace wfs::ecrans::garage::cadre {

void voiles(Renderer& r, const MiseEnPage& mp);                       // voile du bas et voile de gauche
void fleches(Renderer& r, const MiseEnPage& mp);                      // voiture précédente / suivante
void fondPanneau(Renderer& r, const MiseEnPage& mp);                  // .show-panel : verre teinté, coin coupé
void pied(Renderer& r, const MiseEnPage& mp, const CarDef& car, bool voiturePrete);   // crédit ; chargement en cours

} // namespace wfs::ecrans::garage::cadre
