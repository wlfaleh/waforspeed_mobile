// WaForSpeed — tests de WFS-334 (ANO-182, ANO-186, ANO-194) : pluie sous un toit, vue Cockpit, gouttes inclinées par la
// vitesse (règles pures de src/world/meteo.h, sans GPU).
#include <cmath>
#include <cstdio>
#include "../src/world/meteo.h"

using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-03 : tunnel de 100 à 300 m : pluie entière dehors, nulle sous la voûte, fondu court aux deux bouches
    VERIFIE(meteo::pluieSousToit(50, 100, 300) == 1.f, "pluie dehors, avant le tunnel");
    VERIFIE(meteo::pluieSousToit(98, 100, 300) == 1.f, "pluie jusqu'à %.0f m avant la bouche", meteo::TOIT_MARGE);
    VERIFIE(meteo::pluieSousToit(200, 100, 300) == 0.f, "aucune pluie au milieu du tunnel");
    VERIFIE(meteo::pluieSousToit(100 + meteo::TOIT_FONDU, 100, 300) == 0.f, "aucune pluie %.0f m après l'entrée", meteo::TOIT_FONDU);
    VERIFIE(meteo::pluieSousToit(300 - meteo::TOIT_FONDU, 100, 300) == 0.f, "aucune pluie %.0f m avant la sortie", meteo::TOIT_FONDU);
    {
        const float a = meteo::pluieSousToit(101, 100, 300), b = meteo::pluieSousToit(103, 100, 300);
        VERIFIE(a > 0 && a < 1 && b < a, "fondu à l'entrée (%.2f puis %.2f)", a, b);
        VERIFIE(std::fabs(meteo::pluieSousToit(299, 100, 300) - a) < 1e-5f, "même fondu à la sortie");
    }
    VERIFIE(meteo::pluieSousToit(320, 100, 300) == 1.f, "pluie après le tunnel");

    // ANO-194 : à l'arrêt, gouttes verticales ; à vitesse, elles s'écartent du point de fuite (vers la caméra)
    float dx, dy;
    meteo::sensGoutte(100, 100, 640, 300, 0.f, 0.f, dx, dy);
    VERIFIE(std::fabs(dx) < 1e-4f && std::fabs(dy - 1) < 1e-4f, "à l'arrêt : verticale (%.3f, %.3f)", dx, dy);
    meteo::sensGoutte(100, 100, 640, 300, 1.f, 0.f, dx, dy);   // en haut à gauche du point de fuite
    VERIFIE(dx < -0.5f && std::fabs(dx * dx + dy * dy - 1) < 1e-4f, "à pleine vitesse, en haut à gauche : vers la gauche (%.3f, %.3f)", dx, dy);
    meteo::sensGoutte(1100, 600, 640, 300, 1.f, 0.f, dx, dy);  // en bas à droite
    VERIFIE(dx > 0.5f && dy > 0.3f, "à pleine vitesse, en bas à droite : vers le bas à droite (%.3f, %.3f)", dx, dy);
    {   // l'inclinaison croît avec la vitesse
        float d1x, d1y, d2x, d2y;
        meteo::sensGoutte(200, 300, 640, 300, 0.3f, 0.f, d1x, d1y); meteo::sensGoutte(200, 300, 640, 300, 0.7f, 0.f, d2x, d2y);
        VERIFIE(d2x < d1x && d1x < 0, "plus vite, plus penchée (%.3f puis %.3f)", d1x, d2x);
    }
    {   // les gouttes avancent dans leur sens et restent sur l'écran (ou repartent)
        std::vector<meteo::Goutte> g; meteo::Hasard rnd; meteo::initRain(g, 100, 1280, 720, 1, rnd);
        for (int k = 0; k < 600; k++) meteo::avancerVers(g, 1 / 60.f, 1280, 720, 0.8f, 0.f, 640, 300, rnd);
        int dehors = 0; for (const auto& d : g) dehors += d.x < -30 || d.x > 1310 || d.y > 720 || d.y < -80;
        VERIFIE(dehors == 0, "%d gouttes perdues hors de l'écran", dehors);
    }

    // RG-02 : pare-brise (quadrilatère de l'écran) : dedans / dehors, dans les deux sens de parcours
    const float q[8] = {400, 450, 1300, 450, 1250, -50, 450, -50};
    VERIFIE(meteo::dansQuad(q, 800, 200), "milieu du pare-brise");
    VERIFIE(!meteo::dansQuad(q, 800, 600), "planche (sous le pare-brise)");
    VERIFIE(!meteo::dansQuad(q, 200, 200), "à gauche du montant");
    const float qr[8] = {450, -50, 1250, -50, 1300, 450, 400, 450};
    VERIFIE(meteo::dansQuad(qr, 800, 200) && !meteo::dansQuad(qr, 800, 600), "même résultat dans l'autre sens");

    // RG-02 : le balai efface la goutte qu'il croise pendant l'image, dans les deux sens de balayage
    VERIFIE(meteo::balaye(0.5f, 0.4f, 0.6f) && meteo::balaye(0.5f, 0.6f, 0.4f), "balai qui passe sur la goutte (aller et retour)");
    VERIFIE(!meteo::balaye(0.8f, 0.4f, 0.6f) && !meteo::balaye(0.5f, 0.5f, 0.5f), "balai qui ne passe pas (ou immobile)");

    if (echecs) { std::printf("test_pluie : %d échec(s)\n", echecs); return 1; }
    std::printf("test_pluie : OK\n");
    return 0;
}
