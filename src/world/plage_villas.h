#pragma once
// WaForSpeed — WFS-426 : villas de la Corniche (carte « plage ») posées dans le kit d'architecture sans carte graphique, pour la
// cuisson du paquet de la carte (cartes_v2::cuire, bloc « batiments » de WFS-416). Mêmes tirages et mêmes filtres que l'init de la
// carte (plage.cpp) : la signature des bâtiments cuits est donc celle que le jeu calcule au chargement. Ni lumière ni objet de choc déclaré ;
// la liste des bâtiments posés est vidée : l'appelant met de côté ceux de la partie en cours (archi::carte::mettreDeCote / remettre).
namespace wfs {
class Track;
namespace plage {
// t : tracé de la Corniche déjà construit (Track::build) ; vide puis remplit archi::carte::poses()
void poserVillasKit(Track& t);
} // namespace plage
} // namespace wfs
