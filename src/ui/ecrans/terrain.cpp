// WaForSpeed — choix du terrain et du moment (WFS-41) : voir terrain.h.
#include "terrain.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/track.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "reglages.h"
#include "../../platform/manette.h"   // WFS-22

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
vec4 melange(const vec4& a, const vec4& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 BORD{1, 1, 1, 0.10f};

// js/world/maps.js : ordre de MAPS, textes et couleurs des illustrations (art.ground [nuit, jour], art.accent)
struct Carte { const char* id; const char* desc; unsigned solNuit, solJour, accent; };
const Carte CARTES[4] = {
    {"ville", "Avenues entre les gratte-ciel, enseignes au néon et chaussée qui brille.", 0x14151C, 0x55565C, 0x00E5FF},
    {"plage", "Route du bord de mer : sable, palmiers, parasols et l'océan à perte de vue.", 0x2A2A22, 0xE6D3A1, 0x1D8FB8},
    {"campagne", "Petites routes entre les champs, les haies, les fermes et les éoliennes.", 0x18200F, 0x8FAE4C, 0xCDB259},
    {"circuit", "Circuit fermé : vibreurs, tribunes, murs de pneus. Tu doubles d'autres pilotes.", 0x14200F, 0x4F7F32, 0xE2262B},
};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
Zone g_carte, g_prec, g_suiv, g_jour, g_nuit, g_retour, g_rouler;
int g_index = 0;          // carte choisie (MAPS)
bool g_nuitChoisie = true; // TOD du web : nuit par défaut

void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    const int N = 16;
    for (int i = 0; i < N; i++) {
        const float t = (i + 0.5f) / N;
        r.uiRect(x + biais * (1 - t), y + h * i / N, w - biais, h / N + 0.5f, c);
    }
}
// chevron < ou > (path « M15 4 7 12l8 8 » du web) en petits carrés
void chevron(Renderer& r, float cx, float cy, float s, int dir, const vec4& c) {
    const int N = 9;
    for (int i = 0; i <= N; i++) {
        const float t = (float)i / N, dy = (t - 0.5f) * s, dx = (std::fabs(t - 0.5f) - 0.25f) * s * dir;
        r.uiRect(cx - dx - s * 0.06f, cy + dy - s * 0.06f, s * 0.13f, s * 0.13f, c);
    }
}

// ---------------------------------------------------------------- vignette (mapArt du web, en rectangles et disques)
// toutes les formes restent dans le cadre (x, y, w, h) ; repère du web : 400 x 250, horizon à 150
void vignette(Renderer& r, float X, float Y, float W, float H, int idx, bool nuit) {
    const Carte& c = CARTES[idx];
    const MapDef& m = mapById(c.id);
    Mulberry32 R(m.seed * 7 + (nuit ? 3 : 1));
    auto rnd = [&]() { return (float)R.next(); };
    const float sx = W / 400.f, sy = H / 250.f, HZ = Y + 150 * sy;
    auto px = [&](float v) { return X + v * sx; };
    auto py = [&](float v) { return Y + v * sy; };
    // ciel : dégradé à 3 arrêts (0, 62 %, 100 %) en bandes
    unsigned ciel[3] = {nuit ? 0x05050Cu : 0x2F6CC0u, nuit ? 0x1A0F2Eu : 0x79B1E2u, nuit ? 0x43203Fu : 0xDCEBF5u};
    if (!nuit && idx == 0) ciel[2] = 0xE6E3DF;
    if (nuit && idx == 2) ciel[2] = 0x1D2640;
    if (nuit && idx == 1) ciel[2] = 0x3A2448;
    const int NB = 30;
    for (int i = 0; i < NB; i++) {
        const float t = (i + 0.5f) / NB;
        const vec4 col = t < 0.62f ? melange(hex(ciel[0]), hex(ciel[1]), t / 0.62f) : melange(hex(ciel[1]), hex(ciel[2]), (t - 0.62f) / 0.38f);
        r.uiRect(X, Y + (HZ - Y) * i / NB, W, (HZ - Y) / NB + 0.5f, col);
    }
    // astres et nuages
    if (nuit) {
        if (idx != 0) for (int i = 0; i < 40; i++) { const float x = rnd() * 400, y = rnd() * 112; r.uiCircle(px(x), py(y), (0.5f + rnd()) * sx, hex(0xFFFFFF, 0.4f + rnd() * 0.6f)); }
        const float mx = idx == 1 ? 92 : 318;
        for (int k = 3; k >= 1; k--) r.uiCircle(px(mx), py(46), 12 * k * sx, hex(0xFFF6D8, 0.10f));
        r.uiCircle(px(mx), py(46), 13 * sx, hex(0xF4F1FF));
        r.uiCircle(px(mx + 6), py(42), 11 * sx, hex(ciel[0], 0.35f));
    } else {
        const float sy0 = idx == 1 ? 108 : 52;
        for (int k = 3; k >= 1; k--) r.uiCircle(px(300), py(sy0), 18 * k * sx, hex(0xFFF3C4, 0.13f));
        r.uiCircle(px(300), py(sy0), 17 * sx, hex(0xFFF8E0));
        for (int i = 0; i < 4; i++) {
            const float cx = 40 + rnd() * 320, cy = 30 + rnd() * 46, w = 40 + rnd() * 50;
            for (float d : {-0.25f, 0.f, 0.25f}) r.uiCircle(px(cx + d * w), py(cy - (d == 0 ? 6 : 2)), (d == 0 ? 11 : 9) * sx, hex(0xFFFFFF, 0.85f));
        }
    }
    // décor de fond
    if (idx == 0) {   // ville : deux rangées d'immeubles, fenêtres allumées
        const unsigned tons[3] = {nuit ? 0x11121Cu : 0x4A5F7Cu, nuit ? 0x171826u : 0xA3B3C7u, nuit ? 0x0D0E17u : 0x8B9DB4u};
        for (int couche = 0; couche < 2; couche++) {
            float x = 0;
            while (x < 400) {
                const float w = std::min(18 + rnd() * 30, 400 - x), h = (couche ? 40 : 70) + rnd() * (couche ? 50 : 70), y = 150 - h;
                r.uiRect(px(x), py(y), w * sx, h * sy, hex(couche ? tons[0] : tons[1 + (int)(rnd() * 2)]));
                if (couche) for (float wy = y + 6; wy < 144; wy += 7) for (float wx = x + 4; wx < x + w - 4; wx += 6)
                    if (rnd() < (nuit ? 0.42f : 0.25f)) r.uiRect(px(wx), py(wy), 3 * sx, 3.4f * sy, hex(nuit ? (rnd() < 0.75f ? 0xFFCF7A : 0x7FF0FF) : 0xC9DEF2, 0.85f));
                x += w + 2;
            }
        }
        if (nuit) { r.uiRect(px(54), py(96), 44 * sx, 12 * sy, hex(0x00E5FF, 0.55f)); r.uiRect(px(292), py(84), 10 * sx, 40 * sy, hex(0xFF6A00, 0.55f)); }
    } else if (idx == 1) {   // corniche : la mer jusqu'à l'horizon, vaguelettes
        r.uiRect(X, HZ - 2 * sy, W, Y + H - HZ + 2 * sy, hex(nuit ? 0x0C1A2E : 0x1F86B8));
        for (int i = 0; i < 9; i++) r.uiRect(px(rnd() * 340), py(153 + i * 3), (20 + rnd() * 40) * sx, 1.2f * sy, hex(0xFFFFFF, nuit ? 0.18f : 0.5f));
    } else {   // campagne et circuit : collines (bosses en bandes)
        const unsigned c0 = idx == 2 ? (nuit ? 0x141C2C : 0x9AB7A0) : (nuit ? 0x10161C : 0x5F8A52), c1 = nuit ? 0x101622 : 0x7FA45A;
        for (int i = 0; i < 40; i++) {
            const float x = i * 10.f, h0 = 30 + 18 * std::sin(i * 0.31f + 1.f), h1 = 12 + 12 * std::sin(i * 0.23f + 2.5f);
            r.uiRect(px(x), py(150 - h0), 10.5f * sx, h0 * sy, hex(c0));
            if (idx == 2) r.uiRect(px(x), py(150 - h1), 10.5f * sx, h1 * sy, hex(c1));
        }
        if (idx == 2) for (float x : {70.f, 118.f, 338.f}) {   // éoliennes
            r.uiRect(px(x - 0.7f), py(104), 1.4f * sx, 34 * sy, hex(nuit ? 0x2A3350 : 0xF4F6F8));
            r.uiCircle(px(x), py(104), 3 * sx, hex(nuit ? 0xFF1F4B : 0xF4F6F8));
        } else {   // circuit : tribune et spectateurs
            r.uiRect(px(14), py(108), 120 * sx, 42 * sy, hex(nuit ? 0x1B1E2C : 0xC4C9D2));
            const unsigned pub[4] = {0xE2262B, 0xFFD23A, 0x2A6FDB, 0xF2F4F8};
            for (int l = 0; l < 4; l++) for (int i = 0; i < 22; i++) r.uiRect(px(20 + i * 5.2f), py(114 + l * 7), 3.2f * sx, 3 * sy, hex(pub[(int)(rnd() * 4)], nuit ? 0.55f : 0.85f));
            r.uiRect(px(148), py(98), 104 * sx, 11 * sy, hex(nuit ? 0x202434 : 0x2D3038));   // portique de départ
            for (int i = 0; i < 5; i++) r.uiCircle(px(172 + i * 14), py(103.5f), 3 * sx, hex(nuit ? 0xFF1F4B : 0x5B1020));
        }
    }
    // sol et route en perspective (trapèze en bandes)
    const vec4 sol = hex(nuit ? c.solNuit : c.solJour);
    if (idx != 1) r.uiRect(X, HZ, W, Y + H - HZ, sol);
    const float cx = idx == 1 ? 230 : 200, top = 6, bot = 190;
    const int NR = 40;
    for (int i = 0; i < NR; i++) {
        const float t = (i + 0.5f) / NR, y0 = 150 + 100.f * i / NR, demi = top + t * (bot - top);
        if (idx == 1) {   // sable à droite de la route, à la plage
            const float g0 = std::max(0.f, cx - demi - 40 * t - 10);
            r.uiRect(px(g0), py(y0), (400 - g0) * sx, 100.f / NR * sy + 0.5f, sol);
        }
        const float x0 = std::max(0.f, cx - demi), x1 = std::min(400.f, cx + demi);
        r.uiRect(px(x0), py(y0), (x1 - x0) * sx, 100.f / NR * sy + 0.5f, melange(hex(nuit ? 0x1A1B24 : 0x5D6068), hex(nuit ? 0x0B0C12 : 0x3A3C42), t));
        const float bord = (1.2f + 2 * t) * sx;
        if (idx == 3) {   // vibreurs rouge et blanc
            const vec4 v = ((int)(std::sqrt(t) * 9)) % 2 ? hex(0xF2F4F8) : hex(0xE2262B);
            const float e = (4 + demi * 0.12f) * sx;
            const float g0 = std::max(X, px(cx - demi) - e), d1 = std::min(X + W, px(cx + demi) + e);   // vibreurs gardés dans la vignette
            if (px(cx - demi) > g0) r.uiRect(g0, py(y0), px(cx - demi) - g0, 100.f / NR * sy + 0.5f, v);
            if (d1 > px(cx + demi)) r.uiRect(px(cx + demi), py(y0), d1 - px(cx + demi), 100.f / NR * sy + 0.5f, v);
        } else {
            const vec4 v = hex(nuit ? 0xC9CDDA : 0xF2F4F8, 0.8f);
            if (cx - demi >= 0) r.uiRect(px(cx - demi), py(y0), bord, 100.f / NR * sy + 0.5f, v);
            if (cx + demi <= 400) r.uiRect(px(cx + demi) - bord, py(y0), bord, 100.f / NR * sy + 0.5f, v);
        }
        if (((int)(std::pow(t, 1 / 2.2f) * 12)) % 2 == 0)   // tirets des voies (± 1/3)
            for (float l : {-1 / 3.f, 1 / 3.f}) r.uiRect(px(cx + l * demi) - bord / 2, py(y0), bord, 100.f / NR * sy + 0.5f, hex(0xF2F4F8, nuit ? 0.6f : 0.85f));
    }
    if (nuit) {   // feux d'une voiture au loin et reflet de l'accent sur la chaussée mouillée
        r.uiCircle(px(cx - 3), py(174), 14 * sx, hex(0xFF1F4B, 0.16f));
        r.uiCircle(px(cx - 9), py(172), 2 * sx, hex(0xFF1F4B)); r.uiCircle(px(cx + 3), py(172), 2 * sx, hex(0xFF1F4B));
        for (int i = 0; i < 12; i++) { const float t = (i + 0.5f) / 12; r.uiRect(px(cx - 3 - 23 * t), py(158 + 92 * i / 12.f), (6 + 46 * t) * sx, 92 / 12.f * sy + 0.5f, hex(c.accent, 0.14f)); }
    }
}

class Terrain : public System {
public:
    const char* name() const override { return "terrain"; }
    void onEcran(Ecran, Ecran nouveau, const GameView& g) override {
        doigt_ = -1;
        if (nouveau != Ecran::Terrain) { g_carte = g_prec = g_suiv = g_jour = g_nuit = g_retour = g_rouler = {}; return; }
        // carte en cours présélectionnée (pickMap garde MAP)
        for (int i = 0; i < 4; i++) if (g.map && std::string(g.map->id) == CARTES[i].id) g_index = i;
    }
    // RG-02 : glisser sur la vignette = carte suivante / précédente ; appuis sur les flèches, Jour / Nuit, Retour, Rouler
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Terrain) return false;
        const float k = g.uiHeight / 720.f;
        if (t.down) {
            if (doigt_ < 0) { doigt_ = t.id; x0_ = t.x; y0_ = t.y; }
            return true;
        }
        if (t.id != doigt_) return true;
        doigt_ = -1;
        const float dx = t.x - x0_;
        if (g_carte.contient(x0_, y0_) && std::fabs(dx) > 50 * k) { pas(dx < 0 ? 1 : -1); return true; }   // glissé
        if (std::fabs(dx) > 30 * k) return true;
        if (g_prec.contient(t.x, t.y)) pas(-1);
        else if (g_suiv.contient(t.x, t.y)) pas(1);
        else if (g_jour.contient(t.x, t.y)) { g_nuitChoisie = false; WFS_LOG("terrain : Jour"); }
        else if (g_nuit.contient(t.x, t.y)) { g_nuitChoisie = true; WFS_LOG("terrain : Nuit"); }
        else if (g_retour.contient(t.x, t.y)) { WFS_LOG("terrain : « Retour »"); g.allerA(Ecran::Menu); }
        else if (g_rouler.contient(t.x, t.y)) {   // RG-03 : la partie est reconstruite sur la carte choisie, puis la course
            WFS_LOG("terrain : « Rouler » sur %s (%s)", CARTES[g_index].id, g_nuitChoisie ? "nuit" : "jour");
            Game::demandes().carte = CARTES[g_index].id; Game::demandes().nuit = g_nuitChoisie;
        }
        return true;   // l'écran garde tous les touchers
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Terrain || g.sceneDediee || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        const Marges& mg = g.marges;
        const float zl = mg.gauche + 32 * k, zr = W - mg.droite - 32 * k, zw = zr - zl;
        r.uiRect(0, 0, W, H, hex(BG, 0.94f));   // .screen--maps : fond nuit, la scène à peine visible derrière
        // ---------- .maps-head : sur-titre, titre, index « 01 / 04 » ----------
        const float hy = mg.haut + 26 * k;
        ui::text(r, zl, hy, 13 * k, ui::majuscules("Étape 2 sur 2 · Terrain"), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
        ui::text(r, zl, hy + 20 * k, 44 * k, ui::majuscules("Choisis ton terrain"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        char idx[16]; std::snprintf(idx, sizeof idx, "%02d", g_index + 1);
        const std::string tot = " / 04";
        ui::text(r, zr, hy + 30 * k, 22 * k, tot, hex(MUTED), ui::DROITE, 0.04f, false);
        ui::text(r, zr - ui::width(22 * k, tot, 0.04f), hy + 22 * k, 32 * k, idx, hex(ACCENT), ui::DROITE, 0.04f, false, true);
        // ---------- vignette de la carte (mapCard) ----------
        const float bas = H - mg.bas - 24 * k - 56 * k, haut = hy + 84 * k;
        const float chMax = bas - 34 * k - haut, cw = std::min(zw - 2 * 76 * k, chMax * 1.9f), ch = std::min(chMax, cw / 1.6f);
        const float cx0 = zl + (zw - cw) / 2, cy0 = haut;
        g_carte = {cx0, cy0, cw, ch};
        const Carte& c = CARTES[g_index];
        const MapDef& m = mapById(c.id);
        r.uiRect(cx0 - 2 * k, cy0 - 2 * k, cw + 4 * k, ch + 4 * k, hex(c.accent));   // carte choisie : contour à l'accent de la carte
        vignette(r, cx0, cy0, cw, ch, g_index, g_nuitChoisie);
        // .map-shade : dégradé sombre vers le bas, sous les textes
        const int NS = 20;
        for (int i = 0; i < NS; i++) { const float t = (i + 0.5f) / NS; r.uiRect(cx0, cy0 + ch * (0.45f + 0.55f * i / NS), cw, ch * 0.55f / NS + 0.5f, hex(BG, 0.85f * t)); }
        // .map-sel « Choisi »
        const std::string choisi = ui::majuscules("Choisi");
        const float sw = ui::width(13 * k, choisi, 0.12f) + 24 * k;
        r.uiRect(cx0 + 16 * k, cy0 + 16 * k, sw, 28 * k, hex(c.accent, 0.92f));
        ui::text(r, cx0 + 28 * k, cy0 + 23 * k, 13 * k, choisi, hex(BG), ui::GAUCHE, 0.12f, false);
        // .map-info : nom, description, longueur
        const float ix = cx0 + 24 * k, iy = cy0 + ch - 128 * k;
        ui::text(r, ix, iy, 36 * k, ui::majuscules(m.name), hex(TEXTE), ui::GAUCHE, 0.02f, true, true);
        ui::text(r, ix, iy + 46 * k, 15 * k, c.desc, hex(0xC9CDDA), ui::GAUCHE, 0, true);
        ui::text(r, ix, iy + 76 * k, 11 * k, ui::majuscules("Longueur"), hex(MUTED), ui::GAUCHE, 0.16f, false);
        std::string km = ui::decimal(std::round(longueurKm(m) * 10) / 10, 1);
        if (km.size() > 2 && km.compare(km.size() - 2, 2, ",0") == 0) km.resize(km.size() - 2);
        ui::text(r, ix, iy + 92 * k, 20 * k, km + " km", hex(TEXTE), ui::GAUCHE, 0, true, true);
        // ---------- flèches (.map-arrow), grisées aux extrémités ----------
        const float ar = 26 * k, ay = cy0 + ch / 2;
        const float ax[2] = {cx0 - 18 * k - ar, cx0 + cw + 18 * k + ar};
        for (int s = 0; s < 2; s++) {
            const bool actif = s == 0 ? g_index > 0 : g_index < 3;
            (s == 0 ? g_prec : g_suiv) = actif ? Zone{ax[s] - ar - 10 * k, ay - ar - 10 * k, 2 * ar + 20 * k, 2 * ar + 20 * k} : Zone{};
            r.uiCircle(ax[s], ay, ar + k, hex(0xFFFFFF, actif ? 0.14f : 0.05f));
            r.uiCircle(ax[s], ay, ar, hex(SURFACE2, actif ? 0.95f : 0.5f));
            chevron(r, ax[s] + (s == 0 ? -2 : 2) * k, ay, 18 * k, s == 0 ? -1 : 1, hex(TEXTE, actif ? 1.f : 0.3f));
        }
        // ---------- points (.map-dots) ----------
        const float dy = cy0 + ch + 16 * k;
        float dxp = zl + zw / 2 - (3 * 14 * k + 28 * k) / 2;
        for (int i = 0; i < 4; i++) {
            const float w = i == g_index ? 28 * k : 8 * k;
            r.uiRect(dxp, dy, w, 4 * k, i == g_index ? hex(ACCENT) : hex(TEXTE, 0.25f));
            dxp += w + 6 * k;
        }
        // ---------- .maps-bar : Jour / Nuit (.tod-btn), Retour, Rouler ----------
        const float by = H - mg.bas - 24 * k - 56 * k, bh = 56 * k, tw = 132 * k;
        for (int s = 0; s < 2; s++) {
            const bool nuit = s == 1, on = nuit == g_nuitChoisie;
            const float x = zl + s * (tw + 6 * k);
            (nuit ? g_nuit : g_jour) = {x, by, tw, bh};
            r.uiRect(x, by, tw, bh, on ? hex(ACCENT2, 0.16f) : hex(SURFACE2));
            if (on) r.uiRect(x, by + bh - 3 * k, tw, 3 * k, hex(ACCENT2));
            const float icx = x + 30 * k, icy = by + bh / 2;   // icône soleil / lune
            if (!nuit) { r.uiCircle(icx, icy, 7 * k, on ? hex(ACCENT2) : hex(TEXTE)); for (int a = 0; a < 8; a++) r.uiCircle(icx + std::cos(a * 0.785f) * 12 * k, icy + std::sin(a * 0.785f) * 12 * k, 1.6f * k, on ? hex(ACCENT2) : hex(TEXTE)); }
            else { r.uiCircle(icx, icy, 9 * k, on ? hex(ACCENT2) : hex(TEXTE)); r.uiCircle(icx + 5 * k, icy - 4 * k, 8 * k, on ? hex(0x0A2A33) : hex(SURFACE2)); }
            ui::text(r, x + 52 * k, by + (bh - 18 * k) / 2, 18 * k, ui::majuscules(nuit ? "Nuit" : "Jour"), on ? hex(ACCENT2) : hex(TEXTE), ui::GAUCHE, 0.08f, false);
        }
        const float tp = 20 * k;
        const std::string rt = ui::majuscules("Rouler"), bt = ui::majuscules("Retour");
        const float rw = ui::width(tp, rt, 0.12f) + 56 * k + 26 * k, bw = ui::width(tp, bt, 0.12f) + 52 * k;
        const float rx = zr - rw, bx = rx - 10 * k - bw;
        g_rouler = {rx, by, rw, bh}; g_retour = {bx, by, bw, bh};
        for (const Zone* z : {&g_rouler, &g_retour, &g_prec, &g_suiv, &g_jour, &g_nuit}) manette::focus(*z, z == &g_retour);   // WFS-22
        biseau(r, bx, by, bw, bh, 14 * k, hex(SURFACE2));   // .btn--ghost
        ui::text(r, bx + bw / 2, by + (bh - tp) / 2, tp, bt, hex(TEXTE), ui::CENTRE, 0.12f, false);
        biseau(r, rx, by, rw, bh, 14 * k, hex(ACCENT));     // .btn--go avec la flèche
        ui::text(r, rx + 28 * k, by + (bh - tp) / 2, tp, rt, hex(BG), ui::GAUCHE, 0.12f, false);
        const float fx = rx + rw - 40 * k, fy = by + bh / 2;
        r.uiRect(fx, fy - 1.5f * k, 16 * k, 3 * k, hex(BG));
        chevron(r, fx + 13 * k, fy, 14 * k, 1, hex(BG));
    }
private:
    void pas(int dir) {   // stepMap du web : bornée aux extrémités
        const int i = std::clamp(g_index + dir, 0, 3);
        if (i != g_index) { g_index = i; WFS_LOG("terrain : carte %s (%02d / 04)", CARTES[i].id, i + 1); }
    }
    int doigt_ = -1;
    float x0_ = 0, y0_ = 0;
};

} // namespace

bool centreRouler(float& x, float& y) {
    if (g_rouler.w <= 0) return false;
    x = g_rouler.x + g_rouler.w / 2; y = g_rouler.y + g_rouler.h / 2;
    return true;
}

double longueurKm(const MapDef& m) {   // mapLengthKm (screens.js)
    Mulberry32 R(m.seed);
    int n = 120;
    for (int i = 0; i < m.sections; i++) {
        const double t = R.next();
        if (R.next() < 0.5) R.next();
        if (t < 0.22) n += 50 + (int)(40 + R.next() * 40);
        else if (t < 0.7) { R.next(); R.next(); n += 60 + (int)(30 + R.next() * 50); }
        else { R.next(); n += 195; }
    }
    n += 180 + 70;
    return n * SEG * M / 1000;
}

// banc d'essai (WFS-41) : touchers simulés comme ceux d'un doigt
void benchTerrain(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("terrain : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 1100;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, carte %s, %s", nom, nomEcran(g.ecran()), CARTES[g_index].id, quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    for (int i = 0; i < 30; i++) image();
    for (int i = 0; i < 4; i++) WFS_LOG("terrain : %s ≈ %.2f km", CARTES[i].id, longueurKm(mapById(CARTES[i].id)));
    g.allerA(Ecran::Terrain); image(); g.render(t);
    capture("terrain-0", "ville choisie, nuit par défaut");
    // glissé vers la gauche sur la vignette (carte suivante), puis deux appuis sur la flèche droite → circuit ; Jour
    const float y = g_carte.y + g_carte.h / 2;
    g.onTouch({1, g_carte.x + g_carte.w * 0.7f, y, true}); image(); g.onTouch({1, g_carte.x + g_carte.w * 0.3f, y, true}); image();
    g.onTouch({1, g_carte.x + g_carte.w * 0.3f, y, false}); image();
    g.render(t); toucher(2, g_suiv); g.render(t); toucher(3, g_suiv); g.render(t); toucher(4, g_jour);
    capture("terrain-1", "circuit choisi, jour");
    // Rouler : partie reconstruite sur le circuit, décompte puis course en pilote automatique
    g.render(t); toucher(5, g_rouler);
    g.setAutopilot(true);
    for (int i = 0; i < 60 * 6; i++) image();
    WFS_LOG("terrain-rouler : écran %s, %.0f km/h, %.0f m", nomEcran(g.ecran()), g.player().kmh(), g.player().dist * M);
    capture("terrain-rouler", "course lancée après « Rouler »");
    g.shutdown();
    unsetenv("WFS_MOMENT");
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Terrain, 452)   // interface (400-499) : après le menu (450), avant la pause (455)

} // namespace wfs
