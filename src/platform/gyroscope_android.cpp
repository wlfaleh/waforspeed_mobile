// WaForSpeed — capteur d'inclinaison Android pour le gyroscope (WFS-108, épic E3), compilé avec android_main.cpp.
// Capteur de gravité (ASENSOR_TYPE_GRAVITY, déjà filtré par Android), sinon accéléromètre lissé (filtre passe-bas, 0,15).
// Le capteur n'est activé que pendant le jeu au premier plan et quand le réglage « Gyroscope » n'est pas coupé (batterie).
// La file d'événements est vidée une fois par image sur le fil principal (gyroscopeAndroidLire), puis la gravité est publiée
// pour la direction (gyroscope.h). Densité de l'écran : AConfiguration_getDensity (points par pouce / 160 = pixels par dp).
#include <android/configuration.h>
#include <android/looper.h>
#include <android/sensor.h>
#include <dlfcn.h>
#include "../game/direction.h"
#include "gyroscope.h"

namespace wfs {
namespace {
struct CapteurAndroid {
    ASensorManager* gestionnaire = nullptr;
    const ASensor* capteur = nullptr;
    ASensorEventQueue* file = nullptr;
    bool gravite = false, actif = false, lisse = false;
    float gx = 0, gy = 0;
} g_capteur;

ASensorManager* gestionnaireCapteurs() {
    // ASensorManager_getInstanceForPackage (API 26) si présent, sinon getInstance (déprécié mais toujours là)
    using Fn = ASensorManager* (*)(const char*);
    if (void* lib = dlopen("libandroid.so", RTLD_NOW))
        if (auto f = (Fn)dlsym(lib, "ASensorManager_getInstanceForPackage")) return f("com.waforspeed.game");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return ASensorManager_getInstance();
#pragma clang diagnostic pop
}
void vider();
// rappel du looper : vide la file (sinon ALooper_pollOnce de la boucle principale reviendrait sans fin) ; garder le rappel
int rappelCapteur(int, int, void*) { vider(); return 1; }
} // namespace

// au démarrage : capteur choisi, file créée sur le looper du fil principal, avec un rappel qui la vide
void gyroscopeAndroidDemarrer(ALooper* looper) {
    CapteurAndroid& c = g_capteur;
    c.gestionnaire = gestionnaireCapteurs();
    if (!c.gestionnaire) return;
    c.capteur = ASensorManager_getDefaultSensor(c.gestionnaire, ASENSOR_TYPE_GRAVITY);
    c.gravite = c.capteur != nullptr;
    if (!c.capteur) c.capteur = ASensorManager_getDefaultSensor(c.gestionnaire, ASENSOR_TYPE_ACCELEROMETER);
    if (!c.capteur) { WFS_LOG("gyroscope (WFS-108) : aucun capteur d'inclinaison"); return; }
    c.file = ASensorManager_createEventQueue(c.gestionnaire, looper, ALOOPER_POLL_CALLBACK, rappelCapteur, nullptr);
    gyroscope::capteurPresent(c.file != nullptr);
    WFS_LOG("gyroscope (WFS-108) : capteur %s", c.gravite ? "de gravité" : "accéléromètre (lissé)");
}

// à chaque image : capteur activé seulement s'il sert ; événements lus, gravité publiée
void gyroscopeAndroidLire(bool premierPlan) {
    CapteurAndroid& c = g_capteur;
    if (!c.file) return;
    const bool voulu = premierPlan && reglagesGyro().niveau > 0;
    if (voulu != c.actif) {
        if (voulu) { ASensorEventQueue_enableSensor(c.file, c.capteur); ASensorEventQueue_setEventRate(c.file, c.capteur, 16667); c.lisse = false; }
        else { ASensorEventQueue_disableSensor(c.file, c.capteur); gyroscope::etat().valide = false; }
        c.actif = voulu;
    }
    vider();
}

namespace {
void vider() {
    CapteurAndroid& c = g_capteur;
    if (!c.file) return;
    ASensorEvent ev[16];
    ssize_t n;
    bool neuf = false;
    while ((n = ASensorEventQueue_getEvents(c.file, ev, 16)) > 0)
        for (ssize_t i = 0; i < n; i++) {
            const float x = ev[i].acceleration.x, y = ev[i].acceleration.y;
            if (c.gravite || !c.lisse) { c.gx = x; c.gy = y; c.lisse = true; }
            else { c.gx += (x - c.gx) * 0.15f; c.gy += (y - c.gy) * 0.15f; }
            neuf = true;
        }
    if (c.actif && neuf) gyroscope::publier(c.gx, c.gy);
}
} // namespace

// WFS-108 RG-01 : pixels de l'interface par dp (densité de l'écran × rapport entre la taille de l'interface et la fenêtre)
void densiteAndroid(AConfiguration* config, float hauteurInterface, float hauteurFenetre) {
    const int dpi = config ? AConfiguration_getDensity(config) : 0;
    if (dpi <= 0 || dpi == ACONFIGURATION_DENSITY_ANY || dpi == ACONFIGURATION_DENSITY_NONE || hauteurFenetre <= 0) return;
    densiteCommandes() = dpi / 160.f * hauteurInterface / hauteurFenetre;
}

} // namespace wfs
