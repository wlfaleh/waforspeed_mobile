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
#include "../game/trace_cartes.h"   // WFS-137 : sections du tracé 2
#include "zones.h"   // WFS-176 : quartiers des lieux
#include <limits>
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
    // WFS-283 : la dernière zone du tour se replie sur le départ ; jamais de passage piéton sous la grille (45 m derrière la
    // ligne, 5 m devant) : sans le rond-point, le tour plus court en posait un à 7 m derrière la ligne
    bool grilleDepart(int64_t n) const { const int64_t a = (int64_t)idx(n); return a < (int64_t)(5 / SEG_M) || a > L_ - (int64_t)(45 / SEG_M); }
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
        for (int64_t n = z0 + 40; n < z1; n += 150) if (!grilleDepart(n)) { Objet o; o.t = ZEBRA; item(n, o); }
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
        for (int64_t n = z0 + 50; n < z1; n += 170) if (!grilleDepart(n)) { Objet o; o.t = ZEBRA; item(n, o); }
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

namespace {
std::mutex g_verrou;
std::map<std::pair<uint32_t, int64_t>, Plan> g_cache;
}

const Plan& plan(const MapDef& map, int64_t segCount) {
    std::mutex& m = g_verrou;
    std::map<std::pair<uint32_t, int64_t>, Plan>& cache = g_cache;
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

// ---------- WFS-137 : plan adapté au tracé 2 (RG-C1, RG-03) ----------
float Plan::lim(int64_t a, int side) const {
    const auto& v = side > 0 ? limD : limG;
    if (v.empty()) return std::numeric_limits<float>::infinity();
    const int64_t L = (int64_t)v.size();
    return v[(size_t)(((a % L) + L) % L)];
}

float etendue(const Objet& o) {
    const float R = (float)RW;
    switch (o.t) {
    case IMMEUBLE: return R + 6.5f + o.d;            // retrait des façades (SET_BACK de ville.cpp) + profondeur
    case ENTREPOT: return o.lat + o.d;
    case LAMPE: return R + 0.8f;                      // mât au bord du trottoir (le bras passe au-dessus de la chaussée)
    case ZEBRA: return 0;                             // marquage sur la chaussée
    case ARBRE_RUE: return o.lat + 3.f;               // houppier des platanes
    case ARBRE: return o.lat + 4.f;
    case CONTENEURS: return o.lat + o.k * 2.6f + 2.5f;
    case GRUE: return o.lat + 8.f;                    // pieds et tête (la flèche passe au-dessus du quai)
    case FONTAINE: return o.lat + 4.f;
    default: return o.lat + 1.f;                      // abribus, bancs, grilles, grillages
    }
}

// WFS-176 (RG-04, RG-05 de WFS-137) : casino (Centre-ville), centre commercial (Quartier d'affaires), 2 garages (Quartier
// résidentiel, Zone portuaire) sur une ligne droite de rue, loin des carrefours et des courbes serrées (RG-C1) ; emprise vidée ;
// cafés et bars de plus près des carrefours ; promeneurs avec chien. Profondeurs : casino jusqu'à RW + 38, centre commercial
// (parking devant) jusqu'à RW + 58, garage jusqu'à RW + 19.
template <class FL, class FV, class FR>
void lieuxVille(const MapDef& map, Plan& P, int64_t L, FL limite, FV voieSur, FR surRue) {
    const float R = (float)RW;
    struct Voulu { TypeLieu t; const char* zone; float len, prof; };
    const Voulu V[] = {{CASINO, "Centre-ville", 40, R + 38}, {CENTRE_COMMERCIAL, "Quartier d'affaires", 80, R + 58},
                       {GARAGE, "Quartier résidentiel", 14, R + 19}, {GARAGE, "Zone portuaire", 14, R + 19}};
    auto libre = [&](int64_t a, int side, float len, float prof) {
        if (voieSur(a, len + 40) != RUE || surRue(a - (int64_t)(40 / SEG_M), len + 80) || limite(a - (int64_t)(60 / SEG_M), side, len + 60) < prof) return false;
        for (const Lieu& l : P.lieux) { int64_t d = ((a - l.seg) % L + L) % L; if (d > L / 2) d = L - d; if (d * SEG_M < 120) return false; }
        for (const RondPoint& rp : P.ronds) { int64_t d = ((a - rp.centre) % L + L) % L; if (d > L / 2) d = L - d; if (d * SEG_M < 150) return false; }
        return true;
    };
    for (const Voulu& v : V) {
        for (int64_t a = 0; a < L; a++) {
            if (std::strcmp(zoneName(map, a, L), v.zone) || std::strcmp(zoneName(map, a + (int64_t)(v.len / SEG_M), L), v.zone)) continue;
            int side = 0;
            for (int s : {1, -1}) if (!side && libre(a + 20, s, v.len, v.prof)) side = s;
            if (!side) continue;
            const int64_t a0 = a + 20, n = (int64_t)std::ceil(v.len / SEG_M);
            P.lieux.push_back({v.t, a0 % L, side, v.len});
            // emprise : plus d'immeuble ni d'objet de rue derrière le trottoir (lampadaires et passages piétons gardés)
            for (int64_t k = -(int64_t)(30 / SEG_M); k <= n; k++) {
                auto& o = P.objets[(size_t)((((a0 + k) % L) + L) % L)];   // a0 + k peut être négatif (lieu près du départ)
                o.erase(std::remove_if(o.begin(), o.end(), [&](const Objet& x) {
                    if (x.side != side || x.t == LAMPE || x.t == ZEBRA) return false;
                    const float fin = (float)k * (float)SEG_M + (x.t == IMMEUBLE || x.t == ENTREPOT ? x.len : 4.f);
                    return fin > 0 && k <= n;
                }), o.end());
            }
            auto dans = [&](int64_t sg, float len, int sd) { const int64_t d = ((sg - a0) % L + L) % L; return sd == side && (d <= n || d >= L - (int64_t)std::ceil(len / SEG_M)); };
            P.commerces.erase(std::remove_if(P.commerces.begin(), P.commerces.end(), [&](const Commerce& c) { return dans(c.seg, c.len, c.side); }), P.commerces.end());
            P.panneaux.erase(std::remove_if(P.panneaux.begin(), P.panneaux.end(), [&](const Panneau& b) { return dans(b.seg, 6.f, b.side); }), P.panneaux.end());
            break;
        }
    }
    // cafés et bars près des carrefours : devant chaque immeuble à moins de 60 m du passage, sans commerce, une terrasse
    // (café, restaurant, bar, brasserie, glacier, bar à sushis : SHOPS de commerces.cpp, tous avec terrasse)
    static const int TERRASSES[6] = {0, 1, 2, 5, 6, 7};
    Mulberry32 D(map.seed * 31 + 53);
    size_t ajout = 0;
    for (const Carrefour& c : P.carrefours) for (int64_t k = -(int64_t)(60 / SEG_M); k <= (int64_t)(60 / SEG_M); k++) {
        const int64_t a = ((c.croise + k) % L + L) % L;
        for (const Objet& o : P.objets[(size_t)a]) {
            if (o.t != IMMEUBLE || o.len < 8) continue;
            bool deja = false;
            for (const Commerce& m : P.commerces) { int64_t d = ((m.seg - a) % L + L) % L; if (d > L / 2) d = L - d; if (m.side == o.side && d * SEG_M < o.len) deja = true; }
            if (deja || voieSur(a, o.len) != RUE || surRue(a, o.len)) continue;
            const float len = std::min(o.len - 2.f, 12.f);
            P.commerces.push_back({a, o.side, len, TERRASSES[(int)(D.next() * 6) % 6], (uint32_t)(D.next() * 4096) | 0x5, (o.len - len) / 2});
            ajout++;
        }
    }
    // promeneurs avec chien : un toutes les 450 m environ, sur le trottoir (RW + 2,4), dans les rues (pas l'autoroute)
    for (int64_t a = 100; a < L; a += 360 + (int64_t)(D.next() * 40)) {
        if (P.voie[(size_t)a] != RUE) continue;
        const int side = D.next() < 0.5 ? -1 : 1;
        P.promeneurs.push_back({(double)a, side * (R + 2.4), 1.15 + D.next() * 0.25, D.next() < 0.5 ? -1 : 1, (int)P.promeneurs.size() % 2});
    }
    WFS_LOG("Ville (WFS-176) : %zu lieu(x) (casino, centre commercial, garages), %zu terrasses ajoutées près des carrefours, %zu promeneurs avec chien",
            P.lieux.size(), ajout, P.promeneurs.size());
}

const Plan& adapter(const MapDef& map, const Track& t) {
    const int64_t L = (int64_t)t.segmentCount();
    const Plan& p0 = plan(map, L);
    if (p0.objets.empty()) return p0;
    std::lock_guard<std::mutex> verrou(g_verrou);
    Plan& P = g_cache[{map.seed, L}];
    if (P.adapte) return P;
    P.adapte = true;
    const auto& secs = trace::sections(t);
    if (secs.empty()) return P;   // tracé 1 du jeu web : plan inchangé
    constexpr float INF = std::numeric_limits<float>::infinity();
    // rayon intérieur de chaque segment (90 % du rayon des courbes de moins de 60 m, comme les bas-côtés de Game)
    std::vector<float> rg((size_t)L, INF), rd((size_t)L, INF);
    for (int64_t a = 0; a < L; a++) {
        const trace::Rayon ri = trace::rayonInterieur(t, ((double)a + 0.5) * SEG);
        if (ri.gauche < 60) rg[(size_t)a] = (float)(0.9 * ri.gauche);
        if (ri.droite < 60) rd[(size_t)a] = (float)(0.9 * ri.droite);
    }
    // RG-C1 : rien ne cache un virage serré dans les 60 m qui le précèdent : limite = la plus petite des 60 m qui suivent
    const int64_t av = (int64_t)std::ceil(60.0 / SEG_M);
    P.limG.assign((size_t)L, INF); P.limD.assign((size_t)L, INF);
    for (int64_t a = 0; a < L; a++) for (int64_t k = 0; k <= av; k++) {
        const size_t b = (size_t)((a + k) % L);
        P.limG[(size_t)a] = std::min(P.limG[(size_t)a], rg[b]); P.limD[(size_t)a] = std::min(P.limD[(size_t)a], rd[b]);
    }
    // rond-point : l'anneau (14 m) est si serré que les routes d'entrée et de sortie arrivent par son côté extérieur ; le décor
    // posé de ce côté retomberait sur elles. De 60 m avant à 30 m après, décor limité au trottoir des deux côtés (RW + 6) ;
    // l'îlot et les abords sont habillés à part (RG-01)
    for (const trace::Section& s : secs) if (s.type == trace::Type::RondPoint)
        for (int64_t a = s.debut - av; a < s.fin + av / 2; a++) {
            const size_t b = (size_t)(((a % L) + L) % L);
            P.limG[b] = std::min(P.limG[b], (float)RW + 6); P.limD[b] = std::min(P.limD[b], (float)RW + 6);
        }
    P.voie.assign((size_t)L, RUE);
    for (const trace::Section& s : secs) {
        const uint8_t v = s.type == trace::Type::Tunnel ? TUNNEL : s.type == trace::Type::Pont ? PONT : s.autoroute ? AUTOROUTE : RUE;
        if (v != RUE) for (int64_t a = s.debut; a < s.fin; a++) P.voie[(size_t)(((a % L) + L) % L)] = v;
    }
    // RG-01, RG-02 : lieux du tracé 2 ; centre du rond-point = courbure la plus forte de l'anneau (comme la Corniche)
    for (const trace::Section& s : secs) {
        if (s.type == trace::Type::Feux && !s.depart)
            P.carrefours.push_back({s.ligneArret, s.ligneArret + (int64_t)std::lround(trace::CARREFOUR_M / SEG_M), s.feu});
        if (s.type != trace::Type::RondPoint) continue;
        double cmax = 0; int64_t c0 = s.debut;
        for (int64_t a = s.debut; a < s.fin; a++) if (std::abs(t.segAt(a).curve) > cmax) { cmax = std::abs(t.segAt(a).curve); c0 = a; }
        int64_t c1 = c0; while (c1 < s.fin && std::abs(t.segAt(c1).curve) > cmax * 0.99) c1++;
        P.ronds.push_back({(c0 + c1) / 2, s.debut, s.fin, cmax > 0 ? (float)(SEG_M / (cmax * CURVE_K)) : 14.f});
    }
    // rue transversale de chaque carrefour (RG-02) : rien de posé dessus, ni à 1,5 m de ses bords
    const int64_t demiRue = (int64_t)std::ceil((RUE_TRANS_M + 1.5f) / SEG_M);
    auto surRue = [&](int64_t a, float len) {
        for (const Carrefour& c : P.carrefours) {
            int64_t d = ((c.croise - a) % L + L) % L;   // segments de a jusqu'au passage
            if (d <= (int64_t)std::ceil(len / SEG_M) + demiRue || d >= L - demiRue) return true;
        }
        return false;
    };
    // limite d'un côté sur une longueur (m) à partir du segment a : la plus petite des segments couverts
    auto limite = [&](int64_t a, int side, float len) {
        float m = INF;
        for (int64_t k = 0; k <= (int64_t)std::ceil(len / SEG_M); k++) m = std::min(m, P.lim(a + k, side));
        return m;
    };
    auto voieSur = [&](int64_t a, float len) {   // voie la plus « spéciale » sur la longueur
        uint8_t v = RUE;
        for (int64_t k = 0; k <= (int64_t)std::ceil(len / SEG_M); k++) v = std::max(v, P.voie[(size_t)((a + k) % L)]);
        return v;
    };
    size_t retires = 0, avant = 0;
    for (int64_t a = 0; a < L; a++) {
        auto& v = P.objets[(size_t)a];
        avant += v.size();
        const size_t n0 = v.size();
        v.erase(std::remove_if(v.begin(), v.end(), [&](const Objet& o) {
            const float len = o.t == IMMEUBLE || o.t == ENTREPOT ? o.len : o.t == CONTENEURS ? 12.f : 4.f;
            const uint8_t voie = voieSur(a, len);
            if (voie == TUNNEL || voie == PONT || (o.t != LAMPE && surRue(a, len))) return true;   // sous terre, ou au-dessus de l'eau : décor propre (RG-03)
            if (voie == AUTOROUTE && o.t != IMMEUBLE && o.t != ENTREPOT && o.t != CONTENEURS && o.t != GRUE) return true;   // pas de rue
            if (voie == AUTOROUTE && o.side < 0) return true;   // WFS-175 : côté gauche = chaussée opposée, terre-plein et écran antibruit
            return o.t != ZEBRA && etendue(o) > limite(a, o.side, len);
        }), v.end());
        retires += n0 - v.size();
        auto& h = P.chocs[(size_t)a];
        if (P.voie[(size_t)a] != RUE || surRue(a, 0.f)) h.clear();   // RG-02 : plus d'immeuble sur la rue transversale
        else h.erase(std::remove_if(h.begin(), h.end(), [&](const Choc& c) {
            return std::fabs(c.offset) * (float)RW + 1.f > limite(a, c.offset > 0 ? 1 : -1, 4.f); }), h.end());
        {   // WFS-175 : glissière (droite) et séparateur en béton (gauche) de l'autoroute, piédroits du tunnel, parapets du pont
            const float R = (float)RW, d = 0.05f;
            const uint8_t vo = P.voie[(size_t)a];
            if (vo == AUTOROUTE) { h.push_back({(R + 3.6f) / R, d}); h.push_back({-(R + 4.65f) / R, d}); }
            else if (vo == TUNNEL || vo == PONT) { h.push_back({(R + 4.f) / R, d}); h.push_back({-(R + 4.f) / R, d}); }
        }
    }
    const float facade = (float)RW + 6.5f + 4.f;   // commerces et panneaux : devant les façades
    const size_t nc = P.commerces.size(), np = P.panneaux.size();
    // WFS-326 (ANO-175, accroche E2) : un commerce ou un panneau n'est gardé que devant un immeuble encore posé, du même côté et
    // sur toute sa longueur : l'immeuble retiré avant un virage serré laissait sa vitrine allumée, son store et son enseigne dans
    // le vide. Un immeuble qui couvre le segment s commence au plus « le plus long immeuble du plan » (lmax) avant lui.
    float lmax = 0; for (const auto& v : P.objets) for (const Objet& o : v) if (o.t == IMMEUBLE) lmax = std::max(lmax, o.len);
    auto couvert = [&](int64_t s, int side) { for (int64_t k = 0; k * SEG_M <= lmax; k++) for (const Objet& o : P.objets[(size_t)(((s - k) % L + L) % L)])
        if (o.t == IMMEUBLE && o.side == side && k * SEG_M <= o.len) return true; return false; };
    auto bati = [&](int64_t s, int side, float len) { for (int64_t j = 0; j * SEG_M <= len; j++) if (!couvert(s + j, side)) return false; return true; };
    P.commerces.erase(std::remove_if(P.commerces.begin(), P.commerces.end(), [&](const Commerce& c) {   // vitrine : de c.seg + décalage, sur c.len
        return voieSur(c.seg, c.len) != RUE || surRue(c.seg, c.len) || facade > limite(c.seg, c.side, c.len) || !bati(c.seg + (int64_t)std::floor(c.decal / SEG_M), c.side, c.len); }), P.commerces.end());
    P.panneaux.erase(std::remove_if(P.panneaux.begin(), P.panneaux.end(), [&](const Panneau& b) {
        return voieSur(b.seg, 6.f) != RUE || surRue(b.seg, 6.f) || facade > limite(b.seg, b.side, 6.f) || !bati(b.seg, b.side, 6.f); }), P.panneaux.end());
    lieuxVille(map, P, L, limite, voieSur, surRue);   // WFS-176
    WFS_LOG("Ville (WFS-137) : plan adapté au tracé 2 (%zu sections) : %zu / %zu objets, %zu commerces et %zu panneaux retirés "
            "(courbes serrées, autoroute, tunnel, pont, rues des carrefours) ; %zu carrefour(s) à feux, %zu rond(s)-point(s)", secs.size(), retires, avant,
            nc - P.commerces.size(), np - P.panneaux.size(), P.carrefours.size(), P.ronds.size());
    return P;
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

const std::vector<Promeneur>& promeneurs(const MapDef& map, const Track& t) { return adapter(map, t).promeneurs; }

std::vector<Passant> passants(const MapDef& map, const Track& t) {
    std::vector<Passant> v;
    for (const Carrefour& c : adapter(map, t).carrefours) for (int k = 0; k < 4; k++)
        v.push_back({(double)c.ligne + (1.4 + 0.45 * k) / SEG_M, (k % 2 ? 1 : -1) * (RW + 1.2 + 0.5 * k), 1.25 + 0.1 * k, k % 2 ? -1 : 1, c.feu});
    return v;
}

void traverser(double& lat, int& dir, bool& marche, float& yawRel, double& y, int feu, double v, double dt) {
    const double but = dir * (RW + 1.2);
    marche = std::fabs(lat) < RW + 0.6 || trace::feuA(feu) == trace::Feu::Rouge;   // sur la chaussée : on finit de traverser
    if (marche) lat += (but > lat ? 1 : -1) * std::min(v * dt, std::fabs(but - lat));
    if (std::fabs(but - lat) < 1e-3) { dir = -dir; marche = false; }   // arrivé : la prochaine fois, vers l'autre trottoir
    // le personnage regarde vers +Z : π/2 relatif à la route = vers la droite (+latéral) ; à l'arrêt, face à la chaussée
    yawRel = marche ? (but > lat ? 1.5708f : -1.5708f) : (lat > 0 ? -1.5708f : 1.5708f);
    y = std::fabs(lat) < RW ? 0.02 : 0.15;   // chaussée, ou trottoir (0,15 m)
}

} // namespace ville

} // namespace wfs
