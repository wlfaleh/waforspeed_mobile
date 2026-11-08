// WaForSpeed — tests de WFS-137 (Ville au tracé 2) : lieux du plan (carrefours à feux, rond-point), rue transversale
// dégagée (RG-C1), piétons des passages zébrés qui traversent au rouge des voitures et attendent au vert (RG-02).
#include <cmath>
#include <cstdio>
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
#include "../src/world/ville_quartiers.h"

using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// horloge du trafic à laquelle le carrefour i est dans l'état f (balayage du cycle au dixième de seconde)
static double horlogeEn(int i, trace::Feu f) {
    for (double h = 0; h < trace::FEU_CYCLE * 4; h += 0.1) if (trace::feuCarrefour(i, h) == f) return h + 0.05;
    return -1;
}

int main() {
    Track t; t.build(mapById("ville"), 2);
    const ville::Plan& P = ville::adapter(mapById("ville"), t);
    int feux = 0, ronds = 0;
    for (const auto& s : trace::sections(t)) { feux += s.type == trace::Type::Feux && !s.depart; ronds += s.type == trace::Type::RondPoint; }
    VERIFIE((int)P.carrefours.size() == feux && feux > 0, "carrefours à feux : %zu au plan, %d sections", P.carrefours.size(), feux);
    // WFS-283 : plus de rond-point en jeu (carrefour simple) ; le plan de la Ville le lit encore quand l'interrupteur le remet (RG-04)
    VERIFIE((int)P.ronds.size() == ronds && ronds == (trace::rondsPoints() ? 1 : 0), "ronds-points : %zu au plan, %d sections", P.ronds.size(), ronds);
    {
        trace::forcerRondsPoints(true);
        static Track t2; t2.build(mapById("ville"), 2);
        const ville::Plan& P2 = ville::adapter(mapById("ville"), t2);
        int r2 = 0; for (const auto& s : trace::sections(t2)) r2 += s.type == trace::Type::RondPoint;
        VERIFIE((int)P2.ronds.size() == r2 && r2 == 1, "ronds-points remis (RG-04) : %zu au plan, %d sections", P2.ronds.size(), r2);
        for (const auto& r : P2.ronds) VERIFIE(std::fabs(r.rayon - 14) < 1.5f, "rayon de l'anneau %.1f m (14 m attendus)", r.rayon);
        trace::forcerRondsPoints(false);
    }
    // RG-C1 : aucun objet (sauf lampadaire) sur la rue transversale ni à 1,5 m de ses bords
    const int64_t L = (int64_t)P.objets.size();
    for (const auto& c : P.carrefours)
        for (int64_t d = -(int64_t)std::ceil((ville::RUE_TRANS_M + 1.5) / SEG_M); d <= (int64_t)std::ceil((ville::RUE_TRANS_M + 1.5) / SEG_M); d++)
            for (const ville::Objet& o : P.objets[(size_t)(((c.croise + d) % L + L) % L)])
                VERIFIE(o.t == ville::LAMPE, "objet %d sur la rue transversale du carrefour %d (segment %lld)", (int)o.t, c.feu, (long long)(c.croise + d));
    // RG-02 : 4 piétons par carrefour, sur le trottoir au départ
    const auto ps = ville::passants(mapById("ville"), t);
    VERIFIE(ps.size() == 4 * P.carrefours.size(), "piétons des passages : %zu", ps.size());
    for (const auto& p : ps) VERIFIE(std::fabs(p.lat) > RW, "piéton du passage sur la chaussée au départ (%.1f m)", p.lat);
    // au vert : on attend ; au rouge : on traverse jusqu'à l'autre trottoir, puis on attend le prochain rouge
    const ville::Passant& p0 = ps.front();
    double lat = p0.lat, y = 0; int dir = p0.dir; bool marche = false; float yaw = 0;
    trace::poserCarte(&t, horlogeEn(p0.feu, trace::Feu::Vert));
    for (int k = 0; k < 50; k++) ville::traverser(lat, dir, marche, yaw, y, p0.feu, p0.v, 0.1);
    VERIFIE(lat == p0.lat && !marche, "au vert des voitures, le piéton attend (%.2f m)", lat);
    trace::poserCarte(&t, horlogeEn(p0.feu, trace::Feu::Rouge));
    ville::traverser(lat, dir, marche, yaw, y, p0.feu, p0.v, 0.1);
    VERIFIE(marche && std::fabs(y - 0.15) < 1e-9, "au rouge, le piéton s'engage");
    for (int k = 0; k < 400 && marche; k++) ville::traverser(lat, dir, marche, yaw, y, p0.feu, p0.v, 0.1);
    VERIFIE(!marche && std::fabs(lat - p0.dir * (RW + 1.2)) < 1e-6 && dir == -p0.dir,
            "le piéton arrive sur l'autre trottoir (%.2f m, départ %.2f m)", lat, p0.lat);
    if (echecs) { std::printf("test_ville_lieux : %d échec(s)\n", echecs); return 1; }
    std::printf("test_ville_lieux : OK\n");
    return 0;
}
