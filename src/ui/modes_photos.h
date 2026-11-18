// WaForSpeed — WFS-562 (épic E23, maquette R19 02 ; RG-01 à RG-04, RG-06) : images qui disent le mode, dans les cartes de
// l'écran « Choisis ton mode ». Trois photos du jeu lui-même (rendu du moteur, carte Ville), faites UNE fois par le banc d'essai
// PC (banc « modes-photos ») et rangées dans assets/ui/modes/ (JPEG) :
//   - checkpoint.jpg : la voiture du joueur passe sous le portique de checkpoint (trois quarts arrière, jour) ;
//   - course5.jpg    : la voiture du joueur au milieu de 4 concurrents, à touche-touche (jour) ;
//   - poursuite.jpg  : la voiture du joueur poursuivie par la police, gyrophares allumés (nuit).
// Chargées à l'ouverture de l'écran (décodage et envoi d'une planche de 3 photos), gardées jusqu'à l'arrêt du moteur ; rien
// pendant la course. Sans photos (fichier absent ou illisible, mémoire refusée), l'écran garde la scène floutée (RG-06).
#pragma once
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace modes_photos {

constexpr int NB = 3;                         // ordre des cartes de l'écran : Checkpoint, Course à 5, Poursuite
constexpr int LARGEUR = 720, HAUTEUR = 546;   // une photo : carte de 470 × 356 (maquette 1600 × 720) en 2400 × 1080 (705 × 534) + marge
extern const char* const FICHIERS[NB];        // dans assets/ : ui/modes/checkpoint.jpg, course5.jpg, poursuite.jpg

// décode les 3 photos et envoie la planche (une fois) ; faux si une photo manque (repli : scène floutée)
bool charger(Renderer& r);
bool pretes();
void liberer(Renderer& r);   // à l'arrêt du moteur (System::shutdown)
void refuserBanc(bool oui);  // banc seulement : charger() échoue comme une photo absente (repli RG-06)

// photo n° i dans la carte inclinée q (4 sommets dans l'ordre du dessin) de boîte (x, y, w, h) : la photo couvre toute la boîte,
// coupée par le parallélogramme (pas de déformation) ; voile sombre de transparent en yVoile0 à opaque (alpha voile) en yVoile1
void dessiner(Renderer& r, int i, const vec2 q[4], float x, float y, float w, float h, float yVoile0, float yVoile1, float voile);

} // namespace modes_photos
} // namespace wfs
