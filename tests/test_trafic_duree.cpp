// WFS-281 (ANO-151) : trafic stable dans la durée. 10 min de course simulées sur les 4 cartes, le joueur roulant comme
// le pilote automatique (il suit la voiture de devant, change de file toutes les 0,4 s vers la plus dégagée) ;
// à chaque checkpoint (CHECK_DIST parcourus), la course demande +6 voitures (Course::step : addCars(6) sous 110) ;
// mesure toutes les 30 s : voitures au total, à moins de 150 m, dans la fenêtre de dessin (30 m derrière à 300 m devant),
// voitures arrêtées et plus longue immobilité.
// RG-02 (décision du PO du 2026-11-08 20:17:58, regles-de-gestion/WFS-281.md) : total constant (voitures actives + réserve
// de la montée de difficulté) ; mesure = P90 des véhicules dessinés (Traffic::dessines, la sélection de TrafficSystem::render)
// d'une fenêtre de 30 s ; R = la plus haute P90 de la 1re minute ; CHAQUE fenêtre jusqu'à 10 min ≤ max(1,3 × R ; R + 2) et
// ≤ 8, Haute et Basse, 4 cartes, 10 graines : 0 fenêtre hors règle sur 800.
// RG-03 : aucune voiture arrêtée plus de 20 s (hors accident, hors voitures qui traversent).
// usage : test_trafic_duree [carte] [--journal] [--avant] [--ideal]   (--avant : total non fixé, comportement d'avant WFS-281 ;
//         --journal : relevé de chaque fenêtre de la graine 1 ; WFS_GRAINE=n : une seule graine)
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
    int horsTotal = 0, fenetresTotal = 0; double moy1Total = 0, moyRTotal = 0; int nbCas = 0;   // trafic pas plus léger : P90 moyenne, 1re minute / suite
    const int g0 = std::getenv("WFS_GRAINE") ? std::atoi(std::getenv("WFS_GRAINE")) : 1, g1 = std::getenv("WFS_GRAINE") ? g0 : 10;
    for (int graine = g0; graine <= g1; graine++)
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        if (seule && std::strcmp(seule, carte) != 0) continue;
        const bool jr = journal && graine == g0;
        auto tp = std::make_unique<Track>(); Track& t = *tp; t.build(mapById(carte), trace::versionCarte(carte));   // tracé du jeu (WFS-136)
        Traffic tr; tr.parallel = false; tr.totalFixe = !avant;   // comme TrafficSystem::init
        tr.reset(t, (uint32_t)graine * 0x9e3779b1u, mapById(carte).trafficN, 0);
        JoueurAuto j; j.v = 0; double dist = 0, prochainCp = CHECK_DIST;
        TrafficView pv; pv.myLen = webCarLength("golf", 4.29);
        const size_t total0 = tr.cars.size() + (size_t)tr.reserve();   // WFS-281 : voitures actives + réserve de la montée
        std::vector<double> immobile(tr.cars.size(), 0);
        // décision du PO (2026-11-08 02:18) : une mesure = la médiane d'un relevé de 30 s ; départ = la plus haute des mesures
        // de la 1re minute (0, 30 et 60 s) ; chaque mesure jusqu'à 10 min ≤ 1,3 × le départ, Haute et Basse
        std::vector<int> fh, fb; fh.reserve(2000); fb.reserve(2000);
        std::vector<double> medH, medB;   // médianes des relevés (0 s = première image, puis toutes les 30 s)
        auto mediane = [](std::vector<int>& v) {   // P90 de la fenêtre (décision du PO : garde les pics, pas une image seule)
            if (v.empty()) return 0.0;
            const size_t k = std::min(v.size() - 1, (size_t)std::ceil(0.9 * (double)v.size()) - 1);
            std::nth_element(v.begin(), v.begin() + (long)k, v.end()); return (double)v[k];
        };
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
            const int ts = (int)std::lround(tr.sim / T_STEP);
            if (ideal && ts % (int)std::lround(30 / T_STEP) == 0) {
                Mulberry32 R((uint32_t)ts * 2654435761u);
                for (TrafficCar& c : tr.cars) if (c.traverse < 0) { c.z = c.zPrec = t.wrap(R.next() * t.length()); c.speed = c.want; }
            }
            if (ts % (int)std::lround(30 / T_STEP) == 0) {
                const Mesure m = mesurer(tr, t, j.pos, immobile);
                medH.push_back(mediane(fh)); medB.push_back(mediane(fb)); fh.clear(); fb.clear();
                if (jr)
                    std::printf("%s t=%3.0f s : total %zu (actifs %d, réserve %d), < 150 m %d, dessinées (P90 30 s) Haute %.0f Basse %.0f, arrêtées %d, immobilité max %.1f s, joueur %.0f km/h\n",
                                carte, tr.sim, tr.cars.size() + (size_t)tr.reserve(), m.total, tr.reserve(), m.proches, medH.back(), medB.back(), m.arretees, m.immobileMax, j.v * M * 3.6);
                pireImmobile = std::max(pireImmobile, m.immobileMax);
                VERIFIE(tr.cars.size() + (size_t)tr.reserve() == total0, "%s t=%.0f s : %zu voitures au lieu de %zu", carte, tr.sim, tr.cars.size() + (size_t)tr.reserve(), total0);
            }
        }
        // R : la plus haute P90 de la 1re minute (fenêtres 0–30 s et 30–60 s) ; chaque fenêtre ≤ max(1,3 × R ; R + 2) et ≤ 8
        const double refH = std::max(medH[0], medH[1]), refB = std::max(medB[0], medB[1]);
        const double limH = std::min(8.0, std::max(1.3 * refH, refH + 2)), limB = std::min(8.0, std::max(1.3 * refB, refB + 2));
        int horsH = 0, horsB = 0; double pireH = 0, pireB = 0, moy1 = (medH[0] + medH[1]) / 2, moyR = 0;
        for (size_t k = 2; k < medH.size(); k++) moyR += medH[k] / (double)(medH.size() - 2);
        moy1Total += moy1; moyRTotal += moyR; nbCas++;
        for (size_t k = 0; k < medH.size(); k++) {
            pireH = std::max(pireH, medH[k]); pireB = std::max(pireB, medB[k]);
            horsH += medH[k] > limH; horsB += medB[k] > limB;
        }
        horsTotal += horsH + horsB; fenetresTotal += 2 * (int)medH.size();
        std::printf("graine %2d %-8s : total %zu (constant : %s), actifs %d au plus ; véhicules dessinés (P90 de 30 s) : Haute R %.0f, pire %.0f, "
                    "limite %.1f, %d fenêtre(s) hors règle sur %zu ; Basse R %.0f, pire %.0f, limite %.1f, %d hors règle ; immobilité max %.1f s\n",
                    graine, carte, total0, tr.cars.size() + (size_t)tr.reserve() == total0 ? "oui" : "non", actifsMax, refH, pireH, limH, horsH,
                    medH.size(), refB, pireB, limB, horsB, pireImmobile);
        VERIFIE(medH.size() == 20, "%s : %zu fenêtres de 30 s au lieu de 20", carte, medH.size());
        VERIFIE(horsH == 0, "graine %d %s : RG-02 : %d fenêtre(s) au-dessus de %.1f véhicules (Haute, R = %.0f)", graine, carte, horsH, limH, refH);
        VERIFIE(horsB == 0, "graine %d %s : RG-02 : %d fenêtre(s) au-dessus de %.1f véhicules (Basse, R = %.0f)", graine, carte, horsB, limB, refB);
        VERIFIE(pireImmobile <= 20, "%s : RG-03 : voiture arrêtée %.1f s (> 20 s)", carte, pireImmobile);
    }
    std::printf("véhicules dessinés (Haute, P90 de 30 s, moyenne des cartes et graines) : 1re minute %.2f, minutes 2 à 10 %.2f\n", moy1Total / std::max(1, nbCas), moyRTotal / std::max(1, nbCas));
    std::printf("RG-02 : %d fenêtre(s) hors règle sur %d (Haute et Basse, %d graine(s), 4 cartes ; attendu : 0)\n", horsTotal, fenetresTotal, g1 - g0 + 1);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("trafic dans la durée : tout est OK\n");
    return 0;
}
