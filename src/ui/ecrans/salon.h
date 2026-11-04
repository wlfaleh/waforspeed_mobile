// WaForSpeed — écran Salon du duel en ligne (WFS-83, épic E3 ; nouveaux fichiers autorisés dans src/ui/ecrans).
// Section #lobby du jeu web (index.html, js/net/duel.js : renderLobby, renderOpenRooms, createRoom, joinRoom, setReady,
// leaveRoom) : accueil (pseudo, « Créer un salon », « Salons qui t'attendent » vus sur le Wi-Fi, note d'état, « Retour »)
// puis salon (code, voiture, terrain de l'hôte, distance réglée par l'hôte, joueurs et statut, « Prêt », « Quitter le
// salon »). Quand les deux pilotes sont prêts : course (écran de chargement de WFS-74 si la carte de l'hôte n'est pas
// chargée), puis DuelSystem (WFS-21) fait la manche. Transport : reseau::TransportUdp (src/game/reseau_udp.*).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
// banc d'essai : menu-duel, salon-accueil, salon-hote, salon-pret, duel-reseau-course ; le second joueur passe par
// TransportUdp en boucle locale (127.0.0.1)
void benchSalon(Game& g, Renderer& r, const std::string& out);
} // namespace ecrans
} // namespace wfs
