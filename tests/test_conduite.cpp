// WFS-241 : banc de conduite sans GPU (src/game/conduite_banc.h) : fiches des 20 voitures, 6 virages × 3 joueurs simulés ×
// aide activée / désactivée (ancien et nouveau modèle), RG-07 (aucune commande automatique aide désactivée), déterminisme.
// Échec si le nouveau modèle a un écart. usage : test_conduite
#include <cstdio>
#include "../src/game/conduite_banc.h"

int main() {
    std::string rapport;
    const int e = wfs::banc_conduite::mesurer(rapport);
    std::fputs(rapport.c_str(), stdout);
    return e > 0 ? 1 : 0;
}
