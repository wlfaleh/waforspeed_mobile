// WaForSpeed — paquets de cartes : pont JNI vers com.waforspeed.game.Paquets (WFS-306) ; Android seulement, compilé avec android_main.cpp
// (inclus, comme vibration_android.cpp). Appelé depuis le fil du jeu, une fois par image de l'étape « paquets » : chaque appel est court
// (lecture d'un état ; la liste et le téléchargement tournent sur les fils de fond Java).
// La classe Paquets est trouvée par le chargeur de classes de l'activité (FindClass sur un fil natif ne voit que les classes du système),
// une seule fois, gardée en référence globale.
#if defined(__ANDROID__)
#include <jni.h>
#include <string>
#include <utility>
#include <vector>
#include "../core/configgame.h"
#include "../core/carte_cuite.h"
#include "../core/log.h"
#include "../core/platform.h"   // nowSeconds : petits caches (moins d'attaches JNI par image)
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

// L'état Java change au plus quelques fois par seconde : phase gardée 0,1 s, réseau 1 s, proposition jusqu'à la prochaine recherche
// (au lieu de 2 ou 3 attaches JNI par image, remarque du lead dev sur la PR #286).
struct SourceAndroid : ecrans::SourcePaquets {
    double tPhase_ = -1, tReseau_ = -1; Phase phase_ = ABSENTE; long long recus_ = 0, taille_ = 0; int reseau_ = 0;
    bool aProposition_ = false; std::string nom_, format_;
    void chercher(const std::string& c, const std::string& formats, const std::string& empreinte, int versionApk) override {
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "chercher", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
            if (!m) return;
            jstring a = env->NewStringUTF(config().paquets.adresseListe), co = env->NewStringUTF((dossier() + "/cartes.json").c_str()),
                    ca = env->NewStringUTF(c.c_str()), fo = env->NewStringUTF(formats.c_str()), em = env->NewStringUTF(empreinte.c_str());
            env->CallStaticVoidMethod(k, m, a, co, ca, fo, em, (jint)versionApk);
            for (jstring s : {a, co, ca, fo, em}) if (s) env->DeleteLocalRef(s);
        });
        tPhase_ = -1; aProposition_ = false;
    }
    Phase phase(long long& recus, long long& taille) override {
        const double t = nowSeconds();
        if (tPhase_ >= 0 && t - tPhase_ < 0.1) { recus = recus_; taille = taille_; return phase_; }
        Phase p = ABSENTE;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "etat", "()[J");
            jlongArray t = m ? (jlongArray)env->CallStaticObjectMethod(k, m) : nullptr;
            if (!t || env->GetArrayLength(t) < 3) return;
            jlong v[3]; env->GetLongArrayRegion(t, 0, 3, v);
            p = (Phase)v[0]; recus = v[1]; taille = v[2];
            env->DeleteLocalRef(t);
        });
        tPhase_ = t; phase_ = p; recus_ = recus; taille_ = taille;
        return p;
    }
    bool proposition(std::string& nom, std::string& format) override {
        if (aProposition_) { nom = nom_; format = format_; return true; }
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
        if (ok) { aProposition_ = true; nom_ = nom; format_ = format; }
        return ok;
    }
    int reseau() override {
        const double t = nowSeconds();
        if (tReseau_ >= 0 && t - tReseau_ < 1.0) return reseau_;
        int r = 0;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "reseau", "(Landroid/content/Context;)I");
            if (m) r = env->CallStaticIntMethod(k, m, g_paqActivite);
        });
        tReseau_ = t; reseau_ = r;
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
        tPhase_ = -1;
    }
    void arreter() override {
        avecPaquets([&](JNIEnv* env, jclass k) { jmethodID m = env->GetStaticMethodID(k, "arreter", "()V"); if (m) env->CallStaticVoidMethod(k, m); });
        tPhase_ = -1;
    }

    // ---- WFS-307 : toutes les cartes (choix du terrain) ; phase de chaque carte gardée 0,1 s, proposition jusqu'à la prochaine recherche
    struct Cache { double t = -1; Phase phase = ABSENTE; long long recus = 0, taille = 0; bool aProp = false, prop = false; std::string nom, format; };
    std::vector<std::pair<std::string, Cache>> caches_;
    Cache& cache(const std::string& c) {
        for (auto& [n, k] : caches_) if (n == c) return k;
        caches_.push_back({c, Cache{}});
        return caches_.back().second;
    }
    void oublier() { for (auto& [n, k] : caches_) { k.t = -1; k.aProp = false; } tPhase_ = -1; aProposition_ = false; }
    void chercherTout(const std::string& formats, const std::string& empreintes, int versionApk) override {
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "chercherTout", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
            if (!m) return;
            jstring a = env->NewStringUTF(config().paquets.adresseListe), co = env->NewStringUTF((dossier() + "/cartes.json").c_str()),
                    fo = env->NewStringUTF(formats.c_str()), em = env->NewStringUTF(empreintes.c_str());
            env->CallStaticVoidMethod(k, m, a, co, fo, em, (jint)versionApk);
            for (jstring s : {a, co, fo, em}) if (s) env->DeleteLocalRef(s);
        });
        oublier();
    }
    Phase phaseCarte(const std::string& c, long long& recus, long long& taille) override {
        Cache& kc = cache(c);
        const double t = nowSeconds();
        if (kc.t >= 0 && t - kc.t < 0.1) { recus = kc.recus; taille = kc.taille; return kc.phase; }
        Phase p = ABSENTE; recus = taille = 0;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "etatCarte", "(Ljava/lang/String;)[J");
            jstring ca = env->NewStringUTF(c.c_str());
            jlongArray v = (m && ca) ? (jlongArray)env->CallStaticObjectMethod(k, m, ca) : nullptr;
            if (ca) env->DeleteLocalRef(ca);
            if (!v || env->GetArrayLength(v) < 3) return;
            jlong x[3]; env->GetLongArrayRegion(v, 0, 3, x);
            p = (Phase)x[0]; recus = x[1]; taille = x[2];
            env->DeleteLocalRef(v);
        });
        kc.t = t; kc.phase = p; kc.recus = recus; kc.taille = taille;
        return p;
    }
    bool propositionCarte(const std::string& c, std::string& nom, std::string& format) override {
        Cache& kc = cache(c);
        if (kc.aProp) { nom = kc.nom; format = kc.format; return kc.prop; }
        bool ok = false;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "propositionCarte", "(Ljava/lang/String;)[Ljava/lang/String;");
            jstring ca = env->NewStringUTF(c.c_str());
            jobjectArray v = (m && ca) ? (jobjectArray)env->CallStaticObjectMethod(k, m, ca) : nullptr;
            if (ca) env->DeleteLocalRef(ca);
            if (!v || env->GetArrayLength(v) < 2) return;
            for (int i = 0; i < 2; i++) {
                jstring s = (jstring)env->GetObjectArrayElement(v, i);
                const char* u = s ? env->GetStringUTFChars(s, nullptr) : nullptr;
                (i ? format : nom) = u ? u : "";
                if (u) env->ReleaseStringUTFChars(s, u);
                if (s) env->DeleteLocalRef(s);
            }
            env->DeleteLocalRef(v);
            ok = true;
        });
        // pendant la lecture de la liste, ne rien garder (la proposition arrive avec PRETE)
        if (kc.phase != LISTE && kc.phase != RIEN) { kc.aProp = true; kc.prop = ok; kc.nom = nom; kc.format = format; }
        return ok;
    }
    bool installerCarte(const std::string& c) override {
        bool ok = false;
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "installerCarte", "(Ljava/lang/String;Ljava/lang/String;)Z");
            jstring d = env->NewStringUTF(dossier().c_str()), ca = env->NewStringUTF(c.c_str());
            if (m && d && ca) ok = env->CallStaticBooleanMethod(k, m, d, ca);
            for (jstring s : {d, ca}) if (s) env->DeleteLocalRef(s);
        });
        cache(c).t = -1; tPhase_ = -1;
        return ok;
    }
    void appelCarte(const char* methode, const std::string& c) {
        avecPaquets([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, methode, "(Ljava/lang/String;)V");
            jstring ca = env->NewStringUTF(c.c_str());
            if (m && ca) env->CallStaticVoidMethod(k, m, ca);
            if (ca) env->DeleteLocalRef(ca);
        });
        cache(c).t = -1; tPhase_ = -1;
    }
    void arreterCarte(const std::string& c) override { appelCarte("arreterCarte", c); }
    void supprimee(const std::string& c) override { appelCarte("supprimee", c); }
    void ralentir(bool oui) override {
        avecPaquets([&](JNIEnv* env, jclass k) { jmethodID m = env->GetStaticMethodID(k, "ralentir", "(Z)V"); if (m) env->CallStaticVoidMethod(k, m, (jboolean)oui); });
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
