// WaForSpeed — module « course » (WFS-12) : voir course_system.h.
#include "course_system.h"
#include "../core/memoire.h"   // WFS-297 (RG-06)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <tuple>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/model_loader.h"   // WFS-125 : voiture du fantôme
#include "../render/renderer.h"
#include "../ui/text.h"
#include "course5.h"        // WFS-125 : mode de course (records par mode)
#include "duel_system.h"    // WFS-125 : duel (aucun record)
#include "fantome.h"        // WFS-125 : fantôme de ton record (Checkpoint)
#include "game.h"
#include "player.h"
#include "sauvegarde.h"   // WFS-24 : record
#include "traffic_system.h"
#include "../platform/manette.h"   // WFS-22
#include "../ui/ecrans/chargement.h"  // WFS-242 (RG-07) : fondu du chargement (commandes cachées)
#include "../ui/ecrans/terrain.h"     // WFS-125 : banc (« Rouler »)
#include "../ui/garage.h"             // WFS-125 : banc (« Choisir »)
#include "poursuite.h"                // WFS-125 : banc (poursuite)

namespace wfs {

// couleurs du skill design-street-racing (tokens.md), codes sRGB
static const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f};     // --bg #0A0B10 (code sRGB, comme ui::CYAN), voile de l'écran de fin
static bool g_rules = true;   // règles de course actives (voir courseBenchRules)
static const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1};    // --text #F2F4F8
static const vec4 SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};  // --surface #14161F

class CourseSystem : public System {
public:
    const char* name() const override { return "course"; }

    void init(Renderer& r, const GameView& g) override {
        course_.reset(); restartReq_ = false; active_ = this; r_ = &r;
        if (!g_rules) { course_.state = RaceState::Play; course_.clock = 0; course_.timeLeft = 1e9; course_.lastCount = 0; }
        demarrerFantome(g);   // WFS-125
    }
    void shutdown(Renderer& r) override {
        if (active_ == this) active_ = nullptr;
        if (fantModele_.ok) { r.vk().waitIdle(); r.destroy(fantModele_); }   // WFS-125
        fantModele_ = Model{}; fantModeleCar_.clear(); fantActif_ = enregistre_ = false;
    }
    // WFS-125 : modèle de la voiture du fantôme chargé hors de l'image (comme les adversaires du duel)
    void update(double, const GameView&) override {
        if (!r_ || !fantActif_ || fantModeleCar_ == lecture_.car) return;
        if (fantModele_.ok) { r_->vk().waitIdle(); r_->destroy(fantModele_); }
        fantModele_ = Model{};
        loadCarModel(*r_, std::string(carById(lecture_.car.c_str()).file), fantModele_);
        fantModeleCar_ = lecture_.car;
    }
    void onEcran(Ecran a, Ecran n, const GameView&) override {
        if ((a == Ecran::Menu || a == Ecran::Terrain) && n == Ecran::Course) restartReq_ = true;   // WFS-38 / WFS-74 : départ = « Rejouer »
        if (a == Ecran::Course && n != Ecran::Course) WFS_LOG("course : figée (écran %s)", nomEcran(n));   // WFS-74, RG-09
    }

    // après la conduite et les chocs du trafic (ordre 210 > 50) : fin de update() du jeu web
    void collide(double dt, Player& pl, const GameView& g) override {
        if (restartReq_) {   // « Rejouer » : même carte, même graine de trafic (RG-08)
            restartReq_ = false;
            pl.reset(*pl.car);
            trafficRestart(pl.position);
            course_.reset(); nouveauRecord_ = false; attente_ = 0;
            WFS_LOG("course : nouvelle course (même graine de trafic)");
            demarrerFantome(g);   // WFS-125
            return;
        }
        if (g.ecran != Ecran::Course) return;   // WFS-38 / WFS-74 : course figée hors de l'écran Course (menu, garage, terrain…)
        if (attendreDepart(dt)) return;           // WFS-242 (RG-07) : le « 3 » arrive avec les commandes
        const RaceState before = course_.state; const int cp = course_.level;
        course_.step(dt, pl, activeTrafficMut(), pl.position, dt / std::max(1e-6, course_.echelleDt));   // WFS-70 (RG-05) : chrono en temps réel
        if (before == RaceState::Count && course_.state == RaceState::Play) {   // WFS-297 (RG-06, accroche) : départ d'une course = partie suivante
            const int n = memoire::nouvellePartie();   // hors de WFS_LOG (la macro peut évaluer ses arguments deux fois)
            WFS_LOG("course : départ, partie %d depuis le lancement", n);
        }
        // WFS-125 (RG-05 / RG-06) : horloge de jeu de la course (le ralenti ralentit aussi le fantôme), trace toutes les 0,1 s
        if (course_.state == RaceState::Play) {
            tJeu_ += dt;
            if (enregistre_ && !enr_.pas(tJeu_, {pl.dist, pl.playerX, pl.heading + pl.spinYaw, pl.speed}) && !pleinVu_) {
                pleinVu_ = true; WFS_LOG("fantôme : %zu échantillons : trace pleine, enregistrement arrêté", enr_.trace.e.size());
            }
        } else if (course_.state == RaceState::Over) tJeu_ += dt;   // le fantôme finit sa trace
        if (course_.level != cp)
            WFS_LOG("course : checkpoint %d à %.1f s de course, +%.1f s, reste %.2f s, score %lld", course_.level, course_.raceTime,
                    Course::checkpointBonus(course_.level), course_.timeLeft, course_.score(pl));
        if (before != RaceState::Over && course_.state == RaceState::Over) finCourse(pl);   // WFS-24 : record (endRace)
        if (before != RaceState::Over && course_.state == RaceState::Over)
            WFS_LOG("course : temps écoulé — score %lld, %.2f km, %d km/h max, %d quasi-contacts, %d checkpoints", course_.score(pl),
                    pl.dist * 0.006 / 1000, course_.maxKmh, course_.nearCount, course_.level);
        // partie finie : la voiture roule seule à 62 % de la vitesse max (« auto » du jeu web)
        if (course_.state == RaceState::Over) pl.speed += (MAX_SPEED * 0.62 - pl.speed) * std::min(1.0, dt);
        if (course_.state != RaceState::Over) { score_ = course_.score(pl); km_ = pl.dist * 0.006 / 1000; }   // figés à la fin (endRace)
    }

    // WFS-125 (RG-06) : fantôme de ton record : vraie voiture du record rejouée sur sa trace, sans contact, visible de
    // 40 m derrière à 600 m devant ; dessin des adversaires du duel (duel_system.cpp render : point, rotateY, cap lissé 0,15)
    void render(Renderer& r, const GameView& g) override {
        if (!fantActif_ || g.sceneDediee || !g.player || !g.track || g.ecran != Ecran::Course || course_.state == RaceState::Count || !fantModele_.ok || !fantome::affiche()) return;   // WFS-318 (accroche) : réglage « Fantôme du record »
        fantome::Echantillon e;
        if (!fantome::interpoler(lecture_, tJeu_ - (1 - g.alpha) / 120.0, e)) return;   // trace finie : plus de fantôme ; WFS-221 (accroche) : temps interpolé
        const double rel = e.d - g.player->dist;
        if (rel < -40 / M || rel > 600 / M) return;
        fantYaw_ += (e.h - fantYaw_) * 0.15;
        const TrackPoint p = g.track->point(e.d, e.x * RW);
        const mat4 m = translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + fantYaw_)) * rotateX((float)p.slope);
        r.fantomeProchaineVoiture();   // WFS-134 (RG-07, ANO-60) : translucide, teinté cyan, sans ombre ni feux
        r.draw(fantModele_, m, &fantPeinture_);
    }

    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Course || course_.state != RaceState::Over || course_.overTimer < 0.9 || course_.duelFinish > 0) return false;
        // zones de toucher = rectangles dessinés par drawOver (une seule source, WFS-57)
        if (t.down && rejouer_.contient(t.x, t.y)) restartReq_ = true;
        if (t.down && menu_.contient(t.x, t.y)) g.allerA(Ecran::Menu);   // WFS-38
        return true;   // l'écran de fin garde tous les touchers
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.ecran != Ecran::Course || restartReq_) return;   // WFS-38 : ni décompte ni fin sur le menu
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        // compte à rebours : chiffre énorme au centre (showCount du jeu web)
        const std::string c = course_.countText();
        if (!c.empty()) {
            // .countdown span : chiffres --text #F2F4F8 avec halo cyan (0 0 30px rgba(0,229,255,.6)) ;
            // .countdown.go : « GO » --accent orange avec halo orange (--glow-accent). Halo = copies transparentes autour.
            const bool go = c == "GO";
            const float px = 34 * u, x = W * 0.5f, y = H * 0.30f;
            halo(r, x, y, px, c, go ? vec4{1.f, 0.416f, 0.f, 0.10f} : vec4{0.f, 0.898f, 1.f, 0.10f}, ui::CENTRE, 0.04f, px * 0.035f, true);
            ui::text(r, x, y, px, c, go ? ui::ORANGE : TEXTE, ui::CENTRE, 0.04f, false, true);   // WFS-48 : italique
        }
        // écran de fin (endRace / section #over du jeu web), affiché 0,9 s après la fin
        if (course_.state == RaceState::Over && course_.overTimer >= 0.9 && course_.duelFinish <= 0) drawOver(r, W, H, u);   // WFS-21 : résultat du duel à part
    }

    static CourseSystem* active_;
    Course course_;
    // WFS-242 (RG-07, ANO-123) : avant le premier « 3 », le compte à rebours attend la fin du fondu du chargement (les commandes
    // tactiles y sont cachées : elles apparaissaient d'un coup ≈ 1 s après le « 3 », pendant l'image figée du chargement des
    // adversaires) et les modèles des ordinateurs de la Course à 5 ; au plus ATTENTE_MAX_S, pour ne jamais bloquer le départ.
    // Le duel en ligne n'attend pas (horloge commune des téléphones). Vrai : pas de règle de course à ce pas
    static constexpr double ATTENTE_MAX_S = 2;
    double attente_ = 0;
    bool attendreDepart(double dt) {
        if (course_.state != RaceState::Count || course_.lastCount >= 0 || duelSession().inRoom()) return false;
        const bool fondu = ecrans::fonduEnCours(), adversaires = !course5::course5PreteAuDepart();
        if ((fondu || adversaires) && attente_ < ATTENTE_MAX_S) { attente_ += dt; return true; }
        if (attente_ > 0)
            WFS_LOG("course : compte à rebours lancé après %.2f s d'attente (%s)", attente_,
                    fondu || adversaires ? "attente maximale atteinte" : "fondu fini, adversaires prêts");
        attente_ = 0;
        return false;
    }
    // WFS-125 : banc d'essai (parcours RG-09)
    bool bouton(const std::string& nom, float& x, float& y) const {
        const Bouton& b = nom == "rejouer" ? rejouer_ : menu_;
        if (b.w <= 0) return false;
        x = b.x + b.w / 2; y = b.y + b.h / 2; return true;
    }
    bool nouveauRecord() const { return nouveauRecord_; }
    bool positionFantome(double& d, double& x, double& duree) const {   // WFS-318 (accroche) : étiquette « Votre record »
        fantome::Echantillon e;
        if (!fantActif_ || !fantModele_.ok || course_.state == RaceState::Count || !fantome::interpoler(lecture_, tJeu_, e)) return false;
        d = e.d; x = e.x; duree = lecture_.duree(); return true;
    }
    bool fantomeVu(const Player& pl, double& relM, double& latM) const {
        fantome::Echantillon e;
        if (!fantActif_ || !fantModele_.ok || course_.state == RaceState::Count || !fantome::interpoler(lecture_, tJeu_, e)) return false;
        relM = (e.d - pl.dist) * M; latM = (e.x - pl.playerX) * RW;
        return relM >= -40 && relM <= 600;
    }

private:
    // halo néon (text-shadow du web) : 12 copies transparentes du texte sur deux cercles autour de sa place
    static void halo(Renderer& r, float x, float y, float px, const std::string& t, const vec4& c, ui::Align a, float esp, float rad, bool it = false) {
        for (int k = 0; k < 12; k++) {
            const float ang = k * 0.5235988f, d = rad * (k % 2 ? 1.f : 0.55f);
            ui::text(r, x + std::cos(ang) * d, y + std::sin(ang) * d, px, t, c, a, esp, false, it);
        }
    }
    // parallélogramme incliné (clip-path du bouton .btn : 14 px de biais), haut décalé à droite, bas à gauche ;
    // WFS-101 (RG-05, ANO-20) : un seul quadrilatère (plus de bandes horizontales ni de rayures)
    static void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) { biseauFin(r, x, y, w, h, biais, c); }
    // endRace du jeu web (WFS-24, RG-03 / RG-09) : record écrit seulement s'il est battu (s > best), puis enregistrement
    // (thread de travail). Sauvegarde inactive (banc d'essai sans WFS_BANC=sauvegarde) : rien, captures inchangées.
    // WFS-125 (RG-01) : seule une course Checkpoint solo peut battre waforspeed-best : ni le duel, ni la course à 5, ni la
    // poursuite (avant : seul le duel était écarté, par duelFinish ; les autres modes ont leur propre record, RG-02)
    static bool checkpointSolo() { return modeCourse() == ModeCourse::Checkpoint && !duelSession().inRoom(); }
    void finCourse(const Player&) {
        nouveauRecord_ = false;
        Sauvegarde& s = sauvegarde();
        if (!s.actif() || course_.duelFinish > 0 || !checkpointSolo()) return;   // duel, course à 5, poursuite : pas ce record
        const long long sc = score_;   // score figé affiché sur l'écran de fin (dernière image de course)
        nouveauRecord_ = s.battreRecord(sc);
        WFS_LOG("course : score %lld, record %lld%s", sc, s.best, nouveauRecord_ ? " (NOUVEAU RECORD, enregistré)" : "");
        s.enregistrer();
        // WFS-125 (RG-05) : record du terrain battu (meilleur score fait sur ce terrain) → trace écrite (thread de travail)
        if (enregistre_ && !enr_.trace.e.empty() && (memo_.e.empty() || sc > memo_.score)) {
            enr_.trace.score = sc; memo_ = enr_.trace;
            s.enregistrerFichier(memoChemin_, fantome::encoder(memo_));
            WFS_LOG("fantôme : record du terrain %s battu (score %lld) : trace de %zu échantillons (%.1f s) écrite (%s)", memo_.terrain.c_str(), sc,
                    memo_.e.size(), memo_.duree(), memoChemin_.c_str());
        }
        enregistre_ = false;
    }
    // WFS-125 (RG-05 / RG-06) : au départ d'une course : enregistrement de la trace et lecture du fantôme du terrain,
    // seulement en Checkpoint solo avec la sauvegarde active (banc sans sauvegarde : aucun fantôme, captures inchangées)
    void demarrerFantome(const GameView& g) {
        fantActif_ = enregistre_ = pleinVu_ = false; tJeu_ = 0;
        Sauvegarde& s = sauvegarde();
        if (!s.actif() || !checkpointSolo() || !g.map || !g.car) return;
        const std::string terrain = g.map->id, chemin = s.cheminVoisin(fantome::nomFichier(terrain));
        const PeintureDef* pd = peintureDef(s.peinture(g.car->id));
        enr_.demarrer(terrain, g.car->id, pd ? pd->hex : g.car->paint);
        enregistre_ = true;
        if (chemin != memoChemin_) {   // fichier relu une fois par terrain ; ensuite la trace écrite est gardée en mémoire
            memoChemin_ = chemin; memo_ = fantome::Trace{};
            if (!fantome::lireFichier(chemin, memo_) || memo_.terrain != terrain) memo_ = fantome::Trace{};
        }
        lecture_ = memo_;
        fantActif_ = !lecture_.e.empty();
        if (!fantActif_) { WFS_LOG("fantôme : aucun fantôme sur le terrain %s", terrain.c_str()); return; }
        fantYaw_ = lecture_.e[0].h;
        auto lin = [](unsigned h, int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
        fantPeinture_ = {lin(lecture_.peinture, 16), lin(lecture_.peinture, 8), lin(lecture_.peinture, 0)};
        WFS_LOG("fantôme : terrain %s, record %lld, %s, %zu échantillons (%.1f s)", terrain.c_str(), lecture_.score, lecture_.car.c_str(),
                lecture_.e.size(), lecture_.duree());
    }
    void drawOver(Renderer& r, float W, float H, float u) {
        r.uiRect(0, 0, W, H, FOND);
        // WFS-101 (RG-04, ANO-36) : avec la sauvegarde (ligne « RECORD », badge si battu), mise en page aérée : statistiques
        // 16 px (720 p) sous le badge, ligne « RECORD » 12 px au-dessus des boutons, panneau agrandi de 28 px
        const bool rec = sauvegarde().actif();
        const float s = H / 720.f, plus = rec ? 28 * s : 0, dStats = rec ? (31.5f + 5.2f) * u + 16 * s - 38 * u : 0;
        const float pw = std::min(W * 0.86f, 120 * u), px = (W - pw) / 2, py = 10 * u, ph = 80 * u + plus;
        r.uiRect(px, py, pw, ph, SURFACE);
        r.uiRect(px, py, pw, 0.6f * u, ui::ORANGE);   // liseré néon : un seul accent par écran
        const float cx = W / 2;
        // .eyebrow : cyan --accent-2, espacement 0,22 em
        ui::text(r, cx, py + 6 * u, 4.2f * u, ui::majuscules("Temps écoulé"), ui::CYAN, ui::CENTRE, 0.22f, false);
        // .score-big : orange --accent avec halo (--glow-accent)
        halo(r, cx, py + 13 * u, 18 * u, ui::nombre(score_), {1.f, 0.416f, 0.f, 0.08f}, ui::CENTRE, 0.02f, 0.8f * u, true);
        ui::text(r, cx, py + 13 * u, 18 * u, ui::nombre(score_), ui::ORANGE, ui::CENTRE, 0.02f, false, true);
        char kmh[32]; std::snprintf(kmh, sizeof kmh, "%d km/h", course_.maxKmh);
        const std::string vals[4] = {ui::decimal(km_, 2) + " km", kmh, ui::nombre(course_.nearCount), ui::nombre(course_.level)};
        const char* labs[4] = {"Distance", "Vitesse max", "Quasi-contacts", "Checkpoints"};
        for (int i = 0; i < 4; i++) {
            const float x = px + pw * (0.125f + 0.25f * i);
            ui::text(r, x, py + 38 * u + dStats, 3.4f * u, ui::majuscules(labs[i]), ui::GRIS, ui::CENTRE, 0.16f);
            ui::text(r, x, py + 43 * u + dStats, 6.5f * u, vals[i], ui::BLANC, ui::CENTRE, 0.f, true, true);   // .stats dd (WFS-48)
        }
        // bouton « Rejouer » (biseau orange, texte sombre)
        const float bw = 40 * u, bh = 10 * u;
        // WFS-57 (ANO-14) : paire « Rejouer » + « Menu » (0,7 × la largeur, 2 u d'écart) centrée sur le panneau
        const float mw = bw * 0.7f, gap = 2 * u, bx = cx - (bw + gap + mw) / 2, by = py + 60 * u + plus;
        if (rec) dessinerRecord(r, cx, py + 31.5f * u, by - 12 * s, u, ui::nombre(sauvegarde().best), nouveauRecord_);   // WFS-24 ; WFS-125 : commun aux modes
        rejouer_ = {bx, by, bw, bh};
        menu_ = {bx + bw + gap, by, mw, bh}; manette::focus(rejouer_); manette::focus(menu_, true);   // WFS-22 : focus de la manette
        biseau(r, rejouer_.x, by, bw, bh, 14 * H / 720, ui::ORANGE);   // 14 px CSS (référence 720 p)
        ui::text(r, rejouer_.x + bw / 2, by + 2.6f * u, 5 * u, ui::majuscules("Rejouer"), {10 / 255.f, 11 / 255.f, 16 / 255.f, 1}, ui::CENTRE, 0.12f, false);
        biseau(r, menu_.x, by, mw, bh, 14 * H / 720, {30 / 255.f, 33 / 255.f, 48 / 255.f, 1});   // WFS-38 : « Menu » (.btn--ghost, #1E2130)
        ui::text(r, menu_.x + mw / 2, by + 2.6f * u, 5 * u, ui::majuscules("Menu"), TEXTE, ui::CENTRE, 0.12f, false);
    }
    bool nouveauRecord_ = false;
    bool restartReq_ = false;
    // WFS-125 : fantôme
    Renderer* r_ = nullptr;
    fantome::Enregistreur enr_;
    fantome::Trace memo_, lecture_;   // meilleure trace du terrain (mémoire, fichier) ; trace rejouée pendant cette course
    std::string memoChemin_, fantModeleCar_;
    Model fantModele_;
    vec3 fantPeinture_{};
    double tJeu_ = 0, fantYaw_ = 0;
    bool fantActif_ = false, enregistre_ = false, pleinVu_ = false;
    long long score_ = 0; double km_ = 0;
    struct Bouton {
        float x = 0, y = 0, w = 0, h = 0;
        bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
    };
    Bouton rejouer_, menu_;   // boutons de l'écran de fin, posés à chaque dessin (WFS-57)
};
CourseSystem* CourseSystem::active_ = nullptr;
bool fantomeCourant(double& d, double& x, double& duree) { return CourseSystem::active_ && CourseSystem::active_->positionFantome(d, x, duree); }   // WFS-318 (accroche)

// WFS-24 : badge « NOUVEAU RECORD » (.badge #newBest du jeu web, sous le score ; accent orange, texte sombre, biais
// skewX(-10deg)) et ligne « RECORD » avec le meilleur score (.record du menu web), entre les stats et les boutons ;
// bas : bas de la ligne « RECORD » (WFS-101 : 12 px au-dessus des boutons). WFS-125 : commun aux 3 modes (RG-04).
void dessinerRecord(Renderer& r, float cx, float yBadge, float bas, float u, const std::string& val, bool nouveau) {
    if (nouveau) {
        const std::string t = ui::majuscules("Nouveau record");
        const float px = 3.6f * u, bw = ui::width(px, t, 0.12f) + 4.f * u, bh = 5.2f * u, by = yBadge;
        const float x = cx - bw / 2, biais = bh * 0.176f;   // tan(10°) × hauteur ; biseau du bouton .btn (un quadrilatère)
        const vec4 c = ui::ORANGE;
        const UiVertex q[4] = {{x + biais, by, -1, -1, c.x, c.y, c.z, c.w}, {x + bw, by, -1, -1, c.x, c.y, c.z, c.w},
                               {x + bw - biais, by + bh, -1, -1, c.x, c.y, c.z, c.w}, {x, by + bh, -1, -1, c.x, c.y, c.z, c.w}};
        r.uiQuad(q);
        ui::text(r, cx, by + 0.9f * u, px, t, {10 / 255.f, 11 / 255.f, 16 / 255.f, 1}, ui::CENTRE, 0.12f, false, true);
    }
    const std::string lab = ui::majuscules("Record");
    const float lw = ui::width(3.4f * u, lab, 0.16f), vw = ui::width(5.5f * u, val), gap = 2.5f * u, x0 = cx - (lw + gap + vw) / 2;
    const float y = bas - 5.5f * u;   // haut de la valeur (5,5 u) ; libellé (3,4 u) 1,2 u plus bas, comme avant
    ui::text(r, x0, y + 1.2f * u, 3.4f * u, lab, ui::GRIS, ui::GAUCHE, 0.16f);
    ui::text(r, x0 + lw + gap, y, 5.5f * u, val, TEXTE, ui::GAUCHE, 0.f, true, true);
}
// WFS-141 : parallélogramme des boutons de l'écran de fin (avant : CourseSystem::biseau), commun aux 3 modes
void biseauFin(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    const UiVertex q[4] = {{x + biais, y, -1, -1, c.x, c.y, c.z, c.w}, {x + w, y, -1, -1, c.x, c.y, c.z, c.w},
                           {x + w - biais, y + h, -1, -1, c.x, c.y, c.z, c.w}, {x, y + h, -1, -1, c.x, c.y, c.z, c.w}};
    r.uiQuad(q);
}
WFS_SYSTEM(CourseSystem, 210)   // gameplay (200-299), après le trafic (50) dont il lit les chocs

const Course* activeCourse() { return CourseSystem::active_ ? &CourseSystem::active_->course_ : nullptr; }
Course* activeCourseMut() { return CourseSystem::active_ ? &CourseSystem::active_->course_ : nullptr; }
double courseScaleDt(double raw) { return CourseSystem::active_ && g_rules ? CourseSystem::active_->course_.scaleDt(raw) : raw; }
void courseBenchRules(bool on) { g_rules = on; }
bool courseBenchRulesOn() { return g_rules; }
void courseBenchSetTime(double s) { if (auto* c = activeCourseMut()) c->timeLeft = s; }

// banc d'essai : 1) course de référence (mêmes valeurs que le jeu web), 2) décompte « 3 » puis « GO »,
// 3) course en pilote automatique avec checkpoints, 4) écran de fin
void benchCourseScene(Game& g, Renderer& r, const std::string& out) {
    for (auto [m, c, sd] : {std::tuple{"ville", "golf", 12345u}, std::tuple{"circuit", "m4", 777u}})
        for (const auto& line : simulateCourse(m, c, sd)) WFS_LOG("course de référence : %s", line.c_str());
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("course : chargement impossible"); return; }
    const Course* k = activeCourse();
    if (!k) { WFS_ERR("course : module inactif"); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 300;
    bool shot3 = false, shotGo = false;
    for (int i = 0; i < 60 * 5 && !(shot3 && shotGo); i++) {
        g.update(1.0 / 60); t += 1.0 / 60;
        const std::string c = k->countText();
        if (!shot3 && c == "3") { g.render(t); g.render(t + 0.01); r.capturePng(out + "/course-decompte-3.png"); shot3 = true;
            WFS_LOG("capture course-decompte-3 : décompte « 3 », %.2f s avant le départ, %.0f km/h", -k->clock, g.player().kmh()); }
        if (!shotGo && c == "GO") {
            for (int f = 0; f < 6; f++) { g.update(1.0 / 60); t += 1.0 / 60; }
            g.render(t); g.render(t + 0.01); r.capturePng(out + "/course-decompte-go.png"); shotGo = true;
            WFS_LOG("capture course-decompte-go : « %s », %.2f s après le départ, %.0f km/h", k->countText().c_str(), k->clock, g.player().kmh());
        }
    }
    // 25 s de course automatique : checkpoints au journal (WFS_LOG du module)
    for (int i = 0; i < 60 * 25; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(t + i / 60.0); }
    WFS_LOG("course automatique : %.1f s, %d checkpoint(s), reste %.2f s, score %lld, %d quasi-contacts, %d accidents",
            k->raceTime, k->level, k->timeLeft, k->score(g.player()), k->nearCount, k->accidents);
    // fin de partie : temps forcé à 0,05 s, puis écran de fin 1,2 s après
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t + 30 + i / 60.0); }
    g.render(t + 32); g.render(t + 32.01);
    r.capturePng(out + "/course-fin.png");
    WFS_LOG("capture course-fin : état %s, %d checkpoint(s), %d km/h max (score affiché = score à la fin, ci-dessus)",
            k->state == RaceState::Over ? "fin" : "en course", k->level, k->maxKmh);
    g.shutdown();
}

// WFS-24 : banc de la sauvegarde, lancé deux fois de suite sur le même chemin (propre au banc).
// 1er lancement (fichier absent) : voiture m4, peinture rouge, course solo jusqu'à la fin → record battu, capture fin-record.
// 2e lancement (fichier présent) : relecture au journal, vitrine de la voiture relue (capture sauvegarde-relue), puis le
// fichier est effacé (remise à zéro : un lancement suivant repart du premier).
void benchSauvegardeScene(Game& g, Renderer& r, const std::string& out, const std::string& chemin) {
    Sauvegarde& s = sauvegarde();
    std::FILE* f = std::fopen(chemin.c_str(), "rb");
    const bool second = f != nullptr;
    if (f) std::fclose(f);
    s.charger(chemin);
    if (second) {
        WFS_LOG("SAUVEGARDE relue : car=%s paint=%s best=%lld", s.car.c_str(), s.peinture(s.car).c_str(), s.best);
        if (!g.init(r, s.car.c_str(), s.map.c_str())) { WFS_ERR("sauvegarde : chargement impossible"); return; }
        g.camMode = 2; g.orbit = 0.7f;
        g.render(0); g.render(0); r.capturePng(out + "/sauvegarde-relue.png");
        WFS_LOG("capture sauvegarde-relue : vitrine de la voiture relue (%s, peinture %s)", s.car.c_str(), s.peinture(s.car).c_str());
        g.shutdown();
        std::remove(chemin.c_str());
        WFS_LOG("sauvegarde du banc effacée (%s) : le prochain lancement repart du premier", chemin.c_str());
        return;
    }
    s.car = "m4"; s.paint["m4"] = "rouge";   // choix du « joueur » (≠ défauts golf / peinture d'origine)
    courseBenchRules(true);
    if (!g.init(r, s.car.c_str(), s.map.c_str())) { WFS_ERR("sauvegarde : chargement impossible"); courseBenchRules(false); return; }
    const Course* k = activeCourse();
    g.setAutopilot(true); g.camMode = 0;
    double t = 600;
    for (int i = 0; i < 60 * 14; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 0) g.render(t); }   // décompte + 10 s de course
    courseBenchSetTime(0.05);
    for (int i = 0; i < 80; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
    g.render(t); g.render(t + 0.01);
    r.capturePng(out + "/fin-record.png");
    WFS_LOG("capture fin-record : état %s, record %lld", k && k->state == RaceState::Over ? "fin" : "en course", s.best);
    s.enregistrer(); s.attendre();
    WFS_LOG("SAUVEGARDE écrite : car=%s paint=%s best=%lld (%s)", s.car.c_str(), s.peinture(s.car).c_str(), s.best, chemin.c_str());
    g.shutdown();
    courseBenchRules(false);
}

// WFS-125 (RG-09) : parcours complet au doigt, sauvegarde active (fichier propre au banc, effacé à la fin) :
// accueil → Course solo → Checkpoint → Continuer → garage → Choisir → terrain → Rouler → course (record battu, trace
// écrite) → fin « NOUVEAU RECORD » → Rejouer → course avec le fantôme (capture records-fantome) → fin → Menu → accueil ;
// puis Poursuite jusqu'à la victoire (capture records-poursuite) et Course à 5 sur 2 km (capture records-course5) →
// Menu → accueil. Le record Checkpoint ne bouge pas après la poursuite et la course à 5. Rend le nombre d'écarts.
int benchRecords(Game& g, Renderer& r, const std::string& out) {
    Sauvegarde& s = sauvegarde();
    const std::string chemin = out + "/records-banc.txt";
    std::remove(chemin.c_str()); std::remove((out + "/" + fantome::nomFichier("ville")).c_str());
    s.charger(chemin);
    int ecarts = 0;
    auto verifier = [&](bool ok, const char* quoi) { if (ok) WFS_LOG("records (parcours) : %s (ok)", quoi); else { ecarts++; WFS_ERR("records (parcours) : %s (ÉCHEC)", quoi); } };
    {   // RG-03 : ancienne sauvegarde (sans les nouvelles clés) et clés mal formées
        Sauvegarde anc;
        const bool lu = anc.lire("version=1\nwaforspeed-best=48210\ncar=m4\nrecord-course5=x;12\nrecord-poursuite=1;2\n");
        // WFS-169 (ANO-88) : depuis WFS-136 (RG-07 / A7.4), le record du tracé 1 est mis de côté sous best_v1 (best repart de 0)
        const auto v1 = anc.anciens.find("best_v1");
        const std::string bestV1 = v1 != anc.anciens.end() ? v1->second : "—";
        WFS_LOG("records : ancienne sauvegarde relue (%s) : best=%lld, best_v1=%s, course à 5 %s, poursuite %s", lu ? "valide" : "invalide", anc.best,
                bestV1.c_str(), anc.aRecordCourse5() ? "rempli" : "—", anc.aRecordPoursuite() ? "rempli" : "—");
        verifier(lu && anc.best == 0 && bestV1 == "48210" && anc.car == "m4" && !anc.aRecordCourse5() && !anc.aRecordPoursuite(),
                 "ancienne sauvegarde relue sans perte (record mis de côté sous best_v1), records vides");
    }
    courseBenchRules(true);
    if (!g.init(r, "golf", "ville")) { WFS_ERR("records : chargement impossible"); s.desactiver(); courseBenchRules(false); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    double t = 2000;
    int n = 0, doigt = 80;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 4 == 0) g.render(t); };
    auto images = [&](int k) { for (int i = 0; i < k; i++) image(); };
    auto rouler = [&](int k) { for (int i = 0; i < k; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (++n % 30 == 0) g.render(t); } };   // course : peu d'images dessinées
    auto capture = [&](const char* nom) { g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png"); };
    auto appuyer = [&](float x, float y) {   // doigt Android : down, 6 mouvements, up (une image dessinée entre chaque)
        const int id = doigt++;
        g.render(t);
        auto dessinee = [&]() { g.update(1.0 / 60); g.render(t += 1.0 / 60); };
        g.onTouch({id, x, y, true}); dessinee();
        for (int k = 1; k <= 6; k++) { g.onTouch({id, x + 0.4f * k, y, true}); dessinee(); }
        g.onTouch({id, x + 3, y, false}); dessinee();
        images(3);
    };
    // écran de fin : appui court au centre du bouton (trouver : place du bouton, posée par la dernière image dessinée)
    auto toucher = [&](auto trouver, const char* nom) {
        float x = 0, y = 0;
        g.render(t += 0.02);
        if (!trouver(x, y)) { ecarts++; WFS_ERR("records (parcours) : bouton « %s » absent", nom); return; }
        const int id = doigt++;
        g.render(t); g.onTouch({id, x, y, true}); images(2); g.onTouch({id, x, y, false}); images(4);
        WFS_LOG("records (parcours) : toucher « %s » (%.0f, %.0f) → écran %s", nom, x, y, nomEcran(g.ecran()));
    };
    auto attendu = [&](Ecran e, const char* etape) {
        if (g.ecran() == e) WFS_LOG("records (parcours) : %s → écran %s (attendu)", etape, nomEcran(e));
        else { ecarts++; WFS_ERR("records (parcours) : %s → écran %s, attendu %s", etape, nomEcran(g.ecran()), nomEcran(e)); }
    };
    auto panneau = [&](const char* zone) {
        float x, y;
        if (!choixModeCentre(zone, x, y)) { ecarts++; WFS_ERR("records (parcours) : zone %s du panneau absente", zone); return; }
        appuyer(x, y);
    };
    // accueil → « Course solo » (vrai toucher : 1re zone du menu, déclarée pour la manette) → mode → garage → terrain → course
    auto jusquaCourse = [&](const char* carte, const char* km, ModeCourse m) {
        g.allerA(Ecran::Menu); images(40);
        const auto& z = manette::zonesPrecedentes();
        if (z.empty()) { ecarts++; WFS_ERR("records (parcours) : menu sans bouton"); return; }
        appuyer(z[0].x + z[0].w * 0.4f, z[0].y + z[0].h * 0.5f);
        verifier(choixModeOuvert(), "« Course solo » : panneau des modes ouvert");
        panneau(carte); if (km) panneau(km);
        panneau("continuer");
        attendu(Ecran::Garage, "mode puis « Continuer »");
        float x = 0, y = 0;
        for (int i = 0; i < 600 && !ecrans::garageBouton("choisir", x, y); i++) image();
        images(30);
        if (ecrans::garageBouton("choisir", x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("records (parcours) : « Choisir » absent"); }
        attendu(Ecran::Terrain, "garage puis « Choisir »");
        images(20);
        if (ecrans::centreRouler(x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("records (parcours) : « Rouler » absent"); }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) image();
        attendu(Ecran::Course, "terrain puis « Rouler »");
        if (modeCourse() != m) { ecarts++; WFS_ERR("records (parcours) : mode %s, attendu %s", nomMode(modeCourse()), nomMode(m)); }
    };
    auto finForcee = [&]() {   // Checkpoint : temps forcé à 0,05 s, écran de fin 0,9 s après
        courseBenchSetTime(0.05);
        rouler(70); images(20);
        const Course* c = activeCourse();
        return c && c->state == RaceState::Over;
    };
    CourseSystem* cs = nullptr;
    // 1) Checkpoint : record battu (sauvegarde neuve), trace écrite
    jusquaCourse("carte0", nullptr, ModeCourse::Checkpoint);
    g.setAutopilot(true);
    rouler(3 * 60 + 10 * 60);   // décompte puis 10 s de course
    verifier(finForcee(), "course Checkpoint finie");
    cs = CourseSystem::active_;
    verifier(cs && cs->nouveauRecord(), "fin de la 1re course : « NOUVEAU RECORD »");
    capture("records-checkpoint");
    s.attendre();
    fantome::Trace tr;
    const bool ecrite = fantome::lireFichier(s.cheminVoisin(fantome::nomFichier("ville")), tr);
    WFS_LOG("records : record Checkpoint %lld ; fichier du fantôme %s (%zu échantillons, score %lld)", s.best, ecrite ? "relu" : "absent", tr.e.size(), tr.score);
    verifier(ecrite && !tr.e.empty() && tr.score == s.best && tr.terrain == "ville", "trace du record écrite (fantome-ville.bin)");
    // 2) « Rejouer » : course avec le fantôme ; le joueur part 0,5 s après le fantôme (sinon les deux voitures se confondent)
    toucher([](float& x, float& y) { return CourseSystem::active_ && CourseSystem::active_->bouton("rejouer", x, y); }, "Rejouer");
    g.setAutopilot(false);
    rouler(3 * 60 + 30);
    g.setAutopilot(true);
    rouler(3 * 60); images(12);
    double rel = 0, lat = 0;
    cs = CourseSystem::active_;
    const bool vu = cs && cs->fantomeVu(g.player(), rel, lat);
    capture("records-fantome");
    WFS_LOG("capture records-fantome : fantôme %s, %.1f m devant, %.1f m de côté ; joueur %.0f km/h", vu ? "visible" : "absent", rel, lat, g.player().kmh());
    verifier(vu && rel > 3, "fantôme du record visible devant le joueur");
    const long long best0 = s.best;
    verifier(finForcee(), "2e course Checkpoint finie");
    cs = CourseSystem::active_;
    WFS_LOG("records : 2e course : record Checkpoint %lld%s", s.best, cs && cs->nouveauRecord() ? " (battu)" : " (non battu)");
    toucher([](float& x, float& y) { return CourseSystem::active_ && CourseSystem::active_->bouton("menu", x, y); }, "Menu");
    images(10);
    attendu(Ecran::Menu, "fin puis « Menu »");
    const long long bestCp = s.best;
    // 3) Poursuite jusqu'à la victoire (saut près de l'arrivée) : ligne « RECORD » Poursuite, badge au premier passage
    poursuiteBancFixe(true);
    jusquaCourse("carte2", nullptr, ModeCourse::Poursuite);
    rouler(3 * 60 + 60);
    poursuiteBancFin(2);
    for (int i = 0; i < 60 * 60; i++) { const Course* c = activeCourse(); if (!c || c->state == RaceState::Over) break; g.update(1.0 / 60); }
    images(90);
    capture("records-poursuite");
    WFS_LOG("capture records-poursuite : record %s%s", s.aRecordPoursuite() ? (s.recPVictoire ? "victoire" : "arrêté") : "—", poursuiteNouveauRecord() ? ", NOUVEAU RECORD" : "");
    verifier(s.aRecordPoursuite() && poursuiteNouveauRecord(), "fin de la poursuite : record Poursuite, « NOUVEAU RECORD »");
    toucher([](float& x, float& y) { return poursuiteBouton("menu", x, y); }, "Menu");
    images(10);
    attendu(Ecran::Menu, "fin de la poursuite puis « Menu »");
    poursuiteBancFixe(false); poursuiteBancFin(0);
    // 4) Course à 5 sur 2 km : ligne « RECORD » Course à 5, badge au premier passage
    jusquaCourse("carte1", "km0", ModeCourse::Course5);
    for (int i = 0; i < 150 * 60; i++) { const Course* c = activeCourse(); if (!c || c->state == RaceState::Over) break; g.update(1.0 / 60); }
    images(90);
    capture("records-course5");
    WFS_LOG("capture records-course5 : record %d place(s), %lld ms%s", s.rec5Place, s.rec5Ms, course5NouveauRecord() ? ", NOUVEAU RECORD" : "");
    verifier(s.aRecordCourse5() && course5NouveauRecord(), "fin de la course à 5 : record Course à 5, « NOUVEAU RECORD »");
    toucher([](float& x, float& y) { return course5Bouton("menu", x, y); }, "Menu");
    images(10);
    attendu(Ecran::Menu, "fin de la course à 5 puis « Menu »");
    // RG-01 : record Checkpoint inchangé ; relecture des 3 records (redémarrage simulé)
    WFS_LOG("records : record Checkpoint %lld après les courses Checkpoint, %lld après la poursuite et la course à 5", bestCp, s.best);
    verifier(s.best == bestCp && bestCp >= best0, "record Checkpoint (waforspeed-best) inchangé après la poursuite et la course à 5");
    s.enregistrer(); s.attendre();
    Sauvegarde relue;
    {
        std::FILE* f = std::fopen(chemin.c_str(), "rb"); std::string c; char buf[4096]; size_t k;
        if (f) { while ((k = std::fread(buf, 1, sizeof buf, f)) > 0) c.append(buf, k); std::fclose(f); }
        relue.lire(c);
    }
    WFS_LOG("records : sauvegarde relue : best=%lld, course à 5 %d;%lld, poursuite %d;%d;%lld", relue.best, relue.rec5Place, relue.rec5Ms,
            relue.recPVictoire ? 1 : 0, relue.recPScore, relue.recPDistM);
    verifier(relue.best == s.best && relue.rec5Place == s.rec5Place && relue.rec5Ms == s.rec5Ms && relue.recPVictoire == s.recPVictoire &&
             relue.recPScore == s.recPScore && relue.recPDistM == s.recPDistM, "records relus après redémarrage");
    WFS_LOG("records (parcours) : accueil → Course solo → Checkpoint → Continuer → garage → Choisir → terrain → Rouler → course → fin « NOUVEAU RECORD » → "
            "Rejouer → course avec le fantôme → fin → Menu → accueil ; Poursuite → fin → Menu ; Course à 5 → fin → Menu → accueil : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    g.setAutopilot(false);
    std::remove(chemin.c_str()); std::remove(s.cheminVoisin(fantome::nomFichier("ville")).c_str());
    s.desactiver();
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    return ecarts;
}

} // namespace wfs
