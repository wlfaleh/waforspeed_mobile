// WaForSpeed — lampadaires déclarés par le décor pour l'éclairage de nuit (ticket WFS-09, épic E4).
// Le décor (src/world, par une accroche de quelques lignes dans son placement) inscrit chaque lampadaire ; l'éclairage
// de nuit (src/render/nuit.cpp) allume les plus proches (4 lumières au plus), leur halo et leur flaque de lumière au sol.
// La liste appartient à la partie : remplie à l'init du décor, vidée à l'arrêt de l'éclairage (Game::systemsShutdown).
#pragma once
#include <cstdint>
#include <vector>

namespace wfs {

// style du jeu web (decor.js, lamp) : ville (0xffb46b, 26 m), plage, projecteur de stade (0xeaf2ff, 60 m)
enum class StyleLampadaire { Ville, Plage, Stade, Tete };   // WFS-337 (accroche) : Tete = 2e lanterne d'un candélabre double, halo seul (ni lumière ni flaque au sol)

// seg : segment de la piste (sur un tour) ; lat : écart latéral de la tête (m, comme Track::point) ; y : hauteur de la tête
struct Lampadaire { int64_t seg; float lat, y; StyleLampadaire style; };

inline std::vector<Lampadaire>& lampadaires() { static std::vector<Lampadaire> l; return l; }

} // namespace wfs
