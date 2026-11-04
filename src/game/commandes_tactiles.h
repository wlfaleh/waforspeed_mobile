// WaForSpeed — commandes tactiles de la course (ticket WFS-98, épic E3) : pédales « GAZ » / « FREIN » et bouton « NITRO »
// du jeu web (index.html .pedal-zone ; css/style.css .pedal, .pedal-fill, .pedal-icon, .pedal-label, .nitro-touch, .nt-ring,
// .nt-icon, .nt-label ; téléphone couché @media (max-height: 500px) : nitro 68 px, frein 84 × 88, gaz 64 × 120, écarts 10 px).
// Aussi : primitives de dessin 2D à bords lissés (dessin2d), partagées avec le volant (direction.cpp) et les boutons du HUD
// (camera_vue.cpp). Tout est dessiné en uiQuad / uiCircle (WFS-65) : frange d'un pixel pour lisser les bords, aucun shader.
#pragma once
#include <cmath>
#include "../core/math.h"
#include "../core/system.h"   // Marges

namespace wfs {

class Renderer;

// ---------- fonctions pures (tests/test_course.cpp) ----------
enum class Commande { Aucune, Gaz, Frein, Nitro };

struct Cadre {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    float cx() const { return x + w / 2; }
    float cy() const { return y + h / 2; }
};

// place des commandes dessinées, pour un droitier (le gaucher passe par ecrans::miroirX) ; m = marges des commandes
// (ecrans::margesCommandes) ; 1 px CSS = k = H / 720 pixels ; gouttières 16 px à droite et 14 px en bas (WFS-52) ;
// coordonnées arrondies au pixel, tailles arrondies au-dessus (RG-05 : jamais moins que la taille CSS × k). Nitro : disque
// inscrit dans son cadre.
struct DispositionCommandes { Cadre gaz, frein, nitro; float k = 1; };
inline DispositionCommandes dispositionCommandes(float W, float H, const Marges& m) {
    DispositionCommandes d;
    const float k = H / 720.f, bas = std::round(H - m.bas - 14 * k), ecart = std::round(10 * k);
    const float gw = std::ceil(64 * k), gh = std::ceil(120 * k), fw = std::ceil(84 * k), fh = std::ceil(88 * k), n = std::ceil(68 * k);
    d.k = k;
    d.gaz = {std::round(W - m.droite - 16 * k) - gw, bas - gh, gw, gh};
    d.frein = {d.gaz.x - ecart - fw, bas - fh, fw, fh};
    d.nitro = {d.frein.x - ecart - n, bas - n, n, n};
    return d;
}

// commande sous un doigt posé en (x, y) (x déjà passé par ecrans::miroirX). RG-05 / RG-05 bis : chaque dessin est dans sa
// zone, et la zone déborde du dessin pour le confort du pouce : au-dessus des pédales = gaz (le point (0,89 W ; 0,74 H) des
// bancs reste au gaz), à gauche du nitro = frein (comme avant WFS-98), coin haut droit (au-dessus de 0,45 H, 0,53 H pour
// un gaucher) = nitro hérité de l'ancien bouton (appui du banc nitro-lignes). Moitié gauche de l'écran : rien (direction).
inline Commande commandeSous(float x, float y, float W, float H, const Marges& m, bool gaucher) {
    if (x <= W * 0.55f - m.droite) return Commande::Aucune;
    const DispositionCommandes d = dispositionCommandes(W, H, m);
    const float k = d.k, rn = d.nitro.w / 2, dx = x - d.nitro.cx(), dy = y - d.nitro.cy();
    if (dx * dx + dy * dy <= rn * rn) return Commande::Nitro;
    if (d.frein.contient(x, y)) return Commande::Frein;
    if (d.gaz.contient(x, y)) return Commande::Gaz;
    if (x > W * 0.80f - m.droite && y < H * (gaucher ? 0.53f : 0.45f)) return Commande::Nitro;
    if (x < d.nitro.x - 24 * k) return Commande::Frein;
    if (x < d.frein.x - 5 * k) return y >= d.nitro.y - 24 * k ? Commande::Nitro : Commande::Gaz;
    if (x < d.gaz.x - 5 * k) return y >= d.frein.y - 24 * k ? Commande::Frein : Commande::Gaz;
    return Commande::Gaz;
}

// ---------- accroche de Game::drawControls ----------
// état affiché : pédales et nitro appuyés, réserve de nitro (0..1), nitro en marche
struct EtatCommandes { bool gaz = false, frein = false, nitro = false; float reserve = 0; bool nitroActive = false; };
// dessine les pédales et le nitro dans la zone sûre (marges de WFS-51), en miroir pour un gaucher (WFS-75)
void commandesDessiner(Renderer& r, float W, float H, const Marges& marges, const EtatCommandes& e);

// ---------- dessin 2D à bords lissés ----------
// Couleurs en codes sRGB 0..1, alpha non prémultiplié (comme Renderer::uiRect). Angles en radians : 0 en haut, sens horaire.
namespace dessin2d {
// dégradé vertical : `haut` en y0, `bas` en y1 (linear-gradient(180deg, haut, bas) du web)
struct Teinte {
    vec4 haut, bas; float y0 = 0, y1 = 1;
    vec4 en(float y) const;
};
Teinte uni(const vec4& c);
// polygones convexes (64 sommets au plus), sommets dans le sens horaire à l'écran (y vers le bas)
void polygone(Renderer& r, const vec2* p, int n, const Teinte& t);       // remplissage
void contour(Renderer& r, const vec2* p, int n, float ep, const vec4& c);  // bordure intérieure de ep pixels (border)
void halo(Renderer& r, const vec2* p, int n, float flou, const vec4& c);   // lueur extérieure (box-shadow), c → transparent
// parallélogramme des boutons (clip-path des .cam-btn, .steer-pad, .arrow-btn) : haut décalé à droite de `biais`
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const Teinte& t);
// cercles
void couronne(Renderer& r, float cx, float cy, float r0, float r1, const vec4& c0, const vec4& c1);   // dégradé radial, sans bord
void anneau(Renderer& r, float cx, float cy, float r0, float r1, const vec4& c, float a0 = 0, float a1 = 2 * PI);
// disque de rayon R rempli d'un dégradé radial (radial-gradient) : c0 au centre (gx, gy), c1 à gr et au-delà
void disqueRadial(Renderer& r, float cx, float cy, float R, float gx, float gy, float gr, const vec4& c0, const vec4& c1);
// traits opaques à bouts et jointures ronds (stroke-linecap / stroke-linejoin: round)
void trait(Renderer& r, const vec2* p, int n, float ep, const vec4& c);
void arc(Renderer& r, float cx, float cy, float rayon, float ep, float a0, float a1, const vec4& c);
} // namespace dessin2d

} // namespace wfs
