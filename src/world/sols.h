// WaForSpeed — WFS-383 (E12 / E15, ANO-224) : sols des cartes (terrain, talus, collines).
// Le sol n'est plus une couleur unie en facettes : le shader (shaders/sols.glsl, surfaceTerrain) mélange pré, terre / maquis et
// roche selon la pente (< 25°, 25 à 40°, > 40°), avec des taches et un grain, en couleurs mesurées. Ce fichier donne :
//   - le matériau de sol d'une carte (MAT_GROUND, code du sol dans cutoff) ;
//   - le lissage des normales d'un terrain construit en triangles séparés (plus de facettes visibles, RG-01) ;
//   - le climat de chaque carte (cartes du code, cartes de données par leur ligne « sol » du bloc decor).
// Couleurs du shader (albédo sRGB linéaire) et sources :
//   pré tempéré 0x6B7F3A (RG-02 ; « herbe 0,15-0,25 » de DA-realisme.md § 3 ; mat.herbe.albedo de references/reel/materiaux.json,
//   0,105 / 0,133 / 0,041, PB-MAT, Spectral Materials Database) ; herbe sèche 0x9A8F5A ; terre brune 0x6E5A40 ;
//   méditerranéen : herbe sèche 0x9A9262, garrigue 0x5A6238, terre rouge 0x8C6D4E, maquis 0x4A5330, calcaire gris-beige chaud 0xB8AC94 / 0x998E79
//   (« pierre claire 0,40-0,55 » de DA-realisme.md : 0,50 de luminance moyenne) ; automne : pré fané 0x7E7C48, feuilles mortes
//   0x7A4E2A, 0x9C6A30, 0xA88A3A, 0x5E4630, terre 0x5E4A36 ; roche grise 0x8E8A80 / 0x75716A.
#pragma once
#include <vector>
#include "../render/renderer.h"

namespace wfs {
namespace sols {

enum class Climat { Mediterraneen = 1, Tempere = 2, Automne = 3 };

// code du sol lu par le shader (pc.params.z) : climat + 4 × teinte + 8 × rocheux ; WFS-594 (RG-05) : + 16 × sable (couleur de la palette
// gardée, rides de vent, grain), + 32 × neige (neige fraîche au sol), + 64 × bord de route (uv.x = latéral en m : bande de terre et de
// gravier de BANDE_GRAVIER_MIN_M à BANDE_GRAVIER_MAX_M au pied du trottoir de Game, qui finit à BORD_TROTTOIR_M de l'axe)
struct Options { bool teinte = false, rocheux = false, sable = false, neige = false, bordRoute = false, gazon = false; };   // gazon : + 128 (pelouse arrosée de la Ville)
constexpr float code(Climat c, bool teinte, bool rocheux, bool sable = false, bool neige = false, bool bordRoute = false, bool gazon = false) {
    return (float)((int)c + (teinte ? 4 : 0) + (rocheux ? 8 : 0) + (sable ? 16 : 0) + (neige ? 32 : 0) + (bordRoute ? 64 : 0) + (gazon ? 128 : 0));
}
constexpr float BORD_TROTTOIR_M = 11.5f;   // RW (7,5) + WALK_W (4) de game.cpp ; = shaders/sols.glsl (test_sols)
constexpr float BANDE_GRAVIER_MIN_M = 0.3f, BANDE_GRAVIER_MAX_M = 0.8f;   // RG-05 : 30 à 80 cm ; = shaders/sols.glsl

// matériau de sol : MAT_GROUND (pas d'ombre portée, pas de reflet, jamais retourné), rugosité 0,95 ; teinte = le pré prend la couleur
// des sommets (texture de palette passée par l'appelant), sinon la couleur du climat ; rocheux = la pente lue est augmentée de 30° × uv.x du
// sommet (part de roche de 0 à 1, posée par l'appelant : zone de rochers de la Corniche, roche dès 15°)
Material materiau(Climat c, bool teinte = false, bool rocheux = false);
// WFS-594 (RG-05) : matériau de sol avec les options (sable, neige, bord de route) ; mêmes drapeaux et rugosité que materiau()
Material materiau(Climat c, const Options& o);
// WFS-594 : neige au sol de la carte (cartes de données dont la saison couvre la chaussée de neige) : leurs bas-côtés passent au sol
// naturel couvert de neige (avant : bandes blanches unies)
bool neigeCarte(const char* carte);

// lissage des normales d'un terrain fait de triangles séparés : sommets soudés (même position à 1 cm près, même uv), normale moyenne
// pondérée par l'angle du coin, tournée vers le haut. Les sommets soudés sont fusionnés (moins de sommets à envoyer). Thread de travail possible.
void lisserNormales(std::vector<Vertex>& v, std::vector<uint32_t>& i);

// climat des cartes du code (Ville, Campagne, Grand Prix : tempéré ; Corniche : méditerranéen) ; carte de données : sa ligne
// « sol <mediterraneen|tempere|automne> » du bloc decor. Faux (c inchangé) : carte de données sans ligne « sol » (bandes unies
// gardées, comme avant), sauf une carte enneigée (WFS-594 : tempéré, recouvert par la neige : neigeCarte)
bool climatCarte(const char* carte, Climat& c);

} // namespace sols
} // namespace wfs
