// WaForSpeed — WFS-240 (RG-01) : poussettes des familles (épic E11). Règles pures et géométrie (aucun appel Vulkan), testées
// par tests/fidelite_humains_familles.cpp et dessinées par les piétons (pietons.cpp) :
//   - ≈ 1 famille sur 4 a une poussette : l'enfant de la famille devient le petit assis dedans (le total de personnes ne
//     change pas, RG-03 de WFS-233) ; l'adulte la pousse, les mains sur la poignée (IK de WFS-233) ;
//   - la poussette roule devant l'adulte, sur son trottoir : jamais sur la chaussée (les familles n'y vont pas) ;
//   - poussette « 4 roues » d'aujourd'hui, dimensions d'une poussette citadine du commerce (fiches techniques, à confirmer :
//     references/reel/humains/poussette.md) ; ≤ 400 triangles ; tissu, métal et plastique en trois matériaux distincts.
#pragma once
#include <cstdint>
#include "../core/math.h"

namespace wfs::veg { struct Geo; }   // vegetation.h (sommets du moteur) : le test CPU n'a besoin que des règles

namespace wfs::poussette {

// ---------- dimensions (m) : repère local, +z vers l'avant (sens de la marche), y vers le haut, origine au sol entre les roues arrière
constexpr float LONGUEUR = 1.00f;        // de la poignée à l'avant des roues avant (vue de côté)
constexpr float LARGEUR = 0.60f;         // entre les flancs extérieurs des roues arrière
constexpr float POIGNEE_Y = 1.00f;       // hauteur de la poignée
constexpr float POIGNEE_Z = -0.30f;      // la poignée dépasse en arrière de l'essieu arrière
constexpr float ESSIEU_AVANT_Z = 0.58f;  // empattement
constexpr float ROUE_AR = 0.125f, ROUE_AV = 0.09f;   // rayons (roues de 25 et 18 cm)
constexpr float SIEGE_Y = 0.50f, SIEGE_Z = 0.22f;    // assise du siège (fesses du petit)
constexpr float MAINS_ECART = 0.40f;     // écart des mains sur la poignée
constexpr float DEVANT_ADULTE = 0.72f;   // de l'axe de l'adulte à l'essieu arrière : poignée ≈ 0,42 m devant le bassin
constexpr float ECHELLE_PETIT = 0.85f;   // le petit (≈ 2 ans, 0,87 m) : morphotype de 4 ans (1,02 m) à 85 %

// ---------- règle : une famille sur 4 environ (tirage déterministe par la graine de la carte et le rang du lien)
constexpr uint32_t POUR_CENT = 25;
inline bool avecPoussette(uint32_t graine, int lien) {
    uint32_t h = graine * 2654435761u ^ (uint32_t)(lien + 1) * 2246822519u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return h % 100 < POUR_CENT;
}

// parties : 0 tissu (nacelle, capote, panier), 1 métal (châssis, poignée), 2 plastique (roues, poignée en mousse)
enum Partie { TISSU, METAL, PLASTIQUE, NB_PARTIES };
// géométrie de la poussette (uv : centre de la palette de la partie, la couleur vient du matériau) ; renvoie le nombre de triangles
int construire(veg::Geo* out);   // out[NB_PARTIES]

// mains sur la poignée (repère local de la poussette) : gauche (x < 0) et droite
inline vec3 prise(bool gauche) { return {(gauche ? -0.5f : 0.5f) * MAINS_ECART, POIGNEE_Y, POIGNEE_Z}; }

} // namespace wfs::poussette
