// WaForSpeed — HUD de course (WFS-27) : voir hud.h.
#include "hud.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/camera_vue.h"           // WFS-98 : vue cockpit (etatCockpit)
#include "../game/commandes_tactiles.h"   // WFS-98 : place des commandes (appuis du banc)
#include "../game/course_system.h"
#include "../game/course5.h"   // WFS-119 : modeCourse
#include "../game/game.h"
#include "../render/renderer.h"
#include "ecrans/reglages.h"   // WFS-75 : miroirX (lecture seule)
#include "minicarte.h"         // WFS-173 : banc de la mini-carte
#include "text.h"
#include "toasts.h"

namespace wfs {
namespace {

// jetons :root du jeu web (codes sRGB, comme ui::CYAN)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, DANGER = 0xFF1F4B, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF,
               SURFACE2 = 0x1E2130, SUCCESS = 0xB6FF00;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};   // --glass rgba(20,22,31,.72)
// WFS-245 : tailles, couleurs et contrastes des textes du HUD (hud.h, namespace hud)
using hud::CAPITALES; using hud::BASE; using hud::ETIQUETTE_PX; using hud::ARRIVEE_PX; using hud::ETIQUETTE; using hud::ETIQUETTE_ARC;
const vec4 FOND_PANNEAU = hex(hud::FOND, hud::FOND_ALPHA);   // 0x0A0B10 à 0,85 (WFS-135 : celui du score)

// halo néon (text-shadow) : copies transparentes du texte autour de sa place
void halo(Renderer& r, float x, float y, float px, const std::string& t, const vec4& c, ui::Align a, float rad, bool italique = false) {
    for (int k = 0; k < 12; k++) {
        const float ang = k * 0.5235988f, d = rad * (k % 2 ? 1.f : 0.55f);
        ui::text(r, x + std::cos(ang) * d, y + std::sin(ang) * d, px, t, c, a, 0, false, italique);
    }
}
// .label : capitales, espacement 0,16 em, calées sur les pixels entiers ; WFS-245 : 27 px de référence (au lieu de 11, porté à
// 13 au moins), gris clair #A6ADC2 (au lieu de #8A90A6 : 2,3 : 1 seulement)
float labelPx(float k) { return std::round(ETIQUETTE_PX * k); }
void label(Renderer& r, float x, float y, float k, const std::string& t, ui::Align a = ui::GAUCHE, const vec4* c = nullptr) {
    ui::text(r, std::round(x), std::round(y), labelPx(k), ui::majuscules(t), c ? *c : hex(ETIQUETTE), a, 0.16f, false);
}
// étiquette posée sur une ligne de base (alignée sur des chiffres d'une autre taille)
void labelSurBase(Renderer& r, float x, float base, float k, const std::string& t, ui::Align a = ui::GAUCHE, const vec4* c = nullptr) {
    label(r, x, base - BASE * labelPx(k), k, t, a, c);
}
float largeurLabel(float k, const std::string& t) { return ui::width(labelPx(k), ui::majuscules(t), 0.16f); }
// WFS-296 (RG-01) : marche arrière bien visible : pastille rouge inclinée (biais de 10°, comme les boutons du jeu) avec un grand
// « R » noir (texte sur couleur vive : #0A0B10, contraste 5,4 : 1) et un halo rouge ; (x, y) = coin haut gauche, h = hauteur
void pastilleArriere(Renderer& r, float x, float y, float h, float k) {
    const float w = h * 0.95f, biais = h * 0.176f;   // tan 10°
    dessin2d::biseau(r, x - 3 * k, y - 3 * k, w + 6 * k, h + 6 * k, biais, dessin2d::uni(hex(DANGER, 0.30f)));   // halo néon
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(hex(DANGER)));
    const float tp = h * 0.80f, base = y + h * 0.5f + 0.5f * CAPITALES * tp;
    ui::text(r, x + (w + biais) * 0.5f, base - BASE * tp, tp, "R", hex(hud::FOND), ui::CENTRE, 0, false, true);
}
// barre de progression dégradée cyan → orange (.cp-bar i), tranches bord à bord
void barre(Renderer& r, float x, float y, float w, float h, float p) {
    r.uiRect(x, y, w, h, hex(SURFACE2));
    const float plein = w * std::clamp(p, 0.f, 1.f);
    const int N = 12;
    const vec4 a = hex(ACCENT2), b = hex(ACCENT);
    for (int i = 0; i < N && plein > 0; i++) {
        const float t = (i + 0.5f) / N;
        const float x0 = std::floor(x + plein * i / N), x1 = i == N - 1 ? x + plein : std::floor(x + plein * (i + 1) / N);
        if (x1 > x0) r.uiRect(x0, y, x1 - x0, h, {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1});
    }
}

// banc d'essai (WFS-46) : vitesse affichée forcée (captures hud-arc-120 / hud-arc-320), −1 = vitesse réelle
int g_kmhForce = -1;
float g_surfaceHud = 0;   // WFS-245 (RG-04) : surface des panneaux de la dernière image, px de référence (720 p) au carré

// compteur en arc (WFS-46) : drawSpeedo(k) de js/ui/hud.js, canvas 380 px affiché en 190 px (.speedo) → rayon 80 px,
// trait 7 px ; arc de 270° depuis 135° (0,75 π), échelle 0-380, zone rouge au-delà de 300, graduations tous les 20
// (grand trait tous les 60). Dessiné en disques (arc) et petits carrés (graduations) : aucun nouveau shader.
void compteur(Renderer& r, float cx, float cy, float k, int kmh) {
    const float R = 80 * k, ep = 7 * k, a0 = 0.75f * PI, span = 1.5f * PI, vmax = 380;
    const int N = 90;
    auto at = [&](int i) { return a0 + span * i / N; };
    for (int i = 0; i <= N; i++)   // piste rgba(30,33,48,.85), zone rouge rgba(255,31,75,.55) au-delà de 300
        r.uiCircle(cx + std::cos(at(i)) * R, cy + std::sin(at(i)) * R, ep / 2,
                   (float)i / N >= 300 / vmax ? hex(DANGER, 0.55f) : vec4{30 / 255.f, 33 / 255.f, 48 / 255.f, 0.85f});
    const float pct = std::clamp(kmh / vmax, 0.f, 1.f);
    const int n = (int)std::lround(pct * N);
    const bool rouge = kmh > 300;
    for (int pass = 0; pass < 2; pass++)   // halo (shadowBlur 16) puis trait
        for (int i = 0; i <= n && n > 0; i++) {
            const float a = at(i), x = std::cos(a), y = std::sin(a);
            // createLinearGradient(0, s, s, 0) : du bas gauche (cyan) au haut droit (orange)
            const float t = std::clamp(((0.5f + x * 0.42f) + (1 - (0.5f + y * 0.42f))) / 2, 0.f, 1.f);
            const vec4 c0 = hex(ACCENT2), c1 = hex(ACCENT);
            vec4 c = rouge ? hex(DANGER) : vec4{c0.x + (c1.x - c0.x) * t, c0.y + (c1.y - c0.y) * t, c0.z + (c1.z - c0.z) * t, 1};
            if (pass == 0) { c.w = 0.16f; r.uiCircle(cx + x * R, cy + y * R, ep * 1.25f, c); }
            else r.uiCircle(cx + x * R, cy + y * R, ep / 2, c);
        }
    for (int v = 0; v <= 380; v += 20) {   // graduations rgba(242,244,248,.55), trait 1,5 px
        const float a = a0 + span * (v / vmax), r1 = R - 11 * k, r2 = R - ((v % 60 == 0) ? 20 : 15) * k;
        for (int s = 0; s <= 4; s++) {
            const float rr = r1 + (r2 - r1) * s / 4.f, w = 1.6f * k;
            r.uiRect(cx + std::cos(a) * rr - w / 2, cy + std::sin(a) * rr - w / 2, w, w, hex(TEXTE, 0.55f));
        }
    }
}

class Hud : public System {
public:
    const char* name() const override { return "HUD"; }
    void drawUi(Renderer& r, const GameView& g) override {
        const Course* c = activeCourse();
        // RG-01 : seulement en course (pas de menu, pas d'écran de fin, pas de scène dédiée)
        if (!c || !g.player || g.ecran != Ecran::Course || g.sceneDediee || c->state == RaceState::Over || !ui::fontReady()) return;
        // temps illimité = pas de vraie course (vitrines et scènes du banc sans règles de course, courseBenchRules(false))
        if (c->timeLeft > 1e6) return;
        const Player& pl = *g.player;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;   // 1 px CSS = k pixels logiques (720 p de référence)
        const float gl = 16 * k, gt = 14 * k, pad = 14 * k;           // gouttières --gutter-*, marge intérieure .panel
        // WFS-52 : zone sûre (encoche, coins arrondis) = marges de view.marges (WFS-51) + gouttières de WFS-27 ;
        // marges à 0 : positions de WFS-27 inchangées
        const Marges& mg = g.marges;
        g_surfaceHud = 0;

        // ---------- haut gauche : temps + checkpoint (.p-time) ----------
        // WFS-245 (RG-02, RG-04, arbitrage du PO) : panneau plus large et moins haut (TEMPS_L × TEMPS_H, plus petit qu'avant :
        // 200 × 112) ; ligne 1 : chrono puis TEMPS sur sa ligne de base ; ligne 2 : ARRIVÉE puis la distance en grands chiffres
        // (ou CHECKPOINT n) ; ligne 3 : la barre ; fond du score (contraste des étiquettes)
        {
            const bool ligne = c->duelFinish > 0;   // WFS-110 : course à une ligne d'arrivée (course à 5, duel) : chrono et distance
            const bool bas = !ligne && c->timeLeft < 8;   // .time-value.low
            const std::string t = ligne ? ui::chrono(c->raceTime) : ui::decimal(std::max(0.0, c->timeLeft), 1);
            const std::string dist = ligne ? ui::majuscules(ui::decimal(std::max(0.0, c->duelFinish - pl.dist) * M / 1000, 2) + " km") : "";
            const std::string lib2 = ligne ? "Arrivée" : "Checkpoint " + std::to_string(c->level + 1);
            const float tp = hud::CHRONO_PX * k, ap = ARRIVEE_PX * k, m = hud::TEMPS_MARGE * k, ecart = 8 * k;
            const float l1 = ui::width(tp, t, 0) + ecart + largeurLabel(k, "Temps");
            const float l2 = largeurLabel(k, lib2) + (ligne ? ecart + ui::width(ap, dist, 0) : 0);
            const float w = std::max(hud::TEMPS_L * k, 2 * m + std::max(l1, l2)), h = hud::TEMPS_H * k, x = mg.gauche + gl, y = mg.haut + gt;
            r.uiRect(x, y, w, h, FOND_PANNEAU);
            const float base1 = y + 10 * k + CAPITALES * tp, ty = base1 - BASE * tp;   // haut des chiffres à 10 px du haut
            // WFS-48 : .time-value en italique (font-style: italic du web)
            halo(r, x + m, ty, tp, t, bas ? hex(DANGER, 0.12f) : hex(ACCENT2, 0.07f), ui::GAUCHE, 2.5f * k, true);
            ui::text(r, x + m, ty, tp, t, bas ? hex(DANGER) : hex(TEXTE), ui::GAUCHE, 0, false, true);
            labelSurBase(r, x + m + ui::width(tp, t, 0) + ecart, base1, k, "Temps");
            const float base2 = base1 + 9 * k + CAPITALES * ap;   // 9 px entre les deux lignes de chiffres
            labelSurBase(r, x + m, base2, k, lib2);
            if (ligne) ui::text(r, x + m + largeurLabel(k, lib2) + ecart, base2 - BASE * ap, ap, dist, hex(TEXTE), ui::GAUCHE, 0, false, true);
            const float pb = ligne ? (float)std::clamp(pl.dist / c->duelFinish, 0.0, 1.0) : (float)(1 - (c->nextCp - pl.dist) / CHECK_DIST);
            barre(r, x + m, base2 + 7 * k, w - 2 * m, 6 * k, pb);
            g_surfaceHud = w * h / (k * k);
        }
        // ---------- haut droite : score + combo (.p-score) ----------
        if (modeCourse() != ModeCourse::Poursuite) {   // WFS-119 RG-02 : en poursuite, seul le score de poursuite est affiché
            const float w = 180 * k, h = c->combo > 1 ? 92 * k : 70 * k, x = W - mg.droite - gl - w, y = mg.haut + gt;
            r.uiRect(x, y, w, h, FOND_PANNEAU);   // WFS-135 (ANO-64) : fond opaque, l'enseigne derrière ne se lit plus
            const float xr = x + w - pad;
            label(r, xr, y + 8 * k, k, "Score", ui::DROITE);
            // WFS-48 : .score-value et .combo en italique ; WFS-135 : ombre de 2 px sous les chiffres ; WFS-245 : chiffres 40 (au lieu
            // de 34), plus grands que l'étiquette, sous elle (capitales de l'étiquette de 14 à 28 px, chiffres de 36 à 56 px du haut)
            const float sp = hud::SCORE_PX * k, sb = y + 8 * k + BASE * labelPx(k) + 8 * k + CAPITALES * sp, sy = sb - BASE * sp;
            ui::text(r, xr, sy + 2 * k, sp, ui::nombre(c->score(pl)), vec4{0, 0, 0, 0.6f}, ui::DROITE, 0, false, true);
            ui::text(r, xr, sy, sp, ui::nombre(c->score(pl)), hex(TEXTE), ui::DROITE, 0, false, true);
            g_surfaceHud += w * h / (k * k);
            if (c->combo > 1)   // .combo : cyan, 16 px
                ui::text(r, xr, y + 66 * k, 16 * k, "Combo ×" + std::to_string(c->combo), hex(ACCENT2), ui::DROITE, 0.04f, false, true);
        }
        const int v = g_kmhForce >= 0 ? g_kmhForce : std::abs(Course::kmh(pl.speed));
        const bool rouge = v > 300, arriere = (pl.speed < -1 || pl.reversing) && g_kmhForce < 0;   // WFS-108 : « R » dès le passage en marche arrière
        const int on = (int)std::lround(std::clamp(pl.nitro, 0.0, 1.0) * 10);   // cases de nitro allumées
        // ---------- vue cockpit (WFS-98, RG-06, ANO-24) : vitesse sous le temps ----------
        // le cadran du tableau de bord 3D donne déjà la vitesse : pas de compteur en arc ; chiffres 28 px et cases de nitro
        // dans un petit panneau sous le temps, sur la rangée du bouton « Menu » (à sa droite, bord droit sur celui du temps)
        // WFS-245 : chiffres 40 (au lieu de 28) et étiquette 27, marges de 10 px, même hauteur de 40 px (rangée du bouton Menu)
        if (etatCockpit().actif) {
            const std::string lib = arriere ? "R · km/h" : "km/h";
            const float tp = hud::VITESSE_COCKPIT_PX * k, mp = 10 * k, wd = ui::width(tp, "000", 0), h = 40 * k;
            const float w = mp + wd + 6 * k + largeurLabel(k, lib) + mp;
            const float wMenu = pad + 18 * k + 6 * k + ui::width(14 * k, "MENU", 0.1f) + pad;   // .cam-btn#menuBtnHud (pause.cpp)
            const float x = std::max(mg.gauche + gl + hud::TEMPS_L * k - w, mg.gauche + gl + wMenu + 8 * k), y = mg.haut + gt + 112 * k + 10 * k;
            r.uiRect(x, y, w, h, FOND_PANNEAU);
            const float base = y + 17 * k + 0.5f * CAPITALES * tp, ty = base - BASE * tp;   // milieu des chiffres à 17 px du haut
            ui::text(r, x + mp + wd, ty, tp, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::DROITE, 0, false, true);
            labelSurBase(r, x + mp + wd + 6 * k, base, k, lib);   // même ligne de base que les chiffres
            const float gap = 2 * k, cw = (w - 2 * mp - 9 * gap) / 10, ch = 4 * k, cy = y + 31 * k;
            for (int i = 0; i < 10; i++)
                r.uiRect(x + mp + i * (cw + gap), cy, cw, ch, i < on ? hex(SUCCESS, pl.nitroOn ? 1.f : 0.85f) : hex(SURFACE2));
            g_surfaceHud += w * h / (k * k);
            if (arriere) pastilleArriere(r, x + w + 10 * k, y, h, k);   // WFS-296 : à droite du panneau de vitesse, même hauteur
            return;
        }
        // ---------- bas centre : vitesse + nitro (entre le volant et le frein) ----------
        {
            // centré dans la zone sûre (entre les marges gauche et droite)
            // WFS-46 : compteur en arc (Ø 190 px ≈ 26 % de la hauteur) ; lecture au centre ; nitro dans l'ouverture du bas
            const float cx = (mg.gauche + W - mg.droite) / 2, R = 95 * k, ccy = H - mg.bas - gt - R - 4 * k;
            // WFS-245 : disque à 0,72 (au lieu de 0,55) ; KM/H et NITRO (27, #C4C9D6) centrés sous les chiffres : capitales de
            // + 8 à + 22 px et de + 29 à + 43 px du centre, tout le texte à moins de 58 px du centre (graduations dès 60 px)
            r.uiCircle(cx, ccy, R - 2 * k, {GLASS.x, GLASS.y, GLASS.z, hud::DISQUE_ALPHA});   // fond pour la lisibilité
            compteur(r, cx, ccy, k, v);
            ui::text(r, cx, ccy - 30 * k, 44 * k, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::CENTRE, 0, false, true);   // .speedo-read (WFS-48)
            const vec4 clair = hex(ETIQUETTE_ARC);
            label(r, cx, ccy + 8 * k - (BASE - CAPITALES) * labelPx(k), k, arriere ? "R · km/h" : "km/h", ui::CENTRE, &clair);
            if (arriere) {   // WFS-296 : au-dessus du compteur, centrée (sur la route, rien d'autre du HUD à cet endroit)
                const float hp = 64 * k, wp = hp * 0.95f + hp * 0.176f;
                pastilleArriere(r, cx - wp * 0.5f, ccy - R - 10 * k - hp, hp, k);
            }
            // .nitro : libellé + 10 cases (round(nitro × 10) allumées, vert --success ; plus lumineuses en nitro active)
            const vec4 lumineux = hex(SUCCESS);
            const float w = 112 * k, x = cx - w / 2, pad = 0;
            label(r, cx, ccy + 29 * k - (BASE - CAPITALES) * labelPx(k), k, "Nitro", ui::CENTRE, pl.nitroOn ? &lumineux : &clair);
            const float gap = 2 * k, cw = (w - 2 * pad - 9 * gap) / 10, ch = 9 * k, cy = ccy + 64 * k;
            for (int i = 0; i < 10; i++) {
                const float xi = x + pad + i * (cw + gap);
                if (i < on && pl.nitroOn) r.uiRect(xi - 2 * k, cy - 2 * k, cw + 4 * k, ch + 4 * k, hex(SUCCESS, 0.35f));   // halo
                r.uiRect(xi, cy, cw, ch, i < on ? hex(SUCCESS, pl.nitroOn ? 1.f : 0.85f) : hex(SURFACE2));
            }
        }
    }
};

} // namespace

WFS_SYSTEM(Hud, 420)   // interface (400-499) : après le texte (400), avant le fondu du chargement (490)

// banc d'essai : course en pilote automatique, puis temps bas, nitro active (touchers gaz + nitro), combo ×2
void benchHud(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("HUD : chargement impossible"); return; }
    const float W = (float)r.width(), H = (float)r.height();
    // WFS-98 : appuis au centre des commandes dessinées (pédale de gaz, bouton nitro), sans marges
    const DispositionCommandes dc = dispositionCommandes(W, H, ecrans::margesCommandes(Marges{}));
    const float gx = ecrans::miroirX(dc.gaz.cx(), W), gy = dc.gaz.cy(), nx = ecrans::miroirX(dc.nitro.cx(), W), ny = dc.nitro.cy();
    double t = 600;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t); g.render(t + 0.01);
        r.capturePng(out + "/" + nom + ".png");
        const Course* c = activeCourse();
        WFS_LOG("capture %s : temps %.1f s, checkpoint %d, score %lld, combo %d, %.0f km/h, nitro %.2f%s", nom, c ? c->timeLeft : 0,
                c ? c->level + 1 : 0, c ? c->score(g.player()) : 0, c ? c->combo : 0, g.player().kmh(), g.player().nitro,
                g.player().nitroOn ? " (active)" : "");
    };
    g.setAutopilot(true); g.camMode = 0;
    rouler(60 * 9);                       // décompte (3 s) puis 6 s de course
    capture("hud-course");
    courseBenchSetTime(6.0);              // temps bas : rouge sous 8 s
    rouler(30);
    capture("hud-temps-bas");
    courseBenchSetTime(30.0);
    // nitro : le pilote automatique ne l'utilise pas ; on appuie sur gaz + nitro comme un joueur
    g.setAutopilot(false);
    g.onTouch({7, gx, gy, true}); g.onTouch({8, nx, ny, true});
    rouler(24);
    capture("hud-nitro");
    g.onTouch({8, nx, ny, false});
    // combo ×2 : deux quasi-contacts de suite (événements de règles, comme ceux du trafic)
    if (Course* c = activeCourseMut()) { c->nearMiss(); c->nearMiss(); }
    rouler(12);
    capture("hud-combo");
    // WFS-46 : compteur en arc à 120 et 320 km/h (zone rouge)
    g_kmhForce = 120; rouler(6); capture("hud-arc-120");
    g_kmhForce = 320; rouler(6); capture("hud-arc-320");
    g_kmhForce = -1;
    // WFS-52 : encoche simulée de 90 px à gauche (WFS_ENCOCHE, sinon 90) : bande marquée, aucun panneau du HUD dedans
    {
        const char* e = std::getenv("WFS_ENCOCHE");
        g.setMarges({e ? (float)std::atoi(e) : 90.f, 0, 0, 0}); g.montrerMarges = true;
        rouler(12);
        capture("encoche-hud");
        g.setMarges({}); g.montrerMarges = false;
    }
    g.onTouch({7, gx, gy, false});
    // WFS-245 (RG-05) : HUD lisible à la taille du téléphone : Course solo (Checkpoint) et Course à 5, jour et nuit, vue normale et
    // Cockpit ; écrits : surface des panneaux (RG-04), hauteur des capitales des étiquettes et des chiffres d'ARRIVÉE (RG-01)
    {
        const VueCamera vueAvant = vueCamera();
        const ModeCourse modeAvant = modeCourse();
        const int kmAvant = kmCourse5();
        auto scene = [&](const char* nom, ModeCourse mode, bool nuit, bool cockpit) {
            modeCourse() = mode; kmCourse5() = 2;
            if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
            if (!g.init(r, "golf", "ville")) { WFS_ERR("HUD (WFS-245) : chargement impossible (%s)", nom); return; }
            choisirVue(cockpit ? VueCamera::Cockpit : VueCamera::Poursuite);
            g.setAutopilot(true); g.camMode = 0;
            rouler(60 * 9);   // décompte (3 s) puis 6 s de course (comme hud-course)
            capture(nom);
            const float k = H / 720.f, lp = labelPx(k);
            // surface d'avant (calculée avec les tailles d'avant) : temps 200 × 112, score 180 × 70 ; vue Cockpit : panneau de la
            // vitesse en chiffres de 28 et étiquette de max(13, 11 × k), marges de 14
            const float ancienKmh = (14 * k + ui::width(28 * k, "000", 0) + 6 * k + ui::width(std::round(std::max(13.f, 11 * k)), "KM/H", 0.16f) + 14 * k) / k;
            const Course* cc = activeCourse();
            const float avant = hud::AVANT_TEMPS_L * hud::AVANT_TEMPS_H + 180 * (cc && cc->combo > 1 ? 92 : 70) + (cockpit ? ancienKmh * 40 : 0);
            WFS_LOG("capture %s (WFS-245) : panneaux du HUD %.0f px² (px de référence, k = %.2f ; avant : %.0f px²) ; capitales des étiquettes %.1f px "
                    "(%.2f mm à 420 dpi), chiffres d'ARRIVÉE %.1f px (%.2f mm)", nom, g_surfaceHud, k, avant, CAPITALES * lp,
                    CAPITALES * lp / (420.f / 25.4f), CAPITALES * std::round(ARRIVEE_PX * k), CAPITALES * std::round(ARRIVEE_PX * k) / (420.f / 25.4f));
            if (g_surfaceHud > avant + 1) WFS_ERR("HUD (WFS-245, RG-04) : %s : panneaux %.0f px² pour %.0f avant", nom, g_surfaceHud, avant);
        };
        scene("hud-solo-jour", ModeCourse::Checkpoint, false, false);
        scene("hud-solo-nuit", ModeCourse::Checkpoint, true, false);
        scene("hud-course5-jour", ModeCourse::Course5, false, false);
        scene("hud-course5-nuit", ModeCourse::Course5, true, false);
        scene("hud-cockpit-jour", ModeCourse::Course5, false, true);
        scene("hud-cockpit-nuit", ModeCourse::Checkpoint, true, true);
        // WFS-296 (RG-03) : marche arrière : à l'arrêt après le décompte, frein maintenu jusqu'à reculer (comme le joueur) ;
        // vue normale et Cockpit, jour et nuit (la marche avant : hud-solo-jour / -nuit, hud-cockpit-jour / -nuit ci-dessus)
        const float fx = ecrans::miroirX(dc.frein.cx(), W), fy = dc.frein.cy();
        auto arriere = [&](const char* nom, bool nuit, bool cockpit) {
            modeCourse() = ModeCourse::Checkpoint;
            if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
            if (!g.init(r, "golf", "ville")) { WFS_ERR("HUD (WFS-296) : chargement impossible (%s)", nom); return; }
            choisirVue(cockpit ? VueCamera::Cockpit : VueCamera::Poursuite);
            g.setAutopilot(false); g.camMode = 0;
            rouler(60 * 4);   // décompte (3 s), voiture arrêtée
            g.onTouch({9, fx, fy, true});
            rouler(60 * 2);   // frein maintenu : marche arrière
            capture(nom);
            WFS_LOG("capture %s (WFS-296) : marche arrière %s, %.1f km/h", nom, g.player().reversing || g.player().speed < -1 ? "oui" : "NON",
                    g.player().kmh());
            if (!(g.player().reversing || g.player().speed < -1)) WFS_ERR("HUD (WFS-296) : %s : la voiture ne recule pas", nom);
            g.onTouch({9, fx, fy, false});
        };
        arriere("hud-arriere-jour", false, false);
        arriere("hud-arriere-nuit", true, false);
        arriere("hud-arriere-cockpit-jour", false, true);
        arriere("hud-arriere-cockpit-nuit", true, true);
        unsetenv("WFS_MOMENT");
        modeCourse() = modeAvant; kmCourse5() = kmAvant; choisirVue(vueAvant);
    }
    g.shutdown();
    courseBenchRules(false);
    benchToasts(g, r, out);   // WFS-47 : bandeaux (toast-checkpoint, toast-pile)
    benchMiniCarte(g, r, out);   // WFS-173 : minicarte-checkpoint, minicarte-ligne
}

} // namespace wfs
