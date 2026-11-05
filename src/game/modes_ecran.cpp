// WaForSpeed — choix du mode de course (WFS-110, RG-01 / RG-09) : panneau ouvert par « Course solo » au menu, par-dessus
// le menu (comme les réglages). Quatre cartes : Checkpoint (60 s, +30 s par checkpoint), Course à 5 voitures (avec les
// puces 2 / 4 / 6 / 10 km), Poursuite (« Bientôt », WFS-111) et Duel (→ salon). « Continuer » lance le parcours habituel
// (garage, terrain, course) ; « Retour » referme le panneau. Le mode et la distance sont gardés dans la sauvegarde.
#include <algorithm>
#include <cstdio>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/appui.h"
#include "../ui/ecrans/terrain.h"
#include "../ui/garage.h"
#include "course_system.h"
#include "../ui/text.h"
#include "course5.h"
#include "game.h"
#include "sauvegarde.h"

namespace wfs {

namespace {

bool g_ouvert = false;
bool g_forceBanc = false;   // banc d'essai : panneau ouvert sans sauvegarde
const int KMS[4] = {2, 4, 6, 10};

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, SURF = 0x14161F, SURF2 = 0x1E2130, TXT = 0xF2F4F8, MUTE = 0x8A90A6, ACC = 0xFF6A00, ACC2 = 0x00E5FF;

struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };

struct Carte { ModeCourse mode; const char* titre; const char* desc; bool dispo; };
const Carte CARTES[4] = {
    {ModeCourse::Checkpoint, "Checkpoint", "60 s au départ, +30 s à chaque checkpoint", true},
    {ModeCourse::Course5, "Course à 5", "Toi contre 4 ordinateurs, sans trafic", true},
    {ModeCourse::Poursuite, "Poursuite", "La police à tes trousses", true},   // WFS-111
    {ModeCourse::Duel, "Duel entre amis", "Défie un ami où qu'il soit, sans trafic", true},   // WFS-112
};

std::string modeCle(ModeCourse m) { return m == ModeCourse::Course5 ? "course5" : "checkpoint"; }

class ChoixMode : public System {
public:
    const char* name() const override { return "modes"; }
    void update(double, const GameView&) override {
        if (lu_ || !sauvegarde().actif()) return;   // mode et distance relus une fois de la sauvegarde (WFS-24)
        lu_ = true;
        const Sauvegarde& s = sauvegarde();
        modeCourse() = s.mode == "course5" ? ModeCourse::Course5 : ModeCourse::Checkpoint;
        kmCourse5() = s.km5;
        WFS_LOG("modes : sauvegarde relue (%s, %d km)", nomMode(modeCourse()), kmCourse5());
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
        for (int i = 0; i < 4; i++)
            if (cartes_[i].contient(t.x, t.y)) {
                appui_.prendre(t.id);
                if (!CARTES[i].dispo) { WFS_LOG("modes : « %s » pas encore disponible", CARTES[i].titre); return true; }
                choix_ = CARTES[i].mode; WFS_LOG("modes : « %s »", CARTES[i].titre);
                return true;
            }
        if (choix_ == ModeCourse::Course5)
            for (int i = 0; i < 4; i++)
                if (kms_[i].contient(t.x, t.y)) { appui_.prendre(t.id); km_ = KMS[i]; WFS_LOG("modes : %d km", km_); return true; }
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
        if (ouverture_) { bouton_ = 0; doigtBouton_ = -1; choix_ = modeCourse() == ModeCourse::Course5 ? ModeCourse::Course5 : ModeCourse::Checkpoint; km_ = kmCourse5(); ouverture_ = false; }
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        r.uiRect(0, 0, W, H, hex(BG, 0.86f));
        const float L = g.marges.gauche, R = W - g.marges.droite, cx = (L + R) / 2;
        ui::text(r, cx, g.marges.haut + 7 * u, 2.6f * u, "NOUVELLE COURSE", hex(ACC2), ui::CENTRE, 0.16f, false);
        ui::text(r, cx, g.marges.haut + 11 * u, 8 * u, "CHOISIS TON MODE", hex(TXT), ui::CENTRE, 0.04f, false, true);
        // quatre cartes côte à côte
        const float gap = 2.4f * u, cw = std::min(42 * u, ((R - L) - 8 * u - 3 * gap) / 4), ch = 34 * u, y0 = g.marges.haut + 26 * u;
        const float x0 = cx - (4 * cw + 3 * gap) / 2;
        for (int i = 0; i < 4; i++) {
            const Carte& c = CARTES[i];
            const float x = x0 + i * (cw + gap);
            const bool sel = c.dispo && c.mode == choix_;
            cartes_[i] = {x, y0, cw, ch};
            r.uiRect(x, y0, cw, ch, hex(sel ? SURF2 : SURF, c.dispo ? 0.96f : 0.6f));
            r.uiRect(x, y0, cw, 0.6f * u, sel ? hex(ACC) : hex(SURF2));
            if (sel) { r.uiRect(x, y0 + ch - 0.4f * u, cw, 0.4f * u, hex(ACC)); r.uiRect(x, y0, 0.4f * u, ch, hex(ACC)); r.uiRect(x + cw - 0.4f * u, y0, 0.4f * u, ch, hex(ACC)); }
            char num[8]; std::snprintf(num, sizeof num, "0%d", i + 1);
            ui::text(r, x + 2.4f * u, y0 + 3 * u, 2.4f * u, num, hex(c.dispo ? ACC2 : MUTE), ui::GAUCHE, 0.16f, false);
            ui::text(r, x + 2.4f * u, y0 + 8 * u, 4.6f * u, ui::majuscules(c.titre), hex(sel ? ACC : (c.dispo ? TXT : MUTE)), ui::GAUCHE, 0.02f, false, true);
            descr(r, x + 2.4f * u, y0 + 16 * u, cw - 4.8f * u, 2.5f * u, c.desc, hex(MUTE));
            if (!c.dispo) {
                r.uiRect(x + 2.4f * u, y0 + ch - 7 * u, 14 * u, 4.2f * u, hex(SURF2));
                ui::text(r, x + 9.4f * u, y0 + ch - 6.2f * u, 2.4f * u, "BIENTÔT", hex(MUTE), ui::CENTRE, 0.14f, false);
            }
        }
        // puces de distance sous la carte « Course à 5 »
        const float ky = y0 + ch + 3 * u, kh = 6.5f * u;
        for (auto& z : kms_) z = {};
        if (choix_ == ModeCourse::Course5) {
            ui::text(r, x0, ky + 1.6f * u, 2.6f * u, "DISTANCE", hex(MUTE), ui::GAUCHE, 0.16f, false);
            const float kw = 12 * u, kx0 = x0 + 17 * u;
            for (int i = 0; i < 4; i++) {
                const float x = kx0 + i * (kw + 1.6f * u);
                const bool sel = KMS[i] == km_;
                kms_[i] = {x, ky, kw, kh};
                r.uiRect(x, ky, kw, kh, sel ? hex(ACC) : hex(SURF2));
                char t[16]; std::snprintf(t, sizeof t, "%d KM", KMS[i]);
                ui::text(r, x + kw / 2, ky + 1.6f * u, 3.2f * u, t, sel ? hex(BG) : hex(TXT), ui::CENTRE, 0.06f, false);
            }
        }
        // « Retour » et « Continuer »
        const float bw = 28 * u, bh = 8 * u, by = H - g.marges.bas - bh - 6 * u;
        retour_ = {x0, by, bw, bh};
        continuer_ = {x0 + 4 * cw + 3 * gap - bw, by, bw, bh};
        r.uiRect(retour_.x, by, bw, bh, hex(0xFFFFFF, 0.10f));
        ui::text(r, retour_.x + bw / 2, by + 2.2f * u, 3.6f * u, "RETOUR", hex(TXT), ui::CENTRE, 0.08f, false);
        r.uiRect(continuer_.x, by, bw, bh, hex(ACC));
        ui::text(r, continuer_.x + bw / 2, by + 2.2f * u, 3.6f * u, "CONTINUER", hex(BG), ui::CENTRE, 0.08f, false);
    }

private:
    // description sur deux lignes au plus (coupure aux espaces)
    static void descr(Renderer& r, float x, float y, float w, float px, const std::string& s, const vec4& c) {
        std::string ligne, mot; float yy = y;
        auto vider = [&] { if (!ligne.empty()) { ui::text(r, x, yy, px, ligne, c, ui::GAUCHE, 0.f, false); yy += px * 1.5f; ligne.clear(); } };
        for (size_t i = 0; i <= s.size(); i++) {
            if (i < s.size() && s[i] != ' ') { mot += s[i]; continue; }
            const std::string essai = ligne.empty() ? mot : ligne + " " + mot;
            if (!ligne.empty() && ui::width(px, essai) > w) { vider(); ligne = mot; } else ligne = essai;
            mot.clear();
        }
        vider();
    }
    void valider(const GameView& g) {
        g_ouvert = false;
        if (choix_ == ModeCourse::Duel) { WFS_LOG("modes : « Continuer » → salon du duel"); g.allerA(Ecran::Salon); return; }
        modeCourse() = choix_; kmCourse5() = km_;
        if (sauvegarde().actif()) { sauvegarde().mode = modeCle(choix_); sauvegarde().km5 = km_; sauvegarde().enregistrer(); }
        WFS_LOG("modes : « Continuer » → %s%s, garage", nomMode(choix_), choix_ == ModeCourse::Course5 ? (", " + std::to_string(km_) + " km").c_str() : "");
        g.allerA(Ecran::Garage);
    }
    ModeCourse choix_ = ModeCourse::Checkpoint;
    int km_ = 4;
    int bouton_ = 0, doigtBouton_ = -1;   // WFS-115 : bouton appuyé (1 Continuer, 2 Retour) et son doigt
    bool lu_ = false;
    Zone cartes_[4], kms_[4], continuer_, retour_;
    ui::FiltreAppui appui_;
public:
    static bool ouverture_;
    static ChoixMode* actif_;
    // banc d'essai (WFS-115) : centre d'une zone du panneau (« continuer », « retour », « carte0 » à « carte3 », « km0 » à « km3 »)
    bool centre(const std::string& nom, float& x, float& y) const {
        const Zone* z = nom == "continuer" || nom == "continuer-droite" ? &continuer_ : nom == "retour" ? &retour_ : nullptr;
        if (nom.size() == 6 && nom.compare(0, 5, "carte") == 0) z = &cartes_[(nom[5] - '0') & 3];
        if (nom.size() == 3 && nom.compare(0, 2, "km") == 0) z = &kms_[(nom[2] - '0') & 3];
        if (!z || z->w <= 0) return false;
        x = z->x + z->w * (nom == "continuer-droite" ? 0.9f : 0.5f); y = z->y + z->h / 2;   // « -droite » : bord droit du bouton
        return true;
    }
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
// WFS-112 (banc du duel entre amis) : centre d'une zone du panneau des modes (voir ChoixMode::centre)
bool choixModeZone(const std::string& nom, float& x, float& y) { return ChoixMode::actif_ && ChoixMode::actif_->centre(nom, x, y); }
void choixModeBanc(bool force) { g_forceBanc = force; if (!force) g_ouvert = false; }

// ---------- banc d'essai (WFS_BANC=parcours, WFS-115) ----------
// Parcours complet au doigt, comme sur Android : chaque appui = un « down », quelques mouvements (« down » répétés du même
// doigt, une image chacun), puis le « up ». Chemins : Duel → salon ; Showroom → garage → « Choisir » → menu ; Course à 5
// (6 km) → garage → « Retour » (un cran : panneau des modes) → garage → « Choisir » → terrain (mode et km gardés) →
// « Retour » → menu ; Course solo → Checkpoint → garage → « Choisir » → terrain → « Rouler » → course.
// Captures parcours-mode, parcours-terrain, parcours-course. Rend le nombre d'écarts (0 = parcours conforme).
int benchParcours(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { courseBenchRules(false); WFS_ERR("parcours : chargement impossible"); return 1; }
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
    // 1) Duel → salon
    soloOuvert(); panneau("carte3"); panneau("continuer", true);   // appui rapide : tout dans la même boucle d'entrées
    attendu(Ecran::Salon, "Duel puis « Continuer »");
    g.allerA(Ecran::Menu); images(20);
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

WFS_SYSTEM(ChoixMode, 458)   // interface (400-499) : par-dessus le menu (450), le salon (453) et la pause (455)

} // namespace wfs
