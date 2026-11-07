// WaForSpeed — banc de profilage (WFS-222, RG-01) : WFS_BANC=perf. Sur les 4 cartes (Ville, Corniche, Campagne, Grand Prix),
// en Haute puis en Basse, course en pilote automatique : temps processeur de chaque système (update, render, drawUi), du
// moteur (Game hors systèmes, enregistrement des passes du Renderer), attente du GPU, appels de dessin, triangles et coût
// GPU des passes ; moyenne, pire image et images > 33 ms ; tableau au journal (« perf : … »). Rien de tout cela dans l'APK.
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
void benchPerf(Game& g, Renderer& r, const std::string& out);
} // namespace wfs
