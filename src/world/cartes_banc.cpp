// WaForSpeed — banc d'essai des cartes Campagne et Grand Prix (WFS-53) et des quartiers de la Ville (WFS-93) : voir decor_carte.h.
// Pilote automatique, caméra haute, comme la plage (carte-plage-*) : jour et nuit au départ, puis une capture de jour
// au début de chaque autre zone (départ avancé par WFS_DEPART, lu par le système de la carte).
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"   // WFS-212 : caméra haute du banc
#include "../game/course_system.h"   // WFS-137 : courseBenchRules
#include "../game/game.h"
#include "../game/trace_cartes.h"   // WFS-139 : même tracé que le jeu ; WFS-140 : sections du Grand Prix
#include "../game/track.h"
#include "../render/renderer.h"
#include "../ui/ecrans/reglages.h"   // WFS-249 : qualité Basse du banc campagne
#include "decor_carte.h"
#include "ville_quartiers.h"   // WFS-176 : lieux de la Ville
#include "zones.h"
#include "../game/trace_cartes.h"   // WFS-138 : sections du tracé 2

namespace wfs {
namespace campagne { int64_t troupeau(int espece); bool sommet(vec3& s); }   // WFS-186, WFS-216 (campagne.cpp)

// WFS-212 : caméra haute du banc (vue de dessus d'un lieu) : réglée par le crochet ambiance, sans scène dédiée : la carte entière
// reste dessinée (la Ville ne dessine rien en scène dédiée). Inactive hors banc.
namespace {
struct VueHaute { bool actif = false; vec3 oeil{}, cible{}; float fov = 50; vec3 haut{0, 0, -1}; } g_vueHaute;   // WFS-269 : haut {0, 1, 0} (vue à hauteur d'homme)
class VueHauteBanc : public System {
public:
    const char* name() const override { return "vue-haute-banc"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_vueHaute.actif) return;
        fp.view = lookAt(g_vueHaute.oeil, g_vueHaute.cible, g_vueHaute.haut);
        fp.proj = perspectiveVk(g_vueHaute.fov * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
        fp.camPos = g_vueHaute.oeil;
    }
};
} // namespace
WFS_SYSTEM(VueHauteBanc, 995)

void benchCorniche(Game& g, Renderer& r, const std::string& out);   // WFS-139 (plage.cpp)

void benchCartes(Game& g, Renderer& r, const std::string& out) {
    struct Cap { const char* carte; const char* moment; const char* zone; const char* nom; };
    static const Cap CAPS[] = {
        {"campagne", "jour", nullptr, "carte-campagne-jour"}, {"campagne", "nuit", nullptr, "carte-campagne-nuit"},
        {"campagne", "jour", "Forêt", "carte-campagne-foret"},   // WFS-178 : village (feu, rond-point) : benchCampagne
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
        // WFS-199 : tribunes de près (caméra vitrine tournée vers la tribune), jour et nuit ; WFS_TRIBUNES : ces 4 captures seules
        const trace::Section* fx = nullptr; for (const trace::Section& s : S) if (s.type == trace::Type::Feux && s.depart && !fx) fx = &s;
        for (int k = 0; k < 5; k++) if (const trace::Section* sc = k < 2 || k == 4 ? fx : ep) {   // 4 : départ en qualité Basse
            const int64_t n = k < 2 ? sc->debut + 36 : (sc->debut + sc->fin) / 2; const int cote = k < 2 ? 1 : (sc->angle > 0 ? -1 : 1);
            const TrackPoint a = t.point((double)n * SEG, 0), b = t.point((double)n * SEG, cote * 10.0);
            setenv("WFS_MOMENT", k % 2 ? "nuit" : "jour", 1); setenv("WFS_DEPART", std::to_string(n).c_str(), 1);
            r.qualiteBasse = k == 4; g.init(r, "golf", "circuit"); g.setAutopilot(false); g.camMode = 2; g.orbit = std::atan2(-(float)(b.x - a.x), -(float)(b.z - a.z));
            for (int i = 0; i < 60; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
            const auto t0 = std::chrono::steady_clock::now(); g.render(404); g.render(404.02); const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 2;
            const std::string nom = std::string("carte-circuit-tribune-") + (k < 2 || k == 4 ? "depart" : "epingle") + (k % 2 ? "-nuit" : "") + (k == 4 ? "-basse" : "");
            r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s : %u appels de dessin, %.0f ms par image", nom.c_str(), r.appelsDessin(), ms); g.shutdown(); r.qualiteBasse = false;
        }
        if (std::getenv("WFS_TRIBUNES")) return;
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
    int64_t feu = -1, rp = -1;   // WFS-178 : 20 m avant la ligne d'arrêt du feu du village, 15 m avant l'entrée du rond-point
    for (const trace::Section& s : t.sections()) {
        if (s.type == trace::Type::Feux && !s.depart && feu < 0) feu = s.ligneArret - (int64_t)(20 / SEG_M);
        if (s.type == trace::Type::RondPoint && rp < 0) rp = s.debut - (int64_t)(15 / SEG_M);
    }
    int64_t vi = -1;
    for (int64_t a = 0; a < L && vi < 0; a++) if (!std::strcmp(zoneName(*t.map(), a, L), "Vignobles")) vi = a + 40;
    int64_t au = -1;   // WFS-179 : début de l'autoroute (aire de péage à 30 m), et 300 m plus loin (voitures en sens inverse)
    for (const trace::Section& s : t.sections()) if (s.autoroute && au < 0) au = s.debut;
    WFS_LOG("banc campagne : %lld segments, lacets %lld → %lld (+%.0f m), 1re épingle %lld, vignes %lld", (long long)L, (long long)lac->debut,
            (long long)lac->fin, lac->denivele, (long long)ep, (long long)vi);
    int64_t tr[3] = {-1, -1, -1};   // WFS-186 : troupeaux (vaches, chevaux, moutons), caméra 25 m avant le premier de chaque espèce
    if (g.init(r, "golf", "campagne")) { for (int e = 0; e < 3; e++) tr[e] = campagne::troupeau(e) >= 0 ? campagne::troupeau(e) - (int64_t)(25 / SEG_M) : -1; g.shutdown(); }
    // WFS-216 (RG-01, ANO-103) : vue libre (vue = 1 : du haut du col vers la plus haute cime ; 2 : épingle vue d'en haut, depuis
    // l'approche) ; la voiture part du même segment, seule la caméra change
    // WFS-216 (RG-03, ANO-108) : recul = m de recul en plus la nuit : le troupeau (17 à 45 m de l'axe) entre dans le cône des
    // phares (demi-angle 0,7 rad, portée 170 m : nuit.cpp) ; aucune lumière ajoutée
    struct Cap { const char* nom; int64_t seg; int cam; int vue = 0; float reculNuit = 0; };
    const Cap caps[] = {{"carte-campagne-vaches", tr[0], 0, 0, 30}, {"carte-campagne-chevaux", tr[1], 0}, {"carte-campagne-moutons", tr[2], 0, 0, 30},
                        {"carte-campagne-col", haut, 0}, {"carte-campagne-col-haut", haut, 1, 1},
                        {"carte-campagne-epingle", ep - (int64_t)(40 / SEG_M), 0, 2}, {"carte-campagne-vignes", vi, 0},
                        {"carte-campagne-cascade", pont, 0}, {"carte-campagne-village", feu, 0}, {"carte-campagne-rondpoint", rp, 0},
                        {"carte-campagne-peage", au >= 0 ? au : -1, 0}, {"carte-campagne-autoroute", au >= 0 ? au + (int64_t)(300 / SEG_M) : -1, 0}};   // WFS-179
    const char* seule = std::getenv("WFS_CAMPAGNE_LIEU");   // WFS-179 : essai local d'un seul lieu (préfixe du nom)
    const char* moment = std::getenv("WFS_CAMPAGNE_MOMENT");   // WFS-249 : « nuit » (ou « jour ») seulement
    const bool basse = std::getenv("WFS_QUALITE_BASSE") != nullptr;   // WFS-249 (RG-03) : qualité Basse, captures …-basse
    if (basse) { ecrans::reglages().qualite = ecrans::QUALITE_BASSE; r.qualiteBasse = true; }
    for (const char* mo : {"jour", "nuit"}) for (const Cap& c : caps) {
        if (seule && std::string(c.nom).rfind(seule, 0) != 0) continue;
        if (moment && std::strcmp(moment, mo)) continue;
        if (c.seg < 0) { ecarts++; WFS_ERR("banc campagne : %s introuvable", c.nom); continue; }
        if (!std::strcmp(mo, "nuit") && c.cam == 1) continue;
        setenv("WFS_MOMENT", mo, 1);
        setenv("WFS_DEPART", std::to_string(c.seg - (!std::strcmp(mo, "nuit") ? (int64_t)(c.reculNuit / SEG_M) : 0)).c_str(), 1);
        if (!g.init(r, "golf", "campagne")) { ecarts++; continue; }
        g.setAutopilot(false); g.camMode = c.cam; g.allerA(Ecran::Course);
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
        if (c.vue == 1 && campagne::sommet(carte::g_vueCarte.cible)) {   // 30 m au-dessus de la route (au-dessus des sapins), vers la cime ; champ de 38° : les sommets, pas le bord du décor
            const TrackPoint o = t.point((double)haut * SEG, 0);
            carte::g_vueCarte.oeil = {(float)o.x, (float)o.y + 30, (float)o.z}; carte::g_vueCarte.cible.y -= 120; carte::g_vueCarte.fov = 38; carte::g_vueCarte.actif = true;
            vec3 f = carte::g_vueCarte.cible - carte::g_vueCarte.oeil; f.y = 0; f = normalize(f);
            carte::g_vueCarte.cible = carte::g_vueCarte.cible + vec3{-f.z, 0, f.x} * 250.f;   // 250 m à droite : le grand versant, pas le bord du décor
        }
        if (c.vue == 2) {   // épingle : œil 45 m au-dessus de la route, 70 m avant l'épingle ; visé : centre du lacet
            int64_t fin = ep; while (fin < lac->fin && std::abs(t.segAt(fin).curve) > 10) fin++;
            const TrackPoint a = t.point((double)ep * SEG, 0), b = t.point((double)fin * SEG, 0), m = t.point((double)(ep + fin) / 2 * SEG, 0),
                             o = t.point((double)(ep - (int64_t)(70 / SEG_M)) * SEG, 0);
            carte::g_vueCarte.cible = {(float)(a.x + b.x + m.x) / 3, (float)(a.y + b.y + m.y) / 3, (float)(a.z + b.z + m.z) / 3};
            carte::g_vueCarte.oeil = {(float)o.x, (float)o.y + 45, (float)o.z}; carte::g_vueCarte.fov = 55; carte::g_vueCarte.actif = true;
        }
        g.render(404); g.render(404.02);
        const std::string nom = std::string(c.nom) + (!std::strcmp(mo, "nuit") ? "-nuit" : "") + (basse ? "-basse" : "");
        r.capturePng(out + "/" + nom + ".png");
        carte::g_vueCarte.actif = false;
        WFS_LOG("capture %s : segment %lld, %.0f m, %u appels de dessin", nom.c_str(), (long long)c.seg, g.player().dist * M, r.appelsDessin());
        g.shutdown();
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    if (basse) { ecrans::reglages().qualite = ecrans::QUALITE_HAUTE; r.qualiteBasse = false; }
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
            lieux.push_back({"carte-ville-fontaine", s.debut, -20, true, 1});   // WFS-212 (RG-05) : 20 m dans l'anneau, l'îlot et sa fontaine devant
            lieux.push_back({"carte-ville-fontaine-dessus", s.debut, -3, false, 2});   // WFS-212 : place de la fontaine vue de dessus (70 m)
        }
    }
    {   // WFS-176 : casino, centre commercial, garages (jour et nuit), un promeneur et son chien, pigeons d'un carrefour
        const ville::Plan& P = ville::adapter(mapById("ville"), t);
        int ng = 0;
        for (const ville::Lieu& l : P.lieux) {
            if (l.t == ville::CASINO) lieux.push_back({"carte-ville-casino", l.seg, 16, true});
            if (l.t == ville::CENTRE_COMMERCIAL) lieux.push_back({"carte-ville-centre-commercial", l.seg, 15, true});
            if (l.t == ville::GARAGE && ng++ == 0) lieux.push_back({"carte-ville-garage", l.seg, 6, true});
        }
        if (!P.promeneurs.empty()) lieux.push_back({"carte-ville-chien", (int64_t)P.promeneurs[0].s, 12, false});
        if (!P.carrefours.empty()) lieux.push_back({"carte-ville-pigeons", P.carrefours[0].croise - (int64_t)(15 / SEG_M), -20, false});
        // WFS-269 : gros plans de l'occlusion précalculée : pied de façade d'un immeuble de plus de 15 m, vu du trottoir à hauteur
        // d'homme (cam 3) ; 3 immeubles répartis sur le tour, le premier aussi de nuit (WFS_SANS_AO : même vue sans)
        std::vector<int64_t> hauts;
        for (int64_t a = 0; a < (int64_t)P.objets.size(); a++)
            if (a >= (int64_t)P.voie.size() || P.voie[(size_t)a] == 0)
                for (const ville::Objet& o : P.objets[(size_t)a]) if (o.t == ville::IMMEUBLE && o.h >= 15 && o.len >= 12) { hauts.push_back(a * 2 + (o.side > 0)); break; }
        for (int n = 0; n < 3 && !hauts.empty(); n++) for (int c = 0; c < 2; c++) {   // les deux côtés de la rue (l'un est à l'ombre)
            const int64_t h = hauts[hauts.size() * (size_t)(2 * n + 1) / 6];
            lieux.push_back({"carte-ville-ao-" + std::to_string(n + 1) + (c ? "-d" : "-g"), (h & ~1ll) | c, 0, n == 0, 3});
        }
    }
    const char* seul = std::getenv("WFS_VILLE_LIEU");   // essai local : un seul lieu (préfixe du nom)
    for (const Lieu& l : lieux) for (int n = 0; n < (l.nuit ? 2 : 1); n++) {
        const std::string nom = l.nom + (n ? "-nuit" : "") + (l.cam == 3 && std::getenv("WFS_SANS_AO") ? "-sans" : "");
        if (seul && nom.rfind(seul, 0) != 0) continue;
        const int64_t segL = l.cam == 3 ? l.seg / 2 : l.seg;   // cam 3 : seg = segment · 2 + côté
        setenv("WFS_MOMENT", n ? "nuit" : "jour", 1);
        setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, segL - (int64_t)(l.avant / SEG_M))).c_str(), 1);
        courseBenchRules(true);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("banc ville : chargement impossible"); continue; }
        g.setAutopilot(false); g.camMode = l.cam >= 2 ? 0 : l.cam; g.allerA(Ecran::Course);
        for (int i = 0; i < 90; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(500 + i / 60.0); }   // décompte : voiture arrêtée
        if (l.cam == 2) {   // WFS-212 : vue de dessus centrée sur le rond-point (centre de l'anneau), 70 m au-dessus
            const ville::Plan& P = ville::adapter(mapById("ville"), t);
            for (const ville::RondPoint& rp : P.ronds) {
                const TrackPoint c = t.point((double)rp.centre * SEG, 0), d = t.point((double)rp.centre * SEG, rp.rayon * (t.segAt(rp.centre).curve > 0 ? 1.f : -1.f));
                (void)c; g_vueHaute.cible = {(float)d.x, (float)d.y, (float)d.z}; g_vueHaute.oeil = g_vueHaute.cible + vec3{0, 70, 0.01f}; g_vueHaute.actif = true;
                break;
            }
        }
        if (l.cam == 3) {   // WFS-269 : gros plan : œil sur le trottoir (2 m du bord), 1,6 m, 6 m avant ; visé : pied de la façade (6,5 m du bord) 8 m plus loin
            const double s = (l.seg & 1) ? 1 : -1;
            const TrackPoint o = t.point((double)(segL - (int64_t)(6 / SEG_M)) * SEG, s * (RW + 2)), c = t.point((double)(segL + (int64_t)(8 / SEG_M)) * SEG, s * (RW + 6.5));
            g_vueHaute.oeil = {(float)o.x, (float)o.y + 1.6f, (float)o.z}; g_vueHaute.cible = {(float)c.x, (float)c.y + 3.f, (float)c.z};
            g_vueHaute.fov = 55; g_vueHaute.haut = {0, 1, 0}; g_vueHaute.actif = true;
        }
        g.render(502); g.render(502.02); r.capturePng(out + "/" + nom + ".png");
        g_vueHaute.actif = false; g_vueHaute.fov = 50; g_vueHaute.haut = {0, 0, -1};
        WFS_LOG("capture %s : segment %lld, %.0f m avant, %.0f m, %u appels de dessin", nom.c_str(), (long long)l.seg, l.avant, g.player().dist * M, r.appelsDessin());   // WFS-175 : coût
        g.shutdown();
    }
    courseBenchRules(false);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

// WFS-223 (RG-01, RG-06) : chaque rond-point des cartes au tracé 2 : 15 m avant l'entrée (caméra de course) et vue haute,
// de jour et de nuit, qualité Haute puis Basse (WFS_QUALITE_BASSE=1) : captures rondpoint-<carte>-<vue>-<moment>
int benchRondsPoints(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    const bool basse = std::getenv("WFS_QUALITE_BASSE") != nullptr;
    for (const char* carte : {"ville", "campagne", "plage", "circuit"}) {
        Track t; t.build(mapById(carte), trace::versionCarte(carte));
        int n = 0;
        for (const trace::Section& s : t.sections()) {
            if (s.type != trace::Type::RondPoint) continue;
            n++;
            int64_t entree = s.debut;   // entrée de l'anneau : la courbure change de sens (fin de la courbe d'approche)
            for (int64_t a = s.debut + 1; a < s.fin; a++) if (t.segAt(a).curve * t.segAt(s.debut + 2).curve < 0) { entree = a; break; }
            for (const char* mo : {"jour", "nuit"}) for (int cam : {0, 1, 2}) {   // 2 : caméra de course dans l'anneau (milieu)
                setenv("WFS_MOMENT", mo, 1);
                setenv("WFS_DEPART", std::to_string(cam == 2 ? (s.debut + s.fin) / 2 : entree - (int64_t)(15 / SEG_M)).c_str(), 1);
                r.qualiteBasse = basse;   // lue par les décors à l'initialisation
                if (!g.init(r, "golf", carte)) { ecarts++; continue; }
                g.setAutopilot(false); g.camMode = cam == 2 ? 0 : cam; g.allerA(Ecran::Course);
                for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
                g.render(404); g.render(404.02);
                const std::string nom = std::string("rondpoint-") + carte + (n > 1 ? std::to_string(n) : "") + (cam == 1 ? "-haut" : cam == 2 ? "-anneau" : "-course") + "-" + mo + (basse ? "-basse" : "");
                r.capturePng(out + "/" + nom + ".png");
                WFS_LOG("capture %s : segment %lld (anneau de %.0f m), %u appels de dessin", nom.c_str(), (long long)s.debut, s.rayon, r.appelsDessin());
                g.shutdown();
            }
        }
        WFS_LOG("ronds-points : %s : %d", carte, n);
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    return ecarts;
}

} // namespace wfs
