// Regard du conducteur en vue Cockpit (WFS-235) : champ de vision horizontal de 75 à 90° quelle que soit la forme de l'écran
// (RG-02), regard un peu plongeant selon le réglage « Regard cockpit » (RG-03), regard qui suit le virage sans dépasser ±12°,
// distance de visée selon la vitesse, mouvement de tête borné (RG-04) ; géométrie des 20 intérieurs (arbitrage du PO du
// 2026-11-07) : haut du pare-brise vu à 12° au moins, rien dans la fenêtre de la rue (de l'horizon à +8°, ±15° autour de
// l'axe de la voiture) : ni montant, ni rétroviseur.
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
    {   // RG-02 : champ horizontal de 85° à l'arrêt, 90° au plus ; vertical selon l'écran
        VERIFIE(std::fabs(fovHorizontalCockpit(0, false) - 85.f) < 1e-4f, "champ horizontal à l'arrêt : %.1f°", fovHorizontalCockpit(0, false));
        VERIFIE(fovHorizontalCockpit(1.3, true) == fovHorizontalCockpit(0, false), "champ fixe (ni vitesse ni nitro)");
        for (float aspect : {2400.f / 1080.f, 16.f / 9.f, 4.f / 3.f}) {
            const float v = fovVerticalDepuisHorizontal(85.f, aspect);
            // retour au champ horizontal : 2 atan(tan(v / 2) × aspect)
            const float h = 2.f * std::atan(std::tan(v * DEG / 2) * aspect) / DEG;
            VERIFIE(std::fabs(h - 85.f) < 0.1f, "écran %.2f : vertical %.1f° → horizontal %.1f° (85° attendus)", aspect, v, h);
        }
        const float v2400 = fovVerticalDepuisHorizontal(85.f, 2400.f / 1080.f);
        VERIFIE(v2400 > 43.f && v2400 < 47.f, "2400 × 1080 : champ vertical %.1f° (≈ 45° attendus)", v2400);
        VERIFIE(fovVerticalDepuisHorizontal(85.f, 0.1f) <= 75.f && fovVerticalDepuisHorizontal(85.f, 10.f) >= 30.f, "champ vertical borné (30 à 75°)");
    }
    {   // RG-02, RG-03 : regard sous l'horizon, Bas < Normal < Haut, jamais au-dessus de l'horizon
        const float b = inclinaisonRegard(RegardCockpit::Bas), n = inclinaisonRegard(RegardCockpit::Normal), h = inclinaisonRegard(RegardCockpit::Haut);
        VERIFIE(b < n && n < h && h <= 0.f, "inclinaisons : bas %.1f°, normal %.1f°, haut %.1f°", b / DEG, n / DEG, h / DEG);
        VERIFIE(n < -2 * DEG && n > -6 * DEG, "regard normal légèrement plongeant (%.1f°)", n / DEG);
        // l'horizon reste dans l'image (au-dessus du bas) pour les 3 réglages, sur un écran 2400 × 1080
        const float demi = fovVerticalDepuisHorizontal(85.f, 2400.f / 1080.f) * DEG / 2;
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
    {   // géométrie des 20 intérieurs, repère de l'œil (x à droite, y en haut, −z devant ; milieu de la voiture à x = +0,36)
        const float CX = 0.36f, H8 = 8 * DEG, A15 = 15 * DEG;
        int n = 0;
        for (int i = 0; i < cockpit::nombreInterieurs(); i++) {
            const cockpit::InterieurDef& d = cockpit::interieur(i);
            float H = 0; for (const CarDef& c : CARS) if (std::string(c.id) == d.car) H = c.d.H;
            VERIFIE(H > 0, "%s : hauteur de la voiture inconnue", d.car);
            const float y = cockpit::toitAuDessusOeil(d, H), z = cockpit::toitDevantOeil(d, y), el = std::atan2(y, -z);
            VERIFIE(y >= 0.10f && y <= 0.30f && z < -0.3f && z > -0.9f, "%s : haut du pare-brise (%+.2f ; %+.2f)", d.car, y, z);
            VERIFIE(el >= 12 * DEG - 1e-4f, "%s : haut du pare-brise vu à %.1f° (≥ 12°)", d.car, el / DEG);
            // rétroviseur : glace de 250 mm au milieu de la voiture, 7,5 cm sous le ciel de toit : son bord gauche hors de ±15°
            const float bordG = std::atan2(CX - 0.125f * std::cos(0.30f), -z);
            VERIFIE(bordG > A15, "%s : bord du rétroviseur à %.1f° de l'axe (> 15°)", d.car, bordG / DEG);
            // montant gauche (garniture de 65 mm) : entre l'horizon et +8°, son bord intérieur hors de ±15° autour de l'axe
            const vec3 a{CX - 0.74f, d.pareBriseY, d.pareBriseZ}, b{CX - 0.62f, y - 0.01f, z + 0.02f};
            for (int k = 0; k <= 50; k++) {
                const float t = k / 50.f; const vec3 p = a + (b - a) * t;
                const float e = std::atan2(p.y, std::sqrt(p.x * p.x + p.z * p.z));
                if (e < 0 || e > H8) continue;
                const float az = std::atan2(-(p.x + 0.0325f), -p.z);   // bord intérieur (vers l'axe), angle vers la gauche
                VERIFIE(az > A15, "%s : montant gauche à %.1f° (> 15°) à %.1f° au-dessus de l'horizon", d.car, az / DEG, e / DEG);
            }
            n++;
        }
        VERIFIE(n == 20, "%d intérieurs (20 attendus)", n);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("regard cockpit : OK\n");
    return 0;
}
