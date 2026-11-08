// WaForSpeed — paquets de cartes : pont JNI vers com.waforspeed.game.Paquets (WFS-306) ; Android seulement, compilé avec android_main.cpp
// (inclus, comme vibration_android.cpp). Appelé depuis le fil du jeu, une fois par image de l'étape « paquets » : chaque appel est court
// (lecture d'un état ; la liste et le téléchargement tournent sur les fils de fond Java).
// La classe Paquets est trouvée par le chargeur de classes de l'activité (FindClass sur un fil natif ne voit que les classes du système),
// une seule fois, gardée en référence globale.
#if defined(__ANDROID__)
#include <jni.h>
#include <string>
#include "../core/configgame.h"
#include "../core/carte_cuite.h"
#include "../core/log.h"
#include "../ui/ecrans/paquets.h"

namespace wfs {
namespace {
JavaVM* g_paqVm = nullptr;
jobject g_paqActivite = nullptr;   // ANativeActivity::clazz
jclass g_paquets = nullptr;        // com.waforspeed.game.Paquets (référence globale)

template <class F> void avecPaquets(F f) {
    if (!g_paqVm || !g_paqActivite) return;
    JNIEnv* env = nullptr; bool attache = false;
    if (g_paqVm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_paqVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attache = true;
    }
    if (!g_paquets) {   // activite.getClassLoader().loadClass("com.waforspeed.game.Paquets")
        jclass cAct = env->GetObjectClass(g_paqActivite);
        jmethodID mCl = env->GetMethodID(cAct, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject cl = mCl ? env->CallObjectMethod(g_paqActivite, mCl) : nullptr;
        jclass cCl = cl ? env->GetObjectClass(cl) : nullptr;
        jmethodID mLoad = cCl ? env->GetMethodID(cCl, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
        jstring nom = env->NewStringUTF("com.waforspeed.game.Paquets");
        jobject c = (mLoad && nom) ? env->CallObjectMethod(cl, mLoad, nom) : nullptr;
        if (env->ExceptionCheck()) { env->ExceptionClear(); c = nullptr; }
        if (c) g_paquets = (jclass)env->NewGlobalRef(c);
        else WFS_ERR("paquets (WFS-306) : classe Paquets introuvable");
        for (jobject o : {(jobject)cAct, cl, (jobject)cCl, (jobject)nom, c}) if (o) env->DeleteLocalRef(o);
    }
    if (g_paquets) f(env, g_paquets);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attache) g_paqVm->DetachCurrentThread();
}

std::string dossier() { return carte::dossierPaquets(); }

struct SourceAndroid : ecrans::SourcePaquets {
    void chercher(const std::string& c, const std::string& formats, const std::string& empreinte, int versionApk) override {
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "chercher", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
            if (!m) return;
            jstring a = env->NewStringUTF(config().paquets.adresseListe), co = env->NewStringUTF((dossier() + "/cartes.json").c_str()),
                    ca = env->NewStringUTF(c.c_str()), fo = env->NewStringUTF(formats.c_str()), em = env->NewStringUTF(empreinte.c_str());
            env->CallStaticVoidMethod(k, m, a, co, ca, fo, em, (jint)versionApk);
            for (jstring s : {a, co, ca, fo, em}) if (s) env->DeleteLocalRef(s);
        });
    }
    Phase phase(long long& recus, long long& taille) override {
        Phase p = ABSENTE;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "etat", "()[J");
            jlongArray t = m ? (jlongArray)env->CallStaticObjectMethod(k, m) : nullptr;
            if (!t || env->GetArrayLength(t) < 3) return;
            jlong v[3]; env->GetLongArrayRegion(t, 0, 3, v);
            p = (Phase)v[0]; recus = v[1]; taille = v[2];
            env->DeleteLocalRef(t);
        });
        return p;
    }
    bool proposition(std::string& nom, std::string& format) override {
        bool ok = false;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "proposition", "()[Ljava/lang/String;");
            jobjectArray t = m ? (jobjectArray)env->CallStaticObjectMethod(k, m) : nullptr;
            if (!t || env->GetArrayLength(t) < 2) return;
            for (int i = 0; i < 2; i++) {
                jstring s = (jstring)env->GetObjectArrayElement(t, i);
                const char* u = s ? env->GetStringUTFChars(s, nullptr) : nullptr;
                (i ? format : nom) = u ? u : "";
                if (u) env->ReleaseStringUTFChars(s, u);
                if (s) env->DeleteLocalRef(s);
            }
            env->DeleteLocalRef(t);
            ok = true;
        });
        return ok;
    }
    int reseau() override {
        int r = 0;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "reseau", "(Landroid/content/Context;)I");
            if (m) r = env->CallStaticIntMethod(k, m, g_paqActivite);
        });
        return r;
    }
    long long placeLibre() override {
        long long p = -1;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "placeLibre", "(Ljava/lang/String;)J");
            jstring d = env->NewStringUTF(dossier().c_str());
            if (m && d) p = env->CallStaticLongMethod(k, m, d);
            if (d) env->DeleteLocalRef(d);
        });
        return p;
    }
    void installer() override {
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "installer", "(Ljava/lang/String;)V");
            jstring d = env->NewStringUTF(dossier().c_str());
            if (m && d) env->CallStaticVoidMethod(k, m, d);
            if (d) env->DeleteLocalRef(d);
        });
    }
    void arreter() override {
        avecPaquets([&](JNIEnv* env, jclass k) { jmethodID m = env->GetStaticMethodID(k, "arreter", "()V"); if (m) env->CallStaticVoidMethod(k, m); });
    }
};
} // namespace

// accroche d'android_main (avant le premier chargement) : machine Java et activité ; source des paquets pour l'étape et le menu
ecrans::SourcePaquets& sourcePaquetsAndroid(JavaVM* vm, jobject activite) {
    static SourceAndroid s;
    g_paqVm = vm; g_paqActivite = activite;
    return s;
}

} // namespace wfs
#endif
