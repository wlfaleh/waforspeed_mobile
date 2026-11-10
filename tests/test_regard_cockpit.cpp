// Regard du conducteur en vue Cockpit (WFS-235, WFS-282) : champ de configgame (72° en hauteur, RG-00), regard un peu plongeant
// selon le réglage « Regard cockpit », regard qui suit le virage avec douceur (40 %, ±7°, au-dessus de 20 km/h, RG-02),
// distance de visée selon la vitesse (20 à 40 m), mouvement de tête de 1 à 2 cm.
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
    {   // WFS-323 (RG-01) : champ de la vue Cockpit = celui de configgame.json (62° en hauteur : cadrage de la référence ; 72° avant)
        VERIFIE(std::fabs(fovVue(VueCamera::Cockpit, 0, false) - 62.f) < 0.5f, "cockpit : champ vertical %.1f° (62° attendus)", fovVue(VueCamera::Cockpit, 0, false));
        // RG-01 : horizon entre 40 et 45 % de la hauteur depuis le haut avec le regard Normal (voiture à plat)
        const float tv = std::tan(fovVue(VueCamera::Cockpit, 0, false) * DEG / 2), hz = 0.5f + std::tan(inclinaisonRegard(RegardCockpit::Normal)) / (2 * tv);
        VERIFIE(hz >= 0.40f && hz <= 0.45f, "horizon à %.1f %% depuis le haut (40 à 45 %% visés)", hz * 100);
    }
    {   // RG-02, RG-03 : regard sous l'horizon, Bas < Normal < Haut, jamais au-dessus de l'horizon
        const float b = inclinaisonRegard(RegardCockpit::Bas), n = inclinaisonRegard(RegardCockpit::Normal), h = inclinaisonRegard(RegardCockpit::Haut);
        VERIFIE(b < n && n < h && h <= 0.f, "inclinaisons : bas %.1f°, normal %.1f°, haut %.1f°", b / DEG, n / DEG, h / DEG);
        VERIFIE(n < -2 * DEG && n > -6 * DEG, "regard normal légèrement plongeant (%.1f°)", n / DEG);   // WFS-323 : −5°
        // l'horizon reste dans l'image (au-dessus du bas) pour les 3 réglages, sur un écran 2400 × 1080
        const float demi = fovVue(VueCamera::Cockpit, 0, false) * DEG / 2;
        for (float a : {b, n, h}) VERIFIE(-a < demi * 0.8f, "horizon visible : regard à %.1f°, demi-champ %.1f°", a / DEG, demi / DEG);
    }
    {   // WFS-282 (RG-02) : regard plus doux : 40 % de l'angle, ±7°, seulement au-dessus de 20 km/h ; visée de 20 à 40 m
        VERIFIE(std::fabs(lacetVirage(10 * DEG, 90) - 4 * DEG) < 1e-4f, "40 %% d'un virage à 10° : %.2f°", lacetVirage(10 * DEG, 90) / DEG);
        VERIFIE(std::fabs(lacetVirage(80 * DEG, 90) - 7 * DEG) < 1e-4f && std::fabs(lacetVirage(-80 * DEG, 90) + 7 * DEG) < 1e-4f, "lacet borné à ±7°");
        VERIFIE(lacetVirage(0, 90) == 0.f, "ligne droite : regard droit devant");
        VERIFIE(lacetVirage(30 * DEG, 0) == 0.f && lacetVirage(30 * DEG, 20) == 0.f, "à l'arrêt et à 20 km/h : regard droit devant");
        VERIFIE(std::fabs(lacetVirage(10 * DEG, 25) - 2 * DEG) < 1e-4f, "25 km/h : demi-suivi (%.2f°)", lacetVirage(10 * DEG, 25) / DEG);
        VERIFIE(distanceRegard(0) == 20.f && distanceRegard(500) == 40.f && std::fabs(distanceRegard(72) - 30.f) < 1e-3f, "distance de visée : %.1f / %.1f / %.1f m",
                distanceRegard(0), distanceRegard(72), distanceRegard(500));
        VERIFIE(COCKPIT_LISSAGE_REGARD == 1.5f, "lissage du regard 1,5 /s");
    }
    {   // RG-02 (WFS-282) : tête vers l'avant au freinage, vers l'arrière à l'accélération, vers l'extérieur du virage ; 1 à 2 cm
        const vec3 frein = mouvementTete(-8, 0), acc = mouvementTete(4, 0), droite = mouvementTete(0, 6);
        VERIFIE(frein.z < 0 && acc.z > 0, "tête : freinage %.3f m (vers l'avant, −z), accélération %.3f m", frein.z, acc.z);
        VERIFIE(droite.x < 0, "virage à droite : tête vers la gauche (%.3f m)", droite.x);
        const vec3 fort = mouvementTete(-100, 100);
        VERIFIE(std::fabs(fort.z) <= 0.02f + 1e-6f && std::fabs(fort.x) <= 0.015f + 1e-6f && fort.y == 0.f, "tête bornée (2 cm, 1,5 cm)");
        const vec3 cou = decalageCou(12 * DEG);
        VERIFIE(std::fabs(cou.x - 0.0187f) < 0.001f && cou.z > 0 && cou.z < 0.003f && decalageCou(0).x == 0.f, "cou : œil décalé de %.1f cm à 12°", cou.x * 100);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("regard cockpit : OK\n");
    return 0;
}
