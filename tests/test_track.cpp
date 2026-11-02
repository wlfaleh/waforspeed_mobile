// Vérifie que la piste native est identique à celle du jeu web (mêmes segments, mêmes poses).
#include <cstdio>
#include "../src/game/track.h"
#include "../src/game/player.h"
using namespace wfs;
int main() {
    for (const char* id : {"ville", "plage", "campagne", "circuit"}) {
        Track t; t.build(mapById(id));
        TrackPoint p = t.point(1234.5 * SEG, 3.0), q = t.point(-50.25 * SEG, -2.0);
        std::printf("%s %zu %.6f %.6f %.6f %.6f | %.6f %.6f %.6f\n", id, t.segmentCount(), p.x, p.y, p.z, p.th, q.x, q.y, q.z);
    }
    // conduite : 10 s plein gaz en Golf
    Track t; t.build(mapById("ville")); Player pl; pl.reset(carById("golf"));
    DriveInput in; in.gas = true;
    for (int i = 0; i < 600; i++) pl.update(1.0 / 60, in, t, true);
    std::printf("golf 10s: %.1f km/h, %.1f m\n", pl.kmh(), pl.dist * M);
}
