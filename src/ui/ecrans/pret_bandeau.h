// WaForSpeed — bandeau « Voiture prêtée » du menu principal (WFS-502, épic E19, zone E8 ; accroche E8 dans menu.cpp).
// Au-dessus du bouton « Profil » (WFS-482, lui-même au-dessus de « Cadeau du jour »), en bas à droite de la zone sûre (style design-street-racing : verre teinté, coin coupé,
// liseré néon, bouton orange en biseau) :
//   - prêt en attente (RG-01) : « VOITURE PRÊTÉE : <NOM> » · « À activer dans 23 h 12 min » · bouton « Activer » ;
//   - prêt actif (RG-03) : « VOITURE PRÊTÉE : <NOM> » · « Encore 2 h 41 min au volant » (relu à chaque image, minutes) ; appui :
//     garage sur la voiture prêtée ;
//   - prêt perdu (RG-01) : « Prêt expiré : <nom> » · « Pas activée dans les 24 h », une fois ; appui : le bandeau part ;
//   - fin du prêt (RG-05) : « Fin du prêt : <nom> » · « Garde-la pour toujours : 2 200 ★ » · bouton « Garage » (garage sur la voiture,
//     où elle s'achète ; « Market » quand le market existera), une fois.
// Un message vu (dessiné au menu) est oublié en quittant le menu ou à l'appui (pret::oublierMessage, sauvegarde écrite).
// Rien en course (RC-E19-06) : le bandeau n'existe qu'au menu. Textes par ui::tr (FR / EN) ; noms des voitures : noms inventés.
#pragma once
#include "../../core/system.h"   // Marges, GameView

namespace wfs {
class Renderer;
namespace ecrans::pret_bandeau {

bool visible();   // un prêt en attente ou actif, ou un message à montrer
// dessiné par le menu (drawUi), après le bouton « Cadeau du jour » ; presse : état appuyé (action au relâcher, comme le menu)
void dessiner(Renderer& r, float W, float H, const Marges& m, bool presse);
bool dans(float x, float y);         // appui dans le bandeau dessiné à la dernière image
void appuyer(const GameView& g);     // relâcher sur le bandeau : Activer, garage ou message vu
bool centre(float& x, float& y);     // banc : centre du bouton (ou du bandeau sans bouton) ; faux s'il n'est pas dessiné

} // namespace ecrans::pret_bandeau
} // namespace wfs
