// WaForSpeed — inclinaison du téléphone pour le gyroscope (WFS-108, épic E3) : point d'extension commun.
// Android (gyroscope_android.cpp) publie la gravité vue par l'appareil (capteur de gravité, sinon accéléromètre lissé) ;
// le banc PC et les tests publient des valeurs d'essai. Angle = atan2(gy, gx) dans le plan de l'écran : tourner le
// téléphone dans le sens des aiguilles d'une montre l'augmente, quelle que soit l'orientation (paysage ou paysage inversé).
// Téléphone posé à plat (gravité dans le plan de l'écran < 3 m/s²) : pas d'angle (le gyroscope ne braque pas).
// Fil principal seulement.
#pragma once
#include <cmath>

namespace wfs::gyroscope {

struct Etat { bool valide = false; float angle = 0; bool capteur = false; };
inline Etat& etat() { static Etat e; return e; }
// gravité dans le repère de l'appareil (m/s², x vers la droite, y vers le haut en portrait)
inline void publier(float gx, float gy) {
    Etat& e = etat();
    e.valide = gx * gx + gy * gy >= 9.f;
    if (e.valide) e.angle = std::atan2(gy, gx);
}
// un capteur existe (Android) ou des valeurs d'essai sont publiées (banc, tests)
inline void capteurPresent(bool oui) { etat().capteur = oui; }

} // namespace wfs::gyroscope
