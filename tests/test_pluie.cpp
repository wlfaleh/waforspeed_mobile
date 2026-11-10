// WaForSpeed — tests de WFS-334 (ANO-182, ANO-186, ANO-194) : pluie sous un toit, vue Cockpit, gouttes inclinées par la
// vitesse (règles pures de src/world/meteo.h, sans GPU).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/world/meteo.h"
#include "../src/render/vehicule_cockpit.h"   // WFS-354 : vitre seule (coinsVitre) et définitions des intérieurs

using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-03 : tunnel de 100 à 300 m : pluie entière dehors, nulle sous la voûte, fondu court aux deux bouches
    // WFS-354 (2e retour du lead dev) : vue Arrière = calcul de main (voiture, marge 2 m), figé ici valeur par valeur
    VERIFIE(meteo::TOIT_FONDU == 8.f && meteo::TOIT_MARGE == 2.f, "vue Arrière : fondu de 8 m et marge de 2 m, comme main");
    for (double d = 80; d <= 320; d += 0.5) {   // formule de main, recopiée telle quelle
        const double dedans = std::min(d - 100, 300 - d);
        const float main_ = std::clamp((float)(1.0 - (dedans + 2.0) / 8.0), 0.f, 1.f);
        VERIFIE(meteo::pluieTunnel(false, d, 100, 300) == main_, "vue Arrière identique à main à %.1f m", d);
    }
    VERIFIE(meteo::pluieTunnel(false, 106, 100, 300) == 0.f, "vue Arrière : voiture 6 m dans la bouche, plus de pluie (comme main)");
    {
        const float a = meteo::pluieTunnel(false, 100, 100, 300);   // voiture à la bouche : la caméra de poursuite est encore dehors
        VERIFIE(a > 0 && a < 1, "vue Arrière : voiture à la bouche, fondu en cours (%.2f)", a);
    }
    VERIFIE(meteo::pluieTunnel(false, 98, 100, 300) == 1.f, "vue Arrière : pluie entière 2 m avant la bouche");
    // vue Cockpit : la caméra (l'œil) à la bouche : plus aucune pluie ; fondu de 8 m avant la bouche
    VERIFIE(meteo::pluieTunnel(true, 100, 100, 300) == 0.f && meteo::pluieTunnel(true, 300, 100, 300) == 0.f, "Cockpit : plus de pluie dès la bouche");
    VERIFIE(meteo::pluieTunnel(true, 92, 100, 300) == 1.f, "Cockpit : pluie entière jusqu'à 8 m avant la bouche");
    VERIFIE(meteo::pluieTunnel(true, 200, 100, 300) == 0.f && meteo::pluieTunnel(false, 200, 100, 300) == 0.f, "aucune pluie au milieu du tunnel");
    {
        const float a = meteo::pluieTunnel(true, 96, 100, 300), b = meteo::pluieTunnel(true, 99, 100, 300);
        VERIFIE(a > 0 && a < 1 && b < a, "Cockpit : fondu à l'approche de la bouche (%.2f puis %.2f)", a, b);
        VERIFIE(std::fabs(meteo::pluieTunnel(true, 304, 100, 300) - a) < 1e-5f, "Cockpit : même fondu à la sortie");
    }
    VERIFIE(meteo::pluieTunnel(true, 320, 100, 300) == 1.f && meteo::pluieTunnel(false, 320, 100, 300) == 1.f, "pluie après le tunnel");

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

    // WFS-354 (RG-01) : vue de l'œil (origine), aucun point du bord de la vitre visible (ni ce point décalé vers le montant du
    // rayon de la plus grosse goutte, 6 mm) n'est caché par une barre de montant (mêmes axes et cotes que barre() de l'intérieur) :
    // lancer de rayon œil → point contre les deux barres (boîtes orientées). 20 intérieurs, dont Golf, X5 et 500 Abarth
    auto cache = [&](const vec3 axe[4], vec3 P) {   // vrai : le segment œil → P traverse une barre avant d'atteindre P
        for (int s = 0; s < 2; s++) {
            const vec3 a = axe[s ? 1 : 0], b = axe[s ? 2 : 3];
            const vec3 y = normalize(b - a), ref = std::fabs(y.x) < 0.9f ? vec3{1, 0, 0} : vec3{0, 0, 1};
            const vec3 x = normalize(cross(y, ref)), z = cross(x, y), c = (a + b) * 0.5f;
            const vec3 ax[3] = {x, y, z};
            const float he[3] = {cockpit::LARGEUR_MONTANT * 0.5f, length(b - a) * 0.5f, cockpit::EPAISSEUR_MONTANT * 0.5f};
            float t0 = 0, t1 = 1;
            bool dedans = true;
            for (int k = 0; k < 3 && dedans; k++) {   // plaques de la boîte (repère de la barre)
                const float o = dot(vec3{0, 0, 0} - c, ax[k]), d = dot(P, ax[k]);
                if (std::fabs(d) < 1e-9f) { if (std::fabs(o) > he[k]) dedans = false; continue; }
                float ta = (-he[k] - o) / d, tb = (he[k] - o) / d;
                if (ta > tb) std::swap(ta, tb);
                t0 = std::max(t0, ta); t1 = std::min(t1, tb);
                if (t0 > t1) dedans = false;
            }
            if (dedans && t0 < 0.999f) return true;
        }
        return false;
    };
    for (int i = 0; i < cockpit::nombreInterieurs(); i++) {
        const cockpit::InterieurDef& d = cockpit::interieur(i);
        const cockpit::Largeur l = cockpit::largeurPour(d);
        vec3 axe[4], v[4];
        cockpit::coinsVitre(l.oeil, l.montants * 0.5f, d.pareBriseY, d.pareBriseZ, cockpit::HAUT_PARE_BRISE_Y, cockpit::HAUT_PARE_BRISE_Z, axe);   // WFS-323 : haut du pare-brise du dessin
        meteo::vitreVue(axe, cockpit::LARGEUR_MONTANT, cockpit::EPAISSEUR_MONTANT, 0.016f, v);
        int caches = 0;
        for (int a = 0; a <= 40; a++) {
            const float t = a / 40.f;
            const vec3 g = v[0] + (v[3] - v[0]) * t, dr = v[1] + (v[2] - v[1]) * t;   // bords gauche et droit de la vitre
            for (const vec3& P : {g, dr, g - vec3{0.006f, 0, 0}, dr + vec3{0.006f, 0, 0}}) caches += cache(axe, P);
        }
        // contrôle du test : l'ancien bord (axe des montants) est bien caché
        int anciens = 0; for (int a = 0; a <= 40; a++) anciens += cache(axe, axe[1] + (axe[2] - axe[1]) * (a / 40.f));
        VERIFIE(caches == 0, "%s : %d points du bord de la vitre cachés par un montant (vus de l'œil)", d.car, caches);
        VERIFIE(anciens > 0, "%s : le test ne voit pas le montant (contrôle)", d.car);
        if (!std::strcmp(d.car, "golf") || !std::strcmp(d.car, "x5") || !std::strcmp(d.car, "abarth"))
            std::printf("%s : bord droit de la vitre en bas à %.0f mm de l'axe du montant (avant WFS-354 : 0)\n", d.car, (axe[1].x - v[1].x) * 1000);
    }

    if (echecs) { std::printf("test_pluie : %d échec(s)\n", echecs); return 1; }
    std::printf("test_pluie : OK\n");
    return 0;
}
