// WaForSpeed — WFS-149 (RG-04) : tangentes des sommets et lecture des sommets des voitures .wfm versionnées.
//
// Tangente d'un sommet (Vertex::tx..tw, 4 octets signés normalisés, /127) : direction xyz de +u dans le plan du sommet,
// signe w de la bitangente (bitangente = w · normale × tangente, convention glTF / MikkTSpace). Tangente nulle (w = 0) :
// sommet sans tangente (scene.frag garde alors le repère tiré des dérivées écran, comme avant WFS-149).
//
// Calcul : celui de MikkTSpace (M. Mikkelsen, « Simulation of Wrinkled Surfaces Revisited », 2008 ; implémentation de
// référence mikktspace.c, licence zlib) pour un maillage déjà découpé aux coutures d'uv : par coin de triangle, dérivées
// ∂p/∂u et ∂p/∂v projetées dans le plan de la normale du sommet, normalisées et pondérées par l'angle du coin, puis
// sommées par sommet et ré-orthogonalisées (Gram-Schmidt) ; signe = orientation de la bitangente sommée.
// Différence connue avec mikktspace.c : pas de découpe des sommets dont les coins ont des repères opposés (miroirs d'uv
// sur un même sommet) : la conversion des modèles (tools/convert_car.mjs) sépare déjà les sommets aux coutures.
//
// Format .wfm version 2 (tools/convert_car.mjs) : version 1 inchangée (sommets de 32 octets : position, normale, uv), puis
// à la fin du fichier une section 'TAN1' : pour chaque maillage, dans l'ordre, nSommets × 4 octets signés (tx ty tz tw).
// Les lecteurs de la version 1 (tests, planche des voitures, banc du réalisme, alleger_voiture.mjs) lisent la version 2
// sans changement : la section est après tout ce qu'ils lisent. Version 1 : tangentes calculées au chargement.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "renderer.h"

namespace wfs::tangentes {

constexpr uint32_t WFM_SOMMET_OCTETS = 32;          // sommet dans le fichier (pos3 normale3 uv2), versions 1 et 2
constexpr uint32_t WFM_VERSION_TANGENTES = 2;       // première version avec la section 'TAN1'

inline int8_t versOctet(float x) {   // [-1, 1] → [-127, 127] (arrondi au plus proche)
    const float c = x < -1.f ? -1.f : (x > 1.f ? 1.f : x);
    return (int8_t)(c >= 0 ? (int)(c * 127.f + 0.5f) : -(int)(-c * 127.f + 0.5f));
}
inline vec4 lire(const Vertex& v) { return {v.tx / 127.f, v.ty / 127.f, v.tz / 127.f, v.tw / 127.f}; }
inline bool presente(const Vertex& v) { return v.tw != 0; }

// calcule la tangente des nv sommets (indices locaux, triangles), remplace les tangentes existantes ;
// renvoie le nombre de sommets sans uv exploitable (repère arbitraire perpendiculaire à la normale, w = +1)
uint32_t calculer(Vertex* v, uint32_t nv, const uint32_t* idx, uint32_t ni);
// vrai si au moins un sommet a déjà une tangente (maillage fourni avec ses tangentes : on n'y touche pas)
bool auMoinsUne(const Vertex* v, size_t nv);

// copie nv sommets du fichier (32 octets chacun) dans des Vertex (tangente nulle)
void copierSommetsWfm(const uint8_t* src, uint32_t nv, Vertex* dst);
// section 'TAN1' d'un .wfm version 2 (p = juste après le dernier maillage) : tangentes des maillages, dans l'ordre ;
// premiers[i] = indice du premier sommet du maillage i dans v, nombres[i] = son nombre de sommets. Faux si absente ou tronquée.
bool lireSectionTangentes(const uint8_t* p, const uint8_t* fin, const std::vector<size_t>& premiers,
                          const std::vector<uint32_t>& nombres, Vertex* v);

} // namespace wfs::tangentes
