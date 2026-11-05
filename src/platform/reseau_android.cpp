// WaForSpeed — verrou de diffusion du Wi-Fi pour le duel sur le réseau local (WFS-83) ; Android seulement, compilé avec
// android_main.cpp (inclus, comme vibration_android.cpp). Sans ce verrou, beaucoup de téléphones jettent les paquets
// UDP en diffusion pour économiser la batterie : WifiManager.createMulticastLock(…).acquire() à l'ouverture du salon,
// release() quand on quitte le duel (permission CHANGE_WIFI_MULTICAST_STATE du manifeste). Échec : rien ne plante.
// WFS-112 (duel entre amis) : flux TLS vers le relais par javax.net.ssl de la plateforme (aucune bibliothèque tierce),
// sur un fil natif attaché à la machine Java ; partage de l'invitation (JeuActivity.partagerTexte) et lien profond reçu
// (JeuActivity.prendreLienDuel, rempli par onCreate / onNewIntent).
#if defined(__ANDROID__)
#include <jni.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "../core/log.h"
#include "../game/reseau_relais.h"
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
// ---------------------------------------------------------------- WFS-112 : flux TLS (SSLSocket de la plateforme)
struct EtatTls {
    std::mutex m;
    std::string aEnvoyer, recu;
    std::atomic<int> etat{(int)reseau::Flux::Etat::Connexion};
    std::atomic<bool> fin{false}, fermerApres{false};
};
// exception Java en cours : effacée ; vrai s'il y en avait une (*timeout : c'était un délai de lecture dépassé)
bool exceptionJava(JNIEnv* env, jclass cDelai = nullptr, bool* timeout = nullptr) {
    if (!env->ExceptionCheck()) return false;
    jthrowable t = env->ExceptionOccurred();
    env->ExceptionClear();
    if (timeout) *timeout = cDelai && t && env->IsInstanceOf(t, cDelai);
    if (t) env->DeleteLocalRef(t);
    return true;
}
void boucleTls(std::shared_ptr<EtatTls> e, std::string hote, int port) {
    JNIEnv* env = nullptr;
    if (!g_resVm || g_resVm->AttachCurrentThread(&env, nullptr) != JNI_OK) { e->etat = (int)reseau::Flux::Etat::Ferme; return; }
    jobject ssl = nullptr, in = nullptr, out = nullptr;
    jbyteArray tampon = nullptr;
    jclass cSock = env->FindClass("java/net/Socket"), cAdr = env->FindClass("java/net/InetSocketAddress");
    jclass cFab = env->FindClass("javax/net/ssl/SSLSocketFactory"), cHttps = env->FindClass("javax/net/ssl/HttpsURLConnection");
    jclass cVerif = env->FindClass("javax/net/ssl/HostnameVerifier"), cSsl = env->FindClass("javax/net/ssl/SSLSocket");
    jclass cIn = env->FindClass("java/io/InputStream"), cOut = env->FindClass("java/io/OutputStream");
    jclass cDelai = env->FindClass("java/net/SocketTimeoutException");
    bool ok = !exceptionJava(env) && cSock && cAdr && cFab && cHttps && cVerif && cSsl && cIn && cOut && cDelai;
    jmethodID mLire = nullptr, mEcrire = nullptr, mVider = nullptr;
    if (ok) {
        // connexion TCP avec un délai (8 s), puis TLS par-dessus (SNI et nom d'hôte vérifiés)
        jobject brut = env->NewObject(cSock, env->GetMethodID(cSock, "<init>", "()V"));
        jstring jh = env->NewStringUTF(hote.c_str());
        jobject adr = env->NewObject(cAdr, env->GetMethodID(cAdr, "<init>", "(Ljava/lang/String;I)V"), jh, port);
        if (!exceptionJava(env) && brut && adr) env->CallVoidMethod(brut, env->GetMethodID(cSock, "connect", "(Ljava/net/SocketAddress;I)V"), adr, 8000);
        jobject fab = exceptionJava(env) ? nullptr : env->CallStaticObjectMethod(cFab, env->GetStaticMethodID(cFab, "getDefault", "()Ljavax/net/SocketFactory;"));
        if (fab && !exceptionJava(env))
            ssl = env->CallObjectMethod(fab, env->GetMethodID(cFab, "createSocket", "(Ljava/net/Socket;Ljava/lang/String;IZ)Ljava/net/Socket;"), brut, jh, port, JNI_TRUE);
        ok = ssl && !exceptionJava(env);
        if (ok) {
            jmethodID mDelai = env->GetMethodID(cSock, "setSoTimeout", "(I)V");
            env->CallVoidMethod(ssl, mDelai, 8000);
            env->CallVoidMethod(ssl, env->GetMethodID(cSsl, "startHandshake", "()V"));
            ok = !exceptionJava(env);
            if (ok) {
                jobject verif = env->CallStaticObjectMethod(cHttps, env->GetStaticMethodID(cHttps, "getDefaultHostnameVerifier", "()Ljavax/net/ssl/HostnameVerifier;"));
                jobject session = env->CallObjectMethod(ssl, env->GetMethodID(cSsl, "getSession", "()Ljavax/net/ssl/SSLSession;"));
                ok = !exceptionJava(env) && verif && session &&
                     env->CallBooleanMethod(verif, env->GetMethodID(cVerif, "verify", "(Ljava/lang/String;Ljavax/net/ssl/SSLSession;)Z"), jh, session) && !exceptionJava(env);
                if (!ok) WFS_WARN("relais : certificat refusé pour %s", hote.c_str());
                if (verif) env->DeleteLocalRef(verif);
                if (session) env->DeleteLocalRef(session);
            }
            if (ok) {
                env->CallVoidMethod(ssl, mDelai, 15);   // lecture qui rend la main toutes les 15 ms (envois en file)
                in = env->CallObjectMethod(ssl, env->GetMethodID(cSock, "getInputStream", "()Ljava/io/InputStream;"));
                out = env->CallObjectMethod(ssl, env->GetMethodID(cSock, "getOutputStream", "()Ljava/io/OutputStream;"));
                mLire = env->GetMethodID(cIn, "read", "([B)I");
                mEcrire = env->GetMethodID(cOut, "write", "([B)V");
                mVider = env->GetMethodID(cOut, "flush", "()V");
                tampon = env->NewByteArray(4096);
                ok = !exceptionJava(env) && in && out && mLire && mEcrire && mVider && tampon;
            }
        }
        if (fab) env->DeleteLocalRef(fab);
        if (adr) env->DeleteLocalRef(adr);
        env->DeleteLocalRef(jh);
        if (brut) env->DeleteLocalRef(brut);
    }
    if (!ok) WFS_WARN("relais : connexion TLS à %s impossible", hote.c_str());
    else e->etat = (int)reseau::Flux::Etat::Ouvert;
    while (ok && !e->fin) {
        std::string sortie;
        { std::lock_guard<std::mutex> l(e->m); sortie.swap(e->aEnvoyer); }
        if (!sortie.empty()) {
            jbyteArray b = env->NewByteArray((jsize)sortie.size());
            if (b) {
                env->SetByteArrayRegion(b, 0, (jsize)sortie.size(), reinterpret_cast<const jbyte*>(sortie.data()));
                env->CallVoidMethod(out, mEcrire, b);
                env->CallVoidMethod(out, mVider);
                env->DeleteLocalRef(b);
            }
            if (exceptionJava(env)) break;
        }
        if (e->fermerApres) { std::lock_guard<std::mutex> l(e->m); if (e->aEnvoyer.empty()) break; }
        const jint n = env->CallIntMethod(in, mLire, tampon);
        bool delai = false;
        if (exceptionJava(env, cDelai, &delai)) { if (delai) continue; break; }
        if (n < 0) break;
        if (n > 0) {
            char buf[4096];
            env->GetByteArrayRegion(tampon, 0, n, reinterpret_cast<jbyte*>(buf));
            std::lock_guard<std::mutex> l(e->m);
            if (e->recu.size() < 262144) e->recu.append(buf, (size_t)n);
        }
    }
    if (ssl) { env->CallVoidMethod(ssl, env->GetMethodID(cSock, "close", "()V")); exceptionJava(env); }
    for (jobject o : {ssl, in, out, (jobject)tampon, (jobject)cSock, (jobject)cAdr, (jobject)cFab, (jobject)cHttps, (jobject)cVerif,
                      (jobject)cSsl, (jobject)cIn, (jobject)cOut, (jobject)cDelai})
        if (o) env->DeleteLocalRef(o);
    e->etat = (int)reseau::Flux::Etat::Ferme;
    g_resVm->DetachCurrentThread();
}
class FluxTlsAndroid : public reseau::Flux {
public:
    FluxTlsAndroid(const std::string& hote, int port) : e_(std::make_shared<EtatTls>()) {
        std::thread([e = e_, hote, port] { boucleTls(e, hote, port); }).detach();
    }
    ~FluxTlsAndroid() override { if (!e_->fermerApres) e_->fin = true; }
    Etat etat() const override { return (Etat)e_->etat.load(); }
    void envoyer(const std::string& o) override { std::lock_guard<std::mutex> l(e_->m); if (e_->aEnvoyer.size() < 65536) e_->aEnvoyer += o; }
    std::string lire() override { std::lock_guard<std::mutex> l(e_->m); std::string r; r.swap(e_->recu); return r; }
    void fermer() override { e_->fermerApres = true; }
    void couper() override { e_->fin = true; }
private:
    std::shared_ptr<EtatTls> e_;
};

// appel d'une méthode de JeuActivity depuis le fil du jeu (attaché le temps de l'appel)
template <class F> void avecJava(F f) {
    if (!g_resVm || !g_resActivite) return;
    JNIEnv* env = nullptr; bool attache = false;
    if (g_resVm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_resVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attache = true;
    }
    jclass c = env->GetObjectClass(g_resActivite);
    if (c) { f(env, c); env->DeleteLocalRef(c); }
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attache) g_resVm->DetachCurrentThread();
}
// « Inviter » : feuille de partage d'Android (JeuActivity.partagerTexte, sur le fil de l'interface)
void partagerAndroid(const std::string& texte) {
    avecJava([&](JNIEnv* env, jclass c) {
        jmethodID m = env->GetMethodID(c, "partagerTexte", "(Ljava/lang/String;)V");
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
        jstring t = env->NewStringUTF(texte.c_str());
        if (m && t) env->CallVoidMethod(g_resActivite, m, t);
        if (t) env->DeleteLocalRef(t);
    });
}
// lien profond reçu (waforspeed://duel/<code>) : lu une fois (JeuActivity.prendreLienDuel)
std::string lienAndroid() {
    std::string lien;
    avecJava([&](JNIEnv* env, jclass c) {
        jmethodID m = env->GetStaticMethodID(c, "prendreLienDuel", "()Ljava/lang/String;");
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
        jstring j = m ? (jstring)env->CallStaticObjectMethod(c, m) : nullptr;
        if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
        if (!j) return;
        const char* u = env->GetStringUTFChars(j, nullptr);
        if (u) { lien = u; env->ReleaseStringUTFChars(j, u); }
        env->DeleteLocalRef(j);
    });
    return lien;
}
} // namespace

// accroche de android_main : machine Java et activité, puis verrou branché pour reseau::verrou() (salon.cpp)
void installerReseauAndroid(JavaVM* vm, jobject activite) {
    g_resVm = vm; g_resActivite = activite;
    reseau::installerVerrou(verrouAndroid);
    // WFS-112 : relais du duel entre amis (TLS), « Inviter », lien d'invitation
    reseau::installerFluxTls([](const std::string& hote, int port) { return std::unique_ptr<reseau::Flux>(new FluxTlsAndroid(hote, port)); });
    reseau::installerPartage(partagerAndroid);
    reseau::installerLienRecu(lienAndroid);
}
} // namespace wfs
#endif
