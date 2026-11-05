// WaForSpeed — échelle de l'interface à la taille du téléphone (ticket WFS-102, épic E8).
// 1 px « CSS » du jeu web = 1 dp, comme le navigateur d'un téléphone : un bouton de 44 px du web mesure 44 dp
// (≈ 7 mm) sur tous les écrans, au lieu de suivre la seule hauteur en pixels (ancienne règle H / 720).
//   k = ui::echelle(W, H) : pixels de l'interface par px CSS = H / clamp(H / d, 360, 720), d = densité (pixels par dp).
//   Densité : celle lue par WFS-108 (densiteCommandes(), Android) ; banc PC : 2,625 si W ≥ 2000 (420 dpi), sinon 1
//   (captures 1280x720 inchangées, k = 1).
// Aussi : ui::biseau, le parallélogramme des boutons (.btn, clip-path skewX(-10deg)) d'un seul tenant (plus de rayures).
#pragma once
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace ui {

float densite(float W, float H);          // pixels de l'interface par dp
float echelle(float W, float H);          // pixels de l'interface par px CSS (référence des écrans)
float hauteurDp(float W, float H);        // hauteur de l'écran en px CSS (≈ dp), bornée à [360, 720]
inline bool compact(float W, float H) { return hauteurDp(W, H) <= 520.f; }   // @media (max-height: 520px) du web (RG-02)
constexpr float CIBLE_MIN = 44.f;         // RG-03 : zone de toucher minimale, en px CSS (44 × 44 dp)

// parallélogramme : haut décalé à droite de `biais`, coin bas droit coupé (bords lissés, un seul polygone)
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c);

} // namespace ui
} // namespace wfs
