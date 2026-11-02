// WaForSpeed — texte de l'interface (ticket WFS-26, épic E5).
// Police Rajdhani Bold (OFL) en atlas de champ de distance signé : net à toutes les tailles, accents français.
// Utilisation depuis un système (drawUi) :
//   ui::text(r, x, y, 32, "Vitesse : 182 km/h", ui::CYAN, ui::GAUCHE);
//   ui::text(r, cx, y, 48, ui::nombre(12345), ui::BLANC, ui::CENTRE, 0.16f);   // espacement des lettres (em)
#pragma once
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace ui {

enum Align { GAUCHE, CENTRE, DROITE };

// couleurs du skill design-street-racing
inline const vec4 BLANC{1, 1, 1, 1}, CYAN{0.f, 0.898f, 1.f, 1}, ORANGE{1.f, 0.416f, 0.f, 1}, ROUGE{1.f, 0.122f, 0.294f, 1},
                  GRIS{0.62f, 0.66f, 0.72f, 1};

bool fontReady();
// x, y : coin haut-gauche de la ligne (selon l'alignement) en pixels logiques ; px : hauteur des capitales + jambages
// espacement : en fraction de px (0,16 = letter-spacing .16em du web) ; ombre : léger contour sombre pour la lisibilité
void text(Renderer& r, float x, float y, float px, const std::string& utf8, const vec4& color, Align align = GAUCHE,
          float espacement = 0.f, bool ombre = true);
float width(float px, const std::string& utf8, float espacement = 0.f);

// formats français (toLocaleString('fr-FR') du jeu web)
std::string nombre(long long v);                 // 12 345 (espace fine insécable)
std::string decimal(double v, int chiffres);      // 1,2
std::string chrono(double secondes);              // fmtTime du jeu web : m:ss.d
std::string majuscules(const std::string& utf8);  // libellés en capitales (accents compris)

} // namespace ui
} // namespace wfs
