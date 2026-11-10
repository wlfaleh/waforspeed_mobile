// WaForSpeed — WFS-380 : caméra libre du banc de l'eau (WFS_BANC=mer), sans scène dédiée : la carte est dessinée comme en course
// (éclairage physique, exposition, lampadaires et phares allumés la nuit autour de la voiture). Inactive hors banc.
#pragma once
#include "../core/math.h"

namespace wfs::eau {
void vueBanc(bool actif, vec3 oeil = {}, vec3 cible = {}, float fov = 55);
} // namespace wfs::eau
