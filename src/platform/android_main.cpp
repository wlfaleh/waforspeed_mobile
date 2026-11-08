// WaForSpeed — point d'entrée Android (NativeActivity) : cycle de vie de la fenêtre, tactile, boucle de jeu.
#include <android/asset_manager.h>
#include <jni.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <chrono>
#include <mutex>
#include <thread>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <time.h>
#include <string>
#include <vector>
#include "../core/memoire.h"   // WFS-297 (RG-06)
#include "../core/gpu_logiciel.h"
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/configgame.h"   // WFS-128
#include "../game/game.h"
#include "../game/sauvegarde.h"   // WFS-24
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/cgu.h"   // WFS-308 : conditions d'utilisation au premier lancement
#include "../ui/ecrans/chargement.h"
#include "vibration_android.cpp"   // WFS-70 : vibration du téléphone (JNI Vibrator), compilée avec ce fichier
#include "reseau_android.cpp"     // WFS-83 : verrou de diffusion du Wi-Fi (duel sur le réseau local)
#include "manette_jeu.h"   // WFS-22 : manettes
#include "retour_systeme.h"   // WFS-116 : bouton / geste Retour du téléphone
#include "gyroscope_android.cpp"   // WFS-108 : inclinaison du téléphone (gyroscope), densité de l'écran (dp)
#include "chauffe_android.cpp"     // WFS-123 : état thermique, setFrameRate 60 (API 30+, dlsym)
#include "plantage_android.cpp"    // WFS-126 : rapport de plantage (signaux, 200 dernières lignes, déclencheur de test)
#include "../render/qualite_auto.h"   // WFS-246 (accroche, RG-06) : dossier de rendu-mesure.txt
#include "../render/cache_pipelines.h"   // WFS-262 (accroche, RG-01)

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
    memoire::compteurs().octetsLus += (int64_t)n;   // WFS-297 (RG-06)
    return rd == (int)n;
}
double nowSeconds() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
}

using namespace wfs;

// marges de sécurité de l'écran (WFS-51) : écrites par JeuActivity (thread de l'interface Android), lues par la boucle
static std::atomic<int> g_marges[4] = {0, 0, 0, 0};
static std::atomic<bool> g_margesNeuves{false};
extern "C" JNIEXPORT void JNICALL Java_com_waforspeed_game_JeuActivity_nativeMarges(JNIEnv*, jclass, jint g, jint h, jint d, jint b) {
    g_marges[0] = g; g_marges[1] = h; g_marges[2] = d; g_marges[3] = b; g_margesNeuves = true;
    WFS_LOG("Marges de l'écran : gauche %d, haut %d, droite %d, bas %d", g, h, d, b);
}

struct App {
    VkCtx vk; Renderer renderer; Game game;
    bool ready = false, focused = false;
    double last = 0, start = 0;
    int checkFrames = 0, orientRetries = 0;
    int fpsFrames = 0; double fpsT0 = 0;
};

// ---------------------------------------------------------------------------------------------------------------------
// WFS-284 (ANO-153) : veilleur des entrées. Android affiche « … isn't responding » (ANR) si un appui reste plus de 5 s
// dans la file des entrées sans être rendu. Le fil du jeu ne lit cette file qu'entre deux images ; or un chargement le
// bloque longtemps (émulateur : étape « Construction de la route » 47 s, sans aucun rappel de progression), et une image
// du GPU logiciel peut durer 3 à 10 s. Rappeler la progression plus souvent demanderait des appels dans tout le code de
// chargement (hors zone) sans garantie : un petit fil lit donc la file à la place du fil du jeu quand celui-ci est occupé :
//  - pendant tout chargement (premier chargement, changement de carte, « Réessayer ») : toutes les 50 ms ; les appuis et
//    la touche Retour sont pris et ignorés (RG-01, RG-03 : rien n'est rejoué ensuite), les autres touches (volume…)
//    restent à Android ; à la fin, ligne « chargement : entrées lues, plus long écart N ms » (RG-05) ;
//  - hors chargement, seulement si le fil du jeu n'a pas lu la file depuis 1 s (jamais à 30-60 images/s) : appuis et
//    Retour gardés dans un tampon et rejoués par le fil du jeu au début de l'image suivante, dans l'ordre (manette : laissée).
// La lecture de la file est exclusive (verrou g_file, pris aussi par le fil du jeu autour de process_input) et protégée
// par a->mutex, que la colle Android prend quand elle change de file (APP_CMD_INPUT_CHANGED).
namespace veilleur {
struct Evenement { bool retour; int32_t id; float x, y; bool down; int repetition; };
static std::mutex g_file, g_tamponVerrou;
static std::vector<Evenement> g_tampon;                 // lu par le veilleur hors chargement, rejoué par le fil du jeu
static std::atomic<bool> g_chargement{false}, g_arret{false}, g_enAttente{false};
static std::atomic<double> g_dernierLu{0};              // dernière lecture de la file (fil du jeu ou veilleur)
static double g_lectureChargement = 0, g_plusLongEcart = 0; static int g_ignores = 0;   // sous g_file
static std::thread g_fil;

// un événement lu par le veilleur : vrai = gardé (rendu à Android comme traité)
static bool traiter(AInputEvent* e) {
    const int32_t type = AInputEvent_getType(e);
    if (type == AINPUT_EVENT_TYPE_KEY) {
        if (AKeyEvent_getKeyCode(e) != AKEYCODE_BACK) return false;   // volume, manette… : à Android
        const int32_t act = AKeyEvent_getAction(e);
        if (g_chargement) { g_ignores++; return true; }               // RG-01 : Retour pris et ignoré
        if (act == AKEY_EVENT_ACTION_DOWN || act == AKEY_EVENT_ACTION_UP) {
            std::lock_guard<std::mutex> l(g_tamponVerrou);
            g_tampon.push_back({true, 0, 0, 0, act == AKEY_EVENT_ACTION_DOWN, AKeyEvent_getRepeatCount(e)});
        }
        return true;
    }
    if (type != AINPUT_EVENT_TYPE_MOTION) return false;
    if (g_chargement) { g_ignores++; return true; }                   // RG-01, RG-03 : appui ignoré, jamais rejoué
    if ((AInputEvent_getSource(e) & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) return false;   // manette : à Android
    const int32_t action = AMotionEvent_getAction(e), kind = action & AMOTION_EVENT_ACTION_MASK;
    const size_t idx = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    std::lock_guard<std::mutex> l(g_tamponVerrou);
    auto garder = [&](size_t i, bool down) { g_tampon.push_back({false, AMotionEvent_getPointerId(e, i), AMotionEvent_getX(e, i), AMotionEvent_getY(e, i), down, 0}); };
    switch (kind) {   // même découpage que onInput
        case AMOTION_EVENT_ACTION_DOWN: case AMOTION_EVENT_ACTION_POINTER_DOWN: garder(idx, true); break;
        case AMOTION_EVENT_ACTION_MOVE: for (size_t i = 0; i < AMotionEvent_getPointerCount(e); i++) garder(i, true); break;
        case AMOTION_EVENT_ACTION_UP: case AMOTION_EVENT_ACTION_POINTER_UP: garder(idx, false); break;
        case AMOTION_EVENT_ACTION_CANCEL: for (size_t i = 0; i < AMotionEvent_getPointerCount(e); i++) garder(i, false); break;
        default: break;
    }
    return true;
}

// lit toute la file (g_file tenu par l'appelant), comme process_input de la colle Android
static void lire(android_app* a, double t) {
    pthread_mutex_lock(&a->mutex);
    if (AInputQueue* q = a->inputQueue) {
        AInputEvent* e = nullptr;
        while (AInputQueue_getEvent(q, &e) >= 0) {
            if (AInputQueue_preDispatchEvent(q, e)) continue;   // clavier à l'écran : l'événement reviendra
            AInputQueue_finishEvent(q, e, traiter(e) ? 1 : 0);
        }
    }
    pthread_mutex_unlock(&a->mutex);
    if (g_chargement) { g_plusLongEcart = std::max(g_plusLongEcart, t - g_lectureChargement); g_lectureChargement = t; }
    g_dernierLu = t;
}

static void demarrer(android_app* a) {
    g_arret = false; g_dernierLu = nowSeconds();   // android_main peut repartir dans le même processus
    g_fil = std::thread([a] {
        while (!g_arret) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            const double t = nowSeconds();
            if (!g_chargement && (g_enAttente || t - g_dernierLu < 1.0)) continue;   // le fil du jeu lit lui-même
            std::lock_guard<std::mutex> l(g_file);
            lire(a, t);
        }
    });
}
static void arreter() { g_arret = true; if (g_fil.joinable()) g_fil.join(); }

// début / fin d'un chargement (fil du jeu) ; fin : dernière lecture (appuis du chargement ignorés), puis ligne RG-05
static void debutChargement() {
    std::lock_guard<std::mutex> l(g_file);
    g_lectureChargement = nowSeconds(); g_plusLongEcart = 0; g_ignores = 0; g_chargement = true;
}
static void finChargement(android_app* a) {
    std::lock_guard<std::mutex> l(g_file);
    if (!g_chargement) return;
    lire(a, nowSeconds());
    g_chargement = false;
    WFS_LOG("chargement : entrées lues, plus long écart %d ms (%d événements ignorés)", (int)std::lround(g_plusLongEcart * 1000), g_ignores);
}
} // namespace veilleur

static void chargerJeu(App& app, android_app* a, double tVk);
static void initGraphics(App& app, android_app* a) {
    if (app.ready || !a->window || ecrans::cgu().ouvert()) return;
    veilleur::debutChargement();   // WFS-284 : entrées lues par le veilleur jusqu'à la fin, même en cas d'échec
    struct Fin { android_app* a; ~Fin() { veilleur::finChargement(a); } } fin{a};
    if (!app.vk.init(a->window)) { WFS_ERR("Initialisation Vulkan impossible"); return; }
    const double tVk = nowSeconds();
    app.renderer.init(app.vk);
    densiteAndroid(a->config, (float)app.renderer.height(), (float)ANativeWindow_getHeight(a->window));   // WFS-147 : dp dès l'écran de chargement
    if (ecrans::cgu().demarrer(app.renderer)) return;   // WFS-308 (RG-03) : CGU d'abord, chargement après l'acceptation
    chargerJeu(app, a, tVk);
}
static void chargerJeu(App& app, android_app* a, double tVk) {   // WFS-308 : chargement, après les CGU acceptées
    veilleur::debutChargement(); struct Fin { android_app* a; ~Fin() { veilleur::finChargement(a); } } fin{a};   // WFS-284, aussi après les CGU
    ecrans::chargement().debut(app.renderer, tVk);   // WFS-35 : écran de chargement, puis fondu ; échec : message rouge
    if (!app.game.init(app.renderer, sauvegarde().car.c_str(), sauvegarde().map.c_str(), ecrans::Chargement::progression)) { ecrans::chargement().erreur(true); return; }   // WFS-24 : voiture et carte gardées ; WFS-146 : « Quitter »
    ecrans::chargement().fin();
    app.game.allerA(Ecran::Menu);   // WFS-38 : l'application s'ouvre sur le menu après le chargement
    // WFS-74 : écran de chargement de WFS-35 au changement de carte (« Rouler »), étapes réelles, message rouge en cas d'échec
    Game::demandes().debutChargement = [&app] { veilleur::debutChargement(); ecrans::chargement().debut(app.renderer, nowSeconds()); };
    Game::demandes().progression = ecrans::Chargement::progression;
    Game::demandes().finChargement = [a](bool ok) { if (ok) ecrans::chargement().fin(); else ecrans::chargement().erreur(); veilleur::finChargement(a); };
    app.renderer.setViewSize((uint32_t)ANativeWindow_getWidth(a->window), (uint32_t)ANativeWindow_getHeight(a->window));
    app.ready = true;
    app.last = nowSeconds();
    WFS_LOG("QA pret : moteur initialise");
}

static void termGraphics(App& app) {
    if (!app.ready && ecrans::cgu().ouvert()) { ecrans::cgu().perdreFenetre(app.renderer); app.renderer.shutdown(); app.vk.shutdown(); return; }   // WFS-308
    if (!app.ready) return;
    app.game.shutdown();
    app.renderer.shutdown();
    // ANO-52 : Renderer::shutdown remet l'objet à neuf depuis WFS-100 (RG-W1) ; contournement retiré (WFS-111 RG-10)
    app.vk.shutdown();
    app.ready = false;
}

static void onCmd(android_app* a, int32_t cmd) {
    App& app = *static_cast<App*>(a->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: chauffeAndroidFenetre(a->window); initGraphics(app, a); break;   // WFS-123 (RG-04)
        case APP_CMD_TERM_WINDOW: termGraphics(app); break;
        case APP_CMD_GAINED_FOCUS: app.focused = true; app.last = nowSeconds(); break;
        case APP_CMD_PAUSE:   // WFS-39 RG-04 : arrière-plan ou perte du focus en course → pause (web blur / visibilitychange)
            sauvegarde().enregistrer();   // WFS-24 RG-06 : sauvegarde au passage en arrière-plan
            if (app.ready) app.renderer.enregistrerCachePipelines();   // WFS-262 (accroche, RG-01) : cache de pipelines
            [[fallthrough]];
        case APP_CMD_LOST_FOCUS: if (app.ready && app.game.ecran() == Ecran::Course) app.game.allerA(Ecran::Pause);
            if (cmd == APP_CMD_LOST_FOCUS) app.focused = false;
            break;
        case APP_CMD_CONFIG_CHANGED:
        case APP_CMD_WINDOW_RESIZED:
            if (app.ready) { app.vk.waitIdle(); if (app.vk.createSwapchain()) app.renderer.resize(); }
            break;
        default: break;
    }
}

static int32_t onInput(android_app* a, AInputEvent* e) {
    App& app = *static_cast<App*>(a->userData);
    if (app.ready && manette::evenementAndroid(e)) return 1;   // WFS-22 : manette (un toucher rend la main au tactile)
    if (AInputEvent_getType(e) == AINPUT_EVENT_TYPE_KEY && AKeyEvent_getKeyCode(e) == AKEYCODE_BACK) {   // WFS-116 : Retour du téléphone
        const int32_t act = AKeyEvent_getAction(e);
        if (act != AKEY_EVENT_ACTION_DOWN && act != AKEY_EVENT_ACTION_UP) return 1;
        if (!app.ready) return 1;   // chargement : pris et ignoré (RG-03)
        return retourSysteme(app.game, act == AKEY_EVENT_ACTION_DOWN, AKeyEvent_getRepeatCount(e)) ? 1 : 0;
    }
    if ((ecrans::chargement().enErreur() || (ecrans::cgu().ouvert() && !app.ready)) && AInputEvent_getType(e) == AINPUT_EVENT_TYPE_MOTION) {   // WFS-146 (RG-10) : bouton de l'écran d'erreur ; WFS-308 : CGU
        const int32_t k = AMotionEvent_getAction(e) & AMOTION_EVENT_ACTION_MASK;
        if (ecrans::cgu().ouvert() && !app.ready) return ecrans::cgu().toucher(AMotionEvent_getPointerId(e, 0), AMotionEvent_getX(e, 0), AMotionEvent_getY(e, 0), k != AMOTION_EVENT_ACTION_UP && k != AMOTION_EVENT_ACTION_CANCEL) ? 1 : 0;
        return ecrans::chargement().toucher(AMotionEvent_getPointerId(e, 0), AMotionEvent_getX(e, 0), AMotionEvent_getY(e, 0), k != AMOTION_EVENT_ACTION_UP && k != AMOTION_EVENT_ACTION_CANCEL) ? 1 : 0;
    }
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

// WFS-284 : appuis et Retour lus par le veilleur pendant une image trop longue, rejoués dans l'ordre (mêmes chemins que onInput)
static void rejouerEntrees(App& app) {
    std::vector<veilleur::Evenement> evs;
    { std::lock_guard<std::mutex> l(veilleur::g_tamponVerrou); evs.swap(veilleur::g_tampon); }
    for (const auto& ev : evs) {
        if (ev.retour) { if (app.ready) retourSysteme(app.game, ev.down, ev.repetition); }   // « laissé à Android » : sans effet
        else if (ecrans::chargement().enErreur()) ecrans::chargement().toucher(ev.id, ev.x, ev.y, ev.down);
        else if (app.ready) app.game.onTouch({ev.id, ev.x, ev.y, ev.down});
    }
}

void android_main(android_app* a) {
    static App app;
    a->userData = &app;
    a->onAppCmd = onCmd;
    a->onInputEvent = onInput;
    g_assets = a->activity->assetManager;
    // WFS-126 (RG-01) : gestionnaire des signaux installé en premier (dossier interne de l'appli : plantage.txt)
    plantageAndroidDemarrer(a->activity->internalDataPath ? a->activity->internalDataPath : ".");
    // WFS-59 : extra d'intent qa_simu=true (émulateur de la QA) → mode GPU logiciel, lu avant l'initialisation de Vulkan
    if (lireExtraQaSimu(a->activity->vm, a->activity->clazz)) activerModeGpuLogiciel("intent qa_simu=true");
    jobs().start();
    installerVibrationAndroid(a->activity->vm, a->activity->clazz);   // WFS-70 : chocs → vibration
    installerReseauAndroid(a->activity->vm, a->activity->clazz);      // WFS-83 : verrou de diffusion du salon
    gyroscopeAndroidDemarrer(a->looper);                               // WFS-108 : capteur d'inclinaison
    chauffeAndroidDemarrer();                                          // WFS-123 : état thermique (API 30+)
    // WFS-24 RG-08 : fichier privé de l'application ; moment gardé (lu par la plage, choix à l'écran : WFS-41)
    sauvegarde().charger(std::string(a->activity->internalDataPath ? a->activity->internalDataPath : ".") + "/sauvegarde.txt");
    configCheminTelephone(a->activity->externalDataPath ? a->activity->externalDataPath : "");   // WFS-128 : configgame.json posé sur le téléphone
    qualite::dossierMesure = a->activity->externalDataPath ? a->activity->externalDataPath : "";   // WFS-246 (accroche, RG-06) : rendu-mesure.txt
    cache_pipelines::dossier = a->activity->internalDataPath ? a->activity->internalDataPath : "";   // WFS-262 (accroche, RG-01) : pipelines.cache
    setenv("WFS_MOMENT", sauvegarde().tod.c_str(), 1);
    app.start = nowSeconds();
    veilleur::demarrer(a);   // WFS-284
    while (!a->destroyRequested) {
        int events; android_poll_source* src;
        rejouerEntrees(app);   // WFS-284 : avant les événements suivants (ordre gardé)
        // en pause : on attend les événements sans consommer de batterie
        veilleur::g_enAttente = true;
        while (ALooper_pollOnce((app.ready && app.focused) || ecrans::cgu().ouvert() ? 0 : -1, nullptr, &events, (void**)&src) >= 0) {
            veilleur::g_enAttente = false;
            if (src && src->id == LOOPER_ID_INPUT) { std::lock_guard<std::mutex> l(veilleur::g_file); src->process(a, src); veilleur::g_dernierLu = nowSeconds(); }
            else if (src) src->process(a, src);   // commandes (chargement compris : le veilleur lit la file pendant ce temps)
            if (a->destroyRequested) break;
            veilleur::g_enAttente = true;
        }
        veilleur::g_enAttente = false; veilleur::g_dernierLu = nowSeconds();
        if (a->destroyRequested) break;
        if (!app.ready && a->window) switch (ecrans::cgu().image(app.vk, ANativeWindow_getWidth(a->window), ANativeWindow_getHeight(a->window), nowSeconds())) {   // WFS-308
            case ecrans::Cgu::Etat::Quitter: ANativeActivity_finish(a->activity); break;   // RG-04 : refus
            case ecrans::Cgu::Etat::Accepte: chargerJeu(app, a, nowSeconds()); break;      // acceptation notée : chargement
            default: break;
        }
        if (ecrans::chargement().reessaiDemande()) {   // WFS-146 (RG-10) : « Réessayer » (carte) ou « Quitter » (moteur)
            if (app.ready) { Game::demandes().carte = sauvegarde().map; Game::demandes().nuit = sauvegarde().tod == "nuit"; }
            else ANativeActivity_finish(a->activity);
        }
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
            plantage::minuterieDeclencheur(dt, app.game.ecran() == Ecran::Menu);   // WFS-126 (RG-05) : APK de test seulement
            gyroscopeAndroidLire(app.game.ecran() == Ecran::Course);   // WFS-108 : capteur actif en course seulement
            densiteAndroid(a->config, (float)app.renderer.height(), (float)ANativeWindow_getHeight(a->window));   // WFS-108 : dp
            if (g_margesNeuves.exchange(false))
                app.game.setMarges({(float)g_marges[0], (float)g_marges[1], (float)g_marges[2], (float)g_marges[3]});
            if (ecrans::chargement().enErreur()) ecrans::chargement().dessiner();   // WFS-74 : carte non chargée, message rouge
            else { manette::verifierDeconnexion(); manette::appliquer(app.game); app.game.update(dt); retourSystemeApresImage(app.game); app.game.render(t - app.start); }   // WFS-22 : manette ; WFS-116
            // WFS-123 (RG-06) : 30 images/s hors course (batterie), 60 en course : attente du reste de la période de l'image
            // (la synchro FIFO tient déjà 60 sur un écran 60 Hz ; l'attente ne sert qu'aux menus et aux écrans plus rapides)
            // WFS-262 (accroche, RG-04) : sous 60 img/s (cadence 30, menus), échéances sur une grille fixe (précédente + période),
            // réveil 3 ms avant : la synchro de l'écran (FIFO) place l'image ; à 60, la synchro seule (aucune dérive de grille)
            static double echeance = 0;
            const double periode = 1.0 / chauffe::plafondCourant();
            echeance = periode > 0.018 && echeance > 0 && nowSeconds() - echeance < periode ? echeance + periode : t + periode;
            if (const double reste = echeance - (periode > 0.018 ? 0.003 : 0.0) - nowSeconds(); reste > 0.002) {
                const timespec d{0, (long)((reste - 0.001) * 1e9)};
                nanosleep(&d, nullptr);
            }
            // mesure pour la QA : images par seconde, vitesse et distance toutes les 5 s (logcat « QA »)
            if (app.fpsT0 == 0) app.fpsT0 = t;
            if (++app.fpsFrames, t - app.fpsT0 >= 5.0) {
                const Player& p = app.game.player();
                WFS_LOG("QA ips=%.1f vitesse=%.0f km/h distance=%.0f m ecran=%ux%u", app.fpsFrames / (t - app.fpsT0), p.kmh(), p.dist * M,
                        app.renderer.width(), app.renderer.height());
                app.fpsFrames = 0; app.fpsT0 = t;
            }
        }
    }
    termGraphics(app);
    veilleur::arreter();
    jobs().stop();
}
