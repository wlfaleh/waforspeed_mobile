// WaForSpeed — point d'entrée Android (NativeActivity) : cycle de vie de la fenêtre, tactile, boucle de jeu.
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <time.h>
#include <string>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/vk.h"

namespace wfs {
static AAssetManager* g_assets = nullptr;
bool readAsset(const std::string& path, std::vector<uint8_t>& out) {
    if (!g_assets) return false;
    AAsset* a = AAssetManager_open(g_assets, path.c_str(), AASSET_MODE_BUFFER);
    if (!a) return false;
    size_t n = (size_t)AAsset_getLength(a);
    out.resize(n);
    int rd = AAsset_read(a, out.data(), n);
    AAsset_close(a);
    return rd == (int)n;
}
double nowSeconds() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
}

using namespace wfs;

struct App {
    VkCtx vk; Renderer renderer; Game game;
    bool ready = false, focused = false;
    double last = 0, start = 0;
    int checkFrames = 0, orientRetries = 0;
};

static void initGraphics(App& app, android_app* a) {
    if (app.ready || !a->window) return;
    if (!app.vk.init(a->window)) { WFS_ERR("Initialisation Vulkan impossible"); return; }
    app.renderer.init(app.vk);
    app.game.init(app.renderer, "golf", "ville");
    app.renderer.setViewSize((uint32_t)ANativeWindow_getWidth(a->window), (uint32_t)ANativeWindow_getHeight(a->window));
    app.ready = true;
    app.last = nowSeconds();
}

static void termGraphics(App& app) {
    if (!app.ready) return;
    app.game.shutdown();
    app.renderer.shutdown();
    app.vk.shutdown();
    app.ready = false;
}

static void onCmd(android_app* a, int32_t cmd) {
    App& app = *static_cast<App*>(a->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: initGraphics(app, a); break;
        case APP_CMD_TERM_WINDOW: termGraphics(app); break;
        case APP_CMD_GAINED_FOCUS: app.focused = true; app.last = nowSeconds(); break;
        case APP_CMD_LOST_FOCUS: app.focused = false; break;
        case APP_CMD_CONFIG_CHANGED:
        case APP_CMD_WINDOW_RESIZED:
            if (app.ready) { app.vk.waitIdle(); if (app.vk.createSwapchain()) app.renderer.resize(); }
            break;
        default: break;
    }
}

static int32_t onInput(android_app* a, AInputEvent* e) {
    App& app = *static_cast<App*>(a->userData);
    if (!app.ready || AInputEvent_getType(e) != AINPUT_EVENT_TYPE_MOTION) return 0;
    int32_t action = AMotionEvent_getAction(e), kind = action & AMOTION_EVENT_ACTION_MASK;
    size_t idx = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    // coordonnées de l'écran (orientation actuelle) → coordonnées logiques du rendu
    // l'interface est exprimée en pixels de la fenêtre (Renderer::setViewSize) : pas de conversion
    const float sx = 1.f, sy = 1.f;
    auto send = [&](size_t i, bool down) {
        app.game.onTouch({AMotionEvent_getPointerId(e, i), AMotionEvent_getX(e, i) * sx, AMotionEvent_getY(e, i) * sy, down});
    };
    switch (kind) {
        case AMOTION_EVENT_ACTION_DOWN: case AMOTION_EVENT_ACTION_POINTER_DOWN: send(idx, true); break;
        case AMOTION_EVENT_ACTION_MOVE: for (size_t i = 0; i < AMotionEvent_getPointerCount(e); i++) send(i, true); break;
        case AMOTION_EVENT_ACTION_UP: case AMOTION_EVENT_ACTION_POINTER_UP: send(idx, false); break;
        case AMOTION_EVENT_ACTION_CANCEL: for (size_t i = 0; i < AMotionEvent_getPointerCount(e); i++) send(i, false); break;
        default: break;
    }
    return 1;
}

void android_main(android_app* a) {
    static App app;
    a->userData = &app;
    a->onAppCmd = onCmd;
    a->onInputEvent = onInput;
    g_assets = a->activity->assetManager;
    jobs().start();
    app.start = nowSeconds();
    while (!a->destroyRequested) {
        int events; android_poll_source* src;
        // en pause : on attend les événements sans consommer de batterie
        while (ALooper_pollOnce((app.ready && app.focused) ? 0 : -1, nullptr, &events, (void**)&src) >= 0) {
            if (src) src->process(a, src);
            if (a->destroyRequested) break;
        }
        if (a->destroyRequested) break;
        if (app.ready && app.focused) {
            // l'appli démarre souvent en portrait puis passe en paysage : la nouvelle taille et la rotation
            // n'arrivent pas toujours avec l'événement de changement ; on les vérifie régulièrement.
            if (++app.checkFrames >= 15) {
                app.checkFrames = 0;
                const bool winLandscape = ANativeWindow_getWidth(a->window) >= ANativeWindow_getHeight(a->window);
                const bool logicalLandscape = app.renderer.width() >= app.renderer.height();
                if (app.vk.surfaceChanged() || (winLandscape != logicalLandscape && app.orientRetries < 8)) {
                    if (winLandscape != logicalLandscape) app.orientRetries++; else app.orientRetries = 0;
                    app.vk.waitIdle();
                    if (app.vk.createSwapchain()) app.renderer.resize();
                    WFS_LOG("Écran : %ux%u logique", app.renderer.width(), app.renderer.height());
                }
            }
            app.renderer.setViewSize((uint32_t)ANativeWindow_getWidth(a->window), (uint32_t)ANativeWindow_getHeight(a->window));
            double t = nowSeconds(), dt = t - app.last; app.last = t;
            app.game.update(dt);
            app.game.render(t - app.start);
        }
    }
    termGraphics(app);
    jobs().stop();
}
