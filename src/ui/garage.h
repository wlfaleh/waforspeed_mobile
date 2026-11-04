// WaForSpeed — garage (WFS-11, avec WFS-40 : fiche et peintures ; épic E5).
// Port de js/world/garage.js (buildGarage, setGarageCar, renderGarage) et de js/ui/screens.js (renderGarageUI,
// selectPaint, cycleCar, leaveGarage) du jeu web, style css/style.css (.screen--show, .show-head, .show-panel, .specs,
// .statlist, .paint-row, .btn). Système enfichable « Garage » (ordre 440), actif seulement en Ecran::Garage :
//   - scène dédiée (WFS-44) : voiture sur plateau tournant, anneau néon, barres lumineuses, hémisphère + 3 projecteurs
//     (WFS-45), fond #07070c et brouillard 14-40 m ;
//   - fiche (marque, index, modèle, type, accroche, puissance, 0 à 100, vitesse max, 4 barres carStats, crédit) ;
//   - 16 peintures (PAINTS), mémorisées par voiture pendant la session ;
//   - « Choisir » : voiture + peinture retenues pour la course (GameView::choisirVoiture), puis course ; « Retour » : menu.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
// centre du bouton du garage nommé (« choisir », « retour », « suivant », « precedent ») à la dernière image : touchers
// simulés du banc d'essai (menu tactile, garage) ; false si le garage n'a pas encore été dessiné
bool garageBouton(const char* nom, float& x, float& y);
// banc d'essai : captures garage-0/1/2 (3 angles), garage-m4 (voiture suivante), garage-peinture-0/1, garage-peintures
// (planche des 16 peintures), garage-course (course avec la voiture et la peinture choisies), puis g.shutdown()
void benchGarage(Game& g, Renderer& r, const std::string& outDir, const std::string& suffixe = "");
} // namespace ecrans
} // namespace wfs
