// WaForSpeed — choix du mode de course (WFS-110, RG-01 / RG-09) : panneau ouvert par « Course solo » au menu, par-dessus
// le menu (comme les réglages). Trois cartes : Checkpoint (60 s, +30 s par checkpoint), Course à 5 voitures (avec les
// puces 2 / 4 / 6 / 10 km) et Poursuite (WFS-111). WFS-517 (RG-01, RG-02) : plus de carte « Duel entre amis » ici, le duel
// s'ouvre seulement par « Duel en ligne » du menu principal. « Continuer » lance le parcours habituel (garage, terrain,
// course) ; « Retour » referme le panneau. Le mode et la distance sont gardés dans la sauvegarde.
#include <algorithm>
#include <cstdlib>   // WFS-202 : std::getenv
#include <cmath>
#include <cstdio>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../platform/manette.h"   // WFS-517 (RG-03) : focus de la manette sur les cartes
#include "../render/renderer.h"
#include "../ui/appui.h"
#include "../ui/ecrans/terrain.h"
#include "../ui/garage.h"
#include "course_system.h"
#include "../ui/text.h"
#include "../ui/boutons.h"   // WFS-549 (accroche E23) : boutons au modèle R19
#include "../ui/verre.h"     // WFS-557 : fond flouté (images provisoires des modes, RG-22)
#include "commandes_tactiles.h"   // WFS-557 : cartes inclinées (dessin2d)
#include "../ui/langue.h"   // WFS-505 : textes du choix du mode en français ou en anglais
#include "../ui/ecrans/textes_banc.h"   // WFS-505 : banc « textes » (journal des textes de cet écran)
#include "course5.h"
#include "game.h"
#include "poursuite.h"   // WFS-119 : clé « poursuite » du mode
#include "sauvegarde.h"

namespace wfs {

namespace {

bool g_ouvert = false;
bool g_forceBanc = false;   // banc d'essai : panneau ouvert sans sauvegarde
const int KMS[4] = {2, 4, 6, 10};
const char* const DIFFS[3] = {"modes.facile", "modes.normale", "modes.difficile"};   // WFS-133 (RG-01) : indices de course5::Difficulte ; WFS-505 : clés

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, SURF = 0x14161F, SURF2 = 0x1E2130, TXT = 0xF2F4F8, MUTE = 0x8A90A6, ACC = 0xFF6A00, ACC2 = 0x00E5FF;

struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };

// WFS-505 (RG-01) : titre (nom du mode, aussi au journal) et cle (clé du nom affiché dans les tables de textes)
struct Carte { ModeCourse mode; const char* titre; const char* cle; bool dispo; };
constexpr int NB_CARTES = 3;   // WFS-517 (RG-01) : Checkpoint, Course à 5, Poursuite
const Carte CARTES[NB_CARTES] = {
    {ModeCourse::Checkpoint, "Checkpoint", "mode.checkpoint", true},   // description : descCarte (checkpoint.depart, checkpoint.bonus)
    {ModeCourse::Course5, "Course à 5", "mode.course5", true},      // description : descCarte (course5.ordinateurs)
    {ModeCourse::Poursuite, "Poursuite", "mode.poursuite", true},   // WFS-111
};

// nombre de secondes en français : « 60 », « 7,5 »
static std::string secondes(double s) {
    char b[16];
    if (std::fabs(s - std::round(s)) < 1e-6) std::snprintf(b, sizeof b, "%.0f", s);
    else { std::snprintf(b, sizeof b, "%.1f", s); for (char* q = b; *q; q++) if (*q == '.') *q = ','; }
    return b;
}
// description d'une carte : les valeurs réglables (configgame.json) sont lues à chaque affichage, jamais recopiées
static std::string descCarte(const Carte& c) {
    // WFS-505 (RG-01) : phrases des tables de textes, valeurs en {1} {2}
    if (c.mode == ModeCourse::Checkpoint)
        return ui::tr("modes.desc.checkpoint", secondes(TEMPS_DEPART), secondes(TEMPS_CHECKPOINT));
    if (c.mode == ModeCourse::Course5)
        return ui::tr(course5::ORDINATEURS > 1 ? "modes.desc.course5" : "modes.desc.course5_un", std::to_string(course5::ORDINATEURS));
    return ui::tr("modes.desc.poursuite");
}

std::string modeCle(ModeCourse m) { return poursuite::cleMode(m); }   // WFS-119 : « poursuite » gardé

class ChoixMode : public System {
public:
    const char* name() const override { return "modes"; }
    void update(double, const GameView&) override {
        if (lu_ || !sauvegarde().actif()) return;   // mode et distance relus une fois de la sauvegarde (WFS-24)
        lu_ = true;
        const Sauvegarde& s = sauvegarde();
        modeCourse() = poursuite::modeDeCle(s.mode);   // WFS-119 : ancienne sauvegarde → Checkpoint
        kmCourse5() = s.km5;
        course5::difficulte() = course5::difficulteDeCle(s.diff5);   // WFS-133 (RG-02) : inconnue → normale
        WFS_LOG("modes : sauvegarde relue (%s, %d km, %s)", nomMode(modeCourse()), kmCourse5(), course5::cleDifficulte(course5::difficulte()));
    }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override { if (nouveau != Ecran::Menu) g_ouvert = false; }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (!g_ouvert || g.ecran != Ecran::Menu) return false;
        if (t.id == doigtBouton_ && !t.down) {   // WFS-115 : relâcher du doigt de « Continuer » / « Retour »
            appui_.consomme(t.id, false);
            const int b = bouton_; bouton_ = 0; doigtBouton_ = -1;
            if (b == 1 && continuer_.contient(t.x, t.y)) valider(g);
            else if (b == 2 && retour_.contient(t.x, t.y)) { WFS_LOG("modes : « Retour »"); g_ouvert = false; }
            return true;
        }
        if (appui_.consomme(t.id, t.down)) return true;
        if (!t.down) return true;
        // WFS-557 (RG-21) : distance et difficulté sont dans la carte « Course à 5 » : testées avant les cartes
        if (choix_ == ModeCourse::Course5)
            for (int i = 0; i < 4; i++)
                if (kms_[i].contient(t.x, t.y)) { appui_.prendre(t.id); km_ = KMS[i]; WFS_LOG("modes : %d km", km_); return true; }
        if (choix_ == ModeCourse::Course5)   // WFS-133 (RG-01) : difficulté
            for (int i = 0; i < 3; i++)
                if (diffs_[i].contient(t.x, t.y)) {
                    appui_.prendre(t.id); diff_ = (course5::Difficulte)i;
                    WFS_LOG("modes : difficulté %s", course5::cleDifficulte(diff_)); return true;
                }
        for (int i = 0; i < NB_CARTES; i++)
            if (cartes_[i].contient(t.x, t.y)) {
                appui_.prendre(t.id);
                if (!CARTES[i].dispo) { WFS_LOG("modes : « %s » pas encore disponible", CARTES[i].titre); return true; }
                choix_ = CARTES[i].mode; WFS_LOG("modes : « %s »", CARTES[i].titre);
                return true;
            }
        // WFS-115 : « Continuer » et « Retour » agissent au relâcher, sur le bouton appuyé. Ils agissaient à l'appui : le
        // panneau se fermait sous le doigt encore posé, et ses mouvements (Android : un appui par mouvement) arrivaient
        // comme des appuis neufs à l'écran suivant — sur « Retour » du garage, juste sous « Continuer » en 20/9 : retour au
        // menu — ou au menu lui-même (« Ta voiture », « Showroom » : garage en mode regarder, « Choisir » → menu)
        if (continuer_.contient(t.x, t.y)) { appui_.prendre(t.id); bouton_ = 1; doigtBouton_ = t.id; return true; }
        if (retour_.contient(t.x, t.y)) { appui_.prendre(t.id); bouton_ = 2; doigtBouton_ = t.id; return true; }
        return true;   // le panneau garde tous les touchers
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!g_ouvert || g.ecran != Ecran::Menu || !ui::fontReady()) return;
        const ecrans::textes::Journal journal;   // WFS-505 : textes notés au banc « textes »
        // WFS-517 (RG-04) : dernier mode = duel (ou inconnu) → Checkpoint présélectionné
        if (ouverture_) { bouton_ = 0; doigtBouton_ = -1; choix_ = modeCourse() == ModeCourse::Course5 || modeCourse() == ModeCourse::Poursuite ? modeCourse() : ModeCourse::Checkpoint; km_ = kmCourse5(); diff_ = course5::difficulte(); ouverture_ = false; }
        // WFS-557 (maquette 02 ; arbitrages du PO du 2026-11-16 : maquette à 100 %, RG-21, RG-22). Unité k = H / 720 (maquette
        // 1600 × 720), depuis la zone sûre. Titre et « Étape 1 sur 3 » en haut à gauche ; 3 grandes cartes inclinées ; image du mode
        // provisoire = la scène du jeu floutée (RG-22, vraies images : WFS-562) ; nom du mode choisi en orange ; « Distance » et
        // « Difficulté » seulement dans la carte du mode qui s'en sert (Course à 5), au bas de la carte (RG-21) ; « Retour / Menu
        // principal » en autocollant en bas à gauche ; « Suivant » en bouton rond en bas à droite.
        const float W = g.uiWidth, H = g.uiHeight, k = std::min(H / 720.f, (W - g.marges.gauche - g.marges.droite) / 1600.f);   // 16:9 : réduit, centré
        const float ox = g.marges.gauche + (W - g.marges.gauche - g.marges.droite - 1600 * k) / 2, oy = g.marges.haut + (H - g.marges.haut - g.marges.bas - 720 * k) / 2;
        (void)W;
        verre::fondFloute(r); verre::assombrir(r, W, H, 0.62f);
        ui::text(r, ox + 44 * k, oy + 40 * k, 56 * k, ui::majuscules(ui::tr("modes.titre")), hex(TXT), ui::GAUCHE, 0.01f, false, true);
        { ui::AvecPolice inter(ui::Police::Inter); ui::text(r, ox + 44 * k, oy + 98 * k, 13 * k, ui::tr("modes.etape"), hex(0xC9CDDA), ui::GAUCHE, 0, false); }
        manette::zonesImage().clear();   // WFS-517 (RG-03) : panneau ouvert → seuls ses boutons ont le focus (comme les réglages)
        for (auto& z : kms_) z = {};
        for (auto& z : diffs_) z = {};
        const float cw = 470 * k, ch = 356 * k, biais = 40 * k, y0 = oy + 140 * k;
        for (int i = 0; i < NB_CARTES; i++) {
            const Carte& c = CARTES[i];
            const float x = ox + (90 + 506 * i) * k;
            const bool sel = c.dispo && c.mode == choix_;
            cartes_[i] = {x, y0, cw, ch};
            manette::focus(cartes_[i]);
            const vec2 q[4] = {{x + biais, y0}, {x + cw, y0}, {x + cw - biais, y0 + ch}, {x, y0 + ch}};
            dessin2d::polygone(r, q, 4, {hex(0x1A1C22, 0.30f), hex(0x0B0C10, 0.94f), y0, y0 + ch});
            dessin2d::contour(r, q, 4, 1 * k, sel ? hex(TXT, 0.30f) : hex(TXT, 0.12f));
            const bool reglages = sel && c.mode == ModeCourse::Course5;   // RG-21 : distance et difficulté dans la carte
            const float yn = y0 + (reglages ? 160 : 270) * k;
            ui::textBoite(r, x + 26 * k, yn, cw - 70 * k, 50 * k, 48 * k, ui::majuscules(ui::tr(c.cle)), hex(sel ? 0xFF8A3D : TXT, c.dispo ? 1.f : 0.5f), ui::GAUCHE, 0.01f, false, true);
            { ui::AvecPolice inter(ui::Police::Inter); descr(r, x + 22 * k, yn + 58 * k, cw - 70 * k, 14 * k, descCarte(c), hex(0xE6E8EE)); }
            if (!reglages) continue;
            // rangées de verre : 2 · 4 · 6 · 10 km ; Facile · Normale · Difficile (zones ≥ 44 px, dans la carte)
            const float rh = 44 * k, xr = x + 18 * k, wr = cw - 70 * k, yk = yn + 102 * k, yd = yk + rh + 10 * k;
            ui::boutons::selecteurFond(r, xr, yk, wr, rh);
            for (int j = 0; j < 4; j++) {
                const float w = (wr - 8 * k) / 4, xj = xr + 4 * k + j * w;
                kms_[j] = {xj, yk, w, rh}; manette::focus(kms_[j]);
                char t[16]; std::snprintf(t, sizeof t, "%d KM", KMS[j]);
                ui::boutons::selecteurChoix(r, xj, yk + 4 * k, w, rh - 8 * k, "", KMS[j] == km_);
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::text(r, xj + w / 2, yk + (rh - 14 * k) / 2, 14 * k, t, KMS[j] == km_ ? hex(BG) : hex(TXT), ui::CENTRE, 0.06f, false);
            }
            ui::boutons::selecteurFond(r, xr, yd, wr, rh);
            for (int j = 0; j < 3; j++) {
                const float w = (wr - 8 * k) / 3, xj = xr + 4 * k + j * w;
                diffs_[j] = {xj, yd, w, rh}; manette::focus(diffs_[j]);
                ui::boutons::selecteurChoix(r, xj, yd + 4 * k, w, rh - 8 * k, "", j == (int)diff_);
                ui::AvecPolice inter(ui::Police::InterMedium);
                ui::textBoite(r, xj + w / 2, yd, w - 10 * k, rh, 14 * k, ui::majuscules(ui::tr(DIFFS[j])), j == (int)diff_ ? hex(BG) : hex(TXT), ui::CENTRE, 0.06f, false);
            }
        }
        // « Retour / Menu principal » (autocollant 154 × 64) et « Suivant » (rond, 130 px)
        retour_ = {ox + 66 * k, oy + 604 * k, 168 * k, 72 * k};
        const float d = 130 * k;
        continuer_ = {ox + 1460 * k - d / 2, oy + 600 * k - d / 2, d, d};
        manette::focus(retour_, true); manette::focus(continuer_);
        ui::boutons::autocollant(r, retour_.x, retour_.y, retour_.w, retour_.h, ui::majuscules(ui::tr("modes.retour")), 1, bouton_ == 2, 28 * k, ui::tr("reglages.surtitre.menu"));
        ui::boutons::startStop(r, continuer_.x, continuer_.y, d, d, ui::majuscules(ui::tr("modes.suivant")), 1, bouton_ == 1, "", 18 * k);
    }

private:
    // description sur deux lignes au plus (coupure aux espaces)
    static void descr(Renderer& r, float x, float y, float w, float px, const std::string& s, const vec4& c) {
        std::string ligne, mot; float yy = y;
        auto vider = [&] { if (!ligne.empty()) { ui::text(r, x, yy, px, ligne, c, ui::GAUCHE, 0.f, false); yy += px * 1.5f; ligne.clear(); } };
        for (size_t i = 0; i <= s.size(); i++) {
            if (i < s.size() && s[i] != ' ') { mot += s[i]; continue; }
            const std::string essai = ligne.empty() ? mot : ligne + " " + mot;
            if (!ligne.empty() && ui::width(px, essai) > w) { vider(); ligne = mot; } else ligne = essai;   // police de l'appelant (Inter)
            mot.clear();
        }
        vider();
    }
    void valider(const GameView& g) {
        g_ouvert = false;
        modeCourse() = choix_; kmCourse5() = km_; course5::difficulte() = diff_;   // WFS-133 : difficulté gardée
        if (sauvegarde().actif()) {
            sauvegarde().mode = modeCle(choix_); sauvegarde().km5 = km_; sauvegarde().diff5 = course5::cleDifficulte(diff_); sauvegarde().enregistrer();
        }
        WFS_LOG("modes : « Continuer » → %s%s, garage", nomMode(choix_),
                choix_ == ModeCourse::Course5 ? (", " + std::to_string(km_) + " km, " + course5::cleDifficulte(diff_)).c_str() : "");
        g.allerA(Ecran::Garage);
    }
    ModeCourse choix_ = ModeCourse::Checkpoint;
    int km_ = 4;
    course5::Difficulte diff_ = course5::NORMALE;   // WFS-133
    int bouton_ = 0, doigtBouton_ = -1;   // WFS-115 : bouton appuyé (1 Continuer, 2 Retour) et son doigt
    bool lu_ = false;
    Zone cartes_[NB_CARTES], kms_[4], diffs_[3], continuer_, retour_;
    ui::FiltreAppui appui_;
public:
    static bool ouverture_;
    static ChoixMode* actif_;
    // banc d'essai (WFS-115) : centre d'une zone du panneau (« continuer », « retour », « carte0 » à « carte2 », « km0 » à « km3 »)
    bool centre(const std::string& nom, float& x, float& y) const {
        const Zone* z = nom == "continuer" || nom == "continuer-droite" ? &continuer_ : nom == "retour" ? &retour_ : nullptr;
        if (nom.size() == 6 && nom.compare(0, 5, "carte") == 0 && nom[5] >= '0' && nom[5] < '0' + NB_CARTES) z = &cartes_[nom[5] - '0'];   // WFS-517 : « carte3 » n'existe plus
        if (nom.size() == 3 && nom.compare(0, 2, "km") == 0) z = &kms_[(nom[2] - '0') & 3];
        if (nom.size() == 5 && nom.compare(0, 4, "diff") == 0 && nom[4] >= '0' && nom[4] <= '2') z = &diffs_[nom[4] - '0'];   // WFS-133
        if (!z || z->w <= 0) return false;
        x = z->x + z->w * (nom == "continuer-droite" ? 0.9f : 0.5f); y = z->y + z->h / 2;   // « -droite » : bord droit du bouton
        return true;
    }
    course5::Difficulte diff() const { return diff_; }   // WFS-133 : difficulté affichée au panneau (banc)
    ChoixMode() { actif_ = this; }
    ~ChoixMode() override { if (actif_ == this) actif_ = nullptr; }
};
ChoixMode* ChoixMode::actif_ = nullptr;
bool ChoixMode::ouverture_ = false;

} // namespace

bool ouvrirChoixMode() {
    if (!sauvegarde().actif() && !g_forceBanc) return false;   // banc d'essai sans sauvegarde : garage direct (captures inchangées)
    g_ouvert = true; ChoixMode::ouverture_ = true;
    WFS_LOG("modes : panneau ouvert (mode %s, %d km)", nomMode(modeCourse()), kmCourse5());
    return true;
}
bool choixModeOuvert() { return g_ouvert; }
void choixModeBanc(bool force) { g_forceBanc = force; if (!force) g_ouvert = false; }

// ---------- banc d'essai (WFS_BANC=parcours, WFS-115) ----------
// Parcours complet au doigt, comme sur Android : chaque appui = un « down », quelques mouvements (« down » répétés du même
// doigt, une image chacun), puis le « up ». Chemins : « Duel en ligne » du menu → salon (WFS-517) ; 3 cartes ; dernier
// mode duel → Checkpoint → garage ; Showroom → garage → « Choisir » → menu ; Course à 5
// (6 km) → garage → « Retour » (un cran : panneau des modes) → garage → « Choisir » → terrain (mode et km gardés) →
// « Retour » → menu ; Course solo → Checkpoint → garage → « Choisir » → terrain → « Rouler » → course.
// Captures parcours-mode, parcours-terrain, parcours-course. Rend le nombre d'écarts (0 = parcours conforme).
int benchParcours(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    // WFS-202 (accroche, RG-09) : voiture du parcours dans WFS_VOITURE (garage, course en vue Cockpit) ; sans variable : la Golf
    const char* voiture = std::getenv("WFS_VOITURE") && *std::getenv("WFS_VOITURE") ? std::getenv("WFS_VOITURE") : "golf";
    if (!g.init(r, voiture, "ville")) { courseBenchRules(false); WFS_ERR("parcours : chargement impossible"); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    choixModeBanc(true);
    double t = 1500;
    int erreurs = 0, n = 0;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 4 == 0) g.render(t); };   // une image dessinée sur 4
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const char* nom) {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, mode %s, %d km", nom, nomEcran(g.ecran()), nomMode(modeCourse()), kmCourse5());
    };
    int doigt = 30;
    auto appuyer = [&](float x, float y) {   // doigt Android : down, 6 mouvements, up (une image dessinée entre chaque)
        const int id = doigt++;
        g.render(t);   // zones posées à l'image dessinée juste avant l'appui
        auto dessinee = [&]() { g.update(1.0 / 60); g.render(t += 1.0 / 60); };   // doigt posé ≈ 100 ms : 6 images dessinées
        g.onTouch({id, x, y, true}); dessinee();
        for (int k = 1; k <= 6; k++) { g.onTouch({id, x + 0.4f * k, y, true}); dessinee(); }
        g.onTouch({id, x + 3, y, false}); dessinee();
        images(3);
    };
    // doigt Android plus rapide que l'image : down, mouvements et up arrivent avant l'image suivante (même boucle d'entrées)
    auto rafale = [&](float x, float y) {
        const int id = doigt++;
        g.render(t);
        g.onTouch({id, x, y, true});
        for (int k = 1; k <= 4; k++) g.onTouch({id, x + 0.4f * k, y, true});
        g.onTouch({id, x + 2, y, false}); images(4);
    };
    auto attendu = [&](Ecran e, const char* etape) {
        if (g.ecran() == e) { WFS_LOG("parcours : %s → écran %s (attendu)", etape, nomEcran(e)); return; }
        erreurs++; WFS_ERR("parcours : %s → écran %s, attendu %s", etape, nomEcran(g.ecran()), nomEcran(e));
    };
    auto verifier = [&](bool ok, const char* quoi) { if (ok) WFS_LOG("parcours : %s (ok)", quoi); else { erreurs++; WFS_ERR("parcours : %s (ÉCHEC)", quoi); } };
    auto panneau = [&](const char* zone, bool vite = false) {
        float x, y;
        if (!ChoixMode::actif_ || !ChoixMode::actif_->centre(zone, x, y)) { erreurs++; WFS_ERR("parcours : zone %s du panneau absente", zone); return; }
        WFS_LOG("parcours : panneau « %s » à (%.0f, %.0f)", zone, x, y);
        if (vite) rafale(x, y); else appuyer(x, y);
    };
    auto garage = [&](const char* bouton) {
        float x = 0, y = 0;
        for (int i = 0; i < 600 && !ecrans::garageBouton(bouton, x, y); i++) image();   // voiture chargée, panneau posé
        images(30);
        if (!ecrans::garageBouton(bouton, x, y)) { erreurs++; WFS_ERR("parcours : bouton %s du garage absent", bouton); return; }
        float cx = 0, cy = 0, rx = 0, ry = 0; ecrans::garageBouton("choisir", cx, cy); ecrans::garageBouton("retour", rx, ry);
        WFS_LOG("parcours : garage « Choisir » à (%.0f, %.0f), « Retour » à (%.0f, %.0f)", cx, cy, rx, ry);
        appuyer(x, y);
    };
    auto soloOuvert = [&]() { ouvrirChoixMode(); images(10); verifier(choixModeOuvert(), "« Course solo » : panneau des modes ouvert"); };
    g.allerA(Ecran::Menu); images(30);
    // 1) WFS-517 (RG-02, RG-07) : « Duel en ligne » du menu principal → salon (2e bouton déclaré par le menu : focus de la
    // manette, dans l'ordre du dessin) ; la Course solo n'a plus de carte « Duel entre amis » (zone « carte3 » absente)
    {
        g.render(t);
        const auto& z = manette::zonesPrecedentes();
        if (z.size() < 2) { erreurs++; WFS_ERR("parcours : bouton « Duel en ligne » du menu absent"); }
        else appuyer(z[1].x + z[1].w * 0.4f, z[1].y + z[1].h * 0.5f);
        attendu(Ecran::Salon, "menu principal puis « Duel en ligne »");
        g.allerA(Ecran::Menu); images(20);
        soloOuvert();
        float zx = 0, zy = 0;
        verifier(ChoixMode::actif_ && ChoixMode::actif_->centre("carte2", zx, zy) && !ChoixMode::actif_->centre("carte3", zx, zy),
                 "Course solo : 3 cartes (Checkpoint, Course à 5, Poursuite), plus de « Duel entre amis »");
        // RG-04 : dernier mode = duel → Checkpoint présélectionné ; « Continuer » mène au garage, jamais au salon
        modeCourse() = ModeCourse::Duel; g_ouvert = false; soloOuvert(); images(5);
        capture("parcours-mode-3-cartes");
        panneau("continuer", true);
        attendu(Ecran::Garage, "dernier mode duel : Course solo puis « Continuer » (Checkpoint présélectionné)");
        verifier(modeCourse() == ModeCourse::Checkpoint, "dernier mode duel → Checkpoint retenu");
        g.allerA(Ecran::Menu); images(20);
    }
    // 2) Showroom (garage ouvert depuis le menu) : « Choisir » ramène au menu, comme avant
    ecrans::garageRegarder(); g.allerA(Ecran::Garage); images(5);
    garage("choisir");
    attendu(Ecran::Menu, "Showroom puis « Choisir »");
    verifier(!choixModeOuvert(), "Showroom : panneau des modes fermé au retour");
    // 3) Course à 5, 6 km : Retour du garage = un cran (panneau des modes), puis Choisir → terrain, mode et km gardés
    soloOuvert(); panneau("carte1"); panneau("km2"); panneau("continuer");
    attendu(Ecran::Garage, "Course à 5 (6 km) puis « Continuer »");
    garage("retour");
    attendu(Ecran::Menu, "garage du parcours puis « Retour »");
    verifier(choixModeOuvert(), "« Retour » du garage : panneau des modes rouvert (un seul cran)");
    panneau("continuer");
    attendu(Ecran::Garage, "« Continuer » de nouveau");
    garage("choisir");
    attendu(Ecran::Terrain, "garage du parcours puis « Choisir »");
    verifier(modeCourse() == ModeCourse::Course5 && kmCourse5() == 6, "mode Course à 5 et 6 km gardés jusqu'au terrain");
    images(20);
    // 4) Course solo, Checkpoint : mode → garage → terrain → course
    g.allerA(Ecran::Menu); images(20);
    soloOuvert(); panneau("carte0"); images(5);
    capture("parcours-mode");
    panneau("continuer-droite");   // bord droit de « Continuer » : au-dessus de « Retour » du garage en 20/9 (cause de WFS-115)
    attendu(Ecran::Garage, "Checkpoint puis « Continuer » (bord droit, doigt qui bouge)");
    garage("choisir");
    attendu(Ecran::Terrain, "garage du parcours puis « Choisir »");
    verifier(modeCourse() == ModeCourse::Checkpoint, "mode Checkpoint gardé jusqu'au terrain");
    images(20);
    capture("parcours-terrain");
    float rx = 0, ry = 0;
    if (!ecrans::centreRouler(rx, ry)) { erreurs++; WFS_ERR("parcours : bouton « Rouler » absent"); }
    else appuyer(rx, ry);
    for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
    attendu(Ecran::Course, "terrain puis « Rouler »");
    g.setAutopilot(true); images(60 * 5);
    capture("parcours-course");
    g.setAutopilot(false);
    WFS_LOG("parcours : %d écart(s)", erreurs);
    choixModeBanc(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    return erreurs;
}

bool choixModeCentre(const std::string& zone, float& x, float& y) { return ChoixMode::actif_ && ChoixMode::actif_->centre(zone, x, y); }   // WFS-125

// ---------- WFS-133 (RG-10) : parcours complet de la course à 5 avec la difficulté (groupe course5) ----------
// accueil → Course solo → Course à 5 → DIFFICILE → 4 KM → Continuer → garage → Choisir → terrain → Rouler → course → fin →
// « Rejouer » (difficulté gardée) → fin → « Menu » → accueil → Course solo (DIFFICILE resélectionnée). Doigt Android
// (down, mouvements, up) comme benchParcours. Capture parcours5-difficile. Rend le nombre d'écarts (0 = conforme).
int benchParcoursCourse5(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 2; course5::difficulte() = course5::NORMALE;
    if (!g.init(r, "golf", "ville")) { courseBenchRules(false); WFS_ERR("parcours course à 5 : chargement impossible"); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    choixModeBanc(true);
    double t = 2500;
    int erreurs = 0, n = 0, doigt = 60;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 4 == 0) g.render(t); };
    auto images = [&](int k) { for (int i = 0; i < k; i++) image(); };
    auto appuyer = [&](float x, float y) {
        const int id = doigt++;
        g.render(t);
        auto dessinee = [&]() { g.update(1.0 / 60); g.render(t += 1.0 / 60); };
        g.onTouch({id, x, y, true}); dessinee();
        for (int k = 1; k <= 6; k++) { g.onTouch({id, x + 0.4f * k, y, true}); dessinee(); }
        g.onTouch({id, x + 3, y, false}); dessinee();
        images(3);
    };
    auto etape = [&](bool ok, const char* quoi) {
        if (ok) WFS_LOG("parcours course à 5 : %s (ok)", quoi); else { erreurs++; WFS_ERR("parcours course à 5 : %s (ÉCART)", quoi); }
    };
    auto panneau = [&](const char* zone) {
        float x, y;
        if (!ChoixMode::actif_ || !ChoixMode::actif_->centre(zone, x, y)) { erreurs++; WFS_ERR("parcours course à 5 : zone %s absente", zone); return; }
        appuyer(x, y);
    };
    auto garage = [&](const char* bouton) {
        float x = 0, y = 0;
        for (int i = 0; i < 600 && !ecrans::garageBouton(bouton, x, y); i++) image();
        images(30);
        if (!ecrans::garageBouton(bouton, x, y)) { erreurs++; WFS_ERR("parcours course à 5 : bouton %s du garage absent", bouton); return; }
        appuyer(x, y);
    };
    auto jusquaFin = [&]() {
        g.setAutopilot(true);
        float x, y;
        // course sans dessin (comme benchCourse5 : banc court sur la CI), puis l'écran de fin dessiné (0,9 s) avant l'appui
        const Course* c = activeCourse();
        for (int i = 0; i < 400 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); t += 1.0 / 60; c = activeCourse(); }
        images(90);
        g.setAutopilot(false);
        return course5Bouton("rejouer", x, y);
    };
    auto finBouton = [&](const char* nom) {
        float x, y;
        if (!course5Bouton(nom, x, y)) { erreurs++; WFS_ERR("parcours course à 5 : bouton %s de la fin absent", nom); return; }
        appuyer(x, y);
    };
    g.allerA(Ecran::Menu); images(30);
    ouvrirChoixMode(); images(10);
    etape(choixModeOuvert(), "accueil → « Course solo » : panneau des modes ouvert");
    panneau("carte1"); panneau("diff2"); panneau("km1"); images(5);
    g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/parcours5-difficile.png");
    panneau("continuer");
    etape(g.ecran() == Ecran::Garage, "Course à 5, DIFFICILE, 4 KM, « Continuer » → garage");
    etape(course5::difficulte() == course5::DIFFICILE && kmCourse5() == 4 && modeCourse() == ModeCourse::Course5, "difficulté difficile et 4 km retenus");
    garage("choisir");
    etape(g.ecran() == Ecran::Terrain, "garage « Choisir » → terrain");
    images(20);
    float rx = 0, ry = 0;
    if (!ecrans::centreRouler(rx, ry)) { erreurs++; WFS_ERR("parcours course à 5 : bouton « Rouler » absent"); } else appuyer(rx, ry);
    for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
    etape(g.ecran() == Ecran::Course, "terrain « Rouler » → course");
    etape(jusquaFin(), "course de 4 km → écran de fin");
    finBouton("rejouer");
    images(10);
    const Course* c = activeCourse();
    etape(g.ecran() == Ecran::Course && c && c->state != RaceState::Over && course5::difficulte() == course5::DIFFICILE,
          "« Rejouer » → nouvelle course, difficulté difficile gardée");
    etape(jusquaFin(), "2e course → écran de fin");
    finBouton("menu");
    etape(g.ecran() == Ecran::Menu, "« Menu » → accueil");
    images(20);
    ouvrirChoixMode(); images(10);
    float dx, dy;
    etape(choixModeOuvert() && ChoixMode::actif_ && ChoixMode::actif_->diff() == course5::DIFFICILE && ChoixMode::actif_->centre("diff2", dx, dy),
          "accueil → « Course solo » : DIFFICILE resélectionnée");
    WFS_LOG("parcours course à 5 (RG-10) : %d écart(s) %s", erreurs, erreurs ? "ÉCHEC" : "OK");
    choixModeBanc(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4; course5::difficulte() = course5::NORMALE;
    courseBenchRules(false);
    g.shutdown();
    return erreurs;
}

WFS_SYSTEM(ChoixMode, 458)   // interface (400-499) : par-dessus le menu (450), le salon (453) et la pause (455)

} // namespace wfs
