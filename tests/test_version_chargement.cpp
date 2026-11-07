// Version de l'APK à l'écran de chargement (WFS-279, RG-01, RG-03) : mise en forme du texte et valeur par défaut du PC.
// La valeur vient de versionName de android/app/build.gradle (seule source), passée par CMake (WFS_VERSION) ; ce test
// reçoit la même définition que chargement.cpp (CMakeLists.txt).
// usage : test_version_chargement
#include <cstdio>
#include <cstring>
#include <string>
#include "../src/ui/ecrans/chargement.h"
using wfs::ecrans::texteVersion;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // APK de main : versionName « 0.1.N » (N = numéro du run de build.yml)
    VERIFIE(texteVersion("0.1.912") == "v0.1.912", "« 0.1.912 » → « v0.1.912 » (%s)", texteVersion("0.1.912").c_str());
    VERIFIE(texteVersion("0.1.0") == "v0.1.0", "construction Gradle hors CI : « v0.1.0 »");
    // PC, bancs et tests : valeur fixe, captures identiques d'un run à l'autre
    VERIFIE(texteVersion("dev") == "dev", "« dev » reste « dev »");
    // valeur absente ou vide : jamais de texte vide ni de plantage
    VERIFIE(texteVersion(nullptr) == "dev", "valeur absente → « dev »");
    VERIFIE(texteVersion("") == "dev", "valeur vide → « dev »");
    // valeur trop longue (définition externe) : 24 caractères au plus, « v » compris en plus
    VERIFIE(texteVersion("0.1.123456789012345678901234567890").size() == 25, "24 caractères au plus (+ « v »)");
#ifdef WFS_VERSION
    // RG-03 : sans -DWFS_VERSION, CMake donne « dev » (même définition que chargement.cpp)
    VERIFIE(std::strcmp(WFS_VERSION, "dev") == 0, "valeur par défaut du PC : « dev » (reçu : « %s »)", WFS_VERSION);
#else
    VERIFIE(false, "WFS_VERSION non défini pour ce test (CMakeLists.txt, set_source_files_properties)");
#endif
    std::printf("test_version_chargement : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
