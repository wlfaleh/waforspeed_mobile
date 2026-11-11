// WaForSpeed — WFS-423 (épic E4) : banc court des lumières de nuit en Basse (WFS_BANC=lumieres-basse), copie du banc de WFS-421.
// Ville (rue, lampadaires) et Corniche (lampadaires de plage, eau visible) de nuit, temps sec, vue Poursuite, voiture qui roule au
// pilote automatique jusqu'à ≈ 20 m du lampadaire suivant. Basse : suite de 3 images à 0,2 s d'écart en roulant sous les
// lampadaires (RG-03 : aucune lumière qui saute, caisse éclairée) ; Moyenne et Haute : une image (RG-04 : identiques).
// Captures : lumbasse-<carte>-basse-<1|2|3>.png, lumbasse-<carte>-<moyenne|haute>.png.
#include <cmath>
#include <cstdlib>
#include <string>
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../game/ressenti.h"
#include "../ui/ecrans/reglages.h"
#include "eclairage.h"
#include "renderer.h"

namespace wfs {

int benchLumieresBasse(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    setenv("WFS_METEO", "sec", 1);   // pas de pluie : seule la lumière au sol change d'une qualité à l'autre
    const VueCamera vue0 = vueCamera();
    const bool reduit = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;
    const int qualite0 = ecrans::reglages().qualite;
    int erreurs = 0;
    struct Q { int q; const char* nom; };
    const Q qualites[] = {{ecrans::QUALITE_BASSE, "basse"}, {ecrans::QUALITE_MOYENNE, "moyenne"}, {ecrans::QUALITE_HAUTE, "haute"}};
    for (const char* carte : {"ville", "plage"}) for (const Q& q : qualites) {
        Track tr; tr.build(mapById(carte), trace::versionCarte(carte));   // même tracé que la partie (longueur de la boucle)
        ecrans::reglages().qualite = q.q; ecrans::appliquerQualiteNiveau(r);
        setenv("WFS_DEPART", "0", 1);
        if (!g.init(r, "golf", carte)) { WFS_ERR("lumieres-basse : chargement de %s impossible", carte); erreurs++; continue; }
        choisirVue(VueCamera::Poursuite); g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
        double t = 900;
        auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
        // lampadaire (avec mât : pas une tête seule) le plus proche devant la voiture, en mètres
        auto prochain = [&]() {
            const int64_t len = (int64_t)tr.segmentCount(), aP = (int64_t)std::floor(g.player().dist / SEG);
            double best = 1e9;
            for (const Lampadaire& l : lampadaires()) {
                if (l.style == StyleLampadaire::Tete) continue;
                const int64_t d = ((l.seg - aP) % len + len) % len;
                best = std::min(best, (double)d * SEG * M - std::fmod(g.player().dist, SEG) * M);
            }
            return best;
        };
        for (int i = 0; i < 60 * 4; i++) image(i % 8 == 0);   // décompte et départ
        bool vu = false;
        for (int i = 0; i < 60 * 40 && !vu; i++) { const double d = prochain(); vu = d <= 21 && d >= 14; if (!vu) image(i % 8 == 0); }
        if (!vu) { WFS_ERR("lumieres-basse : %s, aucun lampadaire à 14-21 m en 40 s", carte); erreurs++; }
        const int nImages = q.q == ecrans::QUALITE_BASSE ? 3 : 1;
        for (int k = 0; k < nImages; k++) {
            if (k > 0) for (int i = 0; i < 12; i++) image(i == 11);   // 0,2 s plus loin
            g.render(t + 0.001); g.render(t + 0.002);
            const std::string nom = std::string("lumbasse-") + carte + "-" + q.nom + (nImages > 1 ? "-" + std::to_string(k + 1) : "");
            r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s : %.0f km/h, %.0f m, lampadaire suivant à %.1f m, %u appels de dessin", nom.c_str(), g.player().kmh(),
                    g.player().dist * M, prochain(), r.appelsDessin());
        }
        g.shutdown();
    }
    ecrans::reglages().qualite = qualite0; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT"); unsetenv("WFS_METEO");
    choisirVue(vue0); reglagesRessenti().reduireMouvements = reduit;
    return erreurs;
}

} // namespace wfs
