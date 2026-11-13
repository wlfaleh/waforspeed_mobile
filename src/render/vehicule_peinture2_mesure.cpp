// WaForSpeed — WFS-488 (RC-E18-03) : mesure des images par seconde en qualité Basse (WFS_BANC=peinture2-mesure), pour comparer
// main et la branche avec la couleur secondaire posée sur la voiture du joueur (WFS_PEINTURE2=<couleur du catalogue>, lue par
// Game::setCar ; sans effet sur main). Ville, jour, 208 en course au pilote automatique, caméra de poursuite : 180 images de
// chauffe, puis WFS_PEINTURE2_IMAGES (600 par défaut) images chronométrées (update + render). Résultat : moyenne en img/s et
// « 1 % les plus lents » (img/s de la moyenne des 1 % d'images les plus longues), au journal et dans <sortie>/peinture2-mesure.txt.
// Ne dépend d'aucun autre code du ticket : le même fichier se compile sur main pour la mesure « avant ». Domaine : E10 Véhicules.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {

void benchPeinture2Mesure(Game& g, Renderer& r, const std::string& out) {
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    R.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
    const char* p2 = std::getenv("WFS_PEINTURE2");
    const int N = std::getenv("WFS_PEINTURE2_IMAGES") ? std::max(100, std::atoi(std::getenv("WFS_PEINTURE2_IMAGES"))) : 600;
    if (!g.init(r, "p208", "ville")) { WFS_ERR("peinture2-mesure : chargement impossible"); R.qualite = q0; ecrans::appliquerQualiteNiveau(r); return; }
    g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
    double t = 4000;
    for (int i = 0; i < 180; i++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }
    std::vector<double> ms; ms.reserve((size_t)N);
    for (int i = 0; i < N; i++) {
        const double t0 = nowSeconds();
        g.update(1.0 / 60); g.render(t); t += 1.0 / 60;
        ms.push_back((nowSeconds() - t0) * 1000);
    }
    double somme = 0;
    for (double x : ms) somme += x;
    const double moyenne = somme / ms.size();
    std::vector<double> tri = ms; std::sort(tri.begin(), tri.end(), std::greater<double>());
    const size_t n1 = std::max<size_t>(1, tri.size() / 100);
    double lents = 0;
    for (size_t k = 0; k < n1; k++) lents += tri[k];
    lents /= n1;
    char b[512];
    std::snprintf(b, sizeof b, "peinture2-mesure : Basse, Ville, 208, couleur 2 %s : %d images, moyenne %.2f ms = %.3f img/s ; 1 %% les plus lents (%zu images) %.2f ms = %.3f img/s",
                  p2 && *p2 ? p2 : "aucune", N, moyenne, 1000.0 / moyenne, n1, lents, 1000.0 / lents);
    WFS_LOG("%s (WFS-488)", b);
    if (FILE* f = std::fopen((out + "/peinture2-mesure.txt").c_str(), "w")) { std::fprintf(f, "%s\n", b); std::fclose(f); }
    g.setAutopilot(false);
    g.shutdown();
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
}

} // namespace wfs
