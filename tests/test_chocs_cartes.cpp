// WFS-256 : obstacles déclarés de la Ville et des îlots de rond-point (chocs_declaration.cpp) sur la vraie piste, mesures du
// banc des chocs (chocs_banc.h) : tours au pilote sans choc (RG-05), chaque genre arrêté à 30 / 80 / 150 km/h (RG-02) ;
// et les 6 virages du banc de conduite sur la ligne idéale : aucun choc. (Corniche, Campagne, Grand Prix : leur décor est
// déclaré au chargement de la carte, avec le GPU : banc WFS_BANC=chocs.)
// usage : test_chocs_cartes
#include <cstdio>
#include "../src/game/chocs_banc.h"
#include "../src/game/conduite_banc.h"
#include "../src/game/trace_cartes.h"
using namespace wfs;

int main() {
    int ecarts = 0; std::string rapport;
    for (const char* id : {"ville", "campagne", "plage", "circuit"}) {
        Track& t = banc_conduite::piste(id);
        chocs::declarerCarte(t, mapById(id));
        ecarts += banc_chocs::mesurer(t, mapById(id), rapport);
    }
    int c = 0;
    for (const banc_conduite::Virage& v : banc_conduite::virages()) {   // RG-05 : virages du banc de conduite, ligne idéale
        Track& t = banc_conduite::piste(v.carte);
        chocs::declarerCarte(t, mapById(v.carte));
        Player p; p.reset(carById("golf")); p.aide = Aide::Activee;
        p.dist = v.debutU - 80 / M; p.position = t.wrap(p.dist); p.speed = vitesseVirageNormale(t, p.dist);
        for (int n = 0; n < 120 * 60 && p.dist < v.finU + 40 / M; n++) { p.update(1.0 / 120, joueurLigneIdeale(p, t), t, true); c += p.chocsDecor.n; p.chocsDecor = {}; }
    }
    std::fputs(rapport.c_str(), stdout);
    std::printf("virages du banc (ligne idéale) : %d choc(s)\n", c);
    if (c) ecarts++;
    std::printf("obstacles des cartes (WFS-256) : %s\n", ecarts ? "ÉCHEC" : "OK");
    return ecarts ? 1 : 0;
}
