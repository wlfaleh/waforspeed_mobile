#pragma once
// WaForSpeed — WFS-370 (épic E15, file Ville) : détails d'architecture du kit, au niveau du vrai bâtiment.
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : plus aucune forme géométrique nue. Les immeubles du kit (WFS-161) avaient
// encore des volumes « boîte » : toit de zinc à deux pans plats, corniche en pavé unique, garde-corps en plaque pleine,
// édicule de toit en cube, façade sans soubassement ni cordon. Ce fichier les remplace par les volumes du vrai immeuble :
// comble à la Mansart (brisis, terrasson, lucarnes, souches de cheminée), corniche moulurée, garde-corps à barreaux,
// soubassement et cordon de pierre, dormant des vitrines, couvertines de l'acrotère, édicule avec dalle, porte et ventilation.
// Aucun Vulkan : testé par tests/archi_geo.cpp (même repère que archi_geo.h : x le long de la façade, y vers le haut,
// z sortant de la façade, le bâtiment s'étend vers z < 0).
#include "archi_geo.h"

namespace wfs::archi::details {

// garde-corps (NF P01-012 : 1,00 m, vide entre barreaux ≤ 0,11 m) de x0 à x1, plancher à y, du mur (z0) au nu avant (z1) :
// main courante, lisse basse, barreaux ; retours sur les deux côtés si z1 > z0 + 0,05 (balcon)
void gardeCorps(Maillage& m, Maillage& fins, float x0, float x1, float y, float z0, float z1);   // barreaux dans « fins » (pièce FIN)
// corniche moulurée (cordon, doucine en trois ressauts, larmier, couronnement) de hauteur h, saillie s, haut à H, sur 3 côtés
void corniche(Maillage& m, float L, float D, float H, float h, float s);
// comble à la Mansart du classique (zinc) : brisis à 70°, terrasson à 20°, croupes, lucarnes (niveau 0), souches de cheminée
void mansart(const Plan& p, float D, int lod, Batiment& out);
// toiture-terrasse : couvertines de l'acrotère et édicule (cage d'escalier) avec dalle en débord, porte et ventilation
void terrasse(const Plan& p, float D, int lod, Batiment& out);
// façade (niveau 0) : soubassement et cordon (classique), dormant et meneaux des vitrines (tous styles)
void facade(const Plan& p, Batiment& out);

}  // namespace wfs::archi::details
