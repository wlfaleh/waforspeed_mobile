// WaForSpeed — écran de chargement de l'application (WFS-35) : voir chargement.h.
// Style : css/style.css du jeu web (.screen-inner, .eyebrow, .logo, .cp-bar i, @keyframes screen-in).
#include "chargement.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/memoire.h"   // WFS-297 (RG-06)
#include "../../game/traffic_system.h"   // WFS-297 : voitures du trafic chargées
#include "../../core/platform.h"
#include "../../core/system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "../verre.h"
#include "../boutons.h"   // WFS-557 : « Réessayer » rond (Start/Stop), formes arrondies   // WFS-548 (R19) : fenêtre de verre, sur-titre gris, dégradé de la marque
#include "../langue.h"   // WFS-505 : textes de l'écran de chargement en français ou en anglais
#include "textes_banc.h"   // WFS-505 : banc « textes » (journal des textes de cet écran)
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)
#include "../../game/commandes_tactiles.h"   // WFS-146 : dessin2d::biseau du bouton de l'écran d'erreur

namespace wfs {
namespace ecrans {

// jetons :root du jeu web (couleurs de l'interface, comme ui::CYAN / ui::ORANGE de WFS-26)
static vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
static const unsigned BG = 0x0A0B10, SURFACE2 = 0x1E2130, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, DANGER = 0xFF1F4B,
                      ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
static const char* MESSAGE_ECHEC = "Le moteur 3D n'a pas pu démarrer sur cet appareil.";   // main.js boot() ; journal seulement (écran : clé chargement.echec, WFS-505)
// WFS-505 (RG-01) : les étapes arrivent en français (Game::init, écran des paquets) et restent ainsi au journal ; à l'écran,
// le libellé connu est remplacé par sa clé dans la langue active (inconnu : affiché tel quel, le banc « textes » le signale)
static const char* const ETAPES[][2] = {
    {"Préparation du moteur", "chargement.etape.moteur"},
    {"Construction de la route", "chargement.etape.route"},
    {"Chargement du modèle 3D…", "chargement.etape.modele"},
    {"Construction de la ville", "chargement.etape.ville"},
    {"Paquets de cartes", "chargement.etape.paquets"},
};
static std::string libelleAffiche(const std::string& fr) {
    for (const auto& e : ETAPES) if (fr == e[0]) return ui::tr(e[1]);
    return fr;
}
static float clampv(float v, float a, float b) { return std::max(a, std::min(b, v)); }
#ifndef WFS_VERSION
#define WFS_VERSION "dev"   // WFS-279 : compilé sans CMake (valeur fixe du PC)
#endif
// WFS-146 (RG-10, ANO-39) : bouton de l'écran d'erreur (zone posée à la dernière image), doigt qui le tient, demande, libellé
static struct { float x = 0, y = 0, w = 0, h = 0; } g_reessayer;
static int g_doigtErreur = -1; static bool g_reessai = false, g_quitter = false;

MiseEnPage miseEnPage(float W, float H) {
    MiseEnPage m;
    m.k = H / 720.f;   // 1 px CSS = k pixels logiques (720 p de référence) : même aspect à 1280x720 et 2400x1080
    m.logo = clampv(0.14f * W / m.k, 64, 140) * m.k;                     // clamp(64px, 14vw, 140px)
    m.x = clampv(0.06f * W / m.k, 0, 96) * m.k + 16 * m.k;               // margin-left clamp(0, 6vw, 96px) + gouttière
    m.largeur = std::min(600 * m.k, W - m.x - 16 * m.k);                // max-width: 600px
    m.sur = 12 * m.k; m.gap = 22 * m.k; m.ligne = m.logo * 0.82f;
    m.hauteurBloc = m.sur + m.gap + 2 * m.ligne;
    return m;
}

MiseEnPage miseEnPagePetit(float W, float H, float logoMax, float gap) {
    MiseEnPage m;
    m.k = ui::unitePetitEcran(W, H);
    m.logo = clampv(0.14f * W / m.k, 56, logoMax) * m.k;
    m.x = clampv(0.06f * W / m.k, 0, 96) * m.k + 16 * m.k;
    m.largeur = std::min(600 * m.k, W - m.x - 16 * m.k);
    m.sur = 12 * m.k; m.gap = gap * m.k; m.ligne = m.logo * 0.82f;
    m.hauteurBloc = m.sur + m.gap + 2 * m.ligne;
    return m;
}

float dessinerBlocLogo(Renderer& r, const MiseEnPage& m, float y, float op) {
    const float x = m.x, k = m.k, logo = m.logo, sur = m.sur, gap = m.gap, ligne = m.ligne;
    const bool police = ui::fontReady();   // première image : la police n'est pas encore construite (fond + barre)
    // sur-titre .eyebrow : capitales espacées de 0,22 em ; WFS-548 (RG-03, maquette 08) : gris (avant : cyan)
    if (police) verre::surTitre(r, x, y, sur, ui::tr("chargement.surtitre"), ui::GAUCHE, op);
    y += sur + gap;
    if (police) {
        // logo « Wa<em>for</em> » / « Speed » : FOR en contour de 2 px (copies décalées, puis remplissage couleur du fond)
        ui::text(r, x, y, logo, "WA", hex(TEXTE, op), ui::GAUCHE, -0.01f, false);
        const float xf = x + ui::width(logo, "WA", -0.01f) - 0.01f * logo;
        const float e = 2 * k;
        if (op < 0.999f) ui::text(r, xf, y, logo, "FOR", hex(MUTED, op), ui::GAUCHE, -0.01f, false);   // fondu : couleur unie
        else for (int i = 0; i < 8; i++) {
            const float a = i * 0.785398f;
            ui::text(r, xf + std::cos(a) * e, y + std::sin(a) * e, logo, "FOR", hex(TEXTE, op), ui::GAUCHE, -0.01f, false);
        }
        if (op >= 0.999f) ui::text(r, xf, y, logo, "FOR", hex(BG, op), ui::GAUCHE, -0.01f, false);
        y += ligne;
        // SPEED orange avec halo léger (text-shadow 0 0 22px rgba(255,106,0,.45))
        for (int i = 0; i < 4 && op >= 0.999f; i++) {   // halo seulement à pleine opacité (sinon les copies s'additionnent)
            const float a = 0.785398f + i * 1.570796f;
            ui::text(r, x + std::cos(a) * 4 * k, y + std::sin(a) * 4 * k, logo, "SPEED", hex(ACCENT, 0.18f * op), ui::GAUCHE, -0.01f, false);
        }
        ui::text(r, x, y, logo, "SPEED", hex(ACCENT, op), ui::GAUCHE, -0.01f, false);
        y += ligne + gap;
    } else y += 2 * ligne + gap;
    return y;
}

static std::string texteChargement();   // WFS-297 (RG-06), plus bas

// WFS-557 (maquette 08) : 4 étapes sous la barre ; étape en cours = celle du libellé, ou celle de la progression si elle est
// plus loin (le chargement des voitures n'a pas de libellé propre) ; bornes de progression de chaque étape
static const char* const ETAPES_BARRE[4] = {"chargement.barre.moteur", "chargement.barre.modeles", "chargement.barre.voitures", "chargement.barre.ville"};
static const float BORNES[5] = {0.f, 0.20f, 0.45f, 0.70f, 1.f};
static int etapeEnCours(float p, const std::string& libelle) {
    int e = 0;
    while (e < 3 && p >= BORNES[e + 1]) e++;
    if (libelle == "Chargement du modèle 3D…") e = std::max(e, 1);
    if (libelle == "Construction de la ville") e = 3;
    return e;
}

// WFS-557 (maquettes 08 et 08b ; arbitrage du PO du 2026-11-16 : maquette à 100 %). Unité k = H / 720 (maquette 1600 × 720).
// Sur-titre, logo « WA FOR / SPEED » en italique (FOR au trait), fenêtre de verre 687 × 197 : étape en Inter (rouge si échec),
// gros pourcentage à droite, barre arrondie au dégradé de la marque (vers le rouge si échec), 4 étapes (Moteur, Modèles 3D,
// Voitures, Ville), deux lignes d'information en Inter grise ; échec : « Réessayer » rond (Start/Stop) en bas à droite.
void dessinerChargement(Renderer& r, float W, float H, float p, const std::string& libelle, bool erreur, float op) {
    const textes::Journal journal;   // WFS-505 : textes notés au banc « textes »
    const float k = H / 720.f;
    p = clampv(p, 0, 1);
    r.uiRect(0, 0, W, H, hex(BG, op));   // fond --bg plein écran
    // WFS-146 (RG-10, ANO-39) : reflets d'asphalte (centre à l'opacité du web, bord transparent)
    auto reflet = [&](float cx, float cy, float rx, float ry, const vec4& c) {
        const int N = 48;
        for (int i = 0; i < N; i++) {
            const float a0 = 6.2831853f * i / N, a1 = 6.2831853f * (i + 1) / N;
            const UiVertex q[4] = {{cx, cy, -1, -1, c.x, c.y, c.z, c.w}, {cx, cy, -1, -1, c.x, c.y, c.z, c.w},
                                   {cx + rx * std::cos(a1), cy + ry * std::sin(a1), -1, -1, c.x, c.y, c.z, 0},
                                   {cx + rx * std::cos(a0), cy + ry * std::sin(a0), -1, -1, c.x, c.y, c.z, 0}};
            r.uiQuad(q);
        }
    };
    reflet(W * 0.5f, H * 1.1f, W * 1.2f, H * 0.6f, hex(ACCENT, 0.08f * op));
    reflet(W * 0.85f, H * 1.0f, W * 0.8f, H * 0.5f, hex(ACCENT2, 0.05f * op));
    const bool police = ui::fontReady();
    const float x = 70 * k;
    if (police) {
        verre::surTitre(r, x, 71 * k, 17 * k, ui::tr("chargement.surtitre"), ui::GAUCHE, op);
        // logo : « WA » blanc, « FOR » au trait (copies décalées puis fond), « SPEED » orange avec halo ; italique (maquette 08)
        const float logo = 120 * k, xl = x + 6 * k, yl = 83 * k, ys = 166 * k;
        ui::text(r, xl, yl, logo, "WA", hex(TEXTE, op), ui::GAUCHE, -0.02f, false, true);
        const float xf = xl + ui::width(logo, "WA", -0.02f) - 0.02f * logo, e = 2 * k;
        if (op < 0.999f) ui::text(r, xf, yl, logo, "FOR", hex(MUTED, op), ui::GAUCHE, -0.02f, false, true);
        else {
            for (int i = 0; i < 8; i++) { const float a = i * 0.785398f; ui::text(r, xf + std::cos(a) * e, yl + std::sin(a) * e, logo, "FOR", hex(TEXTE, op), ui::GAUCHE, -0.02f, false, true); }
            ui::text(r, xf, yl, logo, "FOR", hex(BG, op), ui::GAUCHE, -0.02f, false, true);
        }
        for (int i = 0; i < 8 && op >= 0.999f; i++) {   // halo orange (text-shadow 0 0 22px)
            const float a = i * 0.785398f;
            ui::text(r, xl + std::cos(a) * 7 * k, ys + std::sin(a) * 7 * k, logo, "SPEED", hex(ACCENT, 0.10f * op), ui::GAUCHE, -0.02f, false, true);
        }
        ui::text(r, xl, ys, logo, "SPEED", hex(ACCENT, op), ui::GAUCHE, -0.02f, false, true);
    }
    // fenêtre de verre
    const float px = x, py = 341 * k, pw = 687 * k, ph = 197 * k, xi = px + 24 * k, wi = pw - 47 * k;
    { verre::Style st; st.rayon = 16 * k; st.lisere = 1.5f * k; st.opacite = op; verre::panneau(r, px, py, pw, ph, st); }
    const vec4 rouge = verre::hexa(verre::PERTE, op);
    // barre : piste sombre arrondie, remplissage du dégradé de la marque (cyan → orange ; échec : cyan → rouge), halo en bout
    const float by = 426 * k, bh = 7 * k;
    {
        vec2 q[ui::boutons::MAX_SOMMETS];
        const int n = ui::boutons::contourArrondi(q, xi, by, wi, bh, bh / 2);
        dessin2d::polygone(r, q, n, dessin2d::uni(hex(0x2A2D36, op)));
        const float plein = std::max(bh, wi * p), R = bh / 2;
        if (p > 0) {
            const vec4 c0 = hex(ACCENT2, op), c1 = erreur ? rouge : hex(ACCENT, op);
            const int m = ui::boutons::contourArrondi(q, xi, by, plein, bh, R);
            dessin2d::halo(r, q, m, 6 * k, {c1.x, c1.y, c1.z, 0.35f * op});
            // corps en dégradé horizontal (couleurs par sommet), bouts arrondis de la couleur de leur côté
            const float x0 = xi + R, x1 = xi + plein - R;
            if (x1 > x0) {
                const UiVertex v[4] = {{x0, by, -1, -1, c0.x, c0.y, c0.z, c0.w}, {x1, by, -1, -1, c1.x, c1.y, c1.z, c1.w},
                                       {x1, by + bh, -1, -1, c1.x, c1.y, c1.z, c1.w}, {x0, by + bh, -1, -1, c0.x, c0.y, c0.z, c0.w}};
                r.uiQuad(v);
            }
            dessin2d::anneau(r, x0, by + R, 0, R, c0, PI, 2 * PI);
            dessin2d::anneau(r, std::max(x0, x1), by + R, 0, R, c1, 0, PI);
        }
    }
    // 4 étapes : trait de 3 px puis libellé (✓ cyan pour une étape finie)
    const int e = etapeEnCours(p, libelle);
    const float sw = (wi - 3 * 8 * k) / 4;
    for (int i = 0; i < 4; i++) {
        const float sx = xi + i * (sw + 8 * k);
        vec2 q[ui::boutons::MAX_SOMMETS];
        int n = ui::boutons::contourArrondi(q, sx, 449 * k, sw, 3 * k, 1.5f * k);
        dessin2d::polygone(r, q, n, dessin2d::uni(i < e ? hex(0xE6E8EE, op) : hex(0x3A3D46, op)));
        if (i == e) {
            const float f = clampv((p - BORNES[i]) / (BORNES[i + 1] - BORNES[i]), 0.05f, 1.f);
            n = ui::boutons::contourArrondi(q, sx, 449 * k, sw * f, 3 * k, 1.5f * k);
            dessin2d::polygone(r, q, n, dessin2d::uni(erreur ? rouge : hex(ACCENT, op)));
        }
        if (!police) continue;
        ui::AvecPolice inter(ui::Police::InterMedium);
        float tx = sx;
        if (i < e) {   // coche
            const vec2 c[3] = {{sx + 3 * k, 467 * k}, {sx + 7 * k, 471 * k}, {sx + 14 * k, 462 * k}};
            dessin2d::trait(r, c, 3, 1.8f * k, hex(ACCENT2, op));
            tx += 25 * k;
        }
        const vec4 c = i < e ? hex(TEXTE, op) : i == e ? (erreur ? rouge : hex(0xFF8A3D, op)) : hex(MUTED, op);
        ui::text(r, tx, 460 * k, 13 * k, ui::tr(ETAPES_BARRE[i]), c, ui::GAUCHE, 0, false);
    }
    // WFS-279 (RG-01) : version en petit, coin bas droit
    if (!police) return;
    ui::text(r, W - 31 * k, H - 31 * k - 12 * k, 12 * k, texteVersion(versionJeu()), hex(MUTED, op), ui::DROITE, 0.15f);
    {   // étape (ou échec) et pourcentage
        ui::AvecPolice inter(ui::Police::InterMedium);
        const std::string lib = erreur ? ui::tr("chargement.echec") : libelleAffiche(libelle);
        ui::textBoite(r, xi, 386 * k, wi - 140 * k, 20 * k, 17 * k, lib, erreur ? rouge : hex(TEXTE, op), ui::GAUCHE, 0);
    }
    {
        const std::string pc = std::to_string((int)std::lround(p * 100));
        const float xp = xi + wi - 20 * k;
        ui::text(r, xp - 4 * k, 359 * k, 60 * k, pc, erreur ? rouge : hex(TEXTE, op), ui::DROITE, 0, false, true);
        ui::text(r, xp, 385 * k, 22 * k, "%", hex(0xC9CDDA, op), ui::GAUCHE, 0, false, true);
    }
    {   // WFS-297 (RG-06) : données chargées et mémoire, en Inter grise
        ui::AvecPolice inter(ui::Police::Inter);
        ui::text(r, xi, 489 * k, 12 * k, texteChargement(), hex(MUTED, op), ui::GAUCHE, 0.f, false);
        ui::text(r, xi, 505 * k, 12 * k, texteMemoire(true), hex(MUTED, op), ui::GAUCHE, 0.f, false);
    }
    if (erreur) {   // WFS-146 (RG-10) : « Réessayer » (« Quitter » si le moteur ne repart pas), bouton rond de 142 px (maquette 08b)
        const float d = 142 * k;
        const char* cle = g_quitter ? "chargement.quitter" : "chargement.reessayer";
        g_reessayer = {W - 116 * k - d, H - 74 * k - d, d, d};
        ui::boutons::startStop(r, g_reessayer.x, g_reessayer.y, d, d, ui::majuscules(ui::tr(cle)), op, g_doigtErreur >= 0, "", 21 * k);
    } else g_reessayer = {};
}

const char* versionJeu() { return WFS_VERSION; }   // WFS-279

// WFS-297 (RG-06) : chiffres mesurés (src/core/memoire.h), milliers avec ui::nombre
std::string texteMemoire(bool enChargement) {
    const double p = memoire::processusMo();
    const int n = memoire::compteurs().partie.load() + (enChargement ? 1 : 0);
    // WFS-505 (RG-01) : « Mémoire : jeu {1} Mo · carte graphique {2} Mo · partie n » dans la langue active
    return ui::tr("chargement.memoire", p < 0 ? std::string("?") : ui::nombre(std::llround(p)), ui::nombre(std::llround(memoire::gpuMo()))) + " · " +
           (n > 0 ? ui::tr("chargement.partie", std::to_string(n)) : ui::tr("chargement.aucune_partie"));
}
static std::string texteChargement() {   // « Lus pour cette partie : 136 Mo · 983 objets 3D · 47 voitures · 114 personnes »
    const Traffic* tr = activeTraffic();
    const memoire::Compteurs& c = memoire::compteurs();
    // WFS-505 (RG-01) : même ligne dans la langue active (tables : chargement.lus, .objets, .voitures, .personnes)
    return ui::tr("chargement.lus", ui::nombre(std::llround(memoire::lusChargementMo()))) + " · " + ui::tr("chargement.objets", ui::nombre(c.objets3d.load())) + " · " +
           ui::tr("chargement.voitures", ui::nombre(tr ? (long long)tr->cars.size() : 0)) + " · " + ui::tr("chargement.personnes", ui::nombre(c.personnes.load()));
}

// ---------------------------------------------------------------- écran plein (avant la scène)
static void image(Renderer& r, float p, const std::string& lib, bool err) {
    FrameParams fp;
    fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
    if (!r.beginFrame(fp)) return;
    dessinerChargement(r, (float)r.width(), (float)r.height(), p, lib, err, 1);
    r.endFrame();
}

void Chargement::debut(Renderer& r, double depuis) {
    r_ = &r; p_ = 0; n_ = 0; erreur_ = false;
    libelle_ = "Préparation du moteur";
    t0_ = tEtape_ = nowSeconds();
    // première image tout de suite (fond + barre), puis police (≈ 0,1 à 0,9 s) et image complète à 0 %
    if (!ui::fontReady()) image(r, p_, libelle_, false);
    const double premiere = nowSeconds() - depuis;
    ui::chargerPolice(r);
    image(r, p_, libelle_, false);
    WFS_LOG("chargement : première image %s s après l'initialisation Vulkan, écran complet (police) %s s",
            ui::decimal(premiere, 3).c_str(), ui::decimal(nowSeconds() - depuis, 3).c_str());
    if (apresImage) apresImage(0);
}

void Chargement::etape(float p, const char* libelle) {
    if (!r_) return;
    const double t = nowSeconds();
    WFS_LOG("chargement : %s %s s", libelle_.c_str(), ui::decimal(t - tEtape_, 2).c_str());
    tEtape_ = t; n_++;
    p_ = std::max(p_, clampv(p, 0, 1));   // la progression ne recule jamais
    if (libelle && *libelle) libelle_ = libelle;
    image(*r_, p_, libelle_, false);
    if (apresImage) apresImage(n_);
}

void Chargement::progression(float p, const char* l) { chargement().etape(p, l); }

static bool g_fonduArme = false, g_fonduParti = false; static double g_fonduT0 = 0;

void Chargement::fin() {
    etape(1, nullptr);   // 100 % : la dernière étape est finie
    WFS_LOG("chargement : total %s s", ui::decimal(nowSeconds() - t0_, 2).c_str());
    g_fonduArme = true; g_fonduParti = false;
}

void Chargement::erreur(bool quitter) {
    erreur_ = true; g_quitter = quitter; g_reessai = false; g_doigtErreur = -1;
    WFS_ERR("chargement : échec à l'étape « %s » — %s", libelle_.c_str(), MESSAGE_ECHEC);
    dessiner();
}

void Chargement::dessiner() {
    if (!r_) return;
    image(*r_, p_, libelle_, erreur_);
    if (apresImage) apresImage(-1);
}

// WFS-505 (banc « textes ») : centre du bouton de l'écran d'erreur (« Réessayer » ou « Quitter ») à la dernière image
bool chargementBouton(float& x, float& y) {
    if (g_reessayer.w <= 0) return false;
    x = g_reessayer.x + g_reessayer.w / 2; y = g_reessayer.y + g_reessayer.h / 2; return true;
}

// WFS-146 (RG-10) : appui « au lever du doigt » sur le bouton de l'écran d'erreur (même règle que les autres écrans)
bool Chargement::toucher(int id, float x, float y, bool down) {
    if (!erreur_) return false;
    const bool dans = x >= g_reessayer.x && x <= g_reessayer.x + g_reessayer.w && y >= g_reessayer.y && y <= g_reessayer.y + g_reessayer.h;
    if (down) { if (g_doigtErreur < 0) g_doigtErreur = dans ? id : -2; return true; }
    if (id == g_doigtErreur && dans) { g_reessai = true; WFS_LOG("chargement : « %s »", g_quitter ? "Quitter" : "Réessayer"); }
    if (id == g_doigtErreur || g_doigtErreur == -2) g_doigtErreur = -1;
    return true;
}
bool Chargement::reessaiDemande() {
    if (!g_reessai) return false;
    g_reessai = false;
    if (!g_quitter) erreur_ = false;   // la plateforme relance le chargement
    return true;
}

Chargement& chargement() { static Chargement c; return c; }

// WFS-146 (RG-09, ANO-45) : fondu du chargement armé ou en cours (commandes tactiles de conduite cachées)
bool fonduEnCours() { return g_fonduArme; }

// fondu de 300 ms (@keyframes screen-in) : commence à la première image de la scène après fin()
float opaciteFondu(double temps) {
    if (!g_fonduArme) return 0;
    if (!g_fonduParti) { g_fonduParti = true; g_fonduT0 = temps; }
    const double d = temps - g_fonduT0;
    if (d < 0 || d >= 0.3) { g_fonduArme = false; return 0; }
    return (float)(1 - d / 0.3);
}

// le fondu est dessiné par-dessus l'interface de la scène (ordre 490 : après le texte et le HUD)
class FonduChargement : public System {
public:
    const char* name() const override { return "fondu du chargement"; }
    void drawUi(Renderer& r, const GameView& g) override {
        const float a = opaciteFondu(g.time);
        if (a > 0) dessinerChargement(r, g.uiWidth, g.uiHeight, 1, chargement().libelle(), false, a);
    }
};

void benchChargement(Game& g, Renderer& r, const std::string& out, double depuis, void (*racine)(bool ok)) {
    Chargement& ch = chargement();
    auto capture = [&](const char* nom) {
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %s %d %%%s", nom, ch.libelle().c_str(), (int)std::lround(ch.pourcent() * 100), ch.enErreur() ? " (échec)" : "");
    };
    // 1) chargement normal : 0 %, début du modèle 3D, 100 %, puis milieu du fondu sur la scène
    ch.apresImage = [&](int n) {
        if (n == 0) capture("chargement-0");
        if (n == 2) capture("chargement-1");
        if (n == 4) capture("chargement-2");
    };
    ch.debut(r, depuis);
    const bool ok = g.init(r, "golf", "ville", Chargement::progression);
    if (!ok) ch.erreur(); else ch.fin();
    ch.apresImage = nullptr;
    g.render(500); g.render(500.15);   // fondu : 0,15 s sur 0,3 s
    r.capturePng(out + "/chargement-fondu.png");
    WFS_LOG("capture chargement-fondu : écran de chargement à %.0f %% d'opacité", opaciteFondu(500.15) * 100);
    g.render(500.4);                   // fondu terminé
    g.shutdown();
    // 2) échec : fichiers introuvables → message rouge, pas de plantage
    ch.apresImage = [&](int n) { if (n < 0) capture("chargement-erreur"); };
    ch.debut(r, nowSeconds());
    racine(false);
    if (!g.init(r, "golf", "ville", Chargement::progression)) ch.erreur(); else WFS_ERR("chargement : l'échec attendu n'a pas eu lieu");
    racine(true);
    ch.apresImage = nullptr;
    // WFS-146 (RG-10) : appui sur « Réessayer » (posé puis levé) → relance demandée, écran d'erreur quitté
    {
        const float bx = g_reessayer.x + g_reessayer.w / 2, by = g_reessayer.y + g_reessayer.h / 2;
        ch.toucher(3, bx, by, true); ch.toucher(3, bx, by, false);
        const bool demande = ch.reessaiDemande();
        WFS_LOG("chargement : « Réessayer » %s, écran d'erreur %s", demande ? "demandé" : "NON demandé (ÉCHEC)", ch.enErreur() ? "encore affiché" : "quitté");
    }
    g.shutdown();
}

} // namespace ecrans

WFS_SYSTEM(ecrans::FonduChargement, 490)

} // namespace wfs
