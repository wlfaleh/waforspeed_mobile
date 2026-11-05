// WaForSpeed — quartiers de la Ville (WFS-93) : voir ville_quartiers.h.
// Les tirages D() sont faits dans l'ordre exact du jeu web (propriétés des objets dans l'ordre de leur écriture,
// tirage du pas de boucle après le corps, même après un continue) : chaque tirage est une instruction séparée.
#include "ville_quartiers.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <mutex>
#include "../core/log.h"
#include "../core/math.h"
#include "../game/track.h"
#include "zones.h"

namespace wfs {
namespace ville {
namespace {

// SHOPS[kind].terrace (js/world/life.js) : la boulangerie (4) et la pharmacie (8) n'ont pas de terrasse
bool aTerrasse(int kind) { return kind != 4 && kind != 8; }

class Generateur {
public:
    Generateur(const MapDef& m, int64_t len, Plan& p) : map_(m), L_(len), P_(p), D_(m.seed * 31 + 7), lampe_((size_t)len, 0) {
        P_.objets.assign((size_t)len, {});
        P_.chocs.assign((size_t)len, {});
    }

    // generateDecor : plages [z0, z1) de même zone, générateur du thème de chaque plage, dans l'ordre de la piste
    void tout() {
        std::vector<std::pair<int64_t, int64_t>> plages;
        for (int64_t a = 0; a < L_; a++) if (a == 0 || zoneIndex(map_, a, L_) != zoneIndex(map_, a - 1, L_)) plages.push_back({a, L_});
        for (size_t i = 0; i + 1 < plages.size(); i++) plages[i].second = plages[i + 1].first;
        for (auto [z0, z1] : plages) {
            const char* z = zoneId(map_, z0, L_);
            if (!std::strcmp(z, "centre")) centre(z0, z1);
            else if (!std::strcmp(z, "residentiel")) residentiel(z0, z1);
            else if (!std::strcmp(z, "parc")) parc(z0, z1);
            else if (!std::strcmp(z, "port")) port(z0, z1);
        }
    }

private:
    double D() { return D_.next(); }
    int64_t fl(double v) { return (int64_t)v; }   // Math.floor(D() · k) : D() ≥ 0
    size_t idx(int64_t n) const { return (size_t)(((n % L_) + L_) % L_); }
    void item(int64_t n, const Objet& o) { P_.objets[idx(n)].push_back(o); }
    void collide(int64_t n, double offM, float hit) { P_.chocs[idx(n)].push_back({(float)(offM / RW), hit}); }
    bool libre(int64_t n) const { return !lampe_[idx(n)]; }

    // lamp(n, side, 'city') : lampadaire, collision au pied du mât (RW + 0,8 m)
    void lamp(int64_t n, int side) {
        lampe_[idx(n)] = (int8_t)side;
        Objet o; o.t = LAMPE; o.side = side; item(n, o);
        collide(n, side * (RW + 0.8), 0.14f);
    }
    // placeShop (life.js) : commerce devant l'immeuble ; pour chaque table (terraceTables) et chaque chaise, D() < 0,65 :
    // client attablé. Centré sur la façade de l'immeuble de longueur lenBld (le commerce reste devant l'immeuble).
    void placeShop(int64_t n, int side, double lenM, float lenBld, int kind) {
        Commerce c{(int64_t)idx(n), side, (float)lenM, kind, 0u, (lenBld - (float)lenM) / 2};
        if (aTerrasse(kind)) {
            int t = 0;
            for (double d = 1.4; d < lenM - 1; d += 2.6, t++)
                for (int sd : {-1, 1}) if (D() < 0.65) c.clients |= 1u << (t * 2 + (sd > 0 ? 1 : 0));
        }
        P_.commerces.push_back(c);
    }
    void arbreRue(int64_t n, int side, double h) {
        Objet o; o.t = ARBRE_RUE; o.side = side; o.lat = (float)(RW + 3.6); o.h = (float)h; item(n, o);
        collide(n, side * (RW + 3.6), 0.08f);
    }

    // city.centre : tours (12-105 m) tous les 11 segments, commerces une fois sur deux, lampadaires tous les 24,
    // passages piétons tous les 150, platanes tous les 16, panneaux néon tous les 130-180
    void centre(int64_t z0, int64_t z1) {
        for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 0 : 5); n < z1 - 11; n += 11) {
            if (D() < 0.08) continue;
            const double tall = D();
            Objet o; o.t = IMMEUBLE; o.side = side;
            o.len = (float)(11 * SEG_M - 0.6 - D() * 2);
            o.d = (float)(10 + D() * 14);
            o.h = (float)(tall < 0.2 ? 60 + D() * 45 : tall < 0.7 ? 26 + D() * 30 : 12 + D() * 14);
            o.k = (int)fl(D() * 4);
            D(); D();                       // u0, v0 (cadrage de la texture de façade du web)
            if (D() < 0.45) D();            // neon : couleur tirée seulement s'il y en a un
            D();                            // sign
            item(n, o);
            if (D() < 0.5) {
                static const int KINDS[7] = {0, 1, 2, 3, 5, 7, 8};
                const int kind = KINDS[fl(D() * 7)];
                placeShop(n, side, 11 * SEG_M - 2.5, o.len, kind);
            }
        }
        for (int64_t n = z0 + 12; n < z1; n += 24) lamp(n, (n / 24) % 2 ? 1 : -1);
        for (int64_t n = z0 + 40; n < z1; n += 150) { Objet o; o.t = ZEBRA; item(n, o); }
        for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 4 : 10); n < z1; n += 16)
            if (libre(n) && libre(n + 1) && D() > 0.2) arbreRue(n, side, 5 + D() * 2.5);
        for (int64_t n = z0 + 60; n < z1 - 40; n += 130 + fl(D() * 50)) {
            const int side = D() < 0.5 ? -1 : 1;
            if (!libre(n)) continue;
            const int texte = (int)fl(D() * 5);   // BOARD_TEXTS
            P_.panneaux.push_back({(int64_t)idx(n), side, texte});
            collide(n, side * (RW + 1), 0.15f);
        }
    }

    // city.residentiel : immeubles bas (11-23 m, teintes 2-3, balcons) tous les 9 segments, commerces une fois sur
    // 0,4, lampadaires tous les 26, arbres tous les 10, abribus tous les 160, passages piétons tous les 170
    void residentiel(int64_t z0, int64_t z1) {
        for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 2 : 7); n < z1 - 9; n += 9) {
            if (D() < 0.08) continue;
            Objet o; o.t = IMMEUBLE; o.side = side; o.balcons = true;
            o.len = (float)(9 * SEG_M - 0.4 - D() * 1.5);
            o.d = (float)(9 + D() * 6);
            o.h = (float)(11 + D() * 12);
            o.k = 2 + (int)fl(D() * 2);
            D(); D();                       // u0, v0
            item(n, o);
            if (D() < 0.4) {
                static const int KINDS[7] = {0, 1, 3, 4, 5, 6, 8};
                const int kind = KINDS[fl(D() * 7)];
                placeShop(n, side, 9 * SEG_M - 2, o.len, kind);
            }
        }
        for (int64_t n = z0 + 12; n < z1; n += 26) lamp(n, (n / 26) % 2 ? 1 : -1);
        for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 3 : 8); n < z1; n += 10)
            if (libre(n) && libre(n + 1) && D() > 0.1) arbreRue(n, side, 6 + D() * 3);
        // voitures garées : retirées du jeu web (parked = no-op), aucun tirage
        for (int64_t n = z0 + 80; n < z1 - 20; n += 160) if (libre(n)) {
            Objet o; o.t = ABRIBUS; o.side = D() < 0.5 ? -1 : 1; item(n, o);
        }
        for (int64_t n = z0 + 50; n < z1; n += 170) { Objet o; o.t = ZEBRA; item(n, o); }
    }

    // city.parc : un côté tiré (parkSide) : grille, arbres ronds / pins, bancs, fontaines ; l'autre : immeubles moyens
    void parc(int64_t z0, int64_t z1) {
        const int ps = D() < 0.5 ? -1 : 1;
        for (int64_t n = z0; n < z1; n += 4) { Objet o; o.t = GRILLE; o.side = ps; o.lat = (float)(RW + 6); item(n, o); }
        for (int64_t n = z0 + 2; n < z1; n += 3 + fl(D() * 3)) {
            Objet o; o.t = ARBRE; o.side = ps;
            o.lat = (float)(RW + 9 + D() * 45);
            o.h = (float)(7 + D() * 8);
            o.k = D() < 0.25 ? 1 : 0;       // 1 : pin, 0 : feuillu rond (tilleul)
            o.along = (float)((D() - 0.5) * 3);
            item(n, o);
        }
        for (int64_t n = z0 + 8; n < z1; n += 22) { Objet o; o.t = BANC; o.side = ps; o.lat = (float)(RW + 4.2); item(n, o); }
        for (int64_t n = z0 + 120; n < z1 - 60; n += 200) { Objet o; o.t = FONTAINE; o.side = ps; o.lat = (float)(RW + 24); item(n, o); }
        for (int64_t n = z0 + (ps < 0 ? 5 : 0); n < z1 - 11; n += 11) if (D() > 0.1) {
            Objet o; o.t = IMMEUBLE; o.side = -ps;
            o.len = (float)(11 * SEG_M - 0.6 - D() * 2);
            o.d = (float)(10 + D() * 10);
            o.h = (float)(14 + D() * 22);
            o.k = (int)fl(D() * 4);
            D(); D();                       // u0, v0
            item(n, o);
        }
        for (int64_t n = z0 + 14; n < z1; n += 28) lamp(n, ps);
    }

    // city.port : entrepôts, conteneurs empilés, grillage, grues, lampadaires tous les 30
    void port(int64_t z0, int64_t z1) {
        for (int side : {-1, 1}) {
            for (int64_t n = z0 + (side < 0 ? 0 : 10); n < z1 - 22; n += 22 + fl(D() * 10)) if (D() < 0.7) {
                Objet o; o.t = ENTREPOT; o.side = side;
                o.lat = (float)(RW + 10 + D() * 6);
                o.len = (float)(18 + D() * 8);
                o.d = (float)(16 + D() * 10);
                o.h = (float)(8 + D() * 5);
                o.k = (int)fl(D() * 4);
                item(n, o);
            }
            for (int64_t n = z0 + 5; n < z1 - 10; n += 9 + fl(D() * 6)) if (D() < 0.6) {
                Objet o; o.t = CONTENEURS; o.side = side;
                o.lat = (float)(RW + 34 + D() * 20);
                o.k = 2 + (int)fl(D() * 3);
                o.graine = (uint32_t)fl(D() * 1000);
                item(n, o);
            }
            for (int64_t n = z0 + 30; n < z1; n += 3) { Objet o; o.t = GRILLAGE; o.side = side; o.lat = (float)(RW + 5); item(n, o); }
        }
        for (int64_t n = z0 + 60; n < z1; n += 110 + fl(D() * 40)) {
            Objet o; o.t = GRUE;
            o.side = D() < 0.5 ? -1 : 1;
            o.lat = (float)(RW + 60 + D() * 30);
            o.h = (float)(34 + D() * 12);
            item(n, o);
        }
        for (int64_t n = z0 + 15; n < z1; n += 30) lamp(n, (n / 30) % 2 ? 1 : -1);
    }

    const MapDef& map_;
    const int64_t L_;
    Plan& P_;
    Mulberry32 D_;
    std::vector<int8_t> lampe_;   // deco.lamp : côté du lampadaire du segment (0 : aucun)
};

} // namespace

const Plan& plan(const MapDef& map, int64_t segCount) {
    static std::mutex m;
    static std::map<std::pair<uint32_t, int64_t>, Plan> cache;
    static const Plan vide;
    if (std::strcmp(map.id, "ville") || segCount <= 0) return vide;
    std::lock_guard<std::mutex> verrou(m);
    auto it = cache.find({map.seed, segCount});
    if (it != cache.end()) return it->second;
    Plan& p = cache[{map.seed, segCount}];
    Generateur(map, segCount, p).tout();
    size_t nb = 0; for (auto& v : p.objets) nb += v.size();
    WFS_LOG("Ville : plan des quartiers, %zu objets, %zu commerces, %zu panneaux", nb, p.commerces.size(), p.panneaux.size());
    return p;
}

int bandes(const char* zone, Bande out[2]) {
    const float R = (float)RW;
    // GROUND.city : pelouse de RW + 6 à RW + 80 (0,15 → 0,4) ; béton de RW + 6 à RW + 70 (0,15 → −0,3)
    if (!std::strcmp(zone, "parc")) { out[0] = {R + 6, R + 80, 0.2f, 0.45f}; out[1] = {R + 80, R + 120, 0.45f, -1.79f}; return 2; }
    if (!std::strcmp(zone, "port")) { out[0] = {R + 6, R + 70, 0.2f, -0.25f}; out[1] = {R + 70, R + 100, -0.25f, -1.21f}; return 2; }
    return 0;
}

float solY(const char* zone, float lat) {
    const float R = (float)RW;
    if (lat <= R + 6) return 0.15f;
    Bande b[2];
    const int n = bandes(zone, b);
    if (!n) return 0.15f - 0.45f * std::min(1.f, (lat - (R + 6)) / 64.f);
    for (int i = 0; i < n; i++) if (lat <= b[i].b) return b[i].ya + (b[i].yb - b[i].ya) * (lat - b[i].a) / (b[i].b - b[i].a);
    return b[n - 1].yb;
}

} // namespace ville
} // namespace wfs
