// WaForSpeed — garage (WFS-11, WFS-40) : voir garage.h. WFS-534 : ce fichier ne garde que l'écran (système « garage »,
// ordre 440), la mise en page, les touchers et l'enchaînement des panneaux (garage_*.cpp).
#include "garage.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/course5.h"   // WFS-115 : « Retour » du parcours de course = panneau des modes
#include "../render/vehicule_plaque.h"   // WFS-483 : plaque 3D du joueur
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "text.h"
#include "appui.h"   // WFS-113
#include "../platform/manette.h"   // WFS-22
#include "../audio/sfx.h"   // WFS-96 : son du changement de voiture
#include "garage_achat.h"
#include "garage_cadre.h"
#include "garage_couleur.h"
#include "garage_decor.h"
#include "garage_fiche.h"
#include "garage_finition.h"
#include "garage_skin.h"

namespace wfs {
namespace ecrans {
using namespace garage;
namespace {
bool g_regarder = false;   // WFS-96 : garageFor = 'look'
// WFS-99 (RG-05, ANO-33) : cap de départ 0,6 + π, voiture vue de 3/4 avant (gYaw = 0,6 de garage.js la montrait de l'arrière)
constexpr float CAP_DEPART = 0.6f + PI;
class Garage;
Garage* g_garage = nullptr;   // garage de la partie en cours (garageBouton, son du showroom, bancs)

class Garage : public System, public AccesBancGarage {
public:
    Garage() { g_garage = this; }
    ~Garage() override { if (g_garage == this) g_garage = nullptr; }
    const char* name() const override { return "garage"; }

    void init(Renderer& r, const GameView&) override {
        r_ = &r;
        decor_.init(r);
        for (int i = 0; i < CAR_COUNT; i++) peinture_[i] = peintureParId(ficheDe(CARS[i].id).paint);
        for (int i = 0; i < CAR_COUNT; i++)   // WFS-24 ; WFS-487 : une couleur hors des 16 pastilles du garage actuel garde la pastille d'origine
            if (const PeintureDef* p = sauvegarde().peintureChoisie(CARS[i].id); p && p - PEINTURES < NB_PEINTURES) peinture_[i] = peintureParId(p->id);
    }

    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        decor_.shutdown(r);
        if (voiture_.ok) r.destroy(voiture_);
        charge_ = -1;
    }

    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        if (nouveau == Ecran::Garage) {   // openGarage : voiture de la partie, plateau au cap de départ, panneau qui glisse
            appui_.vider();   // WFS-113 : aucun doigt retenu d'un écran précédent
            bouton_ = 0; doigtBouton_ = -1;   // WFS-115
            index_ = indexDe(g.car); yaw_ = CAP_DEPART; slide_ = 0; t0_ = -1; doigt_ = -1;
            couleur_.ouvrir(peinture_);   // WFS-489 (accroche) : dernières couleurs possédées, aperçu oublié (RG-03)
            charger(index_);
            WFS_LOG("garage : %s %s (%s / %d), peinture %s", CARS[index_].brand, CARS[index_].model, deuxChiffres(placeDe(index_) + 1).c_str(),
                    voituresPermises().n, PAINTS[peinture_[index_]].nom);
        }
        if (ancien == Ecran::Garage) {
            g_z.prev = g_z.next = Zone{};
            for (PanneauGarage* p : panneaux_) p->viderZones();
            liberer();   // mémoire rendue pendant la course
            // « Choisir » : Garage → Menu → Course, appliqués avant l'image suivante (le menu n'est jamais dessiné) ;
            // l'entrée en course depuis le menu remet la course à zéro (même chemin que « Recommencer » de la pause)
            if (nouveau == Ecran::Menu && choisi_ && !g_regarder) { g.allerA(Ecran::Terrain); }   // WFS-41 : étape 2, le terrain
            // WFS-115 (RG-03) : « Retour » du garage ouvert par le parcours de course = un seul cran, le panneau des modes
            // (sans sauvegarde, banc d'essai, ouvrirChoixMode ne fait rien : le menu, comme avant)
            if (nouveau == Ecran::Menu && !choisi_ && !g_regarder && retour_) ouvrirChoixMode();
            retour_ = false;
            choisi_ = g_regarder = false;   // WFS-96 : mode regarder → on reste au menu
        }
    }

    void update(double dt, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return;
        if (doigt_ < 0) yaw_ += (float)dt * 0.35f;                       // renderGarage : la voiture tourne seule
        slide_ *= std::exp(-(float)dt * 7); if (std::abs(slide_) < 0.002f) slide_ = 0;   // arrivée en glissant sur le côté
    }

    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return false;
        decor_.regler(fp, s, g, CARS[index_]);
        return true;
    }

    void render(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return;
        decor_.dessiner(r, ficheDe(CARS[index_].id).glow);
        if (voiture_.ok) {   // RG-06 : que des vraies voitures, pas de silhouette provisoire
            const vec3 p = lin(PAINTS[peinture_[index_]].hex);
            const mat4 M = translate({slide_ * 7, 0.08f, 0}) * rotateY(yaw_);
            r.peinture2ProchaineVoiture(couleur_.code2(index_));   // WFS-489 (accroche) : toit et rétros montrés (voiture seulement : le renderer remet la couleur 2 à zéro après ce dessin)
            r.draw(voiture_, M, &p);
            plaque_joueur::dessiner(r, CARS[index_].id, M, g.night, vec3{M.at(0, 3), M.at(1, 3), M.at(2, 3)});   // WFS-483 (accroche) : plaque complète (RG-06)
        }
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Garage) return false;
        // WFS-113 : un doigt = un appui. Le doigt qui a actionné un bouton est ignoré jusqu'à ce qu'il se relève
        // (sur Android, ses micro-mouvements arrivent aussi comme des appuis : la flèche sautait des voitures)
        if (appui_.consomme(t.id, t.down)) return true;
        // WFS-115 : « Choisir » et « Retour » agissent au relâcher, sur le bouton appuyé : le doigt est levé quand l'écran
        // change, ses mouvements n'atteignent jamais l'écran suivant (ils y arrivaient comme des appuis neufs)
        if (t.id == doigtBouton_) {
            if (t.down) return true;
            const int b = bouton_, z = achat_.appui(t.x, t.y); bouton_ = 0; doigtBouton_ = -1;
            if (b == AchatGarage::CHOISIR && z == AchatGarage::CHOISIR) { if (!achat_.acheter()) choisir(g); }   // WFS-489 (accroche) : « Acheter N ★ »
            else if (b == AchatGarage::RETOUR && z == AchatGarage::RETOUR) { WFS_LOG("garage : « Retour »"); choisi_ = false; retour_ = true; g.allerA(Ecran::Menu); }
            return true;
        }
        if (const int c = couleur_.toucher(t, doigt_ < 0 && doigtBouton_ < 0, index_, peinture_[index_]); c) { if (c == 1) appui_.prendre(t.id); return true; }   // WFS-489 (accroche) : rangée qui défile
        if (t.down) {
            if (doigt_ >= 0 && t.id == doigt_) {   // glisser : tourner la voiture (0,01 rad par pixel)
                if (glisse_) yaw_ = yaw0_ + (t.x - x0_) * 0.01f * (720.f / g.uiHeight);
                return true;
            }
            if (doigt_ >= 0) return true;
            // boutons et panneaux, dans l'ordre d'avant le découpage : action à l'appui (comme les boutons de la pause)
            if (g_z.prev.contient(t.x, t.y)) { appui_.prendre(t.id); cycle(-1); return true; }
            if (g_z.next.contient(t.x, t.y)) { appui_.prendre(t.id); cycle(1); return true; }
            if (const int p = fiche_.appui(t.x, t.y); p >= 0) { appui_.prendre(t.id); if (const int i = voituresPermises().idx[p]; i != index_) aller(p, i > index_ ? 1 : -1); return true; }
            // WFS-491 / WFS-494 : finition et skin (emplacements vides aujourd'hui : aucune zone, appui toujours -1)
            if (finition_.appui(t.x, t.y) >= 0 || skin_.appui(t.x, t.y) >= 0) { appui_.prendre(t.id); return true; }
            if (const int b = achat_.appui(t.x, t.y); b >= 0) { bouton_ = b; doigtBouton_ = t.id; return true; }   // WFS-115 : action au relâcher
            doigt_ = t.id; glisse_ = true; x0_ = t.x; yaw0_ = yaw_; tAppui_ = g.time;   // gDrag
            return true;
        }
        if (t.id != doigt_) return true;
        // geste rapide (< 320 ms, > 70 px) : voiture suivante / précédente
        const float dx = (t.x - x0_) * (720.f / g.uiHeight);
        if (glisse_ && g.time - tAppui_ < 0.32 && std::abs(dx) > 70) { yaw_ = yaw0_; cycle(dx < 0 ? 1 : -1); }
        doigt_ = -1; glisse_ = false;
        return true;   // le garage garde tous les touchers
    }

    // mise en page et enchaînement des panneaux, dans l'ordre de dessin d'avant le découpage (mêmes pixels)
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Garage || !ui::fontReady()) return;
        if (t0_ < 0) t0_ = g.time;
        MiseEnPage mp;
        const float k = mp.k = g.uiHeight / 720.f;
        mp.W = g.uiWidth; mp.H = g.uiHeight; mp.op = std::clamp((float)(g.time - t0_) / 0.3f, 0.f, 1.f);
        mp.gl = g.marges.gauche + 16 * k; mp.gr = g.marges.droite + 16 * k; mp.gt = g.marges.haut + 14 * k; mp.gb = g.marges.bas + 14 * k;
        const CarDef& car = CARS[index_];
        const EtatGarage e{index_, peinture_[index_], voiture_.ok, couleur_.peinture2(index_)};   // WFS-489 (accroche)
        cadre::voiles(r, mp);
        fiche_.dessiner(r, mp, e);   // .show-head : marque · index, nom, type + accroche, points des voitures
        cadre::fleches(r, mp);
        // ---------- .show-panel : fiche, barres, peintures, boutons ----------
        mp.padX = 24 * k; mp.padY = 18 * k;
        mp.statH = 4 * 14 * k + 3 * 10 * k; mp.paintH = CouleurGarage::hauteur(k); mp.panelH = mp.padY * 2 + mp.statH + 14 * k + mp.paintH;   // WFS-489 (accroche) : ligne des prix
        mp.px0 = mp.gl; mp.px1 = mp.W - mp.gr; mp.py0 = mp.H - mp.gb - 26 * k - mp.panelH;
        cadre::fondPanneau(r, mp);
        mp.y1 = mp.py0 + mp.padY;
        const float xStats = fiche_.specs(r, mp, car);   // .specs : Puissance, 0 à 100, Vitesse max
        mp.bw = 200 * k; mp.bx = mp.px1 - mp.padX - mp.bw;
        achat_.dessiner(r, mp, e);   // Choisir, Retour (achat : WFS-489 / 499 / 502)
        for (const Zone* z : {&g_z.choisir, &g_z.retour, &g_z.prev, &g_z.next}) manette::focus(*z, z == &g_z.retour);   // WFS-22
        fiche_.stats(r, mp, car, xStats);   // .statlist : 4 barres sur 10
        couleur_.dessiner(r, mp, e);        // .paint-line (WFS-489)
        finition_.dessiner(r, mp, e);       // vide (WFS-491)
        skin_.dessiner(r, mp, e);           // vide (WFS-494)
        cadre::pied(r, mp, car, voiture_.ok);
    }

    // ---------- accès du banc d'essai (garage_bancs.cpp) ----------
    // planche des 16 peintures (4 × 4 voitures sur le sol du garage)
    void dessinerPlanche(Renderer& r) override {
        for (int i = 0; i < 16; i++) {   // WFS-489 (accroche) : planche des 16 peintures historiques, inchangée
            const vec3 p = lin(PAINTS[i].hex);
            const mat4 M = translate({(i % 4 - 1.5f) * 5.2f, 0.0f, (i / 4 - 1.5f) * 2.7f}) * rotateY(PI / 2);
            if (voiture_.ok) { r.draw(voiture_, M, &p); plaque_joueur::dessiner(r, CARS[index_].id, M, false, vec3{M.at(0, 3), M.at(1, 3), M.at(2, 3)}); }   // WFS-483
        }
    }
    int index() const override { return index_; }
    void choisirPeinture(int i) override { peinture_[index_] = i; }
    bool reglerScene(FrameParams& fp, SceneDediee& s, const GameView& g) override { return scene(fp, s, g); }

private:
    void charger(int i) {
        if (charge_ == i && voiture_.ok) return;
        liberer();
        if (!loadCarModel(*r_, CARS[i].file, voiture_)) WFS_ERR("garage : modèle %s introuvable", CARS[i].file);
        plaque_joueur::voiturePreparee(*r_, CARS[i].id);   // WFS-483 (accroche) : porte-plaques de cette voiture
        charge_ = i;
    }
    void liberer() {
        if (!r_ || !voiture_.ok) return;
        r_->vk().waitIdle(); r_->destroy(voiture_); voiture_ = Model{}; charge_ = -1;
    }
    void aller(int place, int dir) {   // cycleCar / selectCar : nouvelle voiture, arrivée en glissant (gSlide = dir)
        const VoituresPermises& v = voituresPermises();   // WFS-404 : place parmi les voitures permises
        const int p = (place % v.n + v.n) % v.n;
        index_ = v.idx[p]; slide_ = (float)dir; sfxInterface(SfxId::Garage);   // WFS-96 : selectCar
        charger(index_);
        WFS_LOG("garage : %s %s (%s / %d)", CARS[index_].brand, CARS[index_].model, deuxChiffres(p + 1).c_str(), v.n);
    }
    void cycle(int dir) { aller(placeDe(index_) + dir, dir); }
    // « Choisir » : voiture et peinture retenues ; parcours de course → terrain (via onEcran), Showroom → menu (WFS-96)
    void choisir(const GameView& g) {
        if (!voiture_.ok) return;   // « Chargement… » : bouton inactif
        const CarDef& c = CARS[index_];
        WFS_LOG("garage : « Choisir » %s %s, peinture %s", c.brand, c.model, PAINTS[peinture_[index_]].nom);
        if (g.choisirVoiture) g.choisirVoiture(c.id, lin(PAINTS[peinture_[index_]].hex));
        if (Sauvegarde& s = sauvegarde(); s.actif()) {   // WFS-24
            s.car = c.id; s.paint[c.id] = PAINTS[peinture_[index_]].id;
            s.enregistrer();
        }
        choisi_ = true; g.allerA(Ecran::Menu);
    }

    Renderer* r_ = nullptr;
    DecorGarage decor_;   // sol, plateau, anneau, barres, lumières (garage_decor.cpp)
    FicheGarage fiche_; CouleurGarage couleur_; FinitionGarage finition_; SkinGarage skin_; AchatGarage achat_;   // panneaux (WFS-534)
    PanneauGarage* const panneaux_[5] = {&fiche_, &couleur_, &finition_, &skin_, &achat_};
    Model voiture_; int charge_ = -1;
    int index_ = 0, peinture_[CAR_COUNT] = {};
    float yaw_ = CAP_DEPART, slide_ = 0, yaw0_ = 0, x0_ = 0;
    double t0_ = -1, tAppui_ = 0;
    int doigt_ = -1; bool glisse_ = false, choisi_ = false;
    int bouton_ = 0, doigtBouton_ = -1; bool retour_ = false;   // WFS-115 : bouton appuyé (1 Choisir, 2 Retour) et son doigt
    ui::FiltreAppui appui_;   // WFS-113 : un doigt = un appui
};
using GarageEnregistre = Garage;   // nom du système dans le journal (« Système : ecrans::GarageEnregistre »), inchangé
} // namespace

void garageRegarder() { g_regarder = true; }
AccesBancGarage* garage::garageEnCours() { return g_garage; }

bool garageBouton(const char* nom, float& x, float& y) {
    const Zone* z = std::strcmp(nom, "choisir") == 0 ? &g_z.choisir : std::strcmp(nom, "retour") == 0 ? &g_z.retour
                  : std::strcmp(nom, "suivant") == 0 ? &g_z.next : std::strcmp(nom, "precedent") == 0 ? &g_z.prev : nullptr;
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2;
    return true;
}

// WFS-127 : voiture affichée au garage (lecture seule, son du showroom) ; nullptr si le garage n'existe pas
const char* garageVoitureAffichee() { return g_garage ? CARS[g_garage->index()].id : nullptr; }

} // namespace ecrans

WFS_SYSTEM(ecrans::GarageEnregistre, 440)   // interface (400-499) : avant le menu (450)

} // namespace wfs
