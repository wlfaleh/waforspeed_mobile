// WaForSpeed — WFS-537 (RG-07 à RG-09, épic E22) : compteur d'étoiles du menu principal, petit composant réutilisable.
//   - RG-07 : icône ★ + solde du portefeuille (boutique::solde, WFS-498), en haut du menu, à gauche du bouton « Réglages »,
//     même hauteur et même style vitré que lui ; le bouton « Réglages » et les autres boutons ne bougent pas ;
//   - RG-08 : relu à chaque image (à jour dès le retour au menu après une course, un achat, un cadeau) ; « 12 450 » en
//     français, « 12,450 » en anglais ; libellé pour le lecteur d'écran par ui::tr (menu.etoiles.lecteur) ;
//   - RG-09 : appui → garage (WFS-513 le fera ouvrir le Market).
// Le menu (menu.cpp) l'appelle : dessiner() à chaque image, dans() pour ses appuis (appui au relâcher, comme ses choix).
#pragma once
#include <string>

namespace wfs {
class Renderer;
struct GameView;
namespace ecrans::compteur_etoiles {

// dessine le compteur à gauche du bouton « Réglages » (rien tant que ce bouton n'a pas été dessiné) ; presse : état appuyé
void dessiner(Renderer& r, const GameView& g, bool presse);
// appui dans la cible du compteur (dessin élargi à 48 dp de haut, comme « Réglages ») ; faux s'il n'est pas dessiné
bool dans(float x, float y);
// texte lu par le lecteur d'écran (« 12 450 étoiles » / « 12,450 stars ») ; aussi noté au journal du banc
std::string texteLecteur();
// banc : centre du compteur à la dernière image ; faux s'il n'est pas dessiné
bool centre(float& x, float& y);

} // namespace ecrans::compteur_etoiles
} // namespace wfs
