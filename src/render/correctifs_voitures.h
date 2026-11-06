// WaForSpeed — WFS-170 : correctifs des modèles de voitures appliqués au chargement (aucun .wfm modifié), pour les défauts
// relevés à la recette des 20 voitures (ANO-71 à 77) :
//  - RG-01 : couche de vernis séparée (matériau transparent sombre qui enveloppe toute la carrosserie, « coat » de la 911) :
//    retirée ; le vernis est déjà calculé par le shader sur la peinture (MAT_PAINT), la couleur choisie redevient visible ;
//    peinture de la 911 en peinture unie métallisée (métal 0,45, rugosité 0,32, comme les 16 voitures de WFS-77) ;
//  - RG-02, RG-03 : vitres à points (carte de normales de sérigraphie, Giulia) ou noires rugueuses (camouflage de la S3) :
//    verre teinté uni (MAT_GLASS) ;
//  - RG-03, RG-06 : jantes blanc uni (S3) ou noyées dans le noir (Compass) : métal gris ;
//  - RG-04 : feux arrière pris dans un matériau partagé blanc ou gris (V60, GT86, Abarth, A45) : pièces « feu arrière »
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

} // namespace wfs::correctifs
