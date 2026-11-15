// WaForSpeed — garage : colonne d'action (WFS-534 ; WFS-489 : achat des couleurs en ★) : voir garage_achat.h.
#include "garage_achat.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../core/log.h"
#include "../game/boutique.h"
#include "../game/commandes_tactiles.h"   // dessin2d : étoile
#include "../game/pret.h"   // WFS-502 : voiture prêtée
#include "../render/renderer.h"
#include "langue.h"
#include "text.h"
#include "boutons.h"   // WFS-549 : boutons au modèle R19
#include "garage_onglets.h"   // WFS-551 : bouton rond

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
    const float k = mp.k, op = mp.op;
    const Sauvegarde& s = sauvegarde();
    const char* car = CARS[e.index].id;
    const bool bloquee = voitureBloquee(s, car);   // WFS-499 : 3 boutons (Acheter, Essai, Retour), mêmes tailles, écart réduit à 8 px
    const bool pret = e.voiturePrete;
    const Manque m = manque(s, e);
    // WFS-551 (maquettes 13, 15, 16) : grand bouton rond Start/Stop à droite (« Choisir », ou « Acheter » et le prix dessous)
    const float cx = mp.cxBouton, cy = mp.cyBouton, R = mp.rBouton;
    if (!pret) onglets::boutonRond(r, cx, cy, R, ui::majuscules(ui::tr("garage.chargement")), "", op, true);
    else if (bloquee) onglets::boutonRond(r, cx, cy, R, ui::majuscules(ui::tr("garage.acheter_court")), "★ " + nombreLangue(boutique::prixVoiture(car)), op, false);
    else if (m.n > 0) onglets::boutonRond(r, cx, cy, R, ui::majuscules(ui::tr("garage.acheter_court")), "★ " + nombreLangue(m.total), op, false);
    else onglets::boutonRond(r, cx, cy, R, ui::majuscules(ui::tr("garage.choisir")), "", op, false);
    g_z.choisir = {cx - R, cy - R, 2 * R, 2 * R};
    const bool pretAttente = bloquee && pret::etat(s.cadeau) == pret::Etat::EnAttente && s.cadeau.pretVoiture == car;   // WFS-502 (RG-01)
    if (bloquee && mp.onglet == ONGLET_VOITURE) {   // WFS-551 : onglet « Voiture » seulement (ailleurs, le nom montré est à cette place) ; WFS-499 (RG-03) : « Essai gratuit » (info : cyan) puis « Essai déjà fait » (inactif) ; WFS-502 : « Activer le prêt »
        // WFS-551 : bouton de verre au-dessus du bouton rond (la maquette 15 n'en montre pas : la fonction reste)
        const bool libre = pretAttente || essaiPossible(s, car);
        const std::string t = ui::majuscules(ui::tr(pretAttente ? "pret.garage.activer" : libre ? "garage.essai" : "garage.essai_fait"));
        const float w = std::max(150 * k, ui::width(15 * k, t, 0.1f) + 36 * k), h = 40 * k, x = cx - w / 2, y = cy - R - 18 * k - h;
        const vec4 ct = hex(libre ? ACCENT2 : MUTED, op);
        ui::boutons::verre(r, x, y, w, h, t, op, false, 15 * k, &ct);
        g_z.essai = {x, y, w, h};
    } else {
        g_z.essai = Zone{};
    }
    // WFS-551 (maquettes 13, 15, 16) : « Retour » en autocollant en bas à gauche
    const float rw = 150 * k, rh = 50 * k, rx = mp.gl + std::max(40 * k, mp.W * 0.05f), ry = mp.py0 + mp.panelH / 2 - rh / 2;
    ui::boutons::autocollant(r, rx, ry, rw, rh, ui::majuscules(ui::tr("garage.retour")), op, false, 17 * k);   // WFS-549 (RG-02) : sortie
    g_z.retour = {rx, ry, rw, rh};
    // WFS-551 (maquettes 15, 16) : solde d'étoiles sous le bouton rond ; voiture bloquée : « ★ solde / prix » (valeurs réelles)
    {
        const std::string t1 = "★ " + nombreLangue(boutique::solde(s)), t2 = bloquee && !pretAttente ? " / " + nombreLangue(boutique::prixVoiture(car)) : "";
        const float p1 = 20 * k, p2 = 14 * k, w1 = largeurEtoiles(p1, t1), w2 = t2.empty() ? 0 : ui::width(p2, t2), y0 = cy + R + 18 * k;
        const float x0 = cx - (w1 + w2) / 2;
        texteEtoiles(r, x0, y0, p1, t1, hex(TEXTE, op), ui::GAUCHE);
        if (!t2.empty()) ui::text(r, x0 + w1, y0 + p1 - p2 - 1 * k, p2, t2, hex(MUTED, op), ui::GAUCHE, 0, false);
    }
    // WFS-499 (RG-01) : solde d'étoiles toujours affiché, en haut à droite du garage (au-dessus de la flèche « suivante »)
    texteEtoiles(r, mp.W - mp.gr, mp.gt + 6 * k, 14 * k, ui::majuscules(ui::tr("garage.solde", nombreLangue(boutique::solde(s)))), hex(TEXTE, op), ui::DROITE, 0.12f);
    // WFS-551 (revue du lead dev, maquette 15) : plus de petits cadenas au-dessus des points des voitures : le cadenas-jauge est à
    // côté du nom de la voiture bloquée
    // WFS-499 (RG-01) : carte de la voiture bloquée, au-dessus du panneau, à droite : cadenas, prix, équivalents ; WFS-551 (RG-04) :
    // remplacée par le cadenas-jauge et « ★ solde / prix », gardée seulement pour la voiture prêtée en attente (WFS-502)
    if (bloquee && pretAttente) {
        const long long prix = boutique::prixVoiture(car);
        // WFS-502 (RG-01) : voiture prêtée pas encore activée : « Voiture prêtée » et « À activer dans 23 h 12 min » (carte de même largeur)
        const std::string t1 = nombreLangue(prix) + " ★";
        const std::string t2 = pretAttente ? ui::tr("pret.bandeau.attente", texteDuree(pret::resteActivation(s.cadeau, pret::maintenant()))) : texteEquivalentVoiture(prix);
        const std::string t0 = ui::majuscules(ui::tr(pretAttente ? "pret.garage.titre" : "garage.bloquee"));
        const float pad = 18 * k, cad = 30 * k, w1 = cad + 12 * k + largeurEtoiles(30 * k, t1), w = std::max({w1, ui::width(14 * k, t2), ui::width(14 * k, t0, 0.14f)}) + 2 * pad;
        const float h = 88 * k, y = mp.cyBouton - mp.rBouton - 70 * k - h;   // WFS-551 : au-dessus du bouton rond et de « Essai »
        float x = std::min(mp.cxBouton - w / 2, mp.W - mp.gr - w);
        // jamais sur la flèche « suivante » (panneau plus haut avec la rangée des finitions, WFS-491) : la carte passe à sa gauche
        if (const Zone& n = g_z.next; n.w > 0 && y < n.y + n.h && x + w > n.x) x = std::min(x, n.x - w - 10 * k);
        // .card : verre teinté, coin supérieur droit coupé, liseré cyan à gauche (information)
        const float b = 14 * k;
        const vec2 c[5] = {{x, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h}, {x, y + h}};
        dessin2d::polygone(r, c, 5, dessin2d::uni(vec4{GLASS.x, GLASS.y, GLASS.z, 0.9f * op}));
        dessin2d::contour(r, c, 5, std::max(1.f, k), vec4{BORD.x, BORD.y, BORD.z, BORD.w * op});
        r.uiRect(x, y, 3 * k, h, hex(ACCENT2, op));
        ui::text(r, x + pad, y + 10 * k, 14 * k, t0, hex(TAG, op), ui::GAUCHE, 0.14f, false);
        cadenas(r, x + pad + cad / 2, y + 45 * k, cad, op);
        texteEtoiles(r, x + pad + cad + 12 * k, y + 29 * k, 30 * k, t1, hex(TEXTE, op), ui::GAUCHE);
        ui::text(r, x + pad, y + 66 * k, 14 * k, t2, hex(TAG, op), ui::GAUCHE, 0, false);
    }
    // WFS-502 (RG-03) : voiture prêtée, prêt actif : carte au même endroit que celle d'une voiture bloquée, liseré orange (temps
    // compté) : « Voiture prêtée » · « Prêtée : 2 h 41 min » · « Garde-la pour toujours : 2 200 ★ » ; relue à chaque image (minute)
    if (!bloquee && pret::utilisable(s, car)) {
        const long long reste = pret::restant(s.cadeau, pret::maintenant());
        const std::string t0 = ui::majuscules(ui::tr("pret.garage.titre")), t1 = ui::tr("pret.garage.restant", texteDuree(reste));
        const std::string t2 = ui::tr("pret.garder", nombreLangue(boutique::prixVoiture(car)));
        const float pad = 18 * k, w = std::max({largeurEtoiles(26 * k, t1), largeurEtoiles(14 * k, t2), ui::width(14 * k, t0, 0.14f)}) + 2 * pad;
        const float h = 88 * k, y = mp.cyBouton - mp.rBouton - 70 * k - h;   // WFS-551 : au-dessus du bouton rond et de « Essai »
        float x = std::min(mp.cxBouton - w / 2, mp.W - mp.gr - w);
        if (const Zone& n = g_z.next; n.w > 0 && y < n.y + n.h && x + w > n.x) x = std::min(x, n.x - w - 10 * k);
        const float b = 14 * k;
        const vec2 c[5] = {{x, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h}, {x, y + h}};
        dessin2d::polygone(r, c, 5, dessin2d::uni(vec4{GLASS.x, GLASS.y, GLASS.z, 0.9f * op}));
        dessin2d::contour(r, c, 5, std::max(1.f, k), vec4{BORD.x, BORD.y, BORD.z, BORD.w * op});
        r.uiRect(x, y, 3 * k, h, hex(ACCENT, op));
        ui::text(r, x + pad, y + 10 * k, 14 * k, t0, hex(TAG, op), ui::GAUCHE, 0.14f, false);
        texteEtoiles(r, x + pad, y + 31 * k, 26 * k, t1, hex(TEXTE, op), ui::GAUCHE);
        texteEtoiles(r, x + pad, y + 66 * k, 14 * k, t2, hex(TAG, op), ui::GAUCHE);
    }
    // bandeau « Il te manque N ★ » : glissé au-dessus du panneau, bord gauche rouge (notification de design-street-racing)
    if (manque_ > 0) {
        const std::string t = ui::tr("garage.manque", nombreLangue(manque_));
        const float w = largeurEtoiles(16 * k, t) + 40 * k, h = 40 * k, x = std::min(mp.cxBouton - w / 2, mp.W - mp.gr - w), y = mp.yNom - h - 12 * k;   // WFS-551 : au-dessus du nom montré
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

bool AchatGarage::activerPret() {
    Sauvegarde& s = sauvegarde();
    const char* car = CARS[e_.index].id;
    if (!e_.voiturePrete || !voitureBloquee(s, car) || pret::etat(s.cadeau) != pret::Etat::EnAttente || s.cadeau.pretVoiture != car) return false;
    if (pret::activer(s, pret::maintenant())) { WFS_LOG("garage : « Activer le prêt » de la %s", car); s.enregistrer(); }
    return true;
}

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
        if (const perso::Objet* k = e_.skin >= 0 && e_.skin < BOUTIQUE_SKINS ? perso::objet(BOUTIQUE_SKINS_IDS[e_.skin]) : nullptr; !k) s.skin.erase(car);   // WFS-494 : skin-<voiture>
        else if (perso::possede(s, *k, car)) s.skin[car] = k->id;
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
