// WaForSpeed — WFS-170 : correctifs des modèles de voitures appliqués au chargement (aucun .wfm modifié), pour les défauts
// relevés à la recette des 20 voitures (ANO-71 à 77) :
//  - RG-01 : couche de vernis séparée (matériau transparent sombre qui enveloppe toute la carrosserie, « coat » de la 911) :
//    retirée ; le vernis est déjà calculé par le shader sur la peinture (MAT_PAINT), la couleur choisie redevient visible ;
//    peinture de la 911 en peinture unie métallisée (métal 0,45, rugosité 0,32, comme les 16 voitures de WFS-77) ;
//  - RG-02, RG-03 : vitres à points (carte de normales de sérigraphie, Giulia) ou noires rugueuses (camouflage de la S3) :
//    verre teinté uni (MAT_GLASS) ;
//  - RG-03, RG-06 : jantes blanc uni (S3) ou noyées dans le noir (Compass) : métal gris ;
//  - RG-04 : feux arrière pris dans un matériau partagé blanc ou gris (V60, GT86, Abarth, A45 ; WFS-182 : les 11 autres sans
//    feu reconnu) : pièces « feu arrière »
//    (verre rouge, allumées au freinage, WFS-109) ;
//  - RG-05, RG-06 : carrosseries froissées (normales d'origine incohérentes : Model 3, X5, 208, Compass, et la bande claire
//    de la Giulia) : normales recalculées depuis la géométrie, arêtes de plus de 40° gardées vives.
// Chaque correctif est désigné par voiture (identifiant du fichier) et par matériau, après vérification sur les captures
// vitrine-<id>-0 et vitrine-<id>-feux : jamais de règle générale qui changerait les 15 voitures déjà bonnes (RG-07).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"
#include "feux_modele.h"
#include "renderer.h"

namespace wfs::correctifs {

// matériau tel qu'il est lu dans le .wfm (tools/convert_car.mjs)
struct MatBrut { float col[4], metal, rough, em[3], cut; int32_t tC, tN, tMR; uint32_t flags; };

// « cars/p911.wfm » → « p911 »
std::string idDe(const std::string& chemin);

// RG-05, RG-06 : la carrosserie (matériaux MAT_PAINT) de cette voiture a des normales d'origine à recalculer
bool normalesARecalculer(const std::string& id);
// normales recalculées depuis la géométrie : pour chaque sommet, somme (pondérée par l'aire) des faces qui touchent sa
// position et font moins de 40° avec la face du sommet ; faces orientées comme les normales d'origine de leurs sommets
// (sens de l'extérieur gardé). Rend le nombre de sommets changés de plus de 10°.
uint32_t recalculerNormales(Vertex* V, uint32_t nv, const uint32_t* I, uint32_t ni);

// correctifs des matériaux (après lecture des matériaux et des boîtes de chaque matériau) ; retirer[m] = 1 : matériau non
// dessiné. Rend la liste des corrections faites, pour le journal de chargement.
std::vector<std::string> corrigerMateriaux(const std::string& id, std::vector<MatBrut>& m, std::vector<uint8_t>& retirer);

// RG-04 (ANO-76) : feux arrière pris dans un matériau partagé (réflecteur gris argenté de la V60, blanc de la GT86, de
// l'Abarth et de l'A45) : les triangles de ce matériau dans la boîte du bloc optique arrière forment une pièce « feu
// arrière » (verre rouge, allumée au freinage et la nuit, WFS-109), comme la découpe de la Golf.
struct ZoneFeu { const char* voiture; uint32_t materiau; float couleur; float xMin, yMin, yMax, zMin; };
inline const ZoneFeu ZONES_FEUX[] = {
    {"v60", 12, 0.80f, 0.45f, 0.70f, 1.15f, 1.85f},      // réflecteur gris des feux verticaux (« blanc argenté »)
    {"gt86", 6, 1.00f, 0.40f, 0.66f, 0.86f, 1.84f},      // fond blanc des feux, sous le verre fumé noir
    {"abarth", 0, 1.00f, 0.40f, 0.66f, 0.92f, 1.50f},    // atlas blanc des feux arrière
    {"a45", 12, 1.00f, 0.40f, 0.74f, 1.00f, 1.79f},      // fond blanc des feux (rose pâle allumés)
    {"a45", 15, 1.00f, 0.40f, 0.74f, 1.00f, 1.79f},
    // WFS-182 (RG-01, ANO-91 ; RG-03, reste d'ANO-76) : feux sans drapeau MAT_TAIL, ou matériau trop haut pour être pris
    // entier (lunette, catadioptres du bouclier) : verre ou fond du bloc optique, dans la boîte du feu vue de l'arrière
    {"gallardo", 6, 0.00f, 0.30f, 0.60f, 0.90f, 1.80f},  // verre fumé des feux (x 0,40-0,75, y 0,71-0,82)
    {"p208", 12, 0.00f, 0.45f, 0.80f, 1.00f, 1.60f},     // verre des feux « griffes » (la lunette est au-dessus)
    {"compass", 11, 0.00f, 0.45f, 0.90f, 1.20f, 1.60f},  // verre des feux (lunette au-dessus, catadioptres en bas)
    {"s3", 27, 0.00f, 0.30f, 0.70f, 1.00f, 1.60f},       // fond des feux (catadioptres du bouclier en bas)
    {"x5", 12, 0.00f, 0.30f, 0.90f, 1.40f, 1.60f},       // verre rouge des feux en L
    {"elantra", 17, 0.50f, 0.20f, 0.75f, 1.00f, 1.60f},  // verre du bandeau de feux (catadioptres en bas)
    // WFS-359 : « tcross » (Brezza) et « mustang » (Corvette C7) ont leurs feux arrière à part dans le .wfm (drapeau MAT_TAIL)
    {"sf90", 14, 1.00f, 0.00f, 0.30f, 1.00f, 1.60f},     // fines barres rouges des feux (haut et bas de la poupe)
    {"rrsport", 11, 0.00f, 0.30f, 1.00f, 1.30f, 1.60f},  // verre du bandeau (MAT_TAIL refusé : catadioptres plus bas)
};
// Part : ModelPart (firstIndex, indexCount, vertexOffset, material, feuArriere) ; pos(v) : position du sommet v ;
// couleur(m) : couleur de base (gris) du matériau m, pour vérifier que c'est bien le matériau attendu. Rend le nombre
// de triangles marqués.
template <class Part, class Pos, class Couleur>
uint32_t marquerFeux(const std::string& id, std::vector<uint32_t>& idx, std::vector<Part>& parts, Pos pos, Couleur couleur) {
    uint32_t n = 0;
    for (const ZoneFeu& z : ZONES_FEUX) {
        if (id != z.voiture || std::fabs(couleur(z.materiau) - z.couleur) > 0.02f) continue;
        const size_t nParts = parts.size();
        for (size_t k = 0; k < nParts; k++) {
            if (parts[k].material != z.materiau || parts[k].feuArriere) continue;
            std::vector<uint32_t> garde, feu;
            for (uint32_t t = 0; t + 2 < parts[k].indexCount; t += 3) {
                const uint32_t* T = &idx[parts[k].firstIndex + t];
                vec3 c{};
                for (int j = 0; j < 3; j++) c = c + pos((size_t)parts[k].vertexOffset + T[j]) * (1.f / 3);
                const bool dedans = std::fabs(c.x) > z.xMin && c.y > z.yMin && c.y < z.yMax && c.z > z.zMin;
                for (int j = 0; j < 3; j++) (dedans ? feu : garde).push_back(T[j]);
            }
            if (feu.empty()) continue;
            std::copy(garde.begin(), garde.end(), idx.begin() + parts[k].firstIndex);
            std::copy(feu.begin(), feu.end(), idx.begin() + parts[k].firstIndex + garde.size());
            Part q = parts[k];
            q.firstIndex = parts[k].firstIndex + (uint32_t)garde.size(); q.indexCount = (uint32_t)feu.size(); q.feuArriere = 1;
            parts[k].indexCount = (uint32_t)garde.size();
            parts.push_back(q);
            n += (uint32_t)feu.size() / 3;
        }
    }
    return n;
}

// WFS-413 (RG-01, RG-04, ANO-240) : optiques avant de chaque voiture, repérées sur le modèle vu de face (matériau, rôle,
// boîte en miroir |x| xMin-xMax, y, z ≤ zMax ; avant vers −z). « lampe » : pièces qui s'allument la nuit (réflecteur,
// projecteur, DEL, bandeau) ; « glace » : verre de l'optique (son centre est le point de départ de la lumière). Le nombre de
// matériaux du .wfm est vérifié : un modèle changé n'est pas pris par erreur (journal « optiques non reconnues »).
struct PhareVoiture { const char* voiture; uint32_t materiaux; feux_modele::ZoneOptique zone; };
inline const PhareVoiture PHARES_AVANT[] = {
    // Abarth : grand phare rond (les petits ronds et les antibrouillards restent hors zone) ; glace
    {"abarth", 57, {11, false, 0.40f, 0.80f, 0.52f, 0.85f, -1.20f}},
    {"abarth", 57, {14, true, 0.40f, 0.80f, 0.52f, 0.85f, -1.20f}},
    // Compass : projecteur et module DEL (antibrouillards hors zone) ; glace fumée (pare-brise hors zone)
    {"compass", 16, {2, false, 0.55f, 0.88f, 0.80f, 1.00f, -1.80f}},
    {"compass", 16, {9, false, 0.55f, 0.88f, 0.80f, 1.00f, -1.80f}},
    {"compass", 16, {11, true, 0.55f, 0.88f, 0.80f, 1.00f, -1.80f}},
    // Corsa : intérieur sombre, projecteur, bandeau ; glace (calandre hors zone)
    {"corsa", 17, {3, false, 0.55f, 0.88f, 0.68f, 0.86f, -1.40f}},
    {"corsa", 17, {7, false, 0.55f, 0.88f, 0.68f, 0.86f, -1.40f}},
    {"corsa", 17, {8, false, 0.55f, 0.88f, 0.68f, 0.86f, -1.40f}},
    {"corsa", 17, {9, false, 0.55f, 0.88f, 0.68f, 0.86f, -1.40f}},
    {"corsa", 17, {12, true, 0.55f, 0.88f, 0.68f, 0.86f, -1.40f}},
    // Gallardo : réflecteurs texturés ; glace fumée (prises d’air hors zone)
    {"gallardo", 18, {3, false, 0.50f, 0.88f, 0.48f, 0.80f, -1.50f}},
    {"gallardo", 18, {4, false, 0.50f, 0.88f, 0.48f, 0.80f, -1.50f}},
    {"gallardo", 18, {6, true, 0.50f, 0.88f, 0.48f, 0.80f, -1.50f}},
    // Giulia : projecteur, réflecteurs, cerclages ; 2 glaces claires
    {"giulia", 68, {48, false, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {50, false, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {51, false, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {52, false, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {53, false, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {1, true, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    {"giulia", 68, {8, true, 0.45f, 0.90f, 0.52f, 0.75f, -1.70f}},
    // Golf : lampe LED texturée (matériau partagé avec la grille, pris dans la boîte), bandeau de jour ; glace claire
    {"golf", 21, {10, false, 0.48f, 0.88f, 0.56f, 0.82f, -1.50f}},
    {"golf", 21, {13, false, 0.48f, 0.88f, 0.56f, 0.82f, -1.50f}},
    {"golf", 21, {5, true, 0.48f, 0.88f, 0.56f, 0.82f, -1.50f}},
    // GT86 : projecteur et bandeau (antibrouillards hors zone) ; glace
    {"gt86", 15, {6, false, 0.40f, 0.85f, 0.58f, 0.82f, -1.60f}},
    {"gt86", 15, {0, true, 0.40f, 0.85f, 0.58f, 0.82f, -1.60f}},
    // M4 : 2 modules DEL et leur contour, feu de jour ; glace fumée
    {"m4", 53, {5, false, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    {"m4", 53, {6, false, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    {"m4", 53, {10, false, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    {"m4", 53, {11, false, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    {"m4", 53, {12, false, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    {"m4", 53, {26, true, 0.40f, 0.90f, 0.58f, 0.78f, -1.80f}},
    // Model 3 : réflecteurs et projecteur (ancien drapeau « feu arrière ») ; glace
    {"model3", 37, {11, false, 0.45f, 0.92f, 0.55f, 0.85f, -1.70f}},
    {"model3", 37, {12, false, 0.45f, 0.92f, 0.55f, 0.85f, -1.70f}},
    {"model3", 37, {13, false, 0.45f, 0.92f, 0.55f, 0.85f, -1.70f}},
    {"model3", 37, {14, true, 0.45f, 0.92f, 0.55f, 0.85f, -1.70f}},
    // Mustang (Corvette C7) : bandeau et projecteur ; glaces
    {"mustang", 32, {10, false, 0.55f, 0.90f, 0.56f, 0.78f, -1.50f}},
    {"mustang", 32, {14, false, 0.55f, 0.90f, 0.56f, 0.78f, -1.50f}},
    {"mustang", 32, {24, true, 0.55f, 0.90f, 0.56f, 0.78f, -1.50f}},
    {"mustang", 32, {20, true, 0.55f, 0.90f, 0.56f, 0.78f, -1.50f}},
    // 208 : projecteur et 3 griffes (crocs de jour hors zone) ; glace fumée
    {"p208", 18, {10, false, 0.45f, 0.80f, 0.66f, 0.86f, -1.50f}},
    {"p208", 18, {12, true, 0.45f, 0.80f, 0.66f, 0.86f, -1.50f}},
    // 911 : phare rond (feux de jour du bouclier hors zone) ; glace fumée
    {"p911", 13, {4, false, 0.48f, 0.90f, 0.60f, 0.86f, -1.50f}},
    {"p911", 13, {9, false, 0.48f, 0.90f, 0.60f, 0.86f, -1.50f}},
    {"p911", 13, {8, true, 0.48f, 0.90f, 0.60f, 0.86f, -1.50f}},
    // Range Rover Sport : bandeau DEL et modules ; glace fumée
    {"rrsport", 24, {2, false, 0.45f, 0.92f, 0.85f, 1.06f, -1.90f}},
    {"rrsport", 24, {18, false, 0.45f, 0.92f, 0.85f, 1.06f, -1.90f}},
    {"rrsport", 24, {12, true, 0.45f, 0.92f, 0.85f, 1.06f, -1.90f}},
    // S3 : face noire opaque de l’optique et ses DEL (aucune glace transparente)
    {"s3", 31, {27, false, 0.40f, 0.88f, 0.55f, 0.76f, -1.70f}},
    {"s3", 31, {16, false, 0.40f, 0.88f, 0.55f, 0.76f, -1.70f}},
    {"s3", 31, {10, true, 0.40f, 0.88f, 0.55f, 0.76f, -1.70f}},
    // SF90 : fines barres DEL ; glaces
    {"sf90", 54, {43, false, 0.55f, 0.95f, 0.52f, 0.70f, -1.40f}},
    {"sf90", 54, {49, false, 0.55f, 0.95f, 0.52f, 0.70f, -1.40f}},
    {"sf90", 54, {13, true, 0.55f, 0.95f, 0.52f, 0.70f, -1.40f}},
    {"sf90", 54, {12, true, 0.55f, 0.95f, 0.52f, 0.70f, -1.40f}},
    // T-Cross (Brezza) : 2 projecteurs DEL et bandeau (bas de caisse exclu) ; glace
    {"tcross", 39, {27, false, 0.45f, 0.90f, 0.85f, 1.05f, -1.50f}},
    {"tcross", 39, {22, true, 0.45f, 0.90f, 0.85f, 1.05f, -1.50f}},
    // V60 : réflecteur et projecteur ; glace claire et sa partie fumée
    {"v60", 23, {12, false, 0.40f, 0.90f, 0.52f, 0.82f, -1.60f}},
    {"v60", 23, {4, true, 0.40f, 0.90f, 0.52f, 0.82f, -1.60f}},
    {"v60", 23, {3, true, 0.40f, 0.90f, 0.52f, 0.82f, -1.60f}},
};
// zones de la voiture (vide : voiture inconnue ou modèle changé)
inline std::vector<feux_modele::ZoneOptique> zonesPhares(const std::string& id, uint32_t nMat) {
    std::vector<feux_modele::ZoneOptique> z;
    for (const PhareVoiture& p : PHARES_AVANT) if (id == p.voiture && nMat == p.materiaux) z.push_back(p.zone);
    return z;
}

} // namespace wfs::correctifs
