// WaForSpeed — garage : fiche de la voiture (WFS-534) : voir garage_fiche.h. Code repris tel quel de garage.cpp.
#include "garage_fiche.h"
#include <algorithm>
#include <cmath>
#include "../render/renderer.h"
#include "text.h"

namespace wfs::ecrans::garage {
namespace {
// carStats(c) du jeu web : notes sur 10
struct Stats { int v, a, m, p; };
Stats carStats(const CarDef& c) {
    const CarSpec& s = c.spec;
    auto rnd = [](float x) { return (int)std::floor(x + 0.5f); };
    return {rnd(clampN((s.vmax - 170) / 14.f, 1, 10)), rnd(clampN(13 - s.s100 * 1.05f, 1, 10)), c.grip, rnd(clampN(s.ch / 55.f, 1, 10))};
}
std::string virgule(float v) { std::string s = ui::decimal(v, 1); return s; }
} // namespace

void FicheGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float W = mp.W, k = mp.k, op = mp.op, gl = mp.gl, gt = mp.gt;
    const CarDef& car = CARS[e.index];
    const Fiche& fi = ficheDe(car.id);
    // ---------- .show-head : marque · index, nom, type + accroche, points des voitures ----------
    {
        const float x = gl + std::clamp(W * 0.04f, 0.f, 64 * k);
        float y = gt + 8 * k;
        const std::string eb = ui::majuscules(marque(car)) + " · " + deuxChiffres(placeDe(e.index) + 1) + " / " + deuxChiffres(voituresPermises().n);
        ui::text(r, x, y, 12 * k, eb, hex(ACCENT2, op), ui::GAUCHE, 0.22f, false);
        y += 22 * k;
        float np = std::min(84 * k, std::max(40 * k, W * 0.07f * 0.8f));
        const std::string nm = ui::majuscules(modele(car));   // WFS-191 : nom de configgame.json
        if (const float nw = ui::width(np, nm); nw > W * 0.5f) np *= W * 0.5f / nw;   // WFS-191 (RG-08) : nom long réduit pour tenir
        ui::text(r, x, y, np, nm, hex(TEXTE, op), ui::GAUCHE, 0, true, true);
        y += np * 1.0f + 8 * k;
        // WFS-147 (ANO-47) : libellé de 13 px au moins (règle du HUD : 11 px du web portés à 13 px), calé au pixel entier
        // (en 720p, le « I » de « HYBRIDE » tombait entre deux pixels et le « D » perdait son trait) ; dès 13 px
        // (2400x1080 : 16,5 px), taille et place inchangées
        const std::string body = ui::majuscules(fi.body);
        const float cp = std::max(11 * k, 13.f), cy = cp > 11 * k ? (20 * k - cp) / 2 : 4 * k;
        const float cw = ui::width(cp, body, 0.16f) + 20 * k;
        biseau(r, x, y, cw, 20 * k, 3.5f * k, hex(ACCENT2, op));   // .chip : fond cyan, skewX(-10deg)
        ui::text(r, std::round(x + 10 * k), std::round(y + cy), cp, body, hex(ON_ACCENT, op), ui::GAUCHE, 0.16f, false);
        ui::text(r, x + cw + 12 * k, y + 1 * k, 15 * k, fi.tag, hex(TAG, op), ui::GAUCHE, 0, true);
        y += 20 * k + 14 * k;
        float px = x;
        const int nb = voituresPermises().n;   // WFS-404 : un point par voiture permise
        for (int i = 0; i < nb; i++) {   // .car-dots : 30 × 8, active 54 px cyan
            const bool on = voituresPermises().idx[i] == e.index;
            const float w = (nb > 10 ? (on ? 34 : 16) : (on ? 54 : 30)) * k;   // WFS-13 : 20 voitures sur une ligne
            if (on) biseau(r, px - 2 * k, y - 2 * k, w + 4 * k, 12 * k, 4 * k, hex(ACCENT2, 0.25f * op));
            biseau(r, px, y, w, 8 * k, 3 * k, on ? hex(ACCENT2, op) : hex(SURFACE2, op));
            g_z.points[i] = {px, y - 10 * k, w, 28 * k};
            px += w + 6 * k;
        }
    }
}

int FicheGarage::appui(float x, float y) const {
    for (int p = 0; p < voituresPermises().n; p++) if (g_z.points[p].contient(x, y)) return p;
    return -1;
}

void FicheGarage::viderZones() { for (Zone& z : g_z.points) z = Zone{}; }

float FicheGarage::specs(Renderer& r, const MiseEnPage& mp, const CarDef& car) const {
    const float k = mp.k, op = mp.op, statH = mp.statH, y1 = mp.y1;
    float x = mp.px0 + mp.padX;
    // .specs : Puissance (ch), 0 à 100 (s), Vitesse max (km/h)
    {
        const std::string val[3] = {std::to_string(car.spec.ch), virgule(car.spec.s100), std::to_string(car.spec.vmax)};
        const char* dt[3] = {"Puissance", "0 à 100", "Vitesse max"};
        const char* un[3] = {"ch", "s", "km/h"};
        const float sy = y1 + statH / 2 - 26 * k;
        for (int i = 0; i < 3; i++) {
            ui::text(r, x, sy, 11 * k, ui::majuscules(dt[i]), hex(MUTED, op), ui::GAUCHE, 0.16f, false);
            ui::text(r, x, sy + 16 * k, 34 * k, val[i], hex(TEXTE, op), ui::GAUCHE, 0, false, true);
            const float vw = ui::width(34 * k, val[i]);
            ui::text(r, x + vw + 4 * k, sy + 34 * k, 12 * k, ui::majuscules(un[i]), hex(MUTED, op), ui::GAUCHE, 0.1f, false);
            x += std::max(vw + 8 * k + ui::width(12 * k, ui::majuscules(un[i]), 0.1f), ui::width(11 * k, ui::majuscules(dt[i]), 0.16f)) + 26 * k;
        }
        x += 6 * k;
    }
    return x;
}

void FicheGarage::stats(Renderer& r, const MiseEnPage& mp, const CarDef& car, float x) const {
    const float k = mp.k, op = mp.op, y1 = mp.y1, bx = mp.bx;
    // .statlist : 4 barres sur 10 (Vitesse, Accélération, Maniabilité, Puissance)
    {
        const Stats st = carStats(car);
        const int v[4] = {st.v, st.a, st.m, st.p};
        const char* lab[4] = {"Vitesse", "Accélération", "Maniabilité", "Puissance"};
        const float sx = x, sw = bx - 32 * k - sx, barX = sx + 112 * k + 12 * k, barW = sw - 112 * k - 12 * k - 12 * k - 28 * k;
        for (int i = 0; i < 4; i++) {
            const float yy = y1 + i * 24 * k;
            ui::text(r, sx, yy, 13 * k, lab[i], hex(TAG, op), ui::GAUCHE, 0, false);
            const float by = yy + 4 * k, bh = 10 * k;
            biseau(r, barX, by, barW, bh, 1.8f * k, hex(SURFACE2, op));
            // remplissage en dégradé cyan → orange, découpé en 10 cases (mask repeating-linear-gradient)
            const float seg = barW / 10;
            for (int c = 0; c < v[i]; c++) {
                const float u = (c + 0.5f) / 10;
                const vec4 col{hex(ACCENT2).x + (hex(ACCENT).x - hex(ACCENT2).x) * u, hex(ACCENT2).y + (hex(ACCENT).y - hex(ACCENT2).y) * u,
                               hex(ACCENT2).z + (hex(ACCENT).z - hex(ACCENT2).z) * u, op};
                biseau(r, barX + c * seg, by, seg - 3 * k, bh, 1.8f * k, col);
            }
            ui::text(r, barX + barW + 12 * k + 28 * k, yy, 14 * k, std::to_string(v[i]), hex(TEXTE, op), ui::DROITE, 0, false, true);
        }
    }
}

} // namespace wfs::ecrans::garage
