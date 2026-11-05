// WaForSpeed — banc d'endurance (WFS-91, épic E7) : longue session automatique sur PC (wfs_bench, WFS_BANC=endurance).
// Au moins 8 cycles Menu → Terrain (carte, moment) → Course en pilote automatique → Fin → Rejouer / Menu, sur les 4 cartes
// de jour et de nuit ; un cycle sur trois passe par le Garage (autre voiture, autre peinture) et par la Pause ; 3 retours
// d'arrière-plan simulés (termGraphics / initGraphics d'android_main.cpp : jeu, rendu et Vulkan libérés puis recréés).
// WFS-129 (RG-07) : puis un cycle Course à 5 (2 km) et un cycle Poursuite (jusqu'à la fin), par vrais touchers : accueil →
// « Course solo » → mode → « Continuer » → garage → « Choisir » → terrain → « Rouler » → course (pause par la touche Retour
// du téléphone, « Reprendre ») → fin → « Menu » → accueil ; un écart du parcours = sortie en échec (code 5).
// Mémoire résidente (VmRSS) relevée au même point de chaque cycle (accueil, avant la course, WFS-129 RG-06) : au-delà de
// 10 % du cycle 2, sortie en échec (code 3) ; la mesure de fin reste au journal mais ne compte plus.
// Les erreurs de validation Vulkan (VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation) sont comptées par tools/qa/endurance.sh.
// Rien de tout cela n'existe dans l'APK.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
class VkCtx;
// renvoie le code de sortie du banc (0 : réussi, 3 : fuite de mémoire sur les cycles Checkpoint, 4 : chargement impossible,
// 5 : écart du parcours, 6 : plus de 10 % seulement après la Course à 5 / la Poursuite)
int benchEndurance(VkCtx& vk, Renderer& r, Game& g, const std::string& out, unsigned largeur, unsigned hauteur);
// WFS-129 (RG-08) : en course : touche Retour du téléphone → pause, « Reprendre » au doigt → course (t : horloge du banc) ;
// rend le nombre d'écarts (WFS_BANC=parcours, desktop_main.cpp)
int parcoursPauseRetour(Game& g, double& t);
} // namespace wfs
