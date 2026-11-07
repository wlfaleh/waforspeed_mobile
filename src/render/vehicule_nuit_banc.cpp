// WaForSpeed — WFS-270 (RG-06, ANO-148) : scène du banc de nuit avec des voitures en sens inverse (Campagne, autoroute) :
// une à moins de 70 m (vraie lumière des phares, éblouissement doux de face, RG-01 de WFS-226) et une entre 70 et 160 m
// (lumière peinte au sol, RG-02 de WFS-226). Domaine : E10 Véhicules ; appelée par benchNuit (bench_nuit.cpp).
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "phares.h"
#include "renderer.h"

namespace wfs {
namespace {

bool g_releve = false;                          // relevé actif (banc seulement)
std::vector<phares::PhareAutre> g_voitures;     // voitures notées par Renderer::feuxVoiture à la dernière image

// Lit, juste avant le système de nuit (950, qui vide la liste), les autres voitures dessinées à cette image : trafic,
// adversaires, voitures en sens inverse de la Campagne (systèmes d'ordre < 949). Lecture seule ; rien hors du banc.
class VehiculeNuitBanc final : public System {
public:
    const char* name() const override { return "vehicule-nuit-banc"; }
    void render(Renderer&, const GameView&) override { if (g_releve) g_voitures.assign(phares::autres().begin(), phares::autres().end()); }
};

} // namespace

WFS_SYSTEM(VehiculeNuitBanc, 949)

void benchNuitSensInverse(Game& g, Renderer& r, const std::string& out) {
    Track t; t.build(mapById("campagne"), trace::versionCarte("campagne"));
    int64_t au = -1;   // début de l'autoroute (WFS-179) ; 300 m plus loin : voitures en sens inverse (comme cartes_banc.cpp)
    for (const trace::Section& s : t.sections()) if (s.autoroute && au < 0) au = s.debut;
    if (au < 0) { WFS_ERR("nuit : autoroute de la Campagne introuvable (sens inverse)"); return; }
    setenv("WFS_DEPART", std::to_string(au + (int64_t)(300 / SEG_M)).c_str(), 1);
    const int qualite = ecrans::reglages().qualite;
    ecrans::reglages().qualite = ecrans::QUALITE_BASSE;   // 2 voitures à vraie lumière (RG-02 de WFS-226), le cas le plus sévère
    if (!g.init(r, "golf", "campagne")) { WFS_ERR("nuit : chargement campagne impossible (sens inverse)"); unsetenv("WFS_DEPART"); return; }
    g.setAutopilot(false); g.camMode = 1; g.allerA(Ecran::Course);
    g_releve = true;
    double tps = 900;
    float pres = -1, loin = -1;
    for (int i = 0; i < 2400; i++) {   // 40 s au plus : voiture arrêtée, les voitures en sens inverse passent à 94-119 km/h
        g.update(1.0 / 60); tps += 1.0 / 60;
        if (i % 4) continue;
        g.render(tps);
        if (i < 240) continue;   // après le décompte
        const vec3 j = g.calculerVue(tps).carPos;
        pres = loin = -1;
        for (const phares::PhareAutre& a : g_voitures) {
            if (dot(a.avant, j - a.centre) <= 0) continue;   // sens inverse : la voiture vient vers le joueur
            const float d = length(a.centre - j);
            if (d < 60 && d > 20) pres = d;
            else if (d >= 80 && d <= 150) loin = d;
        }
        if (pres > 0 && loin > 0) break;
    }
    g.render(tps + 0.001); g.render(tps + 0.002); r.capturePng(out + "/phares-sens-inverse-campagne-haut.png");
    WFS_LOG("capture phares-sens-inverse-campagne-haut : sens inverse à %.0f m (vraie lumière) et à %.0f m (lumière au sol) %s",
            pres, loin, pres > 0 && loin > 0 ? "OK" : "INCOMPLET");
    if (!(pres > 0 && loin > 0)) WFS_ERR("nuit : scène sens inverse incomplète (%.0f m, %.0f m)", pres, loin);
    g.camMode = 0;
    g.render(tps + 0.003); g.render(tps + 0.004); r.capturePng(out + "/phares-sens-inverse-campagne-0.png");
    g_releve = false; g_voitures.clear();
    g.shutdown();
    unsetenv("WFS_DEPART");
    ecrans::reglages().qualite = qualite;
}

} // namespace wfs
