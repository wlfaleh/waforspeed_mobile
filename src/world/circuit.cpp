// WaForSpeed — carte Grand Prix (circuit), ticket WFS-53 (ex-WFS-54), épic E2. Port du décor « circuit » de
// js/world/decor.js :
//   - zones Ligne droite des tribunes, Paddock, Virages rapides, Section boisée (ZONES.circuit, ZONE_LEN = 448) ;
//   - bas-côtés GROUND.circuit : vibreurs rouge / blanc (un segment sur deux), herbe ou bac à gravier, voie des stands ;
//   - murs de pneus, panneaux publicitaires, tribunes à spectateurs, projecteurs, mur et garages des stands, drapeaux,
//     postes de commissaires, sapins et chênes des sections boisées ;
//   - chaussée : texture roadCircuit (asphalte clair, traces de gomme, lignes blanches de bord) ;
//   - ambiance envFor : jour brouillard 820 m ; nuit #0e1020 à 640 m.
#include <cmath>
#include <cstring>
#include "decor_carte.h"

namespace wfs {
namespace carte {
namespace {

enum { T_TIRES = CarteDecor::T_CARTE, T_ADBOARD, T_STAND, T_PITWALL, T_GARAGE, T_FLAG, T_MARSHAL };

class Circuit : public CarteDecor {
public:
    const char* name() const override { return "circuit"; }

protected:
    const char* carteId() const override { return "circuit"; }
    Env env() const override { return {0xc4d8e8, 820, 0x1f2440, 0x0e1020, 640}; }
    bool routePropre() const override { return true; }

    int profil(const char* z, int64_t a, int, Band out[6]) const override {
        const float R = (float)RW;
        const bool gravier = !std::strcmp(z, "gravier");
        const int64_t m = ((a % 2) + 2) % 2;
        out[0] = {R, R + 1.3f, 0.04f, 0.04f, m ? C_RED : C_WHITE};   // vibreurs
        out[1] = gravier ? Band{R + 1.3f, R + 15, 0.02f, 0, C_GRAVEL} : Band{R + 1.3f, R + 14, 0.02f, 0, C_GRASS};
        // le jeu web fait commencer cette bande à 14 m même après le gravier (15 m) : elle part ici du bord du gravier
        out[2] = {gravier ? R + 15 : R + 14, R + 100, 0, 0.5f, !std::strcmp(z, "paddock") ? C_PITLANE : C_GRASS2};
        return 3;
    }

    void place(const char* z, int64_t z0, int64_t z1, Mulberry32& Dm) override {
        auto D = [&]() { return (float)Dm.next(); };
        auto fl = [](float v) { return (int64_t)std::floor(v); };
        const float R = (float)RW, SM = (float)SEG_M;
        auto tires = [&](int64_t n, int side, float lat) { Item it; it.t = T_TIRES; it.side = side; it.lat = lat; it.petit = true; item(n, it); };
        auto lamp = [&](int64_t n, int side) { Item it; it.t = T_LAMP; it.side = side; it.k = 1; item(n, it); };
        auto stand = [&](int64_t n, int side, int rows) { Item it; it.t = T_STAND; it.side = side; it.lat = R + 14; it.l = 26 * SM - 1; it.k = rows; item(n, it); };
        auto tree = [&](int64_t n, int side, float lat, float h, int kind, float along) {
            Item it; it.t = T_TREE; it.side = side; it.lat = lat; it.h = h; it.k = kind; it.along = along; item(n, it);
        };
        if (!std::strcmp(z, "tribunes")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 5.6f);
                for (int64_t n = z0; n < z1; n += 20) { Item it; it.t = T_ADBOARD; it.side = side; it.lat = R + 5.0f; it.col = (int)fl(D() * 4); item(n, it); }
            }
            for (int64_t n = z0; n < z1 - 26; n += 26) for (int side : {-1, 1}) if (D() < 0.9f) stand(n, side, 7 + (int)fl(D() * 6));
            for (int64_t n = z0 + 10; n < z1; n += 34) lamp(n, (n / 34) % 2 ? 1 : -1);
        } else if (!std::strcmp(z, "paddock")) {
            const int side = D() < 0.5f ? -1 : 1;
            for (int64_t n = z0; n < z1; n += 3) { Item it; it.t = T_PITWALL; it.side = side; it.lat = R + 1.4f; it.petit = true; item(n, it); }
            for (int64_t n = z0 + 2; n < z1 - 14; n += 14) { Item it; it.t = T_GARAGE; it.side = side; it.lat = R + 16; it.l = 14 * SM - 0.5f; it.col = (int)fl(D() * 6); item(n, it); }
            for (int64_t n = z0; n < z1; n += 4) tires(n, -side, R + 5.6f);
            for (int64_t n = z0; n < z1; n += 26) stand(n, -side, 5 + (int)fl(D() * 4));
            for (int64_t n = z0 + 6; n < z1; n += 18) { Item it; it.t = T_FLAG; it.side = side; it.lat = R + 9; it.col = (int)fl(D() * 6); it.petit = true; item(n, it); }
            for (int64_t n = z0 + 10; n < z1; n += 34) lamp(n, -side);
        } else if (!std::strcmp(z, "gravier")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 16);
                for (int64_t n = z0 + 30; n < z1; n += 90) { Item it; it.t = T_MARSHAL; it.side = side; it.lat = R + 19; it.petit = true; item(n, it); }
                for (int64_t n = z0 + fl(D() * 20); n < z1; n += 10 + fl(D() * 14)) {
                    const float lat = R + 40 + D() * 50, h = 8 + D() * 8; const int kind = D() < 0.5f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, 0);
                }
            }
            for (int64_t n = z0 + 10; n < z1; n += 40) lamp(n, (n / 40) % 2 ? 1 : -1);
        } else if (!std::strcmp(z, "foret")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 5.6f);
                for (int64_t n = z0; n < z1; n += 2) {
                    const float lat = R + 10 + D() * 30, h = 12 + D() * 10; const int kind = D() < 0.6f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, (D() - 0.5f) * 2);
                }
            }
            for (int64_t n = z0 + 10; n < z1; n += 44) lamp(n, (n / 44) % 2 ? 1 : -1);
        }
    }

    void dessine(Track& t, const Item& it, int64_t a, const Pose& p, bool, Geo out[P_COUNT]) const override {
        Geo& S = out[P_UNI];
        const float s = (float)it.side;
        switch (it.t) {
        case T_TIRES: {   // mur de pneus : 4 piles de 3, rangée du haut rouge / blanc
            const vec3 b = lateral(p, s * it.lat);
            for (int i = 0; i < 4; i++) for (int k = 0; k < 3; k++) {
                const int col = k == 2 ? ((i + it.side) % 2 ? C_TIRE_R : C_TIRE_W) : C_TIRE;
                cyl(S, b + p.f * (i * 1.2f) + vec3{0, k * 0.3f, 0}, 0.6f, 0.3f, 12, uv(col));
            }
            break;
        }
        case T_ADBOARD: {   // panneau publicitaire : cadre sombre, fond et bandeau de texte aux couleurs d'AD_TEXTS
            const Pose pm = poseAt(t, a + 5);
            box(S, lateral(pm, s * (it.lat + 0.15f)), p.r, p.f, 0.1f, 1.3f, 11.5f, uv(C_POLE));
            const vec3 c = lateral(pm, s * it.lat, 0.75f), in = pm.r * (-s * 0.02f), up{0, 1, 0};
            quad(S, c - pm.f * 5.5f - up * 0.6f, c + pm.f * 5.5f - up * 0.6f, c + pm.f * 5.5f + up * 0.6f, c - pm.f * 5.5f + up * 0.6f, uv(C_AD0 + it.col * 2));
            const vec3 c2 = c + in;
            quad(S, c2 - pm.f * 3.6f - up * 0.28f, c2 + pm.f * 3.6f - up * 0.28f, c2 + pm.f * 3.6f + up * 0.28f, c2 - pm.f * 3.6f + up * 0.28f, uv(C_AD0 + it.col * 2 + 1));
            break;
        }
        case T_STAND: {   // tribune : gradins, spectateurs (bande de couleurs par rangée), mur du fond, toit
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            const vec3 base = lateral(pm, s * it.lat), out_ = pm.r * s;
            for (int k = 0; k < it.k; k++) {
                const vec3 o = base + out_ * (k * 0.9f);
                box(S, o, pm.r, pm.f, 0.9f, 0.6f + k * 0.55f, it.l, uv(C_STAND));
                box(S, o + vec3{0, 0.6f + k * 0.55f, 0}, pm.r, pm.f, 0.5f, 0.45f, it.l * 0.98f, uv(C_SEAT0 + (int)((((k + a) % 4) + 4) % 4)));
            }
            const float back = it.k * 0.9f;
            box(S, base + out_ * back, pm.r, pm.f, 0.4f, it.k * 0.55f + 4.5f, it.l, uv(C_STAND_BACK));
            box(S, base + out_ * (back * 0.5f) + vec3{0, it.k * 0.55f + 4.5f, 0}, pm.r, pm.f, back + 2, 0.3f, it.l + 1, uv(C_STAND_ROOF));
            break;
        }
        case T_PITWALL:
            box(S, lateral(p, s * it.lat), p.r, p.f, 0.4f, 1.1f, 3 * (float)SEG_M, uv(C_PITWALL));
            box(S, lateral(p, s * (it.lat + 0.1f), 1.1f), p.r, p.f, 0.05f, 1.6f, 3 * (float)SEG_M, uv(C_PITGLASS));
            break;
        case T_GARAGE: {   // garage d'écurie : bâtiment, porte sombre, bandeau aux couleurs de l'équipe
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            box(S, lateral(pm, s * (it.lat + 6)), pm.r, pm.f, 12, 7, it.l, uv(C_GARAGE));
            box(S, lateral(pm, s * (it.lat - 0.02f)), pm.r, pm.f, 0.1f, 4.5f, it.l * 0.8f, uv(C_GARAGE_DOOR));
            box(S, lateral(pm, s * (it.lat - 0.06f), 5.2f), pm.r, pm.f, 0.06f, 1.1f, it.l * 0.85f, uv(C_GARAGE0 + it.col));
            break;
        }
        case T_FLAG: {
            const vec3 b = lateral(p, s * it.lat);
            cyl(S, b, 0.08f, 9, 8, uv(C_FLAGPOLE));
            box(S, b + p.f * 1.2f + vec3{0, 6.8f, 0}, p.r, p.f, 0.04f, 1.8f, 2.4f, uv(C_FLAG0 + it.col));
            break;
        }
        case T_MARSHAL: {
            const vec3 b = lateral(p, s * it.lat);
            box(S, b, p.r, p.f, 1.6f, 2.4f, 1.6f, uv(C_MARSHAL));
            box(S, b + vec3{0, 2.4f, 0}, p.r, p.f, 2, 0.15f, 2, uv(C_MARSHAL_TOP));
            break;
        }
        default: break;
        }
    }
};

} // namespace
} // namespace carte

WFS_SYSTEM(carte::Circuit, 108)   // décor (100-199) : après la campagne (107)

} // namespace wfs
