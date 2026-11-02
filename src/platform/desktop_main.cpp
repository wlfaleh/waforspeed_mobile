// WaForSpeed — banc d'essai sur ordinateur (sans fenêtre) : rend des scènes de test hors écran
// et les enregistre en PNG (utilisé par l'intégration continue pour vérifier le rendu).
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/vk.h"


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
    if (!vk.init(nullptr, 1280, 720)) return 1;
    Renderer r; r.init(vk);
    Game g;
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
    g.init(r, "golf", "ville");
    g.setAutopilot(true);
    for (int cam = 0; cam < 2; cam++) {
        g.camMode = cam;
        for (int shot = 0; shot < 3; shot++) {
            for (int i = 0; i < 240; i++) g.update(1.0 / 60);
            g.render(shot); g.render(shot);
            char name[128]; std::snprintf(name, sizeof name, "%s/course-cam%d-%d.png", out.c_str(), cam, shot);
            r.capturePng(name);
            WFS_LOG("capture %s : %.0f km/h, %.0f m", name, g.player().kmh(), g.player().dist * M);
        }
    }
    g.shutdown();
    r.shutdown();
    vk.shutdown();
    jobs().stop();
    return 0;
}
