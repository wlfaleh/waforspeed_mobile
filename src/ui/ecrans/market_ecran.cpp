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
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../verre.h"   // WFS-548 (R19) : fenêtre et tuiles de verre, sur-titre gris, fond flouté
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
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
constexpr int DEBUTANT = MARKET_ETOILES, SANS_PUB = MARKET_ETOILES + 1;
// WFS-557 (maquette 12) : noms courts des 4 packs d'étoiles sur leur carte (« Bronze », « Argent », « Or », « Coffre »)
const char* const NOMS_COURTS[MARKET_ETOILES] = {"market.court.0", "market.court.1", "market.court.2", "market.court.3"};
// WFS-557 (RG-18) : onglets « Étoiles / Offres / Sans pub » : tout, le pack débutant seul, « Sans pub » seul
enum { O_ETOILES = 0, O_OFFRES = 1, O_SANS_PUB = 2, N_ONGLETS_M = 3 };
const char* const ONGLETS_M[N_ONGLETS_M] = {"market.onglet.etoiles", "market.onglet.offres", "market.onglet.sans_pub"};
constexpr double DUREE_MESSAGE = 2.6;   // message affiché (s)

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// cibles : 0 … 5 = bouton d'achat du produit i (MARKET_PRODUITS_IDS) ; puis Restaurer, Retour
enum Cible { RIEN = -1, RESTAURER = MARKET_PRODUITS, RETOUR = MARKET_PRODUITS + 1, ONGLET0 = MARKET_PRODUITS + 2 };

struct Etat {
    bool ouvert = false;
    Ecran origine = Ecran::Menu;
    double tOuvert = -1, maintenant = 0;
    Zone bouton, boutonGarage, retour, restaurer, achat[MARKET_PRODUITS], onglets[3];
    int onglet = 0;   // WFS-557 (RG-18) : onglet ouvert (Étoiles à l'ouverture)
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
    for (int o = 0; o < 3; o++) if (g.onglets[o].contient(x, y)) return ONGLET0 + o;
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
void boutonAchat(Renderer& r, int i, float x, float y, float w, float h, float k, float) {   // dernier argument (kb) : plus utilisé depuis WFS-549
    g.achat[i] = {x, y, w, h};
    const std::string prix = prixDe(i);
    const calcul::Bouton e = etatBouton(i);
    const bool actif = e == calcul::Bouton::Actif, presse = actif && g.doigt >= 0 && g.cible == i;
    const bool enCours = e == calcul::Bouton::Patiente;
    const float tp = std::min(26 * k, h * 0.50f);   // WFS-557 (maquette 12) : prix en grand
    // WFS-549 (RG-04, RG-07, RG-08) : bouton de prix orange, texte sombre, même rectangle ; pressé : plus clair ; inactif : grisé
    if (actif) {
        const textes::NomPropre np;   // prix donné par Google (monnaie du joueur) : rien à traduire
        ui::boutons::prix(r, x, y, w, h, prix, 1, presse, tp);
    } else {
        const std::string t = ui::majuscules(ui::tr(enCours ? "market.patiente" : "market.acheter"));
        ui::boutons::prix(r, x, y, w, h, t, 1, false, 16 * k, true);
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

// WFS-557 (maquette 12) : amas d'étoiles orange de la carte d'un pack (2, 4, 6 puis 9 étoiles), centre (cx, cy), taille u
void amasEtoiles(Renderer& r, int i, float cx, float cy, float u) {
    static const float P[4][9][3] = {   // x, y (en u, depuis le centre), rayon
        {{0.25f, -0.35f, 0.42f}, {-0.45f, 0.30f, 0.30f}},
        {{-0.25f, -0.55f, 0.36f}, {0.05f, -0.10f, 0.32f}, {0.35f, 0.30f, 0.28f}, {0.60f, 0.60f, 0.18f}},
        {{-0.05f, -0.65f, 0.36f}, {0.25f, -0.35f, 0.30f}, {-0.30f, -0.05f, 0.34f}, {0.15f, 0.05f, 0.28f}, {0.45f, -0.05f, 0.26f}, {-0.20f, 0.45f, 0.24f}},
        {{-0.05f, -0.70f, 0.30f}, {0.30f, -0.50f, 0.28f}, {-0.35f, -0.25f, 0.30f}, {0.05f, -0.20f, 0.26f}, {0.50f, -0.10f, 0.30f},
         {-0.55f, 0.20f, 0.26f}, {-0.15f, 0.25f, 0.28f}, {0.25f, 0.25f, 0.26f}, {0.55f, 0.45f, 0.22f}}};
    static const int N[4] = {2, 4, 6, 9};
    const int n = N[std::clamp(i, 0, 3)];
    // lueur orange douce sous l'amas (un seul disque dégradé), puis chaque étoile : ombre foncée décalée (volume) et étoile claire.
    // Peu de polygones : l'interface a 8 192 quadrilatères par image (renderer.cpp, UI_MAX_VERTS) et tout ce qui dépasse est perdu.
    dessin2d::couronne(r, cx, cy, 0, u * 1.1f, hex(ACCENT, 0.16f), hex(ACCENT, 0));
    for (int j = 0; j < n; j++) {
        const float x = cx + P[i][j][0] * u, y = cy + P[i][j][1] * u, rr = P[i][j][2] * u;
        grandeEtoile(r, x + rr * 0.06f, y + rr * 0.08f, rr, hex(0xC24A00));
        grandeEtoile(r, x, y, rr * 0.92f, hex(0xFF8A1E));
    }
}

// rectangle arrondi (polygone, contour, lueur) aux couleurs de la maquette
void rond(Renderer& r, float x, float y, float w, float h, float R, const vec4& haut, const vec4& bas, float ep = 0, const vec4& bord = {}, float lueur = 0) {
    vec2 q[ui::boutons::MAX_SOMMETS];
    const int n = ui::boutons::contourArrondi(q, x, y, w, h, R);
    if (lueur > 0) dessin2d::halo(r, q, n, lueur, {bord.x, bord.y, bord.z, 0.30f * bord.w});
    dessin2d::polygone(r, q, n, {haut, bas, y, y + h});
    if (ep > 0) dessin2d::contour(r, q, n, ep, bord);
}

// une carte de pack d'étoiles (maquette 12 : 235 × 330) : nom, amas d'étoiles, « ★ 720 », « dont +20 % offertes », prix ;
// Or et Coffre bordés d'orange avec « Populaire » / « Meilleure valeur » posé sur le bord du haut
void carte(Renderer& r, int i, float x, float y, float w, float h, float k) {
    const market::Pack p = market::pack(i);
    const bool vedette = i >= 2;
    rond(r, x, y, w, h, 18 * k, hex(0x22252D, 0.78f), hex(0x121419, 0.86f), vedette ? 2 * k : 1 * k, vedette ? hex(ACCENT) : hex(TEXTE, 0.14f), vedette ? 10 * k : 0);
    { ui::AvecPolice inter(ui::Police::InterMedium);
      ui::textBoite(r, x + w / 2, y + 22 * k, w - 24 * k, 16 * k, 13 * k, ui::majuscules(ui::tr(NOMS_COURTS[i])), hex(0xC9CDDA), ui::CENTRE, 0.24f); }
    if (vedette) {   // badge sur le bord du haut
        ui::AvecPolice inter(ui::Police::InterMedium);
        const std::string b = ui::majuscules(ui::tr(i == 2 ? "market.populaire" : "market.meilleure"));
        const float bp = 12 * k, bw = ui::width(bp, b, 0.12f) + 22 * k, bh = 22 * k;
        rond(r, x + (w - bw) / 2, y - bh / 2, bw, bh, bh / 2, hex(0xFF7A1A), hex(ACCENT));
        ui::textBoite(r, x + w / 2, y - bh / 2, bw - 10 * k, bh, bp, b, hex(BG), ui::CENTRE, 0.12f);
    }
    amasEtoiles(r, i, x + w / 2, y + 125 * k, 64 * k);
    // « ★ 720 » : étoile orange, nombre blanc en italique
    const std::string nb = garage::nombreLangue(p.total);
    const float tp = 54 * k, tw = ui::width(tp, nb, 0.0f), et = tp * 0.8f, tot = et + 10 * k + tw, x0 = x + (w - tot) / 2, yt = y + 176 * k;
    grandeEtoile(r, x0 + et / 2, yt + tp * 0.52f, et * 0.5f, hex(ACCENT));
    ui::text(r, x0 + et + 10 * k, yt, tp, nb, hex(TEXTE), ui::GAUCHE, 0, false, true);
    const long long off = calcul::etoilesOffertes(p.etoilesBase, p.total);
    if (p.offert > 0 && off > 0) { ui::AvecPolice inter(ui::Police::InterMedium);
        ui::textBoite(r, x + w / 2, y + 228 * k, w - 20 * k, 18 * k, 14 * k, ui::tr("market.dont", std::to_string(p.offert)), hex(0x5FD6E8), ui::CENTRE, 0); }
    boutonAchat(r, i, x + 17 * k, y + h - 63 * k, w - 34 * k, 45 * k, k, k);
}

// carte « Pack débutant » (maquette 12 : 468 × 469) : le parking flouté se voit par le haut de la carte, badge cyan, « ★ 500 + 1 skin
// au choix », description, grand bouton de prix ; RG-18 : acheté → « Acheté » ; pas encore proposé → grisée « Après ta 3e course »
void carteDebutant(Renderer& r, float x, float y, float w, float h, float k, bool propose) {
    const market::Pack p = market::pack(DEBUTANT);
    const bool achete = possede(DEBUTANT);
    rond(r, x, y, w, h, 22 * k, hex(0x15171C, 0.20f), hex(0x0E1014, 0.92f), 1 * k, hex(TEXTE, 0.14f));
    rond(r, x, y + h * 0.45f, w, h * 0.55f, 22 * k, hex(0x0E1014, 0.0f), hex(0x0E1014, 0.80f));   // fondu vers le texte
    {   // badge « Pack débutant »
        ui::AvecPolice inter(ui::Police::InterMedium);
        const std::string b = ui::majuscules(ui::tr("market.debutant.titre"));
        const float bp = 13 * k, bw = ui::width(bp, b, 0.14f) + 26 * k, bh = 26 * k;
        rond(r, x + 21 * k, y + 19 * k, bw, bh, bh / 2, hex(0x34E8FF), hex(ACCENT2));
        ui::textBoite(r, x + 21 * k + bw / 2, y + 19 * k, bw - 10 * k, bh, bp, b, hex(BG), ui::CENTRE, 0.14f);
    }
    const float a = propose || achete ? 1.f : 0.45f;   // pas encore proposé : grisée
    const float yt = y + h - 196 * k, et = 46 * k;
    grandeEtoile(r, x + 33 * k + et / 2, yt + 32 * k, et * 0.5f, hex(ACCENT, a));
    const std::string nb = garage::nombreLangue(p.total);
    ui::text(r, x + 42 * k + et + 14 * k, yt, 60 * k, nb, hex(TEXTE, a), ui::GAUCHE, 0, false, true);
    const float xn = x + 42 * k + et + 14 * k + ui::width(60 * k, nb, 0) + 12 * k;
    if (p.bonsSkin > 0) ui::textBoite(r, xn, yt + 18 * k, x + w - 20 * k - xn, 38 * k, 36 * k, ui::tr("market.debutant.skin_choix", std::to_string(p.bonsSkin)), hex(TEXTE, a), ui::GAUCHE, 0, false, true);
    {   // description (2 lignes au plus)
        ui::AvecPolice inter(ui::Police::Inter);
        const std::string d = ui::tr("market.debutant.desc");
        const float dp = 15 * k, lw = w - 64 * k;
        std::string l1 = d, l2;
        if (ui::width(dp, d, 0) > lw) {
            size_t coupe = std::string::npos;
            for (size_t c = d.find(' '); c != std::string::npos; c = d.find(' ', c + 1)) if (ui::width(dp, d.substr(0, c), 0) <= lw) coupe = c; else break;
            if (coupe != std::string::npos) { l1 = d.substr(0, coupe); l2 = d.substr(coupe + 1); }
        }
        ui::text(r, x + 32 * k, y + h - 133 * k, dp, l1, hex(0xE6E8EE, a), ui::GAUCHE, 0, false);
        if (!l2.empty()) ui::textBoite(r, x + 32 * k, y + h - 113 * k, lw, dp, dp, l2, hex(0xE6E8EE, a), ui::GAUCHE, 0);
    }
    const float bx = x + 32 * k, bw = w - 64 * k, by = y + h - 78 * k, bh = 54 * k;
    if (achete) {   // « Acheté », comme aujourd'hui
        g.achat[DEBUTANT] = Zone{};
        const std::string t = ui::majuscules(ui::tr("market.achete"));
        const float c = 22 * k, tw = ui::width(18 * k, t, 0.10f), cx = bx + (bw - tw - c - 8 * k) / 2;
        coche(r, cx, by + (bh - c) / 2, c, hex(ACCENT2));
        ui::text(r, cx + c + 8 * k, by + (bh - 18 * k) / 2, 18 * k, t, hex(ACCENT2), ui::GAUCHE, 0.10f, false);
    } else if (!propose) {   // RG-18 : grisée, sans achat possible
        g.achat[DEBUTANT] = Zone{};
        ui::boutons::prix(r, bx, by, bw, bh, ui::tr("market.debutant.apres", std::to_string(config().market.debutantApres)), 1, false, 20 * k, true);
    } else boutonAchat(r, DEBUTANT, bx, by, bw, bh, k, k);
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
    g.ouvert = true; g.origine = origine; g.tOuvert = -1; g.doigt = -1; g.cible = RIEN; g.onglet = 0;
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
    for (Zone& z : g.onglets) z = Zone{};
}
bool ouvert() { return g.ouvert; }

// WFS-557 (maquette 12 ; arbitrages du PO du 2026-11-16 : maquette à 100 %, RG-18). Unité k = H / 720 (maquette 1600 × 720).
// Pas de grande fenêtre : « Boutique » / « MARKET » en haut à gauche, onglets en sélecteur de verre, solde en pilule et ✕ en
// haut à droite (✕ = ancien « Retour », même action, touche Retour d'Android comprise) ; carte « Pack débutant » à gauche,
// 4 cartes d'étoiles, ligne « Sans pub » ; liens « Restaurer mes achats · Paiement sécurisé par Google Play · Prix TTC » en bas.
void dessiner(Renderer& r, float W, float H, const Marges& m, double temps) {
    g.maintenant = temps;
    if (!g.ouvert || !ui::fontReady()) return;
    const textes::Journal journal;   // banc « textes » : textes de cet écran
    if (g.tOuvert < 0) g.tOuvert = temps;
    suivre();
    const float k = std::min(H / 720.f, (W - m.gauche - m.droite) / 1600.f);   // 16:9 : réduit pour tenir en largeur, centré
    g.dp = ui::pixelsParDp(W);
    const Sauvegarde& s = sauvegarde();
    verre::fenetreSurEcran(r);   // menu ou garage dessous : leur interface n'est plus dessinée (verre::couvert)
    verre::assombrir(r, W, H, 0.55f);
    const float ox = m.gauche + (W - m.gauche - m.droite - 1600 * k) / 2, oy = m.haut + (H - m.haut - m.bas - 720 * k) / 2, xr = ox + 1600 * k;
    // ---- en-tête
    verre::surTitre(r, ox + 44 * k, oy + 33 * k, 16 * k, ui::tr("market.surtitre"));
    ui::text(r, ox + 46 * k, oy + 45 * k, 52 * k, ui::majuscules(ui::tr("market.titre")), hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
    {   // onglets
        const float in = 4 * k, tp = 14 * k, pad = 24 * k, h = 42 * k, x0 = ox + 300 * k, y0 = oy + 41 * k;
        float ow[3], tot = 2 * in;
        { ui::AvecPolice inter(ui::Police::InterMedium);
          for (int o = 0; o < 3; o++) { ow[o] = ui::width(tp, ui::majuscules(ui::tr(ONGLETS_M[o])), 0.12f) + 2 * pad + (o == 0 ? tp * 1.2f : 0); tot += ow[o]; } }
        ui::boutons::selecteurFond(r, x0, y0, tot, h);
        float xo = x0 + in;
        for (int o = 0; o < 3; o++) {
            const bool on = g.onglet == o;
            g.onglets[o] = {xo, y0, ow[o], h};
            ui::boutons::selecteurChoix(r, xo, y0 + in, ow[o], h - 2 * in, "", on);
            const vec4 c = on ? hex(BG) : hex(0xD7DAE2);
            float xt = xo + pad;
            if (o == 0) { grandeEtoile(r, xt + tp * 0.45f, y0 + h / 2, tp * 0.45f, c); xt += tp * 1.2f; }
            ui::AvecPolice inter(ui::Police::InterMedium);
            ui::text(r, xt, y0 + (h - tp) / 2, tp, ui::majuscules(ui::tr(ONGLETS_M[o])), c, ui::GAUCHE, 0.12f, false);
            xo += ow[o];
        }
    }
    // ---- solde (pilule de verre) et ✕
    g.soldeVu = g.solde.suivre(boutique::solde(s), temps);   // RG-03 : solde qui monte quand des étoiles arrivent
    {
        const float h = 50 * k, xf = xr - 45 * k - h, y0 = oy + 38 * k;
        g.retour = {xf, y0, h, h};
        ui::boutons::verreIcone(r, xf, y0, h, h, ui::boutons::Icone::Croix, "", 1, g.doigt >= 0 && g.cible == RETOUR, 20 * k, {12 * k});
        const std::string nb = garage::nombreLangue(g.soldeVu);
        const float tp = 26 * k, w = ui::width(tp, nb, 0) + tp + 52 * k, xs = xf - 27 * k - w;
        rond(r, xs, y0, w, h, 14 * k, hex(0x262933, 0.85f), hex(0x15171D, 0.90f), 1 * k, hex(TEXTE, 0.16f));
        grandeEtoile(r, xs + 24 * k + tp * 0.45f, y0 + h / 2, tp * 0.48f, hex(ACCENT));
        ui::text(r, xs + 24 * k + tp + 8 * k, y0 + (h - tp) / 2, tp, nb, hex(TEXTE), ui::GAUCHE, 0, false, true);
    }
    // ---- contenu de l'onglet
    const bool propose = market::packDebutantPropose(s);
    for (int i = 0; i < MARKET_PRODUITS; i++) g.achat[i] = Zone{};
    if (g.onglet != O_SANS_PUB) carteDebutant(r, ox + 45 * k, oy + 121 * k, 468 * k, 469 * k, k, propose);
    if (g.onglet == O_ETOILES) for (int i = 0; i < MARKET_ETOILES; i++) carte(r, i, ox + (546 + 254 * i) * k, oy + 132 * k, 235 * k, 330 * k, k);
    if (g.onglet != O_OFFRES) {   // « Sans pub » (maquette 12 : 779 × 101)
        const float x0 = ox + (g.onglet == O_SANS_PUB ? 45 : 546) * k, y0 = oy + (g.onglet == O_SANS_PUB ? 121 : 488) * k, w = 779 * k, h = 101 * k;
        rond(r, x0, y0, w, h, 18 * k, hex(0x22252D, 0.78f), hex(0x121419, 0.86f), 1 * k, hex(TEXTE, 0.14f));
        rond(r, x0 + 22 * k, y0 + 22 * k, 57 * k, 57 * k, 12 * k, hex(0x1D4A52, 0.9f), hex(0x123238, 0.9f), 1 * k, hex(ACCENT2, 0.35f));
        {   // icône : écran barré
            const float cx = x0 + 50 * k, cy = y0 + 50 * k;
            ui::boutons::icone(r, ui::boutons::Icone::Ecran, cx, cy, 26 * k, hex(0x5FD6E8));
            const vec2 barre[2] = {{cx - 13 * k, cy - 13 * k}, {cx + 13 * k, cy + 11 * k}};
            dessin2d::trait(r, barre, 2, 2.2f * k, hex(0x5FD6E8));
        }
        const float tx = x0 + 98 * k, bw = 149 * k, bx = x0 + w - 22 * k - bw;
        ui::textBoite(r, tx, y0 + 18 * k, bx - 20 * k - tx, 28 * k, 26 * k, ui::majuscules(nomPack(SANS_PUB)), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        {
            ui::AvecPolice inter(ui::Police::Inter);
            const std::string d = ui::tr("market.sans_pub.desc");
            const float dp = 14 * k, lw = bx - 24 * k - tx;
            std::string l1 = d, l2;
            if (ui::width(dp, d, 0) > lw) {
                size_t coupe = std::string::npos;
                for (size_t c = d.find(' '); c != std::string::npos; c = d.find(' ', c + 1)) if (ui::width(dp, d.substr(0, c), 0) <= lw) coupe = c; else break;
                if (coupe != std::string::npos) { l1 = d.substr(0, coupe); l2 = d.substr(coupe + 1); }
            }
            ui::text(r, tx, y0 + 52 * k, dp, l1, hex(0xE6E8EE), ui::GAUCHE, 0, false);
            if (!l2.empty()) ui::textBoite(r, tx, y0 + 70 * k, lw, dp, dp, l2, hex(0xE6E8EE), ui::GAUCHE, 0);
        }
        if (possede(SANS_PUB)) {
            const std::string t = ui::majuscules(ui::tr("market.achete"));
            const float c = 20 * k, tw = ui::width(16 * k, t, 0.10f), cx = bx + (bw - tw - c - 8 * k) / 2;
            coche(r, cx, y0 + (h - c) / 2, c, hex(ACCENT2));
            ui::text(r, cx + c + 8 * k, y0 + (h - 16 * k) / 2, 16 * k, t, hex(ACCENT2), ui::GAUCHE, 0.10f, false);
        } else boutonAchat(r, SANS_PUB, bx, y0 + 27 * k, bw, 47 * k, k, k);
    }
    // ---- liens du bas : « Restaurer mes achats » (souligné, touchable) · « Paiement sécurisé par Google Play » · « Prix TTC »
    {
        ui::AvecPolice inter(ui::Police::Inter);
        const float tp = 14 * k, yb = oy + 660 * k, x0 = ox + 546 * k;
        const bool resActif = boutique() && !g.restauration;
        const std::string tr = ui::tr("market.restaurer"), tg = ui::tr("market.google"), tt = ui::tr("market.ttc");
        const float w1 = ui::width(tp, tr, 0), w2 = ui::width(tp, tg, 0);
        const bool pRes = g.doigt >= 0 && g.cible == RESTAURER;
        const vec4 c1 = hex(resActif ? (pRes ? 0xFFFFFF : 0xE6E8EE) : MUTED);
        ui::text(r, x0, yb, tp, tr, c1, ui::GAUCHE, 0, false);
        r.uiRect(x0, yb + tp * 1.02f, w1, std::max(1.f, 1 * k), c1);   // souligné
        g.restaurer = {x0 - 6 * k, yb - (44 * k - tp) / 2, w1 + 12 * k, 44 * k};   // zone ≥ 44 px
        ui::text(r, x0 + w1 + 28 * k, yb, tp, tg, hex(0xC9CDDA), ui::GAUCHE, 0, false);
        ui::text(r, x0 + w1 + 28 * k + w2 + 28 * k, yb, tp, tt, hex(0xC9CDDA), ui::GAUCHE, 0, false);
        // RG-05 : hors ligne (aucune boutique ou aucun prix de Google) : une ligne à droite des liens
        bool unPrix = false;
        for (int i = 0; i < MARKET_PRODUITS; i++) unPrix = unPrix || !prixDe(i).empty();
        if (!unPrix) {
            const float lx = x0 + w1 + w2 + ui::width(tp, tt, 0) + 84 * k;
            r.uiRect(lx, yb + tp / 2 - 4 * k, 8 * k, 8 * k, hex(ACCENT));   // pastille d'alerte (interface 2D)
            ui::textBoite(r, lx + 16 * k, yb, std::max(0.f, xr - 45 * k - lx - 16 * k), tp, tp, ui::tr("market.hors_ligne"), hex(TEXTE), ui::GAUCHE, 0);
        }
    }
    // ---- message (« +720 ★ », attente, restauration) : bandeau en haut, 2,6 s, fondu de sortie
    if (!g.message.empty() && g.tMessage >= 0) {
        const double age = temps - g.tMessage;
        if (age >= DUREE_MESSAGE) g.message.clear();
        else {
            const float a = (float)std::clamp((DUREE_MESSAGE - age) / 0.4, 0.0, 1.0) * (float)std::clamp(age / 0.12, 0.0, 1.0);
            const float mp = 24 * k, mw = std::min(W * 0.42f, garage::largeurEtoiles(mp, g.message, 0.04f) + 60 * k), mh = 46 * k;
            const float mx = (W - mw) / 2, my = oy + 100 * k;
            rond(r, mx, my, mw, mh, mh / 2, hex(0xFF7A1A, 0.96f * a), hex(ACCENT, 0.96f * a));
            const float tpm = std::min(mp, (mw - 30 * k) / std::max(1.f, garage::largeurEtoiles(1, g.message, 0.04f)));
            texteEtoiles(r, mx + mw / 2, my + (mh - tpm) / 2, tpm, g.message, hex(BG, a), ui::CENTRE, 0.04f);
        }
    }
    // manette : seules les zones de cet écran ; ✕ est la zone retour (touche Retour d'Android, RG-01)
    manette::zonesImage().clear();
    for (int o = 0; o < 3; o++) manette::focus(g.onglets[o]);
    for (int i = 0; i < MARKET_PRODUITS; i++) manette::focus(g.achat[i]);
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
    else if (c >= ONGLET0) { g.onglet = c - ONGLET0; WFS_LOG("market (WFS-557) : onglet %d", g.onglet); }
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
