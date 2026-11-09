// WaForSpeed — banc des niveaux de détail au loin (WFS-265) : voir bench_lod.h.
// Vues au téléobjectif : œil 2 m derrière le joueur et 2,5 m au-dessus de la route, visée à D m devant, champ qui couvre
// ±12 m autour du point visé : on voit ce que la caméra de jeu dessine à cette distance (personnes et petits objets effacés
// selon leur distance à la caméra, lod_distance.h). Les voitures du trafic, choisies par leur taille à l'écran, sont montrées
// avec la caméra de poursuite normale (feux de nuit à 10, 25 et 50 m).
#include "bench_lod.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../core/math.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/camera_vue.h"     // WFS-335 : vues Arrière et Proche
#include "../game/course5.h"        // WFS-335 : Course à 5 de nuit
#include "../game/course_system.h"  // WFS-335 : règles de la course au banc
#include "../game/traffic_system.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

struct Vue { bool active = false; float devant = 50, fov = 30; };
Vue g_vue;

class LodBanc : public System {
public:
    const char* name() const override { return "lod-banc"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_vue.active || !g.track || !g.player) return;
        const double s = g.player->dist;
        auto point = [&](float a, float h) { const TrackPoint q = g.track->point(s + a / M, 0); return vec3{(float)q.x, (float)q.y + h, (float)q.z}; };
        const vec3 oeil = point(-2, 2.5f), cible = point(g_vue.devant, 1.2f);
        const float aspect = fp.proj.at(0, 0) != 0 ? -fp.proj.at(1, 1) / fp.proj.at(0, 0) : 16.f / 9.f;
        fp.view = lookAt(oeil, cible, {0, 1, 0});
        fp.proj = perspectiveVk(g_vue.fov * PI / 180.f, aspect, 0.1f);
        fp.camPos = oeil;
    }
};
WFS_SYSTEM(LodBanc, 975)   // après les ambiances (nuit 950) et le banc du réalisme (970)

void poserDevant(TrafficCar& c, double pos, double d, double vitesse) {   // voiture du trafic à d m devant le joueur, voie de droite
    c.crashed = false; c.speed = c.want = vitesse;
    c.offset = c.prevOff = c.offPrec = 0;
    c.z = c.zPrec = pos + d / M;
    c.yawPrec = c.laneYaw;
}
void rangerLoin(TrafficCar& c, double pos, size_t i, double vitesse) {
    c.crashed = false; c.speed = c.want = vitesse;
    c.z = c.zPrec = pos - (200 + 5.0 * (double)i) / M;
}

} // namespace

int benchLod(Game& g, Renderer& r, const std::string& out) {
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    int erreurs = 0;
    double t = 1200;
    auto rouler = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); } };
    auto capturer = [&](const std::string& nom) {
        g.render(t + 0.001); g.render(t + 0.002);
        if (!r.capturePng(out + "/" + nom + ".png")) erreurs++;
        WFS_LOG("capture %s (WFS-265) : %u appels de dessin, %u objets masqués", nom.c_str(), r.appelsDessin(), r.objetsMasques());
    };
    // 1) RG-01 : les 4 cartes, Haute et Basse, à 50, 150 et 300 m
    unsetenv("WFS_MOMENT");
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        for (int q : {qualite::HAUTE, qualite::BASSE}) {
            R.qualite = q; ecrans::appliquerQualiteNiveau(r);
            if (!g.init(r, "golf", carte)) { WFS_ERR("lod : chargement %s impossible", carte); erreurs++; continue; }
            g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 1;
            rouler(240);
            for (float d : {50.f, 150.f, 300.f}) {
                g_vue = {true, d, (float)(2 * std::atan(12.0 / d) * 180 / PI)};
                capturer(std::string("lod-") + carte + "-" + std::to_string((int)d) + (q == qualite::BASSE ? "-basse" : ""));
            }
            g_vue.active = false;
            g.shutdown();
        }
    }
    // 2) fondu d'effacement en Basse (Ville) : personnes de 75 à 90 m, petits objets de 95 à 110 m
    R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
    if (g.init(r, "golf", "ville")) {
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 1;
        rouler(240);
        g.setAutopilot(false);
        for (float d : {70.f, 80.f, 85.f, 90.f, 100.f, 105.f, 110.f, 115.f}) {
            g_vue = {true, d, (float)(2 * std::atan(12.0 / d) * 180 / PI)};
            capturer("lod-fondu-ville-" + std::to_string((int)d) + "-basse");
        }
        g_vue.active = false;
        g.shutdown();
    } else erreurs++;
    // 3) nuit, Basse : feux arrière d'une voiture du trafic à 10, 25 et 50 m (caméra de poursuite)
    setenv("WFS_MOMENT", "nuit", 1);
    if (g.init(r, "golf", "ville")) {
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 1;
        rouler(240);
        g.setAutopilot(false);
        for (double d : {10.0, 25.0, 50.0}) {
            if (Traffic* tr = activeTrafficMut()) {
                const double pos = g.player().position;
                for (size_t i = 0; i < tr->cars.size(); i++) { if (i == 0) poserDevant(tr->cars[i], pos, d, g.player().speed); else rangerLoin(tr->cars[i], pos, i, g.player().speed); }
            }
            capturer("lod-nuit-trafic-" + std::to_string((int)d) + "-basse");
        }
        g.shutdown();
    } else erreurs++;
    // 4) WFS-335 (RG-04) : nuit, Basse : voiture du trafic à 60 à 170 m (caméra de poursuite : passages du niveau 1 aux niveaux
    // 2 et 3 en 4 pièces, sans saut), puis Course à 5 après le « GO », vues Arrière et Proche (voitures proches inchangées)
    if (g.init(r, "golf", "ville")) {
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 1;
        rouler(240);
        g.setAutopilot(false);
        for (double d : {60.0, 80.0, 100.0, 130.0, 170.0}) {
            if (Traffic* tr = activeTrafficMut()) {
                const double pos = g.player().position;
                for (size_t i = 0; i < tr->cars.size(); i++) { if (i == 0) poserDevant(tr->cars[i], pos, d, g.player().speed); else rangerLoin(tr->cars[i], pos, i, g.player().speed); }
            }
            capturer("lod-nuit-trafic-loin-" + std::to_string((int)d) + "-basse");
        }
        // WFS-335 (revue du lead dev, PR #296) : la même voiture de 110 à 40 px à l'écran (passages niveau 1 → 2 à 100 px,
        // niveau 2 → 4 pièces à 48 px, fondus compris) ; distance ajustée par le relevé du renderer (longueur à l'écran réelle)
        double d = 18;
        for (int cible : {110, 100, 95, 90, 80, 70, 60, 50, 48, 45, 40}) {
            Renderer::ReleveVoiture vu{};
            for (int essai = 0; essai < 4; essai++) {
                if (Traffic* tr = activeTrafficMut()) {
                    const double pos = g.player().position;
                    for (size_t i = 0; i < tr->cars.size(); i++) { if (i == 0) poserDevant(tr->cars[i], pos, d, g.player().speed); else rangerLoin(tr->cars[i], pos, i, g.player().speed); }
                }
                r.releverVoitures = true; g.render(t + 0.001); r.releverVoitures = false;
                vu = {};
                for (const Renderer::ReleveVoiture& v : r.releveVoitures()) if (!v.joueur && v.px > vu.px) vu = v;
                if (vu.px <= 0 || std::fabs(vu.px - cible) < 0.5f) break;
                d *= vu.px / cible;
            }
            capturer("lod-passage-" + std::to_string(cible) + "px-basse");
            WFS_LOG("lod-passage (WFS-335) : cible %d px, mesuré %.1f px à %.1f m, niveau %d, %u triangles", cible, vu.px, vu.distance, vu.niveau, vu.triangles);
        }
        g.shutdown();
    } else erreurs++;
    {
        const VueCamera vue0 = vueCamera(); const bool regles0 = courseBenchRulesOn();
        modeCourse() = ModeCourse::Course5; kmCourse5() = 2; courseBenchRules(true);
        if (g.init(r, "golf", "ville")) {
            g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
            choisirVue(VueCamera::Poursuite);
            rouler(390);   // décompte, « GO » (5,5 s), puis 1 s de course : adversaires proches
            capturer("lod-course5-nuit-arriere-basse");
            choisirVue(VueCamera::Proche); rouler(30);
            capturer("lod-course5-nuit-proche-basse");
            rouler(600);   // 10 s plus loin : adversaires étalés devant (lointains)
            capturer("lod-course5-nuit-proche-loin-basse");
            g.shutdown();
        } else erreurs++;
        modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4; courseBenchRules(regles0); choisirVue(vue0);
    }
    unsetenv("WFS_MOMENT");
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    WFS_LOG("lod (WFS-265) : %d erreur(s)", erreurs);
    return erreurs;
}

} // namespace wfs
