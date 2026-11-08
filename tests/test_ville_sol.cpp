// WaForSpeed — tests de WFS-305 (ANO-171) : sol fermé côté intérieur des courbes serrées de la Ville.
// Avant : la pelouse du parc (et le béton du port) s'arrêtait à Plan::lim (90 % du plus petit rayon des 60 m qui suivent) alors
// que le sol de base continuait au rayon local : dans les 60 m avant le virage, on voyait ce sol, plus bas et sans herbe (fosse).
//  - RG-04 : sur une grille de 1 m autour de chaque courbe serrée de la Ville (rayon < 60 m), entre le trottoir et 60 m au-delà,
//    chaque point de la zone d'une bande (parc, port) est couvert par une bande ou un raccord : 0 trou après (« avant » au rapport) ;
//  - RG-02 : marche entre deux surfaces voisines ≤ 5 cm (points à 0,25 m l'un de l'autre, pente des bandes comprise) ;
//  - RG-05 : aucun raccord sur une chaussée ni sur un trottoir (RW + 6 m de tout point du tracé), ni au-delà de la bande.
// usage : test_ville_sol
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
#include "../src/world/ville_quartiers.h"
#include "../src/world/zones.h"

using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

namespace {
struct Tri { float ax, az, bx, bz, cx, cz, ya, yb, yc, minx, maxx, minz, maxz; int raccord; };   // 0 bande, 1 carreau, 2 carreau de bord
bool dans(const Tri& t, float x, float z, float& y) {
    if (x < t.minx || x > t.maxx || z < t.minz || z > t.maxz) return false;
    const float d = (t.bz - t.cz) * (t.ax - t.cx) + (t.cx - t.bx) * (t.az - t.cz);
    if (std::abs(d) < 1e-9f) return false;
    const float l1 = ((t.bz - t.cz) * (x - t.cx) + (t.cx - t.bx) * (z - t.cz)) / d, l2 = ((t.cz - t.az) * (x - t.cx) + (t.ax - t.cx) * (z - t.cz)) / d;
    const float l3 = 1 - l1 - l2;
    if (l1 < -1e-5f || l2 < -1e-5f || l3 < -1e-5f) return false;
    y = l1 * t.ya + l2 * t.yb + l3 * t.yc;
    return true;
}
struct P3 { float x, y, z; };
// surfaces de sol des bandes du parc et du port, dessinées comme ville.cpp (carte::poseAt, carte::lateral, quadSol)
struct Sol {
    std::vector<Tri> tris;
    void quad(P3 a, P3 b, P3 c, P3 d, int r = 0) { ajoute(a, b, c, r); ajoute(a, c, d, r); }
    void ajoute(P3 a, P3 b, P3 c, int r) {
        tris.push_back({a.x, a.z, b.x, b.z, c.x, c.z, a.y, b.y, c.y, std::min({a.x, b.x, c.x}), std::max({a.x, b.x, c.x}), std::min({a.z, b.z, c.z}), std::max({a.z, b.z, c.z}), r});
    }
    bool haut(float x, float z, float& y, int* raccord = nullptr) const {   // surface la plus haute au-dessus de (x, z)
        bool ok = false; float yy;
        for (const Tri& t : tris) if (dans(t, x, z, yy) && (!ok || yy > y)) { y = yy; ok = true; if (raccord) *raccord = t.raccord; }
        return ok;
    }
    Sol autour(float x, float z, float r) const {   // triangles qui touchent le carré de côté 2r centré en (x, z)
        Sol o;
        for (const Tri& t : tris) if (t.maxx >= x - r && t.minx <= x + r && t.maxz >= z - r && t.minz <= z + r) o.tris.push_back(t);
        return o;
    }
};
} // namespace

int main() {
    static Track t; t.build(mapById("ville"), trace::versionCarte("ville"));
    const MapDef& map = *t.map();
    const ville::Plan& P = ville::adapter(map, t);
    const auto& rac = ville::carreauxSol(map, t);
    const int64_t L = (int64_t)t.segmentCount();
    const float R = (float)RW;
    VERIFIE((int64_t)rac.size() == L, "raccords : %zu segments au lieu de %lld", rac.size(), (long long)L);
    auto pose = [&](int64_t a, float lat, float dy) {   // comme carte::lateral(carte::poseAt(t, a), lat, dy)
        const TrackPoint p = t.point((double)a * SEG, 0);
        return P3{(float)p.x + std::cos((float)p.th) * lat, (float)p.y + dy, (float)p.z + std::sin((float)p.th) * lat};
    };
    Sol avant, apres, domaineTout;   // domaineTout : bandes sans la limite des courbes serrées (là où le sol de la bande devrait être)
    size_t nRac = 0, hors = 0;
    for (int64_t a = 0; a < L; a++) {
        ville::Bande bd[2];
        const int nb = ville::bandes(zoneId(map, a, L), bd);
        if (!nb) continue;
        const uint8_t vo = P.voie[(size_t)a];
        for (int s : {-1, 1}) for (int i = 0; i < nb; i++) {
            if (vo != ville::RUE && (s < 0 || vo != ville::AUTOROUTE)) continue;
            const ville::Bande& b = bd[i];
            const float l0 = P.lim(a, s), l1 = P.lim(a + 1, s);
            if (b.a >= std::min(l0, l1) || b.a >= b.b) continue;   // (sans le trottoir prolongé des façades : aucune au parc ni au port)
            // avant : bord coupé à la hauteur de la fin de la bande ; après : à la hauteur de la bande à cet endroit (WFS-305)
            const float e0 = std::min(b.b, l0), e1 = std::min(b.b, l1);
            auto yb = [&](float e) { return b.ya + (b.yb - b.ya) * (e - b.a) / (b.b - b.a); };
            avant.quad(pose(a, s * b.a, b.ya), pose(a + 1, s * b.a, b.ya), pose(a + 1, s * e1, b.yb), pose(a, s * e0, b.yb));
            apres.quad(pose(a, s * b.a, b.ya), pose(a + 1, s * b.a, b.ya), pose(a + 1, s * e1, yb(e1)), pose(a, s * e0, yb(e0)));
        }
        for (int s : {-1, 1}) for (int i = 0; i < nb; i++) {
            if (vo != ville::RUE && (s < 0 || vo != ville::AUTOROUTE)) continue;
            const ville::Bande& b = bd[i];
            // au plus jusqu'au centre des courbes serrées (au-delà, les lignes des segments se croisent : quadrilatère replié)
            auto centre = [&](int64_t k) { const trace::Rayon ri = trace::rayonInterieur(t, (double)k * SEG);
                return ri.m < 60 ? (float)(s > 0 ? ri.droite : ri.gauche) : std::numeric_limits<float>::infinity(); };
            const float f = std::min(b.b, 0.999f * std::min(centre(a), centre(a + 1)));
            if (f > b.a) domaineTout.quad(pose(a, s * b.a, 0), pose(a + 1, s * b.a, 0), pose(a + 1, s * f, 0), pose(a, s * f, 0));
        }
        for (const ville::CarreauSol& q : rac[(size_t)a]) {
            nRac++;
            if (q.bande >= nb) hors++;
            const float h = ville::CARREAU_SOL / 2;
            apres.quad({q.x - h, q.y, q.z - h}, {q.x - h, q.y, q.z + h}, {q.x + h, q.y, q.z + h}, {q.x + h, q.y, q.z - h}, q.bord ? 2 : 1);
        }
    }
    VERIFIE(hors == 0, "RG-05 : %zu carreau(x) d'une bande absente", hors);
    // axe du tracé (chaussée et trottoir : RW + 6 m)
    std::vector<P3> axe((size_t)L);
    for (int64_t a = 0; a < L; a++) axe[(size_t)a] = pose(a, 0, 0);
    std::vector<int64_t> tous((size_t)L);
    for (int64_t a = 0; a < L; a++) tous[(size_t)a] = a;
    auto plusProcheDe = [&](const std::vector<int64_t>& liste, float x, float z, int64_t& k) {
        float d = std::numeric_limits<float>::infinity();
        for (int64_t a : liste) { const float e = std::hypot(axe[(size_t)a].x - x, axe[(size_t)a].z - z); if (e < d) { d = e; k = a; } }
        return d;
    };
    auto plusProche = [&](float x, float z, int64_t& k) { return plusProcheDe(tous, x, z, k); };
    // RG-05 : aucun carreau sur une chaussée ni sur un trottoir (ses 4 coins à plus de RW + 6 m de tout point du tracé)
    int surRoute = 0;
    for (int64_t a = 0; a < L; a++) for (const ville::CarreauSol& q : rac[(size_t)a]) {
        const float h = ville::CARREAU_SOL / 2;
        for (const auto& [dx, dz] : {std::pair{-h, -h}, std::pair{-h, h}, std::pair{h, h}, std::pair{h, -h}}) {
            int64_t k; if (plusProche(q.x + dx, q.z + dz, k) < R + 6 - 0.01f) { surRoute++; break; }
        }
    }
    VERIFIE(surRoute == 0, "RG-05 : %d carreau(x) sur une chaussée ou un trottoir", surRoute);
    std::printf("carreaux de sol : %zu (%.0f m²)\n", nRac, nRac * ville::CARREAU_SOL * ville::CARREAU_SOL);


    // RG-04 / RG-02 : chaque courbe serrée de la Ville (rayon < 60 m)
    float pireFente = 0;
    int courbes = 0, trousAvant = 0, trousApres = 0, marches = 0, marchesAvant = 0, marchesInternes = 0, marchesFente = 0; float pireMarche = 0;
    for (int64_t a = 0; a < L;) {
        const trace::Rayon ri = trace::rayonInterieur(t, ((double)a + 0.5) * SEG);
        if (ri.m >= 60) { a++; continue; }
        int64_t b = a;
        while (b < L && trace::rayonInterieur(t, ((double)b + 0.5) * SEG).m < 60) b++;
        const int64_t m = (a + b) / 2;
        courbes++;
        int tAv = 0, tAp = 0, pts = 0;
        const P3 c = axe[(size_t)m];
        std::vector<int64_t> pres;   // points du tracé à moins de 300 m (au-delà, jamais le plus proche d'un point de la grille)
        for (int64_t k = 0; k < L; k++) if (std::hypot(axe[(size_t)k].x - c.x, axe[(size_t)k].z - c.z) < 300) pres.push_back(k);
        const Sol av = avant.autour(c.x, c.z, 92), ap = apres.autour(c.x, c.z, 92), dom = domaineTout.autour(c.x, c.z, 92);
        for (int i = -90; i <= 90; i++) for (int j = -90; j <= 90; j++) {
            const float x = c.x + (float)i, z = c.z + (float)j;
            int64_t k = 0;
            const float d = plusProcheDe(pres, x, z, k);
            if (d <= R + 6.5f || d > R + 6 + 60) continue;   // entre le trottoir et 60 m au-delà
            // domaine d'une bande : la bande d'un segment (parc, port), sans la limite des courbes, passe sous ce point
            float yd; const bool domaine = dom.haut(x, z, yd);
            if (!domaine) continue;   // centre, résidentiel (sol de Game), autoroute, tunnel, pont : hors sujet
            pts++;
            float y;
            if (!av.haut(x, z, y)) tAv++;
            if (!ap.haut(x, z, y)) { tAp++; if (std::getenv("WFS_DETAIL") && tAp % 1 == 0) { const P3 o = axe[(size_t)k]; const TrackPoint q = t.point((double)k * SEG, 0);
                const float lat = (x - o.x) * std::cos((float)q.th) + (z - o.z) * std::sin((float)q.th);
                std::printf("    trou : segment %lld (%s), latéral %+.1f m, lim %.1f / %.1f\n", (long long)k, zoneId(map, k, L), lat, P.lim(k, lat > 0 ? 1 : -1), d); } continue; }
            // RG-02 : marche ≤ 5 cm entre deux points voisins (0,25 m). Une marche compte comme créée par les carreaux si l'un des deux
            // points n'avait pas de sol avant (fosse) ou si sa surface du dessus a changé ; sinon elle existait déjà (bord d'une bande
            // contre la bande d'une autre partie du tracé, posée à une autre hauteur) : comptée à part, pour le rapport
            float yAv; const bool okAv = av.haut(x, z, yAv);
            int rc1 = 0; ap.haut(x, z, y, &rc1);
            for (const auto& [dx, dz] : {std::pair{0.25f, 0.f}, std::pair{0.f, 0.25f}}) {
                float y2, y2Av; int rc2 = 0;
                if (!ap.haut(x + dx, z + dz, y2, &rc2)) continue;
                const float e = std::abs(y2 - y);
                if (e <= 0.05f) continue;
                const bool ok2Av = av.haut(x + dx, z + dz, y2Av);
                if (okAv && ok2Av && std::abs(yAv - y) < 1e-4f && std::abs(y2Av - y2) < 1e-4f) { marchesAvant++; continue; }
                // aucun carreau libre des deux côtés : bande (ou carreau de bord, à sa hauteur) contre bande, là où l'ancienne fosse
                // n'était qu'une fente : c'est la marche entre deux bandes existantes, comptée à part
                if (rc1 != 1 && rc2 != 1) { marchesFente++; pireFente = std::max(pireFente, e); continue; }
                if (rc1 == 1 && rc2 == 1) marchesInternes++;
                pireMarche = std::max(pireMarche, e);
                marches++;
                if (std::getenv("WFS_DETAIL") && marches % 10 == 1) {
                    const P3 o = axe[(size_t)k]; const TrackPoint q = t.point((double)k * SEG, 0);
                    const float lat = (x - o.x) * std::cos((float)q.th) + (z - o.z) * std::sin((float)q.th);
                    std::printf("    marche %.1f cm : segment %lld, latéral %+.1f m (y %.2f → %.2f ; avant %s / %s)\n", e * 100, (long long)k, lat, y, y2,
                                okAv ? "sol" : "fosse", ok2Av ? "sol" : "fosse");
                }
            }
        }
        if (std::getenv("WFS_CARTE") && courbes == std::atoi(std::getenv("WFS_CARTE"))) {   // carte en caractères (mise au point)
            for (int j = 40; j >= -40; j--) { for (int i = -40; i <= 40; i++) {
                const float x = c.x + (float)i, z = c.z + (float)j; int64_t k = 0; const float d = plusProcheDe(pres, x, z, k);
                float y; int rc = 0; const bool cv = ap.haut(x, z, y, &rc);
                std::putchar(d <= R + 6 ? '#' : cv ? (rc ? 'c' : 'b') : '.'); } std::putchar('\n'); }
        }
        std::printf("- courbe %d : %.0f à %.0f m, rayon %.0f m (%s), zone %s : %d point(s) de 1 m, trou(s) avant %d, après %d\n", courbes, a * SEG_M,
                    b * SEG_M, ri.m, ri.droite < ri.gauche ? "à droite" : "à gauche", zoneId(map, m, L), pts, tAv, tAp);
        trousAvant += tAv; trousApres += tAp;
        a = b;
    }
    std::printf("courbes serrées : %d ; trous : avant %d, après %d ; marche la plus haute %.1f cm (%d au-delà de 5 cm créées par les carreaux ; %d marches d'avant entre deux bandes, inchangées)\n",
                courbes, trousAvant, trousApres, pireMarche * 100, marches, marchesAvant);
    std::printf("dont entre deux carreaux : %d ; marches entre deux bandes existantes, là où la fosse n'était qu'une fente (gardées) : %d (au plus %.1f cm)\n",
                marchesInternes, marchesFente, pireFente * 100);
    VERIFIE(courbes >= 3, "courbes serrées de la Ville introuvables (%d)", courbes);
    VERIFIE(trousAvant > 0, "le défaut d'avant n'est plus mesuré (0 trou sans raccord) : test sans effet");
    VERIFIE(trousApres == 0, "RG-04 : %d trou(s) de 1 m dans le sol, côté intérieur des courbes serrées", trousApres);
    VERIFIE(marches == 0, "RG-02 : %d marche(s) de plus de 5 cm créées par les carreaux (au plus %.1f cm)", marches, pireMarche * 100);
    std::printf("sol de la Ville dans les courbes serrées (WFS-305) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
