// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) : voir decor_carte.h.
// Pilote automatique, caméra haute, comme la plage (carte-plage-*) : jour et nuit au départ, puis une capture de jour
// au début de chaque autre zone (départ avancé par WFS_DEPART, lu par le système de la carte).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "decor_carte.h"
#include "zones.h"

namespace wfs {

void benchCartes(Game& g, Renderer& r, const std::string& out) {
    struct Cap { const char* carte; const char* moment; const char* zone; const char* nom; };
    static const Cap CAPS[] = {
        {"campagne", "jour", nullptr, "carte-campagne-jour"}, {"campagne", "nuit", nullptr, "carte-campagne-nuit"},
        {"campagne", "jour", "Forêt", "carte-campagne-foret"}, {"campagne", "jour", "Village", "carte-campagne-village"},
        {"campagne", "jour", "Vignobles", "carte-campagne-vignes"}, {"campagne", "jour", "Bois de pins", "carte-campagne-pins"},
        {"circuit", "jour", nullptr, "carte-circuit-jour"}, {"circuit", "nuit", nullptr, "carte-circuit-nuit"},
        {"circuit", "jour", "Paddock", "carte-circuit-paddock"}, {"circuit", "jour", "Virages rapides", "carte-circuit-gravier"},
        {"circuit", "jour", "Section boisée", "carte-circuit-foret"},
        {"plage", "nuit", nullptr, "nuit-plage-corniche"}, {"plage", "jour", nullptr, "carte-plage-corniche"}};   // WFS-81 : lampadaires « beach »
    for (const Cap& c : CAPS) {
        int64_t depart = 0;
        if (c.zone) {   // premier segment de la zone, + 40 (≈ 50 m) pour être dans le décor de la zone
            Track t; t.build(mapById(c.carte));
            const int64_t L = (int64_t)t.segmentCount();
            depart = -1;
            for (int64_t a = 0; a < L && depart < 0; a++) if (!std::strcmp(zoneName(*t.map(), a, L), c.zone)) depart = a + 40;
            if (depart < 0) { WFS_ERR("banc cartes : zone %s absente de %s", c.zone, c.carte); continue; }
        }
        setenv("WFS_MOMENT", c.moment, 1);
        setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
        g.init(r, "golf", c.carte); g.setAutopilot(true); g.camMode = 1;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }   // peu d'images (rendu logiciel lent)
        g.render(404); g.render(404.02); r.capturePng(out + "/" + c.nom + ".png");
        WFS_LOG("capture %s : %s, %s, départ %lld segments, %.0f m", c.nom, c.carte, c.moment, (long long)depart, g.player().dist * M);
        g.shutdown();
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
