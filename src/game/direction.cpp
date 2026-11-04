// WaForSpeed — modes de direction tactiles (WFS-70) : voir direction.h.
// La partie « volant » de Game::onTouch et de Game::drawControls a été déplacée ici sans changement (mode glissement).
#include "direction.h"
#include <cmath>
#include "../render/renderer.h"
#include "game.h"

namespace wfs {

namespace {
// zone de glissement : doigt du volant, point de départ et position du pouce (steerUI.id, x0 du jeu web)
struct Glisser { int id = -1; float x0 = 0, x = 0; } g_gl;
// flèches : doigt posé sur chaque bouton (dataset.pid des boutons .arrow-btn)
struct Fleches { int gauche = -1, droite = -1; } g_fl;

bool fleches() { return reglagesRessenti().direction == ModeDirection::Fleches; }

// couleurs du skill design-street-racing (tokens.md, codes sRGB comme ui::CYAN)
const vec4 FOND_BOUTON{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.62f};   // rgba(20, 22, 31, .62)
const vec4 BORD_BOUTON{1.f, 1.f, 1.f, 0.12f};                          // 1px solid rgba(255, 255, 255, .12)
const vec4 ACCENT2{0.f, 0.898f, 1.f, 1.f};                             // --accent-2 #00E5FF (bouton appuyé)
const vec4 HALO{0.f, 0.898f, 1.f, 0.22f};                              // box-shadow 0 0 16px rgba(0, 229, 255, .55)
const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1.f};          // --text #F2F4F8
const vec4 SUR_ACCENT{10 / 255.f, 11 / 255.f, 16 / 255.f, 1.f};        // --on-accent #0A0B10

// parallélogramme (clip-path : polygon(10px 0, 100% 0, calc(100% − 10px) 100%, 0 100%)) en bandes horizontales,
// avec un liseré de largeur lw sur les quatre côtés (0 = sans)
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c, float lw, const vec4& bord) {
    const int N = 24;
    for (int i = 0; i < N; i++) {
        const float t = (i + 0.5f) / N, dx = biais * (1 - t), yy = y + h * i / N, hh = y + h * (i + 1) / N - yy;   // bandes jointives (fond translucide)
        r.uiRect(x + dx, yy, w - biais, hh, c);
        if (lw > 0) { r.uiRect(x + dx, yy, lw, hh, bord); r.uiRect(x + dx + w - biais - lw, yy, lw, hh, bord); }
    }
    if (lw > 0) { r.uiRect(x + biais, y, w - biais, lw, bord); r.uiRect(x, y + h - lw, w - biais, lw, bord); }
}

// trait épais (stroke-linecap: round) : disques serrés le long du segment
void trait(Renderer& r, float x0, float y0, float x1, float y1, float rayon, const vec4& c) {
    const float L = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    const int n = std::max(2, (int)(L / (rayon * 0.4f)) + 1);
    for (int i = 0; i <= n; i++) { const float t = (float)i / n; r.uiCircle(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, rayon, c); }
}

// bouton flèche (.arrow-btn, icône SVG 24×24 : « M15 4.5 7.5 12l7.5 7.5 » à gauche, « m9 4.5 7.5 7.5L9 19.5 » à droite)
void bouton(Renderer& r, float x, float y, float c, bool droite, bool appuye) {
    if (appuye) {   // .arrow-btn.down : fond cyan, halo, transform: scale(.96)
        const float h = c * 0.08f;
        biseau(r, x - h, y - h, c + 2 * h, c + 2 * h, c * 10.f / 76.f, HALO, 0, HALO);
        const float d = c * 0.02f; x += d; y += d; c -= 2 * d;
        biseau(r, x, y, c, c, c * 10.f / 76.f, ACCENT2, 0, ACCENT2);
    } else biseau(r, x, y, c, c, c * 10.f / 76.f, FOND_BOUTON, std::max(1.f, c / 76.f), BORD_BOUTON);
    const float ic = c * 38.f / 76.f, k = ic / 24.f, ox = x + (c - ic) / 2, oy = y + (c - ic) / 2;   // icône 38 px
    const float pointe = droite ? 16.5f : 7.5f, dos = droite ? 9.f : 15.f;
    const vec4& col = appuye ? SUR_ACCENT : TEXTE;
    trait(r, ox + dos * k, oy + 4.5f * k, ox + pointe * k, oy + 12.f * k, 1.5f * k, col);   // stroke-width: 3
    trait(r, ox + pointe * k, oy + 12.f * k, ox + dos * k, oy + 19.5f * k, 1.5f * k, col);
}
} // namespace

bool directionPossede(int id) { return id == g_gl.id || id == g_fl.gauche || id == g_fl.droite; }

bool directionToucher(const Touch& t, bool libre, float W, float H, const Marges& m) {
    if (t.down) {
        if (libre && dansZoneDirection(t.x, W, m)) {
            if (!fleches()) { g_gl.id = t.id; g_gl.x0 = g_gl.x = t.x; }   // pointerdown de #steerZone
            else if (flecheDroite(t.x, boutonsFleches(W, H, m))) g_fl.droite = t.id;
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
    if (fleches()) return braquageFleches(g_fl.gauche >= 0, g_fl.droite >= 0);
    return g_gl.id >= 0 ? braquageGlisser(g_gl.x0, g_gl.x, W) : 0.f;
}

void directionDessiner(Renderer& r, float W, float H, const Marges& m) {
    const float u = H / 100.f, mg = m.gauche, mb = m.bas;
    if (fleches()) {   // #steerArrows : deux boutons, le bouton appuyé en cyan
        const BoutonsFleches b = boutonsFleches(W, H, m);
        bouton(r, b.xg, b.y, b.c, false, g_fl.gauche >= 0);
        bouton(r, b.xd, b.y, b.c, true, g_fl.droite >= 0);
        return;
    }
    // volant virtuel (code d'avant WFS-70, inchangé)
    if (g_gl.id >= 0) {
        r.uiCircle(g_gl.x0, H * 0.72f - mb, 11 * u, {1, 1, 1, 0.12f});
        r.uiCircle(g_gl.x0 + clampf(g_gl.x - g_gl.x0, -W * 0.12f, W * 0.12f), H * 0.72f - mb, 5 * u, {0, 0.9f, 1, 0.55f});
    } else r.uiCircle(W * 0.18f + mg, H * 0.72f - mb, 9 * u, {1, 1, 1, 0.08f});
}

void directionLacher() { g_gl.id = -1; g_fl = {}; }

} // namespace wfs
