// WaForSpeed — garage : fiche de la voiture (WFS-534) : voir garage_fiche.h. Code repris tel quel de garage.cpp.
#include "garage_fiche.h"
#include <algorithm>
#include <cmath>
#include "../game/boutique.h"     // WFS-550 (RG-04) : voiture possédée ou bloquée
#include "../game/sauvegarde.h"
#include "../render/renderer.h"
#include "jauges.h"               // WFS-550 : cadrans fins, barres fines, segments de la position
#include "ecrans/textes_banc.h"   // WFS-507 : noms propres notés à part (banc « textes »)
#include "langue.h"   // WFS-507 : textes par ui::tr
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
// WFS-550 (RG-01) : remplissage des cadrans = mêmes échelles que les notes sur 10 (carStats), sans arrondi : la voiture la plus
// forte remplit le cadran, et cadran et barre d'une même voiture racontent la même chose
float fractionPuissance(const CarSpec& s) { return clampN(s.ch / 55.f, 0, 10) / 10; }
float fractionZeroCent(const CarSpec& s) { return clampN(13 - s.s100 * 1.05f, 0, 10) / 10; }
float fractionVmax(const CarSpec& s) { return clampN((s.vmax - 170) / 14.f, 0, 10) / 10; }
} // namespace

void FicheGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float W = mp.W, k = mp.k, op = mp.op, gl = mp.gl, gt = mp.gt;
    const CarDef& car = CARS[e.index];
    const std::string id = ficheDe(car.id).id;   // WFS-507 : type et accroche par ui::tr (garage.type.<id>, garage.accroche.<id>)
    // ---------- .show-head : marque · index, nom, type + accroche ----------
    {
        const float x = gl + std::clamp(W * 0.04f, 0.f, 64 * k);
        float y = gt + 8 * k;
        float np = std::min(84 * k, std::max(40 * k, W * 0.07f * 0.8f));
        {
            const textes::NomPropre nomPropre;   // WFS-507 : marque et modèle inventés, inchangés en anglais (RC-E20-03)
            const std::string eb = ui::majuscules(marque(car)) + " · " + deuxChiffres(placeDe(e.index) + 1) + " / " + deuxChiffres(voituresPermises().n);
            ui::text(r, x, y, 12 * k, eb, hex(ACCENT2, op), ui::GAUCHE, 0.22f, false);
            y += 22 * k;
            const std::string nm = ui::majuscules(modele(car));   // WFS-191 : nom de configgame.json
            if (const float nw = ui::width(np, nm); nw > W * 0.5f) np *= W * 0.5f / nw;   // WFS-191 (RG-08) : nom long réduit pour tenir
            ui::text(r, x, y, np, nm, hex(TEXTE, op), ui::GAUCHE, 0, true, true);
        }
        y += np * 1.0f + 8 * k;
        // WFS-147 (ANO-47) : libellé de 13 px au moins (règle du HUD : 11 px du web portés à 13 px), calé au pixel entier
        // (en 720p, le « I » de « HYBRIDE » tombait entre deux pixels et le « D » perdait son trait) ; dès 13 px
        // (2400x1080 : 16,5 px), taille et place inchangées
        const std::string body = ui::majuscules(ui::tr(("garage.type." + id).c_str()));
        const float cp = std::max(11 * k, 13.f), cy = cp > 11 * k ? (20 * k - cp) / 2 : 4 * k;
        const float cw = ui::width(cp, body, 0.16f) + 20 * k;
        biseau(r, x, y, cw, 20 * k, 3.5f * k, hex(ACCENT2, op));   // .chip : fond cyan, skewX(-10deg)
        ui::text(r, std::round(x + 10 * k), std::round(y + cy), cp, body, hex(ON_ACCENT, op), ui::GAUCHE, 0.16f, false);
        ui::text(r, x + cw + 12 * k, y + 1 * k, 15 * k, ui::tr(("garage.accroche." + id).c_str()), hex(TAG, op), ui::GAUCHE, 0, true);
    }
}

int FicheGarage::appui(float x, float y) const {
    for (int p = 0; p < voituresPermises().n; p++) if (g_z.points[p].contient(x, y)) return p;
    return -1;
}

void FicheGarage::viderZones() { for (Zone& z : g_z.points) z = Zone{}; }

void FicheGarage::position(Renderer& r, const MiseEnPage& mp, int index) const {
    const float W = mp.W, k = mp.k, op = mp.op;
    // WFS-550 (RG-04, maquette 15) : position dans la liste, sous le plateau
    // un segment incliné par voiture permise (le nombre suit le catalogue) : blanc = possédée, gris = bloquée, orange allongé =
    // voiture affichée ; « 17 / 17 » à droite. Remplace les points de l'en-tête (mêmes zones d'appui g_z.points, élargies au doigt).
    {
        const VoituresPermises& vp = voituresPermises();
        const int nb = vp.n;
        jauges::Segment etats[CAR_COUNT];
        for (int i = 0; i < nb; i++)
            etats[i] = vp.idx[i] == index ? jauges::Segment::Affichee
                     : boutique::possedeVoiture(sauvegarde(), CARS[vp.idx[i]].id) ? jauges::Segment::Possedee : jauges::Segment::Bloquee;
        const float sh = 5 * k, sy = mp.py0 - 34 * k;
        jauges::Rect z[CAR_COUNT];
        const float lw = jauges::segments(r, W / 2, sy, sh, etats, nb, op, z);
        for (int i = 0; i < nb; i++) g_z.points[i] = {z[i].x - 2 * k, sy - 12 * k, z[i].w + 4 * k, 28 * k};
        const textes::NomPropre np;   // chiffres seulement
        const std::string place = std::to_string(placeDe(index) + 1), total = " / " + std::to_string(nb);
        const float tx = W / 2 + lw / 2 + 14 * k, ty = sy + sh / 2 - 9 * k;
        ui::text(r, tx, ty, 18 * k, place, hex(TEXTE, op), ui::GAUCHE, 0, false, true);
        ui::text(r, tx + ui::width(18 * k, place) + 2 * k, ty + 5 * k, 13 * k, total, hex(MUTED, op), ui::GAUCHE, 0, false, true);
    }
}

float FicheGarage::specs(Renderer& r, const MiseEnPage& mp, const CarDef& car) const {
    const float k = mp.k, op = mp.op, statH = mp.statH, y1 = mp.y1;
    position(r, mp, (int)(&car - CARS));   // WFS-550 (RG-04) : ici, le haut du panneau (py0) est connu
    float x = mp.px0 + mp.padX;
    // .specs : Puissance (ch), 0 à 100 (s), Vitesse max (km/h) ; WFS-550 (RG-01, maquette 13) : trois cadrans à arc fin
    // cyan → orange (valeur au centre, unité dessous, libellé sous le cadran) ; chiffres et libellés inchangés
    {
        const std::string val[3] = {std::to_string(car.spec.ch), virgule(car.spec.s100), std::to_string(car.spec.vmax)};
        const char* dt[3] = {ui::tr("garage.puissance").c_str(), ui::tr("garage.zero_cent").c_str(), ui::tr("garage.vmax").c_str()};   // WFS-507
        const char* un[3] = {"ch", "s", "km/h"};   // unités inchangées en anglais (WFS-507 RG-01)
        const float fr[3] = {fractionPuissance(car.spec), fractionZeroCent(car.spec), fractionVmax(car.spec)};
        const float rayon = 38 * k, ep = 4.5f * k, cy = y1 + 2 * k + rayon, ly = y1 + statH - 11 * k;   // maquette 13 : grands cadrans
        for (int i = 0; i < 3; i++) {
            const std::string lib = ui::majuscules(dt[i]);
            const float colonne = std::max(2 * rayon + 16 * k, ui::width(11 * k, lib, 0.16f) + 8 * k), cx = x + colonne / 2;
            jauges::cadran(r, cx, cy, rayon, ep, fr[i], op);
            float vp = 30 * k;   // valeur réduite si elle ne tient pas dans l'arc (« 1000 »)
            if (const float vw = ui::width(vp, val[i]); vw > 2 * rayon - 14 * k) vp *= (2 * rayon - 14 * k) / vw;
            ui::text(r, cx, cy - vp * 0.62f, vp, val[i], hex(TEXTE, op), ui::CENTRE, 0, false, true);
            ui::text(r, cx, cy + vp * 0.42f, 11 * k, un[i], hex(MUTED, op), ui::CENTRE, 0.04f, false);
            ui::text(r, cx, ly, 11 * k, lib, hex(MUTED, op), ui::CENTRE, 0.16f, false);
            x += colonne + 14 * k;
        }
        x += 18 * k;
    }
    return x;
}

void FicheGarage::stats(Renderer& r, const MiseEnPage& mp, const CarDef& car, float x) const {
    const float k = mp.k, op = mp.op, y1 = mp.y1, bx = mp.bx;
    // .statlist : 4 barres sur 10 (Vitesse, Accélération, Maniabilité, Puissance)
    {
        const Stats st = carStats(car);
        const int v[4] = {st.v, st.a, st.m, st.p};
        const std::string* lab[4] = {&ui::tr("garage.note.vitesse"), &ui::tr("garage.note.acceleration"), &ui::tr("garage.note.maniabilite"), &ui::tr("garage.note.puissance")};   // WFS-507
        const float sx = x, sw = bx - 32 * k - sx, barX = sx + 112 * k + 12 * k, barW = sw - 112 * k - 12 * k - 12 * k - 28 * k;
        for (int i = 0; i < 4; i++) {
            const float yy = y1 + i * 24 * k;
            ui::text(r, sx, yy, 13 * k, *lab[i], hex(TAG, op), ui::GAUCHE, 0, false);
            // WFS-550 (RG-01, maquette 13) : barre fine graduée (10 cases), dégradé cyan → orange de WFS-548 sur toute la largeur
            jauges::barreFine(r, barX, yy + 7 * k, barW, 3 * k, v[i] / 10.f, op, 10);
            ui::text(r, barX + barW + 12 * k + 28 * k, yy, 14 * k, std::to_string(v[i]), hex(TEXTE, op), ui::DROITE, 0, false, true);
        }
    }
}

} // namespace wfs::ecrans::garage
