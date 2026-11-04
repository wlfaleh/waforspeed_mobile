// Trafic vivant (WFS-67) : même graine → même trafic (et mêmes feux), rythme des clignotants du jeu web (0,38 s
// allumé sur 0,72 s, phase propre à chaque voiture), feux stop à la décélération, hauteurs des feux = catalogue web.
// usage : test_trafic_vivant
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/game/traffic.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // catalogue des feux dans le même ordre que WEB_CARS
    bool ordre = sizeof(WEB_FEUX) / sizeof(WEB_FEUX[0]) == (size_t)WEB_CAR_COUNT;
    for (int i = 0; i < WEB_CAR_COUNT && ordre; i++) ordre = std::strcmp(WEB_FEUX[i].id, WEB_CARS[i].id) == 0 && WEB_FEUX[i].tail > WEB_FEUX[i].nose;
    VERIFIE(ordre, "WEB_FEUX ≠ WEB_CARS");

    // clignotant : allumé 0,38 s sur 0,72 s ; phases différentes selon la voiture
    int allume = 0; const int N = 7200;
    for (int i = 0; i < N; i++) allume += clignotantAllume(i * 0.001, 12345);
    VERIFIE(std::abs(allume / (double)N - 0.38 / 0.72) < 0.01, "rapport cyclique %.3f", allume / (double)N);
    int diff = 0; for (int i = 0; i < 720; i++) diff += clignotantAllume(i * 0.001, 1) != clignotantAllume(i * 0.001, 500);
    VERIFIE(diff > 50, "phases identiques entre voitures");
    std::printf("clignotants : 0,38 s sur 0,72 s, phase propre à chaque voiture : OK\n");

    // feux stop : décélération > 2 m/s² → allumés, gardés 0,4 s
    FeuxStop f; const double dt = 1.0 / 60, v0 = 20 / M;
    f.maj(v0, dt); f.maj(v0 - 1.0 / M * dt, dt);   // −1 m/s² : éteints
    VERIFIE(!f.freine(), "feux stop à −1 m/s²");
    f.maj(v0 - 1.0 / M * dt - 4.0 / M * dt, dt);  // −4 m/s² : allumés
    VERIFIE(f.freine(), "feux stop éteints à −4 m/s²");
    const double v = f.prev; int tenus = 0; while (f.freine() && tenus < 100) { f.maj(v, dt); tenus++; }
    VERIFIE(tenus >= 23 && tenus <= 25, "tenue des feux stop %d pas", tenus);
    std::printf("feux stop : allumés au-delà de 2 m/s², gardés 0,4 s : OK\n");

    // même graine → même trafic, mêmes clignotants et mêmes feux stop, image par image (RG-04)
    static Track ta, tb; ta.build(mapById("ville")); tb.build(mapById("ville"));
    Traffic a, b; a.parallel = b.parallel = false;
    TrafficView pv; pv.speed = MAX_SPEED * 0.45; pv.myLen = webCarLength("golf", 4.29);
    a.reset(ta, 4242, mapById("ville").trafficN, 0); b.reset(tb, 4242, mapById("ville").trafficN, 0);
    std::vector<FeuxStop> fa(a.cars.size()), fb(b.cars.size());
    int ecarts = 0, signaux = 0, freinages = 0;
    for (int s = 0; s < 3600; s++) {   // 60 s
        a.advance(T_STEP, pv); b.advance(T_STEP, pv); pv.position = ta.wrap(pv.position + pv.speed * T_STEP);
        for (size_t i = 0; i < a.cars.size(); i++) {
            fa[i].maj(a.cars[i].speed, T_STEP); fb[i].maj(b.cars[i].speed, T_STEP);
            const auto &x = a.cars[i], &y = b.cars[i];
            if (x.z != y.z || x.offset != y.offset || x.signal != y.signal || x.crashed != y.crashed || fa[i].freine() != fb[i].freine()) ecarts++;
            signaux += x.signal != 0; freinages += fa[i].freine();
        }
    }
    VERIFIE(ecarts == 0, "%d écart(s) entre deux trafics de même graine", ecarts);
    VERIFIE(signaux > 0 && freinages > 0, "le trafic doit clignoter (%d) et freiner (%d)", signaux, freinages);
    std::printf("même graine : 0 écart(s) sur 60 s (%d voitures) ; %d images-voiture avec clignotant, %d avec feux stop : OK\n",
                (int)a.cars.size(), signaux, freinages);

    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("trafic vivant : tout est OK\n");
    return 0;
}
