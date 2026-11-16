// WaForSpeed — HUD de course (WFS-27) : voir hud.h.
#include "hud.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
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
#include "jauges.h"   // WFS-550 : jauge du combo
#include "../core/configgame.h"   // WFS-550 : durée du combo
#include "toasts.h"
#include "boutons.h"   // WFS-557 : contour arrondi des panneaux de verre
#include "../game/textes_course.h"   // WFS-506 : textes en français ou en anglais

namespace wfs {
namespace {

// jetons :root du jeu web (codes sRGB, comme ui::CYAN)
vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, DANGER = 0xFF1F4B, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF,
               SURFACE2 = 0x1E2130, SUCCESS = 0xB6FF00;
const unsigned DECIMALE = 0xFFA066, SEGMENT = 0x474A53;   // WFS-557 (maquette 17) : dixièmes et combo orange clair, segments éteints
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};   // --glass rgba(20,22,31,.72)
// WFS-245 : tailles, couleurs et contrastes des textes du HUD (hud.h, namespace hud)
using hud::CAPITALES; using hud::BASE; using hud::INTER_BASE; using hud::ETIQUETTE_PX; using hud::ARRIVEE_PX; using hud::ETIQUETTE; using hud::ETIQUETTE_ARC;
const vec4 FOND_PANNEAU = hex(hud::FOND, hud::FOND_ALPHA);   // 0x0A0B10 à 0,85 (WFS-135 : celui du score)

// WFS-557 (maquette 17) : panneau de verre sombre aux coins arrondis de 14, liseré clair d'1,2 et ombre douce
void panneauVerre(Renderer& r, float x, float y, float w, float h, float k) {
    vec2 p[ui::boutons::MAX_SOMMETS];
    const int n = ui::boutons::contourArrondi(p, x, y, w, h, 14 * k);
    dessin2d::halo(r, p, n, 10 * k, {0, 0, 0, 0.22f});
    dessin2d::polygone(r, p, n, {hex(0x23262E, 0.84f), hex(0x15171C, 0.88f), y, y + h});
    dessin2d::contour(r, p, n, 1.2f * k, {1, 1, 1, 0.16f});
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
// banc d'essai (WFS-46) : vitesse affichée forcée (captures hud-arc-120 / hud-arc-320), −1 = vitesse réelle
int g_kmhForce = -1;
float g_surfaceHud = 0;   // WFS-245 (RG-04) : surface des panneaux de la dernière image, px de référence (720 p) au carré

// WFS-557 (maquette 17) : arc de la vitesse sans piste ni graduations : départ en bas à gauche (225°, angles comptés depuis le
// haut, sens horaire), 270° pour VITESSE_MAX ; trait de 5,5, cyan sur la gauche, orange au bout (dernier quart du trait),
// point orange de 5 au bout avec un halo ; le départ s'éteint en orange pâle
constexpr float VITESSE_MAX = 250;
void compteur(Renderer& r, float cx, float cy, float k, int kmh) {
    const float R = 82 * k, ep = 5.5f * k, a0 = 225 * PI / 180, span = 1.5f * PI;
    const float pct = std::clamp(kmh / VITESSE_MAX, 0.f, 1.f);
    if (pct <= 0.002f) return;
    const int N = std::max(2, (int)std::lround(pct * 90));
    const vec4 cyan = hex(ACCENT2), orange = hex(0xF59A45), depart = hex(0xE0A040, 0.55f);
    auto melange = [](vec4 a, vec4 b, float t) { return vec4{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t}; };
    for (int i = 0; i < N; i++) {
        const float t0 = (float)i / N, t1 = (float)(i + 1) / N, tm = (t0 + t1) / 2;
        vec4 c = tm < 0.12f ? melange(depart, cyan, tm / 0.12f) : (tm < 0.62f ? cyan : melange(cyan, orange, (tm - 0.62f) / 0.38f));
        const float a = a0 + span * pct * t0, b = a0 + span * pct * t1 + 0.004f;
        dessin2d::arc(r, cx, cy, R, ep * 2.2f, a, b, {c.x, c.y, c.z, c.w * 0.14f});   // halo
        dessin2d::arc(r, cx, cy, R, ep, a, b, c);
    }
    const float ab = a0 + span * pct, px = cx + std::sin(ab) * R, py = cy - std::cos(ab) * R;
    dessin2d::couronne(r, px, py, 0, 11 * k, hex(0xFF8A3D, 0.45f), hex(0xFF8A3D, 0));
    r.uiCircle(px, py, 5 * k, hex(0xFFA060));
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
        // WFS-52 : zone sûre (encoche, coins arrondis) = marges de view.marges (WFS-51) + gouttières de WFS-27 ;
        // marges à 0 : positions de WFS-27 inchangées
        const Marges& mg = g.marges;
        g_surfaceHud = 0;

        // ---------- haut gauche : temps + checkpoint ----------
        // WFS-557 (ordre de l'utilisateur du 2026-11-16 : maquettes R19 à 100 %, maquette 17) : panneau de verre arrondi 276 × 80 en
        // (23, 19) ; chrono « 0:57 » blanc et « ,9 » orange (Rajdhani gras italique 50), « TEMPS » (Inter Medium 11, espacé) sur
        // sa ligne de base ; 6 segments (25 × 4, pas de 30) : passés en blanc, en cours en orange, à venir en gris ; « CP n »
        // (ou la distance restante en course à une ligne d'arrivée) en Inter Medium 10 calé à droite
        {
            const bool ligne = c->duelFinish > 0;   // WFS-110 : course à une ligne d'arrivée (course à 5, duel) : chrono et distance
            const bool bas = !ligne && c->timeLeft < 8;
            const std::string t = ui::chrono(ligne ? c->raceTime : std::max(0.0, c->timeLeft));
            const size_t virgule = t.find_last_of(",.");
            const std::string tEntier = virgule == std::string::npos ? t : t.substr(0, virgule), tDec = virgule == std::string::npos ? "" : t.substr(virgule);
            static std::string tamponCp;   // WFS-506 (RG-04) : tampon réutilisé
            char n[16]; std::snprintf(n, sizeof n, "%d", c->level + 1);
            const std::string lib2 = ligne ? ui::majuscules(ui::decimal(std::max(0.0, c->duelFinish - pl.dist) * M / 1000, 2) + " km")
                                           : ui::majuscules(textes::remplir(tamponCp, "hud.cp", n));
            const float x = mg.gauche + hud::R19_GOUTTIERE_L * k, y = mg.haut + hud::R19_GOUTTIERE_H * k, w = hud::TEMPS_L * k, h = hud::TEMPS_H * k;
            panneauVerre(r, x, y, w, h, k);
            const float tp = hud::CHRONO_PX * k, base1 = y + 41 * k, ty = base1 - BASE * tp, xt = x + 20 * k;
            ui::text(r, xt, ty, tp, tEntier, bas ? hex(DANGER) : hex(TEXTE), ui::GAUCHE, 0, false, true);
            const float xd = xt + ui::width(tp, tEntier, 0);
            ui::text(r, xd, ty, tp, tDec, bas ? hex(DANGER) : hex(DECIMALE), ui::GAUCHE, 0, false, true);
            const float ep = hud::R19_ETIQUETTE_PX * k;
            {
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::text(r, xd + ui::width(tp, tDec, 0) + 12 * k, base1 - INTER_BASE * ep, ep, ui::majuscules(ui::tr("hud.temps")), hex(ETIQUETTE), ui::GAUCHE, 0.2f, false);
                const float cp = hud::R19_CP_PX * k;
                ui::text(r, x + w - 19 * k, y + 66.3f * k - INTER_BASE * cp, cp, lib2, hex(ETIQUETTE), ui::DROITE, 0.2f, false);
            }
            // segments : 6 par bloc ; checkpoint : n° du checkpoint dans le bloc ; ligne d'arrivée : avancement en sixièmes
            const float pb = ligne ? (float)std::clamp(pl.dist / c->duelFinish, 0.0, 1.0) : (float)std::clamp(1 - (c->nextCp - pl.dist) / CHECK_DIST, 0.0, 1.0);
            const int enCours = ligne ? std::min(5, (int)(pb * 6)) : c->level % 6;
            const float sx = x + 17 * k, sy = y + 61 * k, sw = 25.3f * k, sh = 4 * k, pas = 30 * k;
            for (int i = 0; i < 6; i++) {
                const float xi = sx + i * pas;
                r.uiRect(xi, sy, sw, sh, i < enCours ? hex(TEXTE) : hex(SEGMENT));
                if (i == enCours) r.uiRect(xi, sy, sw * (ligne ? pb * 6 - enCours : std::max(pb, 0.15f)), sh, hex(ACCENT));
            }
            g_surfaceHud = w * h / (k * k);
        }
        // ---------- haut droite : score + combo ----------
        if (modeCourse() != ModeCourse::Poursuite) {   // WFS-119 RG-02 : en poursuite, seul le score de poursuite est affiché
            // WFS-557 (maquette 17) : verre arrondi 166 × 88 à 23 / 19 du coin (64 de haut sans combo) ; « SCORE » Inter Medium 10
            // espacé, chiffres Rajdhani gras italique 48 calés à droite ; « COMBO ×3 » orange (Rajdhani gras italique 18) et barre
            // orange du temps restant du combo (59 × 3) sur la même ligne
            const bool combo = c->combo > 1;
            const float w = hud::SCORE_R19_L * k, h = (combo ? hud::SCORE_R19_H_COMBO : hud::SCORE_R19_H) * k;
            const float x = W - mg.droite - hud::R19_GOUTTIERE_L * k - w, y = mg.haut + hud::R19_GOUTTIERE_H * k;
            panneauVerre(r, x, y, w, h, k);
            const float xr = x + w - 18 * k, ep = hud::R19_CP_PX * k;
            {
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::text(r, xr, y + 17.7f * k - INTER_BASE * ep, ep, ui::majuscules(ui::tr("hud.score")), hex(ETIQUETTE), ui::DROITE, 0.22f, false);
            }
            const float sp = hud::SCORE_PX * k, sb = y + 51.7f * k;
            ui::text(r, xr + 2 * k, sb - BASE * sp, sp, ui::nombre(c->score(pl)), hex(TEXTE), ui::DROITE, 0, false, true);
            g_surfaceHud += w * h / (k * k);
            if (combo) {
                static std::string tamponCombo;   // WFS-506 (RG-04)
                char n[16]; std::snprintf(n, sizeof n, "%d", c->combo);
                const float jw = 58.7f * k, dc = (float)config().score.comboDuree, bx = x + w - 18 * k - jw;
                r.uiRect(bx, y + 68.3f * k, jw, 3.3f * k, hex(SEGMENT));
                jauges::barreUnie(r, bx, y + 68.3f * k, jw, 3.3f * k, dc > 0 ? (float)(c->comboTimer / dc) : 0, hex(ACCENT));
                const float cpx = 18.5f * k;
                ui::text(r, x + 18 * k, y + 74.3f * k - BASE * cpx, cpx, ui::majuscules(textes::remplir(tamponCombo, "hud.combo", n)), hex(DECIMALE), ui::GAUCHE, 0.02f, false, true);
            }
        }
        const int v = g_kmhForce >= 0 ? g_kmhForce : std::abs(Course::kmh(pl.speed));
        const bool rouge = v > 300, arriere = ((pl.speed < -1 && pl.reculVoulu) || pl.reversing) && g_kmhForce < 0;   // WFS-108 ; WFS-540 (RG-05) : jamais après un choc
        const int on = (int)std::lround(std::clamp(pl.nitro, 0.0, 1.0) * 10);   // cases de nitro allumées
        // ---------- vue cockpit (WFS-98, RG-06, ANO-24) : vitesse sous le temps ----------
        // le cadran du tableau de bord 3D donne déjà la vitesse : pas de compteur en arc ; chiffres 28 px et cases de nitro
        // dans un petit panneau sous le temps (bord droit sur celui du temps) ; WFS-429 (RG-05) : le bouton « Menu » est parti
        // à côté du score, le panneau ne se cale plus à sa droite
        // WFS-245 : chiffres 40 (au lieu de 28) et étiquette 27, marges de 10 px, hauteur de 40 px
        if (etatCockpit().actif) {
            const std::string& lib = ui::tr(arriere ? "hud.kmh_arriere" : "hud.kmh");
            const float tp = hud::VITESSE_COCKPIT_PX * k, mp = 10 * k, wd = ui::width(tp, "000", 0), h = 40 * k;
            const float w = mp + wd + 6 * k + largeurLabel(k, lib) + mp;
            const float x = mg.gauche + hud::R19_GOUTTIERE_L * k + hud::TEMPS_L * k - w, y = mg.haut + hud::R19_GOUTTIERE_H * k + hud::TEMPS_H * k + 10 * k;
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
        // ---------- bas centre : compteur ----------
        // WFS-557 (maquette 17) : disque de verre sombre Ø 174 centré à 90 du bas (il sort un peu de l'écran), liseré gris ; arc de
        // la vitesse sur la gauche (départ à 225°, 270° pour VITESSE_MAX), cyan puis orange vers le bout, point orange au bout ;
        // vitesse Rajdhani gras italique 75, « KM/H » Inter Medium 10 espacé, « RAPPORT 3 » (Rajdhani gras italique 16,5 et 24)
        {
            const float cx = (mg.gauche + W - mg.droite) / 2, R = 87 * k, ccy = H - mg.bas - 90 * k;
            dessin2d::couronne(r, cx, ccy, R - 2 * k, R + 10 * k, {0, 0, 0, 0.28f}, {0, 0, 0, 0});
            dessin2d::disqueRadial(r, cx, ccy, R, cx, ccy - 30 * k, R * 1.2f, hex(0x3A3B3E, 0.82f), hex(0x1C1D20, 0.88f));
            dessin2d::anneau(r, cx, ccy, R - 2.5f * k, R, hex(0x8E9096, 0.55f));
            compteur(r, cx, ccy, k, v);
            const float vp = 75 * k;
            ui::text(r, cx + 5 * k, ccy - 2 * k - BASE * vp, vp, std::to_string(v), rouge ? hex(DANGER) : hex(TEXTE), ui::CENTRE, 0, false, true);
            const float ep = hud::R19_CP_PX * k;
            {
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::text(r, cx, ccy + 21 * k - INTER_BASE * ep, ep, ui::majuscules(ui::tr(arriere ? "hud.kmh_arriere" : "hud.kmh")), hex(ETIQUETTE_ARC), ui::CENTRE, 0.3f, false);
            }
            // rapport engagé, le même que celui du son du moteur (audio_system.cpp) : boîte de la conduite du monde, sinon la boîte à
            // 5 rapports de l'ancienne conduite (EngineModel::update : seuils 16 / 34 / 54 / 76 % de la vitesse de pointe) ; « R » en
            // marche arrière, « N » à l'arrêt
            int rp = pl.monde.rapport;
            if (!pl.modeleMonde()) {
                static constexpr float SEUILS[5] = {0, 0.16f, 0.34f, 0.54f, 0.76f};
                const double top = MAX_SPEED * pl.perf.top * (pl.nitroOn ? pl.perf.nitroMult : 1);
                const float vr = (float)std::min(1.25, std::abs(pl.speed) / std::max(top, 1.0));
                rp = 1; while (rp < 5 && vr >= SEUILS[rp]) rp++;
            }
            const std::string rap = arriere || rp < 0 ? "R" : (v < 1 ? "N" : std::to_string(std::max(1, rp)));
            const std::string libR = ui::majuscules(ui::tr("hud.rapport"));
            const float lp = 16.5f * k, cp2 = 24 * k, wl = ui::width(lp, libR, 0.04f), wc = ui::width(cp2, rap, 0), ecart = 6 * k;
            const float x0 = cx - (wl + ecart + wc) / 2, bR = ccy + 50 * k;
            ui::text(r, x0, bR - BASE * lp, lp, libR, hex(0xC6C9D2), ui::GAUCHE, 0.04f, false, true);
            ui::text(r, x0 + wl + ecart, bR - BASE * cp2, cp2, rap, hex(TEXTE), ui::GAUCHE, 0, false, true);
            if (arriere) {   // WFS-296 : à gauche du compteur, à mi-hauteur
                const float hp = 64 * k, wp = hp * 0.95f + hp * 0.176f;
                pastilleArriere(r, cx - R - 14 * k - wp, ccy - hp * 0.5f, hp, k);
            }
        }
    }
};

} // namespace

WFS_SYSTEM(Hud, 420)   // interface (400-499) : après le texte (400), avant le fondu du chargement (490)

// WFS-429 : une seule source pour la place et la taille des boutons « Menu » et « Caméra » (dessin et zones de toucher)
void hud::rangeeBoutons(float W, float H, const Marges& m, const std::string& libelleCamera, BoutonHud& menu, BoutonHud& camera) {
    const float k = H / 720.f, tp = BOUTON_TEXTE * k * BOUTON_ECHELLE;
    // WFS-506 (RC-E20-04) : largeur mesurée sur le texte français (« Menu » : même mot en anglais), bouton identique dans les deux langues
    menu = mesuresBouton(k, ui::width(tp, ui::majuscules(ui::trLangue("hud.menu", ui::langue::Langue::Francais)), 0.1f));
    camera = mesuresBouton(k, ui::width(tp, libelleCamera, 0.1f));
    placerBoutons(W, H, m, menu, camera, modeCourse() == ModeCourse::Poursuite);   // WFS-551 (RG-06) : poursuite à part
}

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
