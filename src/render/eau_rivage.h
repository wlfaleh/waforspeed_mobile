// WaForSpeed — WFS-380 (RG-09, ANO-218) : rochers et galets du rivage, au bord de l'eau.
// Le trait de côte n'est plus une ligne droite entre la mer et le terrain : blocs d'enrochement en calcaire (0,6 à 2,4 m) et galets
// (0,15 à 0,45 m) posés à moitié dans l'eau. Chaque modèle est un vrai bloc de pierre, pas une boule : icosaèdre subdivisé, taillé par
// 3 à 5 plans de fracture (faces planes et arêtes émoussées du calcaire éclaté), bosselé par un bruit doux, base aplatie (posé sur le
// fond), proportions d'un bloc d'enrochement (longueur : largeur : hauteur ≈ 1 : 0,75 : 0,55). Modèles faits une fois au chargement
// (quelques variantes), puis copiés dans les maillages des morceaux du décor : rien n'est créé en course (RG-08).
#pragma once
#include <cstdint>
#include <vector>
#include "renderer.h"

namespace wfs::rivage {

struct Maille { std::vector<Vertex> v; std::vector<uint32_t> i; };

// bloc de pierre de « rayon » ≈ 1 (repère local, y vers le haut, base aplatie) ; subdiv 0 : 20 triangles (galet), 1 : 80, 2 : 320
Maille rocher(uint32_t graine, int subdiv);
// copie du modèle posé en c, axes du repère local (longueur comprise : échelle), uv en mètres / tuile (texture de roche du décor)
void poser(const Maille& m, vec3 c, vec3 ax, vec3 ay, vec3 az, float tuile, std::vector<Vertex>& v, std::vector<uint32_t>& i);

} // namespace wfs::rivage
