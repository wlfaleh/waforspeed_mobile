// WaForSpeed — WFS-594 : banc « routes-sols » (WFS_BANC=routes-sols) : chaussée, marquages, bordures, trottoirs et sols vus de
// près, aux mêmes endroits avant (main) et après (branche), de jour et de nuit, en qualité Haute (et une vue en Basse) :
//   routes-sols-<carte>-<moment>-<vue>.png ; carte : ville, corniche (« plage »), neige (« noel-test ») ;
//   vue : cockpit (vue du conducteur), arriere (vue Poursuite), rasante (caméra à 2 m du sol, vers la rive droite de la chaussée),
//   rasante-gauche (même hauteur, vers la rive gauche), bascote (œil à 10,5 m à droite, 1,6 m de haut : trottoir, gravier, herbe, sable, neige).
// La voiture roule au pilote automatique (même graine) : même endroit de la route d'une exécution à l'autre.
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "../core/carte_cuite.h"
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/game.h"
#include "../game/ressenti.h"
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "../world/cartes_v2/carte_donnees.h"
#include "renderer.h"

namespace wfs {
namespace {
// paquet de la carte de neige (« noel-test »), écrit dans dossierPaquets() comme le banc « neige » (cartes_v2/neige_banc.cpp)
void cuireNeige(const std::string& racine) {
    carte::Paquet p; std::string err;
    if (!cartes_v2::sourcesCarte(racine, "noel-test", carte::FormatTexture::RGBA8, p, err)) { WFS_ERR("routes-sols : noel-test : %s", err.c_str()); return; }
    const std::vector<uint8_t> o = carte::ecrire(p);
    std::ofstream(carte::dossierPaquets() + "/" + carte::nomFichier("noel-test", carte::FormatTexture::RGBA8), std::ios::binary).write((const char*)o.data(), (std::streamsize)o.size());
}
} // namespace

int benchRoutesSols(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    // carte de neige (« noel-test ») : lue de son paquet, cuit ici comme le banc « neige » (WFS-346)
    std::error_code ec;
    std::filesystem::create_directories(carte::dossierPaquets(), ec);
    cuireNeige(racine);
    setenv("WFS_CARTES_TEST", "1", 1);
    const bool neige = cartes_v2::decouvrir() >= 1 && cartes_v2::donnees("noel-test");
    if (!neige) WFS_ERR("routes-sols : carte de neige absente (paquet noel-test)");
    struct Cas { const char* nom; const char* carte; int qualite; double secondes; };
    const Cas cas[] = {
        {"ville", "ville", ecrans::QUALITE_HAUTE, 9.0},
        {"corniche", "plage", ecrans::QUALITE_HAUTE, 9.0},
        {"neige", "noel-test", ecrans::QUALITE_HAUTE, 9.0},
    };
    const bool reduit = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;   // pas de gouttes ni de tremblement : seule la surface change d'une capture à l'autre
    const VueCamera vue0 = vueCamera();
    int erreurs = 0;
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        setenv("WFS_METEO", "sec", 1);
        for (const Cas& c : cas) {
            if (!neige && std::string(c.carte) == "noel-test") continue;
            if (const char* f = std::getenv("WFS_RS_CARTE"); f && std::string(f) != c.nom) continue;   // essai local : une seule carte
            if (const char* f = std::getenv("WFS_RS_MOMENT"); f && std::string(f) != moment) continue;
            ecrans::reglages().qualite = c.qualite;
            if (!g.init(r, "golf", c.carte)) { WFS_ERR("routes-sols : chargement %s impossible", c.carte); erreurs++; continue; }
            choisirVue(VueCamera::Poursuite);
            g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
            double t = 800;
            const int n = (int)(c.secondes * 60);
            for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); }
            const std::string base = std::string("routes-sols-") + c.nom + "-" + moment;
            auto capture = [&](const std::string& vue) {
                g.render(t + 0.001); g.render(t + 0.002);
                r.capturePng(out + "/" + base + "-" + vue + ".png");
                WFS_LOG("capture %s-%s : %.0f km/h, %.0f m, qualité %d", base.c_str(), vue.c_str(), g.player().kmh(), g.player().dist * M, c.qualite);
            };
            capture("arriere");
            choisirVue(VueCamera::Cockpit); g.render(t + 0.01);
            capture("cockpit");
            choisirVue(VueCamera::Poursuite);
            // vue rasante : caméra de vitrine à 2 m du sol, 4 m devant et sur le côté de la voiture, tournée vers la rive (chaussée,
            // marquage de rive, bordure, trottoir ou bas-côté) ; « orbit » = direction de l'œil autour de la voiture
            const Game::Vue v = g.calculerVue(t);
            const float cap = std::atan2(v.fwd.x, v.fwd.z);
            const vec3 droite = normalize(vec3{-v.fwd.z, 0, v.fwd.x});
            g.camMode = 2; g.orbitHauteur = 2.0f; g.orbitRecul = 4.0f;
            g.orbit = cap + 0.5f; g.orbitVise = v.fwd * 6.0f + droite * 5.5f - vec3{0, 0.55f, 0};
            capture("rasante");
            g.orbit = cap - 0.5f; g.orbitVise = v.fwd * 6.0f - droite * 5.5f - vec3{0, 0.55f, 0};
            capture("rasante-gauche");
            // bas-côté de près : œil à 10,5 m à droite de la voiture, 1,6 m de haut, regard vers le sol à 14 m de l'axe de la voiture
            // (bord du trottoir, bande de gravier, herbe ; sable de la plage sur la Corniche ; neige sur la carte d'hiver)
            g.orbitHauteur = 1.6f; g.orbitRecul = 10.5f; g.orbit = std::atan2(droite.x, droite.z);
            g.orbitVise = droite * 14.0f + v.fwd * 4.0f - vec3{0, 0.45f, 0};
            capture("bascote");
            g.camMode = 0; g.orbitVise = vec3{}; g.orbitHauteur = 1.7f; g.orbitRecul = 6.5f;
            g.shutdown();
        }
    }
    // qualité Basse : même vue arrière en Ville de jour (aucune baisse attendue, RG-07)
    setenv("WFS_MOMENT", "jour", 1);
    ecrans::reglages().qualite = ecrans::QUALITE_BASSE;
    if (g.init(r, "golf", "ville")) {
        choisirVue(VueCamera::Poursuite);
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
        double t = 800;
        for (int i = 0; i < 540; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); }
        g.render(t + 0.001); g.render(t + 0.002);
        r.capturePng(out + "/routes-sols-ville-jour-basse.png");
        WFS_LOG("capture routes-sols-ville-jour-basse : %.0f km/h, %.0f m", g.player().kmh(), g.player().dist * M);
        g.shutdown();
    } else erreurs++;
    ecrans::reglages().qualite = ecrans::QUALITE_HAUTE;
    choisirVue(vue0);
    reglagesRessenti().reduireMouvements = reduit;
    unsetenv("WFS_METEO"); unsetenv("WFS_MOMENT"); unsetenv("WFS_CARTES_TEST");
    return erreurs + (neige ? 0 : 1);
}

} // namespace wfs
