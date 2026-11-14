// WaForSpeed — écran « Market » (WFS-513) : voir market_ecran.h.
#include "market_ecran.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include "../../core/log.h"
#include "../../game/boutique.h"
#include "../../game/commandes_tactiles.h"   // dessin2d (biseau, trait)
#include "../../game/market.h"
#include "../../game/sauvegarde.h"
#include "../../platform/manette.h"          // focus de la manette ; zone retour = touche Retour d'Android
#include "../../render/renderer.h"
#include "../garage_achat.h"                 // étoile dessinée dans le texte, nombres de la langue active
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "market_ecran_calculs.h"   // calculs testés (tests/test_market_ecran.cpp)
#include "creation_profil.h"   // rien par-dessus « Crée ton pilote »
#include "reglages_appui.h"    // appui::dansCible : cibles de 48 dp
#include "textes_banc.h"       // banc « textes » : journal des textes de cet écran

namespace wfs {
namespace ecrans::market_ecran {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// design-street-racing : fond, surface, accent orange (action : acheter), cyan (information), textes
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const unsigned ACCENT_PRESSE = 0xFF9A4D;   // bouton orange sous le doigt (même teinte que le Profil et les Crédits)
// teintes des médailles (icône de chaque pack, interface 2D) : bronze, argent, or, coffre (orange de la monnaie)
const unsigned TEINTE_PACK[MARKET_PRODUITS] = {0xCD7F32, 0xC9CDDA, 0xFFC531, 0xFF6A00, 0x00E5FF, 0x00E5FF};
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.98f}, GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
constexpr int DEBUTANT = MARKET_ETOILES, SANS_PUB = MARKET_ETOILES + 1;
constexpr double DUREE_MESSAGE = 2.6;   // message affiché (s)

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// cibles : 0 … 5 = bouton d'achat du produit i (MARKET_PRODUITS_IDS) ; puis Restaurer, Retour
enum Cible { RIEN = -1, RESTAURER = MARKET_PRODUITS, RETOUR = MARKET_PRODUITS + 1 };

struct Etat {
    bool ouvert = false;
    Ecran origine = Ecran::Menu;
    double tOuvert = -1, maintenant = 0;
    Zone bouton, boutonGarage, retour, restaurer, achat[MARKET_PRODUITS];
    float dp = 1, dpBouton = 1, dpGarage = 1;
    int doigt = -1, cible = RIEN;
    // RG-03 : achat en cours (produit dont la réponse est attendue)
    std::string attente;
    double tAttente = 0;
    // RG-04 : restauration en cours, achats rendus pendant la fenêtre
    bool restauration = false;
    double tRestauration = 0;
    int restaures = 0;
    // message (« +720 ★ », attente, restauration) et animation du solde
    std::string message;
    double tMessage = -1;
    calcul::Solde solde;   // RG-03 : animation du solde
    long long soldeVu = 0;
    bool horsLigneBanc = false;
    market::Boutique* branchee = nullptr;
} g;

std::unique_ptr<market::Boutique> g_locale;   // PC et bancs (RG-08)
bool g_localeEssayee = false;

bool anglais() { return ui::langue::active() == ui::langue::Langue::Anglais; }

void montrer(const std::string& m) { g.message = m; g.tMessage = g.maintenant; WFS_LOG("market (WFS-513) : message « %s »", m.c_str()); }

// rappel de market::brancher, après le traitement de l'achat (sauvegarde déjà écrite par market::traiterAchat)
void apresAchat(const std::string& produit, const market::Traitement& t) {
    const int i = market::indexProduit(produit);
    if (produit == g.attente) g.attente.clear();   // réponse reçue : bouton de nouveau actif
    if (g.restauration && (t.credit == market::Credit::Credite || t.credit == market::Credit::DejaCredite)) g.restaures++;
    if (!g.ouvert) return;
    if (t.credit == market::Credit::Credite && i >= 0) {
        const market::Pack p = market::pack(i);
        if (i == SANS_PUB) montrer(ui::tr("market.sans_pub.active"));
        else if (!g.restauration) montrer(ui::tr("market.gain", garage::nombreLangue(p.total)));   // RG-03 : « +720 ★ »
    } else if (!t.message.empty()) {
        montrer(ui::tr("market.attente"));   // RG-03 : en attente de paiement (traiterAchat : rien de crédité)
    }
    // annulé, erreur : rien (RG-03 : retour à l'écran, aucun message d'erreur)
}

std::string prixDe(int i) {
    market::Boutique* b = boutique();
    return b ? b->prix(MARKET_PRODUITS_IDS[i]) : std::string();
}
bool possede(int i) {
    const Sauvegarde& s = sauvegarde();
    return (i == SANS_PUB && market::sansPub(s)) || (i == DEBUTANT && s.market.packDebutant);
}
calcul::Bouton etatBouton(int i) { return calcul::etatBouton(possede(i), !g.attente.empty(), g.attente == MARKET_PRODUITS_IDS[i], !prixDe(i).empty()); }
bool achetable(int i) { return etatBouton(i) == calcul::Bouton::Actif; }

int cibleSous(float x, float y) {
    if (appui::dansCible(g.retour.x, g.retour.y, g.retour.w, g.retour.h, g.dp, x, y)) return RETOUR;
    if (appui::dansCible(g.restaurer.x, g.restaurer.y, g.restaurer.w, g.restaurer.h, g.dp, x, y)) return RESTAURER;
    for (int i = 0; i < MARKET_PRODUITS; i++) if (g.achat[i].contient(x, y)) return i;
    return RIEN;
}

void acheter(int i) {
    market::Boutique* b = boutique();
    if (!b || !achetable(i)) { WFS_LOG("market (WFS-513) : %s : bouton inactif (hors ligne, possédé ou achat en cours)", MARKET_PRODUITS_IDS[i]); return; }
    g.attente = MARKET_PRODUITS_IDS[i]; g.tAttente = g.maintenant;
    WFS_LOG("market (WFS-513) : achat demandé : %s (%s)", MARKET_PRODUITS_IDS[i], b->nom());
    b->acheter(g.attente);   // réponse par apresAchat (tout de suite pour la boutique simulée, plus tard pour Google)
}

void restaurer() {
    market::Boutique* b = boutique();
    if (!b || g.restauration) return;
    g.restauration = true; g.tRestauration = g.maintenant; g.restaures = 0;
    WFS_LOG("market (WFS-513) : « Restaurer mes achats » (%s)", b->nom());
    b->restaurer();
}

// fin de la restauration (RG-04) et de l'attente d'un achat sans réponse (sécurité : le bouton revient)
void suivre() {
    if (g.restauration) {
        const calcul::Restauration f = calcul::finRestauration(g.restaures, g.maintenant - g.tRestauration);
        if (f != calcul::Restauration::EnCours) { g.restauration = false; montrer(ui::tr(f == calcul::Restauration::Restaures ? "market.restaures" : "market.rien_a_restaurer")); }
    }
    if (!g.attente.empty() && calcul::achatOublie(g.maintenant - g.tAttente)) { WFS_LOG("market (WFS-513) : %s sans réponse : bouton rendu", g.attente.c_str()); g.attente.clear(); }
}

// bouton d'achat : orange (prix de Google), pressé, ou grisé (hors ligne, achat en cours)
void boutonAchat(Renderer& r, int i, float x, float y, float w, float h, float k, float kb) {
    g.achat[i] = {x, y, w, h};
    const std::string prix = prixDe(i);
    const calcul::Bouton e = etatBouton(i);
    const bool actif = e == calcul::Bouton::Actif, presse = actif && g.doigt >= 0 && g.cible == i;
    const bool enCours = e == calcul::Bouton::Patiente;
    const float tp = 20 * k;
    if (actif) {
        dessin2d::biseau(r, x, y, w, h, 12 * kb, dessin2d::uni(hex(presse ? ACCENT_PRESSE : ACCENT)));
        const textes::NomPropre np;   // prix donné par Google (monnaie du joueur) : rien à traduire
        ui::textBoite(r, x + w / 2, y, w - 24 * kb, h, tp, prix, hex(BG), ui::CENTRE, 0.04f);
    } else {
        dessin2d::biseau(r, x, y, w, h, 12 * kb, dessin2d::uni(hex(SURFACE2)));
        const std::string t = ui::majuscules(ui::tr(enCours ? "market.patiente" : "market.acheter"));
        ui::textBoite(r, x + w / 2, y, w - 24 * kb, h, 16 * k, t, hex(MUTED), ui::CENTRE, 0.10f);
    }
    manette::focus(g.achat[i]);
}

// grande étoile de la médaille (interface 2D) : pentagone plein, puis 5 « cerfs-volants » (centre, creux, pointe, creux)
// posés dessus. Chaque bord intérieur lissé tombe sur une surface déjà pleine : aucune couture visible en grand
// (garage::etoile, faite de 6 morceaux bord à bord, laisse voir ses joints au-delà de 30 px de rayon)
void grandeEtoile(Renderer& r, float cx, float cy, float R, const vec4& c) {
    const float ri = R * 0.42f;
    vec2 ext[5], in[5];
    for (int i = 0; i < 5; i++) {
        const float a = i * 2 * PI / 5, b = a + PI / 5;   // 0 = vers le haut, sens horaire
        ext[i] = {cx + std::sin(a) * R, cy - std::cos(a) * R};
        in[i] = {cx + std::sin(b) * ri, cy - std::cos(b) * ri};
    }
    const dessin2d::Teinte t = dessin2d::uni(c);
    dessin2d::polygone(r, in, 5, t);
    for (int i = 0; i < 5; i++) { const vec2 p[4] = {{cx, cy}, in[(i + 4) % 5], ext[i], in[i]}; dessin2d::polygone(r, p, 4, t); }
}

// texte où chaque « ★ » est une grandeEtoile : mêmes mesures et même place que garage::texteEtoiles (WFS-499)
void texteEtoiles(Renderer& r, float x, float y, float px, const std::string& s, const vec4& c, int align, float esp = 0) {
    static const std::string ETOILE = "\u2605";
    constexpr float LARGEUR = 0.95f;   // garage_achat.cpp : place d'une étoile, × taille de la police
    const float w = garage::largeurEtoiles(px, s, esp);
    float cx = align == ui::CENTRE ? x - w / 2 : align == ui::DROITE ? x - w : x;
    size_t a = 0;
    for (size_t b; (b = s.find(ETOILE, a)) != std::string::npos; a = b + ETOILE.size()) {
        const std::string morceau = s.substr(a, b - a);
        if (!morceau.empty()) { ui::text(r, cx, y, px, morceau, c, ui::GAUCHE, esp, false); cx += ui::width(px, morceau, esp); }
        grandeEtoile(r, cx + px * LARGEUR / 2, y + px * 0.52f, px * 0.42f, c);
        cx += px * LARGEUR;
    }
    if (a < s.size()) ui::text(r, cx, y, px, s.substr(a), c, ui::GAUCHE, esp, false);
}

// coche « Acheté » (interface 2D), dans un carré de côté c
void coche(Renderer& r, float x, float y, float c, const vec4& col) {
    const vec2 p[3] = {{x + 0.12f * c, y + 0.52f * c}, {x + 0.40f * c, y + 0.80f * c}, {x + 0.90f * c, y + 0.22f * c}};
    dessin2d::trait(r, p, 3, std::max(2.f, 0.14f * c), col);
}

// une carte de pack (RG-02)
void carte(Renderer& r, int i, float x, float y, float w, float h, float k, float kb) {
    const market::Pack p = market::pack(i);
    const bool debutant = i == DEBUTANT;
    r.uiRect(x, y, w, h, hex(SURFACE2, 0.92f));
    if (debutant) {   // mis en avant : liseré orange tout autour
        const float e = std::max(2.f, 2 * kb);
        r.uiRect(x, y, w, e, hex(ACCENT)); r.uiRect(x, y + h - e, w, e, hex(ACCENT));
        r.uiRect(x, y, e, h, hex(ACCENT)); r.uiRect(x + w - e, y, e, h, hex(ACCENT));
    } else r.uiRect(x, y, w, 3 * k, hex(TEINTE_PACK[i]));   // liseré de la teinte de la médaille
    const float pad = 14 * k;
    // téléphone couché (environ 410 dp de haut) : carte basse ; le badge chevauche le bord du haut, la médaille rétrécit ou disparaît
    const bool compacte = h < 220 * k;
    float yy = y + (compacte ? 12 : 16) * k;
    ui::textBoite(r, x + w / 2, yy, w - 2 * pad, 18 * k, 14 * k, ui::majuscules(nomPack(i)), hex(TEXTE), ui::CENTRE, 0.10f);
    yy += (compacte ? 22 : 26) * k;
    // badge du bonus (« +20 % offerts ») ou « Une seule fois » (pack débutant)
    const std::string badge = debutant ? ui::majuscules(ui::tr("market.debutant.badge")) : ui::majuscules(texteOffert(i));
    if (!badge.empty()) {
        const float bp = (compacte ? 11 : 12) * k, bh = (compacte ? 20 : 22) * k, bw = std::min(w - 2 * pad, ui::width(bp, badge, 0.10f) + 22 * kb);
        const float by = compacte ? y - bh / 2 : yy;
        dessin2d::biseau(r, x + (w - bw) / 2, by, bw, bh, 8 * kb, dessin2d::uni(hex(debutant ? ACCENT2 : ACCENT)));
        ui::textBoite(r, x + w / 2, by, bw - 12 * kb, bh, bp, badge, hex(BG), ui::CENTRE, 0.10f);
    }
    if (!compacte) yy += 30 * k;   // place du badge gardée sans bonus : les cartes restent alignées
    // mise en page du bas vers le haut : bouton, base, total ; la médaille prend la place qui reste (rien ne se chevauche)
    const float bh = std::min(50 * kb, 46 * k), yb = y + h - pad * 0.8f - bh;
    const std::string total = garage::nombreLangue(p.total) + " \u2605";
    const float yBase = yb - 8 * k - 14 * k;
    const float tp = std::max(14 * k, std::min({34 * k, (w - 2 * pad) / std::max(1.f, garage::largeurEtoiles(1, total, 0.02f)), yBase - 4 * k - yy}));
    const float yTotal = yBase - 4 * k - tp;
    const float place = yTotal - 6 * k - yy, rm = std::min(44 * k, place / 2 / 1.32f);
    if (rm >= 10 * k) {   // médaille : étoile de la teinte du pack, sur un anneau
        const float cy = yy + place / 2;
        dessin2d::anneau(r, x + w / 2, cy, rm * 1.18f, rm * 1.30f, hex(TEINTE_PACK[i], 0.55f));
        grandeEtoile(r, x + w / 2, cy, rm, hex(TEINTE_PACK[i]));
    }
    // étoiles totales en gros (RG-02)
    texteEtoiles(r, x + w / 2, yTotal, tp, total, hex(TEXTE), ui::CENTRE, 0.02f);
    // base et bonus en petit (« 600 ★ + 120 offertes ») ; pack débutant : skin offerte
    const std::string base = debutant ? (p.bonsSkin > 0 ? ui::tr("market.debutant.skin", std::to_string(p.bonsSkin)) : std::string()) : texteBase(i);
    if (!base.empty()) {
        const float bp = std::min(13 * k, (w - 2 * pad) / std::max(1.f, garage::largeurEtoiles(1, base)));
        texteEtoiles(r, x + w / 2, yBase, bp, base, hex(MUTED), ui::CENTRE);
    }
    boutonAchat(r, i, x + pad, yb, w - 2 * pad, bh, k, kb);
}

} // namespace

// ---------------------------------------------------------------- textes
std::string nomPack(int i) {
    if (i < 0 || i >= MARKET_PRODUITS) return "";
    const market::Pack p = market::pack(i);
    return anglais() ? p.nomEn : p.nomFr;
}
std::string texteBase(int i) {
    if (i < 0 || i >= MARKET_ETOILES) return "";
    const market::Pack p = market::pack(i);
    const long long offertes = calcul::etoilesOffertes(p.etoilesBase, p.total);
    if (p.offert <= 0 || offertes <= 0) return "";
    return ui::tr("market.base", garage::nombreLangue(p.etoilesBase), garage::nombreLangue(offertes));
}
std::string texteOffert(int i) {
    if (i < 0 || i >= MARKET_ETOILES) return "";
    const market::Pack p = market::pack(i);
    return p.offert > 0 ? ui::tr("market.offert", std::to_string(p.offert)) : std::string();
}

// ---------------------------------------------------------------- boutique
market::Boutique* boutique() {
    if (g.horsLigneBanc) return nullptr;
    if (market::Boutique* b = market::boutiqueActive()) return b;   // Android : Google Play (WFS-511)
    if (!g_localeEssayee) { g_localeEssayee = true; g_locale = market::creerBoutique(); }   // PC, bancs : WFS_BOUTIQUE=simulee
    return g_locale.get();
}

// ---------------------------------------------------------------- écran
void ouvrir(Ecran origine) {
    g.ouvert = true; g.origine = origine; g.tOuvert = -1; g.doigt = -1; g.cible = RIEN;
    g.message.clear(); g.tMessage = -1; g.solde = {}; g.restauration = false;
    market::Boutique* b = boutique();
    if (b) {
        // le résultat des achats revient à apresAchat ; même traitement qu'avant (market::brancher : crédit écrit, puis confirmation)
        if (b != g.branchee) { market::brancher(*b, sauvegarde(), apresAchat); g.branchee = b; }
        b->chargerProduits();   // prix à jour (connexion revenue) et achats pas encore confirmés rattrapés
    }
    WFS_LOG("market (WFS-513) : écran ouvert depuis %s, boutique %s, solde %lld ★", nomEcran(origine), b ? b->nom() : "aucune (hors ligne)",
            boutique::solde(sauvegarde()));
}
void fermer() {
    if (g.ouvert) WFS_LOG("market (WFS-513) : écran fermé, retour à l'écran %s", nomEcran(g.origine));
    g.ouvert = false; g.doigt = -1; g.cible = RIEN; g.restauration = false;
    for (Zone& z : g.achat) z = Zone{};
    g.retour = g.restaurer = Zone{};
}
bool ouvert() { return g.ouvert; }

void dessiner(Renderer& r, float W, float H, const Marges& m, double temps) {
    g.maintenant = temps;
    if (!g.ouvert || !ui::fontReady()) return;
    const textes::Journal journal;   // banc « textes » : textes de cet écran
    if (g.tOuvert < 0) g.tOuvert = temps;
    suivre();
    const float op = (float)std::clamp((temps - g.tOuvert) / 0.25, 0.0, 1.0);   // fondu d'entrée de 250 ms
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    g.dp = ui::pixelsParDp(W);
    const Sauvegarde& s = sauvegarde();
    r.uiRect(0, 0, W, H, hex(BG, 0.85f + 0.15f * op));
    // panneau (comme le Profil et les Crédits) : surface, liseré cyan en haut
    const float zl = m.gauche + 16 * k, zr = W - m.droite - 16 * k;
    const float pw = std::min(zr - zl, 1100 * k), px = zl + (zr - zl - pw) / 2;
    const float py = m.haut + (cp ? 8 : 20) * k, ph = H - py - m.bas - (cp ? 8 : 20) * k;
    r.uiRect(px, py, pw, ph, SURFACE);
    r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));
    const float x0 = px + 24 * k, x1 = px + pw - 24 * k;
    // ---- en-tête : sur-titre, titre, solde à droite (RG-02)
    float y = py + 14 * k;
    ui::text(r, x0, y, 12 * k, ui::majuscules(ui::tr("market.surtitre")), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
    r.uiRect(x0, y + 22 * k, 4 * k, 26 * k, hex(ACCENT));   // barre néon du titre
    ui::text(r, x0 + 14 * k, y + 18 * k, 30 * k, ui::majuscules(ui::tr("market.titre")), hex(TEXTE), ui::GAUCHE, 0.04f, true);
    g.soldeVu = g.solde.suivre(boutique::solde(s), temps);   // RG-03 : solde qui monte quand des étoiles arrivent
    ui::text(r, x1, y, 12 * k, ui::majuscules(ui::tr("market.solde")), hex(MUTED), ui::DROITE, 0.16f, false);
    texteEtoiles(r, x1, y + 18 * k, 30 * k, garage::nombreLangue(g.soldeVu) + " ★", hex(ACCENT), ui::DROITE, 0.02f);
    y += 64 * k;
    r.uiRect(x0, y, x1 - x0, std::max(1.f, kb), hex(TEXTE, 0.08f));   // séparation
    y += 14 * k;
    // ---- bas : boutons (Restaurer, Retour), mentions (RG-06), « Sans pub » (RG-02)
    const float bh = 50 * kb, by = py + ph - 14 * k - bh;
    const float tp = (cp ? 17 : 19) * k;
    auto largeur = [&](const char* cle) {   // largeur fixe, mesurée sur la plus longue des deux langues (RC-E20-04)
        const float fr = ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Francais)), 0.10f);
        const float en = ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Anglais)), 0.10f);
        return std::max(fr, en) + 48 * kb;
    };
    const float wRet = largeur("market.retour"), wRes = largeur("market.restaurer");
    g.retour = {x1 - wRet, by, wRet, bh};
    g.restaurer = {x1 - wRet - 14 * k - wRes, by, wRes, bh};
    const bool pRet = g.doigt >= 0 && g.cible == RETOUR, pRes = g.doigt >= 0 && g.cible == RESTAURER;
    const bool resActif = boutique() && !g.restauration;
    for (int b = 0; b < 2; b++) {
        const Zone& z = b == 0 ? g.restaurer : g.retour;
        const bool presse = b == 0 ? pRes : pRet;
        dessin2d::biseau(r, z.x - kb, by - kb, z.w + 2 * kb, bh + 2 * kb, 14 * kb, dessin2d::uni(BORD));
        dessin2d::biseau(r, z.x, by, z.w, bh, 14 * kb, dessin2d::uni(GLASS));
        if (presse) dessin2d::biseau(r, z.x, by, z.w, bh, 14 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));
        const vec4 c = b == 1 ? hex(ACCENT2) : hex(resActif ? TEXTE : MUTED);
        ui::textBoite(r, z.x + z.w / 2, by, z.w - 32 * kb, bh, tp, ui::majuscules(ui::tr(b == 0 ? "market.restaurer" : "market.retour")), c, ui::CENTRE, 0.10f);
    }
    // RG-05 : hors ligne (aucune boutique ou aucun prix de Google) : une ligne à gauche des boutons
    bool unPrix = false;
    for (int i = 0; i < MARKET_PRODUITS; i++) unPrix = unPrix || !prixDe(i).empty();
    if (!unPrix) {
        const float lx = x0, lw = std::max(0.f, g.restaurer.x - 18 * k - lx);
        r.uiRect(lx, by + bh / 2 - 4 * k, 8 * k, 8 * k, hex(ACCENT));   // pastille d'alerte (interface 2D)
        ui::textBoite(r, lx + 16 * k, by, std::max(0.f, lw - 16 * k), bh, 14 * k, ui::tr("market.hors_ligne"), hex(TEXTE), ui::GAUCHE, 0);
    }
    const float yMent = by - 10 * k - 16 * k;
    ui::textBoite(r, x0, yMent, x1 - x0, 16 * k, 12 * k, ui::tr("market.mentions"), hex(MUTED), ui::GAUCHE, 0);
    const float sh = 60 * k, ys = yMent - 12 * k - sh;
    {   // « Sans pub » (RG-02) : à part, « Acheté » une fois possédé
        r.uiRect(x0, ys, x1 - x0, sh, hex(SURFACE2, 0.92f));
        r.uiRect(x0, ys, 4 * k, sh, hex(ACCENT2));
        const float bw = 210 * k, bx = x1 - 14 * k - bw, bys = ys + (sh - 46 * kb) / 2;
        ui::textBoite(r, x0 + 20 * k, ys + 8 * k, bx - 20 * k - (x0 + 20 * k), 22 * k, 18 * k, ui::majuscules(nomPack(SANS_PUB)), hex(TEXTE), ui::GAUCHE, 0.08f);
        ui::textBoite(r, x0 + 20 * k, ys + 32 * k, bx - 20 * k - (x0 + 20 * k), 18 * k, 13 * k, ui::tr("market.sans_pub.desc"), hex(MUTED), ui::GAUCHE, 0);
        if (possede(SANS_PUB)) {
            g.achat[SANS_PUB] = Zone{};
            const std::string t = ui::majuscules(ui::tr("market.achete"));
            const float c = 20 * k, tw = ui::width(16 * k, t, 0.10f), cx = bx + (bw - tw - c - 8 * k) / 2;
            coche(r, cx, ys + (sh - c) / 2, c, hex(ACCENT2));
            ui::text(r, cx + c + 8 * k, ys + (sh - 16 * k) / 2, 16 * k, t, hex(ACCENT2), ui::GAUCHE, 0.10f, false);
        } else boutonAchat(r, SANS_PUB, bx, bys, bw, 46 * kb, k, kb);
    }
    // ---- cartes des packs (RG-02) : pack débutant en tête quand il est proposé
    int ordre[MARKET_ETOILES + 1];
    const bool debutant = market::packDebutantPropose(s);
    const int n = calcul::ordreCartes(debutant, MARKET_ETOILES, ordre);
    if (!debutant) g.achat[DEBUTANT] = Zone{};
    const float ecart = 14 * k, ch = ys - 14 * k - y, cw = (x1 - x0 - (n - 1) * ecart) / n;
    for (int j = 0; j < n; j++) carte(r, ordre[j], x0 + j * (cw + ecart), y, cw, ch, k, kb);
    // ---- message (« +720 ★ », attente, restauration) : bandeau au centre de l'en-tête, 2,6 s, fondu de sortie
    if (!g.message.empty() && g.tMessage >= 0) {
        const double age = temps - g.tMessage;
        if (age >= DUREE_MESSAGE) g.message.clear();
        else {
            const float a = (float)std::clamp((DUREE_MESSAGE - age) / 0.4, 0.0, 1.0) * (float)std::clamp(age / 0.12, 0.0, 1.0);
            const float mp = 24 * k, mw = std::min(pw * 0.42f, garage::largeurEtoiles(mp, g.message, 0.04f) + 60 * kb), mh = 46 * kb;
            const float mx = px + (pw - mw) / 2, my = py + 18 * k;
            dessin2d::biseau(r, mx, my, mw, mh, 14 * kb, dessin2d::uni(hex(ACCENT, 0.96f * a)));
            const float tpm = std::min(mp, (mw - 30 * kb) / std::max(1.f, garage::largeurEtoiles(1, g.message, 0.04f)));
            texteEtoiles(r, mx + mw / 2, my + (mh - tpm) / 2, tpm, g.message, hex(BG, a), ui::CENTRE, 0.04f);
        }
    }
    // manette : seules les zones de cet écran ; « Retour » est la zone retour (touche Retour d'Android, RG-01)
    manette::zonesImage().clear();
    for (int j = 0; j < n; j++) manette::focus(g.achat[ordre[j]]);
    manette::focus(g.achat[SANS_PUB]);
    manette::focus(g.restaurer);
    manette::focus(g.retour, true);
}

bool toucher(int id, float x, float y, bool down) {
    if (!g.ouvert) return false;
    const int sous = cibleSous(x, y);
    if (down) {
        if (g.doigt < 0) { g.doigt = id; g.cible = sous; }
        else if (id == g.doigt && sous != g.cible) g.cible = RIEN;   // glissé hors du bouton : annulé
        return true;
    }
    if (id != g.doigt) return true;   // lever d'un autre doigt (ou du doigt qui a ouvert l'écran)
    const int c = g.cible;
    g.doigt = -1; g.cible = RIEN;
    if (c == RIEN || c != sous) return true;
    if (c == RETOUR) fermer();
    else if (c == RESTAURER) restaurer();
    else acheter(c);
    return true;
}

// RG-01 : même bouton que « Profil », « Cadeau du jour » et « Crédits » (vitré, biseau, 44 px), au-dessus de « Profil »,
// avec l'étoile orange de la monnaie devant le libellé
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse) {
    if (!ui::fontReady()) return;
    const textes::Journal journal;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    const float h = 44 * kb, tp = 15 * k, pad = 18 * kb, re = tp * 0.48f, gap = 8 * kb;
    const std::string t = ui::majuscules(ui::tr("market.bouton"));
    const float tw = ui::width(tp, t, 0.12f);
    // « Crédits » : H − bas − 14 − h ; puis « Cadeau du jour », « Profil » : 10 au-dessus chacun ; « Market » : encore 10 au-dessus
    const float w = pad + 2 * re + gap + tw + pad, x = W - m.droite - 16 * k - w, y = H - m.bas - 14 * k - 4 * h - 30 * k;
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    dessin2d::biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, dessin2d::uni(BORD));
    dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(GLASS));
    if (presse) dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));   // appuyé : voile cyan
    garage::etoile(r, x + pad + re, y + h / 2, re, hex(ACCENT));
    ui::textBoite(r, x + pad + 2 * re + gap + tw / 2, y, tw, h, tp, t, hex(ACCENT2), ui::CENTRE, 0.12f, false);
    manette::focus(g.bouton);
}
bool dansBouton(float x, float y) { return g.bouton.w > 0 && appui::dansCible(g.bouton.x, g.bouton.y, g.bouton.w, g.bouton.h, g.dpBouton, x, y); }

// RG-01 : garage : même bouton, sous le solde d'étoiles écrit en haut à droite (WFS-499)
void dessinerBoutonGarage(Renderer& r, float W, float H, const Marges& m, float op) {
    if (!ui::fontReady() || g.ouvert) { g.boutonGarage = Zone{}; return; }
    const textes::Journal journal;
    const float kb = H / 720.f, k = kb;   // unité du garage (H / 720)
    const float h = 40 * kb, tp = 14 * k, pad = 16 * kb, re = tp * 0.48f, gap = 8 * kb;
    const std::string t = ui::majuscules(ui::tr("market.bouton"));
    const float tw = ui::width(tp, t, 0.12f);
    const float w = pad + 2 * re + gap + tw + pad, x = W - m.droite - 16 * k - w, y = m.haut + 14 * k + 6 * k + 14 * k + 12 * k;
    g.boutonGarage = {x, y, w, h};
    g.dpGarage = ui::pixelsParDp(W);
    auto a = [op](vec4 c) { c.w *= op; return c; };
    dessin2d::biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, dessin2d::uni(a(BORD)));
    dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(a(GLASS)));
    garage::etoile(r, x + pad + re, y + h / 2, re, a(hex(ACCENT)));
    ui::textBoite(r, x + pad + 2 * re + gap + tw / 2, y, tw, h, tp, t, a(hex(ACCENT2)), ui::CENTRE, 0.12f, false);
    manette::focus(g.boutonGarage);
}
bool dansBoutonGarage(float x, float y) {
    return !g.ouvert && g.boutonGarage.w > 0 && appui::dansCible(g.boutonGarage.x, g.boutonGarage.y, g.boutonGarage.w, g.boutonGarage.h, g.dpGarage, x, y);
}

// ---------------------------------------------------------------- banc
void horsLigneBanc(bool oui) { g.horsLigneBanc = oui; }
void nouvelleBoutiqueBanc() { g_locale.reset(); g_localeEssayee = false; g.branchee = nullptr; }
bool centre(const char* nom, float& x, float& y) {
    const std::string n = nom;
    const Zone* z = n == "bouton" ? &g.bouton : n == "garage" ? &g.boutonGarage : n == "retour" ? &g.retour
                  : n == "restaurer" ? &g.restaurer : n == "sans_pub" ? &g.achat[SANS_PUB] : nullptr;
    if (!z && n.rfind("pack-", 0) == 0) { const int i = std::atoi(n.c_str() + 5); if (i >= 0 && i < MARKET_PRODUITS) z = &g.achat[i]; }
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2; return true;
}
const std::string& message() { return g.message; }
long long soldeAffiche() { return g.soldeVu; }

namespace {
// système : panneau par-dessus le menu (450), les Réglages (460), le compteur (461), le Profil (468) et le garage (440) ;
// touchers reçus par le menu et le garage (accroches). Rien par-dessus « Crée ton pilote ».
class MarketEcran : public System {
public:
    const char* name() const override { return "market-ecran"; }
    void onEcran(Ecran, Ecran, const GameView&) override { fermer(); }
    void drawUi(Renderer& r, const GameView& v) override {
        g.maintenant = v.time;
        if ((v.ecran == Ecran::Menu || v.ecran == Ecran::Garage) && !creationProfilOuverte()) dessiner(r, v.uiWidth, v.uiHeight, v.marges, v.time);
    }
    void shutdown(Renderer&) override { fermer(); }
};
} // namespace

} // namespace ecrans::market_ecran
} // namespace wfs

WFS_SYSTEM(wfs::ecrans::market_ecran::MarketEcran, 469)   // interface : par-dessus le Profil (468), sous les Crédits (470)
