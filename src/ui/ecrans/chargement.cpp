// WaForSpeed — écran de chargement de l'application (WFS-35) : voir chargement.h.
// Style : css/style.css du jeu web (.screen-inner, .eyebrow, .logo, .cp-bar i, @keyframes screen-in).
#include "chargement.h"
#include <algorithm>
#include <cmath>
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../core/system.h"
#include "../../game/game.h"
#include "../../render/renderer.h"
#include "../text.h"
#include "../echelle.h"   // WFS-102
#include "../../game/commandes_tactiles.h"   // WFS-102 (RG-15) : fondu en cours

namespace wfs {
namespace ecrans {

// jetons :root du jeu web (couleurs de l'interface, comme ui::CYAN / ui::ORANGE de WFS-26)
static vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
static const unsigned BG = 0x0A0B10, SURFACE2 = 0x1E2130, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, DANGER = 0xFF1F4B,
                      ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
static const char* MESSAGE_ECHEC = "Le moteur 3D n'a pas pu démarrer sur cet appareil.";   // main.js boot()
static float clampv(float v, float a, float b) { return std::max(a, std::min(b, v)); }
// WFS-102 (RG-10) : bouton de l'écran d'erreur (zone posée à la dernière image), doigt qui le tient, demande, libellé
static struct { float x = 0, y = 0, w = 0, h = 0; } g_reessayer;
static int g_doigtErreur = -1; static bool g_reessai = false, g_quitter = false;

MiseEnPage miseEnPage(float W, float H) {
    MiseEnPage m;
    m.k = ui::echelle(W, H);   // WFS-102 (RG-01) : 1 px CSS = 1 dp (k = 1 en 1280x720)
    m.logo = clampv(0.14f * W / m.k, 64, 140) * m.k;                     // clamp(64px, 14vw, 140px)
    m.x = clampv(0.06f * W / m.k, 0, 96) * m.k + 16 * m.k;               // margin-left clamp(0, 6vw, 96px) + gouttière
    m.largeur = std::min(600 * m.k, W - m.x - 16 * m.k);                // max-width: 600px
    m.sur = 12 * m.k; m.gap = 22 * m.k; m.ligne = m.logo * 0.82f;
    if (ui::compact(W, H)) { m.logo = 56 * m.k; m.gap = 10 * m.k; m.ligne = m.logo * 0.82f; }   // WFS-102 (RG-02) : téléphone couché
    m.hauteurBloc = m.sur + m.gap + 2 * m.ligne;
    return m;
}

float dessinerBlocLogo(Renderer& r, const MiseEnPage& m, float y, float op) {
    const float x = m.x, k = m.k, logo = m.logo, sur = m.sur, gap = m.gap, ligne = m.ligne;
    const bool police = ui::fontReady();   // première image : la police n'est pas encore construite (fond + barre)
    // sur-titre .eyebrow : capitales cyan espacées de 0,22 em
    if (police) ui::text(r, x, y, sur, ui::majuscules("Sprint urbain · vraies voitures"), hex(ACCENT2, op), ui::GAUCHE, 0.22f, false);
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

void dessinerChargement(Renderer& r, float W, float H, float p, const std::string& libelle, bool erreur, float op) {
    const MiseEnPage m = miseEnPage(W, H);
    const float k = m.k, x = m.x, largeur = m.largeur, gap = m.gap;
    r.uiRect(0, 0, W, H, hex(BG, op));   // fond --bg plein écran
    // WFS-102 (RG-10, ANO-39) : reflets de .screen--loading : radial-gradient(120% 60% at 50% 110%, rgba(255,106,0,.14))
    // et radial-gradient(80% 50% at 85% 100%, rgba(0,229,255,.10)), en éventails de quadrilatères à couleurs par sommet
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
    reflet(W * 0.5f, H * 1.1f, W * 1.2f, H * 0.6f, hex(ACCENT, 0.14f * op));
    reflet(W * 0.85f, H * 1.0f, W * 0.8f, H * 0.5f, hex(ACCENT2, 0.10f * op));
    const float barre = 6 * k, txt = 19 * k;   // libellé : 19 px à 720 p (à 16 px, « 3D » se lisait « 30 »)
    const float total = m.hauteurBloc + gap + barre + 10 * k + txt;
    float y = std::max(16 * k, (H - total) / 2);
    const bool police = ui::fontReady();
    y = dessinerBlocLogo(r, m, y, op);
    // barre .cp-bar : fond --surface-2, remplissage dégradé cyan → orange (16 tranches sur la partie remplie)
    r.uiRect(x, y, largeur, barre, hex(SURFACE2, op));
    const float plein = largeur * clampv(p, 0, 1);
    const int N = 16;
    for (int i = 0; i < N && plein > 0; i++) {
        const float t = (i + 0.5f) / N;
        const vec4 a = hex(ACCENT2, op), b = hex(ACCENT, op);
        // tranches bord à bord, arrêtées au pixel (aucun recouvrement visible pendant le fondu)
        const float x0 = std::floor(x + plein * i / N), x1 = i == N - 1 ? x + plein : std::floor(x + plein * (i + 1) / N);
        if (x1 > x0) r.uiRect(x0, y, x1 - x0, barre, {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, op});
    }
    y += barre + 10 * k;
    // étape et pourcentage (renderLoadProgress : libellé + ' ' + n + ' %') ou message d'échec
    if (!police) return;
    if (erreur) {   // WFS-102 (RG-10) : message 16 px et bouton « Réessayer » (ou « Quitter » si le moteur ne peut pas repartir)
        ui::text(r, x, y, 16 * k, MESSAGE_ECHEC, hex(DANGER, op), ui::GAUCHE);
        const float tp = 20 * k, bh = 56 * k, by = y + 16 * k + 24 * k;
        const std::string t = ui::majuscules(g_quitter ? "Quitter" : "Réessayer");
        const float bw = ui::width(tp, t, 0.12f) + 56 * k;
        g_reessayer = {x, by, bw, bh};
        ui::biseau(r, x, by, bw, bh, 14 * k, hex(ACCENT, op));
        ui::text(r, x + bw / 2, by + (bh - tp) / 2, tp, t, hex(BG, op), ui::CENTRE, 0.12f, false);
    }
    else ui::text(r, x, y, txt, libelle + " " + ui::nombre((long long)std::lround(clampv(p, 0, 1) * 100)) + " %", hex(MUTED, op), ui::GAUCHE);
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
    g_fonduArme = true; g_fonduParti = false; fonduChargement() = true;   // WFS-102 (RG-15)
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

// WFS-102 (RG-10) : appui « au lever du doigt » sur le bouton de l'écran d'erreur (même règle que les autres écrans)
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

// fondu de 300 ms (@keyframes screen-in) : commence à la première image de la scène après fin()
float opaciteFondu(double temps) {
    if (!g_fonduArme) return 0;
    if (!g_fonduParti) { g_fonduParti = true; g_fonduT0 = temps; }
    const double d = temps - g_fonduT0;
    if (d < 0 || d >= 0.3) { g_fonduArme = false; fonduChargement() = false; return 0; }
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
    // WFS-102 (RG-10) : appui sur « Réessayer » (posé puis levé) → relance demandée, écran d'erreur quitté
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
