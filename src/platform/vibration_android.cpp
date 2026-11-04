// WaForSpeed — vibration du téléphone (ticket WFS-70) ; Android seulement, compilé avec android_main.cpp (inclus).
// navigator.vibrate([90, 40, 120]) du jeu web → Vibrator.vibrate(VibrationEffect.createWaveform({0, 90, 40, 120}, −1))
// (Android 8+, minSdk 26 ; permission VIBRATE du manifeste). Appelé depuis la boucle de jeu (thread de android_main).
#if defined(__ANDROID__)
#include <jni.h>
#include "../core/log.h"
#include "../game/ressenti.h"

namespace wfs {
namespace {
JavaVM* g_vibVm = nullptr;
jobject g_vibActivite = nullptr;   // ANativeActivity::clazz (référence globale tenue par NativeActivity)

void vibrerAndroid(const int* ms, int n) {
    if (!g_vibVm || !g_vibActivite || n <= 0) return;
    JNIEnv* env = nullptr; bool attache = false;
    if (g_vibVm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_vibVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attache = true;
    }
    jclass cCtx = env->FindClass("android/content/Context");
    jfieldID fNom = cCtx ? env->GetStaticFieldID(cCtx, "VIBRATOR_SERVICE", "Ljava/lang/String;") : nullptr;
    jobject nom = fNom ? env->GetStaticObjectField(cCtx, fNom) : nullptr;
    jmethodID mSvc = cCtx ? env->GetMethodID(cCtx, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;") : nullptr;
    jobject vib = (nom && mSvc) ? env->CallObjectMethod(g_vibActivite, mSvc, nom) : nullptr;
    if (vib && !env->ExceptionCheck()) {
        jclass cVib = env->FindClass("android/os/Vibrator");
        jclass cEff = env->FindClass("android/os/VibrationEffect");
        jmethodID mHas = cVib ? env->GetMethodID(cVib, "hasVibrator", "()Z") : nullptr;
        jmethodID mVib = cVib ? env->GetMethodID(cVib, "vibrate", "(Landroid/os/VibrationEffect;)V") : nullptr;
        jmethodID mOnde = cEff ? env->GetStaticMethodID(cEff, "createWaveform", "([JI)Landroid/os/VibrationEffect;") : nullptr;
        if (mHas && mVib && mOnde && env->CallBooleanMethod(vib, mHas) == JNI_TRUE) {
            // motif Android : attente, vibration, attente, vibration… (le motif du jeu web commence par une vibration)
            jlong d[17]; int k = 0; d[k++] = 0;
            for (int i = 0; i < n && k < 17; i++) d[k++] = ms[i];
            jlongArray arr = env->NewLongArray(k);
            env->SetLongArrayRegion(arr, 0, k, d);
            jobject eff = env->CallStaticObjectMethod(cEff, mOnde, arr, (jint)-1);
            if (eff && !env->ExceptionCheck()) env->CallVoidMethod(vib, mVib, eff);
            if (eff) env->DeleteLocalRef(eff);
            env->DeleteLocalRef(arr);
        }
        if (cVib) env->DeleteLocalRef(cVib);
        if (cEff) env->DeleteLocalRef(cEff);
    }
    if (env->ExceptionCheck()) { env->ExceptionClear(); WFS_WARN("vibration : impossible (Vibrator indisponible)"); }
    if (vib) env->DeleteLocalRef(vib);
    if (nom) env->DeleteLocalRef(nom);
    if (cCtx) env->DeleteLocalRef(cCtx);
    if (attache) g_vibVm->DetachCurrentThread();
}
} // namespace

// accroche de android_main : machine Java et activité, puis vibreur installé pour vibrer() (ressenti.cpp)
void installerVibrationAndroid(JavaVM* vm, jobject activite) {
    g_vibVm = vm; g_vibActivite = activite;
    installerVibreur(vibrerAndroid);
}
} // namespace wfs
#endif
