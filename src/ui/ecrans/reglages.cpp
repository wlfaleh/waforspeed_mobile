// WaForSpeed — écran de réglages (WFS-68 dans WFS-41) : voir reglages.h.
#include "../../render/qualite_auto.h"   // WFS-104
#include "creation_profil.h"   // WFS-481 (accroche) : écran « Crée ton pilote » ouvert
#include "reglages.h"
#include "../../game/course5.h"   // WFS-557 : choixModeOuvert (maquette 02)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include "../../core/log.h"
#include "cgu.h"   // WFS-308 (RG-05) : conditions d'utilisation en lecture seule
#include "chargement.h"   // WFS-297 (RG-06) : texteMemoire (mémoire du jeu, de la carte graphique, numéro de la partie)
#include "../../core/system.h"
#include "../../game/course_system.h"
#include "../../game/camera_vue.h"
#include "../../game/fantome.h"   // WFS-318 : réglage « Fantôme du record »
#include "../../game/game.h"
#include "../../game/traffic_system.h"   // WFS-75 : choc du banc (vibration coupée)
#include "../../game/sauvegarde.h"   // WFS-24 : réglages dans la sauvegarde commune
#include "../../game/reseau_relais.h"   // WFS-246 (accroche) : partage de la mesure du rendu
#include "../../game/ressenti.h"     // WFS-80 : direction et « réduire les mouvements » (WFS-70)
#include "../../game/direction.h"    // WFS-108 : gyroscope, taille des boutons (commandes_tactiles.h)
#include "../../platform/gyroscope.h"
#include "../../platform/manette.h"  // WFS-80 : focus de la manette
#include "../../platform/manette_jeu.h"   // WFS-80 : banc (appliquer, focusCourant)
#include "../../platform/retour_systeme.h"   // WFS-290 : banc de la touche Retour
#include <deque>
#include "../../render/renderer.h"
#include "../../render/vk.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../credits.h"   // WFS-549 (RG-06) : « Crédits » à côté des conditions d'utilisation
#include "../verre.h"   // WFS-548 (R19) : fenêtre de verre, sur-titre gris, fond flouté
#include "../langue.h"   // WFS-504 : textes dans la langue du joueur (ui::tr), réglage « Langue »
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)
#include "reglages_appui.h"   // WFS-290 : cible de 48 dp, anti-rebond
#include "reglages_onglets.h"   // WFS-291, WFS-318 : lignes et onglets
#include "../../render/diagnostic_perf.h"   // WFS-419 : menu caché des interrupteurs de diagnostic
#include "../../render/passes.h"   // WFS-419 : horodatages GPU disponibles
#include "reglages_banc.h"   // WFS-420 : banc de performance automatique (protocole, résultats)
#include "reglages_etoiles.h"   // WFS-546 : mode testeur, bouton étoile, +2500 ★ par appui (un appui = un crédit)
#include "../../game/score_fin.h"   // WFS-546 (RG-05) : dessinerEtoile, la ★ du compteur d'étoiles du menu (WFS-537)

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
// jetons :root du jeu web (codes sRGB)
const unsigned ACCENT_PRESSE = 0xFF9A4D;   // WFS-290 : orange éclairci d'un bouton appuyé
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.10f};

struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    // WFS-146 (RG-04, ANO-20) : un seul polygone (avant : 16 bandes translucides qui se chevauchaient → fines lignes sombres)
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}

// WFS-504 (RC-E20-04) : largeur d'un bouton = celle de son texte le plus long des deux langues (capitales) : aucun bouton ne
// change de taille avec la langue, le français garde sa taille d'avant
float largeurMax(float tp, const char* cle, float esp) {
    return std::max(ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Francais)), esp),
                    ui::width(tp, ui::majuscules(ui::trLangue(cle, ui::langue::Langue::Anglais)), esp));
}

// lignes du panneau (index.html, #pause .settings ; « Graphismes » en 3 niveaux, « Direction » remplacée plus tard
// par la disposition des commandes)
// WFS-504 (RG-10) : libellés et choix sont des clés des tables de textes (assets/textes/fr.txt, en.txt), affichées par ui::tr
struct Ligne { const char* libelle; int n; const char* choix[4]; };   // WFS-85 : 4 vues
const Ligne LIGNES[N_LIGNES] = {
    {"reglages.camera", 4, {"vue.poursuite", "vue.capot", "vue.cockpit", "vue.proche"}},   // colonne = VueCamera (VUES de WFS-66, id)
    {"reglages.graphismes", 3, {"reglages.graphismes.basse", "reglages.graphismes.moyenne", "reglages.graphismes.haute", "reglages.graphismes.ultra"}},   // WFS-150 (RG-04) : « Ultra » seulement si la Haute tient (nChoix)
    {"reglages.son", 2, {"reglages.son.active", "reglages.son.coupe", nullptr}},
    {"reglages.musique", 2, {"reglages.musique.active", "reglages.musique.coupee", nullptr}},   // WFS-76 : musique seule (l'ambiance reste)
    {"reglages.commandes", 2, {"reglages.commandes.droitier", "reglages.commandes.gaucher", nullptr}},   // WFS-75 : miroir des commandes tactiles
    {"reglages.vibrations", 2, {"reglages.vibrations.activees", "reglages.vibrations.coupees", nullptr}},  // WFS-75 : vibrer() de WFS-70
    {"reglages.direction", 2, {"reglages.direction.glisser", "reglages.direction.fleches", nullptr}},     // WFS-80 : steerMode du web (ModeDirection de WFS-70)
    {"reglages.mouvements", 2, {"reglages.mouvements.non", "reglages.mouvements.oui", nullptr}},   // WFS-80 : reduceMotion du web
    {"reglages.boutons", 2, {"reglages.boutons.normaux", "reglages.boutons.grands", nullptr}},             // WFS-108 RG-04 : taille des commandes tactiles (× 1,25)
    {"reglages.gyro", 4, {"reglages.gyro.coupe", "reglages.gyro.doux", "reglages.gyro.moyen", "reglages.gyro.fort"}},       // WFS-108 RG-03 : sensibilité (28°, 20°, 14° pour braquer à fond)
    {"reglages.gyrozone", 3, {"reglages.gyrozone.petite", "reglages.gyrozone.moyenne", "reglages.gyrozone.grande"}},    // WFS-108 RG-03 : 1,5°, 3°, 5°
    {"reglages.aide", 2, {"reglages.aide.activee", "reglages.aide.desactivee", nullptr}},   // WFS-217 RG-02 : reglagesConduite (player.h)
    {"reglages.regard", 3, {"reglages.regard.bas", "reglages.regard.normal", "reglages.regard.haut"}},   // WFS-235 RG-03 : regardCockpit (camera_vue.h), clé regard
    {"reglages.fantome", 2, {"reglages.fantome.affiche", "reglages.fantome.cache", nullptr}},   // WFS-318 RG-01 : fantome::affiche (fantome.h), clé fantome
    {"reglages.langue", 3, {"reglages.langue.auto", "reglages.langue.fr", "reglages.langue.en"}},   // WFS-504 RG-02 : ui::langue::choisir, clé langue
};
// lignes et onglets : reglages_onglets.h (WFS-291, WFS-318, testé par tests/test_reglages_onglets.cpp)

// ---------------------------------------------------------------- WFS-419 : menu caché de diagnostic de performance
// 5 touchers sur le titre « Réglages » en moins de 3 s ouvrent, par-dessus le panneau, la liste des interrupteurs (un toucher :
// allumé / éteint), l'appareil (RG-03), le temps GPU de chaque passe (RG-04) et l'explication des « 0,0 ». Rien n'est enregistré.
Zone g_titre, g_diagLigne[diag::N_INTERRUPTEURS], g_diagFermer, g_diagTout, g_diagBanc;   // WFS-420 : g_diagBanc, « Banc auto »
// WFS-546 : mode testeur : bouton étoile (+2500 ★ par appui) à gauche de « Banc auto » ; g_etoilesEclat : GameView::time du dernier crédit (bouton éclairé 0,4 s)
Zone g_diagEtoiles; testeur::BoutonEtoiles g_etoiles; double g_etoilesEclat = -10;
bool g_diagOuvert = false;
int g_titreTouchers = 0; double g_titrePremier = -10;
Renderer* g_diagR = nullptr;
double horlogeDiag() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void toucherTitre() {
    const double t = horlogeDiag();
    if (t - g_titrePremier > 3.0) { g_titrePremier = t; g_titreTouchers = 0; }
    if (++g_titreTouchers >= 5) {
        g_titreTouchers = 0; g_titrePremier = -10; g_diagOuvert = true; diag::g_menuVu = true;
        qualite::diagnosticForce = true;   // coût de chaque passe dans la ligne technique (WFS-150 RG-05)
        WFS_LOG("réglages : menu caché de diagnostic ouvert (WFS-419)");
    }
}
// un interrupteur qui change la taille ou le MSAA des cibles les recrée (GPU à l'arrêt dans resize, comme « Graphismes »)
void appliquerDiag(diag::Interrupteur i) {
    if (diag::INFOS[i].recreeCibles && g_diagR) g_diagR->resize();
    WFS_LOG("diagnostic (WFS-419) : %s %s · %s", diag::INFOS[i].libelle, diag::actif(i) ? "COUPÉ" : "rétabli", diag::texteActifs().c_str());
}
bool toucherDiag(int id, float x, float y, double temps) {
    // WFS-546 (RG-02, RG-03) : 2500 ★ par appui, une seule fois par doigt posé (ses mouvements arrivent aussi « posés »), puis la
    // sauvegarde est écrite comme après un achat ; le compteur du menu (WFS-537) relit le solde à chaque image
    if (g_etoiles.poser(id, g_diagEtoiles.contient(x, y), sauvegarde())) {
        if (sauvegarde().actif()) sauvegarde().enregistrer();
        g_etoilesEclat = temps;
        return true;
    }
    if (g_diagEtoiles.contient(x, y)) return true;   // même doigt encore posé sur le bouton étoile : déjà crédité
    if (g_diagFermer.contient(x, y)) { g_diagOuvert = false; g_etoiles.oublier(); WFS_LOG("réglages : menu caché de diagnostic fermé"); return true; }
    if (g_diagBanc.contient(x, y)) { g_diagOuvert = false; g_etoiles.oublier(); banc_telephone::ouvrirProtocole(); return true; }   // WFS-420 : protocole, puis le banc
    if (g_diagTout.contient(x, y)) {
        bool recree = false;
        for (int i = 0; i < diag::N_INTERRUPTEURS; i++) recree |= diag::menu((diag::Interrupteur)i) && diag::INFOS[i].recreeCibles;
        diag::eteindreMenu();
        if (recree && g_diagR) g_diagR->resize();
        WFS_LOG("diagnostic (WFS-419) : menu caché, tout rétabli · %s", diag::texteActifs().c_str());
        return true;
    }
    for (int i = 0; i < diag::N_INTERRUPTEURS; i++) if (g_diagLigne[i].contient(x, y)) {
        diag::basculerMenu((diag::Interrupteur)i); appliquerDiag((diag::Interrupteur)i); return true;
    }
    return true;   // le menu garde tous les touchers
}
bool g_boutonFrais = false;   // WFS-537 (accroche) : « Réglages » dessiné depuis la dernière lecture de boutonReglages
Zone g_bouton, g_fermer, g_choix[N_LIGNES][4], g_mesure, g_cgu;   // WFS-246 (accroche) : g_mesure, ligne de la mesure (toucher : partager) ; WFS-308 : g_cgu, « Conditions d'utilisation »
// WFS-290 (RG-04) : bouton tenu (P_*) et doigt qui le tient ; WFS-549 : P_CREDITS, « Crédits » ouvert au relâcher
enum { P_AUCUN, P_BOUTON, P_FERMER, P_CREDITS };
int g_presse = P_AUCUN, g_doigt = -1;
Zone g_credits;   // WFS-549 (RG-06) : « Crédits » à côté de « Conditions d'utilisation » (menu principal seulement : l'écran Crédits est celui du menu)
bool g_creditsVisible = false;
// WFS-557 (maquettes 14, 24, 25) : « ☆ Crédits » et « 🗎 Conditions d'utilisation » en bas à droite, boutons de verre de 46 px,
// icône à gauche, texte en capitales italiques ; « Crédits » au menu principal seulement. x : bord droit, y : haut.
// Zones de toucher ≥ 44 dp (WFS-308 RG-05). Rend la largeur prise (les deux boutons et leur écart).
float boutonCgu(Renderer& r, float x, float y, float h, float k) {
    const float tp = 19 * k, pad = 22 * k, ic = tp * 0.86f + tp * 0.65f;
    const std::string t = ui::majuscules(ui::tr("reglages.cgu"));
    const float w = largeurMax(tp, "reglages.cgu", 0.10f) + ic + 2 * pad, zh = std::max(h, ui::TOUCHER_MIN_DP * ui::pixelsParDp((float)r.width()));
    g_cgu = {x - w, y + (h - zh) / 2, w, zh};
    ui::boutons::verreIcone(r, x - w, y, w, h, ui::boutons::Icone::Document, t, 1, false, tp, {14 * k, 0, nullptr, 0.10f});
    if (!g_creditsVisible) { g_credits = {}; return w; }
    const std::string tc = ui::majuscules(ui::tr("credits.bouton"));
    const float wc = largeurMax(tp, "credits.bouton", 0.10f) + ic + 2 * pad, ecart = 17 * k;
    g_credits = {x - w - ecart - wc, y + (h - zh) / 2, wc, zh};
    ui::boutons::verreIcone(r, x - w - ecart - wc, y, wc, h, ui::boutons::Icone::Etoile, tc, 1, g_presse == P_CREDITS, tp, {14 * k, 0, nullptr, 0.10f});   // RG-08
    return w + ecart + wc;
}
// WFS-290 (RG-03, ANO-159) : cible d'appui de 48 dp et anti-rebond de 0,3 s : reglages_appui.h (calculs testés). Sur téléphone,
// « Réglages » est dessiné en 82 × 25 dp et « Fermer » en 88 × 29 dp : un doigt posé un peu à côté du dessin ne comptait pas
// (banc reglages-appuis : 36 appuis sur 100 dans la cible d'un doigt) ; « Fermer » est presque à la place de « Réglages »
float g_dpAppui = 1;   // pixels par dp, relevé à chaque image dessinée
bool dansCible(const Zone& z, float x, float y) { return appui::dansCible(z.x, z.y, z.w, z.h, g_dpAppui, x, y); }
double g_horlogeBanc = -1;   // banc : horloge simulée (secondes) ; sinon horloge du téléphone
double maintenant() {
    if (g_horlogeBanc >= 0) return g_horlogeBanc;
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
double g_dernierBasculement = -1e9;
bool basculementPermis() { return appui::basculementPermis(maintenant(), g_dernierBasculement); }
// WFS-290 (RG-04) : appui comme les choix du menu : le bouton s'éclaircit tant que le doigt est posé, l'action se fait au
// relâcher dans la cible ; un doigt qui glisse hors de la cible annule l'appui
int g_onglet = O_AFFICHAGE;   // RG-03 : dernier onglet ouvert, gardé (clé « onglet » du fichier des réglages)
Zone g_onglets[N_ONGLETS];
// WFS-557 (maquette 14) : sélecteur de verre des onglets, icône (écran, volant, haut-parleur) + texte Inter en capitales ;
// largeur de chaque onglet = son contenu + 24 px de chaque côté ; (x, y) : coin haut gauche ; rend la largeur totale.
// Zones de toucher = les onglets dessinés.
const ui::boutons::Icone ICONES_ONGLET[N_ONGLETS] = {ui::boutons::Icone::Ecran, ui::boutons::Icone::Volant, ui::boutons::Icone::HautParleur};
float dessinerOnglets(Renderer& r, float x, float y, float h, float tp, float k) {
    const float in = 4 * k, ic = tp * 1.05f, ecart = 10 * k, pad = 24 * k;
    float ow[N_ONGLETS], total = 2 * in;
    { ui::AvecPolice p(ui::Police::InterMedium);
      for (int o = 0; o < N_ONGLETS; o++) { ow[o] = ic + ecart + ui::width(tp, ui::majuscules(ui::tr(ONGLETS_CLE[o])), 0.10f) + 2 * pad; total += ow[o]; } }
    ui::boutons::selecteurFond(r, x, y, total, h);
    float ox = x + in;
    for (int o = 0; o < N_ONGLETS; o++) {
        const bool on = o == g_onglet;
        g_onglets[o] = {ox, y, ow[o], h};
        ui::boutons::selecteurChoix(r, ox, y + in, ow[o], h - 2 * in, "", on);   // pilule blanche de l'onglet ouvert
        const vec4 c = on ? hex(BG) : hex(0xD7DAE2);
        ui::boutons::icone(r, ICONES_ONGLET[o], ox + pad + ic / 2, y + h / 2, ic, c);
        ui::AvecPolice p(ui::Police::InterMedium);
        ui::text(r, ox + pad + ic + ecart, y + (h - tp) / 2, tp, ui::majuscules(ui::tr(ONGLETS_CLE[o])), c, ui::GAUCHE, 0.10f, false);
        ox += ow[o];
    }
    return total;
}
int nChoix(int l) { return l == L_QUALITE && qualite::ultraPropose() ? 4 : LIGNES[l].n; }   // WFS-150 : Ultra proposé (qualite_auto.h)
bool g_ouvert = false;
std::string g_journalEcran;   // WFS-504 : banc « textes » : nom de l'écran dont les textes sont notés (vide : rien)

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
        ui::langue::choisir(ui::langue::choixDeCle(s.langue));   // WFS-504 (RG-04) : absente → auto
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
        else if (!std::strcmp(cle, "fantome")) fantome::affiche() = v != 0;   // WFS-318 (RG-01) : Affiché par défaut
        else if (!std::strcmp(cle, "langue")) { if (!commune) ui::langue::choisir((ui::langue::Choix)std::clamp(v, 0, 2)); }   // WFS-504 (RG-04) : banc sans sauvegarde commune
        else if (!std::strcmp(cle, "onglet")) g_onglet = v >= 0 && v < N_ONGLETS ? v : O_AFFICHAGE;   // WFS-291 (RG-03) : inconnu → Affichage
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
        s.langue = ui::langue::cleDeChoix(ui::langue::choix());   // WFS-504 (RG-04)
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
    std::fprintf(fp, "onglet=%d\nfantome=%d\nlangue=%d\n", g_onglet, (int)fantome::affiche(), (int)ui::langue::choix());   // WFS-504 : langue   // WFS-318 : fantome   // WFS-291 (RG-03) : pas un réglage, la place où on en était
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
    case L_FANTOME: return fantome::affiche() ? 0 : 1;   // WFS-318
    case L_LANGUE: return (int)ui::langue::choix();   // WFS-504
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
    // WFS-290 : la zone du bouton n'est plus effacée au changement d'écran (même place au menu et en pause) : un appui juste après
    // l'arrivée sur l'écran, avant la 1re image dessinée, comptait pour rien (banc : 0 sur 100) ; hors menu et pause, onTouch l'ignore
    void onEcran(Ecran, Ecran, const GameView&) override { g_diagOuvert = false; g_etoiles.oublier(); g_ouvert = false; g_presse = P_AUCUN; g_doigt = -1; cgu().fermer(); }   // WFS-291 : g_doigt aussi (remarque du lead dev, PR #287)
    void shutdown(Renderer&) override { if (cgu().mode() == Cgu::Mode::Lecture) cgu().fermer(); }   // WFS-308 : fenêtre perdue en jeu (termGraphics)

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Menu && g.ecran != Ecran::Pause) return false;
        if (!g_ouvert) {   // bouton « Réglages » (RG-04 : menu principal et pause)
            if (t.down) {
                if (t.id == g_doigt) { if (g_presse == P_BOUTON && !dansCible(g_bouton, t.x, t.y)) g_presse = P_AUCUN; return true; }   // glissé hors : annulé
                if (g_doigt < 0 && dansCible(g_bouton, t.x, t.y)) { g_doigt = t.id; g_presse = P_BOUTON; return true; }   // WFS-290 : cible de 48 dp
                return false;
            }
            if (t.id != g_doigt) return false;
            const bool vise = g_presse == P_BOUTON && dansCible(g_bouton, t.x, t.y);
            g_doigt = -1; g_presse = P_AUCUN;
            if (!vise) return true;
            if (!basculementPermis()) { WFS_LOG("réglages : appui ignoré (moins de 0,3 s après la fermeture)"); return true; }   // WFS-290 : anti-rebond
            g_ouvert = true; g_dernierBasculement = maintenant(); WFS_LOG("réglages : ouverts (%s)", nomEcran(g.ecran));
            return true;
        }
        if (!t.down) {
            g_etoiles.lever(t.id);   // WFS-546 : doigt levé, le prochain appui sur le bouton étoile crédite de nouveau
            if (t.id == g_doigt) {   // WFS-290 : « Fermer » au relâcher, dans sa cible
                const bool vise = g_presse == P_FERMER && dansCible(g_fermer, t.x, t.y);
                const int presse = g_presse;
                g_doigt = -1; g_presse = P_AUCUN;
                if (vise && !basculementPermis()) WFS_LOG("réglages : appui ignoré (moins de 0,3 s après l'ouverture)");
                else if (vise) { g_ouvert = false; g_dernierBasculement = maintenant(); WFS_LOG("réglages : fermés"); }
                // WFS-549 (RG-06, revue #420) : « Crédits » au relâcher dans sa zone, doigt déjà libéré (le Menu, avant les Réglages,
                // garde ensuite tous les touchers de l'écran Crédits : ouvrir à l'appui laissait g_doigt bloqué et perdait l'appui suivant)
                else if (presse == P_CREDITS && g_credits.contient(t.x, t.y) && g.ecran == Ecran::Menu) { WFS_LOG("réglages : « Crédits »"); credits::ouvrir(); }
            }
            if (cgu().ouvert()) cgu().toucher(t.id, t.x, t.y, false);
            return true;
        }
        if (t.id == g_doigt) {   // doigt encore posé (glissé hors : annulé)
            if ((g_presse == P_FERMER && !dansCible(g_fermer, t.x, t.y)) || (g_presse == P_CREDITS && !g_credits.contient(t.x, t.y))) g_presse = P_AUCUN;
            return true;
        }   // doigt encore posé (glissé hors : annulé)
        if (cgu().ouvert()) { cgu().toucher(t.id, t.x, t.y, t.down); return true; }   // WFS-308 : lecture des conditions par-dessus
        if (banc_telephone::protocoleOuvert()) return banc_telephone::toucherProtocole(t.x, t.y);   // WFS-420 : par-dessus le panneau
        if (banc_telephone::resultatsOuverts()) return banc_telephone::toucherResultats(t.x, t.y);
        if (g_diagOuvert) return toucherDiag(t.id, t.x, t.y, g.time);   // WFS-419 : menu caché par-dessus le panneau
        if (g_titre.contient(t.x, t.y)) { toucherTitre(); return true; }   // WFS-419 : 5 touchers ouvrent le menu caché
        if (g_cgu.contient(t.x, t.y) && r_) { cgu().ouvrir(*r_, Cgu::Mode::Lecture); g_doigt = t.id; return true; }   // WFS-308 (RG-05)
        if (g.ecran == Ecran::Menu && g_credits.contient(t.x, t.y)) { g_doigt = t.id; g_presse = P_CREDITS; return true; }   // WFS-549 (RG-06) : ouvert au relâcher
        if (dansCible(g_fermer, t.x, t.y)) { g_doigt = t.id; g_presse = P_FERMER; return true; }   // WFS-290 : fermé au relâcher
        if (g_mesure.contient(t.x, t.y) && !qualite::texteMesure().empty()) { g_doigt = t.id; WFS_LOG("réglages : mesure du rendu partagée (WFS-246)"); reseau::partager(qualite::texteMesure()); return true; }
        for (int o = 0; o < N_ONGLETS; o++) if (g_onglets[o].contient(t.x, t.y)) {   // WFS-291 : changement d'onglet, gardé
            if (o != g_onglet) { g_onglet = o; WFS_LOG("réglages : onglet %s", ONGLETS[o]); ecrire(); }
            return true;
        }
        for (int l = 0; l < N_LIGNES; l++) for (int c = 0; c < nChoix(l); c++) {
            if (!dansOnglet(l, g_onglet) || !g_choix[l][c].contient(t.x, t.y) || valeur(l) == c) continue;   // WFS-291 : onglet ouvert seul
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
            else if (l == L_FANTOME) fantome::affiche() = c == 0;   // WFS-318 (RG-01, RG-02) : appliqué à l'image suivante
            else if (l == L_LANGUE) ui::langue::choisir((ui::langue::Choix)c);   // WFS-504 (RG-03) : l'image suivante est dans la langue choisie
            else reglagesRessenti().reduireMouvements = c == 1;   // WFS-80 (RG-02) : clé mouvements
            WFS_LOG("réglages : %s = %s", LIGNES[l].libelle, LIGNES[l].choix[c]);
            ecrire();
        }
        return true;   // RG-03 : le panneau garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        // WFS-504 (RG-07) : banc « textes » : seuls les textes dessinés ici sont notés (pas ceux du menu ou de la pause dessous)
        struct Journal { Journal() { if (!g_journalEcran.empty()) ui::langue::journalActif(true, g_journalEcran.c_str()); }
                         ~Journal() { if (!g_journalEcran.empty()) ui::langue::journalActif(false); } } journal;
        if (!ui::fontReady() || g.sceneDediee || (g.ecran != Ecran::Menu && g.ecran != Ecran::Pause)) return;
        if (creationProfilOuverte()) return;   // WFS-481 (accroche, RG-02) : pas de bouton « Réglages » sur l'écran « Crée ton pilote »
        // WFS-147 (RG-01, RG-02, ANO-19) : sur téléphone couché, mise en page propre aux réglages, dessinée en dp
        const bool cp = ui::petitEcran(g.uiWidth, g.uiHeight);
        const float W = g.uiWidth, H = g.uiHeight, k = cp ? ui::unitePetitEcran(W, H) : H / 720.f;
        g_dpAppui = ui::pixelsParDp(W);   // WFS-290 : cible des boutons en dp
        if (!g_ouvert && verre::couvert()) { g_bouton = {}; return; }   // WFS-548 : sous une fenêtre de verre (Profil, Market…) : pas de bouton
        if (!g_ouvert && g.ecran == Ecran::Menu && choixModeOuvert()) { g_bouton = {}; return; }   // WFS-557 (maquette 02) : pas de « Réglages » sur le choix du mode
        if (!g_ouvert) { dessinerBouton(r, g, W, k, H / 720.f); manette::focus(g_bouton); return; }   // WFS-80 : bouton au focus
        g_creditsVisible = g.ecran == Ecran::Menu;   // WFS-549 (RG-06) : « Crédits » au menu principal (l'écran Crédits est celui du menu)
        // WFS-80 (RG-05) : panneau ouvert → seuls ses choix et « Fermer » (B) gardent le focus, pas le menu dessous
        manette::zonesImage().clear();
        // WFS-548 : sous les conditions d'utilisation, le panneau n'est pas dessiné (il se voyait à travers leur fenêtre de verre)
        if (!cgu().ouvert()) dessinerPanneau(r, g, W, H);   // WFS-557 : une seule mise en page (maquettes 14, 24, 25)
        for (int o = 0; o < N_ONGLETS; o++) manette::focus(g_onglets[o]);   // WFS-291 : la manette passe d'un onglet à l'autre
        for (int l = 0; l < N_LIGNES; l++) for (int c = 0; c < nChoix(l); c++) if (dansOnglet(l, g_onglet)) manette::focus(g_choix[l][c]);
        manette::focus(g_fermer, true);
        if (cgu().ouvert()) cgu().dessiner(r, W, H, g.marges, g.time);   // WFS-308 (RG-05) : par-dessus le panneau
        g_diagR = &r;
        if (g_diagOuvert) dessinerDiagnostic(r, W, H, cp ? k : ui::unitePetitEcran(W, H), g.time);   // WFS-419 : menu caché
        if (banc_telephone::protocoleOuvert()) banc_telephone::dessinerProtocole(r, W, H, cp ? k : ui::unitePetitEcran(W, H));   // WFS-420
        else if (banc_telephone::resultatsOuverts()) banc_telephone::dessinerResultats(r, W, H, cp ? k : ui::unitePetitEcran(W, H));
    }

private:
    // .cam-btn : vitré, bordure 1 px, biais 8 px, 44 px de haut, « RÉGLAGES » ; en haut à droite de la zone sûre
    // WFS-168 (RG-01) : taille du bouton d'avant WFS-147 (unité kb = H / 720, comme à 349f7d0) ; texte et place gardés
    void dessinerBouton(Renderer& r, const GameView& g, float W, float k, float kb) {
        if (g.ecran == Ecran::Pause) { dessinerPilulePause(r, g, W, kb); return; }   // WFS-557 (maquette 19)
        if (g.ecran == Ecran::Menu) { dessinerEngrenage(r, g, W, kb); return; }   // WFS-550 (RG-11, maquette 01) : bouton rond
        const float h = 44 * kb, tp = 15 * k, pad = 18 * kb;
        const std::string t = ui::majuscules(ui::tr("reglages.bouton"));   // WFS-504
        const float tw = largeurMax(tp, "reglages.bouton", 0.12f), dx = (tw - ui::width(tp, t, 0.12f)) / 2;   // RC-E20-04 : taille fixe
        const float w = pad + tw + pad, x = W - g.marges.droite - 16 * k - w, y = g.marges.haut + 14 * k;
        g_bouton = {x, y, w, h};
        g_boutonFrais = true;   // WFS-537 (accroche) : compteur d'étoiles posé à cette image
        biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, BORD);
        biseau(r, x, y, w, h, 8 * kb, GLASS);
        if (g_presse == P_BOUTON) biseau(r, x, y, w, h, 8 * kb, hex(ACCENT2, 0.22f));   // WFS-290 (RG-04) : appuyé, voile cyan
        ui::text(r, std::round(x + pad + dx), std::round(y + (h - tp) / 2), tp, t, hex(ACCENT2), ui::GAUCHE, 0.12f, false);
    }
    // WFS-550 (RG-11, maquette 01) : au menu, « Réglages » est un bouton de verre rond (45 px) avec un engrenage, à 32 px du
    // bord droit et 22 px du haut ; zone de toucher ≥ 48 dp par appui::dansCible (comme avant) ; ailleurs, bouton d'avant
    void dessinerEngrenage(Renderer& r, const GameView& g, float W, float kb) {
        const float d = 45 * kb, x = W - g.marges.droite - 32 * kb - d, y = g.marges.haut + 22 * kb, cx = x + d / 2, cy = y + d / 2;
        g_bouton = {x, y, d, d};
        g_boutonFrais = true;   // WFS-537 (accroche) : compteur d'étoiles posé à cette image
        dessin2d::disqueRadial(r, cx, cy, d / 2, cx, cy - d * 0.2f, d * 0.6f, hex(0x2A2D36, 0.9f), hex(0x14161F, 0.9f));
        dessin2d::anneau(r, cx, cy, d / 2 - 1.2f * kb, d / 2, vec4{1, 1, 1, 0.22f});
        if (g_presse == P_BOUTON) dessin2d::disqueRadial(r, cx, cy, d / 2, cx, cy, d / 2, hex(ACCENT2, 0.25f), hex(ACCENT2, 0.15f));
        // engrenage au trait : anneau, 8 dents, moyeu
        const vec4 c{242 / 255.f, 244 / 255.f, 248 / 255.f, 0.95f};
        // WFS-557 (maquette 01 au zoom) : engrenage fin et petit (≈ 16 px sur 45) : anneau, 8 dents courtes, moyeu en point
        const float ra = d * 0.11f, ep = std::max(1.1f, d * 0.03f);
        dessin2d::anneau(r, cx, cy, ra - ep / 2, ra + ep / 2, c);
        for (int i = 0; i < 8; i++) {
            const float an = i * 0.785398f, sx = std::sin(an), sy = -std::cos(an);
            const vec2 dent[2] = {{cx + sx * (ra + ep * 1.2f), cy + sy * (ra + ep * 1.2f)}, {cx + sx * (ra + ep * 2.8f), cy + sy * (ra + ep * 2.8f)}};
            dessin2d::trait(r, dent, 2, ep, c);
        }
        dessin2d::anneau(r, cx, cy, 0, ra * 0.35f, c);
    }
    // WFS-557 (maquette 19, arbitrage du PO du 2026-11-16) : en pause, « ⚙ RÉGLAGES » en bouton de verre arrondi, 155 × 51, à 31 px
    // du bord droit et 22 px du haut ; zone de toucher : la cible de 48 dp (dansCible). Au menu : bouton de WFS-550 (maquette 01).
    void dessinerPilulePause(Renderer& r, const GameView& g, float W, float kb) {
        const std::string t = ui::majuscules(ui::tr("reglages.bouton"));   // WFS-504
        const float h = 51 * kb, w = 155 * kb, x = W - g.marges.droite - 31 * kb - w, y = g.marges.haut + 22 * kb;
        g_bouton = {x, y, w, h};
        g_boutonFrais = true;   // WFS-537 (accroche) : compteur d'étoiles posé à cette image
        ui::boutons::verreIcone(r, x, y, w, h, ui::boutons::Icone::Engrenage, t, 1, g_presse == P_BOUTON, 20 * kb, {12 * kb, 0, nullptr, 0.10f});
    }
    // WFS-557 (maquettes 14, 24, 25 ; arbitrage du PO du 2026-11-16 : maquette à 100 %, boutons compris). Une seule mise en page,
    // en unités de la maquette (1600 × 720, k = H / 720), pour le téléphone (2400 × 1080) comme pour le PC : fenêtre de verre
    // pleine page (16 px du bord), sur-titre et « RÉGLAGES » en haut à gauche, onglets en sélecteur de verre au milieu du haut,
    // « ✕ Fermer » en haut à droite, lignes de l'onglet en deux colonnes (4 au plus par colonne, la gauche d'abord), ligne
    // « Rendu · Mémoire » en Inter grise en bas à gauche, « Crédits » et « Conditions d'utilisation » en bas à droite.
    void dessinerPanneau(Renderer& r, const GameView& g, float W, float H) {
        verre::fenetreSurEcran(r);   // menu ou pause dessous : leur interface n'est plus dessinée (verre::couvert)
        verre::assombrir(r, W, H, 0.55f);
        const float k = H / 720.f;
        const Marges& m = g.marges;
        const float px = m.gauche + 16 * k, py = m.haut + 16 * k, pw = W - m.gauche - m.droite - 32 * k, ph = H - m.haut - m.bas - 32 * k;
        r.uiRect(0, 0, W, H, hex(0x0B0D12, 0.80f));   // maquettes 14, 24, 25 : page sombre opaque (#0B0D12), la scène ne se voit plus
        verre::panneau(r, px, py, pw, ph, {10 * k, 1.5f * k});
        const float x0 = px + 40 * k, xd = px + pw - 41 * k;   // bords intérieurs gauche et droit
        verre::surTitre(r, x0, py + 27 * k, 16 * k, ui::tr(g.ecran == Ecran::Pause ? "reglages.surtitre.pause" : "reglages.surtitre.menu"));
        const std::string titre = ui::majuscules(ui::tr("reglages.titre"));
        ui::text(r, x0, py + 46 * k, 52 * k, titre, hex(TEXTE), ui::GAUCHE, 0.01f, false, true);
        g_titre = {x0, py + 46 * k, ui::width(52 * k, titre, 0.01f), 52 * k};   // WFS-419 : 5 touchers → menu caché
        for (int l = 0; l < N_LIGNES; l++) for (int c = 0; c < 4; c++) g_choix[l][c] = {};
        dessinerOnglets(r, px + 415 * k, py + 30 * k, 50 * k, 16 * k, k);
        {   // « ✕ Fermer » : 143 × 50 en haut à droite
            const float tp = 24 * k, fw = largeurMax(tp, "reglages.fermer", 0.10f) + tp * 1.4f + 48 * k, fh = 50 * k;
            g_fermer = {xd - fw, py + 31 * k, fw, fh};
            ui::boutons::verreOrange(r, g_fermer.x, g_fermer.y, fw, fh, ui::majuscules(ui::tr("reglages.fermer")), 1, g_presse == P_FERMER, tp);
        }
        // lignes : deux colonnes de 699 px (gauche 56 → 755, droite 836 → 1543 sur la maquette), rangs de 85 px
        const int n = nLignesOnglet(g_onglet), parCol = n <= 4 ? 4 : (n + 1) / 2;
        const float colW = (xd - x0 - 81 * k) / 2, rangH = 85 * k, choixH = 44 * k, y0 = py + 165 * k - choixH / 2;
        for (int i = 0; i < n; i++) {
            const int l = LIGNES_ONGLET[g_onglet][i], col = i / parCol, rang = i % parCol;
            const float cx = x0 + col * (colW + 81 * k), cy = y0 + rang * rangH;
            const int nCol = col == 0 ? std::min(n, parCol) : n - parCol;
            if (rang < nCol - 1) r.uiRect(cx, cy + choixH / 2 + rangH / 2, colW, 1 * k, hex(TEXTE, 0.08f));   // séparateur entre deux rangs
            dessinerLibelle(r, cx, cy + choixH / 2, ui::majuscules(ui::tr(LIGNES[l].libelle)), k);
            // choix : largeur de chaque pilule = son texte + 20 px de chaque côté ; rangée calée sur le bord droit de la colonne
            const int nc = nChoix(l);
            const float tp = 16 * k, in = 4 * k, padC = 20 * k;
            float cw[4], total = 2 * in;
            { ui::AvecPolice p(ui::Police::InterMedium);
              for (int c = 0; c < nc; c++) { cw[c] = ui::width(tp, ui::majuscules(ui::tr(LIGNES[l].choix[c])), 0.06f) + 2 * padC; total += cw[c]; } }
            float bx = cx + colW - total;
            ui::boutons::selecteurFond(r, bx, cy, total, choixH);
            bx += in;
            for (int c = 0; c < nc; c++) {
                const bool on = valeur(l) == c;
                g_choix[l][c] = {bx, cy, cw[c], choixH};   // zone de toucher : toute la hauteur de la rangée (44 px de la maquette)
                ui::boutons::selecteurChoix(r, bx, cy + in, cw[c], choixH - 2 * in, "", on);
                ui::AvecPolice p(ui::Police::InterMedium);
                ui::text(r, bx + cw[c] / 2, cy + (choixH - tp) / 2, tp, ui::majuscules(ui::tr(LIGNES[l].choix[c])), on ? hex(BG) : hex(TEXTE), ui::CENTRE, 0.06f, false);
                bx += cw[c];
            }
            for (int c = nc; c < 4; c++) g_choix[l][c] = {};
        }
        // bas : « Crédits » et « Conditions d'utilisation » (46 px) ; ligne technique en Inter grise à gauche, sur deux lignes au plus
        const float bh = 46 * k, by = py + ph - 24 * k - bh;
        const float pris = boutonCgu(r, xd, by, bh, k);
        ui::langue::journalTechnique(true);   // WFS-504 : lignes de mesure (src/render, hors de cette zone), notées à part
        const std::string lr = qualite::ligneRendu() + " · " + texteMemoire(false);
        const float lw = xd - pris - 60 * k - x0, tl = 13 * k;
        {
            ui::AvecPolice p(ui::Police::Inter);
            std::string l1 = lr, l2;
            if (ui::width(tl, lr, 0) > lw) {   // coupe au dernier « · » ou espace qui tient
                size_t coupe = std::string::npos;
                for (size_t i = lr.find(' '); i != std::string::npos; i = lr.find(' ', i + 1))
                    if (ui::width(tl, lr.substr(0, i), 0) <= lw) coupe = i; else break;
                if (coupe != std::string::npos) { l1 = lr.substr(0, coupe); l2 = lr.substr(coupe + 1); }
            }
            const float yb = by + bh / 2 + (l2.empty() ? -tl / 2 : -tl - 1 * k);
            ui::text(r, x0, yb, tl, l1, hex(MUTED), ui::GAUCHE, 0, false);
            if (!l2.empty()) ui::text(r, x0, yb + tl + 3 * k, tl, l2, hex(MUTED), ui::GAUCHE, 0, false);
            // WFS-246 (accroche, RG-06) : ligne de la dernière mesure (toucher : partager), au-dessus de la ligne technique
            const std::string lm = qualite::ligneMesure();
            if (!lm.empty()) { const float tm = std::min(tl, tl * lw / std::max(ui::width(tl, lm, 0.f), 1.f));
                               g_mesure = {x0, yb - tl - 10 * k, lw, tl + 8 * k}; ui::text(r, x0, yb - tl - 6 * k, tm, lm, hex(TEXTE, 0.85f), ui::GAUCHE, 0.f, false); }
            else g_mesure = {};
        }
        ui::langue::journalTechnique(false);
    }

    // libellé d'une ligne (Inter Medium, capitales espacées, gris) centré sur ym ; sur deux lignes au-delà de 210 px (maquette 14 :
    // « Réduire les / mouvements »)
    void dessinerLibelle(Renderer& r, float x, float ym, const std::string& lib, float k) {
        ui::AvecPolice p(ui::Police::InterMedium);
        const float lp = 16 * k, esp = 0.15f;
        if (ui::width(lp, lib, esp) <= 210 * k) { ui::text(r, x, ym - lp / 2, lp, lib, hex(0xA3A8B5), ui::GAUCHE, esp, false); return; }
        size_t coupe = lib.find(' ', lib.size() / 2 - 2);
        if (coupe == std::string::npos) coupe = lib.rfind(' ');
        ui::text(r, x, ym - lp - 1 * k, lp, lib.substr(0, coupe), hex(0xA3A8B5), ui::GAUCHE, esp, false);
        ui::text(r, x, ym + 1 * k, lp, lib.substr(coupe + 1), hex(0xA3A8B5), ui::GAUCHE, esp, false);
    }

    // WFS-419 : menu caché (k = pixels par dp) : interrupteurs en deux colonnes (zones de 44 dp), appareil, temps par passe, explication
    void dessinerDiagnostic(Renderer& r, float W, float H, float k, double temps) {
        r.uiRect(0, 0, W, H, hex(BG, 0.92f));
        const float m = 16 * k, x0 = m, lw = W - 2 * m;
        float y = m;
        ui::text(r, x0, y, 18 * k, ui::majuscules(ui::tr("diag.titre")), hex(ACCENT2), ui::GAUCHE, 0.08f, false);
        {   // « Fermer » et « Tout rétablir » en haut à droite
            const float bh = std::max(44.f * k, 30 * k), tp = 14 * k;
            const std::string ft = ui::majuscules(ui::tr("diag.fermer")), tt = ui::majuscules(ui::tr("diag.tout")), bt = ui::majuscules(ui::tr("diag.banc"));   // WFS-504
            const float fw = ui::width(tp, ft, 0.1f) + 32 * k, tw = ui::width(tp, tt, 0.1f) + 32 * k, bw = ui::width(tp, bt, 0.1f) + 32 * k;
            g_diagFermer = {W - m - fw, y - 6 * k, fw, bh};
            g_diagTout = {W - m - fw - 8 * k - tw, y - 6 * k, tw, bh};
            g_diagBanc = {g_diagTout.x - 8 * k - bw, y - 6 * k, bw, bh};   // WFS-420 : banc de performance automatique
            {   // WFS-546 (RG-01, RG-04, RG-05) : bouton carré à gauche de « Banc auto », même style ; seulement l'icône étoile (aucun
                // texte) ; éclairé 0,4 s après un crédit
                g_diagEtoiles = {g_diagBanc.x - 8 * k - bh, y - 6 * k, bh, bh};
                const bool eclat = temps - g_etoilesEclat >= 0 && temps - g_etoilesEclat < 0.4;
                biseau(r, g_diagEtoiles.x, g_diagEtoiles.y, bh, bh, 10 * k, hex(eclat ? ACCENT_PRESSE : SURFACE2));
                score_fin::dessinerEtoile(r, g_diagEtoiles.x + bh / 2, g_diagEtoiles.y + bh / 2, bh * 0.30f, eclat ? hex(BG) : ui::ORANGE);
            }
            biseau(r, g_diagBanc.x, g_diagBanc.y, bw, bh, 10 * k, hex(ACCENT2));
            ui::text(r, g_diagBanc.x + bw / 2, g_diagBanc.y + (bh - tp) / 2, tp, bt, hex(BG), ui::CENTRE, 0.1f, false);
            biseau(r, g_diagFermer.x, g_diagFermer.y, fw, bh, 10 * k, hex(ACCENT));
            ui::text(r, g_diagFermer.x + fw / 2, g_diagFermer.y + (bh - tp) / 2, tp, ft, hex(BG), ui::CENTRE, 0.1f, false);
            biseau(r, g_diagTout.x, g_diagTout.y, tw, bh, 10 * k, hex(SURFACE2));
            ui::text(r, g_diagTout.x + tw / 2, g_diagTout.y + (bh - tp) / 2, tp, tt, hex(TEXTE), ui::CENTRE, 0.1f, false);
            y += bh + 2 * k;
        }
        constexpr int PAR_COL = (diag::N_INTERRUPTEURS + 1) / 2;
        const float colGap = 12 * k, colW = (lw - colGap) / 2, rowH = 44 * k, gap = 2 * k;
        for (int i = 0; i < diag::N_INTERRUPTEURS; i++) {
            const int col = i / PAR_COL, rang = i % PAR_COL;
            const float cx = x0 + col * (colW + colGap), cy = y + rang * (rowH + gap);
            const bool on = diag::actif((diag::Interrupteur)i);
            g_diagLigne[i] = {cx, cy, colW, rowH};
            r.uiRect(cx, cy, colW, rowH, on ? hex(ACCENT, 0.22f) : hex(SURFACE2));
            if (on) r.uiRect(cx, cy + rowH - 3 * k, colW, 3 * k, hex(ACCENT));
            const std::string etat = ui::majuscules(ui::tr(!on ? "diag.actif" : diag::menu((diag::Interrupteur)i) ? "diag.coupe" : "diag.coupe_json"));   // WFS-504
            const float tp = 14 * k, ew = ui::width(tp, etat, 0.04f);
            const std::string lib = diag::INFOS[i].libelle;
            const float tl = std::min(tp, tp * (colW - ew - 30 * k) / std::max(ui::width(tp, lib, 0.02f), 1.f));
            ui::text(r, cx + 10 * k, cy + (rowH - tl) / 2, tl, lib, hex(TEXTE), ui::GAUCHE, 0.02f, false);
            ui::text(r, cx + colW - 10 * k - ew, cy + (rowH - tp) / 2, tp, etat, on ? hex(ACCENT) : hex(MUTED), ui::GAUCHE, 0.04f, false);
        }
        y += PAR_COL * (rowH + gap) + 6 * k;
        // lignes d'information (réduites si elles ne tiennent pas dans la largeur)
        auto ligne = [&](const std::string& t, const vec4& c) {
            const float tp = std::min(12 * k, 12 * k * lw / std::max(ui::width(12 * k, t, 0.f), 1.f));
            ui::text(r, x0, y, tp, t, c, ui::GAUCHE, 0.f, false); y += 15 * k;
        };
        ligne(diag::texteAppareil(r.vk()), hex(ACCENT2));
        ligne(qualite::ligneRendu(), hex(TEXTE));
        const std::string lm = qualite::ligneMesure();
        ligne(lm.empty() ? ui::tr("diag.mesure_vide") : lm, hex(TEXTE));
        ligne(diag::explicationZero(r.qualiteBasse, passes::horodatagesDisponibles()), hex(MUTED));
    }

    Renderer* r_ = nullptr;
};

} // namespace

Reglages& reglages() { static Reglages R; return R; }
bool reglagesOuverts() { return g_ouvert; }
void ouvrirReglagesBanc(bool oui) { g_ouvert = oui; }   // WFS-297
void ouvrirMenuCacheBanc(bool oui) { g_diagOuvert = oui; if (oui) diag::g_menuVu = true; }   // WFS-420 : banc PC (capture du menu caché)
bool boutonEtoilesDiag(float& x, float& y) { x = g_diagEtoiles.x + g_diagEtoiles.w / 2; y = g_diagEtoiles.y + g_diagEtoiles.h / 2; return g_diagOuvert && g_diagEtoiles.w > 0; }   // WFS-546
bool boutonReglages(float& x, float& y, float& w, float& h) { if (!g_boutonFrais || g_bouton.w <= 0) return false; g_boutonFrais = false; x = g_bouton.x; y = g_bouton.y; w = g_bouton.w; h = g_bouton.h; return true; }   // WFS-537 (accroche)
bool toucheReglages(float x, float y, int doigt) { return g_ouvert || dansCible(g_bouton, x, y) || (doigt >= 0 && doigt == g_doigt); }   // WFS-290 : même cible que onTouch, et le doigt posé sur « Réglages »
bool boutonCguReglages(float& x, float& y) { x = g_cgu.x + g_cgu.w / 2; y = g_cgu.y + g_cgu.h / 2; return g_ouvert && g_cgu.w > 0; }   // WFS-308

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
    auto zoneDe = [&](const Zone& c) {   // index de la zone de focus de la manette qui correspond à c
        int k = -1; const std::vector<manette::Zone>& zs = manette::zonesPrecedentes();
        for (int i = 0; i < (int)zs.size(); i++) if (std::fabs(zs[i].x - c.x) < 0.5f && std::fabs(zs[i].y - c.y) < 0.5f) k = i;
        return k;
    };
    if (!dansOnglet(L_DIRECTION, g_onglet)) {   // WFS-291 : la manette passe à l'onglet « Conduite », puis A
        aller(zoneDe(g_onglets[O_CONDUITE])); appui(manette::A, 6);
        WFS_LOG("manette (WFS-291) : A sur l'onglet « Conduite » → onglet %s", ONGLETS[g_onglet]);
    }
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
    // WFS-291 : un choix se touche dans son onglet (l'onglet est d'abord touché s'il n'est pas ouvert, comme un joueur)
    auto choix = [&](int id, int l, int c) {
        if (!dansOnglet(l, g_onglet)) { toucher(90 + id, g_onglets[ongletDe(l)]); g.render(t); }
        toucher(id, g_choix[l][c]);
    };
    for (int i = 0; i < 30; i++) image();
    g.allerA(Ecran::Menu);
    for (int i = 0; i < 60; i++) image();
    g.render(t); toucher(1, g_bouton);
    capture("reglages-menu", "panneau ouvert depuis le menu");
    choix(2, L_CAMERA, CAM_CAPOT); toucher(3, g_fermer);
    WFS_LOG("réglages : vue = %s (Capot attendu)", VUES[(int)vueCamera()].nom);
    // course (départ du menu), puis pause → réglages
    g.allerA(Ecran::Course);
    for (int i = 0; i < 60 * 5; i++) image();
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(4, g_bouton);
    capture("reglages-pause", "panneau ouvert depuis la pause, vue Capot choisie");
    // qualité basse : scène à 50 %, sans ombres du soleil ; capture en course, puis retour à Haute et Proche
    choix(5, L_QUALITE, QUALITE_BASSE); choix(6, L_CAMERA, CAM_POURSUITE); toucher(7, g_fermer);
    g.allerA(Ecran::Course);
    for (int i = 0; i < 60; i++) image();
    capture("reglages-basse", "course en qualité basse");
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(8, g_bouton);
    choix(9, L_QUALITE, QUALITE_HAUTE); choix(11, L_MUSIQUE, 1);   // WFS-76 : « Musique : Coupée »
    capture("reglages-musique", "musique coupée"); choix(12, L_MUSIQUE, 0); toucher(10, g_fermer);
    // WFS-75 : depuis la pause, « Commandes : Gaucher » et « Vibrations : Coupées », appliqués sans redémarrer
    g.allerA(Ecran::Pause); image(); g.render(t); toucher(13, g_bouton);
    choix(14, L_COMMANDES, 1); choix(15, L_VIBRATIONS, 1);
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
    choix(22, L_DIRECTION, 1);
    WFS_LOG("réglages : direction du ressenti = %s, steer sauvegardé = %s (Flèches / fleches attendus)",
            reglagesRessenti().direction == ModeDirection::Fleches ? "Flèches" : "Glisser", sauvegarde().steer.c_str());
    choix(23, L_MOUVEMENTS, 1);
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
    choix(25, L_BOUTONS, 1); choix(26, L_GYRO, 2);
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

// WFS-290 (RG-01, RG-02) : banc des appuis sur « Réglages » et « Fermer », au menu principal et en pause : 100 essais par cas,
// comme un joueur (appuis injectés par Game::onTouch, une image dessinée après chaque relâcher, comme le téléphone) :
//   centre        appui court au centre du dessin ;
//   dessin        appui court n'importe où sur le dessin du bouton (bords compris) ;
//   doigt 48 dp   appui court n'importe où dans la cible d'un doigt (dessin élargi à 48 dp de haut, + 4 dp de chaque côté) ;
//   long          appui tenu 0,5 s ;
//   double        deux appuis à 2 images d'écart (geste répété) : on attend l'état voulu par le 1er appui ;
//   transition    appui juste après le changement d'écran (aucune image dessinée entre les deux) ;
//   autre doigt   un 2e doigt posé ailleurs pendant l'appui.
// Écrit une ligne « réglages (WFS-290) : <écran> <cas> : ouvrir x / 100, fermer y / 100 » et les captures du ticket.
void benchReglagesAppuis(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("réglages (WFS-290) : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 1000;
    g_horlogeBanc = 0;   // horloge simulée : 1/60 s par image, comme le téléphone à 60 images/s
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; g_horlogeBanc += 1.0 / 60; if (rendu) g.render(t); };
    auto attendre = [&](int images) { for (int i = 0; i < images; i++) image(false); };   // temps de réaction d'un joueur
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, réglages %s, %s", nom, nomEcran(g.ecran()), g_ouvert ? "ouverts" : "fermés", quoi);
    };
    const float W = (float)r.width(), H = (float)r.height(), dp = ui::pixelsParDp(W);
    uint32_t graine = 290;
    auto alea = [&]() { graine = graine * 1664525u + 1013904223u; return (graine >> 8) / 16777216.f; };
    int idDoigt = 100;
    // point d'appui selon le cas, dans la zone dessinée z
    auto point = [&](const Zone& z, int cas, float& x, float& y) {
        x = z.x + z.w / 2; y = z.y + z.h / 2;
        if (cas == 1) { x = z.x + alea() * z.w; y = z.y + alea() * z.h; }
        else if (cas == 2) {
            const float cw = z.w + 8 * dp, ch = std::max(z.h, 48 * dp);
            x = z.x + z.w / 2 + (alea() - 0.5f) * cw; y = z.y + z.h / 2 + (alea() - 0.5f) * ch;
        }
    };
    auto appui = [&](const Zone& z, int cas) {
        float x, y; point(z, cas, x, y);
        const int id = idDoigt++ % 4;   // comme Android : petits numéros de doigt, réutilisés
        g.onTouch({id, x, y, true});
        if (cas == 3) for (int i = 0; i < 30; i++) image(false);
        image(false);
        g.onTouch({id, x, y, false});
        image(false);
        if (cas == 4) {   // double : 2e appui 2 images plus tard, au même endroit
            image(false);
            g.onTouch({id, x, y, true}); image(false); g.onTouch({id, x, y, false}); image(false);
        }
    };
    // RG-04 : appui tenu, capture pendant que le doigt est posé (bouton éclairci), puis relâcher
    auto appuiTenu = [&](const Zone& z, const char* nom, const char* quoi) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({3, x, y, true}); image(false);
        capture(nom, quoi);
        g.onTouch({3, x, y, false}); image(false);
    };
    const char* NOMS[] = {"centre", "dessin", "doigt 48 dp", "long", "double", "transition", "autre doigt"};
    auto allerAuMenu = [&]() { g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) image(i % 10 == 9); };
    auto allerEnPause = [&]() {
        if (g.ecran() != Ecran::Course) { g.allerA(Ecran::Course); for (int i = 0; i < 60 * 4; i++) image(i % 30 == 29); }
        g.allerA(Ecran::Pause); image(true);
    };
    int total[2][2] = {{0, 0}, {0, 0}};
    for (int ecran = 0; ecran < 2; ecran++) {
        const char* nomE = ecran == 0 ? "menu" : "pause";
        if (ecran == 0) allerAuMenu(); else allerEnPause();
        if (g_ouvert) { g_ouvert = false; image(true); }
        const char* ne = std::getenv("WFS_APPUIS_ESSAIS");   // 0 : captures seules (le tableau des 100 appuis prend ≈ 40 min en 2340 × 1080)
        const int essais = ne ? std::max(0, std::atoi(ne)) : 100;
        for (int cas = 0; cas < 7 && essais > 0; cas++) {
            int ok[2] = {0, 0};
            for (int essai = 0; essai < essais; essai++) {
                attendre(24);   // 0,4 s entre deux gestes (temps de réaction d'un joueur)
                if (g_ouvert) { g_ouvert = false; image(true); }   // départ : panneau fermé
                if (essai % 25 == 0) image(true);   // une image dessinée de temps en temps (zones relevées au dessin)
                const Zone vu = g_bouton;   // le doigt vise le bouton vu à l'écran (sa place ne change pas d'un écran à l'autre)
                if (cas == 5) {   // transition : on quitte l'écran et on y revient, appui sans image dessinée entre les deux
                    if (ecran == 0) { g.allerA(Ecran::Garage); image(true); g.allerA(Ecran::Menu); }
                    else { g.allerA(Ecran::Course); image(true); g.allerA(Ecran::Pause); }
                    image(false);
                }
                const int autre = 9;
                const float ax = W * 0.15f, ay = H * 0.80f;
                if (cas == 6) { g.onTouch({autre, ax, ay, true}); image(false); }
                appui(vu, cas == 5 || cas == 6 ? 0 : cas);
                const bool ouvert = g_ouvert;
                ok[0] += ouvert;
                if (ouvert) {
                    image(true); attendre(24);   // le panneau s'affiche, le joueur le voit puis vise « Fermer »
                    appui(g_fermer, cas == 5 || cas == 6 ? 0 : cas);
                    ok[1] += !g_ouvert;
                }
                if (cas == 6) { g.onTouch({autre, ax, ay, false}); image(false); }
                if (g.ecran() != (ecran == 0 ? Ecran::Menu : Ecran::Pause)) { if (ecran == 0) allerAuMenu(); else allerEnPause(); }
            }
            total[ecran][0] += ok[0]; total[ecran][1] += ok[1];
            WFS_LOG("réglages (WFS-290) : %s, %s : ouvrir %d / 100, fermer %d / %d", nomE, NOMS[cas], ok[0], ok[1], ok[0]);
        }
        if (ecran == 0) {
            g_ouvert = false; image(true); attendre(24);
            appuiTenu(g_bouton, "reglages-appuis-menu-reglages-appuye", "doigt posé sur « Réglages » : bouton éclairci (RG-04)"); capture("reglages-appuis-menu-ouvert", "appui sur « Réglages » au menu : panneau ouvert"); attendre(24);
            appuiTenu(g_fermer, "reglages-appuis-menu-fermer-appuye", "doigt posé sur « Fermer » : bouton éclairci (RG-04)"); capture("reglages-appuis-menu-ferme", "appui sur « Fermer » : retour au menu");
        } else {
            g_ouvert = false; image(true); attendre(24);
            appuiTenu(g_bouton, "reglages-appuis-pause-reglages-appuye", "doigt posé sur « Réglages » : bouton éclairci (RG-04)"); capture("reglages-appuis-pause-ouvert", "en pause, appui sur « Réglages » : panneau ouvert"); attendre(24);
            appuiTenu(g_fermer, "reglages-appuis-pause-fermer-appuye", "doigt posé sur « Fermer » : bouton éclairci (RG-04)"); capture("reglages-appuis-pause-ferme", "appui sur « Fermer » : retour à la pause");
            // RG-04 : touche Retour du téléphone, réglages ouverts en pause : panneau fermé, on reste en pause (avant : la course reprenait)
            int okRetour = 0;
            const char* ne2 = std::getenv("WFS_APPUIS_ESSAIS");
            for (int i = 0; i < (ne2 ? std::max(0, std::atoi(ne2)) : 100); i++) {
                attendre(24); appui(g_bouton, 0); image(true); attendre(24);
                retourSysteme(g, true, 0); image(false); retourSysteme(g, false, 0); image(true);
                okRetour += !g_ouvert && g.ecran() == Ecran::Pause;
                if (g.ecran() != Ecran::Pause) allerEnPause();
            }
            WFS_LOG("réglages (WFS-290) : pause, touche Retour du téléphone : réglages fermés et pause gardée %d / 100", okRetour);
        }
    }
    WFS_LOG("réglages (WFS-290) : zones d'appui : « Réglages » %.0f × %.0f dp, « Fermer » %.0f × %.0f dp (%.2f px/dp)",
            g_bouton.w / dp, g_bouton.h / dp, g_fermer.w / dp, g_fermer.h / dp, dp);
    WFS_LOG("réglages (WFS-290) : total menu : ouvrir %d / 700, fermer %d ; pause : ouvrir %d / 700, fermer %d", total[0][0], total[0][1], total[1][0], total[1][1]);
    g_ouvert = false; g_horlogeBanc = -1;
    g.shutdown();
    courseBenchRules(false);
}

// WFS-291 (RG-01 à RG-04) : banc des onglets : Affichage au menu, Conduite en pause, Son en pause, Son en gaucher ; dernier
// onglet relu après un redémarrage simulé ; « Fermer » et la touche Retour ferment le panneau depuis chaque onglet
void benchReglagesOnglets(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("réglages (WFS-291) : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 1200;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g_horlogeBanc = t; g.render(t); };
    auto attendre = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g_horlogeBanc = t; } };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, onglet %s, %s", nom, nomEcran(g.ecran()), ONGLETS[g_onglet], quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        attendre(24); g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    auto onglet = [&](int o) { toucher(50 + o, g_onglets[o]); };
    g_onglet = O_AFFICHAGE;
    g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) image();
    toucher(1, g_bouton); onglet(O_AFFICHAGE);
    capture("reglages-onglet-affichage-menu", "menu principal, onglet Affichage (5 lignes)");
    // WFS-549 (RG-06, revue #420 point 2) : « Crédits » depuis les Réglages, « Retour », puis un appui dans les Réglages qui compte.
    // Doigt n° 0 partout (1er doigt d'Android) : ouvrir les Crédits à l'appui laissait g_doigt = 0 bloqué et perdait cet appui.
    {
        auto doigt0 = [&](float x, float y) { attendre(24); g.onTouch({0, x, y, true}); image(); g.onTouch({0, x, y, false}); image(); };
        doigt0(g_credits.x + g_credits.w / 2, g_credits.y + g_credits.h / 2);
        capture("reglages-credits-ouverts", "« Crédits » touché depuis les Réglages : écran Crédits par-dessus");
        WFS_LOG("réglages (WFS-549) : « Crédits » → écran Crédits %s", credits::ouvert() ? "ouvert" : "FERMÉ (ÉCHEC)");
        float rx = 0, ry = 0;
        if (credits::centreRetour(rx, ry)) doigt0(rx, ry);
        capture("reglages-credits-retour", "« Retour » des Crédits : retour aux Réglages, toujours ouverts");
        WFS_LOG("réglages (WFS-549) : « Retour » des Crédits → Crédits %s, Réglages %s", credits::ouvert() ? "OUVERTS (ÉCHEC)" : "fermés", g_ouvert ? "ouverts" : "FERMÉS (ÉCHEC)");
        const Zone z = g_onglets[O_CONDUITE];
        doigt0(z.x + z.w / 2, z.y + z.h / 2);
        WFS_LOG("réglages (WFS-549) : 1er appui après les Crédits (onglet Conduite, doigt 0) → onglet %s : %s", ONGLETS[g_onglet],
                g_onglet == O_CONDUITE ? "appui compté" : "APPUI PERDU (ÉCHEC)");
        onglet(O_AFFICHAGE);
    }
    toucher(2, g_fermer);
    WFS_LOG("réglages (WFS-291) : « Fermer » depuis Affichage → panneau %s", g_ouvert ? "OUVERT" : "fermé");
    g.allerA(Ecran::Course); for (int i = 0; i < 60 * 4; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 29) g.render(t); }
    g.allerA(Ecran::Pause); image();
    toucher(3, g_bouton);
    WFS_LOG("réglages (WFS-291) : panneau rouvert en pause sur l'onglet %s (Affichage attendu : dernier ouvert)", ONGLETS[g_onglet]);
    onglet(O_CONDUITE);
    capture("reglages-onglet-conduite-pause", "en pause, onglet Conduite (6 lignes)");
    onglet(O_SON);
    capture("reglages-onglet-son-pause", "en pause, onglet Son (2 lignes)");
    // gaucher : « Commandes : Gaucher » dans Conduite, puis retour sur Son
    onglet(O_CONDUITE); toucher(4, g_choix[L_COMMANDES][1]); onglet(O_SON);
    capture("reglages-onglet-son-gaucher", "en pause, onglet Son, commandes en gaucher");
    WFS_LOG("réglages (WFS-291) : commandes %s (Gaucher attendu)", reglages().gaucher ? "Gaucher" : "Droitier");
    // touche Retour depuis l'onglet Son : panneau fermé, pause gardée
    retourSysteme(g, true, 0); image(); retourSysteme(g, false, 0); image();
    WFS_LOG("réglages (WFS-291) : touche Retour depuis l'onglet Son → panneau %s, écran %s", g_ouvert ? "OUVERT" : "fermé", nomEcran(g.ecran()));
    // RG-03 : dernier onglet gardé après un redémarrage simulé (écriture, oubli, relecture)
    {
        const std::string f = out + "/reglages-wfs291.txt";
        setenv("WFS_REGLAGES", f.c_str(), 1);
        ecrire(); const int avant = g_onglet; g_onglet = O_AFFICHAGE; lire();
        WFS_LOG("réglages (WFS-291) : onglet relu après redémarrage : %s (%s attendu)", ONGLETS[g_onglet], ONGLETS[avant]);
        unsetenv("WFS_REGLAGES"); std::remove(f.c_str());
    }
    reglages().gaucher = false; g_onglet = O_AFFICHAGE; g_ouvert = false; g_horlogeBanc = -1;   // banc : défauts
    g.shutdown();
    courseBenchRules(false);
}

void appliquerQualiteNiveau(Renderer& r) { appliquerQualite(r); }   // WFS-15

} // namespace ecrans

WFS_SYSTEM(ecrans::EcranReglages, 460)   // interface (400-499) : par-dessus le menu (450) et la pause (455)

} // namespace wfs

namespace wfs::ecrans {
// WFS-504 (RG-07, RG-08, RG-10) : banc WFS_BANC=textes, dans la langue de WFS_LANGUE (téléphone simulé, réglage Automatique).
// Captures textes-<fr|en>-* : menu principal, Réglages (3 onglets) au menu, Affichage en pause ; textes notés (journal) puis
// vérifiés : aucune phrase française en anglais, aucun texte qui dépasse. Puis le parcours de l'utilisateur : accueil → Réglages
// → Langue = English → retour → course → pause → Réglages (en anglais) → Automatique. Rend le nombre d'échecs.
int benchReglagesTextes(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("textes (WFS-504) : chargement impossible"); courseBenchRules(false); return 1; }
    g.setAutopilot(true); g.camMode = 0;
    ui::langue::choisir(ui::langue::Choix::Automatique);
    const std::string lg = ui::langue::code();
    int echecs = 0;
    double t = 1300;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g_horlogeBanc = t; g.render(t); };
    auto attendre = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g_horlogeBanc = t; } };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, langue %s, onglet %s, %s", nom.c_str(), nomEcran(g.ecran()), ui::langue::code(), ONGLETS[g_onglet], quoi);
    };
    auto toucher = [&](int id, const Zone& z) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        attendre(24); g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
    };
    auto noterEcran = [&](const std::string& ecran) {   // textes des Réglages (drawUi de ce système seulement) de deux images
        g_journalEcran = ecran; g.render(t); g.render(t + 0.01); g_journalEcran.clear();
    };
    const char* NOMS_O[N_ONGLETS] = {"affichage", "conduite", "son"};
    ui::langue::viderJournal();
    g_onglet = O_AFFICHAGE;
    g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) image();
    capture("textes-" + lg + "-menu", "menu principal (bouton « Réglages »)");
    noterEcran("menu-bouton-reglages");   // seul le bouton « Réglages » est à cet écran (le reste du menu : WFS-505)
    toucher(1, g_bouton);
    for (int o = 0; o < N_ONGLETS; o++) {
        toucher(10 + o, g_onglets[o]);
        noterEcran("reglages-" + std::string(NOMS_O[o]) + "-menu");
        capture("textes-" + lg + "-reglages-" + NOMS_O[o], "Réglages ouverts depuis le menu");
    }
    toucher(2, g_fermer);
    g.allerA(Ecran::Course); for (int i = 0; i < 60 * 3; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 29) g.render(t); }
    g.allerA(Ecran::Pause); image();
    toucher(3, g_bouton); toucher(13, g_onglets[O_AFFICHAGE]);
    noterEcran("reglages-affichage-pause");
    capture("textes-" + lg + "-reglages-pause", "Réglages ouverts depuis la pause, onglet Affichage");
    toucher(4, g_fermer);
    echecs += ui::langue::verifierJournal();

    // parcours de l'utilisateur (critère d'acceptation) : Automatique → English → course → pause → Réglages en anglais → Automatique
    auto etape = [&](bool ok, const char* quoi) {
        if (!ok) echecs++;
        WFS_LOG("textes (WFS-504) : parcours : %s : %s", quoi, ok ? "OK" : "ÉCHEC");
    };
    g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) image();
    toucher(5, g_bouton); toucher(14, g_onglets[O_AFFICHAGE]);
    etape(g_ouvert, "accueil → Réglages ouverts");
    toucher(6, g_choix[L_LANGUE][(int)ui::langue::Choix::Anglais]);
    etape(ui::langue::choix() == ui::langue::Choix::Anglais && ui::langue::active() == ui::langue::Langue::Anglais && ui::tr("reglages.titre") == "Settings",
          "Langue = English, panneau aussitôt en anglais (titre « Settings »)");
    capture("textes-" + lg + "-parcours-1-english", "Langue = English choisi : panneau redessiné en anglais tout de suite (RG-03)");
    toucher(7, g_fermer);
    etape(!g_ouvert, "retour (« Close »)");
    g.allerA(Ecran::Course); for (int i = 0; i < 60 * 3; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 29) g.render(t); }
    etape(g.ecran() == Ecran::Course, "course");
    g.allerA(Ecran::Pause); image();
    toucher(8, g_bouton);
    ui::langue::viderJournal(); noterEcran("parcours-reglages-pause-anglais");
    etape(g_ouvert && ui::langue::verifierJournal() == 0, "pause → Réglages en anglais (aucune phrase française)");
    capture("textes-" + lg + "-parcours-2-pause-english", "en pause, Réglages en anglais");
    toucher(9, g_choix[L_LANGUE][(int)ui::langue::Choix::Automatique]);
    const bool attenduFr = ui::langue::langueDuCode(ui::langue::langueTelephone()) == ui::langue::Langue::Francais;
    etape(ui::langue::choix() == ui::langue::Choix::Automatique && (ui::langue::active() == ui::langue::Langue::Francais) == attenduFr,
          "Automatique : retour à la langue du téléphone");
    capture("textes-" + lg + "-parcours-3-auto", "Automatique : langue du téléphone");
    toucher(15, g_fermer);
    {   // RG-04 : le choix reste après un redémarrage (fichier écrit, choix oublié, relu)
        const std::string f = out + "/reglages-wfs504.txt";
        setenv("WFS_REGLAGES", f.c_str(), 1);
        ui::langue::choisir(ui::langue::Choix::Anglais); ecrire();
        ui::langue::choisir(ui::langue::Choix::Automatique); lire();
        etape(ui::langue::choix() == ui::langue::Choix::Anglais, "English gardé après un redémarrage (clé langue relue)");
        unsetenv("WFS_REGLAGES"); std::remove(f.c_str());
        ui::langue::choisir(ui::langue::Choix::Automatique);
    }
    g_horlogeBanc = -1;
    WFS_LOG("textes (WFS-504) : banc %s : %d échec(s) → %s", lg.c_str(), echecs, echecs ? "ROUGE" : "VERT");
    courseBenchRules(false);
    g.shutdown();
    return echecs;
}
} // namespace wfs::ecrans
