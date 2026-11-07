// WaForSpeed — WFS-268 : décor fixe préparé au chargement, passage d'un tour au suivant (src/render/lots_decor_tour.h).
// Le décor d'un seul tour (tour 0) est dessiné à chaque tour avec la matrice du tour : il faut que cette matrice amène
// exactement un point du tour 0 sur le même point du tour k, sur les 4 cartes (tracés 1 et 2), tours négatifs compris
// (morceaux derrière la ligne de départ), pour des points sur l'axe et loin sur les côtés (façades, arbres à 60 m).
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
#include "../src/render/lots_decor_tour.h"

using namespace wfs;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("lots du décor : %-96s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    for (const char* id : {"ville", "campagne", "plage", "circuit"}) for (int version : {1, 2}) {
        Track t; t.build(mapById(id), version);
        const lots::Tour tr = lots::mesurerTour(t);
        const std::string nom = std::string(id) + " (tracé " + std::to_string(version) + ")";
        cas(nom + " : tour mesuré (" + std::to_string(tr.L) + " segments, " + std::to_string(tr.theta * 57.29578) + "°)", tr.L > 0);
        cas(nom + " : contrôle de la mesure, écart < 1 cm", tr.ecart < 0.01);
        double pire = 0;
        for (int64_t k : {-2, -1, 1, 2, 3, 7}) {
            const mat4 M = lots::matriceDuTour(t, tr, k);
            for (int64_t a = 0; a < tr.L; a += std::max<int64_t>(1, tr.L / 37)) for (double lat : {0.0, -18.0, 60.0}) {
                const TrackPoint p0 = t.point((double)a * SEG, lat), pk = t.point((double)(a + k * tr.L) * SEG, lat);
                const vec3 q = M.transformPoint({(float)p0.x, (float)p0.y, (float)p0.z});
                pire = std::max(pire, std::sqrt((q.x - pk.x) * (q.x - pk.x) + (q.y - pk.y) * (q.y - pk.y) + (q.z - pk.z) * (q.z - pk.z)));
            }
        }
        // flottants (32 bits) à quelques kilomètres de l'origine : quelques millimètres au plus
        cas(nom + " : point du tour 0 transporté = même point des tours −2 à 7, pire écart " + std::to_string(pire * 1000) + " mm (≤ 2 cm)", pire <= 0.02);
        cas(nom + " : tour 0 = identité", lots::matriceDuTour(t, tr, 0).transformPoint({1, 2, 3}).x == 1.f);
    }
    std::printf("lots du décor : %s\n", echecs ? "ÉCHEC" : "tout est OK");
    return echecs ? 1 : 0;
}
