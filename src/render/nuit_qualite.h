// WFS-421 : ce que la qualité graphique coupe dans l'éclairage de nuit (lecture seule de la qualité).
// Fonctions pures, sans état : testées par tests/test_nuit_qualite.cpp.
// RG-01 (suite, ordre de l'utilisateur du 2026-11-11) : le disque additif au sol des lampadaires (r.haloSol) est supprimé à toutes
// les qualités, dans nuit.cpp : grand cercle orange à bord net, forme nue interdite (règle du 2026-11-09) ; la vraie lumière des
// projecteurs éclaire la rue seule. Avant (1re version de WFS-421), il n'était coupé qu'en Basse.
#pragma once
#include "../ui/ecrans/reglages.h"

namespace wfs::nuit_qualite {

// RG-02 : le petit halo de la tête de lampe est gardé à toutes les qualités.
constexpr bool haloTeteLampadaire(int) { return true; }

}  // namespace wfs::nuit_qualite
