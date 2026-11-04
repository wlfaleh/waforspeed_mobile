// WaForSpeed — banc d'essai de la qualité (WFS-15) : qualite-basse.png, qualite-moyenne.png, qualite-haute.png (nuit,
// bloom en Moyenne et Haute seulement), appels de dessin et ms par niveau, et suite d'images/s simulées au journal
#pragma once
#include <string>
namespace wfs {
class Game; class Renderer;
void benchQualite(Game& g, Renderer& r, const std::string& out);
}
