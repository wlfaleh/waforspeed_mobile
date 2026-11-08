// Regard du conducteur en vue Cockpit (WFS-235) : champ de vision horizontal de 75 à 90° quelle que soit la forme de l'écran
// (RG-02), regard un peu plongeant selon le réglage « Regard cockpit » (RG-03), regard qui suit le virage sans dépasser ±12°,
// distance de visée selon la vitesse, mouvement de tête borné (RG-04). WFS-282 (RG-00) : intérieurs remis à l'état d'avant
// WFS-235 (planche de bord et carrosserie de WFS-197 / 201 / 202), champ de configgame (72° en hauteur).
// usage : test_regard_cockpit
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/game/camera_vue.h"
#include "../src/game/cars.h"
#include "../src/render/vehicule_cockpit.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
constexpr float DEG = 3.14159265f / 180.f;

int main() {
    {   // WFS-282 (RG-00) : champ de la vue Cockpit = celui de configgame.json (72° en hauteur), comme avant WFS-235
        VERIFIE(std::fabs(fovVue(VueCamera::Cockpit, 0, false) - 72.f) < 0.5f, "cockpit : champ vertical %.1f° (72° attendus)", fovVue(VueCamera::Cockpit, 0, false));
    }
    {   // RG-02, RG-03 : regard sous l'horizon, Bas < Normal < Haut, jamais au-dessus de l'horizon
        const float b = inclinaisonRegard(RegardCockpit::Bas), n = inclinaisonRegard(RegardCockpit::Normal), h = inclinaisonRegard(RegardCockpit::Haut);
        VERIFIE(b < n && n < h && h <= 0.f, "inclinaisons : bas %.1f°, normal %.1f°, haut %.1f°", b / DEG, n / DEG, h / DEG);
        VERIFIE(n < -2 * DEG && n > -6 * DEG, "regard normal légèrement plongeant (%.1f°)", n / DEG);
        // l'horizon reste dans l'image (au-dessus du bas) pour les 3 réglages, sur un écran 2400 × 1080
        const float demi = fovVue(VueCamera::Cockpit, 0, false) * DEG / 2;
        for (float a : {b, n, h}) VERIFIE(-a < demi * 0.8f, "horizon visible : regard à %.1f°, demi-champ %.1f°", a / DEG, demi / DEG);
    }
    {   // RG-02 : regard qui suit le virage, borné ; distance de visée de 12 à 40 m
        VERIFIE(std::fabs(lacetVirage(10 * DEG) - 6 * DEG) < 1e-4f, "60 %% d'un virage à 10° : %.2f°", lacetVirage(10 * DEG) / DEG);
        VERIFIE(std::fabs(lacetVirage(80 * DEG) - 12 * DEG) < 1e-4f && std::fabs(lacetVirage(-80 * DEG) + 12 * DEG) < 1e-4f, "lacet borné à ±12°");
        VERIFIE(lacetVirage(0) == 0.f, "ligne droite : regard droit devant");
        VERIFIE(distanceRegard(0) == 12.f && distanceRegard(500) == 40.f && std::fabs(distanceRegard(72) - 30.f) < 1e-3f, "distance de visée : %.1f / %.1f / %.1f m",
                distanceRegard(0), distanceRegard(72), distanceRegard(500));
    }
    {   // RG-04 : tête vers l'avant au freinage, vers l'arrière à l'accélération, vers l'extérieur du virage ; bornée
        const vec3 frein = mouvementTete(-8, 0), acc = mouvementTete(4, 0), droite = mouvementTete(0, 6);
        VERIFIE(frein.z < 0 && acc.z > 0, "tête : freinage %.3f m (vers l'avant, −z), accélération %.3f m", frein.z, acc.z);
        VERIFIE(droite.x < 0, "virage à droite : tête vers la gauche (%.3f m)", droite.x);
        const vec3 fort = mouvementTete(-100, 100);
        VERIFIE(std::fabs(fort.z) <= 0.03f + 1e-6f && std::fabs(fort.x) <= 0.02f + 1e-6f && fort.y == 0.f, "tête bornée (3 cm, 2 cm)");
        const vec3 cou = decalageCou(12 * DEG);
        VERIFIE(std::fabs(cou.x - 0.0187f) < 0.001f && cou.z > 0 && cou.z < 0.003f && decalageCou(0).x == 0.f, "cou : œil décalé de %.1f cm à 12°", cou.x * 100);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("regard cockpit : OK\n");
    return 0;
}
