// WaForSpeed — WFS-548 (épic E23, interface R19) : dessin commun des fenêtres de l'interface 2D.
// Un seul panneau de verre dépoli (RG-01), le fond flouté des écrans arrêtés (RG-02), les sur-titres gris (RG-03),
// le dégradé de la marque cyan → orange (RG-04) et les couleurs d'état (RG-05), réutilisés par tous les écrans :
// pause, profil, création du profil, CGU, salon, achat, fins de course, réglages, Market, chargement.
// Maquettes : references/maquettes-R19/ ; règles : regles-de-gestion/WFS-548.md. Interface 2D seulement (skill
// design-street-racing) : le monde 3D n'est pas concerné.
#pragma once
#include <string>
#include "../core/math.h"
#include "text.h"

namespace wfs {
class Renderer;
namespace verre {

// code sRGB 0xRRGGBB → couleur de l'interface (mêmes valeurs que les jetons hex() des écrans)
inline vec4 hexa(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }

// RG-05 : couleurs d'état, mêmes valeurs partout
constexpr unsigned TOI = 0xFF8A3D;      // orange = toi (ta ligne d'un classement, ton pseudo dans le salon)
constexpr unsigned PERTE = 0xFF4D6D;    // rouge = perte (course perdue, « Quitter la course » bordé de rouge)
constexpr unsigned PRET = 0x5BE38A;     // vert = prêt (salon)
// RG-03 : sur-titres gris (maquettes 19 à 33) ; intertitres des CGU en orange clair (maquette 22)
constexpr unsigned SUR_TITRE = 0xA3A8B5;
constexpr unsigned INTERTITRE = 0xFFB27A;
// RG-04 : dégradé de la marque, mêmes valeurs que la barre de chargement (--accent-2 → --accent)
constexpr unsigned DEGRADE_DEBUT = 0x00E5FF, DEGRADE_FIN = 0xFF6A00;
// t de 0 (cyan) à 1 (orange), interpolation linéaire des codes sRGB (comme la barre de chargement d'avant)
inline vec4 degradeMarque(float t, float a = 1) {
    const vec4 c0 = hexa(DEGRADE_DEBUT), c1 = hexa(DEGRADE_FIN);
    const float u = t < 0 ? 0 : t > 1 ? 1 : t;
    return {c0.x + (c1.x - c0.x) * u, c0.y + (c1.y - c0.y) * u, c0.z + (c1.z - c0.z) * u, a};
}
// barre horizontale remplie du dégradé de la marque (x0 → x1), en 16 tranches
void barreMarque(Renderer& r, float x, float y, float w, float h, float a = 1);

// RG-01 : style d'un panneau
struct Style {
    float rayon = 14;          // rayon des coins (pixels de l'interface, déjà à l'échelle)
    float lisere = 1.5f;       // épaisseur du liseré clair (1 à 1,5 px en 1080p)
    float opacite = 1;         // fondu d'entrée de l'écran (multiplie tout)
    bool ombre = true;         // ombre portée douce autour du panneau
    unsigned bord = 0;         // 0 : liseré blanc ≈ 20 % ; sinon liseré de cette couleur (état : rouge, orange, vert)
    float force = 1;           // 1 : panneau ; < 1 : tuile plus légère posée sur un panneau
};
// panneau de verre dépoli : dégradé radial (plus clair au centre-haut, plus sombre aux bords), liseré fin, coins arrondis
void panneau(Renderer& r, float x, float y, float w, float h, const Style& s = {});
// sur-titre gris en majuscules espacées (petit titre au-dessus d'un bloc) ; y : haut du texte
void surTitre(Renderer& r, float x, float y, float px, const std::string& texte, ui::Align align = ui::GAUCHE, float opacite = 1);

// RG-02 : fond derrière les fenêtres. À appeler à chaque image d'un écran arrêté (dans drawUi) : la demande vaut pour l'image
// suivante ; le moteur floute alors la scène une seule fois (chaîne du bloom) et relit l'image floutée tant que la demande dure.
// Jamais en course (bandeaux, HUD) : fond assombri seulement (assombrir).
void fondFloute(Renderer& r);
// voile sombre plein écran (sur le fond flouté, ou seul en course)
void assombrir(Renderer& r, float W, float H, float a);

} // namespace verre
} // namespace wfs
