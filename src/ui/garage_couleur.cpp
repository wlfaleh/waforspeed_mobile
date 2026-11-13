// WaForSpeed — garage : rangée des couleurs (WFS-534 ; WFS-489 : 28 couleurs, couleur secondaire, cadenas, prix) : voir garage_couleur.h.
#include "garage_couleur.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "../core/log.h"
#include "../game/commandes_tactiles.h"   // dessin2d : cadenas, chevrons
#include "../game/game.h"                 // Touch
#include "../game/personnalisation.h"
#include "../platform/manette.h"   // WFS-80 : pastille au focus de la manette
#include "../render/renderer.h"
#include "../render/vehicule_peinture2.h"   // WFS-488 : code de la couleur 2
#include "garage_achat.h"   // étoile, texte avec ★, équivalents
#include "langue.h"
#include "text.h"

namespace wfs::ecrans::garage {
namespace {
// mesures de la rangée, unités de 720 px de haut (× k)
constexpr float PASTILLE = 30, ECART = 8, LIGNE_PRIX = 14, ONGLET_H = 30, CHEVRON = 16, NOM = 190, SEUIL_GLISSE = 10;

const std::string& nomCouleur(int p) { return ui::tr(("couleur." + std::string(PAINTS[p].id)).c_str()); }

// petit cadenas (interface 2D) : anse en arc et corps aux coins coupés, cerné de sombre pour se lire sur toute couleur
void cadenas(Renderer& r, float cx, float cy, float u, float op) {
    const vec4 fond = hex(BG, 0.85f * op), clair = hex(TEXTE, op);
    auto corps = [&](float marge, const vec4& c) {
        const float w = u * 0.86f + 2 * marge, h = u * 0.62f + 2 * marge, x = cx - w / 2, y = cy - u * 0.06f - marge, b = u * 0.12f;
        const vec2 p[8] = {{x + b, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h - b}, {x + w - b, y + h}, {x + b, y + h}, {x, y + h - b}, {x, y + b}};
        dessin2d::polygone(r, p, 8, dessin2d::uni(c));
    };
    const float ra = u * 0.26f, ya = cy - u * 0.06f;
    dessin2d::arc(r, cx, ya, ra, u * 0.13f + u * 0.12f, -PI / 2, PI / 2, fond);   // cerne de l'anse
    corps(u * 0.07f, fond);
    dessin2d::arc(r, cx, ya, ra, u * 0.13f, -PI / 2, PI / 2, clair);
    corps(0, clair);
    r.uiCircle(cx, cy + u * 0.22f, u * 0.08f, fond);   // trou de la serrure
}
} // namespace

float CouleurGarage::hauteur(float k) { return (PASTILLE + LIGNE_PRIX) * k; }

void CouleurGarage::ouvrir(int (&peinture)[CAR_COUNT]) {
    toit_ = false; doigt_ = -1; glisse_ = false; selVue_ = indexVu_ = -1;
    const Sauvegarde& s = sauvegarde();
    if (!s.actif()) return;   // banc d'essai sans sauvegarde : couleurs de la partie gardées
    for (int i = 0; i < CAR_COUNT; i++) {   // RG-03 : un aperçu non acheté est oublié, la voiture reprend sa dernière couleur possédée
        const perso::Aspect a = perso::aspectCourse(s, CARS[i].id);
        peinture[i] = peintureParId(a.peinture ? a.peinture->id : ficheDe(CARS[i].id).paint);
        second_[i] = a.peinture2 ? peintureParId(a.peinture2->id) : -1;
    }
}

int CouleurGarage::peinture2(int index) const { return index >= 0 && index < CAR_COUNT ? second_[index] : -1; }

float CouleurGarage::code2(int index) const {
    const int p = peinture2(index);
    return p >= 0 ? vehicule_peinture2::code(PAINTS[p].hex) : vehicule_peinture2::UNE_COULEUR;
}

void CouleurGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float k = mp.k, op = mp.op, bx = mp.bx;
    const Sauvegarde& sv = sauvegarde();
    const char* car = CARS[e.index].id;
    const float yy = mp.y1 + mp.statH + 14 * k, s = PASTILLE * k, pas = (PASTILLE + ECART) * k;
    float cx = mp.px0 + mp.padX;
    // RG-02 : onglets « Principale » / « Toit et rétros » (à la place du libellé « Couleur »)
    for (int o = 0; o < 2; o++) {
        const std::string t = ui::majuscules(ui::tr(o ? "garage.couleur.toit" : "garage.couleur.principale"));
        const float w = ui::width(11 * k, t, 0.14f) + 22 * k;
        const bool on = toit_ == (o == 1);
        biseau(r, cx, yy, w, ONGLET_H * k, 5 * k, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
        biseau(r, cx + 2 * k, yy + 2 * k, w - 4 * k, ONGLET_H * k - 4 * k, 5 * k, on ? hex(SURFACE2, op) : hex(SURFACE2, 0.85f * op));
        ui::text(r, cx + w / 2, yy + 9 * k, 11 * k, t, hex(on ? TEXTE : MUTED, op), ui::CENTRE, 0.14f, false);
        g_z.onglets[o] = {cx, yy - 4 * k, w, ONGLET_H * k + 8 * k};
        manette::focus(g_z.onglets[o]);
        cx += w + 6 * k;
    }
    cx += 12 * k;
    // RG-01 : pastilles de taille fixe ; place comptée jusqu'à la zone du nom ; chevrons si tout ne tient pas
    const int n = nbItems();
    const float place = bx - 32 * k - NOM * k - cx;
    const bool defile = n * pas - ECART * k > place;
    const float x0 = defile ? cx + CHEVRON * k : cx;
    visibles_ = defile ? std::max(1, (int)((place - 2 * CHEVRON * k + ECART * k) / pas)) : n;
    pas_ = pas;
    const int sel = toit_ ? second_[e.index] + 1 : e.peinture;
    if (doigt_ < 0 && (sel != selVue_ || e.index != indexVu_ || toit_ != toitVu_)) {   // nouvelle sélection : la rendre visible
        if (sel < premier_) premier_ = sel;
        if (sel >= premier_ + visibles_) premier_ = sel - visibles_ + 1;
        selVue_ = sel; indexVu_ = e.index; toitVu_ = toit_;
    }
    premier_ = std::clamp(premier_, 0, std::max(0, n - visibles_));
    rangee_ = {x0 - 4 * k, yy - 4 * k, visibles_ * pas, hauteur(k) + 8 * k};
    for (Zone& z : items_) z = Zone{};
    for (Zone& z : g_z.peintures) z = Zone{};
    for (Zone& z : g_z.toit) z = Zone{};
    for (int c = 0; c < 2; c++) {   // chevrons du défilement (seulement s'il reste des pastilles de ce côté)
        g_z.chevrons[c] = Zone{};
        const bool reste = c == 0 ? premier_ > 0 : premier_ + visibles_ < n;
        if (!defile || !reste) continue;
        const float xc = c == 0 ? cx + CHEVRON * k * 0.4f : x0 + visibles_ * pas - ECART * k + CHEVRON * k * 0.6f, yc = yy + s / 2, d = c == 0 ? 1.f : -1.f;
        const vec2 ch[3] = {{xc + 4 * k * d, yc - 8 * k}, {xc - 4 * k * d, yc}, {xc + 4 * k * d, yc + 8 * k}};
        dessin2d::trait(r, ch, 3, 2.4f * k, hex(ACCENT2, op));
        g_z.chevrons[c] = {xc - CHEVRON * k * 0.6f, yy - 4 * k, CHEVRON * k * 1.2f, s + 8 * k};
        manette::focus(g_z.chevrons[c]);
    }
    long long prixSel = 0;
    for (int j = premier_; j < premier_ + visibles_ && j < n; j++) {
        const float x = x0 + (j - premier_) * pas;
        const bool comme = toit_ && j == 0;                     // « Comme la carrosserie »
        const int p = comme ? e.peinture : (toit_ ? j - 1 : j);  // couleur dessinée (indice dans PAINTS)
        const long long prix = comme ? 0 : prixPastille(sv, car, p, toit_);
        const bool on = j == sel;
        if (on) { biseau(r, x - 4 * k, yy - 4 * k, s + 8 * k, s + 8 * k, s * 0.18f, hex(ACCENT2, 0.35f * op)); prixSel = prix; }   // halo
        biseau(r, x - 2 * k, yy - 2 * k, s + 4 * k, s + 4 * k, s * 0.18f, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
        biseau(r, x, yy, s, s, s * 0.18f, hex(PAINTS[p].hex, op));
        if (comme) {   // « = » sombre sur une couleur claire, clair sur une couleur sombre
            const unsigned h = PAINTS[p].hex;
            const bool claire = 0.299f * ((h >> 16) & 255) + 0.587f * ((h >> 8) & 255) + 0.114f * (h & 255) > 140;
            ui::text(r, x + s / 2 + s * 0.09f, yy + 4 * k, 18 * k, "=", hex(claire ? ON_ACCENT : TEXTE, op), ui::CENTRE, 0, false);
        }
        if (prix > 0) {   // RG-03 : cadenas et prix en ★ dessous
            cadenas(r, x + s * 0.72f, yy + s * 0.66f, 11 * k, op);
            texteEtoiles(r, x + s / 2 + s * 0.09f, yy + s + 3 * k, 10 * k, ui::nombre(prix) + " ★", hex(TAG, op), ui::CENTRE);
        }
        items_[j] = {x - 4 * k, yy - 4 * k, s + 8 * k, hauteur(k) + 8 * k};
        (toit_ ? g_z.toit[j] : g_z.peintures[p]) = items_[j];
        manette::focus(items_[j]);   // WFS-80 : pastille au focus de la manette (A = toucher)
    }
    // nom de la couleur montrée et, si elle n'est pas possédée, son prix et ses équivalents (RC-E18-11)
    const float xn = bx - 32 * k - NOM * k + 10 * k;
    const bool commeSel = toit_ && sel == 0;
    ui::text(r, xn, yy + 1 * k, 15 * k, commeSel ? ui::tr("garage.couleur.comme") : nomCouleur(toit_ ? sel - 1 : sel), hex(TEXTE, op), ui::GAUCHE, 0, false);
    if (prixSel > 0) texteEtoiles(r, xn, yy + 21 * k, 11 * k, texteEquivalent(prixSel), hex(MUTED, op), ui::GAUCHE);
}

int CouleurGarage::itemSous(float x, float y) const {
    for (int j = premier_; j < premier_ + visibles_ && j <= NB_PEINTURES; j++) if (items_[j].contient(x, y)) return j;
    return -1;
}

int CouleurGarage::appui(float x, float y) const {
    if (toit_) return -1;
    const int j = itemSous(x, y);
    return j >= 0 && j < NB_PEINTURES ? j : -1;
}

void CouleurGarage::choisirItem(int item, int index, int& peinture) {
    if (item < 0 || item >= nbItems() || index < 0 || index >= CAR_COUNT) return;
    Sauvegarde& s = sauvegarde();
    const char* car = CARS[index].id;
    if (!toit_) {
        peinture = item;
        const long long prix = prixPastille(s, car, item, false);
        WFS_LOG("garage : peinture %s%s", PAINTS[item].id, prix > 0 ? (" (aperçu, " + std::to_string(prix) + " ★)").c_str() : "");
        if (prix == 0 && s.actif()) { s.paint[car] = PAINTS[item].id; s.enregistrer(); }   // RG-04 : choix possédé gardé tout de suite
        return;
    }
    second_[index] = item - 1;
    if (item == 0) {
        WFS_LOG("garage : toit et rétros comme la carrosserie");
        if (s.actif()) { s.paint2.erase(car); s.enregistrer(); }
        return;
    }
    const long long prix = prixPastille(s, car, item - 1, true);
    WFS_LOG("garage : toit et rétros %s%s", PAINTS[item - 1].id, prix > 0 ? (" (aperçu, " + std::to_string(prix) + " ★)").c_str() : "");
    if (prix == 0 && s.actif()) { s.paint2[car] = PAINTS[item - 1].id; s.enregistrer(); }
}

int CouleurGarage::toucher(const Touch& t, bool libre, int index, int& peinture) {
    if (doigt_ >= 0 && t.id == doigt_) {   // doigt suivi : glisser = défiler d'une pastille par pas
        if (t.down) {
            if (std::abs(t.x - x0_) > SEUIL_GLISSE * pas_ / (PASTILLE + ECART)) glisse_ = true;   // 10 px de 720 px de haut
            if (glisse_) premier_ = std::clamp(premier0_ - (int)std::lround((t.x - x0_) / pas_), 0, std::max(0, nbItems() - visibles_));
            return 2;
        }
        doigt_ = -1;
        if (!glisse_) choisirItem(itemAppui_, index, peinture);
        glisse_ = false;
        return 2;
    }
    if (!t.down || !libre) return 0;
    for (int o = 0; o < 2; o++) if (g_z.onglets[o].contient(t.x, t.y)) {
        if (toit_ != (o == 1)) WFS_LOG("garage : onglet %s", o ? "toit et rétros" : "principale");
        toit_ = o == 1;
        return 1;
    }
    for (int c = 0; c < 2; c++) if (g_z.chevrons[c].contient(t.x, t.y)) {
        premier_ = std::clamp(premier_ + (c == 0 ? -visibles_ : visibles_), 0, std::max(0, nbItems() - visibles_));
        return 1;
    }
    if (!rangee_.contient(t.x, t.y)) return 0;
    doigt_ = t.id; x0_ = t.x; premier0_ = premier_; glisse_ = false; itemAppui_ = itemSous(t.x, t.y);
    return 2;
}

void CouleurGarage::viderZones() {
    for (Zone& z : g_z.peintures) z = Zone{};
    for (Zone& z : items_) z = Zone{};
    for (Zone& z : g_z.toit) z = Zone{};
    rangee_ = g_z.chevrons[0] = g_z.chevrons[1] = g_z.onglets[0] = g_z.onglets[1] = Zone{};
    doigt_ = -1; glisse_ = false;
}

} // namespace wfs::ecrans::garage
