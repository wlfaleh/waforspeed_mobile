// WaForSpeed — menu principal (WFS-37) et ses appuis (WFS-38) : voir menu.h.
#include "paquets.h"   // WFS-306 (accroche) : proposition des paquets ouverte depuis le bandeau
#include "menu.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include "../../core/log.h"
#include "../../core/memoire.h"   // WFS-550 : mémoire de la vitrine
#include "../../core/system.h"
#include "../../game/cars.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/sauvegarde.h"   // WFS-24
#include "../../game/course5.h"   // WFS-110 : choix du mode
#include "../../render/renderer.h"
#include "../text.h"
#include "../boutons.h"   // WFS-549 : boutons au modèle R19
#include "../langue.h"   // WFS-505 : textes du menu en français ou en anglais
#include "textes_banc.h"   // WFS-505 : banc « textes » (journal des textes de cet écran)
#include "../petit_ecran.h"   // WFS-147 : téléphone couché (dp)
#include "../../game/direction.h"   // WFS-146 (RG-05) : réglages Direction et Gyroscope, dessin2d
#include "chargement.h"
#include "cadeau_jour.h"   // WFS-501 (accroche) : bouton et écran « Cadeau du jour »
#include "pret_bandeau.h"   // WFS-502 (accroche) : bandeau « Voiture prêtée »
#include "creation_profil.h"   // WFS-481 (accroche) : écran « Crée ton pilote » par-dessus le menu
#include "profil.h"   // WFS-482 (accroche) : bouton et écran « Profil »
#include "market_ecran.h"   // WFS-513 (accroche) : bouton et écran « Market »
#include "reglages.h"
#include "terrain.h"
#include "../garage.h"   // WFS-11
#include "../menu_parking.h"   // WFS-552 : parking souterrain derrière le menu
#include "../../render/model_loader.h"        // WFS-550 : voiture de vitrine du menu
#include "../../render/vehicule_skin.h"
#include "../../render/vehicule_finitions.h"
#include "../../render/vehicule_peinture2.h"
#include "../../game/roues.h"
#include <cstdlib>
#include "../../game/track.h"
#include "../../game/player.h"
#include "../credits.h"   // WFS-406 (accroche E8) : bouton et écran « Crédits »
#include "compteur_etoiles.h"   // WFS-537 (accroche) : compteur d'étoiles à côté de « Réglages »
#include "../../platform/manette.h"   // WFS-22 : focus de la manette
#include "../verre.h"   // WFS-548 : rien du menu sous une fenêtre de verre (verre::couvert)
#include "../coffre_cadeau.h"   // WFS-557 : coffre en bois 3D du cadeau du jour (maquette 05)

namespace wfs {
namespace ecrans {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned BG = 0x0A0B10, TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
               INTRO = 0xC9CDDA;
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f};

// cubic-bezier(.2, .9, .1, 1) (--ease du web), résolu par dichotomie sur x
float ease(float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto bz = [](float a, float b, float s) { const float u = 1 - s; return 3 * u * u * s * a + 3 * u * s * s * b + s * s * s; };
    float lo = 0, hi = 1;
    for (int i = 0; i < 20; i++) { const float m = (lo + hi) / 2; (bz(0.2f, 0.1f, m) < t ? lo : hi) = m; }
    return bz(0.9f, 1.f, (lo + hi) / 2);
}

struct Choix { const char* titre; const char* desc; };
enum { SOLO = 0, DUEL = 1, SHOWROOM = 2, TA_VOITURE = 3 /* WFS-550 : bloc RECORD → Profil */, CREDITS = 4, CADEAU = 5, COMPTEUR = 6, PROFIL = 7, PRET = 8, MARKET = 9 };   // WFS-96 : 4e zone = « Ta voiture » (pied du menu) ; WFS-406 : bouton « Crédits » ; WFS-501 : « Cadeau du jour » ; WFS-537 : compteur d'étoiles ; WFS-482 : « Profil »
// zones touchables des 3 choix (RG-02) : les parallélogrammes dessinés à la dernière image (x, y, largeur, hauteur, biais)
struct Zone { float x = 0, y = 0, w = 0, h = 0, biais = 0; bool ok = false; };
Zone g_zones[4];
long long g_best = 0;   // record de la session : gardé quand la partie est reconstruite sur une autre carte (WFS-41)
bool dansZone(const Zone& z, float x, float y) {
    if (!z.ok || y < z.y || y > z.y + z.h || x < z.x) return false;
    return x <= z.x + z.w - z.biais * (y - z.y) / z.h;   // coin bas droit coupé (clip-path)
}
int choixSous(float x, float y) { for (int i = 0; i < 4; i++) if (dansZone(g_zones[i], x, y)) return i; return -1; }
const Choix CHOIX[3] = {   // section #menu de index.html ; WFS-505 (RG-01) : clés des tables de textes (titre, description)
    {"menu.solo", "menu.solo.desc"},
    {"menu.duel", "menu.duel.desc"},   // WFS-135 (ANO-63)
    {"menu.showroom", "menu.showroom.desc"},
};

class Menu : public System {
public:
    const char* name() const override { return "menu"; }
    // WFS-552 (RG-05) : parking préparé au chargement de la partie ; mémoire refusée → fond d'avant (scène de la course figée)
    void init(Renderer& r, const GameView&) override {
        r_ = &r;
        if (!parking_.pret() && !std::getenv("WFS_MENU_SANS_PARKING")) parking_.init(r);
    }
    void shutdown(Renderer& r) override { r.vk().waitIdle(); parking_.shutdown(r); coffre::commun().shutdown(r); libererVitrine(r); }
    // WFS-550 (revue du lead dev, PR #424 : 98 Mo de mémoire graphique, 254 ms) : la vitrine n'existe qu'au menu : chargée en y
    // entrant (1re image du menu), libérée en le quittant ; la course et le chargement d'une partie ne paient rien
    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        surEcran(ancien, nouveau, g);
        if (ancien == Ecran::Menu && nouveau != Ecran::Menu && vitrine_.ok && r_) { r_->vk().waitIdle(); libererVitrine(*r_); }
    }
    // WFS-552 (RG-01, RG-04, RG-06) : derrière le menu, la voiture du joueur (sa peinture, sa finition et sa skin, dessinées par
    // Game) garée dans le parking souterrain, en caméra rapprochée ; l'interface du menu reste dessinée par-dessus
    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (g.ecran != Ecran::Menu || !parking_.pret() || !g.car || !g.player || !g.track) return false;
        if (!vitrine_.ok && r_) chargerVitrine(*r_);   // 1re image du menu
        voiture_ = matriceVoiture(g);
        parking_.regler(fp, s, g, vitrine_.ok ? carById(VITRINE) : *g.car, voiture_);   // WFS-550 : cadrage sur la SF-X de vitrine
        if (vitrine_.ok) s.joueur = false;   // la voiture du joueur n'est pas dessinée au menu : la vitrine la remplace
        // WFS-557 (maquette 05) : écran du cadeau : le coffre en bois prend la place de la voiture (construit à la 1re ouverture)
        if (cadeau_jour::ouvert() && r_) {
            if (!coffre::commun().pret() && !coffreEchec_) coffreEchec_ = !coffre::commun().init(*r_);
            if (coffre::commun().pret()) { s.joueur = false; s.anime = true; coffre::commun().eclairer(fp, placeCoffre(), cadeau_jour::ouvertureCoffre()); }
        }
        s.interface = true;
        return true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (g.ecran == Ecran::Menu && g.sceneDediee && parking_.pret()) {
            parking_.dessiner(r, voiture_);
            if (cadeau_jour::ouvert() && coffre::commun().pret()) { coffre::commun().dessiner(r, placeCoffre(), cadeau_jour::ouvertureCoffre()); return; }
            if (vitrine_.ok) {   // WFS-550 : SF-X orange caméléon, skin bleu ciel (maquette 01), roues droites, à l'arrêt
                const int f0 = r.finitionJoueur();
                r.finitionJoueur(vehicule_finitions::depuisId("cameleon"));
                r.dessinJoueur = true;
                r.peinture2ProchaineVoiture(vehicule_peinture2::UNE_COULEUR);
                RouesPose rp = roues::poseJoueur(0, 0, 0, vitrine_.nRoues ? vitrine_.roues[0].rayon : roues::RW_DEFAUT);
                const PeintureDef* o = peintureDef("orange");
                const unsigned h = o ? o->hex : 0xFF6A00;
                auto lin = [](float c) { return std::pow(c, 2.2f); };
                const vec3 orange{lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
                r.draw(vitrine_, voiture_, &orange, &rp);
                r.dessinJoueur = false;
                r.finitionJoueur(f0);
            }
        }
    }
    void surEcran(Ecran, Ecran nouveau, const GameView&) {
        if (nouveau == Ecran::Menu) t0_ = -1;
        presse_ = -1; doigt_ = -1;
        for (Zone& z : g_zones) z.ok = false;
    }
    // RG-01 : appui = état pressé ; l'action part au relâcher sur le même choix, glisser hors du choix l'annule.
    // RG-05 : sur le menu, tout toucher est gardé (aucune commande de conduite).
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran == Ecran::Menu && market_ecran::ouvert()) { doigt_ = -1; presse_ = -1; return market_ecran::toucher(t.id, t.x, t.y, t.down); }   // WFS-513
        if (g.ecran == Ecran::Menu && profil_ecran::ouvert()) { doigt_ = -1; presse_ = -1; return profil_ecran::toucher(t.id, t.x, t.y, t.down); }   // WFS-482
        if (g.ecran == Ecran::Menu && cadeau_jour::ouvert()) { doigt_ = -1; presse_ = -1; return cadeau_jour::toucher(t.id, t.x, t.y, t.down); }   // WFS-501
        if (g.ecran == Ecran::Menu && credits::ouvert()) { doigt_ = -1; presse_ = -1; return credits::toucher(t.id, t.x, t.y, t.down); }   // WFS-406 : écran Crédits par-dessus
        if (g.ecran != Ecran::Menu || toucheReglages(t.x, t.y, t.id) || toucheBandeauPaquets(t.x, t.y)) return false;   // WFS-68 : bouton et panneau des réglages (WFS-290 : et le doigt posé sur « Réglages ») ; WFS-306 : proposition des paquets
        if (choixModeOuvert()) { doigt_ = -1; presse_ = -1; return false; }   // WFS-110 : panneau des modes par-dessus
        int sous = choixSous(t.x, t.y);
        if (sous < 0 && cadeau_jour::dansBouton(t.x, t.y)) sous = CADEAU;   // WFS-501
        if (sous < 0 && compteur_etoiles::dans(t.x, t.y)) sous = COMPTEUR;   // WFS-537 (RG-09)
        if (sous < 0 && pret_bandeau::dans(t.x, t.y)) sous = PRET;   // WFS-502
        if (t.down) {
            if (doigt_ < 0) { doigt_ = t.id; presse_ = sous; }   // WFS-83 : « Duel en ligne » actif (plus grisé)
            else if (t.id == doigt_ && sous != presse_) presse_ = -1;               // glissé hors du choix : annulé
            return true;
        }
        if (t.id != doigt_) return true;
        const int choix = presse_;
        doigt_ = -1; presse_ = -1;
        if (choix < 0 || sous != choix) return true;
        if (choix == SOLO) { WFS_LOG("menu : « Course solo »"); if (!ouvrirChoixMode()) g.allerA(Ecran::Garage); }   // WFS-110 : modes             // RG-03 ; WFS-11 : garage d'abord
        else if (choix == DUEL) { WFS_LOG("menu : « Duel en ligne »"); modeCourse() = ModeCourse::Duel; g.allerA(Ecran::Salon); }   // WFS-83 : écran Salon
        else if (choix == SHOWROOM) { WFS_LOG("menu : « Showroom »"); garageRegarder(); g.allerA(Ecran::Garage); }   // WFS-96 (RG-01)
        else if (choix == CADEAU) { WFS_LOG("menu : « Cadeau du jour »"); cadeau_jour::ouvrir(); }   // WFS-501 (RG-01)
        else if (choix == PROFIL) { WFS_LOG("menu : « Profil »"); profil_ecran::ouvrir(); }   // WFS-482 (RG-01)
        else if (choix == PRET) pret_bandeau::appuyer(g);   // WFS-502 : Activer, garage ou message vu
        else if (choix == CREDITS) { WFS_LOG("menu : « Crédits »"); credits::ouvrir(); }   // WFS-406 (RG-02)
        else if (choix == COMPTEUR) { WFS_LOG("menu : compteur d'étoiles → Market"); market_ecran::ouvrir(Ecran::Menu); }   // WFS-537 (RG-09), WFS-513 : le compteur ouvre le Market
        else if (choix == MARKET) { WFS_LOG("menu : « Market »"); market_ecran::ouvrir(Ecran::Menu); }   // WFS-513 (RG-01)
        else if (choix == TA_VOITURE) { WFS_LOG("menu : « RECORD » → Profil"); profil_ecran::ouvrir(); }   // WFS-550 (RG-11) : le bloc RECORD ouvre le Profil
        return true;
    }
    // record de la session (RG-09) : meilleur score des courses terminées
    void update(double, const GameView& g) override {
        const Course* c = activeCourse();
        // score pris à la fin de la course (endRace), pas pendant la roue libre de l'écran de fin (WFS-38)
        const bool fin = c && c->state == RaceState::Over;
        if (fin && !finVue_ && g.player && c->timeLeft < 1e6) g_best = std::max(g_best, c->score(*g.player));
        finVue_ = fin;
        g_best = std::max(g_best, sauvegarde().best);   // WFS-24 : record gardé d'un lancement à l'autre
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Menu || g.sceneDediee || !ui::fontReady()) return;
        if (creationProfilOuverte()) { t0_ = -1; return; }   // WFS-481 (RG-02) : rien du menu sous la création du profil ; fondu d'entrée ensuite
        if (verre::couvert()) return;   // WFS-548 : fenêtre de verre par-dessus (Réglages, CGU, Profil, Market) : seule la scène floutée se voit
        const textes::Journal journal;   // WFS-505 : textes notés au banc « textes »
        if (t0_ < 0) t0_ = g.time;
        const float W = g.uiWidth, H = g.uiHeight, t = (float)(g.time - t0_);
        // WFS-147 (RG-01, RG-02, RG-06, ANO-17, ANO-19) : sur téléphone couché, mise en page propre au menu, dessinée en dp
        // (k = pixels par dp), d'après celle de WFS-102 (6d3ed61) : logo de 56 dp, choix de 52 dp (titre 21 dp,
        // description 14 dp), pied 40 dp, aide en 14 dp : 380 dp de haut en tout ; écran de bureau : inchangé
        const bool cp = ui::petitEcran(W, H);
        const MiseEnPage m = cp ? miseEnPagePetit(W, H, 56, 10) : miseEnPage(W, H);
        const float k = m.k, gap = m.gap;
        // .screen : fondu de 300 ms (screen-in) ; voile linear-gradient(100deg, .94 → .78 à 42 % → .12) ;
        // WFS-96 (RG-03) : dégradé par sommets (2 quadrilatères), plus de bandes visibles
        const float op = ease(t / 0.3f);
        // WFS-550 (maquette 01) : plus de voile sombre sur la scène (il assombrissait la voiture de vitrine de moitié) ; seul le
        // panneau de gauche est sombre, comme sur la maquette
        (void)BG;
        if (choixModeOuvert()) return;   // WFS-141 (RG-04, ANO-54) : panneau des modes ouvert : ni textes ni aide du menu dessous
        if (cadeau_jour::ouvert()) return;   // WFS-501 : écran du cadeau par-dessus (système CadeauJour, 466)
        if (market_ecran::ouvert()) return;   // WFS-513 : écran Market par-dessus (système MarketEcran, 469)
        if (profil_ecran::ouvert()) return;   // WFS-482 : écran Profil par-dessus (système ProfilEcran, 468)
        if (credits::ouvert()) return;   // WFS-406 : écran Crédits par-dessus (système Credits, 470) : rien du menu dessous
        // ---------- WFS-550 (RG-11, maquette 01 ; ordre de l'utilisateur du 2026-11-15 : la maquette à 100 %) ----------
        // logo sur une ligne en haut à gauche ; panneau sombre à gauche avec « COURSE SOLO » et « COURSE EN LIGNE » en boutons
        // ronds et « GARAGE » en pilule chrome ; « RECORD » en bas à gauche (le toucher ouvre le Profil) ; compteur d'étoiles et
        // engrenage en haut à droite (leurs systèmes) ; carte « Cadeau du jour » en bas à droite. Plus de Crédits (Réglages,
        // maquette 14), de Market (toucher le compteur), de Profil (toucher RECORD), de sous-titres ni d'aide sur l'écran.
        // Mesures relevées sur la maquette (2400 × 1080), en unités de 720 px de haut (kb).
        (void)k; (void)gap; (void)m;
        const float kb = H / 720.f, gl = g.marges.gauche, gb = g.marges.bas;
        auto glisse = [&](int n, float& dx) { const float e = ease((t - 0.04f * n) / 0.32f); dx = -36 * kb * (1 - e); return e * op; };
        float dx;
        float a = glisse(0, dx);
        dessinerLogoLigne(r, gl + 50 * kb + dx, 30 * kb, 50 * kb, a);
        // panneau sombre à coin coupé (haut droit), du bord gauche à 570
        a = glisse(1, dx);
        {
            const float x1 = gl + 570 * kb + dx, y0 = 152 * kb, y1 = 616 * kb, coin = 46 * kb;
            const vec2 p[5] = {{0, y0}, {x1 - coin, y0 + 14 * kb}, {x1, y0 + 14 * kb + coin}, {x1, y1}, {0, y1}};
            dessin2d::polygone(r, p, 5, dessin2d::Teinte{hex(0x232427, 0.95f * a), hex(0x111214, 0.96f * a), y0, y1});   // WFS-557 : gris neutre (maquette 01)
            const vec2 bordHaut[3] = {{0, y0}, {x1 - coin, y0 + 14 * kb}, {x1, y0 + 14 * kb + coin}};
            dessin2d::trait(r, bordHaut, 3, 2.5f * kb, vec4{1, 1, 1, 0.38f * a});
            const vec2 bordDroit[2] = {{x1, y0 + 14 * kb + coin}, {x1, y1}};
            dessin2d::trait(r, bordDroit, 2, 1.5f * kb, vec4{1, 1, 1, 0.10f * a});
            // WFS-557 (maquette 01) : tranche droite du panneau, plus claire (le panneau est un boîtier vu de trois quarts)
            const vec2 tranche[4] = {{x1 - 7 * kb, y0 + 14 * kb + coin - 3 * kb}, {x1, y0 + 14 * kb + coin}, {x1, y1}, {x1 - 7 * kb, y1 - 3 * kb}};
            dessin2d::polygone(r, tranche, 4, dessin2d::Teinte{hex(0x3A3B3E, 0.95f * a), hex(0x26272A, 0.95f * a), y0, y1});
        }
        // deux boutons ronds (rayon 116, centres à 152 et 426, hauteur 316) et la pilule « GARAGE » (32 à 532, 440 à 596)
        const float rb = 117 * kb;   // WFS-557 : bague de 117 de rayon (maquette 01)
        for (int i = 0; i < 2; i++) {
            a = glisse(2 + i, dx);
            const float cx = gl + (i == 0 ? 152 : 426) * kb + dx, cy = 318 * kb;
            g_zones[i] = {cx - rb, cy - rb, 2 * rb, 2 * rb, 0, true};
            manette::focus(g_zones[i]);   // WFS-22
            // WFS-557 (maquette 01 mesurée au pixel, ordre de l'utilisateur du 2026-11-16) : bague chrome brillante, centre gris
            ui::boutons::rondMenu(r, cx, cy, kb, ui::majuscules(ui::tr("menu.r19.course")), ui::majuscules(ui::tr(i == 0 ? "menu.r19.solo" : "menu.r19.en_ligne")),
                                  a, presse_ == i);
        }
        a = glisse(4, dx);
        {
            const float x = gl + 30 * kb + dx, y = 443 * kb, w = 503 * kb, h = 162 * kb;   // WFS-557 : place mesurée sur la maquette 01
            g_zones[SHOWROOM] = {x, y, w, h, 0, true};
            manette::focus(g_zones[SHOWROOM]);
            ui::boutons::piluleMenu(r, x, y, w, h, kb, ui::majuscules(ui::tr("menu.r19.garage")), a, presse_ == SHOWROOM);
        }
        // RECORD (bas gauche) : libellé gris espacé, valeur en grand ; le toucher ouvre le Profil (records, maquette 21)
        a = glisse(5, dx);
        {
            const float x = gl + 46 * kb + dx, y = H - gb - 82 * kb;
            { ui::AvecPolice inter(ui::Police::InterMedium); ui::text(r, x, y, 13 * kb, ui::majuscules(ui::tr("menu.record")), hex(0xB8BCC8, a), ui::GAUCHE, 0.22f, false); }   // WFS-557 : Inter Medium (maquette 01)
            const std::string v = g_best > 0 ? ui::nombre(g_best) : "—";
            ui::text(r, x, y + 16 * kb, 34 * kb, v, hex(presse_ == TA_VOITURE ? ACCENT2 : TEXTE, a), ui::GAUCHE, 0, false, true);
            const float zw = std::max(110 * kb, ui::width(34 * kb, v) + 20 * kb);
            g_zones[TA_VOITURE] = {x - 10 * kb, y - 10 * kb, zw, 70 * kb, 0, true};   // ≥ 44 px, sous « Garage » sans le chevaucher
            manette::focus(g_zones[TA_VOITURE]);
        }
        cadeau_jour::dessinerBouton(r, W, H, g.marges, presse_ == CADEAU);   // carte « Cadeau du jour » (bas droit)
        pret_bandeau::dessiner(r, W, H, g.marges, presse_ == PRET);         // carte « Voiture prêtée » au-dessus (maquette 26)
        compteur_etoiles::appuye(presse_ == COMPTEUR);   // WFS-537 (RG-07) : dessiné par son système (461), après « Réglages »
    }
private:
    // coffre avancé de 1,20 m vers l'allée (vers la caméra), centré dans l'image, tourné de 13° vers elle (maquette 05 : vu de
    // trois quarts face, au centre de l'image) ; tremblement autour de la verticale pendant l'animation
    mat4 placeCoffre() const { return voiture_ * translate({0.35f, 0, -1.20f}) * rotateY(0.22f + cadeau_jour::trembleCoffre()); }
    bool coffreEchec_ = false;
    void libererVitrine(Renderer& r) { vehicule_skin::libererVitrine(r); if (vitrine_.ok) r.destroy(vitrine_); vitrine_ = Model{}; }
    void chargerVitrine(Renderer& r) {
        // WFS-550 (ordre de l'utilisateur du 2026-11-16 : « la voiture dans le menu doit être exactement la même, c'est l'image
        // de marque du jeu ») : la SF-X de la maquette 01 (orange, finition caméléon, skin bleu ciel) est la voiture de vitrine
        // du menu, quelle que soit la voiture du joueur (la course, le garage et la sauvegarde ne changent pas)
        if (parking_.pret() && !vitrine_.ok && !vitrineEchec_) {
            const auto t0 = std::chrono::steady_clock::now();
            const double mo0 = memoire::gpuMo();
            if (loadCarModel(r, carById(VITRINE).file, vitrine_)) {
                vehicule_skin::poserVitrine(r, vitrine_, VITRINE, "ciel");   // 2e emplacement : la skin du joueur reste posée
                // maquette 01 : pas de plaque sur la voiture de vitrine ; la plaque de la SF-X est sa propre pièce (matériau n° 9 de
                // tools/plaques_voitures.mjs), retirée de cette copie seulement (la voiture de course garde ses plaques)
                auto sansPlaque = [](std::vector<ModelPart>& v) { v.erase(std::remove_if(v.begin(), v.end(), [](const ModelPart& p) { return p.material == MATERIAU_PLAQUE || p.material == MATERIAU_PLAQUE_AVANT; }), v.end()); };
                sansPlaque(vitrine_.parts); sansPlaque(vitrine_.partsLod);
                for (auto& n : vitrine_.partsNiveaux) sansPlaque(n);
                // maquette 01 : prises d'air du capot, crochet et détails jaunes d'origine (matériau n° 11) en bleu ciel, comme la skin
                if (MATERIAU_JAUNE < vitrine_.mats.size()) {
                    auto lin = [](float c) { return std::pow(c, 2.2f); };
                    vitrine_.mats[MATERIAU_JAUNE].color = {lin(0x9A / 255.f), lin(0xDC / 255.f), lin(0xF2 / 255.f), 1};
                }
                // revue du lead dev (PR #424) : coût de la vitrine, au journal (temps de chargement, mémoire graphique)
                WFS_LOG("menu : voiture de vitrine %s chargée en %.0f ms, %.1f Mo de mémoire graphique en plus (WFS-550)", VITRINE,
                        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(), memoire::gpuMo() - mo0);
            }
            else { vitrineEchec_ = true; WFS_ERR("menu : voiture de vitrine %s introuvable (la voiture du joueur reste affichée)", VITRINE); }
        }
    }
    // repère de la voiture du joueur posée sur la piste (même calcul que Game::carMatrix : tracé lissé, cap, pente)
    static mat4 matriceVoiture(const GameView& g) {
        const TrackPoint p = g.track->pointLisse(g.player->dist, g.player->playerX * RW);
        return translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + g.player->heading + g.player->spinYaw)) * rotateX((float)p.slope);
    }
    parking::Parking& parking_ = parking::commun();   // WFS-552 ; WFS-558 : instance partagée avec le duel en ligne
    static constexpr const char* VITRINE = "sf90";   // Cavallo SF-X (nom inventé du catalogue)
    static constexpr uint32_t MATERIAU_PLAQUE = 9, MATERIAU_PLAQUE_AVANT = 53;   // plaques arrière et avant de sf90.wfm (tools/plaques_voitures.mjs)
    static constexpr uint32_t MATERIAU_JAUNE = 11;      // détails jaunes d'origine de sf90.wfm (couleur 0,97 / 0,70 / 0,01)
    Model vitrine_;              // WFS-550 : voiture de vitrine du menu (seulement pendant le menu)
    bool vitrineEchec_ = false;  // modèle introuvable : pas de nouvel essai à chaque image
    Renderer* r_ = nullptr;      // pour charger / libérer la vitrine aux changements d'écran
    mat4 voiture_ = mat4::identity();
    double t0_ = -1;
    bool finVue_ = false;
    int presse_ = -1, doigt_ = -1;   // choix à l'état pressé, doigt qui le tient (WFS-38)
};

} // namespace

// WFS-505 (banc « textes ») : centre d'un choix du menu à la dernière image : « solo », « duel », « showroom »
bool menuBouton(const std::string& nom, float& x, float& y) {
    const int i = nom == "solo" ? SOLO : nom == "duel" ? DUEL : nom == "showroom" ? SHOWROOM : -1;
    if (i < 0 || !g_zones[i].ok) return false;
    x = g_zones[i].x + g_zones[i].w * 0.4f; y = g_zones[i].y + g_zones[i].h * 0.5f; return true;   // hors du coin coupé
}

void benchMenu(Game& g, Renderer& r, const std::string& out) {
    if (!g.init(r, "golf", "ville")) { WFS_ERR("menu : chargement impossible"); return; }
    g.setAutopilot(false); g.camMode = 0;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);   // la scène se pose
    g.allerA(Ecran::Menu);
    const double t = 700;
    g.render(t);                                        // entrée dans le menu (début des animations)
    g.render(t + 0.1); r.capturePng(out + "/menu-anim.png");
    WFS_LOG("capture menu-anim : écran %s, 100 ms après l'entrée", nomEcran(g.ecran()));
    g.render(t + 1.0); g.render(t + 1.01); r.capturePng(out + "/menu-0.png");
    WFS_LOG("capture menu-0 : écran %s, menu posé (1 s)", nomEcran(g.ecran()));
    g.shutdown();
}

// WFS-38 : parcours tactile du menu et de l'écran de fin, touchers simulés comme ceux d'un doigt
void benchMenuTactile(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("menu tactile : chargement impossible"); courseBenchRules(false); return; }
    g.setAutopilot(false); g.camMode = 0;
    const float W = (float)r.width(), H = (float)r.height();
    double t = 800;
    int n = 0;   // rendu une image sur 10 seulement (comme les autres scènes) : le banc reste rapide en 2400x1080
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 10 == 0) g.render(t); };
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : écran %s, %s", nom, nomEcran(g.ecran()), quoi);
    };
    auto centre = [](int i, float& x, float& y) { x = g_zones[i].x + g_zones[i].w * 0.4f; y = g_zones[i].y + g_zones[i].h * 0.5f; };
    // WFS-41 : écran Terrain → appui sur « Rouler » (carte inchangée : ville), partie reconstruite au pas suivant
    auto rouler = [&]() {
        for (int i = 0; i < 3; i++) image();
        g.render(t); float rx, ry;
        if (centreRouler(rx, ry)) { g.onTouch({20, rx, ry, true}); image(); g.onTouch({20, rx, ry, false}); image(); }
    };
    auto auMenu = [&]() { g.allerA(Ecran::Menu); for (int i = 0; i < 60; i++) image(); };   // 1 s : menu posé
    float x, y;
    for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    auMenu();
    // glisser hors du choix annule : appui sur « Course solo », glissé vers le bas de l'écran, relâcher → on reste au menu
    centre(SOLO, x, y);
    g.onTouch({1, x, y, true}); image(); g.onTouch({1, x, H * 0.95f, true}); image(); g.onTouch({1, x, H * 0.95f, false});
    for (int i = 0; i < 5; i++) image();
    WFS_LOG("menu tactile : appui puis glissé hors de « Course solo » → écran %s (annulé)", nomEcran(g.ecran()));
    // menu-appui : « Course solo » tenu
    g.onTouch({2, x, y, true}); image();
    capture("menu-appui", "« Course solo » à l'état pressé");
    // relâcher → garage (WFS-11) → « Choisir » → course remise à zéro, décompte 3-2-1-GO de WFS-12
    g.onTouch({2, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({7, gx, gy, true}); image(); g.onTouch({7, gx, gy, false}); }
    rouler();   // WFS-41 : garage → terrain → « Rouler »
    const Course* k = nullptr;
    bool trois = false, go = false;
    for (int i = 0; i < 60 * 5 && !(trois && go); i++) {
        image();
        k = activeCourse();
        const std::string c = k ? k->countText() : "";
        if (!trois && c == "3") { trois = true; capture("menu-depart-3", "décompte « 3 » après le relâcher"); }
        if (!go && c == "GO") { for (int f = 0; f < 6; f++) image(); go = true; capture("menu-depart-go", "« GO », départ de la course"); }
    }
    // fin-menu : course depuis le menu, temps forcé à 0,05 s, écran de fin (boutons actifs 0,9 s après la fin)
    centre(SOLO, x, y); g.onTouch({5, x, y, true}); image(); g.onTouch({5, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({8, gx, gy, true}); image(); g.onTouch({8, gx, gy, false}); }   // WFS-11
    rouler();   // WFS-41 : garage → terrain → « Rouler »
    g.setAutopilot(true);                        // une vraie petite course : score et record non nuls
    for (int i = 0; i < 60 * 8; i++) image();   // décompte (3 s) puis 5 s de course
    g.setAutopilot(false);
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) image();
    capture("fin-menu", "écran de fin avec « Rejouer » et « Menu »");
    // bouton « Menu » (CourseSystem::drawOver, WFS-57 : paire centrée, Rejouer 40 u + 2 u + Menu 28 u, à y = 70 u)
    const float u = H / 100, bw = 40 * u, bx = W / 2 - (bw * 1.7f + 2 * u) / 2 + bw + 2 * u;
    g.onTouch({6, bx + bw * 0.35f, 70 * u + 5 * u, true}); image(); g.onTouch({6, bx + bw * 0.35f, 75 * u, false});
    for (int i = 0; i < 60; i++) image();
    capture("fin-retour-menu", "retour au menu après « Menu »");
    // WFS-96 : « Showroom » → garage en mode regarder ; voiture suivante ; « Choisir » → retour au menu avec la voiture
    centre(SHOWROOM, x, y); g.onTouch({9, x, y, true}); image(); g.onTouch({9, x, y, false});
    for (int i = 0; i < 30; i++) image();
    if (float gx, gy; garageBouton("suivant", gx, gy)) { g.onTouch({10, gx, gy, true}); image(); g.onTouch({10, gx, gy, false}); }
    for (int i = 0; i < 40; i++) image();
    capture("menu-showroom", "garage ouvert depuis « Showroom », voiture suivante");
    if (float gx, gy; garageBouton("choisir", gx, gy)) { g.onTouch({11, gx, gy, true}); image(); g.onTouch({11, gx, gy, false}); }
    for (int i = 0; i < 60; i++) image();
    capture("showroom-choisir", "retour au menu après « Choisir », nouvelle voiture sous « Ta voiture »");
    WFS_LOG("menu : « Choisir » depuis le Showroom → écran %s", nomEcran(g.ecran()));
    // « Ta voiture » (pied du menu) → garage en mode regarder ; « Retour » → menu
    // WFS-550 (RG-11) : bloc « RECORD » → Profil ; puis fermé
    centre(TA_VOITURE, x, y); g.onTouch({12, x, y, true}); image(); g.onTouch({12, x, y, false});
    for (int i = 0; i < 30; i++) image();
    WFS_LOG("menu : « RECORD » → Profil %s", profil_ecran::ouvert() ? "ouvert" : "NON ouvert (ÉCHEC)");
    profil_ecran::fermer();
    for (int i = 0; i < 30; i++) image();
    g.shutdown();
    unsetenv("WFS_MOMENT");   // WFS-41 : moment choisi à l'écran Terrain
    courseBenchRules(false);
}

} // namespace ecrans

WFS_SYSTEM(ecrans::Menu, 450)   // interface (400-499) : après le HUD (420), avant le fondu du chargement (490)

} // namespace wfs
