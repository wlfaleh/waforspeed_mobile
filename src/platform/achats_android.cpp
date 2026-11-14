// WaForSpeed — achats Google Play : pont JNI vers com.waforspeed.game.Achats (WFS-511, épic E21) ; Android seulement, compilé avec
// android_main.cpp (inclus, comme paquets_android.cpp, son modèle). Version Android de l'interface market::Boutique de WFS-510 :
//   - démarrage après la lecture de la sauvegarde (achatsAndroidDemarrer) : connexion au service, produits, rattrapage (RG-02, 03, 05) ;
//     la boutique est branchée sur sauvegarde() (market::brancher : crédit + jeton écrits, PUIS consommer / confirmer, RG-04) et
//     publiée par market::boutiqueActive() pour l'écran Market (WFS-513) ;
//   - les réponses de Google arrivent sur le fil de l'interface Java ; le jeu les lit sur son fil, toutes les 0,25 s au plus
//     (achatsAndroidLire, une attache JNI), et les passe au rappel de la boutique ;
//   - RG-07 : sans Google Play ou sans réseau, aucun événement : rien ne plante ; RG-08 : jamais de jeton complet au journal.
// La classe Achats est trouvée par le chargeur de classes de l'activité (même raison que Paquets), une fois, en référence globale.
#if defined(__ANDROID__)
#include <jni.h>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>
#include "../core/configgame.h"
#include "../core/log.h"
#include "../core/platform.h"   // nowSeconds
#include "../game/market.h"
#include "../game/sauvegarde.h"

namespace wfs {
namespace {
JavaVM* g_achVm = nullptr;
jobject g_achActivite = nullptr;   // ANativeActivity::clazz
jclass g_achats = nullptr;         // com.waforspeed.game.Achats (référence globale)

template <class F> void avecAchats(F f) {
    if (!g_achVm || !g_achActivite) return;
    JNIEnv* env = nullptr; bool attache = false;
    if (g_achVm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (g_achVm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attache = true;
    }
    if (!g_achats) {   // activite.getClassLoader().loadClass("com.waforspeed.game.Achats")
        jclass cAct = env->GetObjectClass(g_achActivite);
        jmethodID mCl = env->GetMethodID(cAct, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject cl = mCl ? env->CallObjectMethod(g_achActivite, mCl) : nullptr;
        jclass cCl = cl ? env->GetObjectClass(cl) : nullptr;
        jmethodID mLoad = cCl ? env->GetMethodID(cCl, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
        jstring nom = env->NewStringUTF("com.waforspeed.game.Achats");
        jobject c = (mLoad && nom) ? env->CallObjectMethod(cl, mLoad, nom) : nullptr;
        if (env->ExceptionCheck()) { env->ExceptionClear(); c = nullptr; }
        if (c) g_achats = (jclass)env->NewGlobalRef(c);
        else WFS_ERR("achats (WFS-511) : classe Achats introuvable");
        for (jobject o : {(jobject)cAct, cl, (jobject)cCl, (jobject)nom, c}) if (o) env->DeleteLocalRef(o);
    }
    if (g_achats) f(env, g_achats);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attache) g_achVm->DetachCurrentThread();
}

// appel statique Achats.<methode>(String...) sans retour
void appeler(const char* methode, const char* signature, std::initializer_list<const std::string*> args) {
    avecAchats([&](JNIEnv* env, jclass k) {
        jmethodID m = env->GetStaticMethodID(k, methode, signature);
        if (!m) return;
        jvalue v[3] = {}; int n = 0;
        for (const std::string* a : args) if (n < 3) v[n++].l = env->NewStringUTF(a->c_str());
        env->CallStaticVoidMethodA(k, m, v);
        for (int i = 0; i < n; i++) if (v[i].l) env->DeleteLocalRef(v[i].l);
    });
}

market::Etat etatDe(const std::string& t) {
    if (t == "ACHETE") return market::Etat::Achete;
    if (t == "ATTENTE") return market::Etat::EnAttente;
    if (t == "ANNULE") return market::Etat::Annule;
    return market::Etat::Erreur;
}

class BoutiqueGoogle : public market::Boutique {
public:
    const char* nom() const override { return "Google Play"; }
    void chargerProduits() override { appeler("charger", "()V", {}); }
    // RG-03 : formattedPrice de Google ; vide tant que Google ne l'a pas donné (ou produit introuvable : pack masqué)
    std::string prix(const std::string& id) const override {
        const auto it = prix_.find(id);
        return it == prix_.end() ? std::string() : it->second;
    }
    void acheter(const std::string& id) override { appeler("acheter", "(Ljava/lang/String;)V", {&id}); }
    void restaurer() override { appeler("restaurer", "()V", {}); }
    void confirmer(const std::string& id, const std::string& jeton) override {
        appeler("confirmer", "(Ljava/lang/String;Ljava/lang/String;)V", {&id, &jeton});
    }
    // fil du jeu : événements de Google (triplets type, produit, valeur)
    void lire() {
        std::vector<std::string> ev;
        avecAchats([&](JNIEnv* env, jclass k) {
            jmethodID m = env->GetStaticMethodID(k, "evenements", "()[Ljava/lang/String;");
            jobjectArray t = m ? (jobjectArray)env->CallStaticObjectMethod(k, m) : nullptr;
            if (!t) return;
            const jsize n = env->GetArrayLength(t);
            for (jsize i = 0; i < n; i++) {
                jstring s = (jstring)env->GetObjectArrayElement(t, i);
                const char* u = s ? env->GetStringUTFChars(s, nullptr) : nullptr;
                ev.emplace_back(u ? u : "");
                if (u) env->ReleaseStringUTFChars(s, u);
                if (s) env->DeleteLocalRef(s);
            }
            env->DeleteLocalRef(t);
        });
        for (size_t i = 0; i + 2 < ev.size(); i += 3) {
            if (ev[i] == "PRIX") { prix_[ev[i + 1]] = ev[i + 2]; continue; }
            signaler(ev[i + 1], ev[i + 2], etatDe(ev[i]));   // market::brancher : traiterAchat (sauvegarde écrite), puis confirmer
        }
    }
private:
    std::map<std::string, std::string> prix_;
};

BoutiqueGoogle& google() { static BoutiqueGoogle b; return b; }
bool g_achDemarre = false;
} // namespace

// accroche d'android_main, après sauvegarde().charger : connexion à Google Play, rattrapage des achats (RG-02, RG-05)
void achatsAndroidDemarrer(JavaVM* vm, jobject activite) {
    g_achVm = vm; g_achActivite = activite;
    std::string produits, consommables;
    for (int i = 0; i < MARKET_PRODUITS; i++) {   // les 4 premiers (packs d'étoiles) sont consommables ; pack débutant et sans pub : confirmés
        produits += (i ? "," : "") + std::string(MARKET_PRODUITS_IDS[i]);
        if (i < MARKET_ETOILES) consommables += (i ? "," : "") + std::string(MARKET_PRODUITS_IDS[i]);
    }
    market::brancher(google(), sauvegarde());
    market::boutiqueActive() = &google();
    avecAchats([&](JNIEnv* env, jclass k) {
        jmethodID m = env->GetStaticMethodID(k, "demarrer", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)V");
        if (!m) return;
        jstring p = env->NewStringUTF(produits.c_str()), c = env->NewStringUTF(consommables.c_str());
        env->CallStaticVoidMethod(k, m, g_achActivite, p, c);
        for (jstring s : {p, c}) if (s) env->DeleteLocalRef(s);
        g_achDemarre = true;
    });
    WFS_LOG("achats (WFS-511) : boutique Google Play %s", g_achDemarre ? "démarrée" : "indisponible");
}

// accroche de la boucle d'android_main : réponses de Google lues toutes les 0,25 s au plus
void achatsAndroidLire() {
    static double t0 = -1;
    if (!g_achDemarre) return;
    const double t = nowSeconds();
    if (t0 >= 0 && t - t0 < 0.25) return;
    t0 = t;
    google().lire();
}

} // namespace wfs
#endif
