// WaForSpeed — végétation réaliste (ticket WFS-34, épic E2) : port des générateurs de js/world/life.js du jeu web.
// Tronc et branches en vraie géométrie, feuillage en cartes alpha découpées (atlas assets/world/feuillage.png,
// exporté tel quel du jeu web par tools/atlas_feuillage.cjs).
// Le format de sommet n'a pas de couleur : la luminosité par sommet du jeu web (taches d'écorce, ombre du houppier) est
// portée par la LONGUEUR de la normale (matériaux MAT_VTINT, voir shaders/scene.frag) ; la teinte vient du matériau
// (une teinte de feuillage = un tampon) et le mélange des deux écorces vient de u (texture de 2 texels).
#pragma once
#include <vector>
#include "../core/math.h"
#include "../render/renderer.h"

namespace wfs {
namespace veg {

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };

// tampons d'un arbre : écorce (tronc + branches), cœur sombre du houppier, feuillage (teinte déjà choisie)
struct Out { Geo* bark; Geo* core; Geo* leaf; };

// paramètres de vegBroadleaf (valeurs par défaut = jeu web)
struct Broadleaf { float trunkR = 0.1f, trunkH = 0.42f, cw = 0.33f, ch = 0.27f, cy = 0.66f; int clusters = 6, cards = 11; };

// feuillu : tronc, charpentières, bouquets de feuillage couverts de cartes ; b = pied de l'arbre, h = hauteur
void broadleaf(const Out& o, vec3 b, float h, Mulberry32& R, const Broadleaf& p, bool lod);

// platane d'alignement (DECOR.streettree) : choisit la teinte (0/1) avec R, comme pick(VEG_TINT.plane, R)
// puis construit l'arbre. leaf[2] = tampons des deux teintes du platane.
void platane(Geo* bark, Geo* core, Geo* leaf[2], vec3 b, float itH, Mulberry32& R, bool lod);

// couleurs du jeu web (sRGB 0..1, utilisées telles quelles par le jeu web)
vec3 hex(uint32_t rgb);
inline const vec3 TINT_PLANE[2] = {{1.12f, 1.12f, 0.78f}, {1.05f, 1.1f, 0.8f}};
inline constexpr uint32_t BARK_PLANE = 0xa59a80, BARK_PLANE2 = 0x6f6a55, CORE_LEAF = 0x2a4020;

} // namespace veg
} // namespace wfs
