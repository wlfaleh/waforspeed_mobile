// WaForSpeed — WFS-450 (RG-06) : banc des lumières de nuit EN MOUVEMENT, en Basse (WFS_BANC=lumieres-mouvement).
// Le défaut du ticket ne se voit pas sur une capture fixe : la chaussée éclairée par les lampadaires devenait noire d'un coup, puis
// se rallumait d'un coup, quand deux voitures suivaient le joueur de près (Corniche) ; les phares d'une voiture voisine
// apparaissaient d'un coup (Ville). Deux scènes, chacune rejouée « avant » (phares::avant450 : ordre, quota et apparition d'avant le
// ticket) puis « après », même trajet, 45 images à 10 images par seconde (4,5 s) :
//   - corniche : Corniche de nuit, de 85 à 105 km/h, 2 adversaires collés derrière (7 m et 12 m, files de gauche et de droite) ;
//   - ville    : Ville de nuit, ≈ 90 km/h, une voiture qui remonte sur la file voisine, de 85 m à 4 m derrière le joueur.
// Mesure : luminosité moyenne (luma Rec. 709 des pixels, 0 à 255) de la chaussée autour de la voiture (rectangle du milieu de
// l'image, sous l'horizon, au-dessus du compteur), sur chaque image. RG-06 : après le ticket, aucune chute de plus de 20 % d'une
// image à la suivante (sinon écart). Captures : mouv-<scène>-<avant|apres>-NN.png ; mesures : mouv-<scène>.txt et journal.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../core/log.h"
#include "../game/camera_vue.h"
#include "../game/course5.h"
#include "../game/game.h"
#include "../ui/ecrans/reglages.h"
#include "phares.h"
#include "renderer.h"

namespace wfs {
namespace {

constexpr int IMAGES = 45, PAS_PAR_IMAGE = 6;   // 10 images par seconde pendant 4,5 s (simulation à 60 pas par seconde)
constexpr double CHUTE_MAX = 0.20;              // RG-06

// luminosité moyenne du rectangle « chaussée autour de la voiture » d'une capture (−1 : fichier illisible)
double luminositeChaussee(const std::string& png) {
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load(png.c_str(), &w, &h, &n, 3);
    if (!px) return -1;
    const int x0 = w / 4, x1 = w * 3 / 4, y0 = h / 2, y1 = h * 7 / 10;
    double somme = 0;
    for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) { const unsigned char* p = px + (size_t)(y * w + x) * 3; somme += 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]; }
    stbi_image_free(px);
    return somme / std::max(1, (x1 - x0) * (y1 - y0));
}

} // namespace

int benchLumieresMouvement(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    const VueCamera vue0 = vueCamera();
    const int qualite0 = ecrans::reglages().qualite;
    const ModeCourse mode0 = modeCourse(); const int km0 = kmCourse5();
    ecrans::reglages().qualite = ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
    modeCourse() = ModeCourse::Course5; kmCourse5() = 10;
    int erreurs = 0;
    struct Scene { const char* nom; const char* carte; };
    for (const Scene& sc : {Scene{"corniche", "plage"}, Scene{"ville", "ville"}}) {
        const bool corniche = sc.carte[0] == 'p';
        std::string texte = std::string("WFS-450, banc lumieres-mouvement, scène ") + sc.nom + " : luminosité moyenne de la chaussée autour de la voiture (0 à 255)\nimage | km/h | avant | après\n";
        std::vector<double> lum[2], kmh;
        for (int passe = 0; passe < 2; passe++) {
            phares::avant450 = passe == 0;
            setenv("WFS_DEPART", "0", 1);
            if (!g.init(r, "golf", sc.carte)) { WFS_ERR("lumieres-mouvement : chargement de %s impossible", sc.carte); erreurs++; continue; }
            choisirVue(VueCamera::Poursuite); g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
            double t = 900;
            auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
            // adversaires tenus derrière le joueur dès le départ (corniche) ; en ville, la voiture voisine attend loin derrière
            auto suiveurs = [&](double ecartVoisine) {
                if (corniche) course5BancSuiveurs({{-7.0, 0}, {-12.0, 2}});
                else course5BancSuiveurs({{ecartVoisine, 0}});
            };
            suiveurs(-85);
            const double depart = corniche ? 85 : 90;
            bool lance = false;
            for (int i = 0; i < 60 * 40 && !lance; i++) { lance = g.player().kmh() >= depart; if (!lance) image(i % 8 == 0); }
            if (!lance) { WFS_ERR("lumieres-mouvement : %s, %.0f km/h non atteints en 40 s", sc.nom, depart); erreurs++; }
            for (int i = 0; i < 12; i++) image(i % 2 == 1);   // l'historique des voitures notées (image précédente) est en place
            for (int k = 0; k < IMAGES; k++) {
                for (int i = 0; i < PAS_PAR_IMAGE; i++) {
                    const double u = (double)(k * PAS_PAR_IMAGE + i) / (IMAGES * PAS_PAR_IMAGE);
                    const double cible = corniche ? 85 + 20 * u : 90;   // corniche : de 85 à 105 km/h en 4,5 s
                    g.setAutopilot(g.player().kmh() < cible);            // gaz coupé au-dessus de la vitesse visée
                    suiveurs(-85 + 81 * u);
                    image(i % 2 == 1);                                   // 30 images dessinées par seconde, comme sur le téléphone
                }
                char nom[96]; std::snprintf(nom, sizeof nom, "mouv-%s-%s-%02d", sc.nom, passe == 0 ? "avant" : "apres", k);
                const std::string png = out + "/" + nom + ".png";
                r.capturePng(png);
                const double l = luminositeChaussee(png);
                if (l < 0) { WFS_ERR("lumieres-mouvement : capture illisible %s", png.c_str()); erreurs++; }
                lum[passe].push_back(l); if (passe == 0) kmh.push_back(g.player().kmh());
                WFS_LOG("capture %s : %.0f km/h, luminosité de la chaussée %.1f, %d lumières", nom, g.player().kmh(), l, r.projecteursImage());
            }
            g.setAutopilot(false); course5BancSuiveurs({});
            g.shutdown();
        }
        double chute[2] = {0, 0};
        for (int passe = 0; passe < 2; passe++)
            for (size_t k = 1; k < lum[passe].size(); k++) if (lum[passe][k - 1] > 0) chute[passe] = std::max(chute[passe], 1 - lum[passe][k] / lum[passe][k - 1]);
        for (size_t k = 0; k < lum[0].size() && k < lum[1].size(); k++) {
            char l[96]; std::snprintf(l, sizeof l, "%02zu | %.0f | %.1f | %.1f\n", k, k < kmh.size() ? kmh[k] : 0.0, lum[0][k], lum[1][k]);
            texte += l;
        }
        char fin[200]; std::snprintf(fin, sizeof fin, "plus forte chute d'une image à la suivante : avant %.0f %%, après %.0f %% (RG-06 : 20 %% au plus après)\n", chute[0] * 100, chute[1] * 100);
        texte += fin;
        if (FILE* f = std::fopen((out + "/mouv-" + sc.nom + ".txt").c_str(), "w")) { std::fputs(texte.c_str(), f); std::fclose(f); }
        WFS_LOG("lumieres-mouvement : %s : plus forte chute de luminosité d'une image à la suivante : avant %.0f %%, après %.0f %%", sc.nom, chute[0] * 100, chute[1] * 100);
        if (chute[1] > CHUTE_MAX) { WFS_ERR("lumieres-mouvement : %s : chute de %.0f %% après le ticket (RG-06 : 20 %% au plus)", sc.nom, chute[1] * 100); erreurs++; }
    }
    phares::avant450 = false;
    ecrans::reglages().qualite = qualite0; ecrans::appliquerQualiteNiveau(r);
    modeCourse() = mode0; kmCourse5() = km0;
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    choisirVue(vue0);
    WFS_LOG("lumieres-mouvement (WFS-450) : %d écart(s)", erreurs);
    return erreurs;
}

} // namespace wfs
