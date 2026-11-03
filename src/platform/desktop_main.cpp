// WaForSpeed — banc d'essai sur ordinateur (sans fenêtre) : rend des scènes de test hors écran
// et les enregistre en PNG (utilisé par l'intégration continue pour vérifier le rendu).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../audio/audio_system.h"
#include "../core/scene_essai.h"
#include "../game/game.h"
#include "../game/course_system.h"
#include "../game/traffic_system.h"
#include "../ui/hud.h"
#include "../render/bench_ombres.h"
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/chargement.h"
#include "../ui/ecrans/menu.h"


namespace wfs {
static std::string g_root = "assets/";
bool readAsset(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(g_root + path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
double nowSeconds() {
    using namespace std::chrono;
    static auto t0 = steady_clock::now();
    return duration<double>(steady_clock::now() - t0).count();
}
}

using namespace wfs;

int main(int argc, char** argv) {
    std::string out = argc > 1 ? argv[1] : "captures";
    if (argc > 2) g_root = argv[2];
    jobs().start();
    VkCtx vk;
    // taille des captures (WFS_TAILLE=2400x1080 pour tester un grand écran), 1280x720 par défaut
    unsigned cw = 1280, chh = 720;
    if (const char* t = std::getenv("WFS_TAILLE")) std::sscanf(t, "%ux%u", &cw, &chh);
    if (!vk.init(nullptr, cw, chh)) return 1;
    const double tVk = nowSeconds();
    Renderer r; r.init(vk);
    Game g;
    courseBenchRules(false);   // WFS-12 : scènes existantes sans décompte ni fin (benchCourseScene les remet)
    if (const char* b = std::getenv("WFS_BANC"); b && std::string(b) == "hud") { benchHud(g, r, out); r.shutdown(); vk.shutdown(); jobs().stop(); return 0; }   // WFS-27 : HUD seul (2400x1080)
    if (const char* b = std::getenv("WFS_BANC"); b && std::string(b) == "menu") { ecrans::benchMenu(g, r, out); r.shutdown(); vk.shutdown(); jobs().stop(); return 0; }   // WFS-37
    ecrans::benchChargement(g, r, out, tVk, [](bool ok) { static const std::string keep = g_root; g_root = ok ? keep : "absent/"; });   // WFS-35
    for (const char* id : {"golf", "m4", "p911", "gallardo"}) {
        if (!g.init(r, id, "ville")) { WFS_ERR("chargement %s impossible", id); return 2; }
        // vitrine : 3 angles
        g.camMode = 2;
        for (int k = 0; k < 3; k++) {
            g.orbit = 0.7f + k * 2.1f;
            g.render(0); g.render(0);
            r.capturePng(out + "/vitrine-" + id + "-" + std::to_string(k) + ".png");
        }
        g.shutdown();
    }
    // course : la Golf roule en pilote automatique, captures à différents moments
    g.init(r, "golf", std::getenv("WFS_CARTE") ? std::getenv("WFS_CARTE") : "ville");   // WFS-14 : WFS_CARTE=plage pour essayer une autre carte
    if (const char* e = std::getenv("WFS_ENCOCHE")) g.setMarges({(float)std::atoi(e), 0, 0, 0});   // WFS-51 : encoche simulée à gauche
    g.setAutopilot(true);
    for (int cam = 0; cam < 2; cam++) {
        g.camMode = cam;
        for (int shot = 0; shot < 3; shot++) {
            for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(shot * 10 + i / 60.0); }
            g.render(shot * 10 + 4.0); g.render(shot * 10 + 4.02);
            char name[128]; std::snprintf(name, sizeof name, "%s/course-cam%d-%d.png", out.c_str(), cam, shot);
            r.capturePng(name);
            WFS_LOG("capture %s : %.0f km/h, %.0f m", name, g.player().kmh(), g.player().dist * M);
        }
    }
    benchShadowScenes(g, r, out);    // WFS-07 : ombres du soleil (captures ombres-*.png)
    benchTrafficScenes(g, r, out);   // WFS-08 : trafic de près et choc (captures trafic-*.png)
    // WFS-36 : écran Menu forcé (sans menu dessiné) : scène sans commandes tactiles, voiture immobile
    g.allerA(Ecran::Menu); g.setAutopilot(false);
    for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 15 == 0) g.render(300 + i / 60.0); }
    g.render(301.5); r.capturePng(out + "/ecran-menu-vide.png");
    WFS_LOG("capture ecran-menu-vide : écran %s, %.1f km/h", nomEcran(g.ecran()), g.player().kmh());
    // WFS-44 : un système de test prend la scène (fond uni #07070c, cube, caméra FOV 36°)
    activerSceneEssai(true); g.render(310); g.render(310.02); r.capturePng(out + "/scene-dediee-vide.png"); activerSceneEssai(false);
    g.shutdown();
    // WFS-14 : carte Corniche (plage) de jour et de nuit, en pilote automatique, caméra haute
    for (const char* mo : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", mo, 1); g.init(r, "golf", "plage"); g.setAutopilot(true); g.camMode = 1;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(400 + i / 60.0); }
        g.render(404); g.render(404.02); r.capturePng(out + "/carte-plage-" + mo + ".png"); g.shutdown();
    }
    unsetenv("WFS_MOMENT");
    benchAudioScene(g, r, out);   // WFS-10 : son du moteur (moteur.wav, moteur-courbe.png)
    benchCourseScene(g, r, out);  // WFS-12 : décompte, checkpoints, fin de partie (course-*.png)
    benchHud(g, r, out);          // WFS-27 : HUD (hud-course, hud-temps-bas, hud-nitro, hud-combo)
    ecrans::benchMenu(g, r, out); // WFS-37 : menu principal (menu-anim, menu-0)
    r.shutdown();
    vk.shutdown();
    {   // WFS-51 : encoche de 90 px à gauche en 2400x1080 : bande marquée, volant et jauges décalés à droite de la bande
        VkCtx vk2; Renderer r2; Game g2;
        if (vk2.init(nullptr, 2400, 1080) && r2.init(vk2) && g2.init(r2, "golf", "ville")) {
            g2.setMarges({90, 0, 0, 0}); g2.montrerMarges = true; g2.setAutopilot(true);
            for (int i = 0; i < 120; i++) { g2.update(1.0 / 60); if (i % 10 == 0) g2.render(500 + i / 60.0); }
            g2.render(502); g2.render(502.02); r2.capturePng(out + "/encoche-course.png");
            WFS_LOG("capture encoche-course : marge gauche %.0f px, écran %ux%u", g2.marges().gauche, r2.width(), r2.height());
            g2.shutdown(); r2.shutdown(); vk2.shutdown();
        }
    }
    jobs().stop();
    return 0;
}
