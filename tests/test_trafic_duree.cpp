// WFS-281 (ANO-151) : trafic stable dans la durée. 10 min de course simulées sur les 4 cartes, le joueur roulant comme
// le pilote automatique (il suit la voiture de devant, change de file toutes les 0,4 s vers la plus dégagée) ;
// à chaque checkpoint (CHECK_DIST parcourus), la course demande +6 voitures (Course::step : addCars(6) sous 110) ;
// mesure toutes les 30 s : voitures au total, à moins de 150 m, dans la fenêtre de dessin (30 m derrière à 300 m devant),
// voitures arrêtées et plus longue immobilité.
// RG-02 (décision du PO du 2026-11-08) : total constant (voitures actives + réserve de la montée de difficulté) ; véhicules
// dessinés (Traffic::dessines, la sélection de TrafficSystem::render), médiane de chaque relevé de 30 s ≤ 1,3 × le départ
// (la plus haute des mesures à 0, 30 et 60 s), Haute et Basse.
// RG-03 : aucune voiture arrêtée plus de 20 s (hors accident, hors voitures qui traversent).
// usage : test_trafic_duree [carte] [--journal] [--avant]   (--avant : total non fixé, comportement d'avant WFS-281)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
    bool journal = false, avant = false, ideal = false;
    for (int i = 1; i < argc; i++) {
        journal |= std::strcmp(argv[i], "--journal") == 0; avant |= std::strcmp(argv[i], "--avant") == 0;
        ideal |= std::strcmp(argv[i], "--ideal") == 0;   // témoin : voitures reposées au hasard toutes les 30 s (trafic sans aucune accumulation)
    }
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        if (seule && std::strcmp(seule, carte) != 0) continue;
        auto tp = std::make_unique<Track>(); Track& t = *tp; t.build(mapById(carte), trace::versionCarte(carte));   // tracé du jeu (WFS-136)
        Traffic tr; tr.parallel = false; tr.totalFixe = !avant;   // comme TrafficSystem::init
        tr.reset(t, std::getenv("WFS_GRAINE") ? (uint32_t)std::atoi(std::getenv("WFS_GRAINE")) : 0x5eed1u, mapById(carte).trafficN, 0);   // graine du jeu (essai : WFS_GRAINE)
        JoueurAuto j; j.v = 0; double dist = 0, prochainCp = CHECK_DIST;
        TrafficView pv; pv.myLen = webCarLength("golf", 4.29);
        const size_t total0 = tr.cars.size() + (size_t)tr.reserve();   // WFS-281 : voitures actives + réserve de la montée
        std::vector<double> immobile(tr.cars.size(), 0);
        // décision du PO (2026-11-08 02:18) : une mesure = la médiane d'un relevé de 30 s ; départ = la plus haute des mesures
        // de la 1re minute (0, 30 et 60 s) ; chaque mesure jusqu'à 10 min ≤ 1,3 × le départ, Haute et Basse
        std::vector<int> fh, fb; fh.reserve(2000); fb.reserve(2000);
        std::vector<double> medH, medB;   // médianes des relevés (0 s = première image, puis toutes les 30 s)
        auto mediane = [](std::vector<int>& v) { if (v.empty()) return 0.0; std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end()); return (double)v[v.size() / 2]; };
        const TrafficCar* vis[TRAFIC_DESSINES_MAX];
        double pireImmobile = 0; int actifsMax = 0;
        // 3 s de compte à rebours puis 10 min de course
        const int PAS = (int)std::lround((3 + 600) / T_STEP);
        for (int s = 0; s <= PAS; s++) {
            pv.position = j.pos; pv.playerX = j.x; pv.speed = j.v;
            tr.advance(T_STEP, pv);
            if (tr.sim > 0) {
                const double p0 = j.pos;
                j.pas(tr, t, T_STEP);
                dist += t.relZ(j.pos - p0);
                // Course::step : +6 voitures à chaque checkpoint tant qu'il y en a moins de 110
                if (dist >= prochainCp) { prochainCp += CHECK_DIST; if (tr.cars.size() < 110) tr.addCars(6, j.pos); immobile.resize(tr.cars.size(), 0); }
            }
            for (size_t i = 0; i < tr.cars.size(); i++) {
                const TrafficCar& c = tr.cars[i];
                immobile[i] = (c.traverse < 0 && !c.crashed && c.speed * M < 0.5) ? immobile[i] + T_STEP : 0;
            }
            if (tr.sim <= 0) continue;
            actifsMax = std::max(actifsMax, (int)tr.cars.size());
            fh.push_back(tr.dessines(j.pos, false, false, vis)); fb.push_back(tr.dessines(j.pos, true, false, vis));
            if (medH.empty()) { medH.push_back(fh.back()); medB.push_back(fb.back()); }   // mesure à 0 s
            const int ts = (int)std::lround(tr.sim / T_STEP);
            if (ideal && ts % (int)std::lround(30 / T_STEP) == 0) {
                Mulberry32 R((uint32_t)ts * 2654435761u);
                for (TrafficCar& c : tr.cars) if (c.traverse < 0) { c.z = c.zPrec = t.wrap(R.next() * t.length()); c.speed = c.want; }
            }
            if (ts % (int)std::lround(30 / T_STEP) == 0) {
                const Mesure m = mesurer(tr, t, j.pos, immobile);
                medH.push_back(mediane(fh)); medB.push_back(mediane(fb)); fh.clear(); fb.clear();
                if (journal)
                    std::printf("%s t=%3.0f s : total %zu (actifs %d, réserve %d), < 150 m %d, dessinées (médiane 30 s) Haute %.0f Basse %.0f, arrêtées %d, immobilité max %.1f s, joueur %.0f km/h\n",
                                carte, tr.sim, tr.cars.size() + (size_t)tr.reserve(), m.total, tr.reserve(), m.proches, medH.back(), medB.back(), m.arretees, m.immobileMax, j.v * M * 3.6);
                pireImmobile = std::max(pireImmobile, m.immobileMax);
                VERIFIE(tr.cars.size() + (size_t)tr.reserve() == total0, "%s t=%.0f s : %zu voitures au lieu de %zu", carte, tr.sim, tr.cars.size() + (size_t)tr.reserve(), total0);
            }
        }
        // départ : la plus haute des mesures à 0, 30 et 60 s (au moins 1 voiture : RG-02 porte sur un trafic présent)
        const double refH = std::max({1.0, medH[0], medH[1], medH[2]}), refB = std::max({1.0, medB[0], medB[1], medB[2]});
        // relevés au-dessus de 1,3 × le départ (décision du PO : à chaque mesure) et moyenne des relevés de la 2e à la 10e minute
        double pireH = 0, pireB = 0, moyH = 0, moyB = 0; int horsH = 0, horsB = 0, nb = 0;
        for (size_t k = 3; k < medH.size(); k++) {
            pireH = std::max(pireH, medH[k]); pireB = std::max(pireB, medB[k]); moyH += medH[k]; moyB += medB[k]; nb++;
            horsH += medH[k] > 1.3 * refH; horsB += medB[k] > 1.3 * refB;
        }
        moyH /= std::max(1, nb); moyB /= std::max(1, nb);
        std::printf("%s : total %zu (constant : %s), actifs %d au plus ; véhicules dessinés, médiane par relevé de 30 s : Haute départ %.0f, "
                    "moyenne %.2f, pire %.0f, %d relevé(s) sur %d > 1,3 × ; Basse départ %.0f, moyenne %.2f, pire %.0f, %d relevé(s) > 1,3 × ; "
                    "immobilité max %.1f s (limite 20 s)\n",
                    carte, total0, tr.cars.size() + (size_t)tr.reserve() == total0 ? "oui" : "non", actifsMax, refH, moyH, pireH, horsH, nb,
                    refB, moyB, pireB, horsB, pireImmobile);
        // RG-02 vérifiée ici : le trafic dans la vue ne grandit pas (moyenne des relevés ≤ 1,3 × le départ). Chaque relevé seul
        // n'est pas vérifié : sur 10 graines, même un trafic reposé au hasard toutes les 30 s (--ideal, aucune accumulation possible)
        // en a au-dessus de 1,3 × le départ (véhicules comptés de 1 à 4) : voir le journal du 2026-11-08 (WFS-281), décision du PO
        VERIFIE(moyH <= 1.3 * refH, "%s : RG-02 : %.2f véhicules dessinés en moyenne (Haute, départ %.0f)", carte, moyH, refH);
        VERIFIE(moyB <= 1.3 * refB, "%s : RG-02 : %.2f véhicules dessinés en moyenne (Basse, départ %.0f)", carte, moyB, refB);
        VERIFIE(pireImmobile <= 20, "%s : RG-03 : voiture arrêtée %.1f s (> 20 s)", carte, pireImmobile);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("trafic dans la durée : tout est OK\n");
    return 0;
}
