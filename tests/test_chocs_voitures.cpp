// WFS-256 (RG-04) : jamais deux voitures l'une dans l'autre, voiture du joueur en conduite réelle :
//  - trafic : le joueur fonce dans 60 voitures (Ville) à 60, 150 et 250 km/h dans les 3 files : à la fin de chaque pas, aucun
//    recouvrement des emprises (Traffic::collide) ;
//  - adversaire (règle du duel, aussi course à 5 et police : duel::contacter) : contact côte à côte, le joueur braque vers lui
//    pendant 3 s : aucun recouvrement (avant WFS-256 : 0,59 m, la vitesse latérale ramenait la voiture dans l'autre).
// usage : test_chocs_voitures
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../src/game/duel.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/traffic.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    static Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    for (double kmh : {60.0, 150.0, 250.0})
        for (double x : {0.0, 2.0 / 3, -2.0 / 3}) {
            Traffic tr; tr.reset(t, 1234, 60, 0);
            Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee; p.speed = kmh / 3.6 / M; p.playerX = x;
            double pire = 0; int chocs = 0;
            for (int n = 0; n < 120 * 20; n++) {
                DriveInput in; in.gas = true; in.steer = (float)std::clamp((x - p.playerX) * 3 - p.heading * 4, -1.0, 1.0);
                p.update(1.0 / 120, in, t, true);
                TrafficView pv; pv.position = p.position; pv.playerX = p.playerX; pv.speed = p.speed; pv.myLen = p.car->d.L / M;
                tr.advance(1.0 / 120, pv);
                const HitEvents e = tr.collide(p, 1.0 / 120); chocs += e.bumps + e.scrapes + e.accidents;
                for (const TrafficCar& c : tr.cars) {
                    if (c.cache) continue;
                    const double dz = std::abs(t.relZ(c.z - p.position)) * M, dx = std::abs(c.offset - p.playerX) * RW;
                    const double oz = (p.car->d.L + c.len * M) / 2 - dz, ox = p.car->d.W / 2 + c.w * RW - dx;
                    if (oz > 0 && ox > 0) pire = std::max(pire, std::min(oz, ox));
                }
            }
            std::printf("trafic à %3.0f km/h, file %+.2f : %d choc(s), recouvrement max %.3f m\n", kmh, x, chocs, pire);
            VERIFIE(pire < 0.01, "trafic à %.0f km/h, file %+.2f : voitures l'une dans l'autre (%.3f m)", kmh, x, pire);
        }
    {
        Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee; p.speed = 100 / 3.6 / M; p.dist = 100 / M; p.position = t.wrap(p.dist);
        duel::Ghost g; g.car = "rrsport"; g.x = 0.25; g.rd = p.dist; g.v = p.speed;
        double pire = 0; int fr = 0;
        for (int n = 0; n < 120 * 3; n++) {
            DriveInput in; in.gas = true; in.steer = 0.6f;
            p.update(1.0 / 120, in, t, true); g.rd += g.v / 120;
            fr += duel::contacter(1.0 / 120, p, t, carById("golf"), g).frottements;
            const double touch = (carById("golf").d.W + carById("rrsport").d.W) / 2;
            pire = std::max(pire, touch - std::abs(p.playerX - g.x) * RW);
        }
        std::printf("adversaire côte à côte, volant vers lui 3 s : %d frottement(s), recouvrement max %.3f m\n", fr, pire);
        VERIFIE(fr > 0, "adversaire : aucun contact");
        VERIFIE(pire < 0.05, "adversaire : voitures l'une dans l'autre (%.3f m)", pire);
    }
    std::printf("chocs entre voitures (WFS-256) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
