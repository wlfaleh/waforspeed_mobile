// WFS-281 (ANO-151) : trafic stable dans la durée. 10 min de course simulées sur les 4 cartes, le joueur roulant comme
// le pilote automatique (il suit la voiture de devant, change de file toutes les 0,4 s vers la plus dégagée) ;
// à chaque checkpoint (CHECK_DIST parcourus), la course demande +6 voitures (Course::step : addCars(6) sous 110) ;
// mesure toutes les 30 s : voitures au total, à moins de 150 m, dans la fenêtre de dessin (30 m derrière à 300 m devant),
// voitures arrêtées et plus longue immobilité.
// RG-02 : total constant ; voitures dans la fenêtre de dessin (30 m derrière à 300 m devant, celle de TrafficSystem::render) :
// moyenne sur 10 min ≤ 1,3 × la densité du départ, et chaque minute ≤ 2 × + 2 (le joueur qui rattrape un petit groupe
// de voitures sur 3 voies : tolérance de quelques voitures sur des nombres de 1 à 3).
// RG-03 : aucune voiture arrêtée plus de 20 s (hors accident, hors voitures qui traversent).
// usage : test_trafic_duree [carte] [--journal] [--avant]   (--avant : total non fixé, comportement d'avant WFS-281)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "../src/game/traffic.h"
#include "../src/game/trace_cartes.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

namespace {
struct Mesure { int total = 0, proches = 0, fenetre = 0, arretees = 0; double immobileMax = 0; };

// joueur simulé : même logique que TrafficSystem::autopilot (file la plus dégagée, freine derrière plus lent)
struct JoueurAuto {
    double pos = 0, x = 0, v = 0, cible = 0, minuterie = 0;
    void pas(const Traffic& tr, Track& t, double h) {
        minuterie -= h;
        if (minuterie <= 0) {
            minuterie = 0.4;
            double best = cible, bestGap = -1;
            for (double lx : LANE_X) {
                double gap = 9000;
                for (const auto& c : tr.cars) {
                    if (c.cache || std::abs(c.offset - lx) > 0.3) continue;
                    const double dz = t.relZ(c.z - (pos + PLAYER_Z));
                    if (dz > -800 && dz < gap) gap = dz;
                }
                const double gg = gap + (lx == cible ? 600 : 0);
                if (gg > bestGap) { bestGap = gg; best = lx; }
            }
            cible = best;
        }
        x += std::clamp(cible - x, -0.8 * h, 0.8 * h);
        double ahead = 1e9, aheadV = 0;
        for (const auto& c : tr.cars) {
            if (c.cache || std::abs(c.offset - x) > 0.3) continue;
            const double dz = t.relZ(c.z - pos);
            if (dz > 0 && dz < ahead) { ahead = dz; aheadV = c.speed; }
        }
        const double safe = (8 + v * M * 0.9) / M;
        const bool gaz = !(ahead < safe * 1.4 && aheadV < v), frein = ahead < safe && aheadV < v * 0.9;
        const double vmax = MAX_SPEED * 0.8;
        if (frein) v = std::max(0.0, v - MAX_SPEED * 0.9 * h);
        else if (gaz) v = std::min(vmax, v + MAX_SPEED * 0.12 * h);
        else v = std::max(0.0, v - MAX_SPEED * 0.05 * h);
        pos = t.wrap(pos + v * h);
    }
};

Mesure mesurer(const Traffic& tr, Track& t, double pos, const std::vector<double>& immobile) {
    Mesure m; m.total = (int)tr.cars.size();
    for (size_t i = 0; i < tr.cars.size(); i++) {
        const TrafficCar& c = tr.cars[i];
        if (c.cache) continue;
        const double dz = t.relZ(c.z - pos) * M;
        if (std::abs(dz) < 150) m.proches++;
        if (dz >= -30 && dz <= 300) m.fenetre++;
        if (c.traverse < 0 && !c.crashed && c.speed * M < 0.5) m.arretees++;
        m.immobileMax = std::max(m.immobileMax, immobile[i]);
    }
    return m;
}
} // namespace

int main(int argc, char** argv) {
    const char* seule = argc > 1 && argv[1][0] != '-' ? argv[1] : nullptr;
    bool journal = false, avant = false;
    for (int i = 1; i < argc; i++) { journal |= std::strcmp(argv[i], "--journal") == 0; avant |= std::strcmp(argv[i], "--avant") == 0; }
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        if (seule && std::strcmp(seule, carte) != 0) continue;
        auto tp = std::make_unique<Track>(); Track& t = *tp; t.build(mapById(carte), trace::versionCarte(carte));   // tracé du jeu (WFS-136)
        Traffic tr; tr.parallel = false; tr.totalFixe = !avant;   // comme TrafficSystem::init
        tr.reset(t, 0x5eed1u, mapById(carte).trafficN, 0);
        JoueurAuto j; j.v = 0; double dist = 0, prochainCp = CHECK_DIST;
        TrafficView pv; pv.myLen = webCarLength("golf", 4.29);
        const size_t total0 = tr.cars.size();
        std::vector<double> immobile(tr.cars.size(), 0);
        double fenetreMoy = 0, minute = 0, somme = 0; int nb = 0, nbMinute = 0, nbTotal = 0;
        double pireMinute = 0, pireImmobile = 0;
        // 3 s de compte à rebours puis 10 min de course ; moyenne de la fenêtre sur les 30 s qui précèdent chaque mesure
        const int PAS = (int)std::lround((3 + 600) / T_STEP);
        for (int s = 0; s <= PAS; s++) {
            pv.position = j.pos; pv.playerX = j.x; pv.speed = j.v;
            tr.advance(T_STEP, pv);
            if (tr.sim > 0) {
                const double p0 = j.pos;
                j.pas(tr, t, T_STEP);
                dist += t.relZ(j.pos - p0);
                if (dist >= prochainCp) { prochainCp += CHECK_DIST; if (tr.cars.size() < 110) tr.addCars(6, j.pos); immobile.resize(tr.cars.size(), 0); }
            }
            for (size_t i = 0; i < tr.cars.size(); i++) {
                const TrafficCar& c = tr.cars[i];
                immobile[i] = (c.traverse < 0 && !c.crashed && c.speed * M < 0.5) ? immobile[i] + T_STEP : 0;
            }
            if (tr.sim <= 0) continue;
            const Mesure m = mesurer(tr, t, j.pos, immobile);
            fenetreMoy += m.fenetre; nb++; minute += m.fenetre; nbMinute++; somme += m.fenetre; nbTotal++;
            const int ts = (int)std::lround(tr.sim / T_STEP);
            if (ts % (int)std::lround(30 / T_STEP) == 0) {
                const double moy = fenetreMoy / std::max(1, nb);
                if (journal)
                    std::printf("%s t=%3.0f s : total %d, < 150 m %d, fenêtre de dessin %d (moyenne 30 s %.1f), arrêtées %d, immobilité max %.1f s, joueur %.0f km/h\n",
                                carte, tr.sim, m.total, m.proches, m.fenetre, moy, m.arretees, m.immobileMax, j.v * M * 3.6);
                if (ts % (int)std::lround(60 / T_STEP) == 0) { pireMinute = std::max(pireMinute, minute / nbMinute); minute = 0; nbMinute = 0; }
                pireImmobile = std::max(pireImmobile, m.immobileMax);
                VERIFIE((size_t)m.total == total0, "%s t=%.0f s : %d voitures au lieu de %zu", carte, tr.sim, m.total, total0);
                fenetreMoy = 0; nb = 0;
            }
        }
        // référence : densité du départ (voitures posées uniformément sur le tracé) dans la fenêtre de dessin de 330 m
        const double ref = total0 * 330.0 / (t.length() * M), moyenne = somme / std::max(1, nbTotal);
        std::printf("%s : %zu voitures (constant : %s) ; fenêtre de dessin (30 m derrière, 300 m devant) : référence du départ %.2f, "
                    "moyenne sur 10 min %.2f (limite 1,3 × = %.2f), pire minute %.2f (limite 2 × + 2 = %.2f) ; immobilité max %.1f s (limite 20 s)\n",
                    carte, total0, tr.cars.size() == total0 ? "oui" : "non", ref, moyenne, 1.3 * ref, pireMinute, 2 * ref + 2, pireImmobile);
        VERIFIE(moyenne <= 1.3 * ref, "%s : RG-02 : %.2f voitures en moyenne dans la fenêtre de dessin (référence %.2f)", carte, moyenne, ref);
        VERIFIE(pireMinute <= 2 * ref + 2, "%s : RG-02 : minute à %.2f voitures dans la fenêtre de dessin (référence %.2f)", carte, pireMinute, ref);
        VERIFIE(pireImmobile <= 20, "%s : RG-03 : voiture arrêtée %.1f s (> 20 s)", carte, pireImmobile);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("trafic dans la durée : tout est OK\n");
    return 0;
}
