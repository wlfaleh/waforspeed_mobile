// WFS-421 : halos au sol des lampadaires coupés en Basse, tête de lampe gardée, Moyenne et Haute inchangées.
#include <cstdio>
#include "../src/render/nuit_qualite.h"

static int echecs = 0;
static void verifier(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "OK  " : "ECHEC", quoi);
    if (!ok) echecs++;
}

int main() {
    using namespace wfs; using namespace wfs::nuit_qualite;
    verifier(!haloSolLampadaire(ecrans::QUALITE_BASSE), "RG-01 : Basse -> pas de halo au sol des lampadaires");
    verifier(haloSolLampadaire(ecrans::QUALITE_MOYENNE), "RG-01 : Moyenne -> halo au sol inchangé");
    verifier(haloSolLampadaire(ecrans::QUALITE_HAUTE), "RG-01 : Haute -> halo au sol inchangé");
    verifier(haloTeteLampadaire(ecrans::QUALITE_BASSE), "RG-02 : Basse -> halo de la tête gardé");
    verifier(haloTeteLampadaire(ecrans::QUALITE_MOYENNE) && haloTeteLampadaire(ecrans::QUALITE_HAUTE), "RG-02 : Moyenne et Haute -> tête gardée");
    static_assert(!haloSolLampadaire(ecrans::QUALITE_BASSE) && haloSolLampadaire(ecrans::QUALITE_HAUTE), "WFS-421");
    std::printf(echecs ? "test_nuit_qualite : %d échec(s)\n" : "test_nuit_qualite : tout est bon\n", echecs);
    return echecs ? 1 : 0;
}
