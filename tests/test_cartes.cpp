// WFS-14 : le tracé natif des cartes plage, campagne et circuit est identique à celui du jeu web (buildTrack, même graine) :
// même nombre de segments, même courbure et mêmes altitudes pour chaque segment.
// Référence : tests/cartes_web_ref.txt, produite par `node tests/cartes_web.cjs <jeu web>/waforspeed`.
// Tolérance : 1e-9 (WFS-55 : easeInOut de src/game/track.cpp calcule en double, comme Math.PI du jeu web).
// usage : test_cartes tests/cartes_web_ref.txt
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include "../src/game/track.h"
using namespace wfs;

int main(int argc, char** argv) {
    std::ifstream f(argc > 1 ? argv[1] : "tests/cartes_web_ref.txt");
    if (!f) { std::printf("ÉCHEC : référence introuvable\n"); return 2; }
    int totalBad = 0;
    std::string id; size_t n;
    while (f >> id >> n) {
        Track t; t.build(mapById(id.c_str()));
        int bad = 0; double worst = 0;
        if (t.segmentCount() != n) { std::printf("%s : %zu segments au lieu de %zu\n", id.c_str(), t.segmentCount(), n); bad++; }
        for (size_t i = 0; i < n; i++) {
            double c, y1, y2; f >> c >> y1 >> y2;
            if (i >= t.segmentCount()) continue;
            const Segment& s = t.segAt((int64_t)i);
            const double e = std::max({std::abs(s.curve - c), std::abs(s.y1 - y1), std::abs(s.y2 - y2)});
            worst = std::max(worst, e);
            if (e > 1e-9) { if (bad++ < 3) std::printf("%s segment %zu : natif %.9f %.6f %.6f | web %.9f %.6f %.6f\n", id.c_str(), i, s.curve, s.y1, s.y2, c, y1, y2); }
        }
        std::printf("%s : %zu segments comparés au jeu web, %d écart(s) (écart max %.2g)\n", id.c_str(), n, bad, worst);
        totalBad += bad;
    }
    std::printf(totalBad ? "ÉCHEC\n" : "OK\n");
    return totalBad ? 1 : 0;
}
