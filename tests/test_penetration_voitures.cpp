// WFS-333 (RG-02, RG-03 ; ANO-180, 181, 183) : aucune voiture (trafic, adversaire ; police : WFS-338) n'est dessinée dans celle du joueur.
// Mesure sur les emprises DESSINÉES (rectangles orientés : cap + tête-à-queue du joueur, cap + rotation d'une voiture accidentée),
// à la fin de chaque pas de 1/120 s, comme le rendu les lit :
//  1. trafic : 3 cartes, aide Activée et Désactivée, 6 graines, 60 s de conduite agressive (le joueur vise la voiture la plus
//     proche devant lui, accidents compris) : recouvrement < 3 cm (avant WFS-333 : 2,19 m, voitures accidentées et tête-à-queue) ;
//     aucune voiture accidentée collée au joueur à sa vitesse plus d'1 s (RG-02, ANO-180) ;
//     (police en Poursuite : WFS-338, PR #297, tests/test_poursuite.cpp)
//  2. adversaire de la course à 5 qui pousse par l'arrière un joueur en tête-à-queue (duel::contacter + separerDuJoueur) : < 3 cm.
// usage : test_penetration_voitures
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/game/duel.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/traffic.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
constexpr double TOLERANCE_M = 0.03;

// recouvrement dessiné d'une voiture alignée sur la route (adversaire) avec le joueur
static double recouvrementAligne(const Player& p, double rd, double x, const CarDef& c) {
    return recouvrement(empriseJoueur(p), Emprise{x * RW, (rd - p.dist) * M, 0, c.d.L / 2, c.d.W / 2});
}

static void trafic(const char* carte) {
    static Track t; t.build(mapById(carte), trace::versionCarte(carte));
    int accidentsCarte = 0;
    for (Aide aide : {Aide::Activee, Aide::Desactivee}) {
        double pire = 0, colleMax = 0; int chocs = 0, accidents = 0, toupies = 0;
        for (uint32_t graine = 1; graine <= 6; graine++) {
            Traffic tr; tr.parallel = false; tr.reset(t, graine, 60, 0);
            Player p; p.aide = aide; p.reset(carById("golf")); p.speed = 100 / 3.6 / M;
            Mulberry32 R(graine * 77);
            std::vector<double> colle(tr.cars.size(), 0);
            double cible = 0, tc = 0;
            for (int n = 0; n < 120 * 60; n++) {
                if ((tc -= 1.0 / 120) <= 0) {   // vise la voiture la plus proche (6 m derrière à 40 m devant), sinon une file au hasard
                    tc = 0.5 + R.next() * 1.5; cible = (R.next() - 0.5) * 1.4;
                    double mieux = 1e9;
                    for (const TrafficCar& c : tr.cars) { const double dz = t.relZ(c.z - p.position) * M; if (dz > -6 && dz < 40 && dz < mieux) { mieux = dz; cible = c.offset; } }
                }
                DriveInput in; in.gas = true; in.steer = (float)std::clamp((cible - p.playerX) * 4 - p.heading * 3, -1.0, 1.0);
                p.update(1.0 / 120, in, t, true);
                TrafficView pv; pv.position = p.position; pv.playerX = p.playerX; pv.speed = p.speed; pv.myLen = p.car->d.L / M;
                tr.advance(1.0 / 120, pv);
                const HitEvents e = tr.collide(p, 1.0 / 120);
                chocs += e.bumps + e.scrapes + e.accidents; accidents += e.accidents; toupies += p.spinYaw != 0;
                for (size_t i = 0; i < tr.cars.size(); i++) {
                    const TrafficCar& c = tr.cars[i];
                    if (c.cache) continue;
                    const double dz = t.relZ(c.z - p.position);
                    if (std::abs(dz) * M > 15) { colle[i] = 0; continue; }
                    pire = std::max(pire, recouvrement(empriseJoueur(p), empriseTrafic(c, dz)));
                    const bool g = c.crashed && std::abs(dz) * M < 8 && std::abs(c.offset - p.playerX) * RW < 4 && std::abs(c.speed - p.speed) * M * 3.6 < 5 && p.kmh() > 60;
                    colle[i] = g ? colle[i] + 1.0 / 120 : 0; colleMax = std::max(colleMax, colle[i]);
                }
            }
        }
        const char* nom = aide == Aide::Activee ? "Activée" : "Désactivée";
        std::printf("trafic %s, aide %s : %d contact(s), %d accident(s), %.1f s en tête-à-queue, recouvrement max %.3f m, accidentée collée %.2f s\n",
                    carte, nom, chocs, accidents, toupies / 120.0, std::max(0.0, pire), colleMax);
        VERIFIE(chocs > 50, "trafic %s, aide %s : trop peu de contacts pour conclure (%d)", carte, nom, chocs);
        accidentsCarte += accidents;
        VERIFIE(pire < TOLERANCE_M, "trafic %s, aide %s : voiture dessinée dans celle du joueur (%.3f m)", carte, nom, pire);
        VERIFIE(colleMax < 1.0, "trafic %s, aide %s : voiture accidentée collée au joueur à sa vitesse %.2f s (RG-02)", carte, nom, colleMax);
    }
    VERIFIE(accidentsCarte > 0, "trafic %s : aucun accident (voitures accidentées et tête-à-queue non essayés)", carte);
}

static void adversaire() {
    static Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    const CarDef& oc = carById("rrsport");
    Player p; p.aide = Aide::Activee; p.reset(carById("golf")); p.speed = 90 / 3.6 / M; p.dist = 300 / M; p.position = t.wrap(p.dist);
    p.accident(0.6, 1);   // tête-à-queue : emprise tournée
    duel::Ghost g; g.car = "rrsport"; g.x = p.playerX + 0.05; g.rd = p.dist - 3 / M; g.v = p.speed + 40 / 3.6 / M;
    double pire = 0; int chocs = 0;
    for (int n = 0; n < 120 * 3; n++) {
        DriveInput in; p.update(1.0 / 120, in, t, true); g.rd += g.v / 120;
        chocs += duel::contacter(1.0 / 120, p, t, carById("golf"), g).chocs;
        if (separerDuJoueur(p, g.rd, g.x, oc.d.L, oc.d.W) && g.rd < p.dist) g.v = std::min(g.v, p.speed);
        pire = std::max(pire, recouvrementAligne(p, g.rd, g.x, oc));
    }
    std::printf("adversaire qui pousse un joueur en tête-à-queue, 3 s : %d choc(s), recouvrement max %.3f m\n", chocs, std::max(0.0, pire));
    VERIFIE(chocs > 0, "adversaire : aucun choc");
    VERIFIE(pire < TOLERANCE_M, "adversaire dessiné dans la voiture du joueur (%.3f m)", pire);
}

int main() {
    // emprises : deux voitures côte à côte qui se touchent (0), en travers l'une sur l'autre (recouvrement d'une demi-largeur)
    VERIFIE(std::abs(recouvrement({0, 0, 0, 2, 1}, {2, 0, 0, 2, 1})) < 1e-9, "emprises côte à côte");
    VERIFIE(std::abs(recouvrement({0, 0, 0, 2, 1}, {0, 2.5, 1.5707963267948966, 2, 1}) - 0.5) < 1e-6, "emprise en travers");
    for (const char* carte : {"ville", "campagne", "plage"}) trafic(carte);
    adversaire();
    std::printf("aucune voiture dans celle du joueur (WFS-333) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
