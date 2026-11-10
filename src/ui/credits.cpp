// WaForSpeed — écran « Crédits » (WFS-406) : voir credits.h.
#include "credits.h"
#include <algorithm>
#include <cmath>
#include "../core/configgame.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/commandes_tactiles.h"   // dessin2d::biseau
#include "../game/game.h"
#include "../platform/manette.h"   // focus de la manette (« Retour »)
#include "../render/renderer.h"
#include "ecrans/reglages_appui.h"   // appui::dansCible : cible de 48 dp (WFS-290, comme les Réglages et les CGU)
#include "garage.h"
#include "petit_ecran.h"
#include "text.h"

namespace wfs {
namespace credits {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF;
const unsigned ACCENT_PRESSE = 0xFF9A4D;   // « Retour » sous le doigt (même teinte que les CGU et les Réglages)
const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.98f}, GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.16f};
const char* FICHIER = "licences/CREDITS.txt";

struct Zone { float x = 0, y = 0, w = 0, h = 0; };
struct Ligne { Paragraphe::Style style; std::string texte; float x, y, taille; };

struct Etat {
    bool ouvert = false;
    std::vector<Paragraphe> paras;
    std::vector<Ligne> lignes;
    float largeurPage = -1, kPage = -1, hauteurTexte = 0, defil = 0, defilMax = 0, vue = 0;
    int doigt = -1, cible = 0;   // cible du doigt posé : 0 rien, 1 texte, 2 « Retour »
    float yDoigt = 0, dp = 1;
    Zone retour, texte, bouton;
    float dpBouton = 1;
} g;

enum { RIEN = 0, TEXTE_Z = 1, RETOUR = 2 };

// découpe des paragraphes en lignes de la largeur du cadre (mots entiers ; suite d'une entrée en retrait)
void mettreEnPage(float largeur, float k) {
    g.largeurPage = largeur; g.kPage = k; g.lignes.clear();
    // corps de 16 (dp sur téléphone couché, au-dessus du minimum de 14 dp de petit_ecran.h), titre 22, sections 17 (comme les CGU)
    const float corps = 16 * k, titre = 22 * k, section = 17 * k, retrait = 14 * k;
    float y = 0;
    for (const Paragraphe& p : g.paras) {
        if (p.style == Paragraphe::VIDE) { y += corps * 0.6f; continue; }
        const float t = p.style == Paragraphe::TITRE ? titre : p.style == Paragraphe::SECTION ? section : corps;
        const float interligne = t * 1.38f, x0 = p.style == Paragraphe::ENTREE ? retrait : 0, suite = p.style == Paragraphe::ENTREE ? retrait * 2 : 0;
        const std::string& s = p.style == Paragraphe::SECTION ? ui::majuscules(p.texte) : p.texte;
        const float esp = p.style == Paragraphe::SECTION ? 0.08f : 0.f;
        std::string ligne;
        bool premiere = true;
        auto pousser = [&]() { g.lignes.push_back({p.style, ligne, premiere ? x0 : suite, y, t}); y += interligne; premiere = false; ligne.clear(); };
        size_t i = 0;
        while (i < s.size()) {
            size_t f = s.find(' ', i);
            if (f == std::string::npos) f = s.size();
            const std::string mot = s.substr(i, f - i);
            i = f + 1;
            if (mot.empty()) continue;
            const std::string essai = ligne.empty() ? mot : ligne + " " + mot;
            if (!ligne.empty() && ui::width(t, essai, esp) > largeur - (premiere ? x0 : suite)) { pousser(); ligne = mot; }
            else ligne = essai;
        }
        if (!ligne.empty()) pousser();
    }
    g.hauteurTexte = y;
}

bool dansZone(const Zone& z, float x, float y) { return z.w > 0 && x >= z.x && x <= z.x + z.w && y >= z.y && y <= z.y + z.h; }
int cibleSous(float x, float y) {
    if (g.retour.w > 0 && ecrans::appui::dansCible(g.retour.x, g.retour.y, g.retour.w, g.retour.h, g.dp, x, y)) return RETOUR;
    return dansZone(g.texte, x, y) ? TEXTE_Z : RIEN;
}

} // namespace

void ouvrir() {
    std::vector<uint8_t> a;
    std::string texte;
    if (readAsset(FICHIER, a)) texte.assign(a.begin(), a.end());
    else WFS_ERR("Crédits (WFS-406) : %s introuvable", FICHIER);
    g.paras = paragraphes(texte, config().confidentialiteUrl);
    g.ouvert = true; g.defil = 0; g.doigt = -1; g.cible = RIEN; g.largeurPage = -1;
    WFS_LOG("Crédits (WFS-406) : écran ouvert, %zu paragraphes, politique de confidentialité %s", g.paras.size(),
            config().confidentialiteUrl[0] ? "montrée" : "cachée (adresse vide)");
}
void fermer() { if (g.ouvert) WFS_LOG("Crédits (WFS-406) : écran fermé"); g.ouvert = false; g.doigt = -1; g.cible = RIEN; }
bool ouvert() { return g.ouvert; }

void dessiner(Renderer& r, float W, float H, const Marges& m) {
    if (!g.ouvert || !ui::fontReady()) return;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    g.dp = ui::pixelsParDp(W);
    r.uiRect(0, 0, W, H, hex(BG));
    const float zl = m.gauche + 16 * k, zr = W - m.droite - 16 * k;
    const float pw = std::min(zr - zl, 1040 * k), px = zl + (zr - zl - pw) / 2;
    const float py = m.haut + (cp ? 8 : 24) * k, ph = H - py - m.bas - (cp ? 8 : 24) * k;
    r.uiRect(px, py, pw, ph, SURFACE);
    r.uiRect(px, py, pw, 3 * k, hex(ACCENT2));   // liseré néon
    const float x0 = px + 24 * k;
    ui::text(r, x0, py + 14 * k, 12 * k, ui::majuscules("Menu principal"), hex(ACCENT2), ui::GAUCHE, 0.22f, false);
    const float tt = (cp ? 26 : 34) * k;
    ui::text(r, x0, py + 32 * k, tt, ui::majuscules("Crédits"), hex(TEXTE), ui::GAUCHE, 0.02f, false, true);
    const float haut = py + 32 * k + tt + 12 * k;
    // « Retour » : bouton principal orange en biseau, en bas à droite du panneau (même taille que « Fermer » des CGU)
    const float bh = 50 * kb, tp = (cp ? 18 : 20) * k, by = py + ph - 14 * k - bh;
    const std::string tr = ui::majuscules("Retour");
    const float wr = ui::width(tp, tr, 0.12f) + 56 * kb;
    g.retour = {px + pw - 24 * k - wr, by, wr, bh};
    const bool presse = g.doigt >= 0 && g.cible == RETOUR;
    dessin2d::biseau(r, g.retour.x, g.retour.y, g.retour.w, g.retour.h, 14 * kb, dessin2d::uni(hex(presse ? ACCENT_PRESSE : ACCENT)));
    ui::text(r, g.retour.x + g.retour.w / 2, g.retour.y + (g.retour.h - tp) / 2, tp, tr, hex(BG), ui::CENTRE, 0.12f, false);
    manette::focus(g.retour, true);
    ui::text(r, x0, by + (bh - 14 * k) / 2, 14 * k, "Glisse le doigt sur le texte pour le faire défiler.", hex(MUTED), ui::GAUCHE, 0.02f, false);
    // cadre du texte défilant
    const float tx = px + 20 * k, ty = haut, tw = pw - 40 * k, th = by - 14 * k - ty, pad = 14 * k;
    g.texte = {tx, ty, tw, th};
    r.uiRect(tx, ty, tw, th, hex(BG, 0.65f));
    const float largeur = tw - 2 * pad - 12 * k;
    if (std::fabs(largeur - g.largeurPage) > 0.5f || std::fabs(k - g.kPage) > 1e-4f) mettreEnPage(largeur, k);
    g.vue = th - 2 * pad;
    g.defilMax = std::max(0.f, g.hauteurTexte - g.vue);
    g.defil = std::clamp(g.defil, 0.f, g.defilMax);
    for (const Ligne& l : g.lignes) {   // lignes entièrement visibles seulement
        const float y = l.y - g.defil;
        if (y < 0 || y + l.taille * 1.2f > g.vue) continue;
        const vec4 c = l.style == Paragraphe::TITRE ? hex(ACCENT2) : l.style == Paragraphe::SECTION ? hex(ACCENT) : hex(TEXTE, 0.92f);
        ui::text(r, tx + pad + l.x, ty + pad + y, l.taille, l.texte, c, ui::GAUCHE, l.style == Paragraphe::SECTION ? 0.08f : 0.f, false);
    }
    if (g.defilMax > 0) {   // ascenseur : position et part visible du texte
        const float bx = tx + tw - 8 * k, hb = std::max(24 * k, g.vue * g.vue / g.hauteurTexte), yb = ty + pad + (g.vue - hb) * (g.defil / g.defilMax);
        r.uiRect(bx, ty + pad, 4 * k, g.vue, hex(TEXTE, 0.08f));
        r.uiRect(bx, yb, 4 * k, hb, hex(ACCENT2, 0.85f));
    }
}

bool toucher(int id, float x, float y, bool down) {
    if (!g.ouvert) return false;
    const int sous = cibleSous(x, y);
    if (down) {
        if (g.doigt < 0) { g.doigt = id; g.yDoigt = y; g.cible = sous; }
        else if (id == g.doigt && g.cible == TEXTE_Z) {   // glisser : le texte suit le doigt
            g.defil = std::clamp(g.defil - (y - g.yDoigt), 0.f, g.defilMax);
            g.yDoigt = y;
        }
        return true;
    }
    if (id != g.doigt) return true;   // lever d'un autre doigt, ou du doigt qui a ouvert l'écran : rien
    const int c = g.cible;
    g.doigt = -1; g.cible = RIEN;
    if (c == RETOUR && sous == RETOUR) { WFS_LOG("Crédits (WFS-406) : « Retour » → menu"); fermer(); }
    return true;
}

// .cam-btn des Réglages (reglages.cpp, dessinerBouton) : vitré, bordure 1 px, biais 8 px, 44 px de haut ; texte cyan 15 px
void dessinerBouton(Renderer& r, float W, float H, const Marges& m, bool presse) {
    if (!ui::fontReady()) return;
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    const float h = 44 * kb, tp = 15 * k, pad = 18 * kb;
    const std::string t = ui::majuscules("Crédits");
    const float w = pad + ui::width(tp, t, 0.12f) + pad, x = W - m.droite - 16 * k - w, y = H - m.bas - 14 * k - h;
    g.bouton = {x, y, w, h};
    g.dpBouton = ui::pixelsParDp(W);
    dessin2d::biseau(r, x - kb, y - kb, w + 2 * kb, h + 2 * kb, 8 * kb, dessin2d::uni(BORD));
    dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(GLASS));
    if (presse) dessin2d::biseau(r, x, y, w, h, 8 * kb, dessin2d::uni(hex(ACCENT2, 0.22f)));   // appuyé : voile cyan
    ui::text(r, std::round(x + pad), std::round(y + (h - tp) / 2), tp, t, hex(ACCENT2), ui::GAUCHE, 0.12f, false);
    manette::focus(g.bouton);
}
bool dansBouton(float x, float y) { return g.bouton.w > 0 && ecrans::appui::dansCible(g.bouton.x, g.bouton.y, g.bouton.w, g.bouton.h, g.dpBouton, x, y); }

float defilement() { return g.defil; }
float defilementMax() { return g.defilMax; }
bool centreRetour(float& x, float& y) { if (g.retour.w <= 0) return false; x = g.retour.x + g.retour.w / 2; y = g.retour.y + g.retour.h / 2; return true; }
bool centreBouton(float& x, float& y) { if (g.bouton.w <= 0) return false; x = g.bouton.x + g.bouton.w / 2; y = g.bouton.y + g.bouton.h / 2; return true; }
bool centreTexte(float& x, float& y) { if (g.texte.w <= 0) return false; x = g.texte.x + g.texte.w / 2; y = g.texte.y + g.texte.h / 2; return true; }
bool defilerJusqua(const std::string& mot) {
    for (const Ligne& l : g.lignes)
        if (l.texte.find(mot) != std::string::npos || l.texte.find(ui::majuscules(mot)) != std::string::npos) {   // sections en capitales
            g.defil = std::clamp(l.y, 0.f, g.defilMax); return true;
        }
    return false;
}

namespace {
// système : panneau par-dessus le menu (450), le bandeau des paquets (458) et les Réglages (460) ; touchers reçus par le menu
class Credits : public System {
public:
    const char* name() const override { return "credits"; }
    void onEcran(Ecran, Ecran, const GameView&) override { fermer(); }
    void drawUi(Renderer& r, const GameView& v) override {
        if (v.ecran == Ecran::Menu && !v.sceneDediee) dessiner(r, v.uiWidth, v.uiHeight, v.marges);
    }
    void shutdown(Renderer&) override { fermer(); }
};
} // namespace

int benchCredits(Game& gm, Renderer& r, const std::string& out) {
    int erreurs = 0;
    if (!gm.init(r, "golf", "ville")) { WFS_ERR("crédits : chargement impossible"); return 1; }
    gm.setAutopilot(false); gm.camMode = 0;
    for (int i = 0; i < 30; i++) gm.update(1.0 / 60);
    double t = 900;
    auto image = [&](int n = 1) { for (int i = 0; i < n; i++) { gm.update(1.0 / 60); t += 1.0 / 60; } gm.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        gm.render(t); gm.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s (défilement %.0f / %.0f px)", nom, nomEcran(gm.ecran()), quoi, defilement(), defilementMax());
    };
    auto appui = [&](int id, float x, float y) { gm.onTouch({id, x, y, true}); image(); gm.onTouch({id, x, y, false}); image(); };
    auto verifier = [&](bool ok, const char* quoi) { if (!ok) { erreurs++; WFS_ERR("crédits : ÉCHEC %s", quoi); } else WFS_LOG("crédits : OK %s", quoi); };
    // accueil → menu (RG-05)
    gm.allerA(Ecran::Menu);
    image();     // 1re image du menu : début de son animation d'entrée
    image(60);   // 1 s plus tard : menu posé (remarque du lead dev, PR #338)
    capture("credits-menu", "menu principal avec le bouton « Crédits » en bas à droite");
    float x = 0, y = 0;
    verifier(centreBouton(x, y), "bouton « Crédits » dessiné au menu");
    appui(31, x, y);
    image(5);
    verifier(ouvert(), "menu → Crédits");
    capture("credits-debut", "écran Crédits, début");
    // défilement au doigt (RG-04) : le texte suit le doigt vers le haut
    if (centreTexte(x, y)) {
        const float avant = defilement();
        gm.onTouch({32, x, y, true}); image();
        for (int i = 1; i <= 10; i++) { gm.onTouch({32, x, y - 30.f * i, true}); image(); }
        gm.onTouch({32, x, y - 300, false}); image();
        verifier(defilement() > avant + 250 && ouvert(), "glisser le doigt de 300 px fait défiler le texte de 300 px, écran gardé");
    }
    struct Vue { const char* mot; const char* nom; const char* quoi; };
    for (const Vue& v : {Vue{"Personnes", "credits-milieu", "milieu : voitures, deux-roues, personnes"},
                         Vue{"Deux-roues", "credits-deux-roues", "défilé jusqu'aux deux-roues"},
                         Vue{"android_native_app_glue", "credits-native-app-glue", "défilé jusqu'au code (stb, android_native_app_glue)"}}) {
        verifier(defilerJusqua(v.mot), v.mot);
        capture(v.nom, v.quoi);
    }
    gm.render(t);
    if (centreTexte(x, y)) {   // fin : glisser le doigt jusqu'en bas du texte
        gm.onTouch({33, x, y, true}); image();
        for (int i = 0; i < 400 && defilement() < defilementMax(); i++) { gm.onTouch({33, x, y - 40.f * (i + 1), true}); image(); }
        gm.onTouch({33, x, y, false}); image();
    }
    verifier(defilementMax() > 0 && defilement() >= defilementMax() - 0.5f, "fin du texte atteinte au doigt");
    capture("credits-fin", "écran Crédits, fin (texte de la licence Apache 2.0)");
    // « Retour » → menu
    verifier(centreRetour(x, y), "bouton « Retour » dessiné");
    appui(34, x, y);
    image(30);
    verifier(!ouvert() && gm.ecran() == Ecran::Menu, "« Retour » ramène au menu");
    capture("credits-retour-menu", "retour au menu après « Retour »");
    // garage : les voitures renommées (RG-01)
    ecrans::garageRegarder();
    gm.allerA(Ecran::Garage);
    image(40);
    for (const char* id : {"golf", "m4", "giulia"}) {
        for (int i = 0; i < 25; i++) {
            const char* a = ecrans::garageVoitureAffichee();
            if (a && std::string(a) == id) break;
            if (float gx, gy; ecrans::garageBouton("suivant", gx, gy)) appui(35 + i, gx, gy);
            image(20);
        }
        const char* a = ecrans::garageVoitureAffichee();
        verifier(a && std::string(a) == id, id);
        image(40);
        capture((std::string("credits-garage-") + id).c_str(), "garage : nom inventé de la voiture");
    }
    gm.shutdown();
    WFS_LOG("crédits (WFS-406) : banc terminé, %d erreur(s)", erreurs);
    return erreurs;
}

} // namespace credits

WFS_SYSTEM(credits::Credits, 470)   // interface (400-499) : par-dessus les Réglages (460), avant le fondu du chargement (490)

} // namespace wfs
