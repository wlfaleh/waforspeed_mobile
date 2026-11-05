// WaForSpeed — modes de direction tactiles (WFS-70) : voir direction.h.
// La partie « volant » de Game::onTouch et de Game::drawControls a été déplacée ici sans changement (mode glissement) ;
// WFS-98 : dessin du jeu web (pavé vitré .steer-pad, bouton volant .steer-knob, flèches .arrow-btn), bords lissés (dessin2d).
#include "direction.h"
#include <algorithm>
#include <cmath>
#include "../render/renderer.h"
#include "commandes_tactiles.h"   // WFS-98 : dessin2d (bords lissés)
#include "game.h"
#include "../ui/ecrans/reglages.h"   // WFS-75 : miroirX (gaucher)

namespace wfs {

namespace {
// zone de glissement : doigt du volant, point de départ et position du pouce (steerUI.id, x0 du jeu web)
struct Glisser { int id = -1; float x0 = 0, x = 0; } g_gl;
// flèches : doigt posé sur chaque bouton (dataset.pid des boutons .arrow-btn)
struct Fleches { int gauche = -1, droite = -1; } g_fl;

bool fleches() { return reglagesRessenti().direction == ModeDirection::Fleches; }
BoutonsFleches boutonsPlaces(float W, float H, const Marges& m) {   // WFS-75 : groupe passé par miroirX, flèche gauche à gauche
    BoutonsFleches b = boutonsFleches(W, H, ecrans::margesCommandes(m)); const float cx = (b.xg + b.xd + b.c) / 2, d = ecrans::miroirX(cx, W) - cx;
    b.xg += d; b.xd += d; return b; }

// couleurs du skill design-street-racing (tokens.md, codes sRGB comme ui::CYAN)
const vec4 VITRE_HAUT{30 / 255.f, 33 / 255.f, 48 / 255.f, 0.62f};   // .steer-pad : linear-gradient(180deg, rgba(30,33,48,.62),
const vec4 VITRE_BAS{14 / 255.f, 16 / 255.f, 24 / 255.f, 0.72f};    //   rgba(14,16,24,.72))
const vec4 BORD{1.f, 1.f, 1.f, 0.1f};                                // 1px solid rgba(255,255,255,.1)
const vec4 ACCENT2{0.f, 0.898f, 1.f, 1.f};                           // --accent-2 #00E5FF (appui, piste braquée, repère)
const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1.f};        // --text #F2F4F8 (volant)
const vec4 TEXTE_2{138 / 255.f, 144 / 255.f, 166 / 255.f, 1.f};      // --text-2 #8A90A6 (chevrons)
const vec4 SUR_ACCENT{10 / 255.f, 11 / 255.f, 16 / 255.f, 1.f};      // --on-accent #0A0B10
vec4 alpha(vec4 c, float a) { c.w = a; return c; }

// chevron SVG (viewBox 24) de taille `taille` centré en (cx, cy), trait `ep` unités du viewBox, bouts ronds
void chevron(Renderer& r, float cx, float cy, float taille, const float (&q)[6], float ep, const vec4& c) {
    const float s = taille / 24.f;
    const vec2 p[3] = {{cx + (q[0] - 12) * s, cy + (q[1] - 12) * s}, {cx + (q[2] - 12) * s, cy + (q[3] - 12) * s},
                       {cx + (q[4] - 12) * s, cy + (q[5] - 12) * s}};
    dessin2d::trait(r, p, 3, ep * s, c);
}
void rectangle(Renderer& r, float x, float y, float w, float h, const vec4& c) {
    const vec2 p[4] = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    dessin2d::polygone(r, p, 4, dessin2d::uni(c));
}

// bouton flèche (.arrow-btn, 76 px, coins biaisés 10 px ; icône 38 px « M15 4.5 7.5 12l7.5 7.5 » à gauche,
// « m9 4.5 7.5 7.5L9 19.5 » à droite, trait 3) ; RG-04 : fond vitré du pavé, chevrons #8A90A6
void bouton(Renderer& r, float x, float y, float c, bool droite, bool appuye) {
    const float k = c / 76.f;   // 1 px CSS du bouton
    if (appuye) { const float d = c * 0.02f; x += d; y += d; c -= 2 * d; }   // .arrow-btn.down : transform: scale(.96)
    const float b = c * 10.f / 76.f;
    const vec2 p[4] = {{x + b, y}, {x + c, y}, {x + c - b, y + c}, {x, y + c}};
    if (appuye) {   // fond cyan, box-shadow 0 0 16px rgba(0,229,255,.55)
        dessin2d::halo(r, p, 4, 16 * k, alpha(ACCENT2, 0.4f));
        dessin2d::polygone(r, p, 4, dessin2d::uni(ACCENT2));
    } else {
        dessin2d::polygone(r, p, 4, {VITRE_HAUT, VITRE_BAS, y, y + c});
        dessin2d::contour(r, p, 4, k, BORD);
    }
    static const float G[6] = {15, 4.5f, 7.5f, 12, 15, 19.5f}, D[6] = {9, 4.5f, 16.5f, 12, 9, 19.5f};
    chevron(r, x + c / 2, y + c / 2, c * 38.f / 76.f, droite ? D : G, 3, appuye ? SUR_ACCENT : TEXTE_2);
}

// RG-04 : pavé vitré .steer-pad (156 × 60 px, coins biaisés 12 px) avec piste .steer-track, chevrons et bouton volant
// .steer-knob (56 px, volant SVG 44 px) ; place du jeu web couché : 16 px du bord gauche, 14 px du bas
void pave(Renderer& r, float W, float H, const Marges& m) {
    const Marges mc = ecrans::margesCommandes(m);
    const float k = uniteCommandes(W, H), w = std::round(156 * k), h = std::round(60 * k), y = std::round(H - mc.bas - 14 * k) - h;
    const float x = ecrans::miroirX(std::round(mc.gauche + 16 * k) + w / 2, W) - w / 2;   // WFS-75 : miroir sans retourner
    const float cx = x + w / 2, cy = y + h / 2, b = 12 * k, s = directionBraquage(W);
    const bool appui = g_gl.id >= 0;
    const vec2 p[4] = {{x + b, y}, {x + w, y}, {x + w - b, y + h}, {x, y + h}};
    dessin2d::polygone(r, p, 4, {VITRE_HAUT, VITRE_BAS, y, y + h});
    dessin2d::contour(r, p, 4, k, appui ? alpha(ACCENT2, 0.45f) : BORD);   // .steer-pad.active
    static const float G[6] = {14.5f, 5, 8, 12, 14.5f, 19}, D[6] = {9.5f, 5, 16, 12, 9.5f, 19};   // .steer-chev 20 px
    chevron(r, x + 18 * k, cy, 20 * k, G, 2.6f, TEXTE_2);
    chevron(r, x + w - 18 * k, cy, 20 * k, D, 2.6f, TEXTE_2);
    // piste 4 px (22 px des bords), repère du milieu, remplissage cyan du centre vers le côté braqué (scaleX(|s|))
    const float ep = 4 * k, demi = w / 2 - 22 * k;
    rectangle(r, x + 22 * k, cy - ep / 2, 2 * demi, ep, {1, 1, 1, 0.1f});
    rectangle(r, cx - k, cy - 7 * k, 2 * k, 14 * k, {1, 1, 1, 0.35f});
    const float L = demi * std::fabs(s);
    if (L >= 0.5f) {
        const float x0 = s > 0 ? cx : cx - L;
        const vec2 f[4] = {{x0, cy - ep / 2}, {x0 + L, cy - ep / 2}, {x0 + L, cy + ep / 2}, {x0, cy + ep / 2}};
        dessin2d::halo(r, f, 4, 10 * k, alpha(ACCENT2, 0.35f));   // box-shadow 0 0 10px rgba(0,229,255,.6)
        dessin2d::polygone(r, f, 4, dessin2d::uni(ACCENT2));
    }
    // bouton : translateX(s × 50 px) rotate(s × 110°)
    const float R = 28 * k, kx = cx + s * 50 * k, ky = cy, th = s * 110.f * PI / 180.f, cs = std::cos(th), sn = std::sin(th);
    if (appui) dessin2d::couronne(r, kx, ky, R, R + 14 * k, alpha(ACCENT2, 0.45f), alpha(ACCENT2, 0));   // 0 0 14px cyan
    else dessin2d::couronne(r, kx, ky + 4 * k, 24 * k, 40 * k, {0, 0, 0, 0.35f}, {0, 0, 0, 0});          // 0 4px 14px noir
    // radial-gradient(circle at 50% 35%, #2a2e42, #12141d 70%), tourné avec le bouton
    dessin2d::disqueRadial(r, kx, ky, R, kx + sn * 8.4f * k, ky - cs * 8.4f * k, 32.1f * k,
                           {42 / 255.f, 46 / 255.f, 66 / 255.f, 1}, {18 / 255.f, 20 / 255.f, 29 / 255.f, 1});
    dessin2d::anneau(r, kx, ky, R - k, R, appui ? ACCENT2 : vec4{1, 1, 1, 0.16f});
    // volant (index.html, viewBox 48 dessiné en 44 px) : jante r 19 trait 4, moyeu r 5,5, trois branches, repère cyan
    const float u = 44 * k / 48;
    auto P = [&](float px, float py) { return vec2{kx + (px * cs - py * sn) * u, ky + (px * sn + py * cs) * u}; };
    dessin2d::anneau(r, kx, ky, 17 * u, 21 * u, TEXTE);
    r.uiCircle(kx, ky, 5.5f * u + 0.5f, TEXTE);
    const vec2 b1[2] = {P(-18.5f, -1.5f), P(-5.5f, -1.5f)}, b2[2] = {P(5.5f, -1.5f), P(18.5f, -1.5f)}, b3[2] = {P(0, 5.5f), P(0, 18.5f)};
    dessin2d::trait(r, b1, 2, 3.4f * u, TEXTE);
    dessin2d::trait(r, b2, 2, 3.4f * u, TEXTE);
    dessin2d::trait(r, b3, 2, 3.4f * u, TEXTE);
    const vec2 rep[2] = {P(0, -19), P(0, -14)};
    dessin2d::trait(r, rep, 2, 4 * u, ACCENT2);
}
} // namespace

bool directionPossede(int id) { return id == g_gl.id || id == g_fl.gauche || id == g_fl.droite; }

bool directionToucher(const Touch& t, bool libre, float W, float H, const Marges& m) {
    if (t.down) {
        if (libre && dansZoneDirection(ecrans::miroirX(t.x, W), W, ecrans::margesCommandes(m))) {   // WFS-75
            if (!fleches()) { g_gl.id = t.id; g_gl.x0 = g_gl.x = t.x; }   // pointerdown de #steerZone
            else if (flecheDroite(t.x, boutonsPlaces(W, H, m))) g_fl.droite = t.id;
            else g_fl.gauche = t.id;
        }
        if (t.id == g_gl.id) { g_gl.x = t.x; return true; }   // pointermove : le pouce glisse
        return t.id == g_fl.gauche || t.id == g_fl.droite;   // un bouton garde son doigt (setPointerCapture)
    }
    bool mien = false;   // pointerup / pointercancel
    if (t.id == g_gl.id) { g_gl.id = -1; mien = true; }
    if (t.id == g_fl.gauche) { g_fl.gauche = -1; mien = true; }
    if (t.id == g_fl.droite) { g_fl.droite = -1; mien = true; }
    return mien;
}

float directionBraquage(float W) {
    float t = 0;
    bool doigt = false;
    if (fleches()) { t = braquageFleches(g_fl.gauche >= 0, g_fl.droite >= 0); doigt = g_fl.gauche >= 0 || g_fl.droite >= 0; }
    else if (g_gl.id >= 0) { t = braquageGlisser(g_gl.x0, g_gl.x, W); doigt = true; }
    return doigt ? t : braquageIncline();   // WFS-108 : un doigt sur la direction l'emporte sur l'inclinaison
}

void directionDessiner(Renderer& r, float W, float H, const Marges& m) {
    if (fleches()) {   // #steerArrows : deux boutons, le bouton appuyé en cyan
        const BoutonsFleches b = boutonsPlaces(W, H, m);
        bouton(r, b.xg, b.y, b.c, false, g_fl.gauche >= 0);
        bouton(r, b.xd, b.y, b.c, true, g_fl.droite >= 0);
        return;
    }
    pave(r, W, H, m);   // WFS-98 : volant du jeu web (zone de glissement inchangée : x0 = point d'appui)
}

void directionLacher() {
    g_gl.id = -1; g_fl = {};
    gyroscopeEtalonner();   // WFS-108 RG-03 : à chaque changement d'écran (départ de course, reprise après la pause)
}

} // namespace wfs
