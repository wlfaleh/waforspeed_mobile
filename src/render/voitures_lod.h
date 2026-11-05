// WaForSpeed — versions allégées des voitures (WFS-134, RG-11 : « jamais de voiture en cubes »).
// Modèles « -lod » du jeu web (waforspeed/models/<voiture>/<voiture>-lod.json, ≈ 12 000 triangles, 4 à 7 matériaux),
// convertis en assets/cars/<id>-lod.wfm par tools/convert_car.mjs (silhouette « wfs_far » retirée, WFS_EXCLURE).
// Règle : vrai modèle complet jusqu'à 60 m, version allégée de 60 à 150 m, silhouette en cubes seulement au-delà de 150 m.
// Chargés à la première demande, libérés avec la partie (système enfichable, GPU détruit dans shutdown).
#pragma once
#include "renderer.h"

namespace wfs::voitures_lod {

constexpr double COMPLET_M = 60;    // en deçà : modèle complet (versions -lod interdites, RG-04)
constexpr double CUBES_M = 150;     // au-delà seulement : silhouette en cubes permise (< 10 pixels de haut)

// version allégée de la voiture `id` (golf, m4, p911, gallardo, p208, x5), nullptr si absente ; charge à la demande
const Model* modele(Renderer& r, const char* id);

} // namespace wfs::voitures_lod
