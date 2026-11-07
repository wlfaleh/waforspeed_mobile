// WaForSpeed — carte Campagne, ticket WFS-53, épic E2. Port du décor « country » de js/world/decor.js et life.js :
//   - zones Champs, Forêt, Village, Vignobles, Plaine, Bois de pins (ZONES.country, ZONE_LEN = 448) ;
//   - bas-côtés GROUND.country : verge, fossé, champs de couleurs (fieldCol) ou rangs de vignes sur les coteaux, collines ;
//   - champs : clôtures, haies (masse + cartes de feuilles), chênes et sapins, fermes et granges, bottes de foin,
//     éoliennes (pales animées), poteaux ; forêt : chênes, sapins (pins sylvestres au « Bois de pins »), sous-bois ;
//     village : maisons colorées, église, lampadaires, passages piétons ; vignobles : vignes, cyprès, mas ;
//   - ambiance envFor : jour brouillard #d3e0d0 à 820 m ; nuit sans lampadaires, brouillard #0d1218.
// Placement généré par la graine de la carte (mulberry32(seed · 31 + 7)), dans l'ordre des tirages du jeu web.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>
#include "archi_carte.h"   // WFS-161 (accroche) : maisons de village du kit d'architecture
#include "../core/log.h"
#include "../core/platform.h"   // WFS-186 : readAsset (animaux .wfa)
#include "decor_carte.h"
#include "../game/trace_cartes.h"   // WFS-138 : sections du tracé 2 (lacets du col, pont)
#include "enseignes.h"   // WFS-178 : textes des enseignes du village (atlas)
#include "../game/traffic.h"        // WFS-179 : couleurs du trafic (voitures en sens inverse)
#include "../game/roues.h"          // WFS-179 : roues des voitures en sens inverse
#include "../render/eclairage.h"    // WFS-179 : lumières de nuit du péage
#include "../render/model_loader.h" // WFS-179 : modèles des voitures en sens inverse

namespace wfs {
namespace carte {
namespace {

int64_t g_troupeau[3] = {-1, -1, -1};   // WFS-186 : premier segment de chaque troupeau (vache, cheval, mouton), pour le banc

enum { T_FENCE = CarteDecor::T_CARTE, T_HEDGE, T_FARM, T_BALES, T_TURBINE, T_POLE, T_SIGN, T_VILLA, T_CHURCH, T_ZEBRA, T_VINES,
       T_GLISS, T_MURET, T_DANGER, T_CASCADE,   // WFS-138 : glissière, muret, panneau A1, cascade
       T_FONTAINE, T_FEU, T_COMMERCE,           // WFS-178 : rond-point fleuri, feu tricolore, commerces du village
       T_AUTO, T_PEAGE, T_PANNEAU_A,            // WFS-179 : autoroute (un par segment), aire de péage, panneau bleu
       T_ENCLOS };                              // WFS-186 : enclos des chevaux (clôture de bois sur les 4 côtés)
// WFS-179 : textes des panneaux bleus de l'autoroute et du péage (noms de lieux inventés)
const char* const PANNEAUX_A[4] = {"VALLÉE VERTE  14 km", "LES PINS  9 km", "AIRE DE REPOS  2 km", "PÉAGE"};
// WFS-178 (RG-03) : commerces du village (it.k) et texte de leur enseigne (pas de vraie marque ni de vrai nom de lieu)
enum { CO_CAFE, CO_BAR, CO_GARAGE, CO_BOULANGERIE, CO_SUPERMARCHE, CO_NB };
const char* const ENSEIGNES[CO_NB] = {"CAFÉ", "BAR", "GARAGE", "BOULANGERIE", "SUPERMARCHÉ"};
// ligne de texte de l'atlas sur un plan vertical centré en c, lue selon x (comme circuit.cpp, sans italique)
void ligneTexte(Geo& g, vec3 c, vec3 x, float w, float h, const vie::Texte& T) {
    const vec3 up{0, 1, 0}, n = cross(x, up), hx = x * (w / 2), hy = up * (h / 2);
    const vec3 P[4] = {c - hx - hy, c + hx - hy, c + hx + hy, c - hx + hy};
    const vec2 U[4] = {{T.u0, T.v1}, {T.u1, T.v1}, {T.u1, T.v0}, {T.u0, T.v0}};
    const uint32_t k = (uint32_t)g.v.size();
    for (int i = 0; i < 4; i++) g.v.push_back({P[i].x, P[i].y, P[i].z, n.x, n.y, n.z, U[i].x, U[i].y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}

class Campagne : public CarteDecor {
public:
    const char* name() const override { return "campagne"; }
    // WFS-178 : atlas des enseignes (lettres blanches, néon doux la nuit) et feux tricolores (lampes allumées / éteintes)
    void init(Renderer& r, const GameView& g) override {
        betes_.clear(); numBloc_ = 0; for (int64_t& x : g_troupeau) x = -1;   // WFS-186 : placés de nouveau par CarteDecor::init
        CarteDecor::init(r, g);
        for (auto& T : txt_) T = {};
        if (!active_) return;
        {   // tête du feu : une lampe de Ø 200 mm (disque à 12 côtés, face −z), dessinée 3 fois par feu
            Geo G; const int N = 12;
            for (int i = 0; i < N; i++) { const float a0 = 2 * PI * i / N, a1 = 2 * PI * (i + 1) / N;
                tri(G, {0, 0, 0}, {0.1f * std::cos(a1), 0.1f * std::sin(a1), 0}, {0.1f * std::cos(a0), 0.1f * std::sin(a0), 0}, uv(C_CHECK_B)); }
            lampeMesh_ = r.makeMesh(G.v, G.i);
        }
        const uint32_t COUL[3] = {0xff2a1a, 0xffa000, 0x30ff70};   // rouge, orange (ambre), vert (LED de feu tricolore)
        for (int k = 0; k < 3; k++) {
            Material on; on.rough = 0.3f; on.flags = MAT_DOUBLE; on.emissive = linHex(COUL[k]) * 3.f; lampeOn_[k] = r.makeMaterial(on, nullptr, nullptr, nullptr);
            Material off; off.color = {linHex(COUL[k]).x * 0.06f, linHex(COUL[k]).y * 0.06f, linHex(COUL[k]).z * 0.06f, 1}; off.rough = 0.4f; off.flags = MAT_DOUBLE;
            lampeOff_[k] = r.makeMaterial(off, nullptr, nullptr, nullptr);
        }
        for (const char* t : ENSEIGNES) atlas_.ajouter(t);
        for (const char* t : PANNEAUX_A) atlas_.ajouter(t);   // WFS-179
        if (auDeb_ >= 0)   // WFS-179 : voitures en sens inverse (deux modèles qui ne sont pas ceux du trafic)
            for (int i = 0; i < 2; i++) if (!loadCarModel(r, i ? "cars/corsa.wfm" : "cars/v60.wfm", autos_[i])) WFS_ERR("Campagne : modèle du sens inverse introuvable");
        chargerAnimaux(r);   // WFS-186
        if (!atlas_.construire(r)) return;   // police absente : enseignes sans texte
        // lettres blanches de jour ; la nuit, enseigne éclairée sans halo exagéré (émission modérée, pas de bloom fort)
        mats_[P_TEXTE0] = atlas_.materiau(r, 0xf4f1e8, night_ ? 1.1f : 0.f);
        for (int k = 0; k < CO_NB; k++) txt_[k] = atlas_.texte(ENSEIGNES[k]);
        for (int k = 0; k < 4; k++) txtA_[k] = atlas_.texte(PANNEAUX_A[k]);   // WFS-179
    }

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
        if (s < 0 && autoroute(a)) {   // WFS-179 : terre-plein central (3 m), chaussée opposée (14 m), accotement, puis la campagne
            out[0] = {R, R + 3, 0, 0, C_VERGE};
            out[1] = {R + 3, R + 17, 0.01f, 0.01f, C_PITLANE};   // enrobé (albédo ≈ 0,045 : asphalte neuf 0,04-0,07)
            out[2] = {R + 17, R + 19, 0, -0.05f, C_VERGE};
            out[3] = {R + 19, R + 21, -0.05f, -0.4f, foret ? C_FOREST : C_DITCH};
            out[4] = {R + 21, R + 95, -0.4f, vignes ? 6.f : 0.2f, foret ? C_FOREST : vignes ? vine : field};
            out[5] = {R + 95, R + 220, vignes ? 6.f : 0.2f, vignes ? 26.f : 9.f, foret ? C_FOREST : C_FARHILL};
            return borner(a, s, out, 6);
        }
        out[0] = {R, R + 1.6f, 0, -0.05f, village ? C_WALK : C_VERGE};
        out[1] = {R + 1.6f, R + 3, -0.05f, -0.4f, foret ? C_FOREST : C_DITCH};
        out[2] = {R + 3, R + 95, -0.4f, vignes ? 6.f : 0.2f, foret ? C_FOREST : vignes ? vine : village ? C_VERGE : field};
        out[3] = {R + 95, R + 220, vignes ? 6.f : 0.2f, vignes ? 26.f : 9.f, foret ? C_FOREST : C_FARHILL};
        return borner(a, s, out, 4);
    }

    void place(const char* z, int64_t z0, int64_t z1, Mulberry32& Dm) override {
        if (libre_.empty()) { calculerLibre(); placerCol(); }
        // WFS-138 (RG-C1) : un objet n'est posé que s'il tient avant la mi-distance de la route voisine (2 m de marge)
        if (auDeb_ == -2) calculerAutoroute();   // WFS-179
        std::vector<Obst> obst;   // WFS-186
        auto item = [&](int64_t n, const Item& it) {
            // WFS-179 (RG-C1) : rien sur la chaussée opposée de l'autoroute ni sur l'aire de péage
            if (autoroute(n) && ((it.side < 0 && it.lat < (float)RW + 21) || (aPeage(n) && it.lat < (float)RW + 8))) return;
            // pas de village au bord d'une autoroute : ni maison, ni église, ni commerce, ni lampadaire, ni passage piéton, ni
            // objet à moins de 8 m de la chaussée (clôture, poteau)
            if (autoroute(n) && (it.t == T_VILLA || it.t == T_CHURCH || it.t == T_COMMERCE || it.t == T_LAMP || it.t == T_ZEBRA || it.lat < (float)RW + 8)) return;
            const Libre* l = libreA(n, it.side);
            if (l && it.lat + std::max(it.d, 2.f) + 2 > l->m) return;
            // WFS-178 : pas de maison ni d'arbre à la place d'un commerce du village (même côté, sur sa longueur + 4 m)
            if (it.t == T_VILLA || it.t == T_TREE || it.t == T_LAMP)
                for (const auto& [cs, cc, demi] : commerces_) if (cc == it.side && std::llabs(n - cs) <= demi) return;
            // WFS-178 (RG-C1) : rien de haut près de la route dans les 60 m qui précèdent un feu ou l'entrée du rond-point
            if ((it.t == T_VILLA || it.t == T_TREE || it.t == T_FARM || it.t == T_HEDGE) && it.lat < (float)RW + 14)
                for (int64_t v : vigilance_) { const int64_t d = v - n; if (d >= 0 && d < 50) return; }
            // clairière de la cascade (RG-01) : rien entre la route et la paroi, sur ±15 segments (≈ 18 m)
            if (aCascade_ && it.side == cascadeCote_ && std::llabs(n - cascadeSeg_) < 15) return;
            // WFS-186 : place prise par les arbres, fermes, bottes et éoliennes (les animaux n'y sont pas posés)
            if (it.t == T_TREE) obst.push_back({n - 3, n + 3, it.side, it.lat - 3, it.lat + 3});
            if (it.t == T_FARM) obst.push_back({n - 7, n + 25, it.side, it.lat - 7, it.lat + 8});
            if (it.t == T_BALES) obst.push_back({n - 2, n + it.k * 3 + 2, it.side, it.lat - 2, it.lat + 4});
            if (it.t == T_TURBINE || it.t == T_HEDGE) obst.push_back({n - 3, n + (int64_t)(it.l / SEG_M) + 3, it.side, it.lat - 3, it.lat + 3});
            if (it.t == T_VILLA) { const Pose pv = poseAt(*track_, n); const float s = (float)it.side; const vec3 x = pv.f * -s;   // WFS-161 (accroche) : maison du kit
                archi::carte::poser(n, lateral(pv, s * it.lat, solY(z, n, it.side, it.lat) - 0.2f) - x * (it.w / 2), x, pv.r * -s, it.w, it.d, it.h, archi::Style::Village, (uint32_t)(n * 2 + (it.side > 0))); }
            CarteDecor::item(n, it);
        };
        auto D = [&]() { return (float)Dm.next(); };
        auto fl = [](float v) { return (int64_t)std::floor(v); };
        const float R = (float)RW, SM = (float)SEG_M;
        // WFS-179 (RG-04) : autoroute : un objet par segment (marquage, séparateur), glissières (une lisse tous les 2 segments,
        // des deux côtés, sauf sur l'aire de péage), aire de péage, panneaux bleus tous les 600 m
        for (int64_t n = z0; n < z1; n++) {
            if (!autoroute(n)) continue;
            Item it; it.t = T_AUTO; it.side = -1; CarteDecor::item(n, it);
            if (n % 2 == 0 && !aPeage(n)) {
                Item g1; g1.t = T_GLISS; g1.side = 1; g1.lat = R + 1.0f; g1.petit = true; CarteDecor::item(n, g1);
                Item g2; g2.t = T_GLISS; g2.side = -1; g2.lat = R + 17.3f; g2.petit = true; CarteDecor::item(n, g2);
            }
            if (n == auDeb_ + PEAGE_SEG) { Item pg; pg.t = T_PEAGE; pg.side = 1; CarteDecor::item(n, pg); }
            const int64_t k = n - auDeb_ - (int64_t)(250 / SM);
            if (k >= 0 && k % (int64_t)(600 / SM) == 0) { Item pa; pa.t = T_PANNEAU_A; pa.side = 1; pa.lat = R + 4.6f; pa.k = (int)(k / (int64_t)(600 / SM)) % 3; CarteDecor::item(n, pa); }
        }
        auto tree = [&](int64_t n, int side, float lat, float h, int kind, float along) {
            if (altitude(n) > 60) kind = K_PINE;   // WFS-138 (RG-01) : étage montagnard : les sapins remplacent les feuillus
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
            troupeaux(z0, z1, obst);   // WFS-186 : après le reste du champ (tirages à part : le décor existant ne change pas)
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
                // WFS-138 (RG-05, ANO-65) : rangs continus (blocs de 12 tronçons bord à bord, sans retrait ni trou), un tous les
                // 2 m (écartement réel 1,5 à 2,5 m entre rangs palissés), 16 rangs de chaque côté
                for (int64_t n = z0; n < z1; n += 12) for (int k = 0; k < 16; k++) { Item it; it.t = T_VINES; it.side = side; it.lat = R + 6 + k * 2.f; it.l = 12 * SM; it.petit = true; item(n, it); }
                for (int64_t n = z0 + fl(D() * 10); n < z1; n += 14 + fl(D() * 10)) { const float h = 9 + D() * 4; tree(n, side, R + 3.5f, h, K_CYPRESS, 0); }
                for (int64_t n = z0 + 80; n < z1; n += 200) { Item it; it.t = T_FARM; it.side = side; it.lat = R + 45 + D() * 20; it.k = 0; item(n, it); }
            }
        }
    }

    void dessine(Track& t, const Item& it, int64_t a, const Pose& p, bool lod, Geo out[P_COUNT]) const override {
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
            if (archi::carte::actif()) break;   // WFS-161 (accroche) : maison du kit (archi_carte.cpp) à la place de la boîte
            const vec3 o = lateral(p, s * (it.lat + it.d / 2), gy(it.lat + it.d / 2) - 0.2f);
            // WFS-178 (RG-03) : maisons de village en pierre calcaire (albédo 0,39-0,49) et tuiles de terre cuite
            const int wall = C_PIERRE0 + it.col % 3;
            box(S, o, p.r, p.f, it.d, it.h, it.w, uv(wall));
            for (int i = -1; i <= 1; i++) for (int fl = 0; fl < it.floors; fl++)
                box(S, o + p.r * (-s * (it.d / 2 + 0.02f)) + p.f * (i * it.w * 0.3f) + vec3{0, 1 + fl * 2.6f, 0}, p.r, p.f, 0.06f, 1.3f, 1.2f, uv(C_GLASS));
            toit(S, o + vec3{0, it.h + 0.9f, 0}, p.r, p.f, it.d * 1.05f, 1.8f, it.w * 1.05f, uv(it.roof == 2 ? C_ROOF2 : C_ROOF0));
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
        case T_VINES: {   // WFS-138 (RG-05, ANO-65) : rang de vigne palissé = bande continue de 0,5 m de large (0,4 m au sommet) et
            // 1,2 m de haut, posée sur le sol à chaque tronçon (1,2 m, ≤ 2 m) des deux côtés du rang : elle suit la pente et les
            // virages ; les blocs voisins partagent leurs bouts (même abscisse, même hauteur) : ni trou ni marche
            const double sA = (double)a, sB = sA + it.l / (float)SEG_M;
            const int n = (int)std::lround(sB - sA) + 1;
            const float wb = 0.25f, wt = 0.2f, x0 = s * it.lat, H = 1.2f;
            auto P = [&](double sg, float x, float y) { const TrackPoint q = t.point(sg * SEG, x); return vec3{(float)q.x, (float)q.y + y, (float)q.z}; };
            const vec2 c = uv(C_VINE);
            vec3 B0[2], T0[2];
            for (int i = 0; i < n; i++) {
                const double sg = sA + i;
                // sol lu au segment de l'échantillon (profil de CE segment) : le bout commun de deux blocs a la même hauteur
                const int64_t ag = (int64_t)sg;
                auto gyS = [&](float lat) { return solY(zone(ag), ag, it.side, lat); };
                const float yg = gyS(it.lat);
                vec3 B1[2], T1[2];
                for (int k = 0; k < 2; k++) {
                    const float sx = k ? 1.f : -1.f;
                    B1[k] = P(sg, x0 + sx * wb, gyS(it.lat + s * sx * wb) - 0.05f);
                    T1[k] = P(sg, x0 + sx * wt, yg + H);
                }
                if (i > 0) {   // ordre des sommets de box : normales vers l'extérieur
                    quad(S, T0[0], T0[1], T1[1], T1[0], c);
                    quad(S, B0[1], B1[1], T1[1], T0[1], c);
                    quad(S, B1[0], B0[0], T0[0], T1[0], c);
                }
                for (int k = 0; k < 2; k++) { B0[k] = B1[k]; T0[k] = T1[k]; }
            }
            break;
        }
        case T_FONTAINE: {   // îlot du rond-point (rayon 14 m) : fontaine de pierre Ø 6 m, vasque, colonne, massifs de fleurs
            const vec3 c = lateral(p, s * it.lat, gy(it.lat) - 0.05f);
            cyl(S, c, 3.0f, 0.5f, 20, uv(C_PIERRE0));                       // bassin : margelle de 0,5 m
            cyl(S, c + vec3{0, 0.42f, 0}, 2.75f, 0.1f, 20, uv(C_GLASS));     // eau
            cyl(S, c, 0.35f, 2.4f, 10, uv(C_PIERRE0 + 1));                  // colonne
            cyl(S, c + vec3{0, 2.2f, 0}, 1.0f, 0.25f, 14, uv(C_PIERRE0));   // vasque haute
            for (int i = 0; i < 16; i++) {   // massifs : couronne de fleurs à 5-7 m du centre (rouge, jaune, blanc, mauve)
                const float a = 2 * PI * i / 16, rr = 5.5f + (i % 2) * 1.2f;
                const int col = i % 4 == 0 ? C_RED : i % 4 == 1 ? C_HAY : i % 4 == 2 ? C_WHITE : C_FLAG0 + 2;
                box(S, c + vec3{std::cos(a) * rr, 0, std::sin(a) * rr}, p.r, p.f, 1.4f, 0.35f, 1.4f, uv(col));
            }
            break;
        }
        case T_FEU: {   // feu tricolore R11v : support de 3,2 m, boîtier 0,33 × 0,95 m (3 lampes Ø 200 mm), bas à 2,2 m
            const vec3 b = lateral(p, s * it.lat);
            cyl(S, b, 0.06f, 3.2f, 8, uv(C_POLE));
            box(S, b + vec3{0, 2.2f, 0} + p.f * 0.12f, p.r, p.f, 0.33f, 0.95f, 0.2f, uv(C_METAL));
            break;
        }
        case T_COMMERCE: {   // commerce du village : bâtiment en pierre, store, enseigne (texte de l'atlas), mobilier selon le métier
            const float W = it.w, D = it.d, Hh = it.h;
            const vec3 o = lateral(p, s * (it.lat + D / 2), gy(it.lat + D / 2) - 0.2f);
            const vec3 face = p.r * (-s), avant = o + face * (D / 2);   // façade côté route
            box(S, o, p.r, p.f, D, Hh, W, uv(it.k == CO_SUPERMARCHE || it.k == CO_GARAGE ? C_WALL2 : C_PIERRE0 + it.col % 3));
            if (it.k != CO_SUPERMARCHE && it.k != CO_GARAGE) toit(S, o + vec3{0, Hh + 0.9f, 0}, p.r, p.f, D * 1.05f, 1.8f, W * 1.05f, uv(C_ROOF0));
            for (int i = -1; i <= 1; i += 2)   // vitrines du rez-de-chaussée
                box(S, avant + face * 0.02f + p.f * (i * W * 0.22f) + vec3{0, 0.5f, 0}, p.r, p.f, 0.06f, 2.0f, W * 0.3f, uv(C_GLASS));
            const int store = it.k == CO_CAFE ? C_RED : it.k == CO_BAR ? C_GRASS : it.k == CO_BOULANGERIE ? C_HAY : C_METAL;
            if (it.k <= CO_BAR || it.k == CO_BOULANGERIE) {   // store banne de 2,5 m à 2,6 m de haut
                const vec3 a0 = avant + vec3{0, 3.0f, 0} - p.f * (W * 0.45f), a1 = avant + vec3{0, 3.0f, 0} + p.f * (W * 0.45f);
                quad(S, a0, a1, a1 + face * 2.5f - vec3{0, 0.5f, 0}, a0 + face * 2.5f - vec3{0, 0.5f, 0}, uv(store));
                quad(S, a1, a0, a0 + face * 2.5f - vec3{0, 0.5f, 0}, a1 + face * 2.5f - vec3{0, 0.5f, 0}, uv(store));
            }
            if (it.k <= CO_BAR) for (int i = 0; i < 4; i++) {   // terrasse : tables rondes Ø 0,7 m à 0,75 m, 2 chaises
                const vec3 t0 = avant + face * (1.6f + (i % 2) * 1.6f) + p.f * ((i / 2 - 0.5f) * W * 0.4f) + vec3{0, 0, 0};
                cyl(S, t0, 0.04f, 0.72f, 6, uv(C_METAL)); cyl(S, t0 + vec3{0, 0.72f, 0}, 0.35f, 0.03f, 10, uv(C_WHITE));
                for (int k = -1; k <= 1; k += 2) box(S, t0 + p.f * (k * 0.6f), p.r, p.f, 0.42f, 0.45f, 0.42f, uv(C_WOOD));
            }
            if (it.k == CO_GARAGE) {   // auvent de station (4,5 m) et 2 pompes (0,6 × 1,8 m)
                const vec3 st = avant + face * 5.f;
                for (int k = -1; k <= 1; k += 2) { box(S, st + p.f * (k * 2.f), p.r, p.f, 0.6f, 1.8f, 0.4f, uv(C_RED)); cyl(S, st + p.f * (k * 4.5f), 0.15f, 4.5f, 8, uv(C_STEEL)); }
                box(S, st + vec3{0, 4.5f, 0}, p.r, p.f, 5.f, 0.5f, 10.f, uv(C_WHITE));
            }
            if (it.k == CO_SUPERMARCHE) {   // parking : enrobé et places de 2,5 × 5 m (lignes de 12 cm)
                const vec3 pk = avant + face * 9.f + vec3{0, 0.03f, 0};
                quadSol(S, pk - p.f * 18.f - face * 7.f, pk + p.f * 18.f - face * 7.f, pk + p.f * 18.f + face * 7.f, pk - p.f * 18.f + face * 7.f, uv(C_PITLANE));
                for (int k = -7; k <= 7; k++) { const vec3 l = pk + p.f * (k * 2.5f) + vec3{0, 0.01f, 0};
                    quadSol(S, l - p.f * 0.06f - face * 7.f, l + p.f * 0.06f - face * 7.f, l + p.f * 0.06f - face * 2.f, l - p.f * 0.06f - face * 2.f, uv(C_WHITE)); }
            }
            const vie::Texte& T = txt_[it.k];
            if (T.ok) {   // enseigne : lettres de 0,5 m (1 m au supermarché) au-dessus de la porte, côté route
                const float hT = it.k == CO_SUPERMARCHE ? 1.0f : 0.5f, wT = std::min(W * 0.8f, hT * T.aspect);
                const vec3 c = avant + face * 0.08f + vec3{0, it.k == CO_SUPERMARCHE ? Hh - 1.2f : 3.6f, 0};
                box(S, c - face * 0.04f - vec3{0, hT * 0.6f, 0}, p.r, p.f, 0.06f, hT * 1.2f, wT + 0.4f, uv(it.k == CO_GARAGE ? C_RED : C_METAL));
                ligneTexte(out[P_TEXTE0], c, p.f * s, wT, wT / T.aspect, T);
            }
            break;
        }
        case T_AUTO: {
            // WFS-179 (RG-04) : chaussée opposée de 3 voies de 3,50 m (bande dérasée de gauche 1,00 m, bande d'arrêt d'urgence
            // 2,50 m : ICTAAL, autoroutes de liaison), marquage de 15 cm (2u, u = 7,5 cm sur autoroute), lignes T1 de 3 m pour
            // 10 m d'intervalle entre les voies (IISR, 7e partie, art. 113 et 115) ; séparateur en béton « New Jersey » (DBA :
            // 0,80 m de haut, 0,60 m à la base) au milieu du terre-plein central de 3 m. Côté gauche (s = −1).
            const Pose p1 = poseAt(t, a + 1);
            const float R = (float)RW, y = 0.022f;
            auto bande = [&](float l0, float l1, float f0, float f1) {   // bande peinte de l0 à l1 (m), de f0 à f1 (fraction du segment)
                const vec3 A0 = lateral(p, -l0, y), A1 = lateral(p, -l1, y), B0 = lateral(p1, -l0, y), B1 = lateral(p1, -l1, y);
                quadSol(S, A0 + (B0 - A0) * f0, A1 + (B1 - A1) * f0, A1 + (B1 - A1) * f1, A0 + (B0 - A0) * f1, uv(C_WHITE));
            };
            bande(R + 3.925f, R + 4.075f, 0, 1);    // rive côté terre-plein
            bande(R + 14.425f, R + 14.575f, 0, 1);  // rive côté bande d'arrêt d'urgence
            const double dist = (double)a * SEG_M, ph = std::fmod(dist, 13.0);
            const float f0 = ph < 3.0 ? 0.f : (float)((13.0 - ph) / SEG_M), f1 = ph < 3.0 ? (float)std::min(1.0, (3.0 - ph) / SEG_M) : 1.f;
            if (ph < 3.0 || ph + SEG_M > 13.0) for (float l : {R + 7.5f, R + 11.f}) bande(l - 0.075f, l + 0.075f, f0, f1);
            const float c = R + 1.5f;   // séparateur : profil extrudé du segment a au segment a + 1
            const vec2 dba[8] = {{c + 0.30f, 0}, {c + 0.30f, 0.075f}, {c + 0.20f, 0.25f}, {c + 0.09f, 0.80f},
                                 {c - 0.09f, 0.80f}, {c - 0.20f, 0.25f}, {c - 0.30f, 0.075f}, {c - 0.30f, 0}};
            for (int i = 0; i + 1 < 8; i++)
                quad(S, lateral(p, -dba[i].x, dba[i].y), lateral(p1, -dba[i].x, dba[i].y), lateral(p1, -dba[i + 1].x, dba[i + 1].y), lateral(p, -dba[i + 1].x, dba[i + 1].y), uv(C_WALK));
            break;
        }
        case T_PEAGE: {
            // WFS-179 (RG-04) : aire de péage (début de l'autoroute) : auvent de 14 m sur les deux chaussées, 5,50 m de hauteur
            // libre (≥ 4,75 m de gabarit routier + équipements), cabines de 1,4 × 2,6 m sur des îlots de 0,20 m aux bords de
            // chaque chaussée (rien sur nos voies), barrières levées (lisse rouge et blanche de 3 m, relevée), bandeau bleu
            // « PÉAGE », dalles LED sous l'auvent (lampes vertes des voies : animées, voir animes)
            const float R = (float)RW, g = -(R + 18.f), d = R + 3.f, cg = (g + d) / 2, larg = d - g;
            box(S, lateral(p, cg, 0), p.r, p.f, larg, 1.2f, 14.f, uv(C_WALL), 5.5f);                     // auvent
            box(S, lateral(p, cg, 0) - p.f * 7.05f, p.r, p.f, larg, 0.9f, 0.1f, uv(C_SEAT0), 5.65f);      // bandeau bleu (face aux arrivants)
            box(S, lateral(p, cg, 0) + p.f * 7.05f, p.r, p.f, larg, 0.9f, 0.1f, uv(C_SEAT0), 5.65f);      // et face au sens opposé
            for (float lat : {R + 2.3f, -(R + 1.5f), -(R + 17.8f)}) for (float al : {-5.f, 5.f})
                box(S, lateral(p, lat, 0) + p.f * al, p.r, p.f, 0.5f, 5.5f, 0.5f, uv(C_WALK));             // piliers
            if (!lod) for (float lat : {-(R + 6.f), -(R + 12.f), -1.5f, 4.f})                              // dalles LED (4000 K)
                for (float al : {-4.f, 0.f, 4.f}) box(out[P_NEON], lateral(p, lat, 0) + p.f * al, p.r, p.f, 1.2f, 0.06f, 0.6f, uv(C_LAMP_W), 5.44f);
            struct Cab { float lat, dir; };   // cabine, côté où passe la voiture (dir : +1 vers la droite)
            for (const Cab& cb : {Cab{R + 2.2f, -1.f}, Cab{-(R + 1.5f), 1.f}, Cab{-(R + 17.6f), 1.f}}) {
                box(S, lateral(p, cb.lat, 0), p.r, p.f, 1.8f, 0.2f, 20.f, uv(C_WALK));                    // îlot
                box(S, lateral(p, cb.lat, 0.2f), p.r, p.f, 1.4f, 2.6f, 2.6f, uv(C_WALL));                  // cabine
                box(S, lateral(p, cb.lat, 1.2f), p.r, p.f, 1.42f, 0.9f, 2.0f, uv(C_GLASS));                // vitrages
                const vec3 pied = lateral(p, cb.lat + cb.dir * 0.8f, 0.2f) - p.f * 1.8f;
                box(S, pied, p.r, p.f, 0.3f, 1.0f, 0.3f, uv(C_STEEL));                                     // poteau de la barrière
                for (int k = 0; k < 6; k++) box(S, pied + vec3{0, 1.0f + k * 0.5f, 0}, p.r, p.f, 0.08f, 0.5f, 0.08f, uv(k % 2 ? C_WHITE : C_RED));   // lisse levée
            }
            if (txtA_[3].ok) for (float sf : {-1.f, 1.f}) {   // « PÉAGE » en lettres blanches de 0,6 m sur les deux bandeaux
                const vec3 c = lateral(p, R - 2.f, 6.1f) + p.f * (sf * 7.12f);
                ligneTexte(out[P_TEXTE0], c, p.r * (-sf), 0.6f * txtA_[3].aspect / 0.62f, 0.6f / 0.62f, txtA_[3]);
            }
            break;
        }
        case T_PANNEAU_A: {
            // WFS-179 : panneau bleu d'autoroute (D 21 / D 29 : fond bleu, lettres blanches ; 3,6 × 1,6 m, bas à 2,2 m), sur deux
            // poteaux d'acier galvanisé, sur l'accotement de droite derrière la glissière
            const float x = s * it.lat;
            for (float al : {-1.2f, 1.2f}) box(S, lateral(p, x, 0) + p.f * al, p.r, p.f, 0.1f, 3.8f, 0.1f, uv(C_STEEL));
            box(S, lateral(p, x, 0) - p.f * 0.08f, p.r, p.f, 3.6f, 1.6f, 0.08f, uv(C_SEAT0), 2.2f);
            const vie::Texte& T = txtA_[std::clamp(it.k, 0, 2)];
            if (T.ok) {
                const float h = 0.45f / 0.62f, w = std::min(3.3f, h * T.aspect);
                ligneTexte(out[P_TEXTE0], lateral(p, x, 3.0f) - p.f * 0.13f, p.r, w, w / T.aspect, T);
            }
            break;
        }
        case T_GLISS: {   // glissière simple (GS2) : lisse en acier galvanisé de 0,31 m, haut à 0,70 m, poteaux tous les 2,4 m
            const Pose p1 = poseAt(t, a + 2);
            const vec3 A = lateral(p, s * it.lat, 0.39f), B = lateral(p1, s * it.lat, 0.39f), up{0, 0.31f, 0};
            const vec3 n = p.r * (-s * 0.03f);   // lisse : 3 cm d'épaisseur vers la route
            quad(S, A + n, B + n, B + n + up, A + n + up, uv(C_STEEL));
            quad(S, B, A, A + up, B + up, uv(C_STEEL));
            box(S, lateral(p, s * (it.lat + 0.08f)), p.r, p.f, 0.1f, 0.7f, 0.1f, uv(C_STEEL));
            break;
        }
        case T_MURET: {   // parapet de pierre dans l'épingle : 0,5 m de haut (≥ 0,45 m réglementaire de retenue), 0,4 m d'épaisseur
            const Pose p1 = poseAt(t, a + 1);
            const float x = s * it.lat, e = s * 0.4f;
            const vec3 a0 = lateral(p, x), a1 = lateral(p, x + e), b0 = lateral(p1, x), b1 = lateral(p1, x + e), up{0, 0.5f, 0};
            quad(S, a0 + up, b0 + up, b1 + up, a1 + up, uv(C_WALK));
            if (s > 0) { quad(S, a0, b0, b0 + up, a0 + up, uv(C_WALK)); quad(S, b1, a1, a1 + up, b1 + up, uv(C_WALK)); }
            else { quad(S, b0, a0, a0 + up, b0 + up, uv(C_WALK)); quad(S, a1, b1, b1 + up, a1 + up, uv(C_WALK)); }
            break;
        }
        case T_DANGER: {   // panneau A1 (virage dangereux) en rase campagne : triangle de 1 m, bord rouge, bas à 1,5 m, face aux arrivants
            const vec3 b = lateral(p, s * it.lat);
            cyl(S, b, 0.04f, 2.4f, 6, uv(C_STEEL));
            const vec3 c = b + vec3{0, 2.0f, 0} - p.f * 0.06f, rr = p.r * 0.5f, up{0, 0.866f, 0};
            tri(S, c - rr - up * 0.33f, c + rr - up * 0.33f, c + up * 0.67f, uv(C_SIGNR));   // bord rouge (côté des arrivants : −f)
            tri(S, c - rr * 0.7f - up * 0.23f - p.f * 0.01f, c + rr * 0.7f - up * 0.23f - p.f * 0.01f, c + up * 0.47f - p.f * 0.01f, uv(C_SIGNW));
            tri(S, c + rr - up * 0.33f + p.f * 0.02f, c - rr - up * 0.33f + p.f * 0.02f, c + up * 0.67f + p.f * 0.02f, uv(C_METAL));   // dos
            break;
        }
        case T_CASCADE: {   // cascade : paroi de roche de 16 m, chute d'eau de 3 m de large, vasque au pied (près du pont)
            const vec3 b = lateral(p, s * it.lat, gy(it.lat) - 0.3f);
            box(S, b + p.r * (s * 4.f), p.r, p.f, 6, 16, 22, uv(C_WALK));
            const vec3 face = b + p.r * (s * 0.98f), up{0, 15.5f, 0}, w = p.f * 1.5f;
            if (s > 0) quad(S, face - w, face + w, face + w + up, face - w + up, uv(C_WHITE));
            else quad(S, face + w, face - w, face - w + up, face + w + up, uv(C_WHITE));
            const vec3 v = b + vec3{0, 0.05f, 0};
            quadSol(S, v - p.f * 4.f, v + p.f * 4.f, v + p.f * 4.f - p.r * (s * 6.f), v - p.f * 4.f - p.r * (s * 6.f), uv(C_GLASS));
            break;
        }
        case T_ENCLOS: {   // WFS-186 : enclos des chevaux : poteaux de 1,3 m tous les 2,5 m, deux lisses (0,6 et 1,1 m)
            const vec3 c = lateral(p, s * it.lat), ex = p.r * s, ez = p.f;   // centre ; largeur it.w selon r, longueur it.l selon f
            const float W = it.w / 2, Lg = it.l / 2;
            const vec3 coins[4] = {c - ex * W - ez * Lg, c + ex * W - ez * Lg, c + ex * W + ez * Lg, c - ex * W + ez * Lg};
            for (int k = 0; k < 4; k++) {
                const vec3 A = coins[k], B = coins[(k + 1) % 4], d = B - A;
                const float L = length(d); const vec3 u = d * (1.f / L), lat = cross(vec3{0, 1, 0}, u);
                const int np = std::max(1, (int)std::ceil(L / 2.5f));
                for (int q = 0; q < np; q++) {
                    const vec3 P = A + u * (L * q / np);
                    box(S, P + vec3{0, gy(it.lat + dot(P - c, ex)) - 0.05f, 0}, lat, u, 0.12f, 1.35f, 0.12f, uv(C_WOOD));
                }
                const vec3 m = (A + B) * 0.5f;
                for (float y : {0.6f, 1.1f}) box(S, m + vec3{0, gy(it.lat + dot(m - c, ex)) + y, 0}, lat, u, 0.06f, 0.1f, L, uv(C_WOOD));
            }
            break;
        }
        default: break;
        }
    }

    // pales des éoliennes : objet séparé qui tourne (rotors de scene.js), un maillage par éolienne
    int animes(Renderer& r, const GameView& g, int64_t c0, int64_t c1) override {
        int nS = 0;
        if (!solFond_.indexCount) construireSolFond(r);
        if (solFond_.indexCount) { r.draw(solFond_, mats_[P_UNI], mat4::identity()); nS++; }   // WFS-178 : un appel
        if (!sommets_.empty() && !sommetsMesh_.indexCount) construireSommets(r);
        if (sommetsMesh_.indexCount) { r.draw(sommetsMesh_, mats_[P_UNI], mat4::identity()); nS = 1; }   // WFS-138 : un appel
        if (feuSeg_ >= 0 && lampeMesh_.vb.buf) {   // WFS-178 : lampes du feu selon feuA (rouge en haut, orange, vert en bas)
            int64_t a;
            if (dansMorceau(feuSeg_, c0 * CH, (c1 + 1) * CH, a)) {
                const Pose p = poseAt(*g.track, a);
                const trace::Feu f = trace::feuA(feuIdx_);
                const int allume = f == trace::Feu::Rouge ? 0 : f == trace::Feu::Orange ? 1 : f == trace::Feu::Vert ? 2 : -1;
                for (int side : {-1, 1}) for (int k = 0; k < 3; k++) {
                    const vec3 c = lateral(p, side * ((float)RW + 0.8f), 2.98f - k * 0.3f) + p.f * 0.01f;   // face aux arrivants (−f)
                    r.draw(lampeMesh_, k == allume ? lampeOn_[k] : lampeOff_[k], translate(c) * rotateY(-p.th + PI));
                    nS++;
                }
            }
        }
        nS += autoroutesAnimes(r, g, c0, c1);   // WFS-179
        nS += animauxAnimes(r, g, c0, c1);      // WFS-186
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
        return n + nS;
    }

    // WFS-179 : lampes vertes d'affectation au-dessus de chaque voie du péage (Ø 200 mm, face aux arrivants des deux sens) ;
    // voitures en sens inverse sur la chaussée opposée (3 voies, 94 à 119 km/h, la voie de gauche la plus rapide), à horaire
    // fixe, 2 au plus dans le champ (−30 à 280 m), cachées à moins de 40 m du péage (elles y ralentiraient) ; feux la nuit
    int autoroutesAnimes(Renderer& r, const GameView& g, int64_t c0, int64_t c1) {
        if (auDeb_ < 0) return 0;
        int n = 0;
        Track& t = *g.track;
        const float R = (float)RW;
        int64_t a;
        if (lampeMesh_.vb.buf && dansMorceau(auDeb_ + PEAGE_SEG, c0 * CH, (c1 + 1) * CH, a)) {
            const Pose p = poseAt(t, a);
            for (float lat : {-5.6f, -1.9f, 1.9f, 5.6f}) { r.draw(lampeMesh_, lampeOn_[2], translate(lateral(p, lat, 5.0f) - p.f * 7.12f) * rotateY(-p.th + PI)); n++; }
            for (float lat : {R + 5.75f, R + 9.25f, R + 12.75f}) { r.draw(lampeMesh_, lampeOn_[2], translate(lateral(p, -lat, 5.0f) + p.f * 7.12f) * rotateY(-p.th)); n++; }
        }
        if (!autos_[0].ok) return n;
        const int64_t L = len(), aP = (int64_t)std::floor(g.player->dist / SEG);
        const double lon = (double)(auFin_ - auDeb_) * SEG_M;
        int k = 0;
        for (int i = 0; i < 18 && k < 2; i++) {
            const int vi = i % 3;
            const double v = vi == 0 ? 33.0 : vi == 1 ? 30.0 : 26.0;
            const double m = lon - std::fmod(i * 83.0 + 29.0 * vi + v * g.time, lon);
            const int64_t sl = auDeb_ + (int64_t)(m / SEG_M);
            if (std::llabs(sl - (auDeb_ + PEAGE_SEG)) * SEG_M < 40) continue;
            int64_t sA = aP - (((aP % L) + L) % L) + sl;
            if (sA - aP > L / 2) sA -= L; else if (aP - sA > L / 2) sA += L;
            const double sF = (double)sA + std::fmod(m, SEG_M) / SEG_M, dz = (sF - g.player->dist / SEG) * SEG_M;
            if (dz < -30 || dz > 280) continue;
            const float lat = -(R + 5.75f + 3.5f * vi);
            t.pose((int64_t)sF - 1); t.pose((int64_t)sF + 2);
            const TrackPoint q = t.point(sF * SEG, lat);
            const Model& mo = autos_[i % 2];
            const mat4 w = translate({(float)q.x, (float)q.y + 0.01f, (float)q.z}) * rotateY((float)-(q.th + PI)) * rotateX((float)q.slope);
            const vec3 paint = linHex(TRAFFIC_PAINTS[(i * 5 + 2) % 9]);
            const RouesPose rp = roues::poseTrafic(0, false, v * g.time / M, mo.nRoues ? mo.roues[0].rayon : roues::RW_DEFAUT);
            FeuxVoiture f; f.nuit = night_;
            r.feuxProchaineVoiture(f);
            r.draw(mo, w, &paint, &rp);
            k++; n += (int)mo.parts.size();
        }
        return n;
    }

    void shutdownCarte(Renderer& r) override {
        for (Mesh& m : rotors_) r.destroy(m);
        rotors_.clear(); turbines_.clear();
        if (sommetsMesh_.vb.buf) r.destroy(sommetsMesh_);   // WFS-138
        sommetsMesh_ = Mesh{}; sommets_.clear(); libre_.clear(); aCol_ = aCascade_ = false;
        atlas_.detruire(r); for (auto& T : txt_) T = {};   // WFS-178
        if (lampeMesh_.vb.buf) r.destroy(lampeMesh_);
        for (int k = 0; k < 3; k++) { r.destroy(lampeOn_[k]); r.destroy(lampeOff_[k]); }   // WFS-187 (accroche) : jeux de descripteurs rendus
        lampeMesh_ = Mesh{}; fontaineSeg_ = feuSeg_ = -1; vigilance_.clear(); commerces_.clear();
        for (Model& m : autos_) if (m.ok) { r.destroy(m); m = Model{}; }   // WFS-179
        for (auto& esp : betesMesh_) for (auto& robe : esp) { for (Mesh& m : robe) r.destroy(m); robe.clear(); }   // WFS-186
        betes_.clear();
        for (auto& T : txtA_) T = {};
        auDeb_ = -2; auFin_ = -1;
        if (solFond_.vb.buf) r.destroy(solFond_);
        solFond_ = Mesh{};
    }

private:
    // WFS-138 (RG-C1) : espace libre de chaque côté de chaque segment : m = mi-distance (m depuis l'axe) jusqu'à la route
    // voisine la plus proche dans un cône de 45° autour du côté, h = mi-écart de hauteur (m, + : la voisine est plus haute)
    struct Libre { float m = std::numeric_limits<float>::infinity(), h = 0; };
    // WFS-138 (RG-C1) : profil de la zone coupé à la mi-distance de la route voisine (lacets du col, épingles, rond-point),
    // hauteur corrigée en rampe depuis le bord de la chaussée (correction 0) jusqu'à la coupe (sol = mi-hauteur des deux
    // routes) : le sol rejoint exactement celui de la voisine et ne recouvre jamais sa chaussée ; pente forte vers le haut :
    // talus de roche (paroi côté montagne)
    // WFS-179 : tronçon d'autoroute du tracé 2 (sections autoroute contiguës), aire de péage à 30 m de son début
    static constexpr int64_t PEAGE_SEG = 25;
    void calculerAutoroute() {
        auDeb_ = auFin_ = -1;
        for (const trace::Section& sec : trace::sections(*track_)) if (sec.autoroute) {
            if (auDeb_ < 0) auDeb_ = sec.debut;
            auFin_ = sec.fin;
        }
        if (auDeb_ >= 0) for (int k = 0; k < 2; k++)   // nuit : lumière sous l'auvent (nuit.cpp, les 4 plus proches)
            lampadaires().push_back({auDeb_ + PEAGE_SEG, k ? -((float)RW + 9.f) : 2.f, 5.4f, StyleLampadaire::Ville});
        WFS_LOG("Campagne (WFS-179) : autoroute %lld-%lld, péage au segment %lld", (long long)auDeb_, (long long)auFin_, (long long)(auDeb_ + PEAGE_SEG));
    }
    bool autoroute(int64_t n) const {
        if (auDeb_ < 0) return false;
        const int64_t L = len(), m = ((n % L) + L) % L;
        return m >= auDeb_ && m < auFin_;
    }
    bool aPeage(int64_t n) const {   // ±15 m autour de l'aire de péage
        if (auDeb_ < 0) return false;
        const int64_t L = len(), m = ((n % L) + L) % L;
        return std::llabs(m - (auDeb_ + PEAGE_SEG)) * SEG_M < 15;
    }

    int borner(int64_t a, int s, Band out[6], int n) const {
        const Libre* l = libreA(a, s);
        if (!l || l->m >= out[n - 1].b) return n;
        const float R = (float)RW, m = std::max(l->m, out[0].b + 0.5f);
        int k = 0;
        while (k < n && out[k].b < m) k++;
        if (k == n) return n;
        out[k].yb = out[k].ya + (out[k].yb - out[k].ya) * (m - out[k].a) / (out[k].b - out[k].a); out[k].b = m;
        n = k + 1;
        const float corr = l->h - out[k].yb, pente = corr / (m - R);
        for (int i = 0; i < n; i++) { out[i].ya += corr * (out[i].a - R) / (m - R); out[i].yb += corr * (out[i].b - R) / (m - R); }
        // plus de 24° vers une route nettement plus haute (col) : la roche affleure (gris 0x8a8884, albédo ≈ 0,25) ;
        // en terrain plat (rond-point, village), la rampe ne fait que rattraper le profil : l'herbe reste
        if (pente > 0.45f && l->h > 2.5f) for (int i = 1; i < n; i++) out[i].col = C_WALK;
        return n;
    }
    std::vector<Libre> libre_;   // 2 par segment : [2a] gauche, [2a + 1] droite
    const Libre* libreA(int64_t a, int side) const {
        if (libre_.empty()) return nullptr;
        const int64_t L = len(), k = ((a % L) + L) % L;
        const Libre& l = libre_[(size_t)(2 * k + (side > 0))];
        return std::isfinite(l.m) ? &l : nullptr;
    }
    void calculerLibre() {
        Track& t = *track_;
        const int64_t L = len();
        const float R = (float)RW, PORTEE = 2 * (R + 220);   // au-delà, le profil normal (220 m) ne peut rien toucher
        std::vector<vec3> P((size_t)L);
        std::vector<Pose> Q((size_t)L);
        for (int64_t a = 0; a < L; a++) { Q[(size_t)a] = poseAt(t, a); P[(size_t)a] = Q[(size_t)a].o; }
        const float CELL = 40;
        auto cle = [&](float x, float z) { return ((int64_t)std::floor(x / CELL) << 32) ^ ((int64_t)std::floor(z / CELL) & 0xffffffff); };
        std::unordered_map<int64_t, std::vector<int64_t>> grille;
        for (int64_t a = 0; a < L; a++) grille[cle(P[(size_t)a].x, P[(size_t)a].z)].push_back(a);
        libre_.assign((size_t)(2 * L), Libre{});
        const int NC = (int)std::ceil(PORTEE / CELL);
        int limites = 0;
        for (int64_t a = 0; a < L; a++) {
            const vec3 o = P[(size_t)a]; const Pose& q = Q[(size_t)a];
            const int64_t cx = (int64_t)std::floor(o.x / CELL), cz = (int64_t)std::floor(o.z / CELL);
            for (int side = 0; side < 2; side++) {
                const float sg = side ? 1.f : -1.f;
                float best = PORTEE, dy = 0;
                for (int ix = -NC; ix <= NC; ix++) for (int iz = -NC; iz <= NC; iz++) {
                    const auto it = grille.find(((cx + ix) << 32) ^ ((cz + iz) & 0xffffffff));
                    if (it == grille.end()) continue;
                    for (int64_t b : it->second) {
                        int64_t da = std::llabs(b - a); da = std::min(da, L - da);
                        if (da < 12) continue;   // la route elle-même (≈ 14 m de part et d'autre)
                        const vec3 d = P[(size_t)b] - o;
                        const float proj = (d.x * q.r.x + d.z * q.r.z) * sg, along = d.x * q.f.x + d.z * q.f.z;
                        if (proj <= 0 || std::fabs(along) > proj || proj >= best) continue;
                        best = proj; dy = d.y;
                    }
                }
                if (best < PORTEE) { libre_[(size_t)(2 * a + side)] = {best / 2, dy / 2}; limites++; }
            }
        }
        // lissage sur ±4 segments (minimum de la distance) : bord du sol sans marches d'un segment à l'autre
        std::vector<Libre> lisse = libre_;
        for (int64_t a = 0; a < L; a++) for (int side = 0; side < 2; side++) {
            Libre& d = lisse[(size_t)(2 * a + side)];
            for (int k = -4; k <= 4; k++) {
                const Libre& v = libre_[(size_t)(2 * (((a + k) % L + L) % L) + side)];
                if (v.m < d.m) { d.m = v.m; d.h = v.h; }
            }
        }
        libre_ = lisse;
        WFS_LOG("campagne : espace libre calculé (%lld segments, %d côtés bornés par une route voisine)", (long long)L, limites);
    }

    // WFS-138 (RG-01) : altitude (m) d'un segment au-dessus du pied du col (début des lacets)
    float altitude(int64_t n) const {
        if (!aCol_) return 0;
        const int64_t L = len();
        return (float)track_->point((double)(((n % L) + L) % L) * SEG, 0).y - yPied_;
    }
    // décor du col (RG-01), posé une fois : glissières sur les branches et murets dans les épingles côté vide, panneaux A1
    // 70 m avant chaque épingle, cascade au pont ; sommets du fond (maillage à part, animes)
    void placerCol() {
        Track& t = *track_;
        const float R = (float)RW;
        for (const trace::Section& sec : t.sections()) {
            if (sec.type == trace::Type::Pont && !aCascade_) {   // cascade du côté libre le plus large, au milieu du pont
                const int64_t m = (sec.debut + sec.fin) / 2;
                const Libre* g = libreA(m, -1); const Libre* d = libreA(m, 1);
                const int side = (!d || (g && g->m < d->m)) ? 1 : -1;
                const Libre* l = libreA(m, side);
                if (!l || l->m > R + 36) { Item it; it.t = T_CASCADE; it.side = side; it.lat = R + 14; CarteDecor::item(m, it); aCascade_ = true; cascadeSeg_ = m; cascadeCote_ = side;
                    WFS_LOG("campagne : cascade au pont (segment %lld, côté %s)", (long long)m, side > 0 ? "droit" : "gauche"); }
            }
            if (sec.type == trace::Type::RondPoint && fontaineSeg_ < 0) {   // centre de l'anneau : à gauche du milieu
                fontaineSeg_ = (sec.debut + sec.fin) / 2;
                Item it; it.t = T_FONTAINE; it.side = sec.cote > 0 ? -1 : 1; it.lat = (float)sec.rayon; CarteDecor::item(fontaineSeg_, it);
                vigilance_.push_back(sec.debut);
                // supermarché et parking à l'entrée du village (180 m avant le rond-point), garage 60 m après
                commerce(sec.debut - 150, 1, CO_SUPERMARCHE, 34, 22, 6.5f, 12);
                commerce(sec.fin + 50, 1, CO_GARAGE, 14, 10, 5.f, 9);
                commerce(sec.fin + 100, -1, CO_BOULANGERIE, 9, 9, 6.f, 4);
            }
            if (sec.type == trace::Type::Feux && feuSeg_ < 0 && !sec.depart) {   // feu du village : un poteau de chaque côté
                feuSeg_ = sec.ligneArret; feuIdx_ = sec.feu; vigilance_.push_back(sec.ligneArret);
                for (int side : {-1, 1}) { Item it; it.t = T_FEU; it.side = side; it.lat = R + 0.8f; CarteDecor::item(feuSeg_, it); }
                // place du village : café-terrasse et bar face à face, juste après le carrefour
                commerce(sec.ligneArret + 20, 1, CO_CAFE, 11, 9, 7.f, 6);
                commerce(sec.ligneArret + 24, -1, CO_BAR, 10, 9, 7.f, 6);
            }
            if (sec.type != trace::Type::Lacets) continue;
            aCol_ = true; yPied_ = (float)t.point((double)sec.debut * SEG, 0).y;
            int64_t debutEp = -1, nEp = 0;
            for (int64_t a = sec.debut; a < sec.fin; a++) {
                const bool ep = std::abs(t.segAt(a).curve) > 10;
                if (ep && debutEp < 0) {   // nouvelle épingle : panneau A1 70 m avant, à droite (sens de circulation)
                    debutEp = a; nEp++;
                    Item it; it.t = T_DANGER; it.side = 1; it.lat = R + 1.2f; it.petit = true; CarteDecor::item(a - (int64_t)(70 / SEG_M), it);
                }
                if (!ep) debutEp = -1;
                for (int side : {-1, 1}) {
                    const Libre* l = libreA(a, side);
                    if (l && l->h > 1.5f) continue;   // côté montagne : talus de roche (profil)
                    if (ep) { Item it; it.t = T_MURET; it.side = side; it.lat = R + 0.5f; item(a, it); }
                    else if (a % 2 == 0) { Item it; it.t = T_GLISS; it.side = side; it.lat = R + 0.6f; it.petit = true; CarteDecor::item(a, it); }
                }
            }
            WFS_LOG("campagne : col de %.0f m, %lld épingles balisées, pied à %.0f m", sec.denivele, (long long)nEp, yPied_);
            placerSommets(sec);
        }
    }
    // sommets du fond : au-delà de la dernière branche des lacets, du côté où le col monte, à plus de 400 m de toute route ;
    // 2 enneigés (neige au-dessus de 70 % de la hauteur, albédo 0,80 : 0xe8e8e8)
    struct Sommet { vec3 c; float r, h; bool neige; };
    std::vector<Sommet> sommets_;
    Mesh sommetsMesh_{};
    void placerSommets(const trace::Section& sec) {
        Track& t = *track_;
        const int64_t L = len();
        const vec3 a0 = poseAt(t, sec.debut).o, a1 = poseAt(t, sec.fin).o;
        vec3 U = a1 - a0; U.y = 0; const float lu = std::sqrt(U.x * U.x + U.z * U.z); if (lu < 1) return; U = U * (1 / lu);
        const vec3 V{-U.z, 0, U.x};
        auto loinDesRoutes = [&](vec3 c, float r) {
            for (int64_t a = 0; a < L; a += 4) { const vec3 o = poseAt(t, a).o; const float dx = o.x - c.x, dz = o.z - c.z; if (dx * dx + dz * dz < r * r) return false; }
            return true;
        };
        Mulberry32 D(map().seed * 13 + 5);
        const float base = a1.y - 40;
        for (int k = 0; k < 60 && sommets_.size() < 5; k++) {   // candidats en éventail, de plus en plus loin
            const float r = 220 + (float)D.next() * 160, h = 220 + (float)D.next() * 200, ang = ((float)D.next() - 0.5f) * 2.4f;
            const float dist = r + 350 + k * 40.f;
            const vec3 c = a1 + U * (std::cos(ang) * dist) + V * (std::sin(ang) * dist);
            if (!loinDesRoutes(c, r + 250)) continue;
            bool loinDesAutres = true;
            for (const Sommet& o : sommets_) { const float dx = o.c.x - c.x, dz = o.c.z - c.z; if (dx * dx + dz * dz < (o.r + r) * (o.r + r) * 0.5f) loinDesAutres = false; }
            if (!loinDesAutres) continue;
            sommets_.push_back({{c.x, base, c.z}, r, h, false});
        }
        // les 2 plus hauts sont enneigés
        std::vector<size_t> idx(sommets_.size()); for (size_t i = 0; i < idx.size(); i++) idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](size_t x, size_t y) { return sommets_[x].h > sommets_[y].h; });
        for (size_t i = 0; i < idx.size() && i < 2; i++) sommets_[idx[i]].neige = true;
        WFS_LOG("campagne : %zu sommets en fond du col (2 enneigés)", sommets_.size());
    }
    void construireSommets(Renderer& r) {
        Geo G;
        Mulberry32 D(map().seed * 17 + 3);
        for (const Sommet& m : sommets_) {   // tronc de cône irrégulier : forêt jusqu'à 35 %, roche, neige au-dessus de 70 %
            const int N = 14;
            float rr[N]; for (int i = 0; i < N; i++) rr[i] = 0.75f + 0.5f * (float)D.next();
            const float paliers[4] = {0, 0.35f, 0.7f, 1};
            for (int e = 0; e < 3; e++) {
                const int col = e == 0 ? C_FOREST : e == 1 || !m.neige ? C_WALK : C_WHITE;
                const float t0 = paliers[e], t1 = paliers[e + 1];
                for (int i = 0; i < N; i++) {
                    const float a0 = 2 * PI * i / N, a1 = 2 * PI * (i + 1) / N;
                    auto P = [&](float ang, float tt, float k) { const float rad = m.r * (1 - tt) * k; return vec3{m.c.x + std::cos(ang) * rad, m.c.y + m.h * tt, m.c.z + std::sin(ang) * rad}; };
                    const float k0 = rr[i], k1 = rr[(i + 1) % N];
                    if (t1 >= 1) tri(G, P(a1, t0, k1), P(a0, t0, k0), vec3{m.c.x, m.c.y + m.h, m.c.z}, uv(col));
                    else quad(G, P(a1, t0, k1), P(a0, t0, k0), P(a0, t1, k0), P(a1, t1, k1), uv(col));
                }
            }
        }
        if (!G.i.empty()) sommetsMesh_ = r.makeMesh(G.v, G.i);
    }
    // WFS-178 : commerce du village au segment n, côté side, en retrait de « recul » m du bord de la chaussée
    void commerce(int64_t n, int side, int k, float w, float d, float h, float recul) {
        Item it; it.t = T_COMMERCE; it.side = side; it.k = k; it.w = w; it.d = d; it.h = h; it.lat = (float)RW + recul; it.col = k;
        const Libre* l = libreA(n, side);
        if (l && it.lat + d + 2 > l->m) { WFS_LOG("campagne : %s non posé (route voisine)", ENSEIGNES[k]); return; }
        CarteDecor::item(n, it);
        commerces_.push_back({n, side, (int64_t)std::ceil((w / 2 + 4) / SEG_M) + (k == CO_SUPERMARCHE ? 15 : 0)});   // + parking
        WFS_LOG("campagne : %s au segment %lld, côté %s", ENSEIGNES[k], (long long)n, side > 0 ? "droit" : "gauche");
    }
    int64_t fontaineSeg_ = -1, feuSeg_ = -1; int feuIdx_ = 0;
    std::vector<int64_t> vigilance_;
    struct Emprise { int64_t seg; int side; int64_t demi; };   // segments occupés par un commerce (± demi)
    std::vector<Emprise> commerces_;   // RG-C1 : segments à dégager dans les 60 m qui précèdent (feu, entrée du rond-point)
    vie::AtlasTextes atlas_; vie::Texte txt_[CO_NB];
    Mesh lampeMesh_{}; Material lampeOn_[3]{}, lampeOff_[3]{};
    // WFS-179 : autoroute (segments du tour, −2 : pas encore calculé), voitures en sens inverse, textes des panneaux
    int64_t auDeb_ = -2, auFin_ = -1;
    Model autos_[2];
    vie::Texte txtA_[4];
    // WFS-178 : sol de fond sous toute la carte (prairie, 1 m sous le point le plus bas de la route) : depuis que le sol
    // de chaque route s'arrête à mi-distance de sa voisine (WFS-138), il restait des trous entre deux bandes qui ne se
    // rejoignent pas (abords du rond-point, intérieur des épingles) : on y voyait le ciel ; on y voit maintenant le sol
    Mesh solFond_{};
    void construireSolFond(Renderer& r) {
        Track& t = *track_;
        const int64_t L = len();
        float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f, y0 = 1e9f;
        for (int64_t a = 0; a < L; a += 4) { const vec3 o = poseAt(t, a).o; x0 = std::min(x0, o.x); x1 = std::max(x1, o.x); z0 = std::min(z0, o.z); z1 = std::max(z1, o.z); y0 = std::min(y0, o.y); }
        const float m = 1500, y = y0 - 1.0f;
        Geo G; quadSol(G, {x0 - m, y, z0 - m}, {x1 + m, y, z0 - m}, {x1 + m, y, z1 + m}, {x0 - m, y, z1 + m}, uv(C_FARHILL));
        solFond_ = r.makeMesh(G.v, G.i);
    }
    bool aCol_ = false, aCascade_ = false;
    int64_t cascadeSeg_ = -1; int cascadeCote_ = 0;
    float yPied_ = 0;

    // ==================================================================================================================
    // WFS-186 (RG-02, RG-C4 de WFS-138) : animaux de ferme (vaches, moutons, chevaux) en vrais modèles (convertir_ferme.py)
    // ==================================================================================================================
    struct Obst { int64_t n0, n1; int side; float lat0, lat1; };
    enum { VACHE, CHEVAL, MOUTON, NB_ESPECES };
    struct Bete { int64_t seg; int side; float lat, along, yaw, ech, phase; int8_t espece, robe; };
    static constexpr int NB_ROBES[NB_ESPECES] = {3, 2, 2};
    std::vector<Bete> betes_;
    int numBloc_ = 0;
    std::vector<Mesh> betesMesh_[NB_ESPECES][3];   // [espèce][robe] : 5 poses (4 phases du broutage, repos)

    // poses figées .wfa (même lecture que la Ville et la Plage) ; teinte de chaque matériau d'origine : case de la palette
    bool chargerWfa(Renderer& r, const char* chemin, const int* couleurs, int nCouleurs, std::vector<Mesh>& poses) {
        std::vector<uint8_t> d;
        if (!readAsset(chemin, d) || d.size() < 12 || std::memcmp(d.data(), "WFA1", 4)) { WFS_ERR("Animal illisible : %s", chemin); return false; }
        uint32_t np = 0, nv = 0; std::memcpy(&np, &d[4], 4); std::memcpy(&nv, &d[8], 4);
        if (np == 0 || np > 32 || nv == 0 || nv > 200000 || nv % 3 || d.size() < 12 + (size_t)nv + (size_t)np * nv * 24) { WFS_ERR("Animal invalide : %s", chemin); return false; }
        std::vector<uint32_t> idx(nv);
        for (uint32_t i = 0; i < nv; i++) idx[i] = i;
        for (uint32_t q = 0; q < np; q++) {
            std::vector<Vertex> v(nv);
            const uint8_t* src = d.data() + 12 + nv + (size_t)q * nv * 24;
            for (uint32_t i = 0; i < nv; i++) {
                float f[6]; std::memcpy(f, src + (size_t)i * 24, 24);
                const vec2 t = uv(couleurs[std::min<int>(d[12 + i], nCouleurs - 1)]);
                v[i] = {f[0], f[1], f[2], f[3], f[4], f[5], t.x, t.y};
            }
            poses.push_back(r.makeMesh(v, idx, false));
        }
        return true;
    }
    // robes réelles (cases de la palette commune ; codes = matériaux d'origine, dans l'ordre du .gltf) :
    //   vache (Main, Main_Light, Hooves, Muzzle, Eye_Black, Eye_White, Horns) : limousine (froment, auréoles claires autour des
    //   yeux et du mufle), charolaise (crème), montbéliarde (pie rouge) ;
    //   cheval (Main, Main_Dark, Main_Light, Hooves, Hair, Muzzle, Eye_White, Eye_Black) : bai (crins noirs), gris ;
    //   mouton (White, Black) : laine blanc cassé ou crème, tête et pattes noires (suffolk)
    void chargerAnimaux(Renderer& r) {
        static const int VA[3][7] = {{C_OAK + 3, C_VINE_A + 6, C_OAK + 2, C_VINE_A + 6, C_CHURCH + 9, C_WOOD + 2, C_WOOD + 3},
                                     {C_WOOD + 3, C_WOOD + 2, C_OAK + 1, C_WALLS0 + 2, C_CHURCH + 9, C_WOOD + 2, C_WOOD + 3},
                                     {C_WOOD + 5, C_WOOD + 2, C_OAK + 2, C_WALLS0 + 2, C_CHURCH + 9, C_WOOD + 2, C_WOOD + 3}};
        static const int CH_[2][8] = {{C_OAK + 3, C_OAK + 2, C_VINE_A, C_OAK, C_CHURCH + 9, C_OAK + 2, C_WOOD + 2, C_CHURCH + 9},
                                      {C_CHURCH + 7, C_TIRE + 4, C_CHURCH + 6, C_OAK + 1, C_TIRE + 3, C_TIRE + 4, C_WOOD + 2, C_CHURCH + 9}};
        static const int MO[2][2] = {{C_WALLS0 + 5, C_CHURCH + 9}, {C_VINE_A + 6, C_CHURCH + 9}};
        bool ok = true;
        for (int k = 0; k < 3 && ok; k++) ok = chargerWfa(r, "world/animaux/vache.wfa", VA[k], 7, betesMesh_[VACHE][k]);
        for (int k = 0; k < 2 && ok; k++) ok = chargerWfa(r, "world/animaux/cheval.wfa", CH_[k], 8, betesMesh_[CHEVAL][k]);
        for (int k = 0; k < 2 && ok; k++) ok = chargerWfa(r, "world/animaux/mouton.wfa", MO[k], 2, betesMesh_[MOUTON][k]);
        for (auto& esp : betesMesh_) for (auto& robe : esp) ok = ok && (robe.empty() || robe.size() >= 5);
        if (!ok) {   // RG-03 : pas de modèle réel = pas d'animal (jamais de boîte à la place)
            for (auto& esp : betesMesh_) for (auto& robe : esp) { for (Mesh& m : robe) r.destroy(m); robe.clear(); }
            WFS_ERR("Campagne (WFS-186) : animaux indisponibles, aucun animal affiché");
        }
    }

    // un troupeau par bloc clôturé de 90 segments (côté route : clôture T_FENCE, champs de WFS-138) : vaches 5 à 12, moutons
    // 10 à 30, chevaux 2 à 4 dans un enclos ; tirages à part (graine du bloc), jamais sur une route (RG-C1 : place libre),
    // ni sur un arbre, une ferme, des bottes ou une éolienne
    void troupeaux(int64_t z0, int64_t z1, const std::vector<Obst>& obst) {
        const float R = (float)RW, SM = (float)SEG_M;
        for (int side : {-1, 1}) {
            const int64_t off = side > 0 ? 40 : 0;
            for (int64_t k = (z0 + off) / 90; k * 90 - off < z1; k++) {
                if (k % 2) continue;   // bloc sans clôture (haie)
                const int64_t b0 = k * 90 - off, c = b0 + 45;
                if (b0 < z0 || b0 + 90 > z1) continue;
                bool route = false;
                for (int64_t n = b0; n < b0 + 90 && !route; n++) route = autoroute(n) || aPeage(n);
                if (route) continue;
                Mulberry32 T((uint32_t)(c * 2654435761u) ^ (side > 0 ? 0x9e3779b9u : 0x7f4a7c15u));
                auto D = [&]() { return (float)T.next(); };
                // espèce : suite fixe le long du tracé (chaque espèce présente sur la carte), un bloc sur six laissé vide
                static const int SUITE[6] = {VACHE, MOUTON, CHEVAL, VACHE, -1, MOUTON};
                const int esp = SUITE[numBloc_++ % 6];
                if (esp < 0) continue;
                const int nb = esp == VACHE ? 5 + (int)(D() * 8) : esp == MOUTON ? 10 + (int)(D() * 21) : 2 + (int)(D() * 3);
                const float lat0 = R + 9, prof = esp == VACHE ? 28.f : esp == MOUTON ? 18.f : 16.f, longu = esp == CHEVAL ? 30.f : 60.f;
                const Libre* l = libreA(c, side);
                if (l && lat0 + prof + 4 > l->m) continue;   // champ trop étroit (autre route derrière)
                if (esp == CHEVAL) { Item e; e.t = T_ENCLOS; e.side = side; e.lat = lat0 + prof / 2; e.w = prof + 3; e.l = longu + 3; CarteDecor::item(c, e); }
                const int robe0 = (int)(D() * NB_ROBES[esp]);
                const size_t debut = betes_.size();
                for (int i = 0, essais = 0; i < nb && essais < nb * 6; essais++) {
                    Bete b; b.side = side; b.espece = (int8_t)esp;
                    const float al = (D() - 0.5f) * longu;
                    b.seg = c + (int64_t)std::floor(al / SM); b.along = al - (float)(b.seg - c) * SM;
                    b.lat = lat0 + 1.5f + D() * (prof - 3);
                    bool pris = false;
                    for (const Obst& o : obst) pris = pris || (o.side == side && b.seg >= o.n0 && b.seg <= o.n1 && b.lat >= o.lat0 - 2 && b.lat <= o.lat1 + 2);
                    // RG-02 : deux bêtes à 1,6 m au moins (2,5 m pour les grands animaux)
                    const float ecart = esp == MOUTON ? 1.6f : 2.5f;
                    for (size_t j = debut; j < betes_.size() && !pris; j++) {
                        const float dl = (float)(betes_[j].seg - b.seg) * SM + betes_[j].along - b.along, dt = betes_[j].lat - b.lat;
                        pris = dl * dl + dt * dt < ecart * ecart;
                    }
                    if (pris) continue;
                    // robe : celle du troupeau, une bête sur trois d'une autre robe ; jamais identique à la voisine précédente
                    // (orientation, instant du broutage et taille à ±1 % tirés pour chacune)
                    b.robe = (int8_t)(D() < 0.67f ? robe0 : (int)(D() * NB_ROBES[esp]));
                    b.yaw = D() * 2 * PI; b.phase = D() * 16; b.ech = 0.99f + D() * 0.02f;   // RG-02 : taille réelle à ±1 %
                    betes_.push_back(b); i++;
                }
                if (betes_.size() > debut) WFS_LOG("Campagne (WFS-186) : troupeau de %zu %s au segment %lld, côté %d", betes_.size() - debut,
                                                   esp == VACHE ? "vaches" : esp == CHEVAL ? "chevaux" : "moutons", (long long)c, side);
                const int64_t m = ((c % len()) + len()) % len();
                if (g_troupeau[esp] < 0 || m < g_troupeau[esp]) g_troupeau[esp] = m;
            }
        }
    }

    // RG-02 / RG-C4 : bêtes des morceaux chargés, de 30 m derrière à 240 m devant (160 m en Basse), 40 au plus (16 en Basse) ;
    // broutage : 4 phases de 0,4 s deux fois de suite (tête qui descend et remonte), puis 3,2 s de repos ; un appel par bête
    int animauxAnimes(Renderer& r, const GameView& g, int64_t c0, int64_t c1) {
        if (betes_.empty() || betesMesh_[VACHE][0].empty()) return 0;
        Track& t = *g.track;
        const bool basse = r.qualiteBasse;
        const double loin = basse ? 160 : 240; const int maxi = basse ? 16 : 40;
        int n = 0;
        for (const Bete& b : betes_) {
            if (n >= maxi) break;
            int64_t a;
            if (!dansMorceau(b.seg, c0 * CH, (c1 + 1) * CH, a)) continue;
            const double dz = ((double)a - g.player->dist / SEG) * SEG_M + b.along;
            if (dz < -30 || dz > loin) continue;
            const std::vector<Mesh>& P = betesMesh_[b.espece][std::min<int>(b.robe, NB_ROBES[b.espece] - 1)];
            if (P.size() < 5) continue;
            const int f = (int)std::fmod(g.time * 2.5 + b.phase, 16.0);
            const Mesh& m = P[f < 8 ? f % 4 : 4];
            const Pose p = poseAt(t, a);
            const vec3 o = lateral(p, b.side * b.lat, solY(zone(a), a, b.side, b.lat)) + p.f * b.along;
            r.draw(m, mats_[P_UNI], translate(o) * rotateY(b.yaw) * scale({b.ech, b.ech, b.ech}));
            n++;
        }
        return n;
    }

    std::vector<std::pair<int64_t, Item>> turbines_;
    std::vector<Mesh> rotors_;
};

} // namespace
} // namespace carte

WFS_SYSTEM(carte::Campagne, 107)

namespace campagne { int64_t troupeau(int espece) { return espece >= 0 && espece < 3 ? carte::g_troupeau[espece] : -1; } }   // WFS-186 : banc   // décor (100-199) : après la plage (105) et les piétons (106)

} // namespace wfs
