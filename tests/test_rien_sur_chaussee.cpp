// WFS-212 (RG-04) : « rien sur la chaussée » au tracé 2 de la Ville, de la Campagne et de la Plage, là où le tracé se replie
// (rond-point, épingles, lacets). Le décor (trottoirs, bas-côtés, touffes, objets) n'est posé que si chaussee::surAutre dit
// qu'il ne tombe pas sur une AUTRE partie du tracé ; ce test vérifie, contre un calcul brut indépendant (distance à tous les
// segments du tour), que ce prédicat ne laisse passer aucun point posé sur une chaussée :
//   - autour de chaque section repliée, des points de RW + 0,3 à RW + 12 m des deux côtés (trottoir, bas-côté, touffes) :
//     brut « à moins de RW + marge d'une autre partie » ⇒ surAutre vrai (aucun oubli de la fenêtre ± 80 m) ;
//   - le défaut des recettes existe bien sans le filtre (des points du trottoir de Game tombent sur une autre partie de la
//     route au rond-point de la Ville et de la Campagne) : le filtre est nécessaire, et il les écarte tous.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/world/chaussee.h"
#include "../src/world/bouchons_sol.h"   // WFS-283 : disques de sol du carrefour simple (Corniche)
#include "../src/world/giratoire.cpp"   // WFS-248 : giratoires reconstruits (compilé ici : aucune ligne de CMake)
using namespace wfs;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("chaussée : %-92s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    // WFS-283 : les sections repliées de la Ville et le défaut sans filtre venaient des ronds-points : 1er passage avec les
    // ronds-points remis (code gardé, RG-04), 2e passage comme le jeu (carrefours simples)
    for (const bool avecRp : {true, false}) {
    trace::forcerRondsPoints(avecRp);
    for (const char* id : {"ville", "campagne", "plage"}) {
        Track t; t.build(mapById(id), trace::versionCarte(id));
        const int64_t L = (int64_t)t.segmentCount();
        const auto ax = chaussee::axes(t, {trace::Type::RondPoint, trace::Type::Epingle, trace::Type::Lacets});
        if (avecRp) cas(std::string(id) + " : sections repliées trouvées au tracé 2 (" + std::to_string(ax.size()) + ")", !ax.empty());
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
        cas(std::string(id) + (avecRp ? "" : " (carrefours)") + " : aucun point posé sur une autre chaussée n'échappe au filtre", oublis == 0);
        if (avecRp && std::string(id) != "plage") cas(std::string(id) + " : le défaut existe sans filtre (trottoir ou bas-côté sur une autre branche)", dessus > 0);
        // WFS-283 (RG-02) : le carrefour simple ne replie plus la route : autour de lui (±20 m, jusqu'à 12 m du bord), aucune
        // autre partie de la chaussée, donc rien à cacher sous le décor
        if (!avecRp) for (const trace::Section& s : trace::sections(t)) if (s.exRondPoint) {
            int n = 0, autre = 0;
            for (int64_t a = s.debut - (int64_t)(20 / SEG_M); a <= s.fin + (int64_t)(20 / SEG_M); a++)
                for (int side : {-1, 1}) for (float lat = (float)RW + 0.3f; lat <= (float)RW + 12; lat += 0.7f) {
                    const TrackPoint p = t.point((double)chaussee::modL(a, L) * SEG, side * lat); n++;
                    if (brut({(float)p.x, (float)p.y, (float)p.z}, chaussee::modL(a, L), 0.5f)) autre++;
                }
            cas(std::string(id) + " : carrefour simple : aucune autre partie de la route autour (" + std::to_string(autre) + " / " + std::to_string(n) + " points)", autre == 0);
            // WFS-283 (revue du lead dev, 2e refus) : disques de sol au centre du virage (plage.cpp, bouchons_sol.h) : aucun point
            // d'un disque à moins de RW + 0,6 m de l'axe, contre un calcul brut (ligne brisée de TOUT le tour, entrée et sortie
            // progressives comprises). Sans la borne (commit 3e76065), les disques des segments d'entrée et de sortie (rayon 26 à
            // 39 m, centrés loin du centre du virage de 22 m) débordaient sur la voie intérieure : le test le montre aussi.
            auto pire = [&](bool borne, int& n) {
                double m = 1e18; n = 0;
                for (const bouchons::Disque& d : bouchons::section(t, s, borne)) {
                    n++;
                    double da = 1e18;
                    for (int64_t a = 0; a < L; a++) {
                        const vec3 &p = axe[(size_t)a], &q = axe[(size_t)((a + 1) % L)];
                        const double ux = q.x - p.x, uz = q.z - p.z, l2 = ux * ux + uz * uz;
                        const double u = l2 > 0 ? std::clamp(((d.cx - p.x) * ux + (d.cz - p.z) * uz) / l2, 0.0, 1.0) : 0.0;
                        da = std::min(da, std::hypot(d.cx - (p.x + u * ux), d.cz - (p.z + u * uz)));
                    }
                    m = std::min(m, da - d.r);   // bord du disque le plus proche de l'axe
                }
                return m;
            };
            int nSans = 0, nAvec = 0;
            const double sans = pire(false, nSans), avec = pire(true, nAvec), bord = RW + bouchons::MARGE - 1e-3;   // 1 mm : axe du test en float
            std::printf("chaussée : %s : disques de sol du carrefour simple : sans borne %d disque(s), bord à %.2f m de l'axe ; avec borne %d, à %.2f m (chaussée : %.1f m)\n",
                        id, nSans, sans, nAvec, avec, RW);
            cas(std::string(id) + " : carrefour simple : disques de sol hors de la chaussée (≥ RW + 0,6 m de l'axe)", nAvec > 0 && avec >= bord);
            if (std::string(id) == "plage") cas("plage : carrefour simple : sans la borne, des disques débordaient sur la chaussée (défaut de 3e76065)", sans < bord);
        }
    }
    }
    trace::forcerRondsPoints(true);   // giratoires reconstruits : code gardé (RG-04 de WFS-283)
    // ---------- WFS-248 : giratoires reconstruits (Ville, Campagne, Corniche) ----------
    for (const char* id : {"ville", "campagne", "plage"}) {
        Track t; t.build(mapById(id), trace::versionCarte(id));
        for (const trace::Section& s : trace::sections(t)) {
            if (s.type != trace::Type::RondPoint) continue;
            const giratoire::Giratoire g = giratoire::calculer(t, s);
            const std::string n = std::string(id) + " : giratoire du segment " + std::to_string(s.debut);
            cas(n + " reconnu (centre, entrée, sortie)", g.ok);
            if (!g.ok) continue;
            // dimensions réelles (CEREMA / SETRA) : rayon extérieur 15 à 25 m, anneau de 7 à 9 m, îlot infranchissable entouré d'une couronne
            cas(n + " : rayon extérieur 15-25 m, anneau 7-9 m, îlot < couronne", giratoire::R_EXT >= 15 && giratoire::R_EXT <= 25 &&
                giratoire::R_EXT - giratoire::R_COURONNE >= 7 && giratoire::R_EXT - giratoire::R_COURONNE <= 9 && giratoire::R_ILOT < giratoire::R_COURONNE);
            // la voiture (axe du tracé, ± 1 m de demi-largeur) reste dans l'anneau tout le long du ¾ de tour, avec 2 m de marge
            float dmin = 1e9f, dmax = 0;
            for (int64_t a = g.entree; a < g.sortie; a++) {
                const TrackPoint p = t.point((double)a * SEG, 0);
                const float d = giratoire::distanceCentre(g, {(float)p.x, 0, (float)p.z});
                dmin = std::min(dmin, d); dmax = std::max(dmax, d);
            }
            std::printf("chaussée : %s : axe de l'anneau à %.2f-%.2f m du centre (anneau %.1f-%.1f m), branches à %.0f° et %.0f°\n", n.c_str(), dmin, dmax,
                        giratoire::R_COURONNE, giratoire::R_EXT, g.angEntree * 57.2958f, g.angSortie * 57.2958f);
            cas(n + " : la voiture reste dans l'anneau (≥ 2 m de chaque bord)", dmin - 1 >= giratoire::R_COURONNE + 2 && dmax + 1 <= giratoire::R_EXT - 2);
            // géométrie : anneau continu sur 360° (aucun trou), rien d'asphalte sur l'îlot, cédez-le-passage et îlot séparateur présents
            std::vector<giratoire::Tri> tris; giratoire::geometrie(g, t, tris);
            int secteurs[36] = {}, asphalteIlot = 0, marquages = 0, ilotSep = 0;
            for (const giratoire::Tri& tr : tris) {
                const vec3 m = (tr.a + tr.b + tr.c) * (1.f / 3);
                const float d = giratoire::distanceCentre(g, m);
                if (tr.s == giratoire::ASPHALTE && d < giratoire::R_COURONNE - 0.01f) asphalteIlot++;
                if (tr.s == giratoire::ASPHALTE && d >= giratoire::R_COURONNE && d <= giratoire::R_EXT) {
                    const float a = std::atan2(m.z - g.c.z, m.x - g.c.x);
                    secteurs[std::clamp((int)((a + PI) / (2 * PI) * 36), 0, 35)]++;
                }
                if (tr.s == giratoire::MARQUAGE) marquages++;
                if (tr.s == giratoire::PELOUSE && d > giratoire::R_EXT) ilotSep++;
            }
            int vides = 0; for (int k : secteurs) vides += k == 0;
            cas(n + " : anneau d'asphalte continu (36 secteurs de 10° couverts), aucun asphalte sur l'îlot", vides == 0 && asphalteIlot == 0);
            cas(n + " : lignes de rive, cédez-le-passage et îlot séparateur posés", marquages > 100 && ilotSep > 0);
        }
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("chaussée : tout est OK\n");
    return 0;
}
