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
#include <functional>
#include <vector>
#include "renderer.h"

namespace wfs::precalcul {

// paramètres (réglés au banc ; test fidelite_lumiere_precalcule)
constexpr float VOXEL_M = 0.5f;          // côté d'un voxel de la grille d'occupation (m) ; 0,75 : bande sombre au pied des murs (la cellule du mur englobe le sol voisin)
constexpr float PORTEE_M = 40.0f;        // distance de recherche des obstacles (m) : au-delà, le ciel est vu ; 40 m : la rue entière (28 m de façade à façade)
                                         // (immeuble d'en face : bas des façades plus sombre que le haut) ; 5 m ne voyait que les avancées : effet
                                         // invisible en Ville (revue du lead dev) ; 20 m n'atteignait pas l'autre côté de la rue
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

// calcule l'occlusion de tous les sommets des lots « recoit » (en parallèle, jobs()) ; rend le bilan (journal, RG-04).
// garder : la grille du tour reste en mémoire pour les murs des bâtiments du kit (WFS-161), construits à la volée
// en course : leur occlusion est calculée juste après, au chargement (occlusionKit), puis la grille est rendue
Bilan occlusionDecor(std::vector<Lot>& lots, bool garder = false);

// bâtiments du kit (WFS-161), construits à la volée en course : sommets des murs du bâtiment k au niveau lod (0 ou 1), dans le monde
using SommetsKit = std::function<void(size_t k, int lod, std::vector<Vertex>& v)>;
// au chargement : occlusion des murs de n bâtiments (niveaux 0 et 1) sur la grille gardée, valeurs gardées par bâtiment,
// puis la grille est rendue ; sans grille gardée : les valeurs d'une carte précédente sont oubliées, rien n'est calculé
Bilan occlusionKit(size_t n, const SommetsKit& sommets, bool garderGrille = false);
// WFS-418 (accroche) : garderGrille = vrai : la grille reste pour un 2e appel (bâtiments cuits, puis kit de secours si la mémoire
// graphique manque) ; à rendre ensuite par rendreGrille()
void rendreGrille();
// en course (construction d'un morceau) : normales de v[debut..] × occlusion gardée du bâtiment k (rien si absente ou si le
// nombre de sommets diffère)
void appliquerKit(size_t k, int lod, std::vector<Vertex>& v, size_t debut);

// occlusion lue dans un sommet (longueur de la normale), pour les tests
inline float occlusionSommet(const Vertex& s) { return std::sqrt(s.nx * s.nx + s.ny * s.ny + s.nz * s.nz); }

} // namespace wfs::precalcul
