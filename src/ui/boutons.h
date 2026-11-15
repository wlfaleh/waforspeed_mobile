// WaForSpeed — boutons de l'interface au modèle R19 (WFS-549, épic E23) : un seul dessin commun pour tous les écrans.
// Maquettes : references/maquettes-R19/ (01 menu, 14 Réglages, 19 pause, 27 achat…) ; règles : regles-de-gestion/WFS-549.md.
//   - startStop     : action principale de l'écran (RG-01), bouton de démarrage d'une voiture : bague chrome, centre sombre,
//                     liseré orange allumé, icône « marche » et texte en majuscules ;
//   - autocollant   : actions de sortie (Retour, Quitter, Annuler, Plus tard ; RG-02), étiquette blanche inclinée ;
//   - piluleChrome  : « Showroom / Garage » du menu (RG-03), bague chrome et liseré cyan ;
//   - prix          : boutons de prix (« ★ 6 000 », « 1,99 € », Inviter ; RG-04), orange, texte sombre ;
//   - selecteur     : rangée de verre d'un choix à plusieurs valeurs (RG-05, RG-06), choix actif = pilule blanche, texte sombre ;
//   - verreOrange   : bouton de verre cerclé d'orange (« ✕ Fermer » des Réglages, RG-06).
// Chaque fonction dessine DANS le rectangle (x, y, w, h) donné : la zone de toucher et la taille de chaque bouton restent celles
// de l'écran qui l'appelle (RG-07). L'appui (RG-08) se montre par `appuye` (bouton enfoncé : centre plus clair, liseré plus vif) ;
// le déclenchement unique reste celui de chaque écran (src/ui/appui.h). Interface 2D : skill design-street-racing.
// Couleurs en codes sRGB 0..1 (comme Renderer::uiRect) ; op = opacité de l'écran (fondus d'entrée).
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace ui::boutons {

// contour d'un rectangle aux coins arrondis de rayon `rayon` (borné à min(w, h) / 2 : pilule), sommets dans le sens horaire
// à l'écran ; rend le nombre de sommets écrits dans p (MAX_SOMMETS au plus). Calcul seul : testé par tests/test_boutons.cpp.
constexpr int MAX_SOMMETS = 64;
inline int contourArrondi(vec2* p, float x, float y, float w, float h, float rayon) {
    if (w <= 0 || h <= 0) return 0;
    const float R = std::clamp(rayon, 0.f, std::min(w, h) / 2);
    if (R < 0.5f) {   // coins vifs : 4 sommets
        p[0] = {x, y}; p[1] = {x + w, y}; p[2] = {x + w, y + h}; p[3] = {x, y + h};
        return 4;
    }
    // coins : centres TR, BR, BL, TL ; angles en repère écran (y vers le bas) : croissants = sens horaire
    const int seg = std::clamp((int)std::ceil(R / 3.f), 3, MAX_SOMMETS / 4 - 1);   // arêtes par quart de cercle
    const vec2 c[4] = {{x + w - R, y + R}, {x + w - R, y + h - R}, {x + R, y + h - R}, {x + R, y + R}};
    int n = 0;
    for (int k = 0; k < 4; k++) {
        const float a0 = -0.5f * PI + k * 0.5f * PI;
        for (int i = 0; i <= seg; i++) {
            const float a = a0 + 0.5f * PI * i / seg;
            const vec2 q{c[k].x + R * std::cos(a), c[k].y + R * std::sin(a)};
            // deux coins qui se touchent (pilule) : le dernier sommet d'un coin est le premier du suivant
            if (n > 0 && std::fabs(q.x - p[n - 1].x) < 1e-3f && std::fabs(q.y - p[n - 1].y) < 1e-3f) continue;
            p[n++] = q;
        }
    }
    if (n > 1 && std::fabs(p[0].x - p[n - 1].x) < 1e-3f && std::fabs(p[0].y - p[n - 1].y) < 1e-3f) n--;
    return n;
}

// état inactif (revue #420, point 4) : jamais une simple transparence (texte illisible, bouton qui a l'air actif) ; couleurs pleines
// neutres, contraste texte / fond ≥ 4,5 : 1 (vérifié par tests/test_boutons.cpp, formule WCAG 2.1)
constexpr unsigned INACTIF_TEXTE = 0xB4B9C6,      // texte des boutons inactifs (prix, Start/Stop, sélecteurs)
                   INACTIF_FOND_HAUT = 0x2C2F3A, INACTIF_FOND_BAS = 0x23262F,   // fond du bouton de prix inactif
                   INACTIF_CENTRE = 0x15171D,     // centre sombre du Start/Stop inactif (bas du dégradé, le plus clair)
                   INACTIF_LISERE = 0x5A5F6C,     // liseré éteint (gris au lieu d'orange)
                   INACTIF_VERRE = 0x1A1C24;      // rangée de verre sur le panneau (pire cas du sélecteur, texte clair)
// contraste WCAG 2.1 entre deux couleurs 0xRRGGBB (1 à 21)
inline float contraste(unsigned a, unsigned b) {
    auto lum = [](unsigned c) {
        auto l = [](unsigned v) { const float s = v / 255.f; return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f); };
        return 0.2126f * l((c >> 16) & 255) + 0.7152f * l((c >> 8) & 255) + 0.0722f * l(c & 255);
    };
    const float la = lum(a), lb = lum(b);
    return (std::max(la, lb) + 0.05f) / (std::min(la, lb) + 0.05f);
}

// RG-01 : titre (une ligne, capitales) ; sousTitre facultatif (« ★ 6 000 », « Solo ») ; px = hauteur du titre (0 : d'après h)
void startStop(Renderer& r, float x, float y, float w, float h, const std::string& titre, float op = 1, bool appuye = false,
               const std::string& sousTitre = "", float px = 0, bool grise = false);
// RG-01, texte posé par l'appelant (prix avec étoile dessinée, texteEtoiles du garage) : bague, liseré et icône à gauche ;
// rend la zone libre pour le texte (x0, largeur), à droite de l'icône
struct ZoneTexte { float x = 0, w = 0; };
ZoneTexte startStopFond(Renderer& r, float x, float y, float w, float h, float op = 1, bool appuye = false, bool grise = false);
// RG-02 : texte (capitales) ; sousTitre facultatif (« Garder mes étoiles »)
void autocollant(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                 float px = 0, const std::string& sousTitre = "");
// RG-03 : pilule chrome (liseré cyan, petite voiture dessinée au trait à gauche)
void piluleChrome(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                  float px = 0, const std::string& sousTitre = "");
// RG-04 : bouton de prix orange, texte sombre (le texte peut contenir « ★ », dessinée par l'appelant s'il le faut)
void prix(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
          float px = 0, bool grise = false);
// RG-05 : rangée de verre (fond seul) puis un choix ; le choix actif est une pilule blanche à texte sombre
void selecteurFond(Renderer& r, float x, float y, float w, float h, float op = 1);
void selecteurChoix(Renderer& r, float x, float y, float w, float h, const std::string& texte, bool actif, float op = 1,
                    bool appuye = false, float px = 0, bool grise = false);
// RG-06 : bouton de verre cerclé d'orange (texte orange)
void verreOrange(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                 float px = 0);
// bouton de verre neutre (bord clair) : actions secondaires des maquettes (« Crédits », « Conditions d'utilisation »)
void verre(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
           float px = 0, const vec4* couleurTexte = nullptr);

} // namespace ui::boutons
} // namespace wfs
