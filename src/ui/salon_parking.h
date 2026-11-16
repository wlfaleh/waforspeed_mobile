// WaForSpeed — WFS-558 (épic E23, R19 maquette 04) : scène 3D de l'écran « Duel en ligne ». Le parking souterrain du menu
// (WFS-552, même instance : rien n'est construit deux fois), la voiture du joueur garée (dessinée par Game), une 2e voiture garée
// à sa droite à l'écran (la voiture de l'adversaire dès qu'il est connu, RG-03), et sur le poteau à sa gauche un digicode
// d'interphone mural en inox brossé (RG-02) surmonté de la plaque du pseudo (RG-07).
//
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : aucune forme géométrique nue. Chaque pièce est modélisée au niveau du vrai
// objet (sources et mesures dans salon_parking.cpp) : boîtier chanfreiné, afficheur à cristaux liquides sous sa vitre et son
// cadre, 36 touches chanfreinées en saillie (pavé 6 × 6 de RG-06), vis inviolables, gaine électrique et ses colliers, plaque en
// aluminium à bord embouti, rivets.
//
// Arbitrage du PO (RG-05, 2026-11-16) : le digicode est à vraie taille ; un toucher sur lui fait glisser la caméra (0,4 s, doux)
// tout près, où les touches se touchent ; « Retour » ramène d'abord au cadrage large. Les touches et l'afficheur sont projetés à
// l'écran à chaque image (zones touchables et textes : légendes gravées, code tapé, pseudo).
#pragma once
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
struct GameView;
namespace ecrans::duel3d {

// ---- pavé de RG-06 : 6 colonnes × 6 lignes de touches de 15 mm au pas de 19 mm ; 31 caractères (duel::CODE_CHARS dans l'ordre
// 2 à 9 puis A à Z sans I, L, O), puis ⌫ (2 colonnes) et OK (3 colonnes) sur la dernière ligne, après Z
inline constexpr int COLONNES = 6, LIGNES = 6, NB_TOUCHES = 33;   // 31 caractères + effacer + OK
inline constexpr int EFFACER = 31, OK = 32;                     // rangs des deux touches de fonction
// mesures (m) ; repère de la voiture du joueur : +x à gauche de l'écran (la caméra, dans l'allée, regarde vers +z) ; le poteau du
// digicode est celui de x = 4 m (trame du parking du menu), face à l'allée en z = 1,35 m
inline constexpr float PAS = 0.019f, DEMI_TOUCHE = 0.0075f;               // touche de 15 mm, pas de 19 mm (RG-06)
inline constexpr float FACE_L = 0.120f, FACE_H = 0.180f, EPAIS = 0.030f;   // boîtier (RG-02)
inline constexpr float Y_LIGNE0 = 0.0365f;                                 // centre de la 1re ligne (sous l'afficheur)
inline constexpr float Y_AFF = 0.0670f, AFF_L = 0.090f, AFF_H = 0.026f;    // cadre de l'afficheur
inline constexpr float POTEAU_X = 4.f, POTEAU_FACE = 1.60f - 0.25f, DIGI_Y = 1.62f;   // centre du digicode à 1,62 m (haut à 1,71 m)
inline constexpr float PLAQUE_DY = 0.30f;                                             // plaque : centre 30 cm au-dessus du digicode
inline constexpr float VOISINE_X = -2.50f;                                            // 2e voiture : place voisine, à droite à l'écran
// caractère de la touche (rang 0..30), en minuscule comme duel::CODE_CHARS ; 0 pour effacer et OK
char caractere(int touche);
// colonne de départ, largeur (en colonnes) et ligne de la touche
void place(int touche, int& col, int& largeur, int& ligne);

// rectangle à l'écran (pixels de l'interface)
struct Rect { float x = 0, y = 0, w = 0, h = 0; bool ok = false; bool contient(float px, float py) const { return ok && px >= x && px <= x + w && py >= y && py <= y + h; } };

// état de la vue de l'accueil (partagé avec l'écran Salon) : 0 = cadrage large de la maquette, 1 = tout près du digicode
struct Vue {
    float t = 0;            // avancement de la caméra (0 → 1 en 0,4 s, adouci)
    bool proche = false;    // destination demandée
    bool pret = false;      // décor préparé (sinon : écran 2D d'avant, repli)
    bool active = false;    // la scène 3D est montrée à cette image (écran Salon, accueil ou salon flouté)
    Rect digicode;          // boîtier du digicode à l'écran (toucher en vue large → vue rapprochée)
    Rect touches[NB_TOUCHES];   // touches à l'écran (vue rapprochée seulement ; vides sinon)
    bool rapprochee() const { return proche && t > 0.98f; }
};
Vue& vue();
// code en cours de saisie sur le digicode (celui de l'écran Salon, 0 à 4 caractères, minuscules)
const std::string& codeSaisi();
void rapprocher(bool proche);   // demande la vue rapprochée (true) ou large (false)

// ---- caméra (repère de la voiture du joueur, mètres) : t = 0 cadrage large, t = 1 vue rapprochée ; calcul seul (testé)
struct Camera { vec3 oeil, cible; float fovY; };
Camera camera(float t);
// adoucissement du glissé (0,4 s) : avance de t vers la destination, sans saut
float avancer(float t, bool proche, float dt);

// centre et demi-mesures d'une touche, dans le repère du digicode (m) : x vers la droite vu de face, y vers le haut, z sorti du
// poteau (face avant du boîtier en z = 0,030) ; calcul seul (testé : 15 mm, pas de 19 mm, tout dans 120 × 180 mm)
void touche(int k, vec3& centre, vec2& demi);

} // namespace ecrans::duel3d
} // namespace wfs
