// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) et des quartiers de la Ville (WFS-93) : voir decor_carte.h.
// Pilote automatique, caméra haute, comme la plage (carte-plage-*) : jour et nuit au départ, puis une capture de jour
// au début de chaque autre zone (départ avancé par WFS_DEPART, lu par le système de la carte).
#include "../game/trace_cartes.h"   // WFS-137
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../game/course_system.h"   // WFS-137 : courseBenchRules
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
        {"plage", "nuit", nullptr, "nuit-plage-corniche"}, {"plage", "jour", nullptr, "carte-plage-corniche"},   // WFS-81 : lampadaires « beach »
        {"plage", "jour", "Port de plaisance", "carte-plage-marina"}, {"plage", "jour", "Côte rocheuse", "carte-plage-rochers"},
        {"plage", "jour", "Village de pêcheurs", "carte-plage-village"},   // WFS-94 : quartiers de la Corniche
        {"ville", "jour", "Quartier résidentiel", "carte-ville-residentiel"}, {"ville", "jour", "Parc urbain", "carte-ville-parc"},
        {"ville", "jour", "Zone portuaire", "carte-ville-port"}};   // WFS-93 : quartiers de la ville
    for (const Cap& c : CAPS) {
        int64_t depart = 0;
        if (c.zone) {   // premier segment de la zone, + 40 (≈ 50 m) pour être dans le décor de la zone
            Track t; t.build(mapById(c.carte), trace::versionCarte(c.carte));   // WFS-137 : même tracé que la partie (tracé 2 des cartes actives)
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
    benchVille(g, r, out);   // WFS-137 : lieux de la Ville au tracé 2
}

// WFS-137 (RG-C5) : lieux de la Ville au tracé 2, caméra de course (voiture arrêtée avant le lieu, pendant le décompte),
// de jour et de nuit : carte-ville-rond-point, -feux, -autoroute, -tunnel, -pont (+ -nuit) ; virages serrés de jour
// (carte-ville-virage-1 à 3 : rien sur la chaussée ni côté intérieur)
void benchVille(Game& g, Renderer& r, const std::string& out) {
    if (!trace::carteActive("ville")) return;
    Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    struct Lieu { std::string nom; int64_t seg; double avant; bool nuit; int cam = 0; };   // cam 1 : caméra haute (contrôle du côté intérieur)
    std::vector<Lieu> lieux;
    int virages = 0; bool fx = false, rp = false, au = false, tu = false, po = false;
    const int64_t m40 = (int64_t)(40 / SEG_M);
    for (const trace::Section& s : trace::sections(t)) {
        using T = trace::Type;
        if (s.type == T::RondPoint && !rp) { rp = true; lieux.push_back({"carte-ville-rond-point", s.debut, 45, true}); }
        else if (s.type == T::Feux && !fx) { fx = true; lieux.push_back({"carte-ville-feux", s.ligneArret, 30, true}); }
        else if (s.type == T::Autoroute && !au) { au = true; lieux.push_back({"carte-ville-autoroute", s.debut + 2 * m40, 0, true}); }
        else if (s.type == T::Tunnel && !tu) { tu = true; lieux.push_back({"carte-ville-tunnel", s.debut, 40, true}); }
        else if (s.type == T::Pont && !po) { po = true; lieux.push_back({"carte-ville-pont", s.debut, 40, true}); }
        else if (s.type == T::Virage && s.rayon < 60 && virages < 3) {
            lieux.push_back({"carte-ville-virage-" + std::to_string(++virages), s.debut, 35, false});
            lieux.push_back({"carte-ville-virage-" + std::to_string(virages) + "-haut", (s.debut + s.fin) / 2, 18, false, 1});   // au cœur du virage
        }
        if (s.type == T::RondPoint) lieux.push_back({"carte-ville-rond-point-haut", (s.debut + s.fin) / 2, 12, false, 1});
    }
    const char* seul = std::getenv("WFS_VILLE_LIEU");   // essai local : un seul lieu (préfixe du nom)
    for (const Lieu& l : lieux) for (int n = 0; n < (l.nuit ? 2 : 1); n++) {
        const std::string nom = l.nom + (n ? "-nuit" : "");
        if (seul && nom.rfind(seul, 0) != 0) continue;
        setenv("WFS_MOMENT", n ? "nuit" : "jour", 1);
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, l.seg - (int64_t)(l.avant / SEG_M))).c_str(), 1);
        courseBenchRules(true);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("banc ville : chargement impossible"); continue; }
        g.setAutopilot(false); g.camMode = l.cam; g.allerA(Ecran::Course);
        for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(500 + i / 60.0); }   // décompte : voiture arrêtée
        g.render(502); g.render(502.02); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : segment %lld, %.0f m avant, %.0f m", nom.c_str(), (long long)l.seg, l.avant, g.player().dist * M);
        g.shutdown();
    }
    courseBenchRules(false);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
