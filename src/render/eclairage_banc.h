// WaForSpeed — WFS-151 (épic E13, ST-5) : banc de la lumière physique, appelé par le banc du réalisme (WFS_BANC=realisme).
//   - étalonnage (Campagne, départ, jour, Haute) : charte grise 18 % au soleil et à l'ombre, ColorChecker 24 cases, mesurés
//     dans la cible HDR (avant tonemapping), exposition fixée à EV100 15 (M-01, M-02, M-03) ; capture realisme-etalon-hdr.png ;
//   - ciel au zénith et 12° au-dessus de l'horizon pour 3 hauteurs du soleil (M-04) ;
//   - tunnel de la Corniche : voiture dans le tunnel (realisme-voiture-tunnel), sortie du tunnel avec l'adaptation de
//     l'exposition (realisme-sortie-tunnel-0, -1, -2) ;
//   - lignes du rapport de fidélité (domaine « lumiere ») et informations (EV, sondes, coût).
#pragma once
#include <ostream>
#include <string>
#include "bench_realisme.h"

namespace wfs {
class Game; class Renderer;
// rend le nombre d'erreurs (scène impossible à charger, image HDR illisible)
int benchLumiere(Game& g, Renderer& r, const std::string& out, const reel::Referentiel& R, reel::Rapport& rap, std::ostream& mesures, std::ostream& infos);
}
