#pragma once
// WaForSpeed — WFS-161 (ST-5) : bâtiments du kit d'architecture posés dans les cartes à la place des boîtes.
// Une carte (ville.cpp, plage.cpp, campagne.cpp) déclare ses bâtiments à son init (poser) ; le système « archi-carte »
// (archi_carte.cpp) les construit par morceaux de route autour du joueur, au niveau de détail de leur distance
// (RG-05 : < 120 m complet, < 300 m allégé, au-delà volume seul), et les dessine en un appel par matériau et par morceau.
// WFS-416 : si le paquet de la carte contient le bloc « batiments » (archi_cuit.h), tout est lu au chargement, envoyé une fois à
// la carte graphique dans les 3 niveaux, et plus rien n'est construit en course ; sinon le kit construit par le code (secours).
// Repli (RG-05) : WFS_ARCHI=0 (ou aucun bâtiment posé) → actif() rend false et la carte garde ses anciennes boîtes.
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "archi_kit.h"
#include "archi_cuit.h"   // WFS-416 : bâtiments posés (Bati), cuisson dans le paquet
#include "../render/renderer.h"   // WFS-269 : Vertex

namespace wfs::archi::carte {

// oublie les bâtiments posés (appelé par la carte à son init, avant poser)
void vider();
// pose un bâtiment : seg = segment de la route où il commence (choix du morceau), origine = coin bas gauche de la façade
// (au sol), x = sens de la façade (unitaire, horizontal), z = normale de la façade vers la rue (unitaire, horizontale)
void poser(int64_t seg, vec3 origine, vec3 x, vec3 z, float largeur, float profondeur, float hauteur, Style style, uint32_t graine);
// WFS-416 : les bâtiments posés (lus par la cuisson du paquet, cartes_v2::cuire)
const std::vector<cuit::Bati>& poses();
// WFS-416 (RG-07) : temps passé par le système des bâtiments dans son rendu (choix des morceaux, construction du kit de secours ou
// recopie du paquet, mise en file des dessins), depuis la dernière remise à zéro : preuve chiffrée au banc « batiments »
struct Chrono { double cumulMs = 0, pireMs = 0; uint64_t images = 0, sommetsRecopies = 0; };
Chrono& chrono();
// vrai si le kit remplace les boîtes de la carte (bâtiments posés et kit non désactivé)
bool actif();
// WFS-269 (accroche) : tous les bâtiments posés au niveau lod, dans le monde (ils cachent le ciel : DecorFixe::occultants)
void geometrie(int lod, std::vector<Vertex>& v, std::vector<uint32_t>& i);

} // namespace wfs::archi::carte
