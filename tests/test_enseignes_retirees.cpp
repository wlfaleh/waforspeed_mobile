// WFS-590 (ANO-317, RG-01) : les caissons d'enseigne sur mâts de la Ville ne sont plus dessinés (ville::CAISSONS_SUR_MATS faux) :
// ils ne doivent plus être solides non plus. Pour chaque panneau gardé dans le plan (tirages inchangés), ni la collision du
// plan (Plan::chocs, conduite d'avant) ni le registre des chocs du décor (chocs_declaration.cpp, voiture du joueur) ne déclarent
// d'obstacle à sa place (poteaux de 30 × 30 cm à RW + 1 et RW + 5 m). Aucun autre obstacle n'est retiré : seuls ces 2 poteaux par
// panneau manquent, comparé au compte attendu.
// usage : test_enseignes_retirees
#include <cmath>
#include <cstdio>
#include "../src/game/chocs_decor.h"
#include "../src/game/conduite_banc.h"
#include "../src/world/ville_quartiers.h"
using namespace wfs;

int main() {
    int ecarts = 0;
    static_assert(!ville::CAISSONS_SUR_MATS, "WFS-590 : caissons sur mâts retirés de l'affichage");
    Track& t = banc_conduite::piste("ville");
    const MapDef& map = mapById("ville");
    chocs::declarerCarte(t, map);
    const ville::Plan& P = ville::adapter(map, t);
    const auto& reg = chocs::registre().parSegment();
    const float R = (float)RW;
    std::printf("panneaux gardés dans le plan : %zu ; obstacles déclarés : %zu\n", P.panneaux.size(), chocs::registre().taille());
    if (P.panneaux.empty()) { std::printf("ÉCHEC : aucun panneau dans le plan, le test ne vérifie rien\n"); ecarts++; }
    for (const ville::Panneau& b : P.panneaux) {
        const size_t s = (size_t)(((b.seg % (int64_t)reg.size()) + (int64_t)reg.size()) % (int64_t)reg.size());
        for (const chocs::ObstacleRoute& o : reg[s])
            for (float l : {R + 1.f, R + 5.f})
                if (o.genre == chocs::Genre::Poteau && o.forme == chocs::Forme::Boite && std::fabs(o.lat - (float)b.side * l) < 0.01f) {
                    std::printf("ÉCHEC : poteau invisible au segment %lld, latéral %.2f m (registre des chocs)\n", (long long)b.seg, o.lat); ecarts++;
                }
        if ((size_t)b.seg < P.chocs.size())
            for (const ville::Choc& c : P.chocs[(size_t)b.seg])
                if (std::fabs(c.offset - (float)b.side * (R + 1.f) / R) < 1e-4f && std::fabs(c.hit - 0.15f) < 1e-4f) {
                    std::printf("ÉCHEC : collision du plan au segment %lld (offset %.3f)\n", (long long)b.seg, c.offset); ecarts++;
                }
    }
    std::printf("enseignes sur mâts retirées (WFS-590) : %s\n", ecarts ? "ÉCHEC" : "OK");
    return ecarts ? 1 : 0;
}
