// WaForSpeed — course à 5 voitures (WFS-110) branchée sur le moteur par les systèmes enfichables (WFS-30) : voir course5.h.
// Le départ, le décompte 3-2-1-GO et la ligne d'arrivée sont ceux de la course (course_system, Course::duelFinish comme
// le duel) ; ce module tire et fait rouler les 4 ordinateurs, applique les contacts (règle du duel), dessine les voitures,
// « Position n/5 » avec les barres de distance, et l'écran de fin (5 temps, « Rejouer », « Menu »).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>   // WFS-183 : std::getenv (banc)
#include <map>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "../ui/appui.h"
#include "../ui/text.h"
#include "course5.h"
#include "course_system.h"
#include "duel_system.h"
#include "game.h"
#include "player.h"
#include "poursuite.h"   // WFS-141 : titrePlace
#include "sauvegarde.h"
#include "traffic.h"
#include "traffic_system.h"
#include "trace_cartes.h"
#include "virages_banc.h"   // WFS-327 : épingles de la Corniche (banc)
#include "../audio/voitures_sons.h"   // WFS-272 (accroche) : déclaration des lectures pour le son des voitures

namespace wfs {

// RG-03 / RG-07 : ni trafic ni voitures de décor pendant une course à 5 ou un duel
bool sansTrafic(bool enCourse) { return enCourse && (modeCourse() == ModeCourse::Course5 || duelSession().inRoom()); }

namespace {

uint32_t g_graineBanc = 0;   // banc d'essai : tirage fixe (captures stables)
bool g_vueDessusBanc = false;   // WFS-242 (RG-06) : banc d'essai : grille de départ vue de dessus (scène dédiée)
// WFS-243 (RG-01, RG-06) : banc d'essai : joueur posé à g_bancJoueurM (m), ordinateurs à g_bancAdversaires (m devant lui), au
// pas suivant ; puis journal, à chaque image, de la distance du classement, de la distance dessinée et de la largeur à l'écran
std::vector<double> g_bancAdversaires; double g_bancJoueurM = 0; bool g_journalAdversaires = false;
bool g_bancAvant = false;   // WFS-243 : banc : image « avant » (adversaires coupés par la distance, comme avant le ticket)
bool g_figerBanc = false;   // WFS-243 : banc : ordinateurs arrêtés là où le banc les a posés
// WFS-327 (RG-04) : banc : ordinateurs posés à g_bancVitesseKmh (km/h) ; vue de dessus au-dessus de l'épingle en g_bancEpingleM (m)
double g_bancVitesseKmh = 0, g_bancEpingleM = 0;

const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f}, SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};
const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1}, PISTE{1, 1, 1, 0.12f}, MUTED{138 / 255.f, 144 / 255.f, 166 / 255.f, 1};
vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };

class Course5System : public System {
public:
    const char* name() const override { return "course5"; }
    void init(Renderer& r, const GameView&) override { r_ = &r; actif_ = false; os_.clear(); }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        for (auto& [id, m] : models_) r.destroy(m);
        models_.clear(); os_.clear(); actif_ = false;
    }
    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        // « Rejouer » : Course → Menu → Course (le menu n'est jamais dessiné), la course repart avec les mêmes ordinateurs
        if (ancien == Ecran::Course && nouveau == Ecran::Menu && rejouer_) g.allerA(Ecran::Course);
        if (nouveau == Ecran::Menu && !rejouer_) {
            actif_ = false; graineFixe_ = false;
            os_.clear(); libererHorsGrille();   // WFS-130 (RG-01) : retour au menu hors « Rejouer » : plus aucun ordinateur gardé
        }
        if (nouveau == Ecran::Course) rejouer_ = false;
    }
    void update(double, const GameView&) override {
        if (!r_) return;
        for (const course5::Ordinateur& o : os_) {   // modèles complets des 4 voitures (chargés une fois)
            if (models_.count(o.car)) continue;
            Model m; loadCarModel(*r_, std::string(carById(o.car.c_str()).file), m);
            models_[o.car] = std::move(m);
        }
    }
    // après la course (210) et le duel (220) : départ en grille, conduite des ordinateurs, contacts, arrivée
    void collide(double dt, Player& pl, const GameView& g) override {
        Course* c = activeCourseMut();
        if (modeCourse() != ModeCourse::Course5 || g.ecran != Ecran::Course || !c || duelSession().inRoom()) { actif_ = false; return; }
        if (c->duelFinish <= 0) {   // course remise à zéro (départ du menu, « Rejouer ») : nouvelle grille
            // WFS-183 (RG-04) : au banc (WFS_BANC défini) graine fixe 1234 (minicarte-ligne, course5-rejouer) ; en jeu, horloge
            if (!graineFixe_) { graine_ = g_graineBanc ? g_graineBanc : std::getenv("WFS_BANC") ? 1234u : (uint32_t)(nowSeconds() * 1000.0) * 2654435761u + 17u; graineFixe_ = true; }
            c->duelFinish = kmCourse5() * duel::KM;
            os_ = course5::tirer(graine_, pl.car->id, course5::difficulte());   // WFS-133 (RG-03) : difficulté choisie
            // WFS-242 (RG-01, RG-02) : grille derrière la ligne, place du joueur tirée au sort (même graine : même place)
            placeJoueur_ = std::min((int)os_.size(), course5::tirerPlaceJoueur(graine_));
            course5::grille(os_, placeJoueur_); depasses_.reset();
            course5::placerSurGrille(pl, *g.track, course5::placeGrille(placeJoueur_));
            libererHorsGrille();   // WFS-130 (RG-01 / RG-02) : « Rejouer » retire les mêmes voitures, rien n'est détruit
            actif_ = true; fini_ = false; depart_ = false; ftJoueur_ = 0; overT_ = 0; t_ = 0; nouveauRecord_ = false;
            ecartMin_ = ecartMinimal(pl); ecartLogue_ = false;   // WFS-242 (RG-06) : sur la grille, puis pendant les 5 premières secondes
            WFS_LOG("course à 5 : grille : distance minimale entre deux voitures %.2f m", ecartMin_);
            std::string noms; for (const auto& o : os_) noms += (noms.empty() ? "" : ", ") + o.car;
            WFS_LOG("course à 5 : graine %u, %d km, ordinateurs %s (difficulté %s), joueur à la place %d/%d", graine_, kmCourse5(), noms.c_str(),
                    course5::cleDifficulte(course5::difficulte()), placeJoueur_ + 1, (int)os_.size() + 1);
        }
        if (actif_ && !g_bancAdversaires.empty()) {   // WFS-243 : banc d'essai seulement (ligne droite, voitures arrêtées)
            course5::Place p; p.s = g_bancJoueurM / M; p.x = course5::FILES[1];
            course5::placerSurGrille(pl, *g.track, p);
            for (size_t k = 0; k < os_.size() && k < g_bancAdversaires.size(); k++) {
                course5::Ordinateur& o = os_[k];
                o.file = (int)(k % 3); o.g.x = o.cibleX = course5::FILES[o.file];
                o.g.rd = o.g.rdPrec = pl.dist + g_bancAdversaires[k] / M; o.g.xPrec = o.g.x; o.g.v = g_bancVitesseKmh / 3.6 / M;
            }
            g_bancAdversaires.clear();
        }
        if (!actif_ || c->state == RaceState::Count || g_figerBanc) return;   // décompte : tout le monde attend sur la grille ; banc : figés
        if (!depart_ && c->state == RaceState::Play) {   // WFS-130 (RG-06) : ordinateurs prêts à être dessinés au départ
            depart_ = true;
            int prets = 0; for (const auto& o : os_) { auto it = models_.find(o.car); prets += it != models_.end() && it->second.ok; }
            WFS_LOG("course à 5 : départ, %d/%zu ordinateurs avec leur modèle, %zu modèle(s) d'ordinateur en mémoire", prets, os_.size(), models_.size());
        }
        t_ = c->state == RaceState::Play ? c->raceTime : t_ + dt;
        if (c->state == RaceState::Play && !ecartLogue_) {   // WFS-242 (RG-06) : distance minimale entre deux voitures au départ
            ecartMin_ = std::min(ecartMin_, ecartMinimal(pl));
            if (t_ >= 5) {
                ecartLogue_ = true;
                WFS_LOG("course à 5 : départ : distance minimale entre deux voitures pendant les 5 premières secondes : %.2f m", ecartMin_);
            }
        }
        std::vector<int> nitros; for (const auto& o : os_) nitros.push_back(o.nitrosPrises);
        course5::avancer(os_, dt, c->duelFinish, t_, pl.dist, pl.playerX, pl.speed, g.track);   // WFS-189 : virages
        if (c->state == RaceState::Play && !fini_) {   // WFS-189 (RG-04) : nitro rechargée au temps et aux dépassements
            const int n = depasses_.compter(os_, pl.dist);
            pl.nitro = course5::rechargerNitro(pl.nitro, pl.nitroOn, dt, n);
            if (n) WFS_LOG("course5 : %d adversaire(s) dépassé(s), nitro %.2f", n, pl.nitro);
        }
        for (size_t k = 0; k < os_.size(); k++)   // WFS-133 (RG-06) : nitro des ordinateurs (vitesse seule, rendu inchangé)
            if (os_[k].nitrosPrises > nitros[k]) WFS_LOG("course5 : nitro de %s (%.2f km)", os_[k].nom.c_str(), os_[k].g.rd * M / 1000);
        // WFS-133 (RG-05) : aspiration du joueur derrière un ordinateur (même file, moins de 25 m) : +4 % de vitesse visée,
        // appliquée ici à la vitesse du joueur (player.* inchangé), bornée à 1,04 × sa vitesse de pointe
        const bool asp = c->state == RaceState::Play && pl.speed > 0 && course5::dansAspiration(os_, pl.dist, pl.playerX);
        if (asp) {
            const double bonus = g_configJeu.course5.aspirationBonus, vmax = MAX_SPEED * pl.perf.top * (1 + bonus);
            if (pl.speed < vmax) pl.speed = std::min(vmax, pl.speed * (1 + bonus * dt));
        }
        if (asp != aspJoueur_) { aspJoueur_ = asp; if (asp) WFS_LOG("course5 : aspiration du joueur"); }
        if (c->state == RaceState::Play)
            for (course5::Ordinateur& o : os_) {   // RG-05 : contacts comme au duel (hitGhosts)
                const duel::ContactEvents e = duel::contacter(dt, pl, *g.track, *g.car, o.g);
                if (e.accident) c->accident(); else if (e.chocs) c->bump(e.impact);
                // WFS-333 (RG-03) : duel::contacter ne bouge que le joueur : l'adversaire ne reste jamais dans sa voiture
                if (const CarDef& oc = carById(o.g.car.c_str()); separerDuJoueur(pl, o.g.rd, o.g.x, oc.d.L, oc.d.W) && o.g.rd < pl.dist) o.g.v = std::min(o.g.v, pl.speed);
            }
        if (c->state == RaceState::Over) {
            if (!fini_) {
                fini_ = true; ftJoueur_ = c->raceTime;
                const auto ls = lignes(pl);
                WFS_LOG("course à 5 : arrivée en %.2f s, position %d/5", ftJoueur_, course5::position(ls));
                // WFS-125 (RG-02) : record Course à 5 (meilleure place, puis meilleur temps), écrit seulement s'il est battu
                nouveauRecord_ = false;
                Sauvegarde& sv = sauvegarde();
                if (sv.actif()) {
                    nouveauRecord_ = sv.battreRecordCourse5(course5::position(ls), (long long)std::llround(ftJoueur_ * 1000));
                    WFS_LOG("course à 5 : record %s%s", texteRecord().c_str(), nouveauRecord_ ? " (NOUVEAU RECORD, enregistré)" : "");
                    if (nouveauRecord_) sv.enregistrer();
                }
            }
            overT_ += dt;
        }
    }
    // WFS-242 (RG-06) : banc d'essai seulement : grille vue de dessus, à 42 m de haut au-dessus de la 3e place, le haut de
    // l'image vers l'avant de la course ; Game dessine la route (et la ligne de départ) et la voiture du joueur, render() les ordinateurs
    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        if (!g_vueDessusBanc || !actif_ || !g.track || g.ecran != Ecran::Course) return false;
        const bool epingle = g_bancEpingleM > 0;   // WFS-327 (RG-04) : toute l'épingle, à 75 m de haut, entrée en bas de l'image
        const double s0 = epingle ? g_bancEpingleM / M : course5::placeGrille(2).s;
        const TrackPoint a = g.track->point(s0, 0), b = g.track->point(epingle ? s0 - 30 / M : s0 + 10 / M, 0);
        const vec3 cible{(float)a.x, (float)a.y, (float)a.z}, avant = normalize(epingle ? vec3{(float)(a.x - b.x), 0, (float)(a.z - b.z)} : vec3{(float)(b.x - a.x), 0, (float)(b.z - a.z)});
        const vec3 oeil = cible - avant * 4.f + vec3{0, epingle ? 75.f : 42.f, 0};
        fp.view = lookAt(oeil, cible, avant); fp.camPos = oeil;
        s.cible = cible; s.route = true; s.joueur = true; s.commandes = false;
        return true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || (g.sceneDediee && !g_vueDessusBanc) || !g.player || !g.track) return;
        const double sP = g.player->dist;
        for (const course5::Ordinateur& o : os_) {
            const double rel = o.g.rd - sP;
            if (rel < -60 / M || rel > 700 / M) continue;
            auto it = models_.find(o.car);
            if (it == models_.end() || !it->second.ok) continue;
            const TrackPoint p = g.track->pointLisse(o.g.rdRendu(g.alpha), o.g.xRendu(g.alpha) * RW);   // WFS-221 (accroche) : rendu interpolé
            const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th)) * rotateX((float)p.slope);
            peintures_[o.car] = lin(o.peinture);   // Renderer::draw garde l'adresse de la peinture jusqu'à la fin de l'image
            if (g_journalAdversaires) {   // WFS-243 (RG-01, RG-02) : banc : classement, dessin, largeur sur un téléphone 2340 × 1080
                const vec3 c{(float)p.x, (float)p.y, (float)p.z};
                const float prof = -(g.view.transformPoint(c)).z, focale = std::fabs(g.proj.at(1, 1)) * 1080.f * 0.5f;
                WFS_LOG("course à 5 : %s : classement %.1f m devant, dessiné à %.1f m de la caméra (%.1f m devant le joueur), %.1f px de large en 2340 × 1080",
                        o.nom.c_str(), rel * M, prof, (o.g.rdRendu(g.alpha) - sP) * M, carById(o.car.c_str()).d.W * focale / std::max(prof, 0.1f));
            }
            if (!g_bancAvant) r.adversaireProchaineVoiture();   // WFS-243 (ANO-117) : visible jusqu'à 700 m (avant : coupée à 150 m en Basse, 200 m en Haute)
            r.draw(it->second, m, &peintures_[o.car]);
        }
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || !ui::fontReady() || g.ecran != Ecran::Course || !g.player) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        const auto ls = lignes(*g.player);
        tableau(r, g, u, ls);
        if (fini_ && overT_ >= 0.9) resultat(r, W, H, u, ls);
    }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Course) return false;
        if (appui_.consomme(t.id, t.down)) return true;   // WFS-119 (ANO-53) : relâcher de « Rejouer », déjà dans la course suivante
        if (!actif_ || !fini_ || overT_ < 0.9) return false;
        if (!t.down) return true;
        if (rejouerZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("course à 5 : « Rejouer »"); rejouer_ = true; g.allerA(Ecran::Menu); }
        else if (menuZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("course à 5 : « Menu »"); g.allerA(Ecran::Menu); }
        return true;   // l'écran de fin garde tous les touchers
    }

    // WFS-125 : banc d'essai (parcours RG-09)
    int positions(double* rd, int max) const { int n = 0; if (actif_) for (const auto& o : os_) if (n < max) rd[n++] = o.g.rd; return n; }   // WFS-173
    const std::vector<course5::Ordinateur>* ordinateurs() const { return actif_ ? &os_ : nullptr; }   // WFS-272 (accroche) : son des adversaires
    bool bouton(const std::string& nom, float& x, float& y) const {
        const Zone& z = nom == "rejouer" ? rejouerZ_ : menuZ_;
        if (!actif_ || !fini_ || z.w <= 0) return false;
        x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
    }
    bool nouveauRecord() const { return nouveauRecord_; }
    // WFS-242 (RG-06) : écart le plus petit entre les boîtes de deux voitures (joueur et ordinateurs), le long de la route et en
    // travers (m ; négatif : l'une dans l'autre), comme le test CPU tests/test_course5.cpp
    double ecartMinimal(const Player& pl) const {
        struct B { double s, x, L, W; };
        std::vector<B> bs{{pl.dist * M, pl.playerX * RW, pl.car ? pl.car->d.L : 4.4, pl.car ? pl.car->d.W : 1.8}};
        for (const auto& o : os_) { const CarDef& c = carById(o.car.c_str()); bs.push_back({o.g.rd * M, o.g.x * RW, c.d.L, c.d.W}); }
        double e = 1e9;
        for (size_t a = 0; a < bs.size(); a++)
            for (size_t b = a + 1; b < bs.size(); b++)
                e = std::min(e, std::max(std::abs(bs[a].s - bs[b].s) - (bs[a].L + bs[b].L) / 2, std::abs(bs[a].x - bs[b].x) - (bs[a].W + bs[b].W) / 2));
        return e;
    }
    // WFS-242 (RG-07) : modèle de chaque ordinateur de la grille chargé (ou essayé : un modèle illisible ne bloque pas le départ)
    bool pretAuDepart() const {
        if (!actif_) return true;
        for (const auto& o : os_) if (!models_.count(o.car)) return false;
        return true;
    }
    Course5System() { actif5 = this; }
    ~Course5System() override { if (actif5 == this) actif5 = nullptr; }
    static Course5System* actif5;

private:
    // WFS-130 (RG-01 / RG-04) : détruit les modèles de models_ (et seulement eux : ni le joueur, ni le trafic, le duel ou
    // le garage) qui ne sont pas dans la grille courante os_, après waitIdle (aucune image en vol ne s'en sert encore).
    // RG-03 : une voiture retirée puis tirée de nouveau est rechargée par update (models_ ne la contient plus).
    void libererHorsGrille() {
        std::vector<std::string> aRetirer;
        for (const auto& [id, m] : models_)
            if (std::none_of(os_.begin(), os_.end(), [&](const course5::Ordinateur& o) { return o.car == id; })) aRetirer.push_back(id);
        if (aRetirer.empty() || !r_) return;
        r_->vk().waitIdle();
        for (const std::string& id : aRetirer) { r_->destroy(models_[id]); models_.erase(id); peintures_.erase(id); }
        WFS_LOG("course à 5 : %zu modèle(s) d'ordinateur libéré(s), %zu gardé(s)", aRetirer.size(), models_.size());
    }
    // WFS-125 : « 1re place · 1:23.45 » ; « — » tant que le record est vide (RG-03)
    static std::string texteRecord() {
        const Sauvegarde& sv = sauvegarde();
        if (!sv.aRecordCourse5()) return "—";
        return (sv.rec5Place == 1 ? std::string("1re") : std::to_string(sv.rec5Place) + "e") + " place · " + ui::chrono(sv.rec5Ms / 1000.0);
    }
    std::vector<course5::Ligne> lignes(const Player& pl) const {
        const Course* c = activeCourse();
        const double fin = c ? c->duelFinish : kmCourse5() * duel::KM;
        const std::string nom = sauvegarde().actif() ? sauvegarde().nick : "Pilote";
        return course5::classement(os_, nom, pl.dist, ftJoueur_, fini_, fin);
    }
    // RG-06 : « Position n/5 » et barres de distance (renderBoard du duel), à gauche, sous le bouton « Menu »
    void tableau(Renderer& r, const GameView& g, float u, const std::vector<course5::Ligne>& ls) {
        const Course* c = activeCourse();
        const double fin = c ? c->duelFinish : 1;
        const float x = g.marges.gauche + 2.5f * u, w = 34 * u, rh = 4.6f * u, px = 2.7f * u;
        float y = g.marges.haut + 33 * u;
        char pos[48]; std::snprintf(pos, sizeof pos, "POSITION %d/%d", course5::position(ls), (int)ls.size());
        r.uiRect(x, y, w, rh * (ls.size() + 1) + 1.2f * u, SURFACE);
        ui::text(r, x + 1.2f * u, y + 0.9f * u, px, pos, ui::CYAN, ui::GAUCHE, 0.12f, false, true);
        y += rh;
        for (const auto& l : ls) {
            const float k = (float)std::clamp(l.d / fin, 0.0, 1.0);
            const std::string km = l.fini ? "Arrivé" : ui::decimal(std::max(0.0, l.d) * M / 1000, 2) + " km";   // WFS-242 : grille derrière la ligne : 0,00
            ui::text(r, x + 1.2f * u, y + 0.6f * u, px, l.nom, l.moi ? ui::ORANGE : TEXTE, ui::GAUCHE, 0.f, false);
            ui::text(r, x + w - 1.2f * u, y + 0.6f * u, px, km, TEXTE, ui::DROITE, 0.f, false);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, w - 2.4f * u, 0.5f * u, PISTE);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, (w - 2.4f * u) * k, 0.5f * u, l.moi ? ui::ORANGE : ui::CYAN);
            y += rh;
        }
    }
    // RG-06 : écran de fin : place, temps, les 5 lignes (temps, écart ou distance), « Rejouer » et « Menu » ;
    // WFS-141 (RG-03, ANO-59) : boutons parallélogrammes et liseré orange de la fin Checkpoint (biseauFin), « 2e PLACE »
    // (jamais « 2E ») ; tailles des boutons et des textes inchangées (ordre permanent de l'utilisateur du 2026-11-06)
    void resultat(Renderer& r, float W, float H, float u, const std::vector<course5::Ligne>& ls) {
        const int rang = course5::position(ls);
        r.uiRect(0, 0, W, H, FOND);
        const bool rec = sauvegarde().actif();   // WFS-125 (RG-04) : badge et ligne « RECORD » sous le classement
        const float pw = 66 * u, ph = (34 + 6.5f * ls.size() + (rec ? 13 : 0)) * u, px0 = (W - pw) / 2, py0 = (H - ph) / 2;
        r.uiRect(px0, py0, pw, ph, SURFACE);
        r.uiRect(px0, py0, pw, 0.6f * u, ui::ORANGE);   // WFS-141 : liseré néon de la fin Checkpoint
        char sur[64]; std::snprintf(sur, sizeof sur, "COURSE À 5 · %d KM", kmCourse5());
        ui::text(r, W / 2, py0 + 3 * u, 2.6f * u, sur, ui::CYAN, ui::CENTRE, 0.16f, false);
        ui::text(r, W / 2, py0 + 7 * u, 9 * u, poursuite::titrePlace(rang), rang == 1 ? ui::ORANGE : TEXTE, ui::CENTRE, 0.04f, false, true);
        ui::text(r, W / 2, py0 + 17.5f * u, 4.2f * u, ui::chrono(ftJoueur_), TEXTE, ui::CENTRE, 0.f, false, true);
        float y = py0 + 24 * u;
        const double tete = ls[0].fini ? ls[0].ft : -1;
        for (size_t i = 0; i < ls.size(); i++) {
            const auto& l = ls[i];
            std::string tm;
            if (!l.fini) tm = ui::decimal(std::max(0.0, l.d) * M / 1000, 2) + " km";   // WFS-242 : derrière la ligne : 0,00
            else if (i == 0 || tete < 0) tm = ui::chrono(l.ft);
            else tm = "+" + ui::decimal(l.ft - tete, 1) + " s";
            const vec4 c = l.moi ? ui::ORANGE : TEXTE;
            ui::text(r, px0 + 5 * u, y, 3.4f * u, std::to_string(i + 1) + ". " + l.nom, c, ui::GAUCHE, 0.f, false);
            ui::text(r, px0 + pw - 5 * u, y, 3.4f * u, tm, l.fini ? c : MUTED, ui::DROITE, 0.f, false);
            y += 6.5f * u;
        }
        const float bw = 24 * u, bh = 7 * u, gap = 3 * u, by = py0 + ph - bh - 3 * u, bx = (W - (2 * bw + gap)) / 2;
        if (rec) dessinerRecord(r, W / 2, y - 1 * u, by - 2 * u, u, texteRecord(), nouveauRecord_);
        rejouerZ_ = {bx, by, bw, bh}; menuZ_ = {bx + bw + gap, by, bw, bh};
        biseauFin(r, rejouerZ_.x, by, bw, bh, 14 * H / 720, ui::ORANGE);
        ui::text(r, rejouerZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "REJOUER", vec4{0.04f, 0.04f, 0.06f, 1}, ui::CENTRE, 0.08f, false);
        biseauFin(r, menuZ_.x, by, bw, bh, 14 * H / 720, {30 / 255.f, 33 / 255.f, 48 / 255.f, 1});   // « Menu » de la fin Checkpoint (#1E2130)
        ui::text(r, menuZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "MENU", TEXTE, ui::CENTRE, 0.08f, false);
    }

    Renderer* r_ = nullptr;
    std::vector<course5::Ordinateur> os_;
    std::map<std::string, Model> models_;
    std::map<std::string, vec3> peintures_;
    bool actif_ = false, fini_ = false, depart_ = false, rejouer_ = false, graineFixe_ = false, nouveauRecord_ = false, aspJoueur_ = false;
    course5::SuiviDepassements depasses_;   // WFS-189 (RG-04) : dépassements du joueur
    uint32_t graine_ = 0;
    int placeJoueur_ = 0;   // WFS-242 (RG-02) : place du joueur sur la grille (0 = devant)
    double ecartMin_ = 1e9; bool ecartLogue_ = false;   // WFS-242 (RG-06) : distance minimale au départ (journal)
    double ftJoueur_ = 0, overT_ = 0, t_ = 0;
    Zone rejouerZ_, menuZ_;
    ui::FiltreAppui appui_;
};

Course5System* Course5System::actif5 = nullptr;

} // namespace

// WFS-125 : banc d'essai (parcours RG-09) : boutons de l'écran de fin, record battu
bool course5Bouton(const std::string& nom, float& x, float& y) { return Course5System::actif5 && Course5System::actif5->bouton(nom, x, y); }
int course5Positions(double* rd, int max) { return Course5System::actif5 ? Course5System::actif5->positions(rd, max) : 0; }   // WFS-173
const std::vector<course5::Ordinateur>* course5Ordinateurs() { return Course5System::actif5 ? Course5System::actif5->ordinateurs() : nullptr; }   // WFS-272 (accroche)
bool course5NouveauRecord() { return Course5System::actif5 && Course5System::actif5->nouveauRecord(); }
bool course5::course5PreteAuDepart() { return !Course5System::actif5 || Course5System::actif5->pretAuDepart(); }   // WFS-242 (RG-07)

WFS_SYSTEM(Course5System, 222)   // gameplay (200-299) : après la course (210) et le duel (220)

int benchParcoursCourse5(Game& g, Renderer& r, const std::string& out);   // WFS-133 : modes_ecran.cpp

// ---------- banc d'essai (WFS_BANC=course5) ----------
// WFS-327 (RG-04) : 1re épingle des lacets de la Corniche (R ≈ 22 m) : 4 ordinateurs lancés à 90 km/h, 35 à 80 m avant l'entrée ;
// suite de 6 vues de dessus toutes les 0,75 s, avant (table arcade seule et niveaux 0,90 / 0,97 : comportement de main, rejoué
// par g_avantWfs327) puis après ; journal : vitesse de chaque ordinateur, vitesse physiquement possible pour sa voiture, g latéral
template <class Images, class Capture>
static void benchEpingleCourse5(Game& g, Renderer& r, const std::string&, Images& images, Capture& capture) {
    for (const bool avant : {true, false}) {
        course5::g_avantWfs327 = avant;
        double facteur[3]; std::copy(std::begin(g_configJeu.course5.facteur), std::end(g_configJeu.course5.facteur), facteur);
        if (avant) { g_configJeu.course5.facteur[1] = 0.90; g_configJeu.course5.facteur[2] = 0.97; }
        g_graineBanc = 20261105u;
        if (g.init(r, "golf", "plage")) {
            static Track t;   // même tracé que la partie (épingles, vitesses du journal)
            if (!t.segmentCount()) t.build(mapById("plage"), trace::versionCarte("plage"));
            const std::vector<banc_virages::Epingle> ep = banc_virages::epingles(t);
            if (!ep.empty()) {
                const banc_virages::Epingle& e = ep[0];
                g.allerA(Ecran::Menu); images(30, 10);
                g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(false);
                for (int i = 0; i < 4 * 60; i++) g.update(1.0 / 60);   // fin du décompte
                g_bancJoueurM = e.debut * M - 120; g_bancAdversaires = {40, 55, 70, 85}; g_bancVitesseKmh = 90;
                g_bancEpingleM = (e.debut + e.fin) / 2 * M;
                g.update(1.0 / 60);   // placement
                const char* quand = avant ? "avant" : "apres";
                double pire = 0;
                for (int n = 1; n <= 6; n++) {
                    for (int i = 0; i < 45; i++) {
                        g.update(1.0 / 60);
                        for (const course5::Ordinateur& o : *course5Ordinateurs())
                            pire = std::max(pire, o.g.v / course5::vitesseVirageOrdinateur(t, o.g.rd, carById(o.car.c_str())) - 1);
                    }
                    g_vueDessusBanc = true; capture((std::string("course5-epingle-") + quand + "-" + std::to_string(n)).c_str()); g_vueDessusBanc = false;
                    for (const course5::Ordinateur& o : *course5Ordinateurs()) {
                        const double vp = course5::vitesseVirageOrdinateur(t, o.g.rd, carById(o.car.c_str())) * M, v = o.g.v * M;
                        WFS_LOG("capture course5-epingle-%s-%d : %s (%s) à %.0f m de l'entrée : %.0f km/h, possible %.0f km/h (%+.0f %%), %.2f g latéral",
                                quand, n, o.nom.c_str(), o.car.c_str(), (o.g.rd - e.debut) * M, v * 3.6, vp * 3.6, 100 * (v / vp - 1),
                                v * v * std::abs(banc_virages::courbure(t, o.g.rd)) / 9.81);
                    }
                }
                WFS_LOG("course à 5 : épingle de la Corniche (%s) : ordinateurs au plus %+.1f %% de la vitesse physiquement possible", quand, 100 * pire);
            }
            g_bancAdversaires.clear(); g_bancVitesseKmh = 0; g_bancEpingleM = 0;
            g.shutdown();
        }
        std::copy(std::begin(facteur), std::end(facteur), g_configJeu.course5.facteur);
    }
    course5::g_avantWfs327 = false;
}

// 1) panneau des modes ouvert au menu, 2) grille de départ pendant le décompte, 3) course (4 ordinateurs, aucun trafic),
// 4) écran de fin après l'arrivée du joueur (pilote automatique, 2 km)
void benchCourse5(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    g_graineBanc = 20261105u;
    modeCourse() = ModeCourse::Course5; kmCourse5() = 2;
    if (!g.init(r, "golf", "ville")) { g_graineBanc = 0; courseBenchRules(false); return; }
    double t = 700;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto capture = [&](const char* nom) { g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png"); };
    g.allerA(Ecran::Menu); images(30, 10);
    choixModeBanc(true); ouvrirChoixMode(); images(10, 5);
    capture("modes-choix");
    WFS_LOG("capture modes-choix : panneau %s, mode %s, %d km", choixModeOuvert() ? "ouvert" : "fermé", nomMode(modeCourse()), kmCourse5());
    choixModeBanc(false);
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    images(60, 8);
    capture("course5-grille");
    const Traffic* tr = activeTrafficMut();
    WFS_LOG("capture course5-grille : %zu voiture(s) de trafic", tr ? tr->cars.size() : (size_t)0);
    images(6 * 60, 8);
    capture("course5-course");
    WFS_LOG("capture course5-course : %.0f km/h, %.2f km", g.player().kmh(), g.player().dist * M / 1000);
    const Course* c = activeCourse();
    for (int i = 0; i < 150 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
    images(90, 10);
    capture("course5-fin");
    WFS_LOG("capture course5-fin : %s, %.2f s de course", c && c->state == RaceState::Over ? "arrivé" : "pas arrivé", c ? c->raceTime : 0.0);
    g.setAutopilot(false);
    g.shutdown();
    // WFS-242 (RG-06) : départ sur les 4 cartes, une graine par carte (places du joueur 3, 4, 5 et 1) : au « 3 », vue de derrière
    // (caméra Proche, commandes affichées : RG-07) et vue de dessus ; distance minimale entre voitures au journal (grille, 5 s)
    const std::pair<const char*, uint32_t> DEPARTS[] = {{"ville", 1234u}, {"campagne", 99u}, {"plage", 7u}, {"circuit", 1u}};
    for (const auto& [carte, graine] : DEPARTS) {
        g_graineBanc = graine;
        if (!g.init(r, "golf", carte)) continue;
        g.allerA(Ecran::Menu); images(30, 10);
        g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
        images(30, 6);
        const Course* k = activeCourse();
        const std::string base = std::string("course5-depart-") + carte;
        capture((base + "-arriere").c_str());
        WFS_LOG("capture %s-arriere : « %s », joueur à %.2f m de la ligne, voie %+.2f, %.0f km/h", base.c_str(), k ? k->countText().c_str() : "?",
                g.player().dist * M, g.player().playerX, g.player().kmh());
        g_vueDessusBanc = true; capture((base + "-dessus").c_str()); g_vueDessusBanc = false;
        for (int i = 0; i < 9 * 60; i++) g.update(1.0 / 60);   // fin du décompte, puis 5 s de course (distance minimale au journal)
        g.setAutopilot(false);
        g.shutdown();
    }
    // WFS-243 (RG-01, RG-02, RG-06, ANO-117) : ligne droite de la Campagne, joueur à 0,54 km comme sur la vidéo 3 de la VABF,
    // ordinateurs arrêtés à 190 m et 400 m devant (puis tous à 400 m et plus), en Haute puis en Basse ; journal par image
    for (const bool basse : {false, true}) {
        const bool avant = r.qualiteBasse;
        r.qualiteBasse = basse; g_graineBanc = 20261105u;
        if (!g.init(r, "golf", "campagne")) { r.qualiteBasse = avant; continue; }
        g.allerA(Ecran::Menu); images(30, 10);
        g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(false);
        for (int i = 0; i < 4 * 60; i++) g.update(1.0 / 60);   // fin du décompte
        const char* q = basse ? "basse" : "haute";
        // 190 m : à 0,54 km comme la vidéo (une bosse cache la route au-delà de ≈ 250 m) ; 400 m : à 1,06 km, ligne de vue
        // dégagée sur plus de 400 m (cherchée sur le tracé : axe à moins de 5 m de la ligne de vue, aucune bosse)
        struct Cas { const char* nom; double joueurM; std::vector<double> ecarts; };
        const Cas CAS[] = {{"190m", 540, {190, 400, 402, 404}}, {"400m", 1060, {400, 402, 404, 406}}};
        for (const auto& [nom, joueurM, ecarts] : CAS) {
            g_bancJoueurM = joueurM; g_bancAdversaires = ecarts; g_figerBanc = true;
            g.update(1.0 / 60);   // placement (collide)
            g_journalAdversaires = true;
            g_bancAvant = true; capture((std::string("course5-adversaires-") + nom + "-" + q + "-avant").c_str()); g_bancAvant = false;
            WFS_LOG("capture course5-adversaires-%s-%s-avant : %u appels de dessin (adversaires coupés par la distance)", nom, q, r.appelsDessin());
            capture((std::string("course5-adversaires-") + nom + "-" + q).c_str());
            g_journalAdversaires = false;
            WFS_LOG("capture course5-adversaires-%s-%s : %u appels de dessin", nom, q, r.appelsDessin());
            // coût (condition du PO) : 30 images sans puis avec les adversaires lointains : appels de dessin, triangles, temps
            for (const bool sans : {true, false}) {
                g_bancAvant = sans; double appels = 0, tri = 0, ms = 0; const int N = 30;
                for (int i = 0; i < N; i++) {
                    g.update(1.0 / 60); const double t0 = nowSeconds(); g.render(t += 1.0 / 60);
                    ms += (nowSeconds() - t0) * 1000; appels += r.appelsDessin(); tri += r.chronoImage().triangles;
                }
                WFS_LOG("course à 5 : coût %s %s, %s : %.0f appels de dessin, %.0f triangles, %.1f ms par image (moyenne de %d images, banc)",
                        nom, q, sans ? "sans les adversaires lointains (avant)" : "avec (après)", appels / N, tri / N, ms / N, N);
            }
            g_bancAvant = false;
        }
        g_figerBanc = false;
        g.shutdown();
        r.qualiteBasse = avant;
    }
    benchEpingleCourse5(g, r, out, images, capture);
    g_graineBanc = 0;
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    benchParcoursCourse5(g, r, out);   // WFS-133 (RG-10) : parcours complet avec la difficulté (modes_ecran.cpp)
}

} // namespace wfs
