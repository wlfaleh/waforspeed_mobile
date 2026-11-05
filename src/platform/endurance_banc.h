// WaForSpeed — banc d'endurance (WFS-91, épic E7) : longue session automatique sur PC (wfs_bench, WFS_BANC=endurance).
// Au moins 8 cycles Menu → Terrain (carte, moment) → Course en pilote automatique → Fin → Rejouer / Menu, sur les 4 cartes
// de jour et de nuit ; un cycle sur trois passe par le Garage (autre voiture, autre peinture) et par la Pause ; 3 retours
// d'arrière-plan simulés (termGraphics / initGraphics d'android_main.cpp : jeu, rendu et Vulkan libérés puis recréés).
// Mémoire résidente (VmRSS) relevée au début de chaque cycle : au-delà de 10 % du cycle 2, sortie en échec (code 3).
// Les erreurs de validation Vulkan (VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation) sont comptées par tools/qa/endurance.sh.
// Rien de tout cela n'existe dans l'APK.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
class VkCtx;
// renvoie le code de sortie du banc (0 : réussi, 3 : fuite de mémoire, 4 : chargement impossible)
int benchEndurance(VkCtx& vk, Renderer& r, Game& g, const std::string& out, unsigned largeur, unsigned hauteur);
} // namespace wfs
