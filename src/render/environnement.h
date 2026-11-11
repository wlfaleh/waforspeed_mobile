// WaForSpeed — capture d'environnement (WFS-150, RG-03, épic E0) : cubemap HDR de la scène vue depuis un point.
//
// Six faces R16G16B16A16 (même format que la scène, avant exposition et tonemapping), de 256 px en Ultra, 128 en Haute et
// Moyenne, 64 en Basse (qualite::tailleCubemap). À la demande seulement (changement de carte ou d'heure : demander(),
// invalider(), ou suivi automatique de l'heure par suivre()), UNE face par image : une capture complète dure 6 images, jamais
// une capture à chaque image en course. Rien n'est créé (ni mémoire, ni coût) tant qu'aucune capture n'est demandée.
//
// Chaque face est rendue comme la passe miroir des reflets (WFS-16) : ciel, objets opaques (rigides et animés), puis
// transparents, avec la lumière, les ombres et le brouillard de l'image courante, sans masquage des objets (6 directions).
// Chaque face est rendue à 2 × 2 fois sa taille puis réduite (blit linéaire 2:1) : un texel vaut la luminance moyenne de son
// angle solide (M-01 : ± 1 % contre la scène vue dans la même direction, même quand de petites vitrines très claires la bordent).
// Repère des faces : convention des cubemaps Vulkan (+X, −X, +Y, −Y, +Z, −Z ; texel (s, t) de la spécification), pour
// qu'un shader échantillonne la direction monde telle quelle (samplerCube) : prêt pour l'éclairage d'image (WFS-151).
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "vk.h"
#include "cube_faces.h"   // axesFace, vueProjFace (calcul seul, testé par test_passes)

namespace wfs {
class Renderer;
namespace environnement {

// demande une capture depuis `point` (monde) ; taille 0 : selon le niveau de qualité affiché à la demande
// WFS-428 (ANO-245) : sansJoueur = capture sans la voiture du joueur (sondes d'éclairage du joueur, posées 1,5 m au-dessus d'elle :
// WFS-322) ; par défaut, la capture voit toute la scène, comme une caméra posée au même point (M-01 de WFS-150)
void demander(const vec3& point, int taille = 0, bool sansJoueur = false);
bool enCours();           // faces restant à rendre
bool prete();             // les 6 faces de la dernière demande sont rendues
int faceSuivante();       // face rendue à la prochaine image (0..5), −1 sans capture en cours
int taille();             // côté d'une face (px), 0 avant la première capture
vec3 point();             // point de la dernière capture
// suivi automatique : nouvelle capture depuis `point` quand l'heure change (soleil, ciel, brouillard de l'image) ou après
// invalider() (changement de carte) ; coupé par défaut (aucun consommateur dans WFS-150 : IBL de WFS-151)
void suivre(bool actif, const vec3& point);
void invalider();
int capturesFaites();     // captures complètes depuis le lancement (journal, banc)

// cube pour les shaders (VK_IMAGE_VIEW_TYPE_CUBE, SHADER_READ_ONLY) ; VK_NULL_HANDLE avant la première capture
VkImageView vueCube();
VkSampler echantillonneur();
VkImage image();   // WFS-151 : image cube (6 couches, SHADER_READ_ONLY) pour la relecture des sondes IBL

// banc : relit les 6 faces (rgba flottants, face par face), GPU à l'arrêt
bool lireFaces(Renderer& r, std::vector<float> faces[6], int& cote);

} // namespace environnement
} // namespace wfs
