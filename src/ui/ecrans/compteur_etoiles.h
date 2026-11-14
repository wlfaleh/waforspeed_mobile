// WaForSpeed — WFS-537 (RG-07 à RG-09, épic E22) : compteur d'étoiles du menu principal, petit composant réutilisable.
//   - RG-07 : icône ★ + solde du portefeuille (boutique::solde, WFS-498), en haut du menu, à gauche du bouton « Réglages »,
//     même hauteur et même style vitré que lui ; le bouton « Réglages » et les autres boutons ne bougent pas ;
//   - RG-08 : relu à chaque image (à jour dès le retour au menu après une course, un achat, un cadeau) ; « 12 450 » en
//     français, « 12,450 » en anglais ; libellé pour le lecteur d'écran par ui::tr (menu.etoiles.lecteur) ;
//   - RG-09 : appui → garage (WFS-513 le fera ouvrir le Market).
// Dessin : système CompteurEtoiles (461), juste après le bouton « Réglages » (460) de la même image, seulement si ce bouton a été
// dessiné (rien sous « Crée ton pilote », le choix du mode, les Crédits, le cadeau du jour), avec le fondu d'entrée du menu (0,3 s).
// Le menu (menu.cpp) garde les appuis : dans() pour sa cible, appuye() pour l'état appuyé (action au relâcher, comme ses choix).
#pragma once
#include <string>

namespace wfs {
class Renderer;
struct GameView;
namespace ecrans::compteur_etoiles {

// état appuyé (voile cyan), posé par le menu à chaque image
void appuye(bool oui);
// appui dans la cible du compteur (dessin élargi à 48 dp de haut, comme « Réglages ») ; faux s'il n'est pas dessiné
bool dans(float x, float y);
// texte lu par le lecteur d'écran (« 12 450 étoiles » / « 12,450 stars ») ; aussi noté au journal du banc
std::string texteLecteur();
// banc : centre du compteur à la dernière image ; faux s'il n'est pas dessiné
bool centre(float& x, float& y);

} // namespace ecrans::compteur_etoiles
} // namespace wfs
