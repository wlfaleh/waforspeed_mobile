// WaForSpeed — garage : achat des voitures et essai gratuit (WFS-499, épic E18, zone E5) : voir garage_achat.h.
//   - voiture bloquée, essai possible ;
//   - fenêtre de confirmation de l'achat (garage et écran de fin de l'essai) ;
//   - écran de fin d'un essai gratuit : bouton « Acheter N ★ » en haut à droite, au-dessus de l'écran de fin du mode, sans
//     toucher au code des modes (course solo, course à 5, Poursuite) : deux petits systèmes, l'un avant les modes pour
//     prendre les touchers (205), l'autre après eux pour dessiner par-dessus (445) ;
//   - garde-fou (RG-03) : une voiture bloquée ne part en course que pour son essai gratuit ; au compte à rebours d'une autre
//     course avec elle (« Rejouer », « Recommencer », duel), le jeu ouvre le garage sur cette voiture (Acheter, Retour).
// Sans sauvegarde active (bancs d'essai des autres tickets, parcours de la CI) : aucune voiture bloquée, rien ne change.
#include <algorithm>
#include <cmath>
#include <cstring>
#include "garage_achat.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/boutique.h"
#include "../game/commandes_tactiles.h"   // dessin2d : fond de la fenêtre
#include "verre.h"   // WFS-548 (R19) : fenêtre de verre, sur-titre gris, fond flouté
#include "../game/course.h"
#include "../game/course5.h"              // modeCourse : pas d'essai en duel
#include "../game/course_system.h"        // activeCourse : compte à rebours, écran de fin
#include "../game/duel_system.h"          // duelSession : pas d'essai en duel
#include "../game/game.h"                 // Touch
#include "../platform/manette.h"          // WFS-22 : focus de la manette
#include "../render/renderer.h"
#include "appui.h"
#include "langue.h"
#include "text.h"
#include "boutons.h"   // WFS-549 : boutons au modèle R19

namespace wfs::ecrans::garage {

// ---------------------------------------------------------------- cadenas-jauge (WFS-551, RG-04)
void cadenasJauge(Renderer& r, float x, float y, float h, float part, float op) {
    const float w = h * 0.78f, cx = x + w / 2, ouvert = part >= 1 ? 1.f : 0.f;
    const float yc = y + h * 0.40f, hc = h * 0.60f;   // corps : 60 % du bas
    // anse chrome : deux arcs (sombre dessous, clair dessus) ; à 100 % elle monte et pivote (ouverte)
    const float ra = w * 0.30f, ya = yc - h * 0.02f - ouvert * h * 0.12f, ep = std::max(1.5f, h * 0.10f);
    const float dx = ouvert * ra * 0.9f;
    dessin2d::arc(r, cx + dx, ya, ra, ep + std::max(1.f, h * 0.03f), -PI / 2, PI / 2, {0.12f, 0.13f, 0.16f, 0.9f * op});
    dessin2d::arc(r, cx + dx, ya, ra, ep, -PI / 2, PI / 2, {0.86f, 0.88f, 0.92f, op});
    dessin2d::arc(r, cx + dx, ya, ra, ep * 0.35f, -PI / 2.4f, -PI / 6, {1, 1, 1, 0.9f * op});   // reflet du chrome
    const vec2 pieds[2][2] = {{{cx + dx - ra, ya}, {cx + dx - ra, yc + ep}}, {{cx + dx + ra, ya}, {cx + dx + ra, yc - (ouvert > 0 ? h * 0.1f : -ep)}}};
    for (const auto& pd : pieds) dessin2d::trait(r, pd, 2, ep, {0.86f, 0.88f, 0.92f, op});
    // corps de verre sombre, liseré clair, coins arrondis
    vec2 c[ui::boutons::MAX_SOMMETS];
    const int n = ui::boutons::contourArrondi(c, x, yc, w, hc, h * 0.12f);
    dessin2d::polygone(r, c, n, {{0.20f, 0.22f, 0.28f, 0.92f * op}, {0.06f, 0.07f, 0.09f, 0.95f * op}, yc, yc + hc});
    dessin2d::contour(r, c, n, std::max(1.f, h * 0.025f), {1, 1, 1, 0.35f * op});
    // arc fin autour de la serrure : 270° (ouverture en bas), rempli de cyan → orange jusqu'à `part`, reste éteint
    const float sx = cx, sy = yc + hc * 0.52f, rr = hc * 0.30f, e2 = std::max(1.2f, h * 0.05f);
    const float a0 = -0.75f * PI, a1 = 0.75f * PI, af = a0 + (a1 - a0) * std::clamp(part, 0.f, 1.f);
    dessin2d::arc(r, sx, sy, rr, e2, a0, a1, {1, 1, 1, 0.16f * op});
    constexpr int SEG = 24;
    for (int i = 0; i < SEG; i++) {
        const float t0 = a0 + (a1 - a0) * i / SEG, t1 = a0 + (a1 - a0) * (i + 1) / SEG;
        if (t0 >= af) break;
        dessin2d::arc(r, sx, sy, rr, e2, t0, std::min(t1, af), verre::degradeMarque((float)i / (SEG - 1), op));
    }
    // serrure : rond et fente
    r.uiCircle(sx, sy - rr * 0.18f, rr * 0.30f, {0.92f, 0.93f, 0.96f, op});
    const vec2 fente[2] = {{sx, sy - rr * 0.1f}, {sx, sy + rr * 0.42f}};
    dessin2d::trait(r, fente, 2, rr * 0.26f, {0.92f, 0.93f, 0.96f, op});
}

bool essaiPossible(const Sauvegarde& s, const char* carId) {
    const bool duel = modeCourse() == ModeCourse::Duel || duelSession().inRoom();   // RG-08 de WFS-498 : pas d'essai en duel
    return voitureBloquee(s, carId) && !duel && boutique::essaiDisponible(s, carId);
}

// ---------------------------------------------------------------- fenêtre de confirmation (RG-02)
void FenetreAchatVoiture::ouvrir(int index) {
    ouverte_ = index >= 0 && index < CAR_COUNT;
    index_ = ouverte_ ? index : 0;
    if (ouverte_) WFS_LOG("garage : fenêtre d'achat de la %s (%d ★, solde %lld ★)", CARS[index_].id, boutique::prixVoiture(CARS[index_].id), boutique::solde(sauvegarde()));
}

void FenetreAchatVoiture::dessiner(Renderer& r, float W, float H, float k, float gl, float gr, float gt, float gb) {
    zAnnuler_ = zConfirmer_ = Zone{};
    if (!ouverte_) return;
    const Sauvegarde& s = sauvegarde();
    const CarDef& c = CARS[index_];
    const long long prix = std::max(0, boutique::prixVoiture(c.id)), solde = boutique::solde(s), manque = prix - solde;
    // fond flouté et assombri (WFS-548, RG-02, maquettes 27 et 28 : écran arrêté) : la fenêtre garde le regard et tous les touchers
    verre::fenetreSurEcran(r);   // garage ou écran de fin dessous : leur interface n'est plus dessinée (verre::couvert)
    verre::assombrir(r, W, H, 0.50f);
    const std::string nom = std::string(marque(c)) + " " + modele(c);   // WFS-191 : noms inventés
    const std::string titre = ui::majuscules(ui::tr("garage.achat_titre")), question = questionAchat(nom, prix);
    const std::string eq = texteEquivalentVoiture(prix), soldeT = ui::majuscules(ui::tr("garage.solde", nombreLangue(solde)));
    const std::string manqueT = manque > 0 ? ui::tr("garage.manque", nombreLangue(manque)) : std::string();
    const float bw = 200 * k, hb = 52 * k, hg = 46 * k, gapB = 16 * k, pad = 32 * k;
    // largeur : la question sur une ligne (police réduite si la fenêtre ne peut plus grandir), jamais hors de la zone sûre
    const float wMax = W - gl - gr;
    float pq = 22 * k;
    const float wq = largeurEtoiles(pq, question);
    const float w = std::min(wMax, std::max({wq + 2 * pad, 2 * bw + gapB + 2 * pad, 520 * k}));
    if (wq > w - 2 * pad) pq *= (w - 2 * pad) / wq;
    const float hManque = manque > 0 ? 40 * k + 14 * k : 0;
    const float h = 24 * k + 12 * k + 16 * k + pq + 14 * k + 14 * k + 12 * k + 13 * k + hManque + 24 * k + hb + 26 * k;
    const float x = gl + (wMax - w) / 2, y = gt + (H - gt - gb - h) / 2;
    // WFS-548 (RG-01, RG-03) : fenêtre de verre dépoli (avant : surface à coin coupé et liseré orange), sur-titre gris
    verre::panneau(r, x, y, w, h, {14 * k, std::max(1.f, 1.5f * k)});
    float yy = y + 24 * k;
    verre::surTitre(r, x + w / 2, yy, 12 * k, titre, ui::CENTRE);
    yy += 12 * k + 16 * k;
    texteEtoiles(r, x + w / 2, yy, pq, question, hex(TEXTE), ui::CENTRE);
    yy += pq + 14 * k;
    ui::text(r, x + w / 2, yy, 14 * k, eq, hex(TAG), ui::CENTRE, 0, false);
    yy += 14 * k + 12 * k;
    texteEtoiles(r, x + w / 2, yy, 13 * k, soldeT, hex(MUTED), ui::CENTRE, 0.12f);
    yy += 13 * k;
    if (manque > 0) {   // bandeau « Il te manque N ★ », bord gauche rouge (danger), rien n'est débité
        yy += 14 * k;
        const float wb = largeurEtoiles(16 * k, manqueT) + 44 * k, xb = x + (w - wb) / 2;
        biseau(r, xb, yy, wb, 40 * k, 10 * k, hex(SURFACE2));
        biseau(r, xb, yy, 5 * k, 40 * k, 10 * k * 5 / wb, hex(0xFF1F4B));
        texteEtoiles(r, xb + 24 * k, yy + 11 * k, 16 * k, manqueT, hex(TEXTE), ui::GAUCHE);
        yy += 40 * k;
    }
    yy += 24 * k;
    // boutons : Annuler (.btn--ghost 200 × 46) et Confirmer (.btn orange 200 × 52), tailles du garage (RG-06)
    const float rangee = manque > 0 ? bw : 2 * bw + gapB, xa = x + (w - rangee) / 2;
    zAnnuler_ = {xa, yy + (hb - hg) / 2, bw, hg};
    // WFS-549 (RG-02, RG-07) : « Annuler » en autocollant, même rectangle qu'avant
    ui::boutons::autocollant(r, zAnnuler_.x, zAnnuler_.y, bw, hg, ui::majuscules(ui::tr("garage.annuler")), 1, false, 17 * k);
    if (manque <= 0) {
        zConfirmer_ = {xa + bw + gapB, yy, bw, hb};
        // WFS-549 (RG-01) : confirmation de l'achat en bouton Start/Stop, même rectangle qu'avant
        ui::boutons::startStop(r, zConfirmer_.x, yy, bw, hb, ui::majuscules(ui::tr("garage.confirmer")), 1, false, "", 19 * k);
        manette::focus(zConfirmer_);
    }
    manette::focus(zAnnuler_, true);
}

int FenetreAchatVoiture::appui(float x, float y) const {
    if (!ouverte_) return RIEN;
    if (zConfirmer_.contient(x, y)) return CONFIRMER;
    if (zAnnuler_.contient(x, y)) return ANNULER;
    return DEHORS;
}

bool FenetreAchatVoiture::confirmer() {
    if (!ouverte_) return false;
    Sauvegarde& s = sauvegarde();
    const char* id = CARS[index_].id;
    const boutique::AchatVoiture a = boutique::acheterVoiture(s, id);   // débit une seule fois ; refus : rien de débité
    WFS_LOG("garage : achat de la %s confirmé : %s, solde %lld ★", id, boutique::texteAchat(a), boutique::solde(s));
    if (a != boutique::AchatVoiture::Achetee && a != boutique::AchatVoiture::DejaPossedee) return false;
    // RG-02 : cadenas retiré, voiture choisie (gardée à la relance) ; RG-04 : la fenêtre « Choisis ta skin offerte » s'ouvre à l'image suivante (WFS-494)
    if (s.actif()) { s.car = id; s.enregistrer(); }
    fermer();
    return a == boutique::AchatVoiture::Achetee;
}

// ---------------------------------------------------------------- écran de fin d'un essai gratuit (RG-03) et garde-fou
namespace {
FenetreAchatVoiture g_fenetreFin;   // fenêtre de confirmation de l'écran de fin
Zone g_acheterFin;                  // bouton « Acheter N ★ » posé à la dernière image (vide s'il n'est pas dessiné)

// écran de fin d'une course avec une voiture bloquée : son essai gratuit (le garde-fou empêche toute autre course avec elle)
bool finEssai(const GameView& g) {
    if (g.ecran != Ecran::Course || !g.car || duelSession().inRoom()) return false;
    const Course* c = activeCourse();
    return c && c->state == RaceState::Over && c->overTimer >= 0.9 && voitureBloquee(sauvegarde(), g.car->id);
}

int indexVoiture(const CarDef* car) { for (int i = 0; i < CAR_COUNT; i++) if (car == &CARS[i]) return i; return -1; }

// touchers : avant les modes (course 210, course à 5 222, Poursuite 224), qui gardent tous les touchers de leur écran de fin
class EssaiFinToucher final : public System {
public:
    const char* name() const override { return "garage : achat à la fin de l'essai (touchers)"; }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (appui_.consomme(t.id, t.down)) return true;
        if (!finEssai(g)) { if (g_fenetreFin.ouverte()) g_fenetreFin.fermer(); return false; }
        if (!t.down) return g_fenetreFin.ouverte();
        if (g_fenetreFin.ouverte()) {   // la fenêtre garde tous les touchers
            appui_.prendre(t.id);
            const int c = g_fenetreFin.appui(t.x, t.y);
            if (c == FenetreAchatVoiture::CONFIRMER) g_fenetreFin.confirmer();
            else if (c == FenetreAchatVoiture::ANNULER) { WFS_LOG("fin de l'essai : achat annulé"); g_fenetreFin.fermer(); }
            return true;
        }
        if (g_acheterFin.contient(t.x, t.y)) {
            appui_.prendre(t.id);
            WFS_LOG("fin de l'essai : « Acheter »");
            g_fenetreFin.ouvrir(indexVoiture(g.car));
            return true;
        }
        return false;   // « Rejouer », « Menu » : au mode
    }
    void onEcran(Ecran, Ecran, const GameView&) override { g_fenetreFin.fermer(); g_acheterFin = Zone{}; appui_.vider(); }
private:
    ui::FiltreAppui appui_;
};

// dessin : après les modes et l'écran de fin du score, au-dessus d'eux ; garde-fou au compte à rebours
class EssaiFinDessin final : public System {
public:
    const char* name() const override { return "garage : achat à la fin de l'essai (dessin)"; }
    void update(double, const GameView& g) override {
        // RG-03 : compte à rebours d'une course avec une voiture bloquée dont l'essai est fait (ou en duel) : garage
        const Course* c = g.ecran == Ecran::Course ? activeCourse() : nullptr;
        if (!c || c->state != RaceState::Count || !g.car || !voitureBloquee(sauvegarde(), g.car->id)) return;
        if (essaiPossible(sauvegarde(), g.car->id)) return;   // l'essai gratuit lui-même
        WFS_LOG("garage : la %s est bloquée et son essai gratuit est fait : pas de course, retour au garage", g.car->id);
        if (duelSession().inRoom()) { WFS_LOG("garage : salon du duel quitté (voiture bloquée)"); duelSession().leave(); }   // jamais de salle laissée ouverte
        g.allerA(Ecran::Garage);
    }
    // RG-03 : en quittant la course (écran de fin de l'essai → menu), la voiture de la partie redevient celle de la sauvegarde
    // (voiture possédée) : aucune course suivante, solo ou duel, ne part avec la voiture bloquée. Garage et terrain : pas de
    // changement (le garage montre la voiture essayée, l'essai lui-même passe par le terrain avant son départ).
    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        if (ancien != Ecran::Course || nouveau == Ecran::Pause || nouveau == Ecran::Garage || nouveau == Ecran::Terrain) return;
        const Sauvegarde& s = sauvegarde();
        if (!g.car || !g.choisirVoiture || !voitureBloquee(s, g.car->id) || essaiPossible(s, g.car->id)) return;
        if (s.car.empty() || voitureBloquee(s, s.car.c_str()) || !boutique::voitureVendue(s.car)) return;
        const PeintureDef* pd = peintureDef(s.peinture(s.car));
        const unsigned h = pd ? pd->hex : 0xFFFFFF;
        auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
        WFS_LOG("garage : fin de l'essai de la %s : voiture de la partie remise sur la %s (sauvegarde)", g.car->id, s.car.c_str());
        g.choisirVoiture(s.car.c_str(), vec3{c(16), c(8), c(0)});
    }
    void drawUi(Renderer& r, const GameView& g) override {
        g_acheterFin = Zone{};
        if (!ui::fontReady() || !finEssai(g)) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        const float gl = g.marges.gauche + 16 * k, gr = g.marges.droite + 16 * k, gt = g.marges.haut + 14 * k, gb = g.marges.bas + 14 * k;
        const long long prix = boutique::prixVoiture(g.car->id);
        // bouton « Acheter N ★ » (.btn orange, 200 × 52 comme au garage) en haut à droite, au-dessus des panneaux de fin de tous les
        // modes (leur haut est à 10 % de l'écran au moins), libellé « Essai gratuit terminé » à sa gauche
        const float bw = 200 * k, hb = 52 * k, x = W - gr - bw, y = gt;
        const std::string lib = ui::majuscules(ui::tr("fin.essai_termine"));
        g_acheterFin = {x, y, bw, hb};
        if (verre::couvert()) { g_fenetreFin.dessiner(r, W, H, k, gl, gr, gt, gb); return; }   // WFS-548 : fenêtre ouverte : rien d'autre sous le verre
        ui::text(r, x - 14 * k, y + (hb - 16 * k) / 2, 16 * k, lib, verre::hexa(verre::SUR_TITRE), ui::DROITE, 0.14f, false);   // WFS-548 (RG-03) : gris
        // WFS-549 (RG-01, RG-07) : « Acheter N ★ » en bouton Start/Stop, même rectangle (200 × 52), texte à droite de l'icône
        const ui::boutons::ZoneTexte zt = ui::boutons::startStopFond(r, x, y, bw, hb);
        const std::string t = ui::majuscules(ui::tr("garage.acheter", nombreLangue(prix)));
        float px = 19 * k;
        if (const float lw = largeurEtoiles(px, t, 0.1f); lw > zt.w) px *= zt.w / lw;   // anglais plus long : police réduite
        texteEtoiles(r, zt.x + zt.w / 2, y + (hb - px) / 2 - 1 * k, px, t, hex(TEXTE), ui::CENTRE, 0.1f);
        g_acheterFin = {x, y, bw, hb};
        manette::focus(g_acheterFin);
        g_fenetreFin.dessiner(r, W, H, k, gl, gr, gt, gb);
    }
};
} // namespace

bool finEssaiVisible() { return g_acheterFin.w > 0; }

bool finEssaiBouton(const char* nom, float& x, float& y) {
    const Zone* z = std::strcmp(nom, "acheter") == 0 ? &g_acheterFin : std::strcmp(nom, "confirmer") == 0 ? &g_fenetreFin.zoneConfirmer()
                  : std::strcmp(nom, "annuler") == 0 ? &g_fenetreFin.zoneAnnuler() : nullptr;
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2;
    return true;
}

} // namespace wfs::ecrans::garage

namespace wfs {
using EssaiFinToucherEnregistre = ecrans::garage::EssaiFinToucher;
using EssaiFinDessinEnregistre = ecrans::garage::EssaiFinDessin;
WFS_SYSTEM(EssaiFinToucherEnregistre, 205)   // gameplay (200-299) : avant la course (210), pour le bouton de l'écran de fin
WFS_SYSTEM(EssaiFinDessinEnregistre, 445)    // interface (400-499) : après le garage (440), avant le menu (450)
} // namespace wfs
