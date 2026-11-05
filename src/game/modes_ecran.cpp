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
    {ModeCourse::Poursuite, "Poursuite", "La police à tes trousses", false},
    {ModeCourse::Duel, "Duel", "Défie un ami, sans trafic", true},
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
        if (continuer_.contient(t.x, t.y)) { appui_.prendre(t.id); valider(g); return true; }
        if (retour_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("modes : « Retour »"); g_ouvert = false; return true; }
        return true;   // le panneau garde tous les touchers
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!g_ouvert || g.ecran != Ecran::Menu || !ui::fontReady()) return;
        if (ouverture_) { choix_ = modeCourse() == ModeCourse::Course5 ? ModeCourse::Course5 : ModeCourse::Checkpoint; km_ = kmCourse5(); ouverture_ = false; }
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
    bool lu_ = false;
    Zone cartes_[4], kms_[4], continuer_, retour_;
    ui::FiltreAppui appui_;
public:
    static bool ouverture_;
};
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

WFS_SYSTEM(ChoixMode, 458)   // interface (400-499) : par-dessus le menu (450), le salon (453) et la pause (455)

} // namespace wfs
