// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) et des quartiers de la Ville (WFS-93) : voir decor_carte.h.
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
#include "../game/trace_cartes.h"   // WFS-138 : sections du tracé 2

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
        {"plage", "nuit", nullptr, "nuit-plage-corniche"}, {"plage", "jour", nullptr, "carte-plage-corniche"},   // WFS-81 : lampadaires « beach »
        {"plage", "jour", "Port de plaisance", "carte-plage-marina"}, {"plage", "jour", "Côte rocheuse", "carte-plage-rochers"},
        {"plage", "jour", "Village de pêcheurs", "carte-plage-village"},   // WFS-94 : quartiers de la Corniche
        {"ville", "jour", "Quartier résidentiel", "carte-ville-residentiel"}, {"ville", "jour", "Parc urbain", "carte-ville-parc"},
        {"ville", "jour", "Zone portuaire", "carte-ville-port"}};   // WFS-93 : quartiers de la ville
    for (const Cap& c : CAPS) {
        int64_t depart = 0;
        if (c.zone) {   // premier segment de la zone, + 40 (≈ 50 m) pour être dans le décor de la zone
            Track t; t.build(mapById(c.carte), trace::versionCarte(c.carte));   // WFS-138 : tracé de la partie (2 si actif)
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

// WFS-138 (RG-C5) : campagne au tracé 2 : début du col (lacets), 1re épingle (40 m avant), vignes ; jour et nuit ;
// caméra de course (voiture arrêtée après le décompte), plus une vue haute du col (-haut) pour juger le relief
int benchCampagne(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    Track t; t.build(mapById("campagne"), trace::versionCarte("campagne"));
    const int64_t L = (int64_t)t.segmentCount();
    const trace::Section* lac = nullptr;
    for (const trace::Section& s : t.sections()) if (s.type == trace::Type::Lacets) { lac = &s; break; }
    if (!lac) { WFS_ERR("banc campagne : pas de lacets (tracé %d)", trace::versionCarte("campagne")); return 1; }
    int64_t ep = -1;
    for (int64_t a = lac->debut; a < lac->fin && ep < 0; a++) if (std::abs(t.segAt(a).curve) > 10) ep = a;
    int64_t vi = -1;
    for (int64_t a = 0; a < L && vi < 0; a++) if (!std::strcmp(zoneName(*t.map(), a, L), "Vignobles")) vi = a + 40;
    WFS_LOG("banc campagne : %lld segments, lacets %lld → %lld (+%.0f m), 1re épingle %lld, vignes %lld", (long long)L, (long long)lac->debut,
            (long long)lac->fin, lac->denivele, (long long)ep, (long long)vi);
    struct Cap { const char* nom; int64_t seg; int cam; };
    const Cap caps[] = {{"carte-campagne-col", lac->debut + 120, 0}, {"carte-campagne-col-haut", lac->debut + 120, 1},
                        {"carte-campagne-epingle", ep - (int64_t)(40 / SEG_M), 0}, {"carte-campagne-vignes", vi, 0}};
    for (const char* mo : {"jour", "nuit"}) for (const Cap& c : caps) {
        if (c.seg < 0) { ecarts++; WFS_ERR("banc campagne : %s introuvable", c.nom); continue; }
        if (!std::strcmp(mo, "nuit") && c.cam == 1) continue;
        setenv("WFS_MOMENT", mo, 1);
        setenv("WFS_DEPART", std::to_string(c.seg).c_str(), 1);
        if (!g.init(r, "golf", "campagne")) { ecarts++; continue; }
        g.setAutopilot(false); g.camMode = c.cam; g.allerA(Ecran::Course);
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
        g.render(404); g.render(404.02);
        const std::string nom = std::string(c.nom) + (!std::strcmp(mo, "nuit") ? "-nuit" : "");
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : segment %lld, %.0f m, %u appels de dessin", nom.c_str(), (long long)c.seg, g.player().dist * M, r.appelsDessin());
        g.shutdown();
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    return ecarts;
}

} // namespace wfs
