// WFS-421 : plus de disque au sol des lampadaires, à aucune qualité (suite, ordre de l'utilisateur du 2026-11-11) ; tête de lampe gardée.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include "../src/render/nuit_qualite.h"

static int echecs = 0;
static void verifier(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "OK  " : "ECHEC", quoi);
    if (!ok) echecs++;
}

int main() {
    using namespace wfs; using namespace wfs::nuit_qualite;
    verifier(haloTeteLampadaire(ecrans::QUALITE_BASSE), "RG-02 : Basse -> halo de la tête gardé");
    verifier(haloTeteLampadaire(ecrans::QUALITE_MOYENNE) && haloTeteLampadaire(ecrans::QUALITE_HAUTE), "RG-02 : Moyenne et Haute -> tête gardée");
    // RG-01 (suite) : aucun disque au sol, à aucune qualité : nuit.cpp n'appelle plus r.haloSol (lu dans la source)
    {
        std::ifstream f(WFS_SOURCE_DIR "/src/render/nuit.cpp");
        const std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        verifier(!src.empty(), "source nuit.cpp lue");
        verifier(src.find("r.haloSol(") == std::string::npos, "RG-01 : plus aucun disque additif au sol des lampadaires (Basse, Moyenne, Haute)");
    }
    std::printf(echecs ? "test_nuit_qualite : %d échec(s)\n" : "test_nuit_qualite : tout est bon\n", echecs);
    return echecs ? 1 : 0;
}
