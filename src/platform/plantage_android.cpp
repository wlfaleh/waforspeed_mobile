// WaForSpeed — rapport de plantage, côté Android (WFS-126, épic E7), compilé avec android_main.cpp.
// JeuActivity.onCreate (avant super.onCreate, donc avant le thread du jeu) donne la version de l'appli et le déclencheur de
// test (RG-05, extra `wfs_plantage`, déjà filtré côté Java : APK de test seulement) ; le modèle du téléphone et la version
// d'Android sont lus ici dans les propriétés du système. android_main installe ensuite le gestionnaire (RG-01).
#include <jni.h>
#include <sys/system_properties.h>
#include <atomic>
#include <cstdio>
#include "plantage.h"

namespace wfs {
namespace {
char g_versionAppli[64] = "?";
std::atomic<bool> g_declencheurJava{false};
} // namespace

// au démarrage du thread du jeu : gestionnaire de signaux et anneau du journal (RG-01, RG-02), puis déclencheur (RG-05)
inline void plantageAndroidDemarrer(const char* dossier) {
    char modele[PROP_VALUE_MAX] = "", marque[PROP_VALUE_MAX] = "", android[PROP_VALUE_MAX] = "", api[PROP_VALUE_MAX] = "";
    __system_property_get("ro.product.model", modele);
    __system_property_get("ro.product.manufacturer", marque);
    __system_property_get("ro.build.version.release", android);
    __system_property_get("ro.build.version.sdk", api);
    char appareil[128];
    std::snprintf(appareil, sizeof appareil, "%s %s, Android %s (API %s)", marque, modele, android, api);
    plantage::installer(dossier, g_versionAppli, appareil);
    plantage::rapportPrecedent(dossier);   // déjà renommé par JeuActivity (boîte) ; filet si la boîte n'a pas pu s'ouvrir
    if (g_declencheurJava) plantage::armerDeclencheur(true);
}
} // namespace wfs

// JeuActivity.onCreate : versionName, versionCode, déclencheur de test (RG-05)
extern "C" JNIEXPORT void JNICALL Java_com_waforspeed_game_JeuActivity_nativeInfosPlantage(JNIEnv* env, jclass, jstring version, jint code,
                                                                                         jboolean declencheur) {
    const char* v = version ? env->GetStringUTFChars(version, nullptr) : nullptr;
    std::snprintf(wfs::g_versionAppli, sizeof wfs::g_versionAppli, "%s (%d)", v ? v : "?", (int)code);
    if (v) env->ReleaseStringUTFChars(version, v);
    wfs::g_declencheurJava = declencheur == JNI_TRUE;
}
