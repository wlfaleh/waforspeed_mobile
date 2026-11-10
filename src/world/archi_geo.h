#pragma once
// WaForSpeed — WFS-161 (ST-1) : géométrie du kit d'architecture (aucun Vulkan : testée par tests/archi_geo.cpp).
//
// Repère local d'un bâtiment : x le long de la façade (0 → largeur), y vers le haut (0 = sol), z sortant de la façade
// (la façade est le plan z = 0, le bâtiment s'étend vers z < 0 sur sa profondeur). La carte place ce repère dans le
// monde (archi::versMonde). Les ouvertures sont de vrais trous dans le mur (RG-03) : tableaux en retrait, vitre ou
// menuiserie au fond, appui en saillie. uv en mètres pour les murs (textures en répétition), 0..1 par baie pour les vitres
// (intérieurs de nuit, ST-3).
#include <cstdint>
#include <vector>
#include "archi_kit.h"

namespace wfs::archi {

struct Sommet { vec3 p, n; vec2 uv; };
struct Maillage {
    std::vector<Sommet> v;
    std::vector<uint32_t> i;
    size_t triangles() const { return i.size() / 3; }
};

// un maillage par matériau (un appel de dessin par matériau)
enum Part : int { MUR, ENCADREMENT, VITRE, MENUISERIE, TOIT, METAL, VOLET, FIN, NB_PARTS };   // VOLET : ST-2, volets peints (méditerranéen) ; FIN : WFS-370, barreaux (fils de 16 mm, dessinés en couverture partielle)

// niveaux de détail (RG-05) : 0 complet ; 1 tableaux et vitres gardés, sans appuis, balcons, garde-corps ni édicules ;
// 2 volume seul (murs pleins, vitres peintes à plat sur la façade, toiture), pour le loin
struct Batiment { Maillage part[NB_PARTS]; };

// WFS-370 : graine (0..99) d'une baie, codée dans ses uv : fixe pour une même baie, différente d'une fenêtre et d'un bâtiment à l'autre
inline int graineBaie(const Plan& p, int niveau, int travee) {
    uint32_t x = (uint32_t)(p.hRdc * 1e4f) * 2654435761u ^ (uint32_t)(p.hEtage * 1e4f) * 40503u ^ (uint32_t)(niveau * 131 + travee * 7919 + (int)p.style * 31);
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return (int)(x % 100u);
}

// construit le bâtiment d'un plan, de profondeur donnée, au niveau de détail lod (0, 1 ou 2)
void construire(const Plan& p, float profondeur, int lod, Batiment& out);

// aire des murs de la façade avant (z = 0, normale +z) entre le sol et yMax (sans le socle ni l'acrotère), pour les
// contrôles : façade − ouvertures
double aireFacade(const Maillage& mur, float yMax);

} // namespace wfs::archi
