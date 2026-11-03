// WaForSpeed — zones des cartes (ticket WFS-14, épic E2) : port de ZONES et du découpage de generateDecor (js/world/decor.js).
// Une zone dure ZONE_LEN tronçons (≈ 540 m) ; la dernière, trop courte, prolonge la précédente (comme le jeu web).
// Le nom de la zone est exposé ici pour l'interface (affichage : épic E5).
#pragma once
#include <cstdint>

namespace wfs {

struct MapDef;
constexpr int64_t ZONE_LEN = 448;

// indice de la zone du tronçon a (0 .. nombre de thèmes - 1), identifiant (« corniche »…) et nom affiché (« Corniche »…)
int zoneIndex(const MapDef& map, int64_t a, int64_t segCount);
const char* zoneId(const MapDef& map, int64_t a, int64_t segCount);
const char* zoneName(const MapDef& map, int64_t a, int64_t segCount);

} // namespace wfs
