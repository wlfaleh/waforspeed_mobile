// WaForSpeed — WFS-381 (épic E13) : banc de la brume au loin (WFS_BANC=brume).
//   - captures brume-<vue>-jour (Corniche, Campagne, Ville, vues de poursuite et vues hautes) et brume-corniche-nuit ;
//   - brume-mesures.txt : réglage par carte, contraste restant calculé à 200 m, 1 km, 5 km (RG-02), coût GPU en Basse (RG-05).
#pragma once
#include <string>

namespace wfs {
class Game; class Renderer;
// rend le nombre d'erreurs (zone absente, carte impossible à charger)
int benchBrume(Game& g, Renderer& r, const std::string& out);
}
