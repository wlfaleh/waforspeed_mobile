// WFS-421 : ce que la qualité graphique coupe dans l'éclairage de nuit (lecture seule de la qualité).
// Fonctions pures, sans état : testées par tests/test_nuit_qualite.cpp.
#pragma once
#include "../ui/ecrans/reglages.h"

namespace wfs::nuit_qualite {

// RG-01 : en Basse, plus de halo au sol (disque additif) au pied des lampadaires. Il s'ajoutait à la vraie lumière des
// projecteurs (nappe orange vue par l'utilisateur, forme nue interdite le 2026-11-09) et coûtait ≈ 14 ms de scène en Ville
// sur le S21 FE (interrupteur « sans halos additifs » de WFS-419, 62 → 47-48 ms). Moyenne et Haute ne changent pas.
constexpr bool haloSolLampadaire(int qualite) { return qualite != ecrans::QUALITE_BASSE; }

// RG-02 : le petit halo de la tête de lampe est gardé à toutes les qualités.
constexpr bool haloTeteLampadaire(int) { return true; }

}  // namespace wfs::nuit_qualite
