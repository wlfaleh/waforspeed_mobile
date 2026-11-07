// WFS-212 (RG-04) : « rien sur la chaussée » au tracé 2 de la Ville, de la Campagne et de la Plage, là où le tracé se replie
// (rond-point, épingles, lacets). Le décor (trottoirs, bas-côtés, touffes, objets) n'est posé que si chaussee::surAutre dit
// qu'il ne tombe pas sur une AUTRE partie du tracé ; ce test vérifie, contre un calcul brut indépendant (distance à tous les
// segments du tour), que ce prédicat ne laisse passer aucun point posé sur une chaussée :
//   - autour de chaque section repliée, des points de RW + 0,3 à RW + 12 m des deux côtés (trottoir, bas-côté, touffes) :
//     brut « à moins de RW + marge d'une autre partie » ⇒ surAutre vrai (aucun oubli de la fenêtre ± 80 m) ;
//   - le défaut des recettes existe bien sans le filtre (des points du trottoir de Game tombent sur une autre partie de la
//     route au rond-point de la Ville et de la Campagne) : le filtre est nécessaire, et il les écarte tous.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/world/chaussee.h"
using namespace wfs;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("chaussée : %-92s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    for (const char* id : {"ville", "campagne", "plage"}) {
        Track t; t.build(mapById(id), trace::versionCarte(id));
        const int64_t L = (int64_t)t.segmentCount();
        const auto ax = chaussee::axes(t, {trace::Type::RondPoint, trace::Type::Epingle, trace::Type::Lacets});
        cas(std::string(id) + " : sections repliées trouvées au tracé 2 (" + std::to_string(ax.size()) + ")", !ax.empty());
        std::vector<vec3> axe((size_t)L);
        for (int64_t a = 0; a < L; a++) { const TrackPoint p = t.point((double)a * SEG, 0); axe[(size_t)a] = {(float)p.x, (float)p.y, (float)p.z}; }
        const int64_t propre = (int64_t)(chaussee::PROPRE_M / SEG_M);
        auto brut = [&](const vec3& q, int64_t seg, float marge) {   // distance à TOUS les segments du tour (± 30 m exclus)
            const float lim2 = ((float)RW + marge) * ((float)RW + marge);
            for (int64_t a = 0; a < L; a++) {
                if (std::llabs(chaussee::ecartSigne(a, seg, L)) < propre) continue;
                const vec3& o = axe[(size_t)a];
                if ((o.x - q.x) * (o.x - q.x) + (o.z - q.z) * (o.z - q.z) < lim2) return true;
            }
            return false;
        };
        int points = 0, dessus = 0, oublis = 0, trottoir = 0;
        for (const chaussee::Axe& x : ax) {
            for (int64_t k = -x.rayonSeg; k <= x.rayonSeg; k += 2) {
                const int64_t seg = chaussee::modL(x.c + k, L);
                for (int side : {-1, 1}) for (float lat = (float)RW + 0.3f; lat <= (float)RW + 12; lat += 0.7f) {
                    const TrackPoint p = t.point((double)seg * SEG, side * lat);
                    const vec3 q{(float)p.x, (float)p.y, (float)p.z};
                    points++;
                    const bool b = brut(q, seg, 0.5f);
                    if (b) { dessus++; if (lat <= (float)RW + 4) trottoir++; }
                    if (b && !chaussee::surAutre(ax, L, q, seg, 0.5f)) oublis++;
                }
            }
        }
        std::printf("chaussée : %s : %d points testés, %d sur une autre partie de la route (dont %d sur le trottoir de RW à RW + 4 m)\n",
                    id, points, dessus, trottoir);
        cas(std::string(id) + " : aucun point posé sur une autre chaussée n'échappe au filtre", oublis == 0);
        if (std::string(id) != "plage") cas(std::string(id) + " : le défaut existe sans filtre (trottoir ou bas-côté sur une autre branche)", dessus > 0);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("chaussée : tout est OK\n");
    return 0;
}
