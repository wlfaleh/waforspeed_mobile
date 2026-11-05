// WaForSpeed — banc d'essai des caméras (WFS-66) : voir camera_banc.h.
#include "camera_banc.h"
#include <algorithm>
#include <cstdio>
#include <vector>
#include "../core/log.h"
#include "../render/renderer.h"
#include "camera_vue.h"
#include "course_system.h"
#include "game.h"
#include "traffic_system.h"   // WFS-105 : voitures placées devant pour camera-poursuite-trafic

namespace wfs {

void benchCameras(Game& g, Renderer& r, const std::string& out) {
    const VueCamera avant = vueCamera();
    choisirVue(VueCamera::Poursuite);
    courseBenchRules(true);   // vraie course : HUD et bouton de vue affichés
    if (!g.init(r, "golf", "ville")) { WFS_ERR("caméras : chargement impossible"); courseBenchRules(false); return; }
    const float W = (float)r.width(), H = (float)r.height();
    double t = 700;
    auto rouler = [&](int images) { for (int i = 0; i < images; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); } };
    auto capture = [&](const char* nom) {
        g.render(t); g.render(t + 1.0 / 60); t += 1.0 / 60;
        r.capturePng(out + "/" + nom + ".png");
        const EtatCockpit& ck = etatCockpit();
        WFS_LOG("capture %s : vue %s, champ de vision %.1f°, %.0f km/h, braquage %+.2f, volant %+.2f rad, aiguille %+.2f rad", nom,
                VUES[(int)vueCamera()].nom, ck.fov, g.player().kmh(), g.player().steer, ck.actif ? ck.volant : 0.f,
                ck.actif ? ck.aiguille : 0.f);
    };
    // appui bref sur le bouton de vue (RG-01), comme un joueur
    auto appuyer = [&](int id) {
        float x, y;
        if (!boutonVue(x, y)) { WFS_ERR("caméras : bouton de vue absent"); return; }
        g.onTouch({id, x, y, true}); g.render(t); g.onTouch({id, x, y, false});
        WFS_LOG("caméras : appui sur le bouton (%.0f, %.0f) → vue %s", x, y, VUES[(int)vueCamera()].nom);
    };
    g.setAutopilot(true); g.camMode = 0;
    // WFS-105 : à l'arrêt (décompte), voiture entière au-dessus du compteur, en Poursuite puis en Proche
    rouler(90);
    capture("camera-poursuite-arret");
    choisirVue(VueCamera::Proche);
    rouler(1);
    capture("camera-proche-arret");
    choisirVue(VueCamera::Poursuite);
    rouler(60 * 9 - 91);                  // décompte (3 s) puis 6 s de course, comme avant WFS-105
    capture("camera-poursuite");
    appuyer(19);                          // WFS-85 : Poursuite (haute) → Proche (basse) → Capot → Cockpit
    rouler(30);
    capture("camera-proche");
    appuyer(20);
    rouler(30);
    capture("camera-capot");
    appuyer(21);
    rouler(12);
    capture("camera-cockpit-droit");   // même vue en ligne droite : volant au neutre, pour comparer
    // virage : gaz et doigt glissé vers la droite dans la zone de braquage (braquage ≈ 0,75), volant braqué visible
    g.setAutopilot(false);
    g.onTouch({22, W * 0.89f, H * 0.74f, true});
    g.onTouch({23, W * 0.20f, H * 0.70f, true}); g.onTouch({23, W * 0.20f + W * 0.09f, H * 0.70f, true});
    rouler(30);
    capture("camera-cockpit");
    g.onTouch({23, W * 0.29f, H * 0.70f, false}); g.onTouch({22, W * 0.89f, H * 0.74f, false});
    rouler(6);
    appuyer(24);   // le cycle revient à la poursuite
    // WFS-105 (RG-03) : Poursuite dans le trafic : 6 voitures placées de 16 à 68 m devant, sur les 3 files, un peu plus
    // lentes que le joueur ; au moins 4 doivent se voir, avec la route au loin
    g.setAutopilot(true);
    rouler(90);
    if (Traffic* tr = activeTrafficMut()) {
        const Player& pl = g.player();
        std::vector<int> idx;
        for (size_t i = 0; i < tr->cars.size(); i++) if (!tr->cars[i].crashed) idx.push_back((int)i);
        auto devant = [&](int i) { const double d = tr->cars[(size_t)i].prevDz; return d < 0 ? d + 1e9 : d; };   // les plus proches devant d'abord
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return devant(a) < devant(b); });
        const double DISTANCE_M[6] = {16, 23, 30, 42, 55, 68};
        const int VOIE[6] = {1, 0, 2, 1, 0, 2};
        for (size_t k = 0; k < idx.size(); k++) {
            TrafficCar& c = tr->cars[(size_t)idx[k]];
            if (k >= 6) {   // les autres voitures proches passent loin derrière (hors champ) : aucune voiture superposée
                if (c.prevDz * M > -12 && c.prevDz * M < 90) { c.z = pl.position - (150 + 10.0 * k) / M; c.prevDz = c.z - pl.position; }
                continue;
            }
            c.z = pl.position + PLAYER_Z + DISTANCE_M[k] / M; c.prevDz = DISTANCE_M[k] / M;   // z ramené sur la piste par Traffic::step
            c.lane = c.target = VOIE[k]; c.offset = c.prevOff = LANE_X[VOIE[k]]; c.laneYaw = 0; c.signal = 0; c.pending = false;
            c.speed = c.want = pl.speed * 0.92; c.think = 6;   // pas de changement de file pendant la capture (comme WFS-101)
        }
        rouler(30);
        capture("camera-poursuite-trafic");
        int n = 0; std::string liste;
        for (const TrafficCar& c : tr->cars) {
            const double dz = c.prevDz * M;
            if (dz <= 0 || dz >= 100) continue;
            char b[48]; std::snprintf(b, sizeof b, "%s%.1f m (file %+.2f)", n ? ", " : "", dz, c.offset);
            liste += b; n++;
        }
        WFS_LOG("camera-poursuite-trafic : %d voiture(s) entre 0 et 100 m devant : %s", n, liste.c_str());
    } else {
        WFS_ERR("caméras : trafic inactif, pas de capture camera-poursuite-trafic");
    }
    g.shutdown();
    courseBenchRules(false);
    choisirVue(avant);
}

} // namespace wfs
