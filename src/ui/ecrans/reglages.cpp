// WaForSpeed — écran de réglages (WFS-68 dans WFS-41) : voir reglages.h.
#include "../../render/qualite_auto.h"   // WFS-104
#include "reglages.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../core/log.h"
#include "chargement.h"   // WFS-297 (RG-06) : texteMemoire (mémoire du jeu, de la carte graphique, numéro de la partie)
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/camera_vue.h"
#include "../../game/game.h"
#include "../../game/traffic_system.h"   // WFS-75 : choc du banc (vibration coupée)
#include "../../game/sauvegarde.h"   // WFS-24 : réglages dans la sauvegarde commune
#include "../../game/reseau_relais.h"   // WFS-246 (accroche) : partage de la mesure du rendu
#include "../../game/ressenti.h"     // WFS-80 : direction et « réduire les mouvements » (WFS-70)
#include "../../game/direction.h"    // WFS-108 : gyroscope, taille des boutons (commandes_tactiles.h)
#include "../../platform/gyroscope.h"
#include "../../platform/manette.h"  // WFS-80 : focus de la manette
#include "../../platform/manette_jeu.h"   // WFS-80 : banc (appliquer, focusCourant)
#include <deque>
#include "../../render/renderer.h"
#include "../../render/vk.h"
#include "../text.h"
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)

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
    // WFS-146 (RG-04, ANO-20) : un seul polygone (avant : 16 bandes translucides qui se chevauchaient → fines lignes sombres)
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}

// lignes du panneau (index.html, #pause .settings ; « Graphismes » en 3 niveaux, « Direction » remplacée plus tard
// par la disposition des commandes)
struct Ligne { const char* libelle; int n; const char* choix[4]; };   // WFS-85 : 4 vues
const Ligne LIGNES[13] = {
    {"Caméra", 4, {VUES[0].nom, VUES[1].nom, VUES[2].nom, VUES[3].nom}},   // colonne = VueCamera   // vues de WFS-66 (CAM_MODES du web)
    {"Graphismes", 3, {"Basse", "Moyenne", "Haute", "Ultra"}},   // WFS-150 (RG-04) : « Ultra » seulement si la Haute tient (nChoix)
    {"Son", 2, {"Activé", "Coupé", nullptr}},
    {"Musique", 2, {"Activée", "Coupée", nullptr}},   // WFS-76 : musique seule (l'ambiance reste)
    {"Commandes", 2, {"Droitier", "Gaucher", nullptr}},   // WFS-75 : miroir des commandes tactiles
    {"Vibrations", 2, {"Activées", "Coupées", nullptr}},  // WFS-75 : vibrer() de WFS-70
    {"Direction", 2, {"Glisser", "Flèches", nullptr}},     // WFS-80 : steerMode du web (ModeDirection de WFS-70)
    {"Réduire les mouvements", 2, {"Non", "Oui", nullptr}},   // WFS-80 : reduceMotion du web
    {"Boutons", 2, {"Normaux", "Grands", nullptr}},             // WFS-108 RG-04 : taille des commandes tactiles (× 1,25)
    {"Gyroscope", 4, {"Coupé", "Doux", "Moyen", "Fort"}},       // WFS-108 RG-03 : sensibilité (28°, 20°, 14° pour braquer à fond)
    {"Zone morte gyro", 3, {"Petite", "Moyenne", "Grande"}},    // WFS-108 RG-03 : 1,5°, 3°, 5°
    {"Aide à la direction", 2, {"Activée", "Désactivée", nullptr}},   // WFS-217 RG-02 : reglagesConduite (player.h)
    {"Regard cockpit", 3, {"Bas", "Normal", "Haut"}},   // WFS-235 RG-03 : regardCockpit (camera_vue.h), clé regard
};
enum { L_CAMERA = 0, L_QUALITE = 1, L_SON = 2, L_MUSIQUE = 3, L_COMMANDES = 4, L_VIBRATIONS = 5, L_DIRECTION = 6, L_MOUVEMENTS = 7,
       L_BOUTONS = 8, L_GYRO = 9, L_GYROZONE = 10, L_AIDE = 11, L_REGARD = 12, N_LIGNES = 13 };

Zone g_bouton, g_fermer, g_choix[N_LIGNES][4], g_mesure;   // WFS-246 (accroche) : g_mesure, ligne de la mesure (toucher : partager)
int nChoix(int l) { return l == L_QUALITE && qualite::ultraPropose() ? 4 : LIGNES[l].n; }   // WFS-150 : Ultra proposé (qualite_auto.h)
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
    const bool commune = sauvegarde().actif();
    if (Sauvegarde& s = sauvegarde(); commune) {   // WFS-24 : sauvegarde commune (cam, quality, muted)
        Reglages& R = reglages(); R.camera = s.camIndex(); R.son = !s.muted; R.musique = s.music;
        R.gaucher = s.gaucher; R.vibrations = s.vibrations;   // WFS-75
        grandsBoutons() = s.grandsBoutons; reglagesGyro() = {s.gyro, s.gyroZone};   // WFS-108
        reglagesConduite().aideDirection = s.aideDirection;   // WFS-217
        if (s.qualityChoisie) { R.qualite = s.qualiteIndex(); R.qualiteChoisie = true; }
    }
    const std::string f = fichier();
    FILE* fp = f.empty() ? nullptr : std::fopen(f.c_str(), "r");
    if (!fp) return commune;
    Reglages& R = reglages();
    char cle[32]; int v;
    while (std::fscanf(fp, " %31[^=]=%d", cle, &v) == 2) {
        if (!std::strcmp(cle, "mouvements")) reglagesRessenti().reduireMouvements = v != 0;   // WFS-80 (RG-02)
        else if (!std::strcmp(cle, "ultra")) qualite::ultraChoisi = v != 0;   // WFS-150 : Ultra (absent de la sauvegarde commune)
        else if (!std::strcmp(cle, "regard")) choisirRegard((RegardCockpit)std::clamp(v, 0, 2));   // WFS-235 (RG-03), comme « mouvements »
        else if (commune) continue;   // WFS-24 : les autres réglages viennent de la sauvegarde commune
        else if (!std::strcmp(cle, "camera")) R.camera = std::clamp(v, 0, 2);
        else if (!std::strcmp(cle, "qualite")) { R.qualite = std::clamp(v, 0, 2); R.qualiteChoisie = true; }
        else if (!std::strcmp(cle, "son")) R.son = v != 0;
        else if (!std::strcmp(cle, "musique")) R.musique = v != 0;   // WFS-76
        else if (!std::strcmp(cle, "gaucher")) R.gaucher = v != 0;   // WFS-75
        else if (!std::strcmp(cle, "vibrations")) R.vibrations = v != 0;
        else if (!std::strcmp(cle, "boutons")) grandsBoutons() = v != 0;   // WFS-108
        else if (!std::strcmp(cle, "gyro")) reglagesGyro().niveau = std::clamp(v, 0, 3);
        else if (!std::strcmp(cle, "gyrozone")) reglagesGyro().zone = std::clamp(v, 0, 2);
    }
    std::fclose(fp);
    WFS_LOG("réglages lus : caméra %d, qualité %d, son %d, gaucher %d, vibrations %d, mouvements %d", R.camera, R.qualite, (int)R.son,
            (int)R.gaucher, (int)R.vibrations, (int)reglagesRessenti().reduireMouvements);
    return true;
}
void ecrire() {
    if (Sauvegarde& s = sauvegarde(); s.actif()) {   // WFS-24 : écriture atomique sur le thread de travail
        const Reglages& R = reglages(); s.setCamIndex(R.camera); s.muted = !R.son; s.music = R.musique;
        s.gaucher = R.gaucher; s.vibrations = R.vibrations;   // WFS-75
        s.grandsBoutons = grandsBoutons(); s.gyro = reglagesGyro().niveau; s.gyroZone = reglagesGyro().zone;   // WFS-108
        s.aideDirection = reglagesConduite().aideDirection;   // WFS-217
        if (R.qualiteChoisie) s.setQualiteIndex(R.qualite);
        s.enregistrer();   // WFS-80 : le fichier simple garde en plus « mouvements » (absent de la sauvegarde commune)
    }
    const std::string f = fichier();
    FILE* fp = f.empty() ? nullptr : std::fopen(f.c_str(), "w");
    if (!fp) return;
    const Reglages& R = reglages();
    std::fprintf(fp, "camera=%d\nqualite=%d\nson=%d\nmusique=%d\ngaucher=%d\nvibrations=%d\nmouvements=%d\n", R.camera, R.qualite, (int)R.son,
                 (int)R.musique, (int)R.gaucher, (int)R.vibrations, (int)reglagesRessenti().reduireMouvements);
    std::fprintf(fp, "boutons=%d\ngyro=%d\ngyrozone=%d\nultra=%d\nregard=%d\n", (int)grandsBoutons(), reglagesGyro().niveau, reglagesGyro().zone, (int)qualite::ultraChoisi, (int)regardCockpit());   // WFS-108 ; WFS-150 ; WFS-235
    std::fclose(fp);
}

// ---------------------------------------------------------------- application immédiate
void appliquerCamera() { choisirVue((VueCamera)reglages().camera); }   // WFS-66 : vue mémorisée pour la session
// qualité : échelle de la scène 3D (cibles recréées) et ombres du soleil ; jamais en mode GPU logiciel (WFS-59)
void appliquerQualite(Renderer& r) {
    if (r.gpuLogiciel()) return;
    const int q = reglages().qualite;
    r.vk().renderScale = qualite::echelleScene(q, 1.f);   // WFS-104 : Moyenne et Haute à 100 % ; WFS-134 : netteté retenue (chauffe)
    r.forceNoShadows = q == QUALITE_BASSE;
    r.bloomForce = q == QUALITE_BASSE ? 0.f : 1.f;   // WFS-15 : bloom en Moyenne et Haute
    r.qualiteBasse = q == QUALITE_BASSE;               // WFS-82 : petits objets coupés à 150 m
    r.niveauOmbres = qualite::niveauAffiche(q);         // WFS-227 (accroche) : nombre de cascades d'ombre
    r.resize();
    WFS_LOG("réglages : qualité %s (scène à %.0f %%, ombres %s)", LIGNES[L_QUALITE].choix[q], r.vk().renderScale * 100,
            r.forceNoShadows ? "coupées" : "actives");
}
int valeur(int ligne) {
    const Reglages& R = reglages();
    switch (ligne) {
    case L_CAMERA: return R.camera;
    case L_QUALITE: return qualite::niveauAffiche(R.qualite);   // WFS-150 : 3 = Ultra
    case L_SON: return R.son ? 0 : 1;
    case L_MUSIQUE: return R.musique ? 0 : 1;
    case L_COMMANDES: return R.gaucher ? 1 : 0;
    case L_DIRECTION: return (int)reglagesRessenti().direction;
    case L_MOUVEMENTS: return reglagesRessenti().reduireMouvements ? 1 : 0;
    case L_BOUTONS: return grandsBoutons() ? 1 : 0;   // WFS-108
    case L_GYRO: return reglagesGyro().niveau;
    case L_GYROZONE: return reglagesGyro().zone;
    case L_AIDE: return reglagesConduite().aideDirection ? 0 : 1;   // WFS-217
    case L_REGARD: return (int)regardCockpit();   // WFS-235
    default: return R.vibrations ? 0 : 1;
    }
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
        if (g_mesure.contient(t.x, t.y) && !qualite::texteMesure().empty()) { doigt_ = t.id; WFS_LOG("réglages : mesure du rendu partagée (WFS-246)"); reseau::partager(qualite::texteMesure()); return true; }
        for (int l = 0; l < N_LIGNES; l++) for (int c = 0; c < nChoix(l); c++) {
            if (!g_choix[l][c].contient(t.x, t.y) || valeur(l) == c) continue;
            Reglages& R = reglages();
            if (l == L_CAMERA) { R.camera = c; appliquerCamera(); }
            else if (l == L_QUALITE) { qualite::ultraChoisi = c == qualite::ULTRA; R.qualite = std::min(c, (int)QUALITE_HAUTE); R.qualiteChoisie = true; qualite::oublierRetenus(); if (r_) appliquerQualite(*r_); }   // WFS-134
            else if (l == L_SON) R.son = c == 0;
            else if (l == L_MUSIQUE) R.musique = c == 0;
            else if (l == L_COMMANDES) R.gaucher = c == 1;   // WFS-75 : lu à chaque toucher et à chaque image (miroirX)
            else if (l == L_VIBRATIONS) R.vibrations = c == 0;   // WFS-75 : lu à chaque vibrer()
            else if (l == L_DIRECTION) {   // WFS-80 (RG-01) : sauvegarde (clé steer) et ressenti tout de suite
                reglagesRessenti().direction = (ModeDirection)c; sauvegarde().steer = c ? "fleches" : "glisser";
            } else if (l == L_BOUTONS) grandsBoutons() = c == 1;   // WFS-108 : appliqué à l'image suivante
            else if (l == L_GYRO) reglagesGyro().niveau = c;
            else if (l == L_GYROZONE) reglagesGyro().zone = c;
            else if (l == L_AIDE) reglagesConduite().aideDirection = c == 0;   // WFS-217 : appliquée au pas suivant
            else if (l == L_REGARD) choisirRegard((RegardCockpit)c);   // WFS-235 (RG-03) : appliqué à l'image suivante
            else reglagesRessenti().reduireMouvements = c == 1;   // WFS-80 (RG-02) : clé mouvements
            WFS_LOG("réglages : %s = %s", LIGNES[l].libelle, LIGNES[l].choix[c]);
            ecrire();
        }
        return true;   // RG-03 : le panneau garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.sceneDediee || (g.ecran != Ecran::Menu && g.ecran != Ecran::Pause)) return;
        // WFS-147 (RG-01, RG-02, ANO-19) : sur téléphone couché, mise en page propre aux réglages, dessinée en dp
        const bool cp = ui::petitEcran(g.uiWidth, g.uiHeight);
        const float W = g.uiWidth, H = g.uiHeight, k = cp ? ui::unitePetitEcran(W, H) : H / 720.f;
        if (!g_ouvert) { dessinerBouton(r, g, W, k, H / 720.f); manette::focus(g_bouton); return; }   // WFS-80 : bouton au focus
        // WFS-80 (RG-05) : panneau ouvert → seuls ses choix et « Fermer » (B) gardent le focus, pas le menu dessous
        manette::zonesImage().clear();
        if (cp) dessinerPanneauPetit(r, g, W, H, k); else dessinerPanneau(r, g, W, H, k);
        for (int l = 0; l < N_LIGNES; l++) for (int c = 0; c < nChoix(l); c++) manette::focus(g_choix[l][c]);
        manette::focus(g_fermer, true);
    }

private:
    // .cam-btn : vitré, bordure 1 px, biais 8 px, 44 px de haut, « RÉGLAGES » ; en haut à droite de la zone sûre
    // WFS-168 (RG-01) : taille du bouton d'avant WFS-147 (unité kb = H / 720, comme à 349f7d0) ; texte et place gardés
    void dessinerBouton(Renderer& r, const GameView& g, float W, float k, float kb) {
        const float h = 44 * kb, tp = 15 * k, pad = 18 * kb;
        const std::string t = ui::majuscules("Réglages");
        const float w = pad + ui::width(tp, t, 0.12f) + pad, x = W - g.marges.droite - 16 * k - w, y = g.marges.haut + 14 * k;
        g_bouton = {x, y, w, h};
        biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, BORD);
        biseau(r, x, y, w, h, 8 * kb, GLASS);
        ui::text(r, std::round(x + pad), std::round(y + (h - tp) / 2), tp, t, hex(ACCENT2), ui::GAUCHE, 0.12f, false);
    }
    // section .settings : voile, .eyebrow, .big « Réglages », lignes .set-row (libellé + boutons .seg), « Fermer »
    void dessinerPanneau(Renderer& r, const GameView& g, float W, float H, float k) {
        r.uiRect(0, 0, W, H, hex(BG, 0.86f));
        const Marges& m = g.marges;
        const float zl = m.gauche, zw = W - m.gauche - m.droite;
        const float pw = std::min(zw - 32 * k, 760 * k), px = zl + (zw - pw) / 2, py = m.haut + 40 * k;
        // WFS-75 : 6 lignes → lignes de 50 px (64 avant) séparées de 10 px, première ligne 22 px plus haut ;
        // WFS-80 (RG-03) : 8 lignes → lignes de 38 px séparées de 7 px, même hauteur de panneau (pas de défilement) ;
        // WFS-108 : 11 lignes → lignes de 30 px séparées de 5 px
        const float rowH = 30 * k, gap = 5 * k, haut = 128 * k, bh = 50 * k, ph = haut + (float)N_LIGNES * (rowH + gap) + bh + 30 * k;
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
        const float x0 = px + 36 * k;
        ui::text(r, x0, py + 30 * k, 13 * k, ui::majuscules(g.ecran == Ecran::Pause ? "Course en pause" : "Menu principal"), hex(ACCENT2),
                 ui::GAUCHE, 0.22f, false);
        ui::text(r, x0, py + 52 * k, 56 * k, ui::majuscules("Réglages"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        float y = py + haut;
        const float segW = std::min(170 * k, (pw - 72 * k - 190 * k - 3 * 6 * k) / 4);
        for (int l = 0; l < N_LIGNES; l++) {
            r.uiRect(x0, y + rowH + gap / 2, pw - 72 * k, 1 * k, hex(TEXTE, 0.08f));   // séparateur
            ui::text(r, x0, y + (rowH - 14 * k) / 2, 14 * k, ui::majuscules(LIGNES[l].libelle), hex(MUTED), ui::GAUCHE, 0.16f, false);
            const int n = nChoix(l);
            float bx = px + pw - 36 * k - n * segW - (n - 1) * 6 * k;
            for (int c = 0; c < n; c++) {
                const bool on = valeur(l) == c;   // aria-pressed : fond cyan léger, bordure et texte cyan
                g_choix[l][c] = {bx, y, segW, rowH};
                if (on) {
                    r.uiRect(bx, y, segW, rowH, hex(ACCENT2, 0.16f));
                    r.uiRect(bx, y + rowH - 3 * k, segW, 3 * k, hex(ACCENT2));
                } else r.uiRect(bx, y, segW, rowH, hex(SURFACE2));
                ui::text(r, bx + segW / 2, y + (rowH - 15 * k) / 2, 15 * k, ui::majuscules(LIGNES[l].choix[c]), on ? hex(ACCENT2) : hex(TEXTE),
                         ui::CENTRE, 0.08f, false);
                bx += segW + 6 * k;
            }
            for (int c = n; c < 4; c++) g_choix[l][c] = {};
            y += rowH + gap;
        }
        // .btn « Fermer » (orange, biais 14 px), à droite, sous la dernière ligne ; WFS-286 (RG-01, ANO-156) : toutes les lignes à la
        // suite (avant : « Aide à la direction » et « Regard cockpit » sous le bouton, placé après la ligne 11) ; tailles inchangées
        const float tp = 20 * k;
        const std::string ft = ui::majuscules("Fermer");
        const float fw = ui::width(tp, ft, 0.12f) + 56 * k, fx = px + pw - 36 * k - fw, fy = py + haut + (float)N_LIGNES * (rowH + gap) + 10 * k;
        g_fermer = {fx, fy, fw, bh};
        // WFS-134 (RG-01) : rendu réellement utilisé (niveau, échelle, MSAA, images/s, chauffe), à gauche de « Fermer »
        const std::string lm = qualite::ligneMesure();   // WFS-246 (accroche, RG-06) : 2e ligne, réduite pour tenir avant « Fermer »
        {   // WFS-297 (RG-06) : rendu puis mémoire sur la même ligne, réduite pour tenir avant « Fermer » (aucun bouton ne bouge)
            const std::string lr = qualite::ligneRendu() + " · " + texteMemoire(false);
            const float tr = std::min(15 * k, 15 * k * (fx - x0 - 8 * k) / std::max(ui::width(15 * k, lr, 0.02f), 1.f));
            ui::text(r, x0, lm.empty() ? fy + (bh - tr) / 2 : fy + bh / 2 - tr, tr, lr, hex(ACCENT2), ui::GAUCHE, 0.02f, false);
        }
        if (!lm.empty()) { const float tm = std::min(12 * k, 12 * k * (fx - x0 - 8 * k) / std::max(ui::width(12 * k, lm, 0.f), 1.f)); g_mesure = {x0, fy, fx - x0 - 8 * k, bh};
                           ui::text(r, x0, fy + bh / 2 + 1 * k, tm, lm, hex(TEXTE), ui::GAUCHE, 0.f, false); } else g_mesure = {};
        biseau(r, fx, fy, fw, bh, 14 * k, hex(ACCENT));
        ui::text(r, fx + fw / 2, fy + (bh - tp) / 2, tp, ft, hex(BG), ui::CENTRE, 0.12f, false);
    }

    // WFS-147 (RG-01, RG-02, RG-06, ANO-19) : téléphone couché (k = pixels par dp). Reprise de la mise en page petit écran de
    // WFS-102 (6d3ed61), sans échelle commune : 11 lignes en deux colonnes de 6, choix de 42 dp de haut (zone de toucher de
    // 48 dp, l'écart compris, sans déborder sur la voisine), textes des choix en 14 dp, libellés en capitales 12 dp,
    // « Fermer » en haut à droite (48 dp), ligne « Rendu » en bas (14 dp) : tout tient dans 400 dp de haut, sans défilement.
    void dessinerPanneauPetit(Renderer& r, const GameView& g, float W, float H, float k) {
        r.uiRect(0, 0, W, H, hex(BG, 0.86f));
        const Marges& m = g.marges;
        const float zl = m.gauche, zw = W - m.gauche - m.droite;
        const int PAR_COL = (L_REGARD + 1) / 2;   // WFS-235 : « Regard cockpit » dans l'en-tête, les 12 autres lignes ne bougent pas
        // WFS-168 (RG-01) : taille des choix et de « Fermer » d'avant WFS-147 (unité kb = H / 720, comme à 349f7d0 :
        // choix de 30 x 170 au plus, « Fermer » de 50) ; lignes, colonnes et textes en dp gardés (choix centrés dans leur ligne)
        const float kb = H / 720.f, choixH = 30 * kb;
        const float rowH = 42 * k, gap = 6 * k, haut = 66 * k, bh = 50 * kb;
        const float ph = haut + PAR_COL * (rowH + gap) + 36 * k;
        const float pw = std::min(zw - 16 * k, 900 * k), px = zl + (zw - pw) / 2, py = std::max(m.haut + 4 * k, (H - ph) / 2);
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
        const float x0 = px + 24 * k;
        ui::text(r, x0, py + 14 * k, 12 * k, ui::majuscules(g.ecran == Ecran::Pause ? "Course en pause" : "Menu principal"), hex(ACCENT2),
                 ui::GAUCHE, 0.22f, false);
        ui::text(r, x0, py + 30 * k, 30 * k, ui::majuscules("Réglages"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
        {   // .btn « Fermer » (orange, biais 14 px), en haut à droite du panneau
            const float tp = 18 * k;
            const std::string ft = ui::majuscules("Fermer");
            const float fw = ui::width(tp, ft, 0.12f) + 56 * kb, fx = px + pw - 24 * k - fw, fy = py + 10 * k;
            g_fermer = {fx, fy, fw, bh};
            biseau(r, fx, fy, fw, bh, 14 * kb, hex(ACCENT));
            ui::text(r, fx + fw / 2, fy + (bh - tp) / 2, tp, ft, hex(BG), ui::CENTRE, 0.12f, false);
        }
        const float colGap = 24 * k, colW = (pw - 48 * k - colGap) / 2, libW = 104 * k, segGap = 6 * k;
        for (int l = 0; l < N_LIGNES; l++) {
            const int col = l / PAR_COL, rang = l % PAR_COL;
            const bool entete = l == L_REGARD;   // WFS-235 (RG-03) : entre le titre et « Fermer »
            const float cx = entete ? px + (pw - colW) / 2 : x0 + col * (colW + colGap), y = entete ? py + (haut - rowH) / 2 : py + haut + rang * (rowH + gap);
            r.uiRect(cx, y + rowH + gap / 2, colW, 1 * k, hex(TEXTE, 0.08f));   // séparateur
            // libellé : sur deux lignes s'il dépasse sa place (« Réduire les mouvements », « Zone morte gyro »)
            const std::string lib = ui::majuscules(LIGNES[l].libelle);
            const float lp = 12 * k;
            if (ui::width(lp, lib, 0.12f) <= libW) ui::text(r, cx, y + (rowH - lp) / 2, lp, lib, hex(MUTED), ui::GAUCHE, 0.12f, false);
            else {
                size_t coupe = lib.find(' ', lib.size() / 2 - 2);
                if (coupe == std::string::npos) coupe = lib.rfind(' ');
                ui::text(r, cx, y + rowH / 2 - lp - 1 * k, lp, lib.substr(0, coupe), hex(MUTED), ui::GAUCHE, 0.12f, false);
                ui::text(r, cx, y + rowH / 2 + 1 * k, lp, lib.substr(coupe + 1), hex(MUTED), ui::GAUCHE, 0.12f, false);
            }
            const int n = nChoix(l);
            // WFS-168 : largeur d'avant WFS-147 (panneau de 760 au plus, une colonne), bornée par la place de la colonne
            const float pwAvant = std::min(zw - 32 * kb, 760 * kb), segAvant = std::min(170 * kb, (pwAvant - 72 * kb - 190 * kb - 3 * 6 * kb) / 4);
            const float place = colW - libW, segW = std::min(segAvant, (place - 3 * segGap) / 4);   // même largeur pour 2 ou 4 choix
            const float cy = y + (rowH - choixH) / 2;   // choix centré dans sa ligne
            float bx = cx + colW - n * segW - (n - 1) * segGap;
            for (int c = 0; c < n; c++) {
                const bool on = valeur(l) == c;   // aria-pressed : fond cyan léger, bordure et texte cyan
                g_choix[l][c] = {bx, cy, segW, choixH};   // WFS-168 : zone de toucher d'avant WFS-147 (le choix lui-même)
                if (on) {
                    r.uiRect(bx, cy, segW, choixH, hex(ACCENT2, 0.16f));
                    r.uiRect(bx, cy + choixH - 3 * kb, segW, 3 * kb, hex(ACCENT2));
                } else r.uiRect(bx, cy, segW, choixH, hex(SURFACE2));
                const std::string ch = ui::majuscules(LIGNES[l].choix[c]);
                float tp = 14 * k, esp = 0.06f;
                if (ui::width(tp, ch, esp) > segW - 6 * k) esp = 0;   // texte serré plutôt que plus petit (RG-02 : 14 dp)
                ui::text(r, bx + segW / 2, cy + (choixH - tp) / 2, tp, ch, on ? hex(ACCENT2) : hex(TEXTE), ui::CENTRE, esp, false);
                bx += segW + segGap;
            }
            for (int c = n; c < 4; c++) g_choix[l][c] = {};
        }
        // WFS-134 (RG-01) : rendu réellement utilisé, sous les deux colonnes
        {   // WFS-297 (RG-06) : rendu puis mémoire du jeu, de la carte graphique et numéro de la partie, réduits s'ils ne tiennent pas
            const std::string lr = qualite::ligneRendu() + " · " + texteMemoire(false);
            const float tr = std::min(14 * k, 14 * k * (pw - 48 * k) / std::max(ui::width(14 * k, lr, 0.02f), 1.f));
            ui::text(r, x0, py + ph - 12 * k - tr, tr, lr, hex(ACCENT2), ui::GAUCHE, 0.02f, false);
        }
        const std::string lm = qualite::ligneMesure();   // WFS-246 (accroche, RG-06) : 2e ligne sous le rendu (toucher : partager)
        if (!lm.empty()) { const float tm = std::min(10 * k, 10 * k * (pw - 48 * k) / std::max(ui::width(10 * k, lm, 0.f), 1.f)); g_mesure = {x0, py + ph - 28 * k, pw - 48 * k, 28 * k};
                           ui::text(r, x0, py + ph - 11 * k, tm, lm, hex(TEXTE), ui::GAUCHE, 0.f, false); } else g_mesure = {};
    }

    Renderer* r_ = nullptr;
    int doigt_ = -1;
};

} // namespace

Reglages& reglages() { static Reglages R; return R; }
bool reglagesOuverts() { return g_ouvert; }
void ouvrirReglagesBanc(bool oui) { g_ouvert = oui; }   // WFS-297
bool toucheReglages(float x, float y) { return g_ouvert || g_bouton.contient(x, y); }

// WFS-80 : banc manette du garage (pastilles) et des réglages : le focus va à la zone visée par la croix (chemin le plus
// court calculé avec manette::voisin, comme un joueur), A = toucher, B = retour
static void benchManetteEcrans(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) { WFS_ERR("manette (WFS-80) : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 1500;
    auto image = [&]() { manette::appliquer(g); g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto appui = [&](manette::Bouton b, int n = 4) { manette::bouton(b, true); image(); manette::bouton(b, false); for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, focus %d, %s", nom, nomEcran(g.ecran()), manette::focusCourant(), quoi);
    };
    auto aller = [&](int cible) {   // parcours en largeur sur les 4 directions de la croix
        const std::vector<manette::Zone> z = manette::zonesPrecedentes();
        const int dep = manette::focusCourant();
        if (cible < 0 || cible >= (int)z.size() || dep < 0) { WFS_ERR("manette (WFS-80) : cible %d absente", cible); return; }
        const int DX[4] = {1, -1, 0, 0}, DY[4] = {0, 0, 1, -1};
        const manette::Bouton BT[4] = {manette::DROITE, manette::GAUCHE, manette::BAS, manette::HAUT};
        std::vector<int> prec(z.size(), -2), coup(z.size(), -1);
        std::deque<int> file{dep}; prec[dep] = -1;
        while (!file.empty()) {
            const int i = file.front(); file.pop_front();
            for (int d = 0; d < 4; d++) { const int j = manette::voisin(z, i, DX[d], DY[d]); if (prec[j] == -2) { prec[j] = i; coup[j] = d; file.push_back(j); } }
        }
        if (prec[cible] == -2) { WFS_ERR("manette (WFS-80) : zone %d injoignable", cible); return; }
        std::vector<int> chemin;
        for (int i = cible; i != dep; i = prec[i]) chemin.insert(chemin.begin(), coup[i]);
        for (int d : chemin) appui(BT[d], 1);
    };
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    // garage : zones choisir, retour, prev, next, puis les 16 pastilles ; A sur la 6e (index 5)
    g.allerA(Ecran::Garage);
    for (int i = 0; i < 40; i++) image();
    appui(manette::SELECT);   // la manette prend la main (focus sur « Choisir »)
    const size_t nz = manette::zonesPrecedentes().size();
    WFS_LOG("manette (WFS-80) : garage, %zu zones de focus (20 attendues : 4 boutons + 16 pastilles)", nz);
    if (nz == 20) {
        aller(4 + 5);
        WFS_LOG("manette (WFS-80) : focus sur la pastille %d", manette::focusCourant() - 4);
        appui(manette::A, 12);
        capture("manette-garage-peinture", "cadre sur la 6e pastille, peinture prise après A");
    }
    appui(manette::B, 40);
    WFS_LOG("manette (WFS-80) : B dans le garage → écran %s", nomEcran(g.ecran()));
    // menu : bouton « Réglages » (dernière zone déclarée), A → panneau ; « Direction : Flèches », A ; B → fermé
    g.allerA(Ecran::Menu); for (int i = 0; i < 70; i++) image();
    aller((int)manette::zonesPrecedentes().size() - 1);
    appui(manette::A, 6);
    WFS_LOG("manette (WFS-80) : A sur « Réglages » → panneau %s, %zu zones", g_ouvert ? "ouvert" : "fermé", manette::zonesPrecedentes().size());
    int cible = -1;
    const std::vector<manette::Zone>& z = manette::zonesPrecedentes();
    const Zone& c = g_choix[L_DIRECTION][1];
    for (int i = 0; i < (int)z.size(); i++) if (std::fabs(z[i].x - c.x) < 0.5f && std::fabs(z[i].y - c.y) < 0.5f) cible = i;
    aller(cible);
    appui(manette::A, 6);
    WFS_LOG("manette (WFS-80) : A sur « Direction : Flèches » → direction %s, steer %s", reglagesRessenti().direction == ModeDirection::Fleches ? "Flèches" : "Glisser",
            sauvegarde().steer.c_str());
    capture("manette-reglages", "cadre sur « Flèches », réglage changé par A");
    appui(manette::B, 6);
    WFS_LOG("manette (WFS-80) : B dans les réglages → panneau %s, écran %s", g_ouvert ? "ouvert" : "fermé", nomEcran(g.ecran()));
    manette::toucherEcran(); image();
    reglagesRessenti() = {}; sauvegarde().steer = "glisser";   // banc : retour aux défauts
    g.shutdown();
}

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
    // WFS-75 : depuis la pause, « Commandes : Gaucher » et « Vibrations : Coupées », appliqués sans redémarrer
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(13, g_bouton);
    toucher(14, g_choix[L_COMMANDES][1]); toucher(15, g_choix[L_VIBRATIONS][1]);
    capture("reglages-commandes", "Commandes : Gaucher, Vibrations : Coupées");
    toucher(16, g_fermer); g.allerA(Ecran::Course); g.setAutopilot(false);
    for (int i = 0; i < 30; i++) image();
    {   // appui simulé à gauche, à la place de l'accélérateur du droitier en miroir : gaz attendu
        const float W = (float)r.width(), H = (float)r.height(), x = W * 0.11f, y = H * 0.74f;
        const double v0 = g.player().kmh();
        g.onTouch({20, x, y, true});
        for (int i = 0; i < 60; i++) image();
        WFS_LOG("réglages : gaucher, appui à gauche (%.0f ; %.0f) = gaz : %.0f → %.0f km/h", x, y, v0, g.player().kmh());
        capture("course-gaucher", "commandes gaucher, accélérateur à gauche appuyé");
        g.onTouch({20, x, y, false});
    }
    // choc du banc, vibrations coupées : « vibration ignorée (réglage) » attendu au journal
    if (const Course* k = activeCourse()) {
        g.setAutopilot(true);
        const int avant = k->accidents;
        trafficBenchPlace(14, 0, 80, true);
        for (int f = 0; f < 150 && k->accidents == avant; f++) image();
        WFS_LOG("réglages : choc du banc avec vibrations coupées : %d accident(s)", k->accidents - avant);
    }
    reglages().gaucher = false; reglages().vibrations = true;   // banc : retour aux défauts
    // WFS-80 : « Direction : Flèches » et « Réduire les mouvements : Oui » depuis la pause (capture reglages), puis relecture
    g.setAutopilot(true);
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(21, g_bouton);
    toucher(22, g_choix[L_DIRECTION][1]);
    WFS_LOG("réglages : direction du ressenti = %s, steer sauvegardé = %s (Flèches / fleches attendus)",
            reglagesRessenti().direction == ModeDirection::Fleches ? "Flèches" : "Glisser", sauvegarde().steer.c_str());
    toucher(23, g_choix[L_MOUVEMENTS][1]);
    capture("reglages", "8 lignes : Direction Flèches, Réduire les mouvements Oui");
    {   // relancement simulé : écriture du fichier des réglages, mouvements remis à Non, puis relecture
        const std::string f = out + "/reglages-wfs80.txt";
        setenv("WFS_REGLAGES", f.c_str(), 1);
        ecrire(); reglagesRessenti().reduireMouvements = false; lire();
        WFS_LOG("réglages : après relecture, réduire les mouvements = %s (Oui attendu)", reglagesRessenti().reduireMouvements ? "Oui" : "Non");
        unsetenv("WFS_REGLAGES"); std::remove(f.c_str());
    }
    // WFS-108 : « Boutons : Grands », « Gyroscope : Moyen » (capture reglages-108), puis course : inclinaison d'essai de 12°
    // (braquage attendu (12 − 3) / (20 − 3) ≈ 0,53), gaz ; puis frein maintenu jusqu'à la marche arrière (« R » au HUD)
    toucher(25, g_choix[L_BOUTONS][1]); toucher(26, g_choix[L_GYRO][2]);
    capture("reglages-108", "11 lignes : Boutons Grands, Gyroscope Moyen, Zone morte Moyenne");
    reglagesRessenti() = {}; sauvegarde().steer = "glisser";
    gyroscope::capteurPresent(true); gyroscope::publier(9.81f, 0.f);   // téléphone droit : zéro pris au départ de la course
    toucher(24, g_fermer);
    g.allerA(Ecran::Course); g.setAutopilot(false);
    for (int i = 0; i < 6; i++) image();
    {
        const float W = (float)r.width(), H = (float)r.height();
        const DispositionCommandes d = dispositionCommandes(W, H, Marges{});
        const float a = 12.f * PI / 180.f;
        gyroscope::publier(9.81f * std::cos(a), 9.81f * std::sin(a));
        g.onTouch({30, d.gaz.cx(), d.gaz.cy(), true});
        for (int i = 0; i < 45; i++) image();
        WFS_LOG("commandes (WFS-108) : grands boutons, gaz %.0f × %.0f px (%.2f px/dp), gyroscope 12° → consigne %.2f (0,53 attendu), braquage de la voiture %.2f%s, %.0f km/h",
                d.gaz.w, d.gaz.h, d.k, directionBraquage(W), g.player().steer, g.player().controlLock > 0 ? " (bloqué : accident)" : "", g.player().kmh());
        capture("commandes-grandes-gyro", "boutons grands, gaz appuyé, téléphone incliné de 12° vers la droite");
        g.onTouch({30, d.gaz.cx(), d.gaz.cy(), false});
        gyroscope::publier(9.81f, 0.f);
        g.onTouch({31, d.frein.cx(), d.frein.cy(), true});   // frein maintenu : arrêt puis marche arrière sans relâcher
        int f = 0;
        for (; f < 60 * 8 && !(g.player().reversing && g.player().speed < REV_MAX * 0.5); f++) image();
        WFS_LOG("commandes (WFS-108) : frein maintenu %.1f s → marche arrière %s, %.0f km/h", f / 60.0, g.player().reversing ? "oui" : "non",
                g.player().kmh());
        capture("commandes-marche-arriere", "frein maintenu à l'arrêt : marche arrière, « R » au compteur");
        g.onTouch({31, d.frein.cx(), d.frein.cy(), false});
    }
    grandsBoutons() = false; reglagesGyro() = {}; gyroscope::capteurPresent(false); gyroscope::etat() = {};   // banc : défauts
    g.shutdown();
    benchManetteEcrans(g, r, out);
    courseBenchRules(false);
}

void appliquerQualiteNiveau(Renderer& r) { appliquerQualite(r); }   // WFS-15

} // namespace ecrans

WFS_SYSTEM(ecrans::EcranReglages, 460)   // interface (400-499) : par-dessus le menu (450) et la pause (455)

} // namespace wfs
