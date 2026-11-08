#pragma once
// WaForSpeed — WFS-269 (A, arbitrage du PO du 2026-11-08) : éclairage du décor fixe précalculé au chargement.
// Occlusion ambiante par sommet : part du ciel vue depuis chaque sommet du décor fixe des blocs (WFS-268), calculée une fois
// derrière l'écran de chargement sur une grille de voxels de tout le décor du tour, puis rangée dans la LONGUEUR de la normale
// du sommet (comme la luminosité de la végétation, MAT_VTINT) : aucun octet de plus par sommet, aucun calcul en course.
// Lue par scene.frag pour les matériaux marqués MAT_AO : ambiance du ciel et reflet de l'environnement × occlusion
// (pied des murs, dessous des auvents, entre les immeubles). Le soleil direct, ses ombres, les lampes et les phares restent
// calculés à chaque image (RG-03) ; voitures et personnages ne sont pas concernés (aucun MAT_AO).
#include <cmath>
#include <cstdint>
#include <vector>
#include "renderer.h"

namespace wfs::precalcul {

// paramètres (réglés au banc ; test fidelite_lumiere_precalcule)
constexpr float VOXEL_M = 0.5f;          // côté d'un voxel de la grille d'occupation (m) ; 0,75 : bande sombre au pied des murs (la cellule du mur englobe le sol voisin)
constexpr float PORTEE_M = 5.0f;         // distance de recherche des obstacles (m) : au-delà, le ciel est vu
constexpr float DECALAGE_M = 0.55f;      // départ des rayons au-dessus de la surface (> 1 voxel : la surface ne s'occulte pas elle-même)
constexpr int RAYONS = 8;                // directions par sommet, réparties en cosinus dans l'hémisphère de la normale
constexpr float OCCLUSION_MIN = 0.25f;   // plancher : jamais noir (lumière rebondie que la grille ne voit pas)

// un lot de géométrie : sommets (modifiés si recoit) et indices d'un matériau d'un morceau
struct Lot {
    std::vector<Vertex>* v = nullptr;
    const std::vector<uint32_t>* i = nullptr;
    bool occulte = true;   // ses triangles cachent le ciel (bâtiments, murs, arbres, auvents…)
    bool recoit = false;   // ses sommets reçoivent l'occlusion (longueur de la normale MULTIPLIÉE : luminosité MAT_VTINT gardée)
    float force = 1;       // 1 : occlusion entière ; < 1 : atténuée (matériau MAT_VTINT, où elle touche aussi le soleil direct)
};

struct Bilan { size_t voxels = 0, sommets = 0; double ms = 0, msGrille = 0, moyenne = 1, memoireMo = 0; };

// calcule l'occlusion de tous les sommets des lots « recoit » (en parallèle, jobs()) ; rend le bilan (journal, RG-04)
Bilan occlusionDecor(std::vector<Lot>& lots);

// occlusion lue dans un sommet (longueur de la normale), pour les tests
inline float occlusionSommet(const Vertex& s) { return std::sqrt(s.nx * s.nx + s.ny * s.ny + s.nz * s.nz); }

} // namespace wfs::precalcul
