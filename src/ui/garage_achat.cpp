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

std::string texteEquivalent(long long prix) {
    // médailles : reprise du texte de boutique::equivalentMedailles (« 3 or + 1 argent + 1 bronze »), chaque nom traduit
    std::string med, t = boutique::equivalentMedailles(prix) + " + ";
    for (size_t a = 0, b; (b = t.find(" + ", a)) != std::string::npos; a = b + 3) {
        const std::string part = t.substr(a, b - a);
        const size_t sp = part.find(' ');
        if (sp == std::string::npos) continue;
        med += (med.empty() ? "" : " + ") + part.substr(0, sp) + " " + ui::tr(("medaille." + part.substr(sp + 1)).c_str());
    }
    const long long c = boutique::equivalentCentimes(prix);
    char eu[48];
    if (ui::langue::active() == ui::langue::Langue::Francais) std::snprintf(eu, sizeof eu, "%lld,%02lld €", c / 100, c % 100);
    else std::snprintf(eu, sizeof eu, "€%lld.%02lld", c / 100, c % 100);
    return ui::nombre(prix) + " ★ = " + med + " = " + eu;
}

// ---------- boutons (colonne de droite) : Choisir / Acheter (.btn orange), Retour (.btn--ghost) ----------
void AchatGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    e_ = e;
    if (manque_ > 0 && !(e == eManque_)) manque_ = 0;   // autre couleur montrée : le bandeau disparaît
    const float k = mp.k, op = mp.op, bx = mp.bx, bw = mp.bw;
    const float hb = 52 * k, hg = 46 * k, gap = 12 * k, tot = hb + gap + hg, by = mp.py0 + (mp.panelH - tot) / 2;
    const bool pret = e.voiturePrete;
    const Sauvegarde& s = sauvegarde();
    const Manque m = manque(s, e);
    biseau(r, bx, by, bw, hb, 14 * k, pret ? hex(ACCENT, op) : hex(SURFACE2, op));
    if (!pret) ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.chargement")), hex(MUTED, op), ui::CENTRE, 0.1f, false);
    else if (m.n > 0) texteEtoiles(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.acheter", ui::nombre(m.total))), hex(ON_ACCENT, op), ui::CENTRE, 0.1f);
    else ui::text(r, bx + bw / 2, by + 13 * k, 20 * k, ui::majuscules(ui::tr("garage.choisir")), hex(ON_ACCENT, op), ui::CENTRE, 0.1f, false);
    g_z.choisir = {bx, by, bw, hb};
    const float gy = by + hb + gap;
    biseau(r, bx, gy, bw, hg, 14 * k, hex(SURFACE2, op));
    ui::text(r, bx + bw / 2, gy + 13 * k, 16 * k, ui::majuscules(ui::tr("garage.retour")), hex(TEXTE, op), ui::CENTRE, 0.1f, false);
    g_z.retour = {bx, gy, bw, hg};
    // solde d'étoiles, au-dessus des boutons (RG-03)
    texteEtoiles(r, bx + bw, by - 19 * k, 12 * k, ui::majuscules(ui::tr("garage.solde", ui::nombre(boutique::solde(s)))), hex(TAG, op), ui::DROITE, 0.12f);
    // bandeau « Il te manque N ★ » : glissé au-dessus du panneau, bord gauche rouge (notification de design-street-racing)
    if (manque_ > 0) {
        const std::string t = ui::tr("garage.manque", ui::nombre(manque_));
        const float w = largeurEtoiles(16 * k, t) + 40 * k, h = 40 * k, x = mp.px1 - mp.padX - w, y = mp.py0 - h - 12 * k;
        biseau(r, x, y, w, h, 10 * k, hex(SURFACE2, 0.96f * op));
        biseau(r, x, y, 5 * k, h, 10 * k * 5 / w, hex(0xFF1F4B, op));
        texteEtoiles(r, x + 22 * k, y + 11 * k, 16 * k, t, hex(TEXTE, op), ui::GAUCHE);
    }
}

bool AchatGarage::acheter() {
    if (!e_.voiturePrete) return false;
    Sauvegarde& s = sauvegarde();
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
        s.enregistrer();
    }
    manque_ = 0;
    return true;
}

int AchatGarage::appui(float x, float y) const {
    if (g_z.choisir.contient(x, y)) return CHOISIR;
    if (g_z.retour.contient(x, y)) return RETOUR;
    return -1;
}

void AchatGarage::viderZones() { g_z.choisir = g_z.retour = Zone{}; manque_ = 0; }

} // namespace wfs::ecrans::garage
