// WaForSpeed — WFS-552 (épic E23, R19 maquette 01) : décor 3D du menu, le parking souterrain où la voiture du joueur est garée.
//
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : aucune forme géométrique nue. Aucun modèle libre CC0 / CC BY d'un parking
// souterrain aux vraies mesures n'est atteignable par les sources permises (regles-reseau.md : GitHub public, Sketchfab pour les
// voitures seulement) ; RG-02 du ticket : chaque pièce est donc modélisée par le code au niveau du vrai objet, aux mesures d'un
// parking réel (sources dans menu_parking.cpp) : poteaux à angles chanfreinés et bande jaune et noire peinte, poutres retombées,
// prédalles à joints en V, voiles en béton banché, dalle sciée en panneaux, marquage peint, butées de roue en caoutchouc,
// réglettes LED étanches (boîtier, diffuseur opale, embouts, clips), canalisation de sprinklers rouge avec suspentes et têtes.
// Un maillage par matériau, tout construit au chargement (init) ; rien n'est créé pendant l'affichage (RG-05).
// Repère : celui de la voiture du joueur (mètres, y vers le haut, sol en y = 0, avant de la voiture vers −Z, centre de la place
// en x = 0) ; la voiture est garée en marche arrière, l'avant vers l'allée.
#pragma once
#include <string>
#include "../render/renderer.h"
#include "menu_parking_cadrage.h"   // cadrage de la caméra rapprochée (calcul seul)

namespace wfs {
struct GameView;
struct SceneDediee;
struct CarDef;
namespace ecrans::parking {

// mesures du parking (m), d'après NF P 91-100 et les guides de conception des parcs de stationnement (menu_parking.cpp)
inline constexpr float HAUTEUR_DALLE = 2.60f;      // sous-face de la dalle
inline constexpr float RETOMBEE_POUTRE = 0.40f;    // poutre : hauteur libre sous poutre 2,20 m
inline constexpr float PLACE_L = 2.50f, PLACE_P = 5.00f, ALLEE = 6.00f;
inline constexpr float POTEAU = 0.50f;             // poteau carré de 50 cm, angles chanfreinés de 2 cm

class Parking {
public:
    // construit et envoie les maillages (une fois) ; faux si la mémoire graphique manque (rien n'est gardé : le menu garde le fond
    // d'avant, RG-05)
    bool init(Renderer& r);
    void shutdown(Renderer& r);
    bool pret() const { return pret_; }
    // caméra rapprochée de la maquette 01 (RG-04) et lumière du parking (RG-06 : la même de jour et de nuit) ; voiture = matrice
    // de la voiture du joueur dans le monde
    void regler(FrameParams& fp, SceneDediee& s, const GameView& g, const CarDef& car, const mat4& voiture) const;
    void dessiner(Renderer& r, const mat4& voiture);
    // triangles et appels de dessin du décor (journal du banc, RG-05)
    size_t triangles() const { return triangles_; }
    int appels() const;

    enum Mat : int { BETON, SOL, BLANC, JAUNE, NOIR, ROUGE, ALU, OPALE, CAOUTCHOUC, ACIER, LAITON, NB_MATS };

private:
    Mesh mesh_[NB_MATS]{};
    Material mat_[NB_MATS]{};
    bool pret_ = false;
    size_t triangles_ = 0;
};

} // namespace ecrans::parking
class Game;
namespace ecrans { int benchMenuParking(Game& g, Renderer& r, const std::string& out); }   // WFS_BANC=menu3d
} // namespace wfs
