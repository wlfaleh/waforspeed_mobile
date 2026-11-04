// WaForSpeed — banc d'essai des caméras (WFS-66) : voir camera_banc.h.
#include "camera_banc.h"
#include "../core/log.h"
#include "../render/renderer.h"
#include "camera_vue.h"
#include "course_system.h"
#include "game.h"

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
    rouler(60 * 9);                       // décompte (3 s) puis 6 s de course
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
    g.shutdown();
    courseBenchRules(false);
    choisirVue(avant);
}

} // namespace wfs
