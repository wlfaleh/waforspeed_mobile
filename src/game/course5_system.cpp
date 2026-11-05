// WaForSpeed — course à 5 voitures (WFS-110) branchée sur le moteur par les systèmes enfichables (WFS-30) : voir course5.h.
// Le départ, le décompte 3-2-1-GO et la ligne d'arrivée sont ceux de la course (course_system, Course::duelFinish comme
// le duel) ; ce module tire et fait rouler les 4 ordinateurs, applique les contacts (règle du duel), dessine les voitures,
// « Position n/5 » avec les barres de distance, et l'écran de fin (5 temps, « Rejouer », « Menu »).
#include <algorithm>
#include <cmath>
#include <cstdio>
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
#include "sauvegarde.h"
#include "traffic.h"
#include "traffic_system.h"

namespace wfs {

// RG-03 / RG-07 : ni trafic ni voitures de décor pendant une course à 5 ou un duel
bool sansTrafic(bool enCourse) { return enCourse && (modeCourse() == ModeCourse::Course5 || duelSession().inRoom()); }

namespace {

uint32_t g_graineBanc = 0;   // banc d'essai : tirage fixe (captures stables)

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
            if (!graineFixe_) { graine_ = g_graineBanc ? g_graineBanc : (uint32_t)(nowSeconds() * 1000.0) * 2654435761u + 17u; graineFixe_ = true; }
            c->duelFinish = kmCourse5() * duel::KM;
            os_ = course5::tirer(graine_, pl.car->id, course5::difficulte());   // WFS-133 (RG-03) : difficulté choisie
            course5::grille(os_, pl.dist);
            libererHorsGrille();   // WFS-130 (RG-01 / RG-02) : « Rejouer » retire les mêmes voitures, rien n'est détruit
            actif_ = true; fini_ = false; depart_ = false; ftJoueur_ = 0; overT_ = 0; t_ = 0; nouveauRecord_ = false;
            std::string noms; for (const auto& o : os_) noms += (noms.empty() ? "" : ", ") + o.car;
            WFS_LOG("course à 5 : graine %u, %d km, ordinateurs %s (difficulté %s)", graine_, kmCourse5(), noms.c_str(),
                    course5::cleDifficulte(course5::difficulte()));
        }
        if (!actif_ || c->state == RaceState::Count) return;   // décompte : tout le monde attend sur la grille
        if (!depart_ && c->state == RaceState::Play) {   // WFS-130 (RG-06) : ordinateurs prêts à être dessinés au départ
            depart_ = true;
            int prets = 0; for (const auto& o : os_) { auto it = models_.find(o.car); prets += it != models_.end() && it->second.ok; }
            WFS_LOG("course à 5 : départ, %d/%zu ordinateurs avec leur modèle, %zu modèle(s) d'ordinateur en mémoire", prets, os_.size(), models_.size());
        }
        t_ = c->state == RaceState::Play ? c->raceTime : t_ + dt;
        std::vector<int> nitros; for (const auto& o : os_) nitros.push_back(o.nitrosPrises);
        course5::avancer(os_, dt, c->duelFinish, t_, pl.dist, pl.playerX, pl.speed);
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
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || !g.player || !g.track) return;
        const double sP = g.player->dist;
        for (const course5::Ordinateur& o : os_) {
            const double rel = o.g.rd - sP;
            if (rel < -60 / M || rel > 700 / M) continue;
            auto it = models_.find(o.car);
            if (it == models_.end() || !it->second.ok) continue;
            const TrackPoint p = g.track->point(o.g.rd, o.g.x * RW);
            const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th)) * rotateX((float)p.slope);
            peintures_[o.car] = lin(o.peinture);   // Renderer::draw garde l'adresse de la peinture jusqu'à la fin de l'image
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
    bool bouton(const std::string& nom, float& x, float& y) const {
        const Zone& z = nom == "rejouer" ? rejouerZ_ : menuZ_;
        if (!actif_ || !fini_ || z.w <= 0) return false;
        x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
    }
    bool nouveauRecord() const { return nouveauRecord_; }
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
            const std::string km = l.fini ? "Arrivé" : ui::decimal(l.d * M / 1000, 2) + " km";
            ui::text(r, x + 1.2f * u, y + 0.6f * u, px, l.nom, l.moi ? ui::ORANGE : TEXTE, ui::GAUCHE, 0.f, false);
            ui::text(r, x + w - 1.2f * u, y + 0.6f * u, px, km, TEXTE, ui::DROITE, 0.f, false);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, w - 2.4f * u, 0.5f * u, PISTE);
            r.uiRect(x + 1.2f * u, y + rh - 0.9f * u, (w - 2.4f * u) * k, 0.5f * u, l.moi ? ui::ORANGE : ui::CYAN);
            y += rh;
        }
    }
    // RG-06 : écran de fin : place, temps, les 5 lignes (temps, écart ou distance), « Rejouer » et « Menu »
    void resultat(Renderer& r, float W, float H, float u, const std::vector<course5::Ligne>& ls) {
        const int rang = course5::position(ls);
        r.uiRect(0, 0, W, H, FOND);
        const bool rec = sauvegarde().actif();   // WFS-125 (RG-04) : badge et ligne « RECORD » sous le classement
        const float pw = 66 * u, ph = (34 + 6.5f * ls.size() + (rec ? 13 : 0)) * u, px0 = (W - pw) / 2, py0 = (H - ph) / 2;
        r.uiRect(px0, py0, pw, ph, SURFACE);
        char sur[64]; std::snprintf(sur, sizeof sur, "COURSE À 5 · %d KM", kmCourse5());
        ui::text(r, W / 2, py0 + 3 * u, 2.6f * u, sur, ui::CYAN, ui::CENTRE, 0.16f, false);
        const std::string titre = rang == 1 ? "Victoire" : std::to_string(rang) + "e place";
        ui::text(r, W / 2, py0 + 7 * u, 9 * u, ui::majuscules(titre), rang == 1 ? ui::ORANGE : TEXTE, ui::CENTRE, 0.04f, false, true);
        ui::text(r, W / 2, py0 + 17.5f * u, 4.2f * u, ui::chrono(ftJoueur_), TEXTE, ui::CENTRE, 0.f, false, true);
        float y = py0 + 24 * u;
        const double tete = ls[0].fini ? ls[0].ft : -1;
        for (size_t i = 0; i < ls.size(); i++) {
            const auto& l = ls[i];
            std::string tm;
            if (!l.fini) tm = ui::decimal(l.d * M / 1000, 2) + " km";
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
        r.uiRect(rejouerZ_.x, by, bw, bh, ui::ORANGE);
        ui::text(r, rejouerZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "REJOUER", vec4{0.04f, 0.04f, 0.06f, 1}, ui::CENTRE, 0.08f, false);
        r.uiRect(menuZ_.x, by, bw, bh, vec4{1, 1, 1, 0.10f});
        ui::text(r, menuZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "MENU", TEXTE, ui::CENTRE, 0.08f, false);
    }

    Renderer* r_ = nullptr;
    std::vector<course5::Ordinateur> os_;
    std::map<std::string, Model> models_;
    std::map<std::string, vec3> peintures_;
    bool actif_ = false, fini_ = false, depart_ = false, rejouer_ = false, graineFixe_ = false, nouveauRecord_ = false, aspJoueur_ = false;
    uint32_t graine_ = 0;
    double ftJoueur_ = 0, overT_ = 0, t_ = 0;
    Zone rejouerZ_, menuZ_;
    ui::FiltreAppui appui_;
};

Course5System* Course5System::actif5 = nullptr;

} // namespace

// WFS-125 : banc d'essai (parcours RG-09) : boutons de l'écran de fin, record battu
bool course5Bouton(const std::string& nom, float& x, float& y) { return Course5System::actif5 && Course5System::actif5->bouton(nom, x, y); }
bool course5NouveauRecord() { return Course5System::actif5 && Course5System::actif5->nouveauRecord(); }

WFS_SYSTEM(Course5System, 222)   // gameplay (200-299) : après la course (210) et le duel (220)

int benchParcoursCourse5(Game& g, Renderer& r, const std::string& out);   // WFS-133 : modes_ecran.cpp

// ---------- banc d'essai (WFS_BANC=course5) ----------
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
    g_graineBanc = 0;
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    benchParcoursCourse5(g, r, out);   // WFS-133 (RG-10) : parcours complet avec la difficulté (modes_ecran.cpp)
}

} // namespace wfs
