// WaForSpeed — WFS-387 (RG-11, ANO-234) : épingles du col de la Campagne, la voiture s'arrête au parapet VISIBLE.
// Le parapet est dessiné face côté route à RW + 0,5 = 8,0 m (placerCol, campagne.cpp) ; la glissière de la piste est à 10,35 m
// (edge 1,38). Le décor déclare pour chaque tronçon une boîte de choc (WFS-256) tirée des mêmes mesures que le dessin
// (campagne::chocMuret, decor_campagne_modeles.h) : ce test la déclare de la même façon sur la vraie piste, lance la voiture
// droit sur le parapet extérieur de chaque épingle (30, 80 et 150 km/h, deux modèles de conduite) et vérifie :
//   - au moins un choc avec le parapet (la voiture ne file pas jusqu'à la glissière de 10,35 m) ;
//   - le coin de la voiture le plus avancé vers le parapet s'arrête à la face visible (8,0 m) à ± 0,2 m (RG-11) : ni avant
//     (mur invisible), ni dedans.
// usage : test_muret_campagne
#include <cmath>
#include <cstdio>
#include "../src/game/chocs_banc.h"
#include "../src/game/conduite_banc.h"
#include "../src/game/trace_cartes.h"
#include "../src/world/decor_campagne_modeles.h"
using namespace wfs;

int main() {
    Track& t = banc_conduite::piste("campagne");
    const MapDef& map = mapById("campagne");
    chocs::declarerCarte(t, map);   // registre préparé pour la piste (la Campagne n'y met rien : son décor se déclare au chargement)
    const float R = (float)RW, lat = R + 0.5f, SG = (float)SEG_M;
    const campagne::ChocMuret cm = campagne::chocMuret(lat);
    // épingles : segments à courbure forte dans les lacets (mêmes règles que placerCol)
    struct Ep { int64_t debut, fin; };
    std::vector<Ep> eps;
    for (const trace::Section& sec : t.sections()) {
        if (sec.type != trace::Type::Lacets) continue;
        int64_t d = -1;
        for (int64_t a = sec.debut; a < sec.fin; a++) {
            const bool ep = std::abs(t.segAt(a).curve) > 10;
            if (ep && d < 0) d = a;
            if (!ep && d >= 0) { eps.push_back({d, a}); d = -1; }
            if (ep) for (int side : {-1, 1}) {
                chocs::ObstacleRoute o; o.forme = chocs::Forme::Boite; o.lat = side * cm.centre; o.hx = cm.demi; o.hz = SG / 2 + 0.05f;
                o.along = SG / 2; o.genre = chocs::Genre::Mur;
                chocs::declarerObjet(t, a, o, false);
            }
        }
        if (d >= 0) eps.push_back({d, sec.fin});
    }
    int ecarts = 0;
    std::printf("parapet visible : face côté route à %.2f m, boîte de choc de %.2f à %.2f m ; %zu épingle(s)\n", lat, cm.centre - cm.demi, cm.centre + cm.demi, eps.size());
    if (eps.empty()) { std::printf("[ÉCHEC] aucune épingle trouvée sur la piste de la Campagne\n"); return 1; }
    const bool mondeDefaut = g_conduiteMonde;
    for (bool monde : {true, false}) {
        g_conduiteMonde = monde;
        for (const Ep& e : eps) {
            const int64_t a = (e.debut + e.fin) / 2;
            // côté extérieur de l'épingle : opposé au sens du virage (courbure > 0 : virage à droite → parapet extérieur à gauche)
            const int ext = t.segAt(a).curve > 0 ? -1 : 1;
            for (double kmh : {30.0, 80.0, 150.0}) {
                Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee;
                const double s0 = a * SEG + SG / 2 / M, depart = s0 - 12 / M, latD = -ext * 1.0;
                p.dist = depart; p.position = t.wrap(depart); p.playerX = latD / RW; p.speed = kmh / 3.6 / M;
                const TrackPoint A = t.pointLisse(depart, latD), B = t.pointLisse(s0, ext * cm.centre);   // visée : milieu du parapet
                p.heading = std::remainder(std::atan2(B.x - A.x, -(B.z - A.z)) - A.th, 2 * 3.14159265358979);
                int nChocs = 0; double latMax = -1e9;
                for (int n = 0; n < 120 * 4; n++) {
                    DriveInput in; in.gas = nChocs == 0 && p.chocsDecor.n == 0 && p.kmh() < kmh;
                    p.update(1.0 / 120, in, t, true);
                    nChocs += p.chocsDecor.n; p.chocsDecor = {};
                    // coin de la voiture le plus avancé vers le parapet (repère de la route : cap relatif p.heading)
                    const double h = p.heading, coin = ext * p.playerX * RW + p.car->d.L / 2 * std::abs(std::sin(h)) + p.car->d.W / 2 * std::abs(std::cos(h));
                    latMax = std::max(latMax, coin);
                }
                // le coin le plus avancé de la voiture s'arrête à la face visible du parapet (8,0 m), à 0,2 m près
                const bool bon = nChocs >= 1 && std::abs(latMax - lat) <= 0.2;
                std::printf("%s %s, épingle %lld-%lld (parapet %s), %.0f km/h : %d choc(s), coin de la voiture arrêté à %.2f m (face visible %.2f ± 0,2)\n",
                            bon ? "[OK]   " : "[ÉCHEC]", monde ? "conduite réelle" : "conduite d'avant", (long long)e.debut, (long long)e.fin, ext > 0 ? "droit" : "gauche",
                            kmh, nChocs, latMax, lat);
                if (!bon) ecarts++;
            }
        }
    }
    g_conduiteMonde = mondeDefaut;
    std::printf("parapet des épingles (WFS-387, RG-11) : %s\n", ecarts ? "ÉCHEC" : "OK");
    return ecarts ? 1 : 0;
}
