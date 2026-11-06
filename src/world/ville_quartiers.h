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
class Track;
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

// WFS-137 (RG-01, RG-02) : lieux du tracé 2. Carrefour à feux : ligne d'arrêt, passage de la rue transversale (8 m après),
// indice du feu (trace::feuA). Rond-point : segment du centre de l'anneau (courbure la plus forte), début et fin de la section
struct Carrefour { int64_t ligne, croise; int feu; };
struct RondPoint { int64_t centre, debut, fin; float rayon; };
constexpr float RUE_TRANS_M = 4.5f;   // demi-largeur de la rue transversale d'un carrefour (2 voies, 9 m)

struct Plan {
    std::vector<std::vector<Objet>> objets;   // par segment de la piste
    std::vector<std::vector<Choc>> chocs;     // par segment de la piste
    std::vector<Commerce> commerces;
    std::vector<Panneau> panneaux;
    // WFS-137 (RG-C1) : latéral maximal du décor (m) de chaque côté de chaque segment, côté intérieur des courbes serrées
    // du tracé 2 (90 % du rayon, sur les 60 m qui suivent : rien ne cache un virage serré) ; infini ailleurs ou au tracé 1
    std::vector<float> limG, limD;
    // WFS-137 (RG-03) : segments de l'autoroute urbaine, du tunnel et du pont (pas de décor de rue sur ces segments)
    std::vector<uint8_t> voie;   // 0 rue, 1 autoroute, 2 tunnel, 3 pont
    std::vector<Carrefour> carrefours;   // WFS-137 (RG-02)
    std::vector<RondPoint> ronds;        // WFS-137 (RG-01)
    bool adapte = false;
    float lim(int64_t a, int side) const;   // limG / limD du segment a (ramené sur un tour) ; infini si non adapté
};
enum Voie : uint8_t { RUE = 0, AUTOROUTE = 1, TUNNEL = 2, PONT = 3 };

// plan de la ville, calculé au premier appel puis gardé (vide pour une autre carte)
const Plan& plan(const MapDef& map, int64_t segCount);
// WFS-137 (RG-C1) : plan adapté aux sections du tracé 2 de la piste (une seule fois ; sans effet au tracé 1) : retire les objets,
// commerces, panneaux et chocs qui retomberaient sur la chaussée côté intérieur d'une courbe serrée ou cacheraient le virage
// dans les 60 m qui le précèdent, et le décor de rue de l'autoroute, du tunnel et du pont. À appeler par la Ville (ordre 100)
// avant les commerces (109) et les enseignes (107), qui lisent le même plan.
const Plan& adapter(const MapDef& map, const Track& t);
// WFS-137 (RG-02) : piétons des passages zébrés (4 par carrefour à feux, ligne d'arrêt + 1,4 à 2,8 m) : s (segment), latéral de
// départ (m), vitesse (m/s), dir : côté du trottoir visé (±1), feu du carrefour ; traverser() les fait passer d'un trottoir à
// l'autre au rouge des voitures (ou finir de traverser), attendre au vert ; orientation (yaw relatif à la route) et hauteur du sol
struct Passant { double s, lat, v; int dir, feu; };
std::vector<Passant> passants(const MapDef& map, const Track& t);
void traverser(double& lat, int& dir, bool& marche, float& yawRel, double& y, int feu, double v, double dt);
// extension latérale d'un objet (m depuis l'axe) : bord extérieur du volume (immeuble, arbre, grue…)
float etendue(const Objet& o);

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
