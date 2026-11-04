// WaForSpeed — verrou de diffusion du Wi-Fi pour le duel sur le réseau local (WFS-83) ; Android seulement, compilé avec
// android_main.cpp (inclus, comme vibration_android.cpp). Sans ce verrou, beaucoup de téléphones jettent les paquets
// UDP en diffusion pour économiser la batterie : WifiManager.createMulticastLock(…).acquire() à l'ouverture du salon,
// release() quand on quitte le duel (permission CHANGE_WIFI_MULTICAST_STATE du manifeste). Échec : rien ne plante.
#if defined(__ANDROID__)
#include <jni.h>
#include "../core/log.h"
#include "../game/reseau_udp.h"

namespace wfs {
namespace {
JavaVM* g_resVm = nullptr;
jobject g_resActivite = nullptr;   // ANativeActivity::clazz
jobject g_verrou = nullptr;        // référence globale vers le MulticastLock

void verrouAndroid(bool prendre) {
    if (!g_resVm || !g_resActivite) return;
    JNIEnv* env = nullptr; bool attache = false;
    if (g_resVm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_resVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attache = true;
    }
    if (!g_verrou && prendre) {   // créé une fois : getApplicationContext().getSystemService(WIFI_SERVICE).createMulticastLock
        jclass cAct = env->GetObjectClass(g_resActivite);
        jmethodID mApp = cAct ? env->GetMethodID(cAct, "getApplicationContext", "()Landroid/content/Context;") : nullptr;
        jobject app = mApp ? env->CallObjectMethod(g_resActivite, mApp) : nullptr;
        jclass cCtx = env->FindClass("android/content/Context");
        jfieldID fNom = cCtx ? env->GetStaticFieldID(cCtx, "WIFI_SERVICE", "Ljava/lang/String;") : nullptr;
        jobject nom = fNom ? env->GetStaticObjectField(cCtx, fNom) : nullptr;
        jmethodID mSvc = cCtx ? env->GetMethodID(cCtx, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;") : nullptr;
        jobject wifi = (app && nom && mSvc && !env->ExceptionCheck()) ? env->CallObjectMethod(app, mSvc, nom) : nullptr;
        if (wifi && !env->ExceptionCheck()) {
            jclass cWifi = env->FindClass("android/net/wifi/WifiManager");
            jmethodID mCree = cWifi ? env->GetMethodID(cWifi, "createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;") : nullptr;
            jstring etiquette = env->NewStringUTF("waforspeed-duel");
            jobject v = mCree ? env->CallObjectMethod(wifi, mCree, etiquette) : nullptr;
            if (v && !env->ExceptionCheck()) {
                jclass cV = env->GetObjectClass(v);
                jmethodID mRef = cV ? env->GetMethodID(cV, "setReferenceCounted", "(Z)V") : nullptr;
                if (mRef) env->CallVoidMethod(v, mRef, JNI_FALSE);
                g_verrou = env->NewGlobalRef(v);
                if (cV) env->DeleteLocalRef(cV);
            }
            if (v) env->DeleteLocalRef(v);
            env->DeleteLocalRef(etiquette);
            if (cWifi) env->DeleteLocalRef(cWifi);
        }
        if (wifi) env->DeleteLocalRef(wifi);
        if (nom) env->DeleteLocalRef(nom);
        if (cCtx) env->DeleteLocalRef(cCtx);
        if (app) env->DeleteLocalRef(app);
        if (cAct) env->DeleteLocalRef(cAct);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    if (g_verrou) {
        jclass cV = env->GetObjectClass(g_verrou);
        jmethodID m = cV ? env->GetMethodID(cV, prendre ? "acquire" : "release", "()V") : nullptr;
        if (m) env->CallVoidMethod(g_verrou, m);
        if (cV) env->DeleteLocalRef(cV);
    }
    if (env->ExceptionCheck()) { env->ExceptionClear(); WFS_WARN("duel réseau : verrou de diffusion impossible (Wi-Fi indisponible ?)"); }
    if (attache) g_resVm->DetachCurrentThread();
}
} // namespace

// accroche de android_main : machine Java et activité, puis verrou branché pour reseau::verrou() (salon.cpp)
void installerReseauAndroid(JavaVM* vm, jobject activite) {
    g_resVm = vm; g_resActivite = activite;
    reseau::installerVerrou(verrouAndroid);
}
} // namespace wfs
#endif
