// WaForSpeed — écran de réglages (WFS-68 dans WFS-41) : voir reglages.h.
#include "reglages.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../core/log.h"
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/camera_vue.h"
#include "../../game/game.h"
#include "../../game/sauvegarde.h"   // WFS-24 : réglages dans la sauvegarde commune
#include "../../render/renderer.h"
#include "../../render/vk.h"
#include "../text.h"

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// jetons :root du jeu web (codes sRGB)
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f}, SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.96f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    const int N = 16;
    for (int i = 0; i < N; i++) {
        const float t = (i + 0.5f) / N;
        r.uiRect(x + biais * (1 - t), y + h * i / N, w - biais, h / N + 0.5f, c);
    }
}

// lignes du panneau (index.html, #pause .settings ; « Graphismes » en 3 niveaux, « Direction » remplacée plus tard
// par la disposition des commandes)
struct Ligne { const char* libelle; int n; const char* choix[3]; };
const Ligne LIGNES[4] = {
    {"Caméra", 3, {VUES[0].nom, VUES[1].nom, VUES[2].nom}},   // vues de WFS-66 (CAM_MODES du web)
    {"Graphismes", 3, {"Basse", "Moyenne", "Haute"}},
    {"Son", 2, {"Activé", "Coupé", nullptr}},
    {"Musique", 2, {"Activée", "Coupée", nullptr}},   // WFS-76 : musique seule (l'ambiance reste)
};
enum { L_CAMERA = 0, L_QUALITE = 1, L_SON = 2, L_MUSIQUE = 3 };

Zone g_bouton, g_fermer, g_choix[4][3];
bool g_ouvert = false;

// ---------------------------------------------------------------- sauvegarde simple (camera=, qualite=, son=)
std::string fichier() {
#ifdef __ANDROID__
    return "/data/data/com.waforspeed.game/files/reglages.txt";   // dossier interne de l'application (getFilesDir)
#else
    const char* p = std::getenv("WFS_REGLAGES");   // banc d'essai : aucune sauvegarde sans WFS_REGLAGES (captures stables)
    return p ? p : "";
#endif
}
bool lire() {
    if (Sauvegarde& s = sauvegarde(); s.actif()) {   // WFS-24 : sauvegarde commune (cam, quality, muted)
        Reglages& R = reglages(); R.camera = s.camIndex(); R.son = !s.muted; R.musique = s.music;
        if (s.qualityChoisie) { R.qualite = s.qualiteIndex(); R.qualiteChoisie = true; }
        return true;
    }
    const std::string f = fichier();
    FILE* fp = f.empty() ? nullptr : std::fopen(f.c_str(), "r");
    if (!fp) return false;
    Reglages& R = reglages();
    char cle[32]; int v;
    while (std::fscanf(fp, " %31[^=]=%d", cle, &v) == 2) {
        if (!std::strcmp(cle, "camera")) R.camera = std::clamp(v, 0, 2);
        else if (!std::strcmp(cle, "qualite")) { R.qualite = std::clamp(v, 0, 2); R.qualiteChoisie = true; }
        else if (!std::strcmp(cle, "son")) R.son = v != 0;
        else if (!std::strcmp(cle, "musique")) R.musique = v != 0;   // WFS-76
    }
    std::fclose(fp);
    WFS_LOG("réglages lus : caméra %d, qualité %d, son %d", R.camera, R.qualite, (int)R.son);
    return true;
}
void ecrire() {
    if (Sauvegarde& s = sauvegarde(); s.actif()) {   // WFS-24 : écriture atomique sur le thread de travail
        const Reglages& R = reglages(); s.setCamIndex(R.camera); s.muted = !R.son; s.music = R.musique;
        if (R.qualiteChoisie) s.setQualiteIndex(R.qualite);
        s.enregistrer(); return;
    }
    const std::string f = fichier();
    FILE* fp = f.empty() ? nullptr : std::fopen(f.c_str(), "w");
    if (!fp) return;
    const Reglages& R = reglages();
    std::fprintf(fp, "camera=%d\nqualite=%d\nson=%d\nmusique=%d\n", R.camera, R.qualite, (int)R.son, (int)R.musique);
    std::fclose(fp);
}

// ---------------------------------------------------------------- application immédiate
void appliquerCamera() { choisirVue((VueCamera)reglages().camera); }   // WFS-66 : vue mémorisée pour la session
// qualité : échelle de la scène 3D (cibles recréées) et ombres du soleil ; jamais en mode GPU logiciel (WFS-59)
void appliquerQualite(Renderer& r) {
    if (r.gpuLogiciel()) return;
    const int q = reglages().qualite;
    r.vk().renderScale = q == QUALITE_BASSE ? 0.5f : q == QUALITE_MOYENNE ? 0.75f : 1.f;
    r.forceNoShadows = q == QUALITE_BASSE;
    r.bloomForce = q == QUALITE_BASSE ? 0.f : 1.f;   // WFS-15 : bloom en Moyenne et Haute
    r.qualiteBasse = q == QUALITE_BASSE;               // WFS-82 : petits objets coupés à 150 m
    r.resize();
    WFS_LOG("réglages : qualité %s (scène à %.0f %%, ombres %s)", LIGNES[L_QUALITE].choix[q], r.vk().renderScale * 100,
            r.forceNoShadows ? "coupées" : "actives");
}
int valeur(int ligne) {
    const Reglages& R = reglages();
    return ligne == L_CAMERA ? R.camera : ligne == L_QUALITE ? R.qualite : ligne == L_SON ? (R.son ? 0 : 1) : (R.musique ? 0 : 1);
}

class EcranReglages : public System {
public:
    const char* name() const override { return "réglages"; }
    void init(Renderer& r, const GameView&) override {
        r_ = &r; g_ouvert = false;
        static bool lu = false;   // une seule lecture par lancement (Game::init est rappelé à chaque changement de carte)
        if (lu) return;
        lu = true;
        if (!lire()) return;
        if (reglages().camera != CAM_POURSUITE) appliquerCamera();
        if (reglages().qualite != QUALITE_HAUTE) appliquerQualite(r);
    }
    void onEcran(Ecran, Ecran, const GameView&) override { g_ouvert = false; g_bouton = {}; }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Menu && g.ecran != Ecran::Pause) return false;
        if (!g_ouvert) {   // bouton « Réglages » (RG-04 : menu principal et pause)
            if (t.down && g_bouton.contient(t.x, t.y)) { g_ouvert = true; doigt_ = t.id; WFS_LOG("réglages : ouverts (%s)", nomEcran(g.ecran)); return true; }
            return t.id == doigt_ && !t.down ? (doigt_ = -1, true) : false;
        }
        if (!t.down) { if (t.id == doigt_) doigt_ = -1; return true; }
        if (t.id == doigt_) return true;
        if (g_fermer.contient(t.x, t.y)) { g_ouvert = false; doigt_ = t.id; WFS_LOG("réglages : fermés"); return true; }
        for (int l = 0; l < 4; l++) for (int c = 0; c < LIGNES[l].n; c++) {
            if (!g_choix[l][c].contient(t.x, t.y) || valeur(l) == c) continue;
            Reglages& R = reglages();
            if (l == L_CAMERA) { R.camera = c; appliquerCamera(); }
            else if (l == L_QUALITE) { R.qualite = c; R.qualiteChoisie = true; if (r_) appliquerQualite(*r_); }
            else if (l == L_SON) R.son = c == 0; else R.musique = c == 0;
            WFS_LOG("réglages : %s = %s", LIGNES[l].libelle, LIGNES[l].choix[c]);
            ecrire();
        }
        return true;   // RG-03 : le panneau garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.sceneDediee || (g.ecran != Ecran::Menu && g.ecran != Ecran::Pause)) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        if (!g_ouvert) dessinerBouton(r, g, W, k);
        else dessinerPanneau(r, g, W, H, k);
    }

private:
    // .cam-btn : vitré, bordure 1 px, biais 8 px, 44 px de haut, « RÉGLAGES » ; en haut à droite de la zone sûre
    void dessinerBouton(Renderer& r, const GameView& g, float W, float k) {
        const float h = 44 * k, tp = 15 * k, pad = 18 * k;
        const std::string t = ui::majuscules("Réglages");
        const float w = pad + ui::width(tp, t, 0.12f) + pad, x = W - g.marges.droite - 16 * k - w, y = g.marges.haut + 14 * k;
        g_bouton = {x, y, w, h};
        biseau(r, x - k, y - k, w + 2 * k, h + 2 * k, 8 * k, BORD);
        biseau(r, x, y, w, h, 8 * k, GLASS);
        ui::text(r, std::round(x + pad), std::round(y + (h - tp) / 2), tp, t, hex(ACCENT2), ui::GAUCHE, 0.12f, false);
    }
    // section .settings : voile, .eyebrow, .big « Réglages », lignes .set-row (libellé + boutons .seg), « Fermer »
    void dessinerPanneau(Renderer& r, const GameView& g, float W, float H, float k) {
        r.uiRect(0, 0, W, H, hex(BG, 0.86f));
        const Marges& m = g.marges;
        const float zl = m.gauche, zw = W - m.gauche - m.droite;
        const float pw = std::min(zw - 32 * k, 760 * k), px = zl + (zw - pw) / 2, py = m.haut + 40 * k;
        const float rowH = 64 * k, bh = 50 * k, ph = 150 * k + 4 * (rowH + 16 * k) + bh + 30 * k;
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
        const float x0 = px + 36 * k;
        ui::text(r, x0, py + 30 * k, 13 * k, ui::majuscules(g.ecran == Ecran::Pause ? "Course en pause" : "Menu principal"), hex(ACCENT2),
                 ui::GAUCHE, 0.22f, false);
        ui::text(r, x0, py + 52 * k, 56 * k, ui::majuscules("Réglages"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        float y = py + 150 * k;
        const float segW = std::min(170 * k, (pw - 72 * k - 190 * k - 2 * 6 * k) / 3);
        for (int l = 0; l < 4; l++) {
            r.uiRect(x0, y + rowH + 7 * k, pw - 72 * k, 1 * k, hex(TEXTE, 0.08f));   // séparateur
            ui::text(r, x0, y + (rowH - 14 * k) / 2, 14 * k, ui::majuscules(LIGNES[l].libelle), hex(MUTED), ui::GAUCHE, 0.16f, false);
            const int n = LIGNES[l].n;
            float bx = px + pw - 36 * k - n * segW - (n - 1) * 6 * k;
            for (int c = 0; c < n; c++) {
                const bool on = valeur(l) == c;   // aria-pressed : fond cyan léger, bordure et texte cyan
                g_choix[l][c] = {bx, y, segW, rowH};
                if (on) {
                    r.uiRect(bx, y, segW, rowH, hex(ACCENT2, 0.16f));
                    r.uiRect(bx, y + rowH - 3 * k, segW, 3 * k, hex(ACCENT2));
                } else r.uiRect(bx, y, segW, rowH, hex(SURFACE2));
                ui::text(r, bx + segW / 2, y + (rowH - 18 * k) / 2, 18 * k, ui::majuscules(LIGNES[l].choix[c]), on ? hex(ACCENT2) : hex(TEXTE),
                         ui::CENTRE, 0.08f, false);
                bx += segW + 6 * k;
            }
            for (int c = n; c < 3; c++) g_choix[l][c] = {};
            y += rowH + 16 * k;
        }
        // .btn « Fermer » (orange, biais 14 px), à droite
        const float tp = 20 * k;
        const std::string ft = ui::majuscules("Fermer");
        const float fw = ui::width(tp, ft, 0.12f) + 56 * k, fx = px + pw - 36 * k - fw, fy = y + 14 * k;
        g_fermer = {fx, fy, fw, bh};
        biseau(r, fx, fy, fw, bh, 14 * k, hex(ACCENT));
        ui::text(r, fx + fw / 2, fy + (bh - tp) / 2, tp, ft, hex(BG), ui::CENTRE, 0.12f, false);
    }

    Renderer* r_ = nullptr;
    int doigt_ = -1;
};

} // namespace

Reglages& reglages() { static Reglages R; return R; }
bool reglagesOuverts() { return g_ouvert; }
bool toucheReglages(float x, float y) { return g_ouvert || g_bouton.contient(x, y); }

// banc d'essai (WFS-68) : touchers simulés ; reglages-menu, reglages-pause, puis qualité basse en course (reglages-basse)
void benchReglages(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("réglages : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 1000;
    int n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    for (int i = 0; i < 30; i++) image();
    g.allerA(Ecran::Menu);
    for (int i = 0; i < 60; i++) image();
    g.render(t); toucher(1, g_bouton);
    capture("reglages-menu", "panneau ouvert depuis le menu");
    toucher(2, g_choix[L_CAMERA][CAM_CAPOT]); toucher(3, g_fermer);
    WFS_LOG("réglages : vue = %s (Capot attendu)", VUES[(int)vueCamera()].nom);
    // course (départ du menu), puis pause → réglages
    g.allerA(Ecran::Course);
    for (int i = 0; i < 60 * 5; i++) image();
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(4, g_bouton);
    capture("reglages-pause", "panneau ouvert depuis la pause, vue Capot choisie");
    // qualité basse : scène à 50 %, sans ombres du soleil ; capture en course, puis retour à Haute et Proche
    toucher(5, g_choix[L_QUALITE][QUALITE_BASSE]); toucher(6, g_choix[L_CAMERA][CAM_POURSUITE]); toucher(7, g_fermer);
    g.allerA(Ecran::Course);
    for (int i = 0; i < 60; i++) image();
    capture("reglages-basse", "course en qualité basse");
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(8, g_bouton);
    toucher(9, g_choix[L_QUALITE][QUALITE_HAUTE]); toucher(11, g_choix[L_MUSIQUE][1]);   // WFS-76 : « Musique : Coupée »
    capture("reglages-musique", "musique coupée"); toucher(12, g_choix[L_MUSIQUE][0]); toucher(10, g_fermer);
    g.shutdown();
    courseBenchRules(false);
}

void appliquerQualiteNiveau(Renderer& r) { appliquerQualite(r); }   // WFS-15

} // namespace ecrans

WFS_SYSTEM(ecrans::EcranReglages, 460)   // interface (400-499) : par-dessus le menu (450) et la pause (455)

} // namespace wfs
