// WaForSpeed — lecture de l'extra d'intent `qa_simu` (WFS-59) ; Android seulement.
#if defined(__ANDROID__)
#include <jni.h>
#include <cstring>
#include "gpu_logiciel.h"

namespace wfs {
bool lireExtraQaSimu(void* vmp, void* activiteP) {
    JavaVM* vm = (JavaVM*)vmp; jobject activite = (jobject)activiteP;
    if (!vm || !activite) return false;
    JNIEnv* env = nullptr; bool attache = false;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        attache = true;
    }
    bool oui = false;
    jclass ca = env->GetObjectClass(activite);
    jmethodID getIntent = env->GetMethodID(ca, "getIntent", "()Landroid/content/Intent;");
    jobject intent = getIntent ? env->CallObjectMethod(activite, getIntent) : nullptr;
    if (intent && !env->ExceptionCheck()) {
        jclass ci = env->GetObjectClass(intent);
        jstring cle = env->NewStringUTF("qa_simu");
        jmethodID getBool = env->GetMethodID(ci, "getBooleanExtra", "(Ljava/lang/String;Z)Z");
        if (getBool) oui = env->CallBooleanMethod(intent, getBool, cle, JNI_FALSE) == JNI_TRUE;
        if (!oui && !env->ExceptionCheck()) {   // extra passé en texte (`--es qa_simu true`)
            jmethodID getStr = env->GetMethodID(ci, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
            jstring v = getStr ? (jstring)env->CallObjectMethod(intent, getStr, cle) : nullptr;
            if (v && !env->ExceptionCheck()) {
                const char* s = env->GetStringUTFChars(v, nullptr);
                oui = s && (std::strcmp(s, "true") == 0 || std::strcmp(s, "1") == 0);
                if (s) env->ReleaseStringUTFChars(v, s);
                env->DeleteLocalRef(v);
            }
        }
        env->DeleteLocalRef(cle); env->DeleteLocalRef(ci); env->DeleteLocalRef(intent);
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(ca);
    if (attache) vm->DetachCurrentThread();
    return oui;
}
}
#endif
