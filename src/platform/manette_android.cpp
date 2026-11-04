// WaForSpeed — manettes de jeu sur Android (WFS-22 / WFS-23) : événements de manette Bluetooth / USB → manette.h.
// Appelé par onInput (android_main.cpp) sur le thread de la boucle de jeu ; la déconnexion à chaud arrive de JeuActivity
// (InputManager.InputDeviceListener, thread de l'interface) par une variable atomique lue à chaque image.
#include <android/input.h>
#include <android/keycodes.h>
#include <jni.h>
#include <algorithm>
#include <atomic>
#include "manette.h"
#include "../core/log.h"

namespace wfs {
namespace manette {

static std::atomic<bool> g_retiree{false};

static bool sourceManette(int32_t src) {
    return (src & AINPUT_SOURCE_GAMEPAD) == AINPUT_SOURCE_GAMEPAD || (src & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK ||
           (src & AINPUT_SOURCE_DPAD) == AINPUT_SOURCE_DPAD;
}

static int boutonDe(int32_t code) {
    switch (code) {
        case AKEYCODE_BUTTON_A: case AKEYCODE_DPAD_CENTER: return A;
        case AKEYCODE_BUTTON_B: case AKEYCODE_BACK: return B;   // B arrive souvent comme « retour » : gardé par le jeu
        case AKEYCODE_BUTTON_X: return X;
        case AKEYCODE_BUTTON_Y: return Y;
        case AKEYCODE_BUTTON_START: return START;
        case AKEYCODE_BUTTON_SELECT: return SELECT;
        case AKEYCODE_BUTTON_L1: return L1;
        case AKEYCODE_BUTTON_R1: return R1;
        case AKEYCODE_BUTTON_L2: return L2;
        case AKEYCODE_BUTTON_R2: return R2;
        case AKEYCODE_DPAD_UP: return HAUT;
        case AKEYCODE_DPAD_DOWN: return BAS;
        case AKEYCODE_DPAD_LEFT: return GAUCHE;
        case AKEYCODE_DPAD_RIGHT: return DROITE;
        default: return -1;
    }
}

// renvoie true si l'événement vient d'une manette (il ne va alors pas au tactile)
bool evenementAndroid(const AInputEvent* e) {
    if (g_retiree.exchange(false)) deconnexion();
    const int32_t src = AInputEvent_getSource(e), type = AInputEvent_getType(e);
    if (type == AINPUT_EVENT_TYPE_MOTION && (src & AINPUT_SOURCE_TOUCHSCREEN) == AINPUT_SOURCE_TOUCHSCREEN) { toucherEcran(); return false; }
    if (!sourceManette(src)) return false;
    if (type == AINPUT_EVENT_TYPE_KEY) {
        const int b = boutonDe(AKeyEvent_getKeyCode(e));
        if (b < 0) return false;
        const int32_t act = AKeyEvent_getAction(e);
        if (act == AKEY_EVENT_ACTION_DOWN) bouton((Bouton)b, true);
        else if (act == AKEY_EVENT_ACTION_UP) bouton((Bouton)b, false);
        return true;
    }
    if (type == AINPUT_EVENT_TYPE_MOTION && (src & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
        auto ax = [&](int32_t a) { return AMotionEvent_getAxisValue(e, a, 0); };
        // gâchettes : AXIS_RTRIGGER / AXIS_LTRIGGER, ou AXIS_GAS / AXIS_BRAKE selon les manettes
        const float gd = std::max(ax(AMOTION_EVENT_AXIS_RTRIGGER), ax(AMOTION_EVENT_AXIS_GAS));
        const float gg = std::max(ax(AMOTION_EVENT_AXIS_LTRIGGER), ax(AMOTION_EVENT_AXIS_BRAKE));
        axes(ax(AMOTION_EVENT_AXIS_X), ax(AMOTION_EVENT_AXIS_Y), gg, gd, ax(AMOTION_EVENT_AXIS_HAT_X), ax(AMOTION_EVENT_AXIS_HAT_Y));
        return true;
    }
    return false;
}

// à chaque image (la déconnexion peut arriver sans aucun autre événement)
void verifierDeconnexion() { if (g_retiree.exchange(false)) deconnexion(); }

} // namespace manette
} // namespace wfs

// JeuActivity (InputDeviceListener) : une manette connue est partie
extern "C" JNIEXPORT void JNICALL Java_com_waforspeed_game_JeuActivity_nativeManetteRetiree(JNIEnv*, jclass) {
    wfs::manette::g_retiree = true;
    WFS_LOG("manette : retirée (Android)");
}
