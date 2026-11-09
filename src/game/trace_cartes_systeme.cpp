// WaForSpeed — tracé 2 (ticket WFS-136, épic E3) : système des feux de la carte en cours (feuA pour le décor), feu grillé
// sans choc (+50, RG-03) et bandeau « Nouveaux tracés » au premier départ après la mise de côté des records (RG-07).
#include "trace_cartes.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include "game.h"
#include "../render/renderer.h"
#include "course.h"
#include "course_system.h"
#include "player.h"
#include "sauvegarde.h"
#include "track.h"
#include "traffic.h"
#include "traffic_system.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../ui/toasts.h"

namespace wfs::trace {
// ---------------------------------------------------------------- système : feux de la carte, feu grillé, records
// Ordre 215 : après les règles de course (210), avant les bandeaux (425).
namespace {
// banc d'essai : vue de dessus (ou plongeante) d'une section, route seule (scène dédiée), pour voir la forme du tracé
struct VueBanc { bool actif = false; vec3 oeil{}, cible{}, haut{0, 1, 0}; float fov = 50; } g_vue;

class TraceSysteme : public System {
public:
    const char* name() const override { return "Tracé et feux"; }
    bool scene(FrameParams& fp, SceneDediee& sd, const GameView& g) override {
        if (!g_vue.actif) return false;
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        fp.view = lookAt(g_vue.oeil, g_vue.cible, g_vue.haut);
        fp.proj = perspectiveVk(g_vue.fov * 3.14159265f / 180.f, aspect, 0.5f);
        fp.camPos = g_vue.oeil;
        fp.fogDensity = 0; fp.shadows = false; fp.hasFocus = false;
        sd.route = true; sd.joueur = true; sd.cible = g_vue.cible;
        return true;
    }
    void update(double, const GameView& g) override {
        const Track* piste = g.track && g.track->version() == 2 ? g.track : nullptr;
        const Traffic* tr = activeTraffic();
        if (tr) horloge_ = tr->sim;
        poserCarte(piste, horloge_);
        Course* c = activeCourseMut();
        if (!piste || !g.player || !c || g.ecran != Ecran::Course) { attente_ = -1; return; }
        // RG-07 : nouveaux tracés, une fois, au premier départ après la mise de côté des anciens records
        if (c->state == RaceState::Play && sauvegarde().nouveauxTraces) {
            sauvegarde().nouveauxTraces = false;
            toasts::pousser("Nouveaux tracés", "nouveaux records à battre", 0);
        }
        if (c->state != RaceState::Play) { attente_ = -1; return; }
        // RG-03 : feu grillé sans toucher personne → +50 (arcade : aucune pénalité)
        const double pos = g.player->position;
        for (const Section& s : piste->sections()) {
            if (s.feu < 0 || s.depart) continue;
            const double d = piste->relZ(pos - s.ligneArret * SEG);
            const double av = avant_[(size_t)std::min(s.feu, 15)];
            if (av < 0 && d >= 0 && d < 30 / M && attente_ < 0 && feuCarrefour(s.feu, horloge_) == Feu::Rouge) {
                attente_ = s.feu; chocs_ = c->accidents + c->bumps + c->scrapes;
            }
            if (attente_ == s.feu) {
                if (c->accidents + c->bumps + c->scrapes != chocs_ || d < -1 / M) attente_ = -1;   // choc ou recul : rien
                else if (d > (CARREFOUR_M + 15) / M) {
                    attente_ = -1; c->bonus += 50;
                    toasts::pousser("Feu grillé", "+50", 0);
                    WFS_LOG("feux : feu %d grillé sans choc : +50", s.feu);
                }
            }
            avant_[(size_t)std::min(s.feu, 15)] = d;
        }
    }
private:
    double avant_[16] = {};
    int attente_ = -1, chocs_ = 0;
    double horloge_ = -3;
};
WFS_SYSTEM(TraceSysteme, 215)
} // namespace

// ---------------------------------------------------------------- banc d'essai (RG-08)
// Captures trace-rond-point (Ville, vue de dessus), trace-feux-rouge (Ville, au sol : file arrêtée à la ligne, voiture qui
// traverse), trace-epingle (Campagne, vue de dessus), trace-autoroute (Ville, vue plongeante sur la longue courbe), et au sol
// avec le décor actuel : trace-rond-point-sol, trace-epingle-sol. Rend le nombre d'écarts.
namespace {
// copie (la piste du banc est reconstruite à chaque scène : pas de pointeur vers ses sections)
bool premiere(const Track& t, Type ty, Section& out) { for (const Section& s : t.sections()) if (s.type == ty) { out = s; return true; } return false; }
// WFS-283 : carrefour simple qui remplace le rond-point (mêmes captures trace-rond-point*, à la même place)
bool premiereCarrefour(const Track& t, Section& out) { for (const Section& s : t.sections()) if (s.exRondPoint) { out = s; return true; } return false; }
vec3 vers3(const Pose& p) { return {(float)p.x, (float)p.y, (float)p.z}; }
// centre de courbure d'un segment (côté intérieur), en m
vec3 centre(Track& t, int64_t a) {
    const Pose& p = t.pose(a); const double c = t.segAt(a).curve, r = SEG_M / (std::abs(c) * CURVE_K), k = c > 0 ? 1 : -1;
    return {(float)(p.x + std::cos(p.th) * r * k), (float)p.y, (float)(p.z + std::sin(p.th) * r * k)};
}
} // namespace

int benchTrace(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    forcerTrace2(true);   // A7.3 : forme du tracé 2 sur les 4 cartes (le jeu normal n'utilise que CARTES_ACTIVES)
    double t = 3000;
    auto image = [&](int i) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 4 == 0) g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(i); };
    auto capture = [&](const char* nom) { g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s", nom); };
    // départ avancé (WFS_DEPART, lu par le décor de la carte) à 'avant' m d'un segment, voiture arrêtée
    auto aller = [&](const char* carte, int64_t seg, double avant) -> Track* {
        const int64_t d = std::max<int64_t>(0, seg - (int64_t)(avant / SEG_M));
        setenv("WFS_DEPART", std::to_string(d).c_str(), 1);
        courseBenchRules(true);
        const bool ok = g.init(r, "golf", carte);
        unsetenv("WFS_DEPART");
        if (!ok) { ecarts++; WFS_ERR("trace : chargement de %s impossible", carte); return nullptr; }
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
        g.allerA(Ecran::Course); images(60 * 4);   // décompte, voiture immobile
        static Track copie; copie.build(mapById(carte), VERSION);   // même tracé que la partie (construction déterministe)
        return &copie;
    };
    auto vueDessus = [&](vec3 c, float h, const char* nom) {
        g_vue.actif = true; g_vue.oeil = c + vec3{0, h, 0}; g_vue.cible = c; g_vue.haut = {0, 0, -1}; g_vue.fov = 50;
        capture(nom); g_vue.actif = false;
    };
    // ---- Ville : rond-point, feu rouge, autoroute
    if (Track* tv = aller("ville", 0, 0)) {
        Section rp, fx, au;
        const bool aRp = premiere(*tv, Type::RondPoint, rp) || premiereCarrefour(*tv, rp), aAu = premiere(*tv, Type::Autoroute, au);
        bool aFx = false; for (const Section& s : tv->sections()) if (s.feu == 0) { fx = s; aFx = true; }
        g.shutdown();
        if (aRp && (tv = aller("ville", rp.debut, 60))) {
            // centre de l'anneau : centre de courbure au milieu de la section
            const int64_t mi = (rp.debut + rp.fin) / 2;
            vueDessus(centre(*tv, mi), 115, "trace-rond-point");
            capture("trace-rond-point-sol");   // au sol, 60 m avant l'entrée, décor actuel
            g.shutdown();
        } else ecarts++;
        if (aFx && (tv = aller("ville", fx.ligneArret, 22))) {
            // WFS-172 (RG-03, ANO-89) : à l'orange, 3 voitures du trafic (loin du joueur) sont placées 23 à 38 m derrière lui,
            // dans les files de gauche et de droite, à 29 km/h : elles le dépassent et forment la file arrêtée à la ligne ;
            // capture au rouge (3,4 s dedans au moins : 1re voiture en travers au milieu de la chaussée) avec au moins
            // 2 voitures arrêtées avant la ligne
            const Traffic* tr = activeTraffic(); double tp = 0; int n = 0, arretees = 0, travers = 0;
            const double ligne = fx.ligneArret * SEG;
            auto compter = [&](bool journal) {
                arretees = travers = 0;
                if (tr) for (const TrafficCar& c : tr->cars) {
                    if (c.cache) continue;
                    const double d = tv->relZ(c.z - ligne) * M;
                    if (c.traverse >= 0) { travers++; if (journal) WFS_LOG("trace : voiture en travers à %.1f m de la ligne, %.1f m du centre (joueur à %.1f m de la ligne)", d, c.offset * RW, tv->relZ(g.player().position - ligne) * M); }
                    else if (d < 0 && d > -60 && c.speed * M < 0.5) arretees++;
                }
            };
            bool placees = false;
            while (tr && n++ < 120 * 60) {
                const Feu f = feuCarrefour(0, tr->sim, &tp);
                if (!placees && f == Feu::Orange) {
                    placees = true;
                    const double dJ = tv->relZ(g.player().position - ligne) * M;
                    const struct { int file; double m; } places[3] = {{0, dJ - 23}, {2, dJ - 28}, {0, dJ - 38}};
                    int k = 0;
                    for (TrafficCar& c : activeTrafficMut()->cars) {
                        if (k == 3) break;
                        if (c.traverse >= 0 || c.cache || c.crashed || std::abs(tv->relZ(c.z - g.player().position)) * M < 300) continue;
                        c.z = tv->wrap(ligne + places[k].m / M); c.lane = c.target = places[k].file;
                        c.offset = c.prevOff = LANE_X[places[k].file]; c.speed = 8 / M; c.think = 1e9; c.signal = 0; c.pending = false; c.laneYaw = 0;
                        c.prevDz = tv->relZ(c.z - (g.player().position + PLAYER_Z));
                        k++;
                    }
                    WFS_LOG("trace : orange : %d voiture(s) placée(s) derrière le joueur (%.1f m de la ligne)", k, dJ);
                }
                if (f == Feu::Rouge && tp >= 3.4) { compter(false); if (arretees >= 2 && travers > 0) break; }
                image(n);
            }
            compter(true);
            WFS_LOG("trace : feu rouge (%.1f s) : %d voiture(s) arrêtée(s) avant la ligne, %d en travers", tp, arretees, travers);
            if (travers == 0) { ecarts++; WFS_ERR("trace : aucune voiture ne traverse au rouge"); }
            if (arretees < 2) { ecarts++; WFS_ERR("trace : %d voiture(s) arrêtée(s) à la ligne, au moins 2 attendues", arretees); }
            capture("trace-feux-rouge");
            g.shutdown();
        } else ecarts++;
        if (aAu && (tv = aller("ville", au.debut, 80))) {
            // vue plongeante : 120 m au-dessus de l'entrée de l'autoroute, regard 600 m plus loin
            const vec3 a = vers3(tv->pose(au.debut)), b = vers3(tv->pose(au.debut + (int64_t)(600 / SEG_M)));
            const vec3 f = normalize(vec3{b.x - a.x, 0, b.z - a.z});
            g_vue.actif = true; g_vue.oeil = a - f * 120.f + vec3{0, 140, 0}; g_vue.cible = b; g_vue.haut = {0, 1, 0}; g_vue.fov = 55;
            capture("trace-autoroute"); g_vue.actif = false;
            g.shutdown();
        } else ecarts++;
    } else return ecarts + 1;
    // ---- Campagne : 1re épingle du col
    if (Track* tc = aller("campagne", 0, 0)) {
        Section l; const bool aL = premiere(*tc, Type::Lacets, l);
        int64_t ep = -1;
        if (aL) for (int64_t a = l.debut; a < l.fin; a++) if (std::abs(tc->segAt(a).curve) > 10) { ep = a; break; }
        g.shutdown();
        if (ep >= 0 && (tc = aller("campagne", ep, 50))) {
            int64_t mi = ep; while (mi < l.fin && std::abs(tc->segAt(mi).curve) > 10) mi++;
            mi = (ep + mi) / 2;
            vueDessus(centre(*tc, mi), 85, "trace-epingle");
            capture("trace-epingle-sol");
            g.shutdown();
        } else ecarts++;
    } else ecarts++;
    // ---- Corniche (1re courbe de la montée, WFS-379 : plus de lacets) et Grand Prix (épingle) : au sol, décor actuel
    for (const char* carte : {"plage", "circuit"}) {
        Track* tk = aller(carte, 0, 0);
        if (!tk) { ecarts++; continue; }
        int64_t ep = -1;
        const bool corniche = !std::strcmp(carte, "plage");
        for (const Section& s : tk->sections()) if (corniche ? (s.type == Type::Montee && s.rayon > 0) : (s.type == Type::Lacets || s.type == Type::Epingle)) {
            if (corniche) { ep = (s.debut + s.fin) / 2; break; }
            for (int64_t a = s.debut; a < s.fin; a++) if (std::abs(tk->segAt(a).curve) > 10) { ep = a; break; }
            break;
        }
        g.shutdown();
        if (ep >= 0 && aller(carte, ep, 50)) { capture((std::string("trace-epingle-sol-") + carte).c_str()); g.shutdown(); } else ecarts++;
    }
    forcerTrace2(false);
    // A7.6 : jeu normal : Ville au tracé 1, Grand Prix au tracé 2 ; course de 60 s en pilote automatique sur le Grand Prix
    WFS_LOG("trace : jeu normal : ville au tracé %d, plage %d, campagne %d, circuit %d", versionCarte("ville"), versionCarte("plage"), versionCarte("campagne"), versionCarte("circuit"));
    if (versionCarte("ville") != (carteActive("ville") ? 2 : 1) || versionCarte("circuit") != 2) ecarts++;   // WFS-137 : ville activée
    if (aller("circuit", 0, 0)) {
        g.modeManette = false; g.setAutopilot(true);
        images(60 * 60);
        const double km = g.player().dist * M / 1000;
        WFS_LOG("trace : Grand Prix (tracé 2), 60 s en pilote automatique : %.2f km, écran %s", km, g.ecran() == Ecran::Course ? "course" : "autre");
        if (km < 0.8) { ecarts++; WFS_ERR("trace : le Grand Prix n'avance pas (%.2f km)", km); }
        capture("trace-circuit-course");
        g.setAutopilot(false); g.shutdown();
    } else ecarts++;
    courseBenchRules(false);
    g.modeManette = false; g.manette = DriveInput{};
    WFS_LOG("trace : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::trace
