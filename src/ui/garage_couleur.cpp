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
#include "boutons.h"   // WFS-549 : boutons au modèle R19
#include "garage_onglets.h"   // WFS-551 : panneau de l'onglet

namespace wfs::ecrans::garage {
namespace {
// mesures de la rangée, unités de 720 px de haut (× k)
constexpr float PASTILLE = 30, LIGNE_PRIX = 14, ONGLET_H = 30, CHEVRON = 16, SEUIL_GLISSE = 10;
constexpr unsigned SUR_TITRE_GOUTTES = 0xA3A8B5;
// WFS-551 (maquette 06) : gouttes en grand dans le panneau de verre de l'onglet « Peinture »
constexpr float GOUTTE = 40, PAS_GOUTTE = 56, MARGE_PANNEAU = 28;   // WFS-551 : sur-titre gris de la R19 (verre::SUR_TITRE, WFS-548)

const std::string& nomCouleur(int p) { return ui::tr(("couleur." + std::string(PAINTS[p].id)).c_str()); }

// WFS-551 (RG-01) : prix de base d'une couleur (configgame.json, boutique.couleurs ; −1 = gratuite partout)
int prixBase(int p) {
    if (p < 0 || p >= NB_PEINTURES || p >= BOUTIQUE_COULEURS) return -1;
    const auto& o = config().boutique.couleur[p];
    return o.gratuit ? -1 : o.prix;
}

// WFS-551 (RG-01) : goutte brillante de la couleur c (sRGB 0..1) : liseré clair, dégradé, reflet blanc ; h = hauteur, (cx, haut) = pointe
void goutte(Renderer& r, float cx, float haut, float h, unsigned c, float op) {
    vec2 p[1 + GOUTTE_ARC];
    const vec4 coul = hex(c, op);
    const float l = 0.299f * coul.x + 0.587f * coul.y + 0.114f * coul.z;   // une couleur sombre garde un liseré lisible
    const float m = std::max(1.f, h * 0.05f);
    const int n = contourGoutte(p, cx, haut - m, h + 2 * m);
    dessin2d::polygone(r, p, n, dessin2d::uni({1, 1, 1, (l < 0.25f ? 0.38f : 0.16f) * op}));   // liseré
    contourGoutte(p, cx, haut, h);
    const vec4 clair{std::min(1.f, coul.x * 0.7f + 0.3f), std::min(1.f, coul.y * 0.7f + 0.3f), std::min(1.f, coul.z * 0.7f + 0.3f), op};
    const vec4 sombre{coul.x * 0.62f, coul.y * 0.62f, coul.z * 0.62f, op};
    dessin2d::polygone(r, p, n, {clair, sombre, haut + h * 0.2f, haut + h});
    // reflet : petite ellipse blanche inclinée en haut à gauche du ventre, et un point
    const float R = h * 0.36f, vx = cx - R * 0.42f, vy = haut + h - R * 1.25f;
    vec2 e[16];
    for (int i = 0; i < 16; i++) {
        const float t = 2 * PI * i / 16, ax = R * 0.16f, ay = R * 0.36f, sn = std::sin(0.45f), cs = std::cos(0.45f);
        const float x = ax * std::cos(t), y = ay * std::sin(t);
        e[i] = {vx + x * cs - y * sn, vy + x * sn + y * cs};
    }
    dessin2d::polygone(r, e, 16, dessin2d::uni({1, 1, 1, 0.85f * op}));
    r.uiCircle(cx + R * 0.38f, haut + h - R * 0.45f, std::max(0.8f, R * 0.09f), {1, 1, 1, 0.55f * op});
}
} // namespace

int groupeCouleur(int peinture) {
    const int base = prixBase(peinture);
    if (base < 0) return 0;
    int vus[BOUTIQUE_COULEURS], n = 0;   // prix de base payants rencontrés avant, dans l'ordre des couleurs
    for (int i = 0; i < peinture && i < NB_PEINTURES; i++) {
        const int b = prixBase(i);
        if (b < 0) continue;
        bool deja = false;
        for (int j = 0; j < n; j++) deja = deja || vus[j] == b;
        if (!deja) vus[n++] = b;
    }
    for (int j = 0; j < n; j++) if (vus[j] == base) return 1 + j;
    return 1 + n;
}

// petit cadenas (WFS-491 : partagé avec la rangée des finitions, garage_commun.h ; WFS-499 : aussi les voitures bloquées) (interface 2D) : anse en arc et corps aux coins coupés, cerné de sombre pour se lire sur toute couleur
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
    const float k = mp.k, op = mp.op;
    const Sauvegarde& sv = sauvegarde();
    const char* car = CARS[e.index].id;
    // WFS-551 (maquette 06) : « Carrosserie » / « Toit et rétros » sous le nom de la voiture, à gauche
    {
        float cx = mp.gl + std::clamp(mp.W * 0.04f, 0.f, 64 * k);
        const float sy = mp.gt + 184 * k;
        float wTot = 0;
        for (int o = 0; o < 2; o++) wTot += ui::width(11 * k, ui::majuscules(ui::tr(o ? "garage.couleur.toit" : "garage.couleur.principale")), 0.14f) + 22 * k;
        ui::boutons::selecteurFond(r, cx, sy, wTot + 6 * k, ONGLET_H * k, op);
        for (int o = 0; o < 2; o++) {
            const std::string t = ui::majuscules(ui::tr(o ? "garage.couleur.toit" : "garage.couleur.principale"));
            const float w = ui::width(11 * k, t, 0.14f) + 22 * k;
            const bool on = toit_ == (o == 1);
            ui::boutons::selecteurChoix(r, cx, sy, w, ONGLET_H * k, t, on, op, false, 11 * k);
            g_z.onglets[o] = {cx, sy - 4 * k, w, ONGLET_H * k + 8 * k};
            manette::focus(g_z.onglets[o]);
            cx += w + 6 * k;
        }
    }
    // WFS-551 (maquette 06) : panneau de verre en bas, gouttes en grand, rangées par prix ; chevrons si tout ne tient pas
    const int n = nbItems();
    const float s = GOUTTE * k, pas = PAS_GOUTTE * k, ecart = pas - s, pad = MARGE_PANNEAU * k, ch = CHEVRON * k * 1.4f;
    const float wMax = mp.px1 - mp.px0, wPlein = n * pas - ecart + 2 * pad;
    const bool defile = wPlein > wMax;
    const float xp = panneauOnglet(r, mp, defile ? wMax : wPlein);
    const float cx = xp + pad, yy = mp.py0 + 50 * k;   // yy : haut de la case d'une goutte
    const float x0 = defile ? cx + ch : cx;
    visibles_ = defile ? std::max(1, (int)((wMax - 2 * pad - 2 * ch + ecart) / pas)) : n;
    pas_ = pas;
    const int sel = toit_ ? second_[e.index] + 1 : e.peinture;
    if (doigt_ < 0 && (sel != selVue_ || e.index != indexVu_ || toit_ != toitVu_)) {   // nouvelle sélection : la rendre visible
        if (sel < premier_) premier_ = sel;
        if (sel >= premier_ + visibles_) premier_ = sel - visibles_ + 1;
        selVue_ = sel; indexVu_ = e.index; toitVu_ = toit_;
    }
    premier_ = std::clamp(premier_, 0, std::max(0, n - visibles_));
    rangee_ = {x0 - 4 * k, yy - 4 * k, visibles_ * pas, s + 30 * k};
    for (Zone& z : items_) z = Zone{};
    for (Zone& z : g_z.peintures) z = Zone{};
    for (Zone& z : g_z.toit) z = Zone{};
    for (int c = 0; c < 2; c++) {   // chevrons du défilement (seulement s'il reste des pastilles de ce côté)
        g_z.chevrons[c] = Zone{};
        const bool reste = c == 0 ? premier_ > 0 : premier_ + visibles_ < n;
        if (!defile || !reste) continue;
        const float xc = c == 0 ? cx + ch * 0.4f : x0 + visibles_ * pas - ecart + ch * 0.6f, yc = yy + s / 2, d = c == 0 ? 1.f : -1.f;
        const vec2 chv[3] = {{xc + 4 * k * d, yc - 8 * k}, {xc - 4 * k * d, yc}, {xc + 4 * k * d, yc + 8 * k}};
        dessin2d::trait(r, chv, 3, 2.4f * k, hex(ACCENT2, op));
        g_z.chevrons[c] = {xc - ch * 0.6f, yy - 4 * k, ch * 1.2f, s + 8 * k};
        manette::focus(g_z.chevrons[c]);
    }
    // WFS-551 (RG-01) : couleur portée (sauvegarde) ; sans sauvegarde (banc), la couleur montrée
    const perso::Aspect porte = perso::aspectCourse(sv, car);
    const int porteeP = sv.actif() ? (porte.peinture ? peintureParId(porte.peinture->id) : -1) : e.peinture;
    const int porteeT = sv.actif() ? (porte.peinture2 ? peintureParId(porte.peinture2->id) + 1 : 0) : sel;
    int groupePrec = -2;
    for (int j = premier_; j < premier_ + visibles_ && j < n; j++) {
        const float x = x0 + (j - premier_) * pas;
        const bool comme = toit_ && j == 0;                     // « Comme la carrosserie »
        const int p = comme ? e.peinture : (toit_ ? j - 1 : j);  // couleur dessinée (indice dans PAINTS)
        const long long prix = comme ? 0 : prixPastille(sv, car, p, toit_);
        const bool on = j == sel, portee = toit_ ? j == porteeT : j == porteeP;
        // RG-01 : rangées par prix : titre au-dessus de la 1re goutte visible de chaque rangée, trait fin entre deux rangées
        const int gr = comme ? 0 : groupeCouleur(p);
        if (gr != groupePrec) {
            if (groupePrec >= -1) r.uiRect(x - ecart / 2 - 0.5f * k, yy - 26 * k, std::max(1.f, k), s + 30 * k, {1, 1, 1, 0.16f * op});
            const int base = comme ? -1 : prixBase(p);
            const std::string titre = gr == 0 ? ui::tr("garage.couleur.groupe.classiques")
                : ui::tr(gr == 1 ? "garage.couleur.groupe.vives" : "garage.couleur.groupe.speciales",
                         ui::nombre((long long)std::lround(base * perso::coefficient(car))));
            texteEtoiles(r, x, yy - 32 * k, 11 * k, ui::majuscules(titre), hex(SUR_TITRE_GOUTTES, op), ui::GAUCHE, 0.12f);
            groupePrec = gr;
        }
        const float hg = (on ? 1.18f : 1.f) * s, cxg = x + s / 2;   // RG-01 : aperçu = goutte plus grande
        if (on) {   // halo et trait orange dessous
            r.uiCircle(cxg, yy + s * 0.68f, s * 0.62f, hex(ACCENT, 0.16f * op));
            r.uiRect(cxg - s * 0.3f, yy + s + 6 * k, s * 0.6f, 2.5f * k, hex(ACCENT, op));
        }
        goutte(r, cxg, yy + s - hg, hg, PAINTS[p].hex, op);
        if (portee) r.uiCircle(cxg, yy + s + 14 * k, 2.5f * k, hex(TEXTE, op));   // RG-01 : couleur portée = point blanc dessous
        if (comme) {   // « = » sombre sur une couleur claire, clair sur une couleur sombre
            const unsigned h = PAINTS[p].hex;
            const bool claire = 0.299f * ((h >> 16) & 255) + 0.587f * ((h >> 8) & 255) + 0.114f * (h & 255) > 140;
            ui::text(r, cxg, yy + s * 0.42f, 18 * k, "=", hex(claire ? ON_ACCENT : TEXTE, op), ui::CENTRE, 0, false);
        }
        if (prix > 0) {   // RG-01 : petit cadenas sur les gouttes à acheter ; « Toit et rétros » : prix total dessous (droit compris)
            cadenas(r, x + s * 0.8f, yy + s * 0.8f, 12 * k, op);
            if (toit_) texteEtoiles(r, cxg, yy + s + 18 * k, 10 * k, ui::nombre(prix) + " ★", hex(TAG, op), ui::CENTRE);
        }
        items_[j] = {x - ecart / 2, yy - 8 * k, pas, s + 34 * k};
        (toit_ ? g_z.toit[j] : g_z.peintures[p]) = items_[j];
        manette::focus(items_[j]);   // WFS-80 : pastille au focus de la manette (A = toucher)
    }
    // nom de la couleur montrée (au-dessus du bouton rond, maquette 06) et, si elle n'est pas possédée, « aperçu · pas encore à toi »
    // et ses équivalents (RC-E18-11)
    const bool commeSel = toit_ && sel == 0;
    const long long prixSel = commeSel ? 0 : prixPastille(sv, car, toit_ ? sel - 1 : sel, toit_);   // même si la goutte est hors de la vue
    nomMontre(r, mp, commeSel ? ui::tr("garage.couleur.comme") : nomCouleur(toit_ ? sel - 1 : sel), prixSel > 0);
    if (prixSel > 0) texteEtoiles(r, mp.cxBouton, mp.yNom + 48 * k, 11 * k, texteEquivalent(prixSel), hex(MUTED, op), ui::CENTRE);
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
            if (std::abs(t.x - x0_) > SEUIL_GLISSE * pas_ / PAS_GOUTTE) glisse_ = true;   // 10 px de 720 px de haut
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
