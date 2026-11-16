// WaForSpeed — garage : fiche de la voiture (WFS-534) : voir garage_fiche.h. Code repris tel quel de garage.cpp.
#include "garage_fiche.h"
#include <algorithm>
#include <cmath>
#include "../game/boutique.h"     // WFS-550 (RG-04) : voiture possédée ou bloquée
#include "../game/sauvegarde.h"
#include "../render/renderer.h"
#include "jauges.h"               // WFS-550 : cadrans fins, barres fines, segments de la position
#include "garage_achat.h"         // WFS-551 (accroche) : cadenas-jauge
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
        const float x = gl + std::clamp(W * 0.04f, 0.f, 28 * k);   // WFS-557 : bord gauche à 44 (maquettes 13, 15)
        float y = gt + 16 * k;
        float np = std::min(70 * k, std::max(40 * k, W * 0.07f));   // WFS-557 (maquettes 13, 15) : nom de 70 px
        {
            const textes::NomPropre nomPropre;   // WFS-507 : marque et modèle inventés, inchangés en anglais (RC-E20-03)
            // WFS-551 (accroche, maquettes 06, 07, 13, 15, 16) : la marque seule, en gris clair (la place « 05 / 17 » est sous le plateau)
            const std::string eb = ui::majuscules(marque(car));
            { ui::AvecPolice inter(ui::Police::InterMedium); ui::text(r, x, y, 14 * k, eb, hex(0xC9CDDA, op), ui::GAUCHE, 0.22f, false); }   // WFS-557 : Inter Medium de 14
            y += 17 * k;
            const std::string nm = ui::majuscules(modele(car));   // WFS-191 : nom de configgame.json
            if (const float nw = ui::width(np, nm); nw > W * 0.5f) np *= W * 0.5f / nw;   // WFS-191 (RG-08) : nom long réduit pour tenir
            ui::text(r, x, y, np, nm, hex(TEXTE, op), ui::GAUCHE, 0, true, true);
            // WFS-551 (RG-04, accroche) : voiture bloquée : cadenas-jauge juste à côté du nom (solde / prix réels)
            if (const Sauvegarde& sv = sauvegarde(); voitureBloquee(sv, car.id))
                cadenasJauge(r, x + ui::width(np, nm) + np * 0.28f, y + np * 0.08f, np * 0.92f, partJauge(boutique::solde(sv), boutique::prixVoiture(car.id)), op);
        }
        y += np * 0.92f + 4 * k;
        // WFS-147 (ANO-47) : libellé de 13 px au moins (règle du HUD : 11 px du web portés à 13 px), calé au pixel entier
        // (en 720p, le « I » de « HYBRIDE » tombait entre deux pixels et le « D » perdait son trait) ; dès 13 px
        // (2400x1080 : 16,5 px), taille et place inchangées
        const std::string body = ui::majuscules(ui::tr(("garage.type." + id).c_str()));
        const float cp = std::max(11 * k, 13.f), cy = cp > 11 * k ? (20 * k - cp) / 2 : 4 * k;
        const float cw = ui::width(cp, body, 0.16f) + 20 * k;
        biseau(r, x, y, cw, 20 * k, 3.5f * k, hex(TEXTE, op));   // .chip : WFS-551 (accroche, maquettes) : étiquette blanche, texte noir
        ui::text(r, std::round(x + 10 * k), std::round(y + cy), cp, body, hex(ON_ACCENT, op), ui::GAUCHE, 0.16f, false, true);
        ui::AvecPolice inter(ui::Police::Inter);   // WFS-557 : accroche en Inter de 15 (maquettes 13, 15)
        ui::text(r, x + cw + 12 * k, y + 1.5f * k, 15 * k, mp.sousTitre.empty() ? ui::tr(("garage.accroche." + id).c_str()) : mp.sousTitre, hex(TEXTE, op), ui::GAUCHE, 0, true);   // WFS-551 : couleur · finition · skin selon l'onglet
        // WFS-551 (maquette 15, RG-11) : la place des segments ; WFS-550 (RG-04) : leur dessin (position()), onglet Voiture seulement
        if (mp.onglet != ONGLET_VOITURE) { viderZones(); return; }   // maquettes 06, 07, 16 : pas de segments
        position(r, mp, e.index);
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
        const float sh = 5 * k, sy = mp.H - mp.gb - 20 * k - 140 * k - 26 * k;   // WFS-551 (RG-11) : sous le plateau, au-dessus du panneau
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

// WFS-557 (maquettes 13 et 15 mesurées, ordre de l'utilisateur du 2026-11-16 : maquette à 100 %) : places relevées depuis le coin
// haut gauche du panneau (894 × 140) : cadrans centrés à 81, 212 et 342 (rayon 46, à 66 du haut), libellés à 130 ; barres : libellés
// à 434, barres de 538 à 838, valeurs alignées à 870, rangées à 35, 57, 79 et 101
float FicheGarage::specs(Renderer& r, const MiseEnPage& mp, const CarDef& car) const {
    const float k = mp.k, op = mp.op, x0 = mp.px0, y0 = mp.py0;
    const std::string val[3] = {std::to_string(car.spec.ch), virgule(car.spec.s100), std::to_string(car.spec.vmax)};
    const char* dt[3] = {ui::tr("garage.puissance").c_str(), ui::tr("garage.zero_cent").c_str(), ui::tr("garage.vmax").c_str()};   // WFS-507
    const char* un[3] = {"ch", "s", "km/h"};   // unités inchangées en anglais (WFS-507 RG-01)
    const float fr[3] = {fractionPuissance(car.spec), fractionZeroCent(car.spec), fractionVmax(car.spec)};
    const float cxs[3] = {81, 212, 342}, rayon = 46 * k, ep = 5 * k, cy = y0 + 66 * k;
    for (int i = 0; i < 3; i++) {
        const float cx = x0 + cxs[i] * k;
        jauges::cadran(r, cx, cy, rayon, ep, fr[i], op);
        float vp = 32 * k;   // valeur réduite si elle ne tient pas dans l'arc (« 1000 »)
        if (const float vw = ui::width(vp, val[i]); vw > 2 * rayon - 18 * k) vp *= (2 * rayon - 18 * k) / vw;
        ui::text(r, cx, cy - 5 * k - vp * 0.5f, vp, val[i], hex(TEXTE, op), ui::CENTRE, 0, false, true);
        ui::AvecPolice inter(ui::Police::InterMedium);
        ui::text(r, cx, cy + 16 * k - 5 * k, 10.5f * k, un[i], hex(0xB8BCC8, op), ui::CENTRE, 0.02f, false);
        ui::text(r, cx, y0 + 130 * k - 6 * k, 11 * k, ui::majuscules(dt[i]), hex(0xB8BCC8, op), ui::CENTRE, 0.18f, false);
    }
    return x0 + 434 * k;
}

void FicheGarage::stats(Renderer& r, const MiseEnPage& mp, const CarDef& car, float x) const {
    const float k = mp.k, op = mp.op, x0 = mp.px0, y0 = mp.py0;
    // .statlist : 4 barres sur 10 (Vitesse, Accélération, Maniabilité, Puissance)
    const Stats st = carStats(car);
    const int v[4] = {st.v, st.a, st.m, st.p};
    const std::string* lab[4] = {&ui::tr("garage.note.vitesse"), &ui::tr("garage.note.acceleration"), &ui::tr("garage.note.maniabilite"), &ui::tr("garage.note.puissance")};   // WFS-507
    const float barX = x0 + 538 * k, barW = 300 * k;
    for (int i = 0; i < 4; i++) {
        const float yc = y0 + (35 + 22 * i) * k;
        { ui::AvecPolice inter(ui::Police::Inter); ui::text(r, x, yc - 7.5f * k, 14.5f * k, *lab[i], hex(0xE2E4EA, op), ui::GAUCHE, 0, false); }
        // WFS-550 (RG-01, maquette 13) : barre fine graduée (10 cases), dégradé cyan → orange de WFS-548 sur toute la largeur
        jauges::barreFine(r, barX, yc - 2 * k, barW, 4 * k, v[i] / 10.f, op, 10);
        ui::text(r, x0 + 870 * k, yc - 9 * k, 18 * k, std::to_string(v[i]), hex(TEXTE, op), ui::DROITE, 0, false, true);
    }
}

} // namespace wfs::ecrans::garage
