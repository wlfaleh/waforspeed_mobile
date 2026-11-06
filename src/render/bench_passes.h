// WaForSpeed — banc du socle des passes (WFS-150), lancé à la fin du banc du réalisme (WFS_BANC=realisme) :
//   - passes enregistrées et coût GPU de chacune (realisme-mesures.txt), passe d'essai + pré-passe (realisme-prepasse-essai),
//     contrôle « 0 écart » de l'image avec et sans la passe d'essai ;
//   - coût des passes du Renderer par niveau (Basse, Moyenne, Haute, Ultra), même scène figée (RG-05, P-02) ;
//   - capture d'environnement : planche des 6 faces (realisme-cubemap-essai, Ultra 256 px) et M-01 (luminance de chaque
//     face contre la scène vue dans la même direction, même image HDR, ± 1 %) ;
//   - niveau Ultra : reglages-qualite-ultra, reglages-diagnostic et parcours accueil → Réglages (Ultra) → course → accueil.
#pragma once
#include <ostream>
#include <string>

namespace wfs {
class Game; class Renderer;
// mesures : section ajoutée à realisme-mesures.txt ; infos : lignes « hors indice » de realisme-fidelite.txt ; rend le nombre d'écarts
int benchPasses(Game& g, Renderer& r, const std::string& out, std::ostream& mesures, std::ostream& infos);
}
