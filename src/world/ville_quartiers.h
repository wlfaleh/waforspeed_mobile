// WaForSpeed — quartiers de la Ville (ticket WFS-93, épic E2) : port de generateDecor (js/world/decor.js) pour
// ZONES.city : Centre-ville, Quartier résidentiel, Parc urbain, Zone portuaire, Quartier d'affaires (= centre),
// Vieux quartier (= résidentiel), une zone de 448 segments chacune (src/world/zones.*).
// Un seul plan par carte, calculé une fois avec la graine de la carte (mulberry32(seed · 31 + 7)) : mêmes générateurs
// (city.centre, residentiel, parc, port), mêmes pas, mêmes tirages D() dans le même ordre que le web. Il est partagé :
//   - Ville (ville.cpp) : immeubles, lampadaires, arbres, passages piétons, abribus, parc, port, collisions du décor ;
//   - Commerces (commerces.cpp) : commerces devant les immeubles (placeShop) et clients attablés des terrasses ;
//   - Enseignes (enseignes.cpp) : panneaux néon du centre (DECOR.board).
#pragma once
#include <cstdint>
#include <vector>

namespace wfs {
struct MapDef;
namespace ville {

// objets du décor (deco.items du jeu web)
enum Type : uint8_t { IMMEUBLE, LAMPE, ARBRE_RUE, ZEBRA, ABRIBUS, GRILLE, ARBRE, BANC, FONTAINE, ENTREPOT, CONTENEURS, GRILLAGE, GRUE };

struct Objet {
    Type t = IMMEUBLE;
    int side = 1;
    float lat = 0, h = 0, len = 0, d = 0, along = 0;   // len : longueur le long de la route (immeuble, entrepôt), m
    int k = 0;              // immeuble : teinte (TINTS) ; arbre : 1 = pin ; entrepôt : couleur ; conteneurs : colonnes
    uint32_t graine = 0;    // conteneurs : graine des piles et des couleurs (mulberry32)
    bool balcons = false;   // immeuble du quartier résidentiel
};
// commerce (placeShop) : len = longueur de la vitrine (m) ; clients : bit 2 · table + (sd > 0) = client attablé ;
// decal : début de la vitrine après le début de l'immeuble (m), pour la centrer sur la façade
struct Commerce { int64_t seg; int side; float len; int kind; uint32_t clients; float decal; };
// panneau néon (DECOR.board) : texte = indice dans BOARD_TEXTS
struct Panneau { int64_t seg; int side, texte; };
// sprite de collision (collide du jeu web) : décalage en largeurs de chaussée (offset / RW), demi-largeur touchée
struct Choc { float offset, hit; };

struct Plan {
    std::vector<std::vector<Objet>> objets;   // par segment de la piste
    std::vector<std::vector<Choc>> chocs;     // par segment de la piste
    std::vector<Commerce> commerces;
    std::vector<Panneau> panneaux;
};

// plan de la ville, calculé au premier appel puis gardé (vide pour une autre carte)
const Plan& plan(const MapDef& map, int64_t segCount);

// bandes de sol propres à la zone (GROUND.city) : pelouse du parc, béton du port, de a à b m de l'axe, hauteurs
// ya → yb ; relevées de 5 cm et prolongées en pente jusqu'au sol de Game (qui reste dessous, sans se croiser).
// Renvoie le nombre de bandes (0 : centre et résidentiel gardent le sol et les trottoirs de Game).
struct Bande { float a, b, ya, yb; };
int bandes(const char* zone, Bande out[2]);
// hauteur du sol (groundY du jeu web) à lat m de l'axe dans la zone : trottoir (0,15 m) jusqu'à RW + 6, puis les
// bandes de la zone (au-delà : hauteur de fin de la dernière) ; profil du web (0,15 → −0,3 m) hors parc et port
float solY(const char* zone, float lat);

} // namespace ville
} // namespace wfs
