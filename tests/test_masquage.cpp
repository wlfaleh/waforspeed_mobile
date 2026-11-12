// Masquage des objets (WFS-82) : sphère dedans, dehors, à cheval (caméra, miroir, boîte d'ombre orthographique),
// sphère après une matrice monde mise à l'échelle, coupure au loin selon le rayon et le niveau de qualité.
// usage : test_masquage
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include "../src/render/masquage.h"
using namespace wfs;
using namespace wfs::masquage;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // caméra de course : en (0, 2, 0), regarde vers −Z, FOV 60°, 16/9, profondeur inversée, plan lointain à l'infini
    const mat4 vp = perspectiveVk(60.f * PI / 180.f, 16.f / 9.f, 0.1f) * lookAt({0, 2, 0}, {0, 2, -10}, {0, 1, 0});
    const Frustum f = depuis(vp);
    VERIFIE(visible(f, {{0, 2, -20}, 1}), "devant : visible");
    VERIFIE(visible(f, {{0, 2, -5000}, 1}), "très loin devant : visible (plan lointain infini)");
    VERIFIE(!visible(f, {{0, 2, 20}, 1}), "derrière : masqué");
    VERIFIE(!visible(f, {{60, 2, -20}, 1}), "loin à droite : masqué");
    VERIFIE(!visible(f, {{0, 60, -20}, 1}), "loin au-dessus : masqué");
    // à cheval sur le bord droit (tan 30° × 16/9 × 20 m ≈ 20,5 m) : visible
    VERIFIE(visible(f, {{21.5f, 2, -20}, 2}), "à cheval sur le bord : visible");
    VERIFIE(!visible(f, {{25, 2, -20}, 2}), "juste dehors : masqué");
    VERIFIE(visible(f, {{0, 2, 5}, -1}), "rayon inconnu : jamais masqué");
    // le point projeté au centre de l'écran est bien dedans (cohérence avec la matrice)
    const vec3 p{3, 1, -40};
    const float cx = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3), cw = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
    VERIFIE(std::abs(cx / cw) < 1 && visible(f, {p, 0.01f}), "point projeté dans l'écran");
    std::printf("caméra : devant, très loin, derrière, côtés, à cheval, rayon inconnu : OK\n");

    // miroir de la chaussée (reflets WFS-16) : y → −y ; un objet sous la caméra (haut de bâtiment reflété) suit le miroir
    mat4 R = mat4::identity(); R.at(1, 1) = -1;
    const Frustum fm = depuis(vp * R);
    VERIFIE(visible(fm, {{0, -2, -20}, 1}) && !visible(fm, {{0, 2, 20}, 1}), "miroir");
    // boîte d'ombre orthographique : x, y ∈ [−1, 1], z ∈ [0, 1]
    mat4 O = mat4::identity(); O.at(0, 0) = 1.f / 40; O.at(1, 1) = 1.f / 40; O.at(2, 2) = -1.f / 260; O.at(2, 3) = 0.5f;
    const Frustum fo = depuis(O);
    VERIFIE(visible(fo, {{10, 10, 0}, 1}) && !visible(fo, {{80, 0, 0}, 1}) && visible(fo, {{40.5f, 0, 0}, 1}), "boîte d'ombre");
    std::printf("miroir des reflets et boîte d'ombre orthographique : OK\n");

    // sphère monde : translation + échelle 3 → rayon × 3
    const Sphere s = monde(translate({5, 0, 0}) * scale({1, 3, 1}), {{0, 1, 0}, 2});
    VERIFIE(std::abs(s.c.x - 5) < 1e-5f && std::abs(s.c.y - 3) < 1e-5f && std::abs(s.r - 6) < 1e-5f, "sphère monde (%.2f %.2f r %.2f)", s.c.x, s.c.y, s.r);
    const Sphere b = deBoite({-1, 0, -2}, {1, 2, 2});
    VERIFIE(std::abs(b.r - std::sqrt(24.f) / 2) < 1e-5f && std::abs(b.c.y - 1) < 1e-6f, "sphère d'une boîte");

    // RG-02 : coupure au loin selon le rayon et le niveau
    VERIFIE(distanceMax(1, false, 500) == 200 && distanceMax(1, true, 500) == 110, "petits objets : 200 m / 110 m en Basse (WFS-265)");
    VERIFIE(distanceMax(5, false, 500) == 500 && distanceMax(5, true, 500) == 420 && distanceMax(5, true, 300) == 300, "objets moyens : fin du brouillard");
    VERIFIE(distanceMax(30, true, 100) > 1e8f && distanceMax(-1, true, 100) > 1e8f, "grands objets et rayon inconnu : jamais coupés");
    VERIFIE(std::abs(finBrouillard(0, 0.0022f) - 3.912f / 0.0022f) < 1e-2f && finBrouillard(40, 0.01f) == 40, "fin du brouillard");
    std::printf("sphère monde, coupure selon le rayon (200 / 110 m, brouillard, jamais) : OK\n");

    // WFS-453 (ANO-247) : banc qualité, Basse, vue Poursuite (caméra 8 m derrière le joueur, 2,6 m au-dessus, game.cpp) : une
    // citadine du trafic (boîte 1,8 × 1,5 × 4,3 m, rayon ≈ 2,4 m) posée à 107 m, 118 m ou 120 m DEVANT LE JOUEUR (limite de dessin
    // du trafic en Basse, Traffic::dessines) est gardée par la coupe à distance ; avant : 110 m de la caméra, la voiture à 107 m
    // (≈ 110,6 m de la caméra, rayon déduit) était coupée, celle à 118 m dessinée seulement si elle était grande (bus, camion)
    {
        const vec3 cam{0, 2.6f, 8.f};
        const Sphere boite = deBoite({-0.9f, 0, -2.15f}, {0.9f, 1.5f, 2.15f});
        auto distance = [&](float d) { const Sphere s = monde(translate({0, 0, -d}), boite); return length(s.c - cam) - s.r; };
        for (float d : {8.f, 96.f, 107.f, 118.f, 120.f})
            VERIFIE(distance(d) <= distanceMaxTrafic(boite.r, true, 500), "trafic Basse : voiture à %.0f m du joueur gardée (%.1f m de la caméra)", d, distance(d));
        VERIFIE(distance(107) > distanceMax(boite.r, true, 500), "le cas d'ANO-247 : 107 m coupée par la limite des petits objets (110 m)");
        VERIFIE(distanceMaxTrafic(boite.r, true, 500) == TRAFIC_BASSE_M && distanceMaxTrafic(boite.r, true, 100) == TRAFIC_BASSE_M,
                "trafic Basse : 130 m, brouillard compris");
        VERIFIE(distanceMaxTrafic(boite.r, false, 500) == distanceMax(boite.r, false, 500) && distanceMaxTrafic(5, false, 500) == distanceMax(5, false, 500) &&
                distanceMaxTrafic(5, true, 500) == distanceMax(5, true, 500), "Haute et grands véhicules : inchangés");
        VERIFIE(distanceMax(boite.r, true, 500) == 110, "autres petits objets en Basse : 110 m, inchangés (WFS-265)");
        std::printf("trafic en Basse, vue Poursuite : 107 / 118 / 120 m gardées (WFS-453) : OK\n");
    }

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("masquage : tout est OK\n");
    return 0;
}
