// WaForSpeed — garage : colonne d'action (WFS-534 ; WFS-489 : achat des couleurs en ★) : voir garage_achat.h.
#include "garage_achat.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/commandes_tactiles.h"   // dessin2d : étoile
#include "../render/renderer.h"
#include "langue.h"
#include "text.h"

namespace wfs::ecrans::garage {
namespace {
const char ETOILE[] = "\xE2\x98\x85";   // ★ (U+2605) : absent de la police Rajdhani, dessiné à la place
constexpr float LARGEUR_ETOILE = 0.95f;   // place d'une étoile dans le texte, × taille de la police
}

// ---------- étoile et texte avec ★ ----------
void etoile(Renderer& r, float cx, float cy, float R, const vec4& c) {
    // 5 pointes (triangles) autour d'un pentagone central : chaque morceau est convexe (dessin2d::polygone)
    const float ri = R * 0.42f;
    vec2 ext[5], in[5];
    for (int i = 0; i < 5; i++) {
        const float a = i * 2 * PI / 5, b = a + PI / 5;   // 0 = vers le haut, sens horaire (comme dessin2d::polaire)
        ext[i] = {cx + std::sin(a) * R, cy - std::cos(a) * R};
        in[i] = {cx + std::sin(b) * ri, cy - std::cos(b) * ri};
    }
    const dessin2d::Teinte t = dessin2d::uni(c);
    dessin2d::polygone(r, in, 5, t);
    for (int i = 0; i < 5; i++) { const vec2 p[3] = {in[(i + 4) % 5], ext[i], in[i]}; dessin2d::polygone(r, p, 3, t); }
}

float largeurEtoiles(float px, const std::string& s, float esp) {
    float w = 0; size_t a = 0;
    for (size_t b; (b = s.find(ETOILE, a)) != std::string::npos; a = b + 3) w += ui::width(px, s.substr(a, b - a), esp) + px * LARGEUR_ETOILE;
    return w + ui::width(px, s.substr(a), esp);
}

float texteEtoiles(Renderer& r, float x, float y, float px, const std::string& s, const vec4& c, int align, float esp) {
    const float w = largeurEtoiles(px, s, esp);
    float cx = align == ui::CENTRE ? x - w / 2 : align == ui::DROITE ? x - w : x;
    size_t a = 0;
    for (size_t b; (b = s.find(ETOILE, a)) != std::string::npos; a = b + 3) {
        const std::string morceau = s.substr(a, b - a);
        if (!morceau.empty()) { ui::text(r, cx, y, px, morceau, c, ui::GAUCHE, esp, false); cx += ui::width(px, morceau, esp); }
        etoile(r, cx + px * LARGEUR_ETOILE / 2, y + px * 0.52f, px * 0.42f, c);   // centrée sur la hauteur des chiffres
        cx += px * LARGEUR_ETOILE;
    }
    if (a < s.size()) ui::text(r, cx, y, px, s.substr(a), c, ui::GAUCHE, esp, false);
    return w;
}

// ---------- boutons (colonne de droite) : Choisir / Acheter (.btn orange), Essai (WFS-499), Retour (.btn--ghost) ----------
void AchatGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    e_ = e;
    if (manque_ > 0 && !(e == eManque_)) manque_ = 0;   // autre couleur montrée : le bandeau disparaît
    const float k = mp.k, op = mp.op, bx = mp.bx, bw = mp.bw;
    const Sauvegarde& s = sauvegarde();
    const char* car = CARS[e.index].id;
    const bool bloquee = voitureBloquee(s, car);   // WFS-499 : 3 boutons (Acheter, Essai, Retour), mêmes tailles, écart réduit à 8 px
    const float hb = 52 * k, hg = 46 * k, gap = (bloquee ? 8 : 12) * k, tot = hb + gap + hg + (bloquee ? gap + hg : 0), by = mp.py0 + (mp.panelH - tot) / 2;
    const bool pret = e.voiturePrete;
    const Manque m = manque(s, e);
    biseau(r, bx, by, bw, hb, 14 * k, pret ? hex(ACCENT, op) : hex(SURFACE2, op));
    if (!pret) ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.chargement")), hex(MUTED, op), ui::CENTRE, 0.1f, false);
    else if (bloquee) texteEtoiles(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.acheter", nombreLangue(boutique::prixVoiture(car)))), hex(ON_ACCENT, op), ui::CENTRE, 0.1f);
    else if (m.n > 0) texteEtoiles(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.acheter", nombreLangue(m.total))), hex(ON_ACCENT, op), ui::CENTRE, 0.1f);
    else ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.choisir")), hex(ON_ACCENT, op), ui::CENTRE, 0.1f, false);
    g_z.choisir = {bx, by, bw, hb};
    float gy = by + hb + gap;
    if (bloquee) {   // WFS-499 (RG-03) : « Essai gratuit » (info : cyan) puis « Essai déjà fait » (inactif)
        const bool libre = essaiPossible(s, car);
        biseau(r, bx, gy, bw, hg, 14 * k, hex(SURFACE2, op));
        if (libre) biseau(r, bx, gy, 4 * k, hg, 14 * k * 4 / bw, hex(ACCENT2, op));   // liseré cyan : bouton actif
        const std::string t = ui::majuscules(ui::tr(libre ? "garage.essai" : "garage.essai_fait"));
        ui::textBoite(r, bx + bw / 2, gy, bw - 28 * k, hg, 16 * k, t, hex(libre ? ACCENT2 : MUTED, op), ui::CENTRE, 0.1f, false);
        g_z.essai = {bx, gy, bw, hg};
        gy += hg + gap;
    } else {
        g_z.essai = Zone{};
    }
    biseau(r, bx, gy, bw, hg, 14 * k, hex(SURFACE2, op));
    ui::text(r, bx + bw / 2, gy + 13 * k, 16 * k, ui::majuscules(ui::tr("garage.retour")), hex(TEXTE, op), ui::CENTRE, 0.1f, false);
    g_z.retour = {bx, gy, bw, hg};
    // WFS-499 (RG-01) : solde d'étoiles toujours affiché, en haut à droite du garage (au-dessus de la flèche « suivante »)
    texteEtoiles(r, mp.W - mp.gr, mp.gt + 6 * k, 14 * k, ui::majuscules(ui::tr("garage.solde", nombreLangue(boutique::solde(s)))), hex(TEXTE, op), ui::DROITE, 0.12f);
    // WFS-499 (RG-01) : cadenas juste au-dessus du point (vignette) de chaque voiture bloquée, entre l'étiquette du type et les
    // points (dessous, il passait sur le toit de la voiture en 4:3 : revue de la PR #404)
    for (int p = 0; p < voituresPermises().n; p++)
        if (const Zone& z = g_z.points[p]; z.w > 0 && voitureBloquee(s, CARS[voituresPermises().idx[p]].id)) cadenas(r, z.x + z.w / 2, z.y + 1 * k, 9 * k, op);
    // WFS-499 (RG-01) : carte de la voiture bloquée, au-dessus du panneau, à droite : cadenas, prix, équivalents
    if (bloquee) {
        const long long prix = boutique::prixVoiture(car);
        const std::string t1 = nombreLangue(prix) + " ★", t2 = texteEquivalentVoiture(prix), t0 = ui::majuscules(ui::tr("garage.bloquee"));
        const float pad = 18 * k, cad = 30 * k, w1 = cad + 12 * k + largeurEtoiles(30 * k, t1), w = std::max({w1, ui::width(14 * k, t2), ui::width(14 * k, t0, 0.14f)}) + 2 * pad;
        const float h = 100 * k, x = mp.px1 - mp.padX - w, y = mp.py0 - h - 12 * k;
        // .card : verre teinté, coin supérieur droit coupé, liseré cyan à gauche (information)
        const float b = 14 * k;
        const vec2 c[5] = {{x, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h}, {x, y + h}};
        dessin2d::polygone(r, c, 5, dessin2d::uni(vec4{GLASS.x, GLASS.y, GLASS.z, 0.9f * op}));
        dessin2d::contour(r, c, 5, std::max(1.f, k), vec4{BORD.x, BORD.y, BORD.z, BORD.w * op});
        r.uiRect(x, y, 3 * k, h, hex(ACCENT2, op));
        ui::text(r, x + pad, y + 12 * k, 14 * k, t0, hex(TAG, op), ui::GAUCHE, 0.14f, false);
        cadenas(r, x + pad + cad / 2, y + 50 * k, cad, op);
        texteEtoiles(r, x + pad + cad + 12 * k, y + 34 * k, 30 * k, t1, hex(TEXTE, op), ui::GAUCHE);
        ui::text(r, x + pad, y + 76 * k, 14 * k, t2, hex(TAG, op), ui::GAUCHE, 0, false);
    }
    // bandeau « Il te manque N ★ » : glissé au-dessus du panneau, bord gauche rouge (notification de design-street-racing)
    if (manque_ > 0) {
        const std::string t = ui::tr("garage.manque", nombreLangue(manque_));
        const float w = largeurEtoiles(16 * k, t) + 40 * k, h = 40 * k, x = mp.px1 - mp.padX - w, y = mp.py0 - h - 12 * k;
        biseau(r, x, y, w, h, 10 * k, hex(SURFACE2, 0.96f * op));
        biseau(r, x, y, 5 * k, h, 10 * k * 5 / w, hex(0xFF1F4B, op));
        texteEtoiles(r, x + 22 * k, y + 11 * k, 16 * k, t, hex(TEXTE, op), ui::GAUCHE);
    }
}

void AchatGarage::dessinerFenetre(Renderer& r, const MiseEnPage& mp) {
    if (fenetre_.ouverte()) fenetre_.dessiner(r, mp.W, mp.H, mp.k, mp.gl, mp.gr, mp.gt, mp.gb);
}

bool AchatGarage::boutonFenetre(const char* nom, float& x, float& y) const {
    const Zone& z = nom[0] == 'c' ? fenetre_.zoneConfirmer() : fenetre_.zoneAnnuler();
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2;
    return true;
}

bool AchatGarage::essaiDemande() const { return e_.voiturePrete && essaiPossible(sauvegarde(), CARS[e_.index].id); }

bool AchatGarage::acheter() {
    if (!e_.voiturePrete) return false;
    Sauvegarde& s = sauvegarde();
    if (voitureBloquee(s, CARS[e_.index].id)) {   // WFS-499 (RG-02) : jamais d'achat de voiture sans la fenêtre de confirmation
        fenetre_.ouvrir(e_.index);
        return true;
    }
    const Manque m = manque(s, e_);
    if (m.n == 0) return false;   // tout est possédé : « Choisir »
    const char* car = CARS[e_.index].id;
    const long long solde = boutique::solde(s);
    if (solde < m.total) {   // RG-03 : rien n'est débité
        manque_ = m.total - solde; eManque_ = e_;
        WFS_LOG("garage : achat refusé pour %s : %lld ★ demandées, solde %lld ★ : il manque %lld ★", car, m.total, solde, manque_);
        return true;
    }
    for (int i = 0; i < m.n; i++) {
        const perso::Achat a = boutique::acheterObjet(s, *m.objets[i], car);
        WFS_LOG("garage : achat %s (%s) pour %s : %s, solde %lld ★", m.objets[i]->id, perso::nomType(m.objets[i]->type), car, perso::texteAchat(a), boutique::solde(s));
    }
    // RG-04 : le choix acheté est gardé tout de suite pour cette voiture
    if (manque(s, e_).n == 0 && s.actif()) {
        s.paint[car] = PAINTS[e_.peinture].id;
        if (e_.peinture2 >= 0) s.paint2[car] = PAINTS[e_.peinture2].id; else s.paint2.erase(car);
        if (e_.finition > 0) s.finition[car] = BOUTIQUE_FINITIONS_IDS[e_.finition]; else s.finition.erase(car);   // WFS-491 (RG-03) : finition-<voiture>
        s.enregistrer();
    }
    manque_ = 0;
    return true;
}

int AchatGarage::appui(float x, float y) const {
    if (g_z.choisir.contient(x, y)) return CHOISIR;
    if (g_z.retour.contient(x, y)) return RETOUR;
    if (g_z.essai.contient(x, y)) return ESSAI;   // WFS-499
    return -1;
}

void AchatGarage::viderZones() { g_z.choisir = g_z.retour = g_z.essai = Zone{}; manque_ = 0; fenetre_.fermer(); }

} // namespace wfs::ecrans::garage
