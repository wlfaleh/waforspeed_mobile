// WaForSpeed — garage : icône du pistolet de peinture de l'utilisateur (WFS-551, RG-03, épic E23 interface R19, maquette 16).
// L'icône fournie par l'utilisateur (references/maquettes-R19/icone-pistolet-utilisateur.png, copiée en 256 px dans
// assets/ui/icone-pistolet.png) est reprise telle quelle : le pistolet blanc ne change pas, le godet et le jet (parties colorées de
// l'image) prennent la finition dans la couleur de la voiture.
// Mise en œuvre (interface 2D, aucun nouvel appel de dessin ni nouvelle texture) : au chargement de la police, l'image est rangée
// dans les canaux G, B et A de l'atlas de police (le texte ne lit que le canal R), sous forme de masques :
//   G = luminosité du pixel (le pistolet blanc et ses ombres, le noir du contour), B = part colorée (godet, jet), A = opacité.
// Le shader d'interface (shaders/ui.frag, u décalé de +4) recompose : pistolet = G en gris, partie colorée = teinte × G.
// La teinte vient de la couleur des sommets : un dégradé entre les 4 coins donne les finitions (caméléon, chromé, nacré…).
#pragma once
#include <cstdint>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace ecrans::garage::pistolet {

constexpr int COTE = 256;   // côté de l'icône dans l'atlas (texels)

// Calcul seul (testé par tests/test_pistolet.cpp) : masques d'un pixel RGBA (codes sRGB 0..255) de l'icône → G, B, A.
struct Masque { uint8_t g, b, a; };
inline Masque masque(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const int mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    const float v = mx / 255.f, s = mx > 0 ? (float)(mx - mn) / (float)mx : 0.f;
    // saturation sous 0,18 : blanc, gris, noir (pistolet, contour, halo sombre) ; au-dessus de 0,45 : couleur pleine (godet, jet)
    const float part = s <= 0.18f ? 0.f : s >= 0.45f ? 1.f : (s - 0.18f) / (0.45f - 0.18f);
    return {(uint8_t)(v * 255.f + 0.5f), (uint8_t)(part * 255.f + 0.5f), a};
}

// Accroche du chargement de la police (src/ui/text.cpp) : lit assets/ui/icone-pistolet.png et range ses masques dans le coin bas
// droit de l'atlas RGBA (côté `cote`), si les caractères s'arrêtent avant (yLibre = 1re ligne libre de l'atlas). Sinon, ou si l'image
// manque, l'icône reste absente (journal) : dessiner() ne dessine rien et la rangée garde ses échantillons.
void ajouterAtlas(uint8_t* rgba, int cote, int yLibre);
bool pret();
// Dessine l'icône dans le carré (x, y, s) de l'interface. teinte[4] : couleur du godet et du jet aux coins haut gauche, haut droit,
// bas droit, bas gauche (codes sRGB 0..1, alpha = opacité).
void dessiner(Renderer& r, float x, float y, float s, const vec4 (&teinte)[4]);

} // namespace ecrans::garage::pistolet
} // namespace wfs
