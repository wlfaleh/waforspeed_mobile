// WaForSpeed — mode « Poursuite police » (WFS-111) branché sur le moteur par les systèmes enfichables (WFS-30) :
// voir poursuite.h. Le départ, le décompte 3-2-1-GO et la ligne d'arrivée (10 km) sont ceux de la course
// (course_system, Course::duelFinish, comme la course à 5) ; ce module tient le score (100 → 0), fait rouler la police
// (gyrophares, sirène, renforts), densifie le trafic (accroche de traffic_system.cpp), place les embouteillages et les
// surprises, dessine le HUD de la poursuite et l'écran de fin (victoire ou « ARRÊTÉ », « Rejouer », « Menu »).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../audio/sfx.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../render/effets_particules.h"   // WFS-141 : effets::versHdr (gyrophares)
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "../ui/appui.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/ecrans/terrain.h"   // RG-12 : « Rouler »
#include "../ui/garage.h"           // RG-12 : « Choisir »   // banc : appels de dessin en qualité Basse
#include "../ui/text.h"
#include "course5.h"
#include "course_system.h"
#include "duel.h"
#include "duel_system.h"
#include "game.h"
#include "player.h"
#include "poursuite.h"
#include "sauvegarde.h"   // WFS-119 : redémarrage simulé
#include "traffic.h"
#include "traffic_system.h"

namespace wfs {

int poursuiteTrafic(int trafficN, bool enCourse) {
    return enCourse && modeCourse() == ModeCourse::Poursuite ? (int)std::lround(trafficN * poursuite::DENSITE) : 0;
}

namespace {

using namespace poursuite;

bool g_banc = false;                 // banc d'essai : programme fixe (captures stables)
int g_bancFin = 0;                   // banc : 1 = arrêter tout de suite, 2 = sauter près de l'arrivée
double g_bancSautKm = 0;             // WFS-141 : banc : saut en avant du joueur jusqu'à ce point (km)
// voiture existante repeinte (RG-03) : la 208 (la Golf si le joueur roule en 208)
const char* policeCar(const Player* pl) { return pl && pl->car && !std::strcmp(pl->car->id, "p208") ? "golf" : "p208"; }

const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f}, SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};
const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1}, MUTED{138 / 255.f, 144 / 255.f, 166 / 255.f, 1};
const vec4 DANGER{1.f, 31 / 255.f, 75 / 255.f, 1}, BLEU{0.25f, 0.45f, 1.f, 1};
vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };
struct Plot { double s = 0, x = 0; bool tombe = false; };
struct Camion { double s = 0; Travers tv; int fileLibre = 2; double cd = 0; };   // WFS-119 : camion en travers
struct Bande { double s = 0; bool passe = false; };                              // WFS-119 : ralentisseur

class PoursuiteSystem : public System {
public:
    static PoursuiteSystem* actif;
    const char* name() const override { return "poursuite"; }
    void init(Renderer& r, const GameView&) override {
        r_ = &r; actif_ = false; police_.clear(); plots_.clear(); actif = this;
        std::vector<Vertex> v; std::vector<uint32_t> idx;   // cube unité : x, z ∈ [−0,5 ; 0,5], y ∈ [0 ; 1]
        const vec3 nrm[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (const vec3& f : nrm) {
            const vec3 a = std::abs(f.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, b = cross(f, a);
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i < 4; i++) {
                const float sa = (i == 1 || i == 2) ? 0.5f : -0.5f, sb = i >= 2 ? 0.5f : -0.5f;
                const vec3 q = f * 0.5f + a * sa + b * sb + vec3{0, 0.5f, 0};
                v.push_back({q.x, q.y, q.z, f.x, f.y, f.z, sa + 0.5f, sb + 0.5f});
            }
            idx.insert(idx.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        cube_ = r.makeMesh(v, idx);
        auto mat = [&](unsigned c, vec3 em, float rough) { Material m; const vec3 l = lin(c); m.color = {l.x, l.y, l.z, 1}; m.rough = rough; m.emissive = em; return r.makeMaterial(m, nullptr, nullptr, nullptr); };
        // WFS-141 (RG-01, ANO-56) : lampes allumées à leur couleur (versHdr, comme les clignotants de WFS-109), sans le × 6
        auto web = [](unsigned h) { return effets::versHdr(vec3{((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}, 1.f); };
        matRouge_ = mat(0x200000, web(GYRO_ROUGE), 0.4f);
        matBleu_ = mat(0x000020, web(GYRO_BLEU), 0.4f);
        haloRouge_ = web(GYRO_ROUGE); haloBleu_ = web(GYRO_BLEU);
        matPlot_ = mat(0xff6a00, lin(0xff6a00) * 0.4f, 0.6f);
        matBlanc_ = mat(0xf2f4f8, {0, 0, 0}, 0.5f);
        matBleuCaisse_ = mat(0x14224a, {0, 0, 0}, 0.45f);
        matVitre_ = mat(0x15181e, {0, 0, 0}, 0.15f);
        matJaune_ = mat(0xffc400, lin(0xffc400) * 0.7f, 0.6f);   // WFS-119 : ralentisseur jaune et noir
        matNoir_ = mat(0x101010, {0, 0, 0}, 0.7f);
    }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        if (modele_.ok) r.destroy(modele_);
        modele_ = Model{};
        if (camionModele_.ok) r.destroy(camionModele_);
        camionModele_ = Model{};
        r.destroy(cube_);
        police_.clear(); plots_.clear(); actif_ = false;
        if (actif == this) actif = nullptr;
    }
    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        // « Rejouer » : Course → Menu → Course (le menu n'est jamais dessiné), comme la course à 5
        if (ancien == Ecran::Course && nouveau == Ecran::Menu && rejouer_) g.allerA(Ecran::Course);
        if (nouveau == Ecran::Menu && !rejouer_) actif_ = false;
        if (nouveau == Ecran::Course) rejouer_ = false;
    }
    void update(double, const GameView& g) override {
        if (actif_ && policeCar_ != policeCar(g.player) && modele_.ok) { r_->vk().waitIdle(); r_->destroy(modele_); modele_ = Model{}; modeleEssaye_ = false; }
        if (actif_) policeCar_ = policeCar(g.player);
        if (r_ && actif_ && !modele_.ok && !modeleEssaye_) { modeleEssaye_ = true; loadCarModel(*r_, std::string(carById(policeCar_).file), modele_); WFS_LOG("poursuite : police %s, %zu pièces", policeCar_, modele_.parts.size()); }
        // WFS-119 : camion en travers (x5, rrsport si le joueur roule en x5)
        const char* cam = camionCar(g.player && g.player->car ? g.player->car->id : nullptr);
        if (actif_ && cam != camionCar_ && camionModele_.ok) { r_->vk().waitIdle(); r_->destroy(camionModele_); camionModele_ = Model{}; camionEssaye_ = false; }
        if (actif_) camionCar_ = cam;
        if (r_ && actif_ && !camionModele_.ok && !camionEssaye_) { camionEssaye_ = true; loadCarModel(*r_, std::string(carById(camionCar_).file), camionModele_); WFS_LOG("poursuite : camion %s, %zu pièces", camionCar_, camionModele_.parts.size()); }
    }

    // après la course (210), le duel (220) et la course à 5 (222)
    void collide(double dt, Player& pl, const GameView& g) override {
        Course* c = activeCourseMut();
        if (modeCourse() != ModeCourse::Poursuite || g.ecran != Ecran::Course || !c || duelSession().inRoom()) { actif_ = false; return; }
        if (c->duelFinish <= 0) demarrer(*c, pl);   // course remise à zéro (départ du menu, « Rejouer »)
        if (!actif_ || c->state == RaceState::Count) return;   // décompte : la police attend derrière
        Track& t = *g.track;
        if (g_bancFin == 2) {   // banc : saut à 150 m de l'arrivée (capture de la victoire)
            const double d = c->duelFinish - 150 / M - pl.dist;
            if (d > 0) { pl.dist += d; pl.position = t.wrap(pl.position + d); for (Police& p : police_) p.rd += d; }
            g_bancFin = 0; distAvant_ = pl.dist;
        }
        if (g_bancSautKm > 0) {   // WFS-141 : banc : saut après le barrage et l'embouteillage (le pilote automatique y reste bloqué)
            const double d = g_bancSautKm * duel::KM - pl.dist;
            if (d > 0) { pl.dist += d; pl.position = t.wrap(pl.position + d); }   // la police reste derrière et rattrape
            g_bancSautKm = 0; distAvant_ = pl.dist;
        }
        Traffic* tr = activeTrafficMut();
        if (c->state == RaceState::Play) {
            // RG-02 : chocs avec le trafic (compteurs du module trafic), quasi-contacts (+1)
            if (const TrafficStats* st = activeTrafficStats()) {
                const int chocs = st->bumps + st->accidents + st->scrapes;
                if (chocs > chocsVus_) chocTrafic(etat_);
                for (int k = quasiVus_; k < st->nearMisses; k++) quasiContact(etat_);
                chocsVus_ = chocs; quasiVus_ = st->nearMisses;
            }
            // glissière : au moment où la voiture touche la bordure (Player::update la bloque à map->edge)
            const double bord = g.map ? g.map->edge : 1.6;
            const bool contre = std::abs(pl.playerX) >= bord - 1e-6;
            if (contre && !contreBord_) chocGlissiere(etat_);
            contreBord_ = contre;
            plotsTouches(pl);
            camionsTouches(dt, pl, *c, t);   // WFS-119
            bandesPassees(pl);
        }
        distAvant_ = pl.dist;
        camionsTrafic(pl, t, tr);
        // police : trafic autour d'elle, conduite, contacts (règle du duel : duel::contacter)
        obs_.clear();
        if (tr) for (const TrafficCar& v : tr->cars) {
            const double s = pl.dist + t.relZ(v.z - pl.position);
            if (s - pl.dist > -300 / M && s - pl.dist < 400 / M) obs_.push_back({s, v.offset, v.crashed ? 0.0 : v.speed});
        }
        for (const Plot& p : plots_) if (!p.tombe) obs_.push_back({p.s, p.x, 0});
        for (const Camion& k : camions_) for (int f = 0; f < 3; f++) if (f != k.fileLibre) obs_.push_back({k.s, FILES[f], 0});   // WFS-119
        poursuite::avancer(police_, dt, c->raceTime, pl.dist, pl.playerX, pl.speed, obs_);
        if (c->state == RaceState::Play) {
            for (Police& p : police_) {
                duel::Ghost gh; gh.rd = p.rd; gh.x = p.x; gh.v = p.v; gh.car = policeCar_;
                const duel::ContactEvents e = duel::contacter(dt, pl, t, *g.car, gh);
                if ((e.chocs || e.frottements) && p.hitCd <= 0) {
                    p.hitCd = 1.5; chocPolice(etat_);
                    if (!p.fixe) p.v *= 0.85;
                    if (e.accident) c->accident(); else c->bump(std::max(0.1, e.impact));
                }
            }
            poursuite::pas(etat_, dt, plusProcheM(police_, pl.dist, pl.playerX));
            // RG-03 : un renfort tous les 2 km (6 au plus)
            const double km = pl.dist * M / 1000;
            int mobiles = 0; for (const Police& p : police_) mobiles += !p.fixe;
            if (mobiles < policeVoulue(km)) {
                police_.push_back(renfort(mobiles, pl.dist, pl.speed, graine_));
                annoncer("RENFORT DE POLICE");
                WFS_LOG("poursuite : renfort à %.2f km, %d voitures de police", km, mobiles + 1);
            }
            surprises(pl, t, tr);
            if (g_bancFin == 1) { perdre(etat_, etat_.score); g_bancFin = 0; }
            arrivee(etat_, km);
            if (etat_.arrete && c->state != RaceState::Over) {
                c->state = RaceState::Over;
                WFS_LOG("poursuite : ARRÊTÉ à %.2f km (%d chocs trafic, %d glissière, %d police)", km, etat_.chocsTrafic, etat_.chocsGlissiere, etat_.chocsPolice);
            }
        }
        bouchons(pl, t, tr);
        {   // WFS-131 (RG-07) : sirène continue du module son : police mobile la plus proche (m) et vitesse vers le joueur (m/s)
            double dSir = 1e9, vSir = 0;
            for (const Police& p : police_) if (!p.fixe) {
                const double dz = (p.rd - pl.dist) * M, dx = (p.x - pl.playerX) * RW, d = std::sqrt(dz * dz + dx * dx);
                if (d < dSir) { dSir = d; vSir = (dz < 0 ? p.v - pl.speed : pl.speed - p.v) * M; }
            }
            sfxSirene(dSir, vSir);
        }
        if (banT_ > 0) banT_ -= dt;
        if (c->state == RaceState::Over) {
            if (!fini_) {
                fini_ = true; ftJoueur_ = c->raceTime; arrivee(etat_, pl.dist * M / 1000);
                sfxInterface(etat_.gagne ? SfxId::PoursuiteEchappe : SfxId::PoursuiteArrete);   // WFS-131 (RG-06) : jingle de fin
                WFS_LOG("poursuite : fin — %s, score %d, %.2f km, %.1f s", etat_.gagne ? "victoire" : "ARRÊTÉ", etat_.score, pl.dist * M / 1000, ftJoueur_);
                // WFS-125 (RG-02) : record Poursuite (victoire > « ARRÊTÉ », puis score ou distance), écrit seulement s'il est battu
                nouveauRecord_ = false;
                Sauvegarde& sv = sauvegarde();
                if (sv.actif()) {
                    nouveauRecord_ = sv.battreRecordPoursuite(etat_.gagne, etat_.score, std::llround(pl.dist * M));
                    WFS_LOG("poursuite : record %s%s", texteRecord().c_str(), nouveauRecord_ ? " (NOUVEAU RECORD, enregistré)" : "");
                    if (nouveauRecord_) sv.enregistrer();
                }
            }
            overT_ += dt;
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || !g.player || !g.track) return;
        const double sP = g.player->dist;
        {   // banc : angle de la caméra « vitrine » placée devant la voiture (police visible derrière)
            const TrackPoint a = g.track->point(sP, 0), b = g.track->point(sP + 5 / M, 0);
            orbitAvant_ = (float)std::atan2(b.x - a.x, b.z - a.z);
        }
        for (const Police& p : police_) {
            const double rel = (p.rd - sP) * M;
            if (rel < -60 || rel > 300) continue;
            if (r.qualiteBasse && std::abs(rel) > 90) continue;   // WFS-134 (RG-11) : Basse : police dessinée jusqu'à 90 m (coût)
            const TrackPoint q = g.track->point(p.rd, p.x * RW);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th)) * rotateX((float)q.slope);
            const CarDef& cd = carById(policeCar_);
            const float L = cd.d.L, W = cd.d.W, H = cd.d.H;
            // WFS-134 (RG-11, remplace RG-08 / WFS-119 RG-04) : jamais de cubes, Basse comprise : toujours le vrai modèle (version
            // allégée au-delà de 60 m, avec fondu, choisie par Renderer::draw) ; sans modèle chargé : rien plutôt qu'une boîte
            (void)L;
            if (modele_.ok && &p - police_.data() < 16) { peintures_[&p - police_.data()] = lin(p.peinture); r.lotsProchaineVoiture(); r.draw(modele_, m, &peintures_[&p - police_.data()]); }
            // barre lumineuse dessinée par le code : rouge à gauche, bleu à droite, halos alternés (WFS-119 RG-04 : en qualité
            // Basse, au-delà de 80 m, halos seuls)
            // WFS-141 (RG-01, ANO-56) : rouge puis bleu, jamais ensemble (lampe éteinte : verre sombre, sans halo) ; halo de
            // 1,5 × la largeur de la lampe, couleur convertie par versHdr (avant : 1,4 m de demi-côté, × 4 : disque rose)
            const Gyrophare gy = gyrophare(g.time, (int)(&p - police_.data()));
            const float lampe = W * 0.32f, demi = HALO_GYRO * lampe / 2;
            if (!r.qualiteBasse || std::abs(rel) <= 80) {
                r.draw(cube_, gy.rouge ? matRouge_ : matVitre_, m * translate({-W * 0.17f, H - 0.02f, 0.1f}) * scale({lampe, 0.14f, 0.3f}));
                r.draw(cube_, gy.bleu ? matBleu_ : matVitre_, m * translate({W * 0.17f, H - 0.02f, 0.1f}) * scale({lampe, 0.14f, 0.3f}));
            }
            if (gy.rouge) r.halo(m.transformPoint({-W * 0.17f, H + 0.14f, 0.1f}), demi, haloRouge_);
            if (gy.bleu) r.halo(m.transformPoint({W * 0.17f, H + 0.14f, 0.1f}), demi, haloBleu_);
        }
        for (const Plot& p : plots_) {   // RG-05 : travaux (plots orange)
            const double rel = (p.s - sP) * M;
            if (p.tombe || rel < -30 || rel > 250) continue;
            const TrackPoint q = g.track->point(p.s, p.x * RW);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th));
            r.draw(cube_, matPlot_, m * scale({0.38f, 0.75f, 0.38f}));
        }
        for (const Camion& k : camions_) {   // WFS-119 : camion en travers (modèle existant agrandi), silhouette au loin et en qualité Basse (RG-04)
            const double rel = (k.s - sP) * M;
            if (rel < -40 || rel > 300) continue;
            const TrackPoint q = g.track->point(k.s, k.tv.xM);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th + 1.5707963)) * rotateX((float)q.slope);
            const CarDef& cd = carById(camionCar_);
            const float E = CAMION_ECHELLE, L = cd.d.L * E, W = cd.d.W * E, H = cd.d.H * E;
            (void)L; (void)W; (void)H;   // WFS-134 (RG-11) : camion = vrai modèle partout (allégé au-delà de 60 m), jamais de cubes
            if (camionModele_.ok) { peintureCamion_ = lin(0xd8dade); r.draw(camionModele_, m * scale({E, E, E}), &peintureCamion_); }
        }
        for (const Bande& b : bandes_) {   // WFS-119 : ralentisseur jaune (bandes noires hors qualité Basse) et 2 plots
            const double rel = (b.s - sP) * M;
            if (rel < -30 || rel > 250) continue;
            const TrackPoint q = g.track->point(b.s, 0);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th));
            r.draw(cube_, matJaune_, m * scale({(float)(2 * RW), 0.10f, 0.8f}));
            if (!r.qualiteBasse)
                for (int i = -3; i <= 3; i++) r.draw(cube_, matNoir_, m * translate({(float)(i * RW * 2 / 7), 0, 0}) * scale({1.0f, 0.105f, 0.82f}));
            for (int sg = -1; sg <= 1; sg += 2) r.draw(cube_, matPlot_, m * translate({(float)(sg * (RW + 0.4)), 0, 0}) * scale({0.38f, 0.75f, 0.38f}));
        }
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || !ui::fontReady() || g.ecran != Ecran::Course || !g.player) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        const Course* c = activeCourse();
        if (fini_ && overT_ >= 0.9) { resultat(r, W, H, u); return; }
        if (!c || c->state == RaceState::Over) return;
        // RG-06 : en haut au centre : score (100 → 1), police (nombre de voitures), bandeau de surprise
        const float cx = (g.marges.gauche + W - g.marges.droite) / 2, y0 = g.marges.haut + 2 * u, pw = 40 * u, ph = 15.5f * u;
        r.uiRect(cx - pw / 2, y0, pw, ph, SURFACE);
        const vec4 coul = etat_.score > 50 ? TEXTE : etat_.score > 20 ? ui::ORANGE : DANGER;
        ui::text(r, cx - pw / 2 + 2 * u, y0 + 1.6f * u, 2.4f * u, "SCORE", MUTED, ui::GAUCHE, 0.16f, false);
        ui::text(r, cx - pw / 2 + 2 * u, y0 + 4.6f * u, 9 * u, std::to_string(etat_.score), coul, ui::GAUCHE, 0.f, false, true);
        int mobiles = 0; for (const Police& p : police_) mobiles += !p.fixe;
        ui::text(r, cx + pw / 2 - 2 * u, y0 + 1.6f * u, 2.4f * u, "POLICE", MUTED, ui::DROITE, 0.16f, false);
        const bool flash = std::fmod(g.time, 0.5) < 0.25;
        for (int i = 0; i < POLICE_MAX; i++) {   // une pastille par voiture (allumées : rouge / bleu)
            const float px = cx + pw / 2 - 2 * u - (POLICE_MAX - i) * 2.8f * u + 0.6f * u;
            r.uiCircle(px + 1.f * u, y0 + 6.6f * u, 1.f * u, i < mobiles ? ((i % 2) ^ flash ? DANGER : BLEU) : vec4{1, 1, 1, 0.12f});
        }
        const double reste = std::max(0.0, KM_ARRIVEE - g.player->dist * M / 1000);
        ui::text(r, cx + pw / 2 - 2 * u, y0 + 9.6f * u, 3 * u, ui::decimal(reste, 2) + " KM", TEXTE, ui::DROITE, 0.06f, false, true);
        const float k = (float)std::clamp(g.player->dist * M / 1000 / KM_ARRIVEE, 0.0, 1.0);
        r.uiRect(cx - pw / 2 + 2 * u, y0 + ph - 1.8f * u, pw - 4 * u, 0.6f * u, vec4{1, 1, 1, 0.12f});
        r.uiRect(cx - pw / 2 + 2 * u, y0 + ph - 1.8f * u, (pw - 4 * u) * k, 0.6f * u, ui::ORANGE);
        if (banT_ > 0 && !ban_.empty()) {   // RG-05 : bandeau court
            // WFS-141 (RG-02, ANO-57) : à gauche sous le chrono et les boutons « Menu » et de vue (14 + 112 + 10 + 40 + 8 + 40 px
            // de 720, pause.cpp et camera_vue.cpp, + 10) : il ne cache plus l'obstacle, au centre de la route ; effacé après
            // 1,5 s. Taille inchangée (ordre permanent du 2026-11-06 sur la taille des textes : pas de passage à 5 %)
            const float k = H / 720.f, bw = std::max(46 * u, ui::width(3.6f * u, ban_, 0.1f) + 8 * u), bx = g.marges.gauche + 16 * k, by = g.marges.haut + 234 * k;
            r.uiRect(bx, by, bw, 7 * u, vec4{DANGER.x, DANGER.y, DANGER.z, 0.88f});
            ui::text(r, bx + bw / 2, by + 1.7f * u, 3.6f * u, ban_, TEXTE, ui::CENTRE, 0.1f, false, true);
        }
    }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Course) return false;
        if (appui_.consomme(t.id, t.down)) return true;   // relâcher du doigt de « Rejouer » (déjà dans la course suivante)
        if (!actif_ || !fini_ || overT_ < 0.9) return false;
        if (!t.down) return true;
        if (rejouerZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("poursuite : « Rejouer »"); rejouer_ = true; g.allerA(Ecran::Menu); }
        else if (menuZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("poursuite : « Menu »"); g.allerA(Ecran::Menu); }
        return true;   // l'écran de fin garde tous les touchers
    }

    // banc d'essai et test de parcours
    const Etat& etat() const { return etat_; }
    bool fini() const { return fini_ && overT_ >= 0.9; }
    int policeMobiles() const { int n = 0; for (const Police& p : police_) n += !p.fixe; return n; }
    int police(double* rd, bool* fixe, int max) const { int n = 0; if (actif_) for (const Police& p : police_) if (n < max) { rd[n] = p.rd; fixe[n++] = p.fixe; } return n; }   // WFS-173
    int policeDerriere(double sJ) const { int n = 0; for (const Police& p : police_) n += !p.fixe && p.rd < sJ && (sJ - p.rd) * M < 40; return n; }
    const std::string& bandeauActuel() const { return ban_; }
    void bancScore(int score) { etat_.score = score; }   // banc (WFS-119) : score remis avant les captures camion / ralentisseur
    float orbitAvant_ = 0;
    Zone rejouerZone() const { return rejouerZ_; }
    bool nouveauRecord() const { return nouveauRecord_; }
    // WFS-125 : « Victoire · 87 pts » ou « Arrêté · 3,42 km » ; « — » tant que le record est vide (RG-03)
    static std::string texteRecord() {
        const Sauvegarde& sv = sauvegarde();
        if (!sv.aRecordPoursuite()) return "—";
        return sv.recPVictoire ? "Victoire · " + std::to_string(sv.recPScore) + " pts" : "Arrêté · " + ui::decimal(sv.recPDistM / 1000.0, 2) + " km";
    }
    Zone menuZone() const { return menuZ_; }

private:
    void demarrer(Course& c, const Player& pl) {
        graine_ = g_banc ? 20261105u : (uint32_t)(nowSeconds() * 1000.0) * 2654435761u + 29u;
        c.duelFinish = KM_ARRIVEE * duel::KM;
        etat_ = Etat{}; prog_ = programme(graine_, g_banc);
        police_.clear(); plots_.clear(); camions_.clear(); bandes_.clear(); distAvant_ = pl.dist;
        for (int i = 0; i < POLICE_DEPART; i++) { Police p = renfort(i, pl.dist, 0, graine_); p.rd = pl.dist - (22 + 14 * i) / M; police_.push_back(p); }
        const TrafficStats* st = activeTrafficStats();
        chocsVus_ = st ? st->bumps + st->accidents + st->scrapes : 0; quasiVus_ = st ? st->nearMisses : 0;
        actif_ = true; fini_ = false; nouveauRecord_ = false; ftJoueur_ = 0; overT_ = 0; banT_ = 0; ban_.clear(); contreBord_ = false; sireneT_ = 0;
        WFS_LOG("poursuite : graine %u, %.0f km, score %d, %d voitures de police, %zu embouteillages, %zu surprises", graine_, KM_ARRIVEE,
                etat_.score, POLICE_DEPART, prog_.bouchons.size(), prog_.surprises.size());
    }
    void annoncer(const std::string& t) { ban_ = t; banT_ = BANDEAU_S; }   // WFS-141 (RG-02) : 1,5 s (avant : 4 s)

    // place une voiture du trafic loin du joueur (hors de vue) à la distance s, file f, vitesse v (unités/s)
    TrafficCar* deplacer(Traffic& tr, const Player& pl, Track& t, double s, int f, double v) {
        TrafficCar* best = nullptr; double loin = 0;
        for (TrafficCar& c : tr.cars) {
            const double rel = t.relZ(c.z - pl.position) * M;
            const double d = rel < 0 ? -rel * 3 : rel;   // derrière d'abord, puis le plus loin devant
            if ((rel < -80 || rel > 900) && d > loin && !reserve(&c)) { loin = d; best = &c; }
        }
        if (!best) return nullptr;
        best->z = t.wrap(pl.position + (s - pl.dist)); best->lane = best->target = f; best->offset = best->prevOff = FILES[f];
        best->speed = v; best->pending = false; best->signal = 0; best->crashed = false; best->laneYaw = 0;
        best->prevDz = t.relZ(best->z - pl.position);
        reserves_.push_back(best);
        return best;
    }
    bool reserve(const TrafficCar* c) const { return std::find(reserves_.begin(), reserves_.end(), c) != reserves_.end(); }

    // RG-04 : embouteillages : files lentes sur les voies sauf le passage, remplies 650 m avant
    void bouchons(const Player& pl, Track& t, Traffic* tr) {
        if (!tr) return;
        for (Bouchon& b : prog_.bouchons) {
            const double s0 = b.km * duel::KM, s1 = s0 + b.longueurM / M, devant = (s0 - pl.dist) * M;
            if (!b.place && devant < 650 && devant > 280) {
                b.place = true; reserves_.clear();
                Mulberry32 R(graine_ + (uint32_t)(b.km * 1000));
                int n = 0;
                for (int f = 0; f < 3; f++) {
                    if (f == b.passage) continue;
                    for (double d = 4 + R.next() * 4; d < b.longueurM && n < 48; d += 8.5 + R.next() * 3, n++)   // file serrée en tête
                        deplacer(*tr, pl, t, s0 + d / M, f, (b.kmhMin + R.next() * (b.kmhMax - b.kmhMin)) / 3.6 / M);
                }
                WFS_LOG("poursuite : embouteillage à %.2f km, %.0f m, %d voitures, passage file %d", b.km, b.longueurM, n, b.passage);
            }
            if (devant < 160 && devant > 120 && b.place && ban_ != "EMBOUTEILLAGE") annoncer("EMBOUTEILLAGE");
            if (!b.place || pl.dist > s1 + 200 / M) continue;
            // dans la zone : 20 à 40 km/h, pas de changement de file vers le passage (il reste étroit)
            for (TrafficCar& c : tr->cars) {
                const double s = pl.dist + t.relZ(c.z - pl.position);
                if (s < s0 || s > s1 || c.crashed || c.lane == b.passage) continue;
                const double vmax = (b.kmhMin + (c.rng % 1000) / 1000.0 * (b.kmhMax - b.kmhMin)) / 3.6 / M;
                c.speed = std::min(c.speed, vmax);
                if (c.target == b.passage) { c.target = c.lane; c.pending = false; c.signal = 0; }
            }
        }
    }

    // RG-05 : surprises annoncées par un bandeau, déclenchées 260 m avant
    void surprises(const Player& pl, Track& t, Traffic* tr) {
        for (Evenement& e : prog_.surprises) {
            const double s = e.km * duel::KM;
            if (e.declenche && !e.rappel && (e.type == Surprise::Camion || e.type == Surprise::Ralentisseur) && pl.dist >= s - RAPPEL_M / M && pl.dist < s) {
                e.rappel = true; annoncer(bandeau(e.type));   // WFS-119 : rappel du bandeau à l'approche
            }
            if (e.declenche || pl.dist < s - 260 / M) continue;
            e.declenche = true;
            reserves_.clear();
            switch (e.type) {
                case Surprise::Barrage:   // deux voitures de police en travers de deux files ; la file e.file reste libre
                    for (int f = 0; f < 3; f++) if (f != e.file) { Police p; p.fixe = true; p.rd = s; p.x = p.cibleX = FILES[f]; p.peinture = f % 2 ? 0x14224a : 0xf2f4f8; police_.push_back(p); }
                    break;
                case Surprise::Deboite: {   // une voiture d'une file voisine se rabat devant le joueur
                    if (!tr) break;
                    int fj = 0; double bd = 9; for (int f = 0; f < 3; f++) if (std::abs(FILES[f] - pl.playerX) < bd) { bd = std::abs(FILES[f] - pl.playerX); fj = f; }
                    const int fv = fj == 1 ? (e.file % 2 ? 0 : 2) : 1;
                    TrafficCar* c = deplacer(*tr, pl, t, pl.dist + 45 / M, fv, std::max(pl.speed * 0.7, 8 / M));
                    if (c) { c->target = fj; c->signal = fj > fv ? 1 : -1; }
                    break;
                }
                case Surprise::Travaux:   // plots sur 90 m : la file e.file est fermée
                    for (int i = 0; i < 12; i++) plots_.push_back({s + i * 8 / M, FILES[e.file] + (i == 0 ? 0.25 : 0), false});
                    break;
                case Surprise::Accident:  // deux voitures accidentées en travers d'une file (feux de détresse du trafic vivant)
                    if (!tr) break;
                    for (int i = 0; i < 2; i++)
                        if (TrafficCar* c = deplacer(*tr, pl, t, s + i * 6 / M, e.file, 0)) {
                            c->crashed = true; c->crashT = -12; c->crashYaw = i ? -0.9 : 0.7; c->crashSpin = 0; c->crashVx = 0;
                        }
                    break;
                case Surprise::Camion: {  // WFS-119 : gros véhicule en travers de 2 files, la file e.file reste libre
                    const CarDef& cd = carById(camionCar_);
                    Camion k; k.s = s; k.fileLibre = fileLibreCamion(e.file); k.tv = camionTravers(k.fileLibre, cd.d.L, cd.d.W);
                    camions_.push_back(k);
                    break;
                }
                case Surprise::Ralentisseur: bandes_.push_back({s, false}); break;   // WFS-119 : toute la largeur
                default: break;
            }
            annoncer(bandeau(e.type));
            WFS_LOG("poursuite : surprise « %s » à %.2f km (file %d)", bandeau(e.type), e.km, e.file);
        }
    }
    // plots percutés : −3 comme la glissière, la voiture ralentit, le plot tombe
    void plotsTouches(Player& pl) {
        const double demiL = pl.car->d.L / 2 / M, demiW = pl.car->d.W / 2 / RW;
        for (Plot& p : plots_) {
            if (p.tombe || std::abs(p.s - pl.dist) > demiL + 0.2 / M || std::abs(p.x - pl.playerX) > demiW + 0.03) continue;
            p.tombe = true; pl.speed *= 0.9; chocGlissiere(etat_);
        }
    }

    // WFS-119 : camion en travers : boîte (demi-largeur tv.demiX, demi-épaisseur tv.demiZ) ; de face : arrêté net
    // derrière lui (comme un choc avec le trafic, −5) ; par le bout : repoussé de côté (frottement)
    void camionsTouches(double dt, Player& pl, Course& c, Track& t) {
        for (Camion& k : camions_) {
            k.cd = std::max(0.0, k.cd - dt);
            const double dzM = (k.s - pl.dist) * M, demiZ = k.tv.demiZ + pl.car->d.L / 2;
            const double dxM = pl.playerX * RW - k.tv.xM, demiX = k.tv.demiX + pl.car->d.W / 2;
            if (std::abs(dzM) >= demiZ || std::abs(dxM) >= demiX) continue;
            const double ovZ = demiZ - std::abs(dzM), ovX = demiX - std::abs(dxM), impact = std::abs(pl.speed) / MAX_SPEED;
            const int cote = dxM > 0 ? 1 : -1;
            if (ovX < ovZ * 0.7) { pl.playerX += cote * (ovX + 0.05) / RW; pl.speed *= 1 - 0.8 * dt; }
            else {
                const double rec = (dzM > 0 ? 1 : -1) * (ovZ + 0.15) / M;
                pl.position = t.wrap(pl.position - rec); pl.dist = std::max(0.0, pl.dist - rec);
                pl.speed = std::min(pl.speed, pl.speed * 0.15);
                if (k.cd <= 0 && impact > 0.3) { pl.accident(std::min(1.0, impact), cote); c.accident(); }
            }
            if (k.cd <= 0) { k.cd = 1.0; chocTrafic(etat_); c.bump(std::max(0.1, impact)); WFS_LOG("poursuite : choc contre le camion (%.0f km/h)", impact * 260); }
        }
    }
    // WFS-119 : le trafic des 2 files fermées s'arrête derrière le camion (la file libre roule) ; une voiture posée
    // dans le camion au déclenchement passe devant lui
    void camionsTrafic(const Player& pl, Track& t, Traffic* tr) {
        if (!tr) return;
        for (const Camion& k : camions_) {
            const double devant = (k.s - pl.dist) * M;
            if (devant < -150 || devant > 700) continue;
            for (TrafficCar& c : tr->cars) {
                if (c.crashed || c.lane == k.fileLibre) continue;
                if (std::abs(c.offset * RW - k.tv.xM) >= k.tv.demiX + c.w * RW) continue;
                const double s = pl.dist + t.relZ(c.z - pl.position);
                const double ecart = (k.s - s) * M - k.tv.demiZ - c.len * M / 2;   // > 0 : derrière le camion
                if (ecart < -2 * k.tv.demiZ - c.len * M) continue;                  // déjà passée
                if (ecart < 0) { c.z = t.wrap(c.z + (-ecart + 2 * k.tv.demiZ + c.len * M + 2) / M); continue; }
                if (ecart < 60) c.speed = std::min(c.speed, std::max(0.0, ecart - 3) * 0.6 / M);
            }
        }
    }
    // WFS-119 : ralentisseur franchi : −3 au-dessus de 90 km/h (poursuite::ralentisseur), la voiture tressaute et ralentit
    void bandesPassees(Player& pl) {
        for (Bande& b : bandes_) {
            if (b.passe || distAvant_ >= b.s || pl.dist < b.s) continue;
            b.passe = true;
            const double kmh = std::abs(pl.kmh());
            const int perdu = ralentisseur(etat_, kmh);
            if (perdu > 0) pl.speed *= 0.85;
            WFS_LOG("poursuite : ralentisseur passé à %.0f km/h : %s", kmh, perdu > 0 ? "−3" : "rien");
        }
    }

    // RG-07 : écran de fin : victoire (score final, temps) ou « ARRÊTÉ » ; « Rejouer » / « Menu » ;
    // WFS-141 (RG-03, ANO-59) : boutons parallélogrammes et liseré orange de la fin Checkpoint (biseauFin) ; tailles des
    // boutons et des textes inchangées (ordre permanent de l'utilisateur du 2026-11-06)
    void resultat(Renderer& r, float W, float H, float u) {
        r.uiRect(0, 0, W, H, FOND);
        const bool rec = sauvegarde().actif();   // WFS-125 (RG-04) : badge et ligne « RECORD » sous les lignes du résultat
        const float pw = 66 * u, ph = (58 + (rec ? 13 : 0)) * u, px0 = (W - pw) / 2, py0 = (H - ph) / 2;
        r.uiRect(px0, py0, pw, ph, SURFACE);
        r.uiRect(px0, py0, pw, 0.6f * u, ui::ORANGE);   // WFS-141 : liseré néon de la fin Checkpoint
        ui::text(r, W / 2, py0 + 3 * u, 2.6f * u, "POURSUITE POLICE · 10 KM", ui::CYAN, ui::CENTRE, 0.16f, false);
        const bool v = etat_.gagne;
        ui::text(r, W / 2, py0 + 7 * u, 9 * u, v ? "VICTOIRE" : "ARRÊTÉ", v ? ui::ORANGE : DANGER, ui::CENTRE, 0.04f, false, true);
        float y = py0 + 20 * u;
        auto ligne = [&](const char* a, const std::string& b) {
            ui::text(r, px0 + 6 * u, y, 3.4f * u, a, MUTED, ui::GAUCHE, 0.f, false);
            ui::text(r, px0 + pw - 6 * u, y, 3.4f * u, b, TEXTE, ui::DROITE, 0.f, false, true);
            y += 5.6f * u;
        };
        ligne("Score final", std::to_string(etat_.score));
        ligne("Temps", ui::chrono(ftJoueur_));
        ligne("Chocs trafic / glissière / police", std::to_string(etat_.chocsTrafic) + " / " + std::to_string(etat_.chocsGlissiere) + " / " + std::to_string(etat_.chocsPolice));
        ligne("Voitures de police", std::to_string(policeMobiles()));
        const float bw = 24 * u, bh = 7 * u, gap = 3 * u, by = py0 + ph - bh - 3 * u, bx = (W - (2 * bw + gap)) / 2;
        if (rec) dessinerRecord(r, W / 2, y + 0.5f * u, by - 2 * u, u, texteRecord(), nouveauRecord_);
        rejouerZ_ = {bx, by, bw, bh}; menuZ_ = {bx + bw + gap, by, bw, bh};
        biseauFin(r, rejouerZ_.x, by, bw, bh, 14 * H / 720, ui::ORANGE);
        ui::text(r, rejouerZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "REJOUER", vec4{0.04f, 0.04f, 0.06f, 1}, ui::CENTRE, 0.08f, false);
        biseauFin(r, menuZ_.x, by, bw, bh, 14 * H / 720, {30 / 255.f, 33 / 255.f, 48 / 255.f, 1});   // « Menu » de la fin Checkpoint (#1E2130)
        ui::text(r, menuZ_.x + bw / 2, by + 1.9f * u, 3.4f * u, "MENU", TEXTE, ui::CENTRE, 0.08f, false);
    }

    Renderer* r_ = nullptr;
    Model modele_; bool modeleEssaye_ = false; const char* policeCar_ = "p208";
    Mesh cube_; Material matRouge_, matBleu_, matPlot_, matBlanc_, matBleuCaisse_, matVitre_, matJaune_, matNoir_;
    vec3 haloRouge_{}, haloBleu_{};   // WFS-141 : couleurs HDR des halos des gyrophares
    Model camionModele_; bool camionEssaye_ = false; const char* camionCar_ = "x5"; vec3 peintureCamion_{};   // WFS-119
    std::vector<Camion> camions_;
    std::vector<Bande> bandes_;
    double distAvant_ = 0;
    vec3 peintures_[16];   // Renderer::draw garde l'adresse de la peinture jusqu'à la fin de l'image
    std::vector<Police> police_;
    std::vector<Obstacle> obs_;
    std::vector<Plot> plots_;
    std::vector<const TrafficCar*> reserves_;
    Programme prog_;
    Etat etat_;
    uint32_t graine_ = 0;
    int chocsVus_ = 0, quasiVus_ = 0;
    bool actif_ = false, fini_ = false, rejouer_ = false, contreBord_ = false, nouveauRecord_ = false;
    double ftJoueur_ = 0, overT_ = 0, banT_ = 0, sireneT_ = 0;
    std::string ban_;
    Zone rejouerZ_, menuZ_;
    ui::FiltreAppui appui_;
};
PoursuiteSystem* PoursuiteSystem::actif = nullptr;

} // namespace

WFS_SYSTEM(PoursuiteSystem, 224)   // gameplay (200-299) : après la course (210), le duel (220) et la course à 5 (222)

// WFS-125 : banc d'essai (parcours RG-09)
int poursuitePolice(double* rd, bool* fixe, int max) { return PoursuiteSystem::actif ? PoursuiteSystem::actif->police(rd, fixe, max) : 0; }   // WFS-173
bool poursuiteBouton(const std::string& nom, float& x, float& y) {
    const PoursuiteSystem* s = PoursuiteSystem::actif;
    if (!s) return false;
    const Zone z = nom == "rejouer" ? s->rejouerZone() : s->menuZone();
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
}
bool poursuiteNouveauRecord() { return PoursuiteSystem::actif && PoursuiteSystem::actif->nouveauRecord(); }
void poursuiteBancFin(int mode) { g_bancFin = mode; }
void poursuiteBancFixe(bool oui) { g_banc = oui; }

// ---------- banc d'essai (WFS_BANC=course5) ----------
// police avec gyrophares derrière le joueur, bandeau de surprise (barrage), embouteillage, HUD du score, fin « ARRÊTÉ »
// puis « Rejouer » et victoire à 10 km
void benchPoursuite(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    g_banc = true;
    modeCourse() = ModeCourse::Poursuite;
    if (!g.init(r, "golf", "ville")) { g_banc = false; courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint; return; }
    double t = 1200;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto capture = [&](const char* nom) {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png");
        const PoursuiteSystem* s = PoursuiteSystem::actif;
        const Traffic* tr = activeTraffic();
        WFS_LOG("capture %s : %.2f km, %.0f km/h, score %d, police %d (dont %d à moins de 40 m derrière), trafic %zu, bandeau « %s », %u appels de dessin",
                nom, g.player().dist * M / 1000, g.player().kmh(), s ? s->etat().score : -1, s ? s->policeMobiles() : 0,
                s ? s->policeDerriere(g.player().dist) : 0, tr ? tr->cars.size() : (size_t)0, s ? s->bandeauActuel().c_str() : "", r.appelsDessin());
    };
    auto jusqua = [&](double km, int max) { for (int i = 0; i < max && g.player().dist * M / 1000 < km; i++) { g.update(1.0 / 60); if (i % 12 == 0) g.render(t += 0.2); } };
    struct Mesure { const char* ou; double km; unsigned appels; };
    std::vector<Mesure> mesures;   // WFS-119 RG-04 : comparées plus bas à une course Checkpoint, mêmes distances
    // appels de dessin en qualité Basse (téléphone), sans capture ; WFS-119 : moyenne de 10 images sur 0,5 s (le trafic
    // autour du joueur fait varier le compte d'une image à l'autre)
    auto appelsBasse = [&]() {
        ecrans::Reglages& R = ecrans::reglages(); const int q0 = R.qualite;
        R.qualite = 0; ecrans::appliquerQualiteNiveau(r);
        unsigned somme = 0;
        for (int i = 0; i < 10; i++) {
            if (i) for (int k = 0; k < 3; k++) g.update(1.0 / 60);
            g.render(t += 0.02); g.render(t += 0.02); somme += r.appelsDessin();
        }
        const unsigned n = (somme + 5) / 10;
        R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
        return n;
    };
    auto basse = [&](const char* ou) {
        const unsigned n = appelsBasse();
        mesures.push_back({ou, g.player().dist * M / 1000, n});
        WFS_LOG("poursuite (RG-08) : %s en qualité Basse : %u appels de dessin", ou, n);
    };
    // RG-12 : parcours au doigt : accueil → « Course solo » (panneau des modes) → carte « Poursuite » → « Continuer » →
    // garage → « Choisir » → terrain → « Rouler » → course de poursuite (la fin et « Menu » → accueil : plus bas)
    int ecarts = 0, doigt = 60;
    auto appuyer = [&](float x, float y) {   // doigt Android : down, 4 mouvements, up (une image dessinée entre chaque)
        const int id = doigt++;
        g.render(t += 1.0 / 60);
        g.onTouch({id, x, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60);
        for (int k = 1; k <= 4; k++) { g.onTouch({id, x + 0.4f * k, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60); }
        g.onTouch({id, x + 2, y, false}); images(4, 2);
    };
    auto attendu = [&](Ecran e, const char* etape) {
        if (g.ecran() == e) WFS_LOG("poursuite (parcours) : %s → écran %s (attendu)", etape, nomEcran(e));
        else { ecarts++; WFS_ERR("poursuite (parcours) : %s → écran %s, attendu %s", etape, nomEcran(g.ecran()), nomEcran(e)); }
    };
    {   // cartes et « Continuer » du panneau des modes (mise en page de modes_ecran.cpp, marges nulles au banc)
        const float W = (float)r.width(), H = (float)r.height(), u = H / 100.f, cx = W / 2;
        const float gap = 2.4f * u, cw = std::min(42 * u, (W - 8 * u - 3 * gap) / 4), x0 = cx - (4 * cw + 3 * gap) / 2;
        g.allerA(Ecran::Menu); images(30, 10);
        choixModeBanc(true); ouvrirChoixMode(); images(10, 5);
        if (!choixModeOuvert()) { ecarts++; WFS_ERR("poursuite (parcours) : panneau des modes fermé"); }
        appuyer(x0 + 2 * (cw + gap) + cw / 2, 26 * u + 17 * u);                         // carte 03 « Poursuite »
        appuyer(x0 + 4 * cw + 3 * gap - 14 * u, H - 8 * u - 6 * u + 4 * u);               // « Continuer »
        attendu(Ecran::Garage, "« Poursuite » puis « Continuer »");
        if (modeCourse() != ModeCourse::Poursuite) { ecarts++; WFS_ERR("poursuite (parcours) : mode %s après « Continuer »", nomMode(modeCourse())); }
        else WFS_LOG("poursuite (parcours) : mode %s retenu", nomMode(modeCourse()));
        float x = 0, y = 0;
        for (int i = 0; i < 600 && !ecrans::garageBouton("choisir", x, y); i++) images(1, 4);
        images(30, 4);
        if (ecrans::garageBouton("choisir", x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("poursuite (parcours) : « Choisir » absent"); }
        attendu(Ecran::Terrain, "garage puis « Choisir »");
        images(20, 4);
        if (ecrans::centreRouler(x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("poursuite (parcours) : « Rouler » absent"); }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) images(1, 1);
        attendu(Ecran::Course, "terrain puis « Rouler »");
        choixModeBanc(false);
        if (g.ecran() != Ecran::Course || modeCourse() != ModeCourse::Poursuite) {   // parcours raté : la suite des captures quand même
            modeCourse() = ModeCourse::Poursuite; g.allerA(Ecran::Menu); images(2, 1); g.allerA(Ecran::Course); images(2, 1);
        }
    }
    g.camMode = 0; g.setAutopilot(true);
    images(3 * 60, 8);        // décompte (3 s)
    for (int i = 0; i < 20 * 60; i++) { const Course* c = activeCourse(); if (c && c->raceTime >= 8.6) break; g.update(1.0 / 60); if (i % 12 == 0) g.render(t += 0.2); }
    // la 2e voiture de police tente de pousser (attaque de 7,6 à 10,6 s) : vue de devant, police derrière le joueur
    if (const PoursuiteSystem* s = PoursuiteSystem::actif) { g.camMode = 2; g.render(t += 0.02); g.orbit = s->orbitAvant_ + 0.5f; }
    capture("poursuite-police");
    g.camMode = 0; g.render(t += 0.02);
    basse("police au contact");
    jusqua(0.21, 60 * 60);    // barrage annoncé 260 m avant (0,45 km) ; WFS-141 (RG-02) : 20 m après l'annonce (bandeau de 1,5 s visible)
    capture("poursuite-surprise");
    jusqua(0.93, 60 * 60);    // embouteillage à 0,9 km
    capture("poursuite-bouchon");
    basse("embouteillage");
    images(5 * 60, 10);
    capture("poursuite-hud");
    basse("HUD (police au contact, embouteillage)");
    // WFS-119 : ralentisseur puis camion en travers (score remis à 100 : l'embouteillage du banc l'a entamé)
    if (PoursuiteSystem* s0 = PoursuiteSystem::actif) { WFS_LOG("poursuite (banc) : score %d remis à %d", s0->etat().score, SCORE_DEPART); s0->bancScore(SCORE_DEPART); }
    // WFS-141 (RG-02) : le pilote automatique reste bloqué derrière le barrage (0,45 km) : saut à 150 m du ralentisseur
    g_bancSautKm = BANC_RALENTISSEUR_KM - 0.15; images(2, 1);
    WFS_LOG("poursuite (banc) : saut à %.2f km (ralentisseur à %.2f km)", g.player().dist * M / 1000, BANC_RALENTISSEUR_KM);
    jusqua(BANC_RALENTISSEUR_KM - 0.018, 60 * 60);   // WFS-141 (RG-02) : capture à 18 m de l'obstacle (15 à 20 m)
    capture("poursuite-ralentisseur");
    basse("ralentisseur");
    jusqua(BANC_RALENTISSEUR_KM + 0.03, 20 * 60);
    if (const PoursuiteSystem* s0 = PoursuiteSystem::actif) WFS_LOG("poursuite (banc) : après le ralentisseur : score %d, %d ralentisseur(s) passé(s) trop vite", s0->etat().score, s0->etat().ralentisseurs);
    jusqua(BANC_CAMION_KM - 0.018, 60 * 60);
    capture("poursuite-camion");
    basse("camion en travers");
    g_bancFin = 1; images(90, 10);
    capture("poursuite-arrete");
    // RG-07 / RG-12 : vrais touchers sur l'écran de fin : « Rejouer » (nouvelle poursuite), saut à 150 m de l'arrivée
    // (victoire), puis « Menu » → accueil
    auto toucher = [&](const Zone& z, const char* nom) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({7, x, y, true}); images(2, 1); g.onTouch({7, x, y, false}); images(4, 2);
        WFS_LOG("poursuite (parcours) : toucher « %s » (%.0f, %.0f) → écran %s", nom, x, y, nomEcran(g.ecran()));
    };
    if (PoursuiteSystem* s = PoursuiteSystem::actif) toucher(s->rejouerZone(), "Rejouer");
    images(30, 10);
    const PoursuiteSystem* s = PoursuiteSystem::actif;
    WFS_LOG("poursuite (parcours) : après « Rejouer » : écran %s, score %d, %.2f km", nomEcran(g.ecran()), s ? s->etat().score : -1, g.player().dist * M / 1000);
    images(4 * 60, 8);
    g_bancFin = 2;
    const Course* c = activeCourse();
    for (int i = 0; i < 60 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
    images(90, 10);
    capture("poursuite-victoire");
    if (PoursuiteSystem* s2 = PoursuiteSystem::actif) toucher(s2->menuZone(), "Menu");
    images(10, 5);
    attendu(Ecran::Menu, "fin puis « Menu »");
    WFS_LOG("poursuite (parcours) : accueil → Course solo → Poursuite → Continuer → garage → Choisir → terrain → Rouler → poursuite → fin → Menu → accueil : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    // WFS-119 RG-03 / RG-06 : redémarrage simulé : la sauvegarde écrite par « Continuer » (clé mode) est relue par un
    // nouveau lancement (mode par défaut : Checkpoint) ; « Course solo » → le panneau rouvre sur Poursuite (« Continuer »
    // sans toucher de carte → garage en Poursuite). Ancienne sauvegarde (sans « poursuite ») : Checkpoint.
    {
        Sauvegarde ecrite; ecrite.mode = cleMode(modeCourse());
        Sauvegarde relue; const bool lu = relue.lire(ecrite.texte());
        modeCourse() = ModeCourse::Checkpoint;   // nouveau lancement
        modeCourse() = modeDeCle(relue.mode);
        WFS_LOG("poursuite (parcours) : redémarrage : sauvegarde « mode=%s » relue (%s) → mode %s", relue.mode.c_str(), lu ? "valide" : "invalide", nomMode(modeCourse()));
        Sauvegarde ancienne; ancienne.lire("version=1\ncar=golf\n");
        Sauvegarde inconnue; inconnue.lire("version=1\nmode=helico\n");
        if (modeDeCle(ancienne.mode) != ModeCourse::Checkpoint || modeDeCle(inconnue.mode) != ModeCourse::Checkpoint) { ecarts++; WFS_ERR("poursuite (parcours) : ancienne sauvegarde : mode %s", nomMode(modeDeCle(ancienne.mode))); }
        else WFS_LOG("poursuite (parcours) : ancienne sauvegarde (sans clé mode, ou mode inconnu) → Checkpoint");
        const float W = (float)r.width(), H = (float)r.height(), u = H / 100.f, cx = W / 2;
        const float gap = 2.4f * u, cw = std::min(42 * u, (W - 8 * u - 3 * gap) / 4), x0 = cx - (4 * cw + 3 * gap) / 2;
        choixModeBanc(true); ouvrirChoixMode(); images(10, 5);
        if (!choixModeOuvert()) { ecarts++; WFS_ERR("poursuite (parcours) : redémarrage : panneau des modes fermé"); }
        capture("poursuite-redemarrage");
        appuyer(x0 + 4 * cw + 3 * gap - 14 * u, H - 8 * u - 6 * u + 4 * u);   // « Continuer », sans toucher de carte
        attendu(Ecran::Garage, "redémarrage, Course solo puis « Continuer »");
        if (modeCourse() != ModeCourse::Poursuite) { ecarts++; WFS_ERR("poursuite (parcours) : redémarrage : panneau sur %s, attendu Poursuite", nomMode(modeCourse())); }
        else WFS_LOG("poursuite (parcours) : redémarrage : le panneau des modes rouvre sur Poursuite");
        choixModeBanc(false);
        g.allerA(Ecran::Menu); images(10, 5);
        WFS_LOG("poursuite (parcours) : parcours complet et redémarrage : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    }
    // WFS-119 RG-04 : même terrain, même heure, course Checkpoint : appels de dessin en Basse aux mêmes distances
    {
        modeCourse() = ModeCourse::Checkpoint;
        g.allerA(Ecran::Course); images(3 * 60, 8);
        int pire = -100000; double kmMesure = -1; unsigned nMesure = 0;
        for (const Mesure& ms : mesures) {
            jusqua(ms.km, 60 * 60);
            // WFS-144 : mesures de la poursuite au même endroit (bouchon) : Checkpoint mesuré une fois à cet endroit, pas 50 m plus loin
            const unsigned n = std::abs(ms.km - kmMesure) < 0.01 ? nMesure : (kmMesure = ms.km, nMesure = appelsBasse());
            const int ecart = (int)ms.appels - (int)n;
            pire = std::max(pire, ecart);
            WFS_LOG("poursuite (RG-04) : %s, %.2f km : poursuite %u, Checkpoint %u (%.2f km) : écart %+d", ms.ou, ms.km, ms.appels, n, g.player().dist * M / 1000, ecart);
        }
        WFS_LOG("poursuite (RG-04) : écart maximal en qualité Basse : %+d appels de dessin (cible ≤ 60) : %s", pire, pire <= 60 ? "OK" : "ÉCHEC");
    }
    g.setAutopilot(false);
    g_banc = false; g_bancFin = 0;
    modeCourse() = ModeCourse::Checkpoint;
    courseBenchRules(false);
    g.shutdown();
}

// ---------- WFS-119 RG-08 (ANO-53) : course à 5, « Rejouer » puis appui suivant (groupe course5) ----------
// Le doigt de « Rejouer » se relève dans la course suivante : avant la correction, le filtre d'appui de la course à 5 le
// gardait (relâcher ignoré hors écran de fin), et le premier appui de ce doigt sur l'écran de fin suivant était perdu.
int benchCourse5Rejouer(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    modeCourse() = ModeCourse::Course5; kmCourse5() = 2;
    if (!g.init(r, "golf", "ville")) { courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4; return 1; }
    double t = 900;
    int ecarts = 0;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto jusquaFin = [&]() {
        const Course* c = activeCourse();
        for (int i = 0; i < 150 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
        images(90, 10);
        return c && c->state == RaceState::Over;
    };
    // WFS-141 : place des boutons de l'écran de fin de la course à 5 lue sur l’écran dessiné (course5Bouton) et non plus recalculée
    const float W = (float)r.width(), H = (float)r.height();
    const int doigt = 0;   // Android : le premier doigt posé a toujours l'identifiant 0
    auto toucher = [&](float x, float y, const char* nom) {
        g.onTouch({doigt, x, y, true}); images(2, 1); g.onTouch({doigt, x, y, false}); images(4, 2);
        WFS_LOG("course à 5 (parcours) : toucher « %s » (%.0f, %.0f) → écran %s", nom, x, y, nomEcran(g.ecran()));
    };
    g.allerA(Ecran::Menu); images(10, 5);
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    if (!jusquaFin()) { ecarts++; WFS_ERR("course à 5 (parcours) : première course pas finie"); }
    float bx = 0, by = 0;
    if (!course5Bouton("rejouer", bx, by)) { ecarts++; WFS_ERR("course à 5 (parcours) : « Rejouer » absent"); }
    toucher(bx, by, "Rejouer");
    const Course* c = activeCourse();
    if (g.ecran() != Ecran::Course || !c || c->state == RaceState::Over) { ecarts++; WFS_ERR("course à 5 (parcours) : « Rejouer » → pas de nouvelle course"); }
    else WFS_LOG("course à 5 (parcours) : « Rejouer » → nouvelle course");
    g.setAutopilot(false);
    images(4 * 60, 10);   // décompte 3-2-1-GO
    const double v0 = g.player().kmh();
    g.onTouch({doigt, W * 0.89f, H * 0.74f, true}); images(90, 10);   // gaz tenu 1,5 s, même doigt
    const double v1 = g.player().kmh();
    g.onTouch({doigt, W * 0.89f, H * 0.74f, false}); images(2, 1);
    if (v1 < v0 + 20) { ecarts++; WFS_ERR("course à 5 (parcours) : gaz non pris (%.0f → %.0f km/h)", v0, v1); }
    else WFS_LOG("course à 5 (parcours) : appui suivant (gaz) pris : %.0f → %.0f km/h", v0, v1);
    g.setAutopilot(true);
    if (!jusquaFin()) { ecarts++; WFS_ERR("course à 5 (parcours) : deuxième course pas finie"); }
    {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/course5-rejouer.png");
        WFS_LOG("capture course5-rejouer : écran de fin de la 2e course (après « Rejouer »)");
    }
    if (!course5Bouton("menu", bx, by)) { ecarts++; WFS_ERR("course à 5 (parcours) : « Menu » absent"); }
    toucher(bx, by, "Menu");
    if (g.ecran() != Ecran::Menu) { ecarts++; WFS_ERR("course à 5 (parcours) : « Menu » (même doigt que « Rejouer ») → écran %s, attendu menu", nomEcran(g.ecran())); }
    WFS_LOG("course à 5 (parcours) : fin → « Rejouer » → nouvelle course → gaz pris → fin → « Menu » → accueil : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    g.setAutopilot(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    return ecarts;
}

} // namespace wfs
