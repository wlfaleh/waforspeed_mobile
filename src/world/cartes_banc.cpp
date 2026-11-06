// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) et des quartiers de la Ville (WFS-93) : voir decor_carte.h.
// Pilote automatique, caméra haute, comme la plage (carte-plage-*) : jour et nuit au départ, puis une capture de jour
// au début de chaque autre zone (départ avancé par WFS_DEPART, lu par le système de la carte).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../game/course_system.h"   // WFS-137 : courseBenchRules
#include "../game/game.h"
#include "../game/trace_cartes.h"   // WFS-139 : même tracé que le jeu ; WFS-140 : sections du Grand Prix
#include "../game/track.h"
#include "../render/renderer.h"
#include "decor_carte.h"
#include "zones.h"
#include "../game/trace_cartes.h"   // WFS-138 : sections du tracé 2

namespace wfs {

void benchCorniche(Game& g, Renderer& r, const std::string& out);   // WFS-139 (plage.cpp)

void benchCartes(Game& g, Renderer& r, const std::string& out) {
    struct Cap { const char* carte; const char* moment; const char* zone; const char* nom; };
    static const Cap CAPS[] = {
        {"campagne", "jour", nullptr, "carte-campagne-jour"}, {"campagne", "nuit", nullptr, "carte-campagne-nuit"},
        {"campagne", "jour", "Forêt", "carte-campagne-foret"}, {"campagne", "jour", "Village", "carte-campagne-village"},
        {"campagne", "jour", "Bois de pins", "carte-campagne-pins"},   // WFS-138 : vignes, col et épingle : benchCampagne (fin de la fonction)
        {"circuit", "jour", nullptr, "carte-circuit-jour"}, {"circuit", "nuit", nullptr, "carte-circuit-nuit"},
        {"circuit", "jour", "Paddock", "carte-circuit-zone-paddock"}, {"circuit", "jour", "Virages rapides", "carte-circuit-gravier"},
        {"circuit", "jour", "Section boisée", "carte-circuit-foret"},
        {"plage", "nuit", nullptr, "nuit-plage-corniche"}, {"plage", "jour", nullptr, "carte-plage-corniche"},   // WFS-81 : lampadaires « beach »
        {"plage", "jour", "Côte rocheuse", "carte-plage-rochers"},   // WFS-94 ; WFS-139 : marina et village dans benchCorniche
        {"ville", "jour", "Quartier résidentiel", "carte-ville-residentiel"}, {"ville", "jour", "Parc urbain", "carte-ville-parc"},
        {"ville", "jour", "Zone portuaire", "carte-ville-port"}};   // WFS-93 : quartiers de la ville
    for (const Cap& c : CAPS) {
        int64_t depart = 0;
        if (c.zone) {   // premier segment de la zone, + 40 (≈ 50 m) pour être dans le décor de la zone
            Track t; t.build(mapById(c.carte), trace::versionCarte(c.carte));   // WFS-138, WFS-140 : tracé de la partie (2 si actif)
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
    benchVille(g, r, out);   // WFS-137 : lieux de la Ville au tracé 2
    benchCorniche(g, r, out);   // WFS-139 : Corniche au tracé 2, caméra de course (plage.cpp)
    benchCampagne(g, r, out, false);   // WFS-138 : col, épingle, vignes au tracé 2 (jour, nuit), sans le parcours (job de la CI)
}

// WFS-138 (RG-C5) : campagne au tracé 2 : début du col (lacets), 1re épingle (40 m avant), vignes ; jour et nuit ;
// caméra de course (voiture arrêtée après le décompte), plus une vue haute du col (-haut) pour juger le relief
int benchCampagne(Game& g, Renderer& r, const std::string& out, bool parcours) {
    int ecarts = 0;
    Track t; t.build(mapById("campagne"), trace::versionCarte("campagne"));
    const int64_t L = (int64_t)t.segmentCount();
    const trace::Section* lac = nullptr;
    for (const trace::Section& s : t.sections()) if (s.type == trace::Type::Lacets) { lac = &s; break; }
    if (!lac) { WFS_ERR("banc campagne : pas de lacets (tracé %d)", trace::versionCarte("campagne")); return 1; }
    int64_t ep = -1;
    for (int64_t a = lac->debut; a < lac->fin && ep < 0; a++) if (std::abs(t.segAt(a).curve) > 10) ep = a;
    int64_t haut = -1;   // col : 1er segment droit à plus de 80 m au-dessus du pied (étage des sapins, sommets en face)
    const double y0 = t.point((double)lac->debut * SEG, 0).y;
    for (int64_t a = lac->debut; a < lac->fin && haut < 0; a++) if (t.point((double)a * SEG, 0).y - y0 > 80 && std::abs(t.segAt(a).curve) < 1) haut = a;
    int64_t pont = -1;   // cascade : 30 m avant le milieu du pont
    for (const trace::Section& s : t.sections()) if (s.type == trace::Type::Pont && pont < 0) pont = (s.debut + s.fin) / 2 - (int64_t)(30 / SEG_M);
    int64_t vi = -1;
    for (int64_t a = 0; a < L && vi < 0; a++) if (!std::strcmp(zoneName(*t.map(), a, L), "Vignobles")) vi = a + 40;
    WFS_LOG("banc campagne : %lld segments, lacets %lld → %lld (+%.0f m), 1re épingle %lld, vignes %lld", (long long)L, (long long)lac->debut,
            (long long)lac->fin, lac->denivele, (long long)ep, (long long)vi);
    struct Cap { const char* nom; int64_t seg; int cam; };
    const Cap caps[] = {{"carte-campagne-col", haut, 0}, {"carte-campagne-col-haut", haut, 1},
                        {"carte-campagne-epingle", ep - (int64_t)(40 / SEG_M), 0}, {"carte-campagne-vignes", vi, 0},
                        {"carte-campagne-cascade", pont, 0}};
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
    // RG-C5 : la campagne se roule de bout en bout au tracé 2 : du départ jusqu'au haut du col (+ 200 m) en pilote
    // automatique, sans jamais rester arrêté (décor sur la chaussée, caméra dans le décor), puis retour à l'accueil
    if (!parcours) return ecarts;
    if (g.init(r, "golf", "campagne")) {
        g.setAutopilot(true); g.camMode = 0; g.allerA(Ecran::Course);
        const double fin = (double)lac->fin * SEG + 200 / M;
        double tps = 0, arret = 0, pire = 0; int images = 0;
        while (g.player().dist < fin && tps < 400) {
            g.update(1.0 / 60); tps += 1.0 / 60; if (++images % 120 == 0) g.render(600 + tps);
            if (tps > 5 && g.player().kmh() < 5) { arret += 1.0 / 60; pire = std::max(pire, arret); } else arret = 0;
        }
        const bool arrive = g.player().dist >= fin;
        g.render(600 + tps); g.render(600.02 + tps); r.capturePng(out + "/carte-campagne-haut-du-col.png");
        g.setAutopilot(false); g.allerA(Ecran::Menu);
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(700 + i / 60.0); }
        const bool menu = g.ecran() == Ecran::Menu;
        WFS_LOG("banc campagne : parcours départ → haut du col (%.2f km) %s en %.0f s, plus long arrêt %.1f s ; retour à l'accueil : %s",
                fin * M / 1000, arrive ? "fait" : "PAS FAIT", tps, pire, menu ? "oui" : "NON");
        if (!arrive || pire > 3 || !menu) { ecarts++; WFS_ERR("banc campagne : parcours en échec"); }
        g.shutdown();
    } else ecarts++;
    WFS_LOG("banc campagne : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    return ecarts;
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
        if (s.type == T::RondPoint) {
            lieux.push_back({"carte-ville-rond-point-haut", (s.debut + s.fin) / 2, 12, false, 1});
            lieux.push_back({"carte-ville-fontaine", s.debut, -3, true, 1});   // à l'entrée de l'anneau, l'îlot devant à gauche
        }
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
