// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) et des quartiers de la Ville (WFS-93) : voir decor_carte.h.
// Pilote automatique, caméra haute, comme la plage (carte-plage-*) : jour et nuit au départ, puis une capture de jour
// au début de chaque autre zone (départ avancé par WFS_DEPART, lu par le système de la carte).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"   // WFS-140 : sections du Grand Prix (captures en caméra de course)
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
        {"circuit", "jour", "Paddock", "carte-circuit-zone-paddock"}, {"circuit", "jour", "Virages rapides", "carte-circuit-gravier"},
        {"circuit", "jour", "Section boisée", "carte-circuit-foret"},
        {"plage", "nuit", nullptr, "nuit-plage-corniche"}, {"plage", "jour", nullptr, "carte-plage-corniche"},   // WFS-81 : lampadaires « beach »
        {"plage", "jour", "Port de plaisance", "carte-plage-marina"}, {"plage", "jour", "Côte rocheuse", "carte-plage-rochers"},
        {"plage", "jour", "Village de pêcheurs", "carte-plage-village"},   // WFS-94 : quartiers de la Corniche
        {"ville", "jour", "Quartier résidentiel", "carte-ville-residentiel"}, {"ville", "jour", "Parc urbain", "carte-ville-parc"},
        {"ville", "jour", "Zone portuaire", "carte-ville-port"}};   // WFS-93 : quartiers de la ville
    for (const Cap& c : CAPS) {
        int64_t depart = 0;
        if (c.zone) {   // premier segment de la zone, + 40 (≈ 50 m) pour être dans le décor de la zone
            Track t; t.build(mapById(c.carte), trace::versionCarte(c.carte));   // WFS-140 : même tracé que le jeu
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
    // WFS-140 (RG-C5) : Grand Prix en caméra de course, de jour et de nuit, devant chaque lieu des sections du tracé 2
    {
        Track t; t.build(mapById("circuit"), trace::versionCarte("circuit"));
        const auto& S = t.sections();
        const trace::Section *ep = nullptr, *ch = nullptr, *mo = nullptr;
        for (const trace::Section& s : S) {
            if (s.type == trace::Type::Epingle && !ep) ep = &s;
            if (s.type == trace::Type::Chicane && !ch) ch = &s;
            if (s.type == trace::Type::Montee && !mo) mo = &s;
        }
        // nom, segment de départ, images simulées (le départ s'arrête au milieu du décompte : 3 feux allumés)
        struct Lieu { const char* nom; int64_t depart; int images; };
        const Lieu LIEUX[] = {{"carte-circuit-depart", 0, 100}, {"carte-circuit-epingle", ep ? ep->debut - 60 : -1, 240},
                              {"carte-circuit-chicane", ch ? ch->debut - 45 : -1, 240}, {"carte-circuit-raidillon", mo ? mo->debut + 10 : -1, 240},
                              {"carte-circuit-paddock", 40, 240}};
        for (const Lieu& l : LIEUX)
            for (const char* moment : {"jour", "nuit"}) {
                if (l.depart < 0) { WFS_ERR("banc cartes (WFS-140) : section absente pour %s", l.nom); continue; }
                const std::string nom = std::string(l.nom) + (std::strcmp(moment, "nuit") ? "" : "-nuit");
                setenv("WFS_MOMENT", moment, 1);
                setenv("WFS_DEPART", std::to_string(l.depart).c_str(), 1);
                g.init(r, "golf", "circuit"); g.setAutopilot(l.images > 100); g.camMode = 0;   // départ : voiture arrêtée sur la grille
                for (int i = 0; i < l.images; i++) { g.update(1.0 / 60); if (i % 60 == 0) g.render(400 + i / 60.0); }   // 4 rendus : temps du job
                g.render(404); g.render(404.02); r.capturePng(out + "/" + nom + ".png");
                WFS_LOG("capture %s : circuit, %s, caméra de course, départ %lld segments, %.0f m", nom.c_str(), moment, (long long)l.depart, g.player().dist * M);
                g.shutdown();
            }
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
