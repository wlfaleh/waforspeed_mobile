// WaForSpeed — carte Campagne, ticket WFS-53, épic E2. Port du décor « country » de js/world/decor.js et life.js :
//   - zones Champs, Forêt, Village, Vignobles, Plaine, Bois de pins (ZONES.country, ZONE_LEN = 448) ;
//   - bas-côtés GROUND.country : verge, fossé, champs de couleurs (fieldCol) ou rangs de vignes sur les coteaux, collines ;
//   - champs : clôtures, haies (masse + cartes de feuilles), chênes et sapins, fermes et granges, bottes de foin,
//     éoliennes (pales animées), poteaux ; forêt : chênes, sapins (pins sylvestres au « Bois de pins »), sous-bois ;
//     village : maisons colorées, église, lampadaires, passages piétons ; vignobles : vignes, cyprès, mas ;
//   - ambiance envFor : jour brouillard #d3e0d0 à 820 m ; nuit sans lampadaires, brouillard #0d1218.
// Placement généré par la graine de la carte (mulberry32(seed · 31 + 7)), dans l'ordre des tirages du jeu web.
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "decor_carte.h"

namespace wfs {
namespace carte {
namespace {

enum { T_FENCE = CarteDecor::T_CARTE, T_HEDGE, T_FARM, T_BALES, T_TURBINE, T_POLE, T_SIGN, T_VILLA, T_CHURCH, T_ZEBRA, T_VINES };

class Campagne : public CarteDecor {
public:
    const char* name() const override { return "campagne"; }

protected:
    const char* carteId() const override { return "campagne"; }
    Env env() const override { return {0xd3e0d0, 820, 0x1b2236, 0x0d1218, 540}; }

    // GROUND.country : distances depuis le bord de la chaussée
    int profil(const char* z, int64_t a, int s, Band out[6]) const override {
        const float R = (float)RW;
        const bool foret = !std::strcmp(z, "foret"), vignes = !std::strcmp(z, "vignes"), village = !std::strcmp(z, "village");
        // fieldCol : couleur du champ tirée par bloc de 70 segments ; vineCol : rangs de terre tous les 3 segments
        const int field = C_FIELD0 + (int)(Mulberry32((uint32_t)((int)std::floor(a / 70.0) * 7 + (s > 0 ? 3 : 0))).next() * 6);
        const int vine = (int)std::floor(a / 3.0) % 2 ? C_VINE_A : C_VINE_B;
        out[0] = {R, R + 1.6f, 0, -0.05f, village ? C_WALK : C_VERGE};
        out[1] = {R + 1.6f, R + 3, -0.05f, -0.4f, foret ? C_FOREST : C_DITCH};
        out[2] = {R + 3, R + 95, -0.4f, vignes ? 6.f : 0.2f, foret ? C_FOREST : vignes ? vine : village ? C_VERGE : field};
        out[3] = {R + 95, R + 220, vignes ? 6.f : 0.2f, vignes ? 26.f : 9.f, foret ? C_FOREST : C_FARHILL};
        return 4;
    }

    void place(const char* z, int64_t z0, int64_t z1, Mulberry32& Dm) override {
        auto D = [&]() { return (float)Dm.next(); };
        auto fl = [](float v) { return (int64_t)std::floor(v); };
        const float R = (float)RW, SM = (float)SEG_M;
        auto tree = [&](int64_t n, int side, float lat, float h, int kind, float along) {
            Item it; it.t = T_TREE; it.side = side; it.lat = lat; it.h = h; it.k = kind; it.along = along; item(n, it);
        };
        if (!std::strcmp(z, "champs")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 3) if (fl((n + (side > 0 ? 40 : 0)) / 90.f) % 2 == 0) { Item it; it.t = T_FENCE; it.side = side; it.lat = R + 3.4f; it.petit = true; item(n, it); }
                for (int64_t n = z0; n < z1; n += 24) if (fl((n + (side > 0 ? 40 : 0)) / 90.f) % 2 == 1 && D() < 0.8f) { Item it; it.t = T_HEDGE; it.side = side; it.lat = R + 4.2f; it.l = 24 * SM - 2; item(n, it); }
                for (int64_t n = z0 + fl(D() * 10); n < z1; n += 6 + fl(D() * 18)) {
                    const float u = D(), lat = R + 6 + u * (D() < 0.3f ? 70 : 20), h = 6 + D() * 7;
                    const int kind = D() < 0.35f ? K_PINE : K_ROUND;
                    tree(n, side, lat, h, kind, (D() - 0.5f) * 6);
                }
                for (int64_t n = z0 + 30 + fl(D() * 60); n < z1; n += 140 + fl(D() * 120)) { Item it; it.t = T_FARM; it.side = side; it.lat = R + 28 + D() * 30; it.k = D() < 0.5f; item(n, it); }
                for (int64_t n = z0 + fl(D() * 40); n < z1; n += 40 + fl(D() * 60)) { Item it; it.t = T_BALES; it.side = side; it.lat = R + 16 + D() * 40; it.k = 2 + (int)fl(D() * 4); it.petit = true; item(n, it); }
                for (int64_t n = z0 + 60 + fl(D() * 50); n < z1; n += 180 + fl(D() * 150)) {
                    Item it; it.t = T_TURBINE; it.side = side; it.lat = R + 90 + D() * 80; it.h = 55 + D() * 25; item(n, it);
                    turbines_.push_back({n, it});
                }
            }
            for (int64_t n = z0 + 20; n < z1; n += 50) { Item it; it.t = T_POLE; it.side = (n / 50) % 2 ? 1 : -1; it.lat = R + 2.6f; it.petit = true; item(n, it); }
        } else if (!std::strcmp(z, "foret")) {
            for (int side : {-1, 1}) for (int64_t n = z0 + fl(D() * 3); n < z1; n += 1 + fl(D() * 2)) {
                { const float lat = R + 4.5f + D() * 9, h = 12 + D() * 10; const int kind = D() < 0.7f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, (D() - 0.5f) * 1.2f); }
                if (D() < 0.6f) { const float lat = R + 14 + D() * 35, h = 14 + D() * 10; const int kind = D() < 0.7f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, (D() - 0.5f) * 1.2f); }
            }
            for (int64_t n = z0 + 30; n < z1; n += 90) { Item it; it.t = T_SIGN; it.side = D() < 0.5f ? -1 : 1; it.lat = R + 2; it.petit = true; item(n, it); }
        } else if (!std::strcmp(z, "village")) {
            for (int side : {-1, 1}) for (int64_t n = z0 + (side < 0 ? 1 : 4); n < z1 - 7; n += 7 + fl(D() * 4)) {
                Item it; it.t = T_VILLA; it.side = side; it.lat = R + 4 + D() * 3; it.w = 7 + D() * 4; it.d = 8 + D() * 3; it.h = 5 + D() * 3;
                it.roof = (int)fl(D() * 3); it.floors = D() < 0.5f ? 2 : 1; it.col = (int)fl(D() * 6); item(n, it);
            }
            { Item it; it.t = T_CHURCH; it.side = D() < 0.5f ? -1 : 1; it.lat = R + 20; item(z0 + (z1 - z0) / 2, it); }
            for (int64_t n = z0 + 10; n < z1; n += 24) { Item it; it.t = T_LAMP; it.side = (n / 24) % 2 ? 1 : -1; it.k = 0; item(n, it); }
            for (int64_t n = z0 + 40; n < z1; n += 120) { Item it; it.t = T_ZEBRA; item(n, it); }
        } else if (!std::strcmp(z, "vignes")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 12) for (int k = 0; k < 10; k++) { Item it; it.t = T_VINES; it.side = side; it.lat = R + 6 + k * 3.2f; it.l = 12 * SM - 0.6f; it.petit = true; item(n, it); }
                for (int64_t n = z0 + fl(D() * 10); n < z1; n += 14 + fl(D() * 10)) { const float h = 9 + D() * 4; tree(n, side, R + 3.5f, h, K_CYPRESS, 0); }
                for (int64_t n = z0 + 80; n < z1; n += 200) { Item it; it.t = T_FARM; it.side = side; it.lat = R + 45 + D() * 20; it.k = 0; item(n, it); }
            }
        }
    }

    void dessine(Track& t, const Item& it, int64_t a, const Pose& p, bool, Geo out[P_COUNT]) const override {
        Geo& S = out[P_UNI];
        const char* z = zone(a);
        const float s = (float)it.side;
        auto gy = [&](float lat) { return solY(z, a, it.side, lat); };
        switch (it.t) {
        case T_FENCE:
            box(S, lateral(p, s * it.lat), p.r, p.f, 0.12f, 1.2f, 0.12f, uv(C_WOOD));
            for (float y : {0.45f, 0.95f}) box(S, lateral(p, s * it.lat, y), p.r, p.f, 0.06f, 0.1f, 3 * (float)SEG_M, uv(C_WOOD));
            break;
        case T_HEDGE: {   // haie champêtre (life.js) : masse sombre + cartes de feuilles sur les faces visibles
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            const vec3 c = lateral(pm, s * it.lat, -0.2f);
            box(S, c, pm.r, pm.f, 1.1f, 1.5f, it.l, uv(C_HEDGE));
            Mulberry32 R((uint32_t)((int)a * 31 + (it.side > 0 ? 7 : 0)));
            veg::cartesHaie(out[P_FEUILLU], c, pm.r, pm.f, it.side, it.l, R, 1, false);
            break;
        }
        case T_FARM: {
            const vec3 b = lateral(p, s * it.lat, gy(it.lat) - 0.2f);
            box(S, b, p.r, p.f, 8, 4.5f, 11, uv(C_WALL2));
            toit(S, b + vec3{0, 4.5f + 1.1f, 0}, p.r, p.f, 8.6f, 2.2f, 11.6f, uv(C_ROOF0));
            if (it.k) {
                const vec3 bb = b + p.f * 16;
                box(S, bb, p.r, p.f, 10, 6, 14, uv(C_BARN));
                toit(S, bb + vec3{0, 6 + 1.4f, 0}, p.r, p.f, 10.6f, 2.8f, 14.6f, uv(C_ROOF2));
            }
            break;
        }
        case T_BALES: {   // bottes couchées, axe selon −r
            const vec3 b = lateral(p, s * it.lat, gy(it.lat));
            for (int i = 0; i < it.k; i++) cylAxe(S, b + p.f * (i * 3.2f) + p.r * ((i % 2) * 2.f) + vec3{0, 0.75f, 0}, p.r * -1.f, 0.75f, 1.4f, 12, uv(C_HAY));
            break;
        }
        case T_TURBINE: {   // mât et nacelle ; pales : animes()
            const vec3 b = lateral(p, s * it.lat, gy(it.lat) - 0.5f), hub = b + vec3{0, it.h, 0};
            cyl(S, b, 1.0f, it.h, 8, uv(C_TURB));
            box(S, hub - vec3{0, 1, 0}, p.r, p.f, 2, 2, 4, uv(C_TURB));
            break;
        }
        case T_POLE: {
            const vec3 b = lateral(p, s * it.lat, -0.2f);
            cyl(S, b, 0.14f, 8.5f, 8, uv(C_WOOD));
            box(S, b + vec3{0, 7.8f, 0}, p.r, p.f, 1.6f, 0.12f, 0.12f, uv(C_WOOD));
            break;
        }
        case T_SIGN: {
            const vec3 b = lateral(p, s * it.lat);
            box(S, b, p.r, p.f, 0.1f, 2.2f, 0.1f, uv(C_METAL));
            box(S, b + vec3{0, 2.2f, 0}, p.r, p.f, 0.06f, 0.8f, 1.2f, uv(C_SIGNW));
            box(S, b - p.r * (s * 0.01f) + vec3{0, 2.3f, 0}, p.r, p.f, 0.06f, 0.6f, 1.0f, uv(C_SIGNR));
            break;
        }
        case T_VILLA: {   // maison colorée : murs, fenêtres côté route, toit
            const vec3 o = lateral(p, s * (it.lat + it.d / 2), gy(it.lat + it.d / 2) - 0.2f);
            const int wall = it.roof == 1 ? C_WALLS0 + it.col : C_WALLS0 + (it.col + 2) % 6;
            box(S, o, p.r, p.f, it.d, it.h, it.w, uv(wall));
            for (int i = -1; i <= 1; i++) for (int fl = 0; fl < it.floors; fl++)
                box(S, o + p.r * (-s * (it.d / 2 + 0.02f)) + p.f * (i * it.w * 0.3f) + vec3{0, 1 + fl * 2.6f, 0}, p.r, p.f, 0.06f, 1.3f, 1.2f, uv(C_GLASS));
            toit(S, o + vec3{0, it.h + 0.9f, 0}, p.r, p.f, it.d * 1.05f, 1.8f, it.w * 1.05f, uv(C_ROOF0 + it.roof));
            break;
        }
        case T_CHURCH: {
            const vec3 b = lateral(p, s * it.lat, gy(it.lat) - 0.2f);
            box(S, b, p.r, p.f, 10, 9, 18, uv(C_CHURCH));
            toit(S, b + vec3{0, 10.2f, 0}, p.r, p.f, 10.6f, 2.6f, 18.6f, uv(C_ROOF0));
            const vec3 tw = b - p.f * 10;
            box(S, tw, p.r, p.f, 4.5f, 20, 4.5f, uv(C_CHURCH));
            cone(S, tw + vec3{0, 20, 0}, 3.4f, 8, 8, uv(C_ROOF2), PI / 4 - p.th);
            break;
        }
        case T_ZEBRA: {   // passage piéton : bandes blanches parallèles à la route
            const Pose p1 = poseAt(t, a + 3);
            const float R = (float)RW;
            for (int k = 0; k < 8; k++) {
                const float x0 = -R + 0.6f + k * (2 * R - 1.2f) / 8, x1 = x0 + 0.95f;
                quadSol(S, lateral(p, x0, 0.015f), lateral(p, x1, 0.015f), lateral(p1, x1, 0.015f), lateral(p1, x0, 0.015f), uv(C_ZEBRA));
            }
            break;
        }
        case T_VINES: {   // rang de vigne : boîte de DECOR.vines (0,7 × 1,2 m, effilée de 0,6 en largeur et en longueur),
            // posée sur le coteau à chaque tronçon (WFS-93) : le rang suit la pente et les virages, plus de marches
            const float Ls = it.l / (float)SEG_M, e = 0.2f * Ls;   // longueur et retrait du dessus à chaque bout (tronçons)
            const double sA = (double)a + (double)std::lround(Ls / 2) - Ls / 2.0, sB = sA + Ls;
            double sb[24]; int n = 0;   // tronçons du bas : bouts du rang et tronçons entiers entre les deux
            sb[n++] = sA;
            for (int64_t k = (int64_t)std::floor(sA) + 1; k < sB && n < 23; k++) sb[n++] = (double)k;
            sb[n++] = sB;
            const float yb = gy(it.lat) - 0.05f, wb = 0.35f, wt = 0.21f, x0 = s * it.lat;
            auto P = [&](double sg, float x, float dy) { const TrackPoint q = t.point(sg * SEG, x); return vec3{(float)q.x, (float)q.y + dy, (float)q.z}; };
            vec3 B[24][2], T[24][2];   // [tronçon][côté −r, +r]
            for (int i = 0; i < n; i++) {
                const double st = sA + e + (sb[i] - sA) * (Ls - 2 * e) / Ls;   // dessus plus court : même fraction du rang
                for (int k = 0; k < 2; k++) {
                    const float sx = k ? 1.f : -1.f;
                    B[i][k] = P(sb[i], x0 + sx * wb, gy(it.lat + s * sx * wb) - 0.05f);
                    T[i][k] = P(st, x0 + sx * wt, yb + 1.2f);
                }
            }
            const vec2 c = uv(C_VINE);   // ordre des sommets de box : normales vers l'extérieur
            for (int i = 0; i + 1 < n; i++) {
                quad(S, T[i][0], T[i][1], T[i + 1][1], T[i + 1][0], c);
                quad(S, B[i][1], B[i + 1][1], T[i + 1][1], T[i][1], c);
                quad(S, B[i + 1][0], B[i][0], T[i][0], T[i + 1][0], c);
            }
            quad(S, B[n - 1][1], B[n - 1][0], T[n - 1][0], T[n - 1][1], c);
            quad(S, B[0][0], B[0][1], T[0][1], T[0][0], c);
            break;
        }
        default: break;
        }
    }

    // pales des éoliennes : objet séparé qui tourne (rotors de scene.js), un maillage par éolienne
    int animes(Renderer& r, const GameView& g, int64_t c0, int64_t c1) override {
        if (rotors_.empty()) {
            for (const auto& [seg, it] : turbines_) {
                Geo B;
                for (int k = 0; k < 3; k++) {
                    const float ph = k * 2 * PI / 3;
                    const vec3 X{std::cos(ph), std::sin(ph), 0}, Y{-std::sin(ph), std::cos(ph), 0}, Z{0, 0, -1};
                    // box de 1,2 × h · 0,42 × 0,25 dans le repère (X, Y, Z) de la pale, du moyeu vers l'extérieur
                    const float w = 1.2f, hh = it.h * 0.42f, l = 0.25f;
                    auto C = [&](float sx, float sy, float sz) { return X * (sx * w / 2) + Y * (sy * hh) + Z * (sz * l / 2); };
                    quad(B, C(-1, 1, -1), C(1, 1, -1), C(1, 1, 1), C(-1, 1, 1), uv(C_TURB));
                    quad(B, C(1, 0, -1), C(1, 0, 1), C(1, 1, 1), C(1, 1, -1), uv(C_TURB));
                    quad(B, C(-1, 0, 1), C(-1, 0, -1), C(-1, 1, -1), C(-1, 1, 1), uv(C_TURB));
                    quad(B, C(1, 0, 1), C(-1, 0, 1), C(-1, 1, 1), C(1, 1, 1), uv(C_TURB));
                    quad(B, C(-1, 0, -1), C(1, 0, -1), C(1, 1, -1), C(-1, 1, -1), uv(C_TURB));
                }
                rotors_.push_back(r.makeMesh(B.v, B.i));
            }
        }
        int n = 0;
        Track& t = *g.track;
        for (size_t i = 0; i < turbines_.size(); i++) {
            const auto& [seg, it] = turbines_[i];
            int64_t a;
            if (!dansMorceau(seg, c0 * CH, (c1 + 1) * CH, a)) continue;
            const Pose p = poseAt(t, a);
            const vec3 b = lateral(p, it.side * it.lat, solY(zone(a), a, it.side, it.lat) - 0.5f);
            const vec3 hub = b + vec3{0, it.h, 0} - p.f * 2.2f;
            const float spin = 0.6f + std::fmod(it.h, 7.f) * 0.05f;
            r.draw(rotors_[i], mats_[P_UNI], translate(hub) * rotateY(-p.th) * rotateZ(spin * (float)g.time));
            n++;
        }
        return n;
    }

    void shutdownCarte(Renderer& r) override {
        for (Mesh& m : rotors_) r.destroy(m);
        rotors_.clear(); turbines_.clear();
    }

private:
    std::vector<std::pair<int64_t, Item>> turbines_;
    std::vector<Mesh> rotors_;
};

} // namespace
} // namespace carte

WFS_SYSTEM(carte::Campagne, 107)   // décor (100-199) : après la plage (105) et les piétons (106)

} // namespace wfs
