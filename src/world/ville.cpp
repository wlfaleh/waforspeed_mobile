// WaForSpeed — Ville (tickets WFS-06 puis WFS-93, épic E2) : les 6 quartiers du jeu web le long de la route.
// Le plan du décor (ville_quartiers.*) porte generateDecor de js/world/decor.js pour ZONES.city : centre-ville et quartier
// d'affaires (tours, platanes, passages piétons), quartier résidentiel et vieux quartier (immeubles bas à balcons,
// platanes, abribus), parc urbain (grille, pelouse, tilleuls et pins, bancs, fontaines ; immeubles en face), zone
// portuaire (entrepôts, conteneurs, grillage, grues, béton). Ville dessine ce plan :
//   - immeubles en boîtes aux façades texturées (fenêtres), en retrait de RW + 6,5 m ; balcons du résidentiel alignés
//     sur des étages de 3 m ;
//   - lampadaires (inscrits dans lampadaires() pour la nuit, WFS-09) ; platanes (WFS-34), tilleuls et pins du parc
//     (src/world/vegetation.cpp), allégés au-delà de 200 m (LOD_FAR_M) ;
//   - objets du jeu web en couleurs unies (DECOR.busstop, railing, bench, fountain, warehouse, containers, mesh, crane,
//     zebra ; palette C2) avec les primitives de decor_carte.* ; petits objets (SMALL_DECOR) : rien au-delà de 200 m ;
//   - pelouse du parc et béton du port (GROUND.city), relevés de 5 cm au-dessus du sol de Game.
// Construit par morceaux de CH segments en parallèle sur les threads de travail ; un maillage par matériau et par
// morceau (au plus 10). Collisions du décor (lampadaires, platanes, panneaux néon, update.js) : crash() de la course.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course.h"
#include "../game/course_system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "../render/eclairage.h"   // WFS-09
#include "../render/effets_particules.h"   // WFS-94 : effets::versHdr (fenêtres allumées)
#include "../../third_party/stb/stb_image.h"
#include "../core/platform.h"
#include "decor_carte.h"
#include "vegetation.h"
#include "ville_quartiers.h"
#include "zones.h"

namespace wfs {
namespace {

enum Part { P_FACADE_A, P_FACADE_B, P_METAL, P_BARK, P_CORE, P_LEAF0, P_LEAF1, P_LAMP, P_UNI, P_CONIF, P_TROTTOIR, P_FEN_CHAUD, P_FEN_FROID, P_FEN_RARE, P_COUNT };   // WFS-94 : trottoir (ANO-49), fenêtres allumées la nuit (ANO-37)

// couleurs unies du jeu web (C2, DECOR.*, BLD_C, GCOL2, BARK) : palette d'une ligne, une case par couleur
enum Couleur {
    U_METAL, U_BANC, U_VITRE, U_PIERRE, U_EAU, U_ENTREPOT0, U_TOLE = U_ENTREPOT0 + 4, U_PORTE, U_CONT0, U_GRUE = U_CONT0 + 6,
    U_CABINE, U_ZEBRA, U_DALLE, U_GARDE_CORPS, U_PELOUSE, U_BETON, U_CHENE2, U_CHENE, U_PIN, U_PIN_BAS,
    U_BLEU, U_ASPHALTE, U_BORDURE, U_NB   // WFS-137 : panneaux et plaques bleus, rue transversale, bordures de l'îlot
};
constexpr uint32_t PALETTE[U_NB] = {
    0x3a3d44, 0x6b4a2e, 0xbfe8ff, 0xc9c4b8, 0x4aa3c7,
    0x8a8f96, 0xa0723c, 0x5d6b78, 0x8e3b2e,                       // C2.wh
    0x55585e, 0x2c2f35,                                           // toit et portes des entrepôts
    0xb8352a, 0x1f5f9a, 0xd8a12a, 0x3a7d44, 0x7a3fa0, 0xd06a1d,   // C2.cont
    0xd9a21b, 0xe8e8e8, 0xe9e9e6, 0xcfcac0, 0x3a3d42,             // grue, cabine, passage piéton, dalle, garde-corps
    0x4e7a34, 0x6e6e6a,                                           // GCOL2.park, GCOL2.concrete
    0x5c4c3c, 0x4a3b2e, 0x8a5236, 0x4e4038,                       // BARK.oak2, oak, pine, pineLow (cases voisines)
    0x1d5fb4, 0x3b3e43, 0xbdb9b0};                                // WFS-137 : bleu signalisation, asphalte, bordure
vec2 uvc(int c) { return {(c + 0.5f) / (float)U_NB, 0.5f}; }

// teintes du feuillage (VEG_TINT de life.js) : tilleuls sur le matériau des platanes, pins sur leur propre matériau
const vec3 TILLEUL[2] = {{1.08f, 1.12f, 0.82f}, {1, 1.08f, 0.8f}};
const vec3 TINT_PINE[2] = {{0.66f, 0.8f, 0.74f}, {0.72f, 0.84f, 0.74f}};
float tkOf(vec3 t, vec3 base) { return (t.x + t.y + t.z) / (base.x + base.y + base.z); }

constexpr int AHEAD = 22, BEHIND = 2;
constexpr double LOD_FAR_M = 200;   // au-delà, arbres allégés et petits objets non dessinés (jeu web : renderer.js)
constexpr float SET_BACK = 6.5f;     // retrait des façades par rapport au bord de la chaussée (jeu web)

using Geo = veg::Geo;

// repère de la route au segment (fractionnaire) s : comme carte::poseAt
carte::Pose poseS(Track& t, double s) {
    const TrackPoint p = t.point(s * SEG, 0);
    const float th = (float)p.th;
    return {{(float)p.x, (float)p.y, (float)p.z}, {std::cos(th), 0, std::sin(th)}, {std::sin(th), 0, -std::cos(th)}, th};
}

void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec3 n, vec2 ua, vec2 ub, vec2 uc, vec2 ud) {
    uint32_t k = (uint32_t)g.v.size();
    for (auto [p, u] : {std::pair{a, ua}, {b, ub}, {c, uc}, {d, ud}}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}

// boîte orientée : centre au sol c, axes r / f, demi-largeur hx (selon r), demi-longueur hz (selon f), hauteur h
// uv des côtés en mètres / tuile (façades) : v(y) = (yRef − y) / tuile, yRef = haut + bas par défaut (fenêtres calées
// sur le haut) ; dessus avec uv minuscule (toit uni)
void box(Geo& g, vec3 c, vec3 r, vec3 f, float hx, float hz, float y0, float h, float tile, float yRef = NAN) {
    vec3 up{0, 1, 0};
    auto P = [&](float x, float y, float z) { return c + r * x + up * y + f * z; };
    const float y1 = y0 + h, ref = std::isnan(yRef) ? y0 + y1 : yRef;
    // 4 côtés
    struct S { float x0, z0, x1, z1; vec3 n; float w; } sides[4] = {
        {-hx, -hz, hx, -hz, -f, 2 * hx}, {hx, -hz, hx, hz, r, 2 * hz}, {hx, hz, -hx, hz, f, 2 * hx}, {-hx, hz, -hx, -hz, -r, 2 * hz}};
    for (auto& s : sides) {
        float u1 = s.w / tile, vb = (ref - y0) / tile, vh = (ref - y1) / tile;
        quad(g, P(s.x0, y0, s.z0), P(s.x1, y0, s.z1), P(s.x1, y1, s.z1), P(s.x0, y1, s.z0), s.n, {0, vb}, {u1, vb}, {u1, vh}, {0, vh});
    }
    quad(g, P(-hx, y1, -hz), P(hx, y1, -hz), P(hx, y1, hz), P(-hx, y1, hz), up, {0.01f, 0.01f}, {0.02f, 0.01f}, {0.02f, 0.02f}, {0.01f, 0.02f});
}

// cylindre vertical (8 côtés) : troncs, mâts
void cylinder(Geo& g, vec3 base, float rad, float h, int n = 8) {
    for (int k = 0; k < n; k++) {
        float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        vec3 d0{std::cos(a0), 0, std::sin(a0)}, d1{std::cos(a1), 0, std::sin(a1)};
        quad(g, base + d0 * rad, base + d1 * rad, base + d1 * rad + vec3{0, h, 0}, base + d0 * rad + vec3{0, h, 0},
             normalize(d0 + d1), {0, 0}, {1, 0}, {1, 1}, {0, 1});
    }
}

// façade d'immeuble : béton teinté, grille de fenêtres (2 par tuile), quelques vitres plus claires
// WFS-94 (RG-A2) : de nuit, couleurs de la texture de nuit du jeu web (makeTextures : fond #0d0f17, fenêtres éteintes
// #161a26, haut de façade #07080c) : façades non éclairées, sombres même dans l'axe des phares
std::vector<uint8_t> facadeTexture(int variant, int& W, int& H, bool nuit = false) {
    W = H = 256;
    if (nuit) {
        std::vector<uint8_t> px((size_t)W * H * 4);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            const float u = (x + 0.5f) / W * 2, v = (y + 0.5f) / H * 2, fu = u - (int)u, fv = v - (int)v;
            const bool fen = fu > 0.16f && fu < 0.84f && fv > 0.22f && fv < 0.82f;
            const uint32_t h = fen ? 0x161a26 : fv < 0.12f ? 0x07080c : 0x0d0f17;
            uint8_t* p = &px[((size_t)y * W + x) * 4];
            p[0] = (h >> 16) & 255; p[1] = (h >> 8) & 255; p[2] = h & 255; p[3] = 255;
        }
        return px;
    }
    std::vector<uint8_t> px((size_t)W * H * 4);
    Mulberry32 R(77 + variant * 13);
    const float wall[2][3] = {{0.55f, 0.52f, 0.48f}, {0.36f, 0.38f, 0.42f}};
    float lit[64];
    for (float& l : lit) l = (float)R.next();
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float u = (x + 0.5f) / W * 2, v = (y + 0.5f) / H * 2;   // 2x2 fenêtres par tuile
        int cx = (int)u, cy = (int)v; float fu = u - cx, fv = v - cy;
        bool window = fu > 0.16f && fu < 0.84f && fv > 0.22f && fv < 0.82f;
        bool frame = window && (std::fabs(fu - 0.5f) < 0.025f);
        float n = 0.92f + 0.08f * (float)R.next();
        float c[3];
        if (window && !frame) {
            float l = lit[(cx + cy * 2 + variant * 4) & 63];
            float g = 0.06f + 0.1f * (1 - fv) + (l > 0.82f ? 0.25f : 0);   // reflet du ciel en haut de la vitre
            c[0] = g * 0.8f; c[1] = g * 0.9f; c[2] = g * 1.1f;
        } else for (int k = 0; k < 3; k++) c[k] = wall[variant][k] * n * (fv < 0.12f ? 0.8f : 1);
        uint8_t* p = &px[((size_t)y * W + x) * 4];
        for (int k = 0; k < 3; k++) p[k] = (uint8_t)(std::pow(std::min(1.f, c[k]), 1 / 2.2f) * 255);
        p[3] = 255;
    }
    return px;
}

class Ville : public System {
public:
    const char* name() const override { return "ville"; }

    void init(Renderer& r, const GameView& g) override {
        active_ = g.map && std::strcmp(g.map->id, "ville") == 0;
        if (!active_) return;
        const char* dep = std::getenv("WFS_DEPART");   // banc d'essai : départ dans un quartier (segments)
        depart_ = dep ? std::atoll(dep) : 0;
        { const char* mo = std::getenv("WFS_MOMENT"); nuit_ = mo && !std::strcmp(mo, "nuit"); }   // WFS-94 : comme traffic_system.cpp
        // matériaux
        for (int v = 0; v < 2; v++) {
            int W, H; auto px = facadeTexture(v, W, H, nuit_);
            facadeTex_[v] = r.vk().uploadTexture(px.data(), (uint32_t)W, (uint32_t)H, true);
            Material m; m.rough = 0.75f; m.metal = 0.f;
            if (nuit_) {
                // WFS-137 (RG-B, ANO-84) : la nuit, le jeu web dessine les façades en MeshBasicMaterial (sans éclairage) : la
                // texture de nuit (fenêtres éteintes #161a26 sur #0d0f17) s'affiche telle quelle. Ici : émission × texture
                // (MAT_EMTEX), facteur étalonné sur la couleur des fenêtres éteintes par versHdr (comme les fenêtres allumées)
                const float c = 0x1a / 255.f, k = effets::versHdr(c, 1.1f) / std::pow(c, 2.2f);
                m.emissive = {k, k, k}; m.flags |= MAT_EMTEX;
            }
            mats_[P_FACADE_A + v] = r.makeMaterial(m, &facadeTex_[v], nullptr, nullptr);
        }
        auto plain = [&](Part p, vec3 c, float rough, float metal, vec3 em = {0, 0, 0}) {
            Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal; m.emissive = em;
            mats_[p] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        plain(P_METAL, {0.08f, 0.09f, 0.11f}, 0.45f, 0.8f);
        {   // WFS-94 (RG-A3, RG-A4) : fenêtres allumées (petits quads émissifs), couleurs du jeu web converties par versHdr
            const uint32_t cols[3] = {0x694b2d, 0x3e5569, 0x007f8c};   // chaud rgb(95-115, 68-82, 40-50), froid, sarcelle
            for (int k = 0; k < 3; k++) {
                Material m; m.color = {0, 0, 0, 1}; m.rough = 1;
                m.emissive = effets::versHdr(vec3{((cols[k] >> 16) & 255) / 255.f, ((cols[k] >> 8) & 255) / 255.f, (cols[k] & 255) / 255.f}, 1.1f);
                mats_[P_FEN_CHAUD + k] = r.makeMaterial(m, nullptr, nullptr, nullptr);
            }
        }
        { Material m; m.flags = MAT_WALK; m.rough = 0.85f; mats_[P_TROTTOIR] = r.makeMaterial(m, nullptr, nullptr, nullptr); }   // comme le trottoir de Game
        initVegetation(r);
        plain(P_LAMP, {1.f, 0.95f, 0.85f}, 0.3f, 0.f, {0.6f, 0.55f, 0.45f});
        // couleurs unies : palette sRGB, éclairage du feuillage (MAT_VTINT, sans reflet), comme CarteDecor
        uint8_t px[U_NB * 4];
        for (int c = 0; c < U_NB; c++) {
            px[c * 4] = (PALETTE[c] >> 16) & 255; px[c * 4 + 1] = (PALETTE[c] >> 8) & 255; px[c * 4 + 2] = PALETTE[c] & 255; px[c * 4 + 3] = 255;
        }
        palTex_ = r.vk().uploadTexture(px, U_NB, 1, true);
        Material mu; mu.rough = 0.85f; mu.flags = MAT_DOUBLE | MAT_VTINT;
        mats_[P_UNI] = r.makeMaterial(mu, &palTex_, nullptr, nullptr);
        Material mc; const vec3 cc = carte::lin(TINT_PINE[0]); mc.color = {cc.x, cc.y, cc.z, 1}; mc.rough = 0.8f; mc.cutoff = 0.5f;
        mc.flags = MAT_MASK | MAT_DOUBLE | MAT_VTINT;
        mats_[P_CONIF] = r.makeMaterial(mc, leafTex_.img ? &leafTex_ : nullptr, nullptr, nullptr);
        // plan des quartiers (partagé avec les commerces et les enseignes) ; lampadaires pour la nuit (WFS-09)
        plan_ = &ville::adapter(*g.map, *g.track);   // WFS-137 (RG-C1) : plan adapté aux sections du tracé 2 (inchangé au tracé 1)
        {   // WFS-137 (RG-A, ANO-81) : segments bordés d'un immeuble, par côté (bit 0 gauche, bit 1 droite) : trottoir jusqu'à la façade
            const int64_t L = (int64_t)plan_->objets.size();
            bati_.assign((size_t)L, 0);
            for (int64_t a = 0; a < L; a++) for (const ville::Objet& o : plan_->objets[(size_t)a]) if (o.t == ville::IMMEUBLE)
                for (int64_t k = 0; k <= (int64_t)std::ceil(o.len / SEG_M); k++) bati_[(size_t)((a + k) % L)] |= o.side > 0 ? 2 : 1;
        }
        {   // WFS-137 : optiques des feux (éteinte : verre sombre ; LED allumées, comme la Corniche) et jet de la fontaine
            Material ml; ml.color = {0.03f, 0.03f, 0.03f, 1}; ml.rough = 0.2f; ml.flags = MAT_DOUBLE;
            lentilleEteinte_ = r.makeMaterial(ml, nullptr, nullptr, nullptr);
            const vec3 couleurs[3] = {{1.f, 0.04f, 0.02f}, {1.f, 0.42f, 0.01f}, {0.05f, 1.f, 0.45f}};
            for (int k = 0; k < 3; k++) { ml.emissive = couleurs[k] * 6.f; lentille_[k] = r.makeMaterial(ml, nullptr, nullptr, nullptr); }
            Material me; me.color = {0.78f, 0.9f, 1.f, 0.45f}; me.rough = 0.1f; me.flags = MAT_BLEND | MAT_DOUBLE;
            if (nuit_) me.emissive = {0.05f, 0.07f, 0.09f};
            eauJet_ = r.makeMaterial(me, nullptr, nullptr, nullptr);
            if (!plan_->ronds.empty()) {   // gerbe : colonne de 1,6 m, puis corolle qui retombe dans la vasque (0,85 m)
                Geo j;
                carte::cyl(j, {0, 0, 0}, 0.07f, 1.6f, 8, {0, 0});
                for (int k = 0; k < 16; k++) {
                    const float a0 = 2 * PI * k / 16, a1 = 2 * PI * (k + 1) / 16;
                    const vec3 hA{std::cos(a0) * 0.25f, 1.55f, std::sin(a0) * 0.25f}, hB{std::cos(a1) * 0.25f, 1.55f, std::sin(a1) * 0.25f};
                    const vec3 bA{std::cos(a0) * 0.8f, 0.02f, std::sin(a0) * 0.8f}, bB{std::cos(a1) * 0.8f, 0.02f, std::sin(a1) * 0.8f};
                    carte::tri(j, {0, 1.7f, 0}, hB, hA, {0, 0});
                    carte::quad(j, hA, hB, bB, bA, {0, 0});
                }
                jet_ = r.makeMesh(j.v, j.i, false);
            }
        }
        abordsRondPoint(*g.track);   // WFS-137 (RG-01)
        size_t nl = 0;
        for (size_t a = 0; a < plan_->objets.size(); a++) for (const ville::Objet& o : plan_->objets[a])
            if (o.t == ville::LAMPE) { lampadaires().push_back({(int64_t)a, o.side * ((float)RW - 1.6f), 7.1f, StyleLampadaire::Ville}); nl++; }
        WFS_LOG("Ville : %zu lampadaires%s", nl, depart_ ? " (départ avancé, banc d'essai)" : "");
    }

    // matériaux des arbres (WFS-34) : atlas de feuillage du jeu web, écorce en dégradé de 2 texels, cœur sombre.
    // Couleurs du jeu web = valeurs d'affichage (sRGB) : converties en linéaire (puissance 2,2) pour le même rendu.
    void initVegetation(Renderer& r) {
        auto lin = [](vec3 c) { return vec3{std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f)}; };
        std::vector<uint8_t> png;
        int w = 0, h = 0, n = 0;
        stbi_uc* px = readAsset("world/feuillage.png", png) ? stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4) : nullptr;
        vec3 leafAvg{0.2f, 0.3f, 0.12f};   // couleur moyenne des feuilles (case 0 de l'atlas), pour le cœur du houppier
        if (px) {
            double acc[3] = {0, 0, 0}; int cnt = 0;
            for (int y = 0; y < h / 2; y += 2) for (int x = 0; x < w / 2; x += 2) {
                const stbi_uc* q = px + ((size_t)y * w + x) * 4;
                if (q[3] > 200) { for (int k = 0; k < 3; k++) acc[k] += q[k] / 255.0; cnt++; }
            }
            if (cnt) leafAvg = {(float)(acc[0] / cnt), (float)(acc[1] / cnt), (float)(acc[2] / cnt)};
            leafTex_ = r.vk().uploadTexture(px, (uint32_t)w, (uint32_t)h, true); stbi_image_free(px);
        }
        else WFS_ERR("Atlas de feuillage introuvable (world/feuillage.png)");
        for (int k = 0; k < 2; k++) {
            Material m; vec3 c = lin(veg::TINT_PLANE[k]); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.8f; m.cutoff = 0.5f;
            m.flags = MAT_MASK | MAT_DOUBLE | MAT_VTINT;
            mats_[P_LEAF0 + k] = r.makeMaterial(m, leafTex_.img ? &leafTex_ : nullptr, nullptr, nullptr);
        }
        uint8_t bark[8];
        for (int k = 0; k < 2; k++) {
            vec3 c = veg::hex(k ? veg::BARK_PLANE2 : veg::BARK_PLANE);
            bark[k * 4] = (uint8_t)(c.x * 255 + 0.5f); bark[k * 4 + 1] = (uint8_t)(c.y * 255 + 0.5f); bark[k * 4 + 2] = (uint8_t)(c.z * 255 + 0.5f); bark[k * 4 + 3] = 255;
        }
        barkTex_ = r.vk().uploadTexture(bark, 2, 1, true);
        Material mb; mb.rough = 0.9f; mb.flags = MAT_VTINT;
        mats_[P_BARK] = r.makeMaterial(mb, &barkTex_, nullptr, nullptr);
                // = feuille moyenne × teinte du platane, à l'ombre (×0,45) : se confond avec le feuillage au lieu de faire un bloc sombre
        const vec3 la = lin(leafAvg), tp = lin(veg::TINT_PLANE[0]);
        Material mc; vec3 c = vec3{la.x * tp.x, la.y * tp.y, la.z * tp.z} * 0.45f; mc.color = {c.x, c.y, c.z, 1}; mc.rough = 0.9f; mc.flags = MAT_VTINT;
        mats_[P_CORE] = r.makeMaterial(mc, nullptr, nullptr, nullptr);
    }

    // WFS-94 (RG-A3) : fenêtres allumées de la façade côté route (normale n, à hx du centre c), sur la grille de la texture
    // (2 fenêtres par tuile, rangées calées sur ref : v = (ref − y) / tuile) ; tirage fixe par immeuble (mulberry32) :
    // 24 % chaudes, 8 % froides, 1,5 % rares (moitié sarcelle, moitié orange #8c3b00 rangée avec les chaudes) ;
    // rez-de-chaussée (< 3 m) laissé aux vitrines ; sens = −1 quand u de la face court vers −f (face de normale −r)
    void fenetres(Geo out[P_COUNT], vec3 c, vec3 n, vec3 f, float hx, float len, float h, float tuile, float ref, uint32_t graine, float sens) const {
        Mulberry32 R(31u + graine * 2654435761u);
        const float cel = tuile / 2, ncol = std::floor(len / cel);
        const vec3 up{0, 1, 0}, o = c + n * (hx + 0.04f);
        for (int cy = 0;; cy++) {
            const float yh = ref - (cy + 0.22f) * cel, yb = ref - (cy + 0.82f) * cel;
            if (yb < 3.f) break;
            if (yh > h - 0.3f) continue;
            for (int cx = 0; cx < (int)ncol; cx++) {
                const float q = (float)R.next();
                const int part = q < 0.24f ? P_FEN_CHAUD : q < 0.32f ? P_FEN_FROID : q < 0.3275f ? P_FEN_RARE : q < 0.335f ? P_FEN_CHAUD : -1;
                if (part < 0) continue;
                const float z0 = sens * (-len / 2 + (cx + 0.16f) * cel), z1 = sens * (-len / 2 + (cx + 0.84f) * cel);   // sens : u de la face
                const vec3 a = o + f * z0 + up * yb, b = o + f * z1 + up * yb, d = o + f * z1 + up * yh, e = o + f * z0 + up * yh;
                if (dot(cross(b - a, d - a), n) > 0) quad(out[part], a, b, d, e, n, {0, 0}, {1, 0}, {1, 1}, {0, 1});
                else quad(out[part], a, e, d, b, n, {0, 0}, {0, 1}, {1, 1}, {1, 0});
            }
        }
    }

    // objet du plan (deco.items du jeu web) au segment absolu a (seg : son indice sur la piste, z : sa zone)
    void objet(Track& t, const ville::Objet& o, int64_t a, int64_t seg, const char* z, const carte::Pose& p, bool lod, Geo out[P_COUNT]) const {
        const float R = (float)RW, s = (float)o.side;
        const vec3 up{0, 1, 0};
        Geo& U = out[P_UNI];
        switch (o.t) {
        case ville::IMMEUBLE: {   // DECOR.bld : boîte centrée sur l'immeuble, fondations 3 m sous le trottoir
            const carte::Pose pm = poseS(t, (double)a + o.len / SEG_M / 2);
            const vec3 c = pm.o + pm.r * (s * (R + SET_BACK + o.d / 2));
            Geo& fac = out[o.k == 1 || o.k == 2 ? P_FACADE_B : P_FACADE_A];   // TINTS : 0 et 3 chaudes, 1 et 2 froides
            if (nuit_) fenetres(out, c, pm.r * -s, pm.f, o.d / 2, o.len, o.h, o.balcons ? 6.f : 4.f, o.balcons ? 301.f : o.h - 3.f, (uint32_t)(seg * 2 + (o.side > 0)), s > 0 ? -1.f : 1.f);
            if (!o.balcons) { box(fac, c, pm.r, pm.f, o.d / 2, o.len / 2, -3.f, o.h + 3.f, 4.f); break; }
            // résidentiel : étages de 3 m (rangées de fenêtres à partir de y = 1, 4, 7…) derrière les balcons (y = 4, 7…)
            box(fac, c, pm.r, pm.f, o.d / 2, o.len / 2, -3.f, o.h + 3.f, 6.f, 301.f);
            for (float y = 4; y < o.h - 1; y += 3) {   // dalle + garde-corps
                carte::box(U, carte::lateral(pm, s * (R + 6.2f), y), pm.r, pm.f, 0.7f, 0.12f, o.len * 0.85f, uvc(U_DALLE));
                if (!lod) carte::box(U, carte::lateral(pm, s * (R + 5.87f), y + 0.12f), pm.r, pm.f, 0.04f, 0.95f, o.len * 0.85f, uvc(U_GARDE_CORPS));
            }
            break;
        }
        case ville::LAMPE: {   // mât au bord du trottoir, bras vers la chaussée, tête lumineuse
            const vec3 y15{0, 0.15f, 0};
            cylinder(out[P_METAL], p.o + p.r * (s * (R + 0.8f)) + y15, 0.09f, 7.4f, 6);
            box(out[P_METAL], p.o + p.r * (s * (R - 0.55f)) + y15, p.r, p.f, 1.4f, 0.06f, 7.3f, 0.12f, 1.f);
            box(out[P_LAMP], p.o + p.r * (s * (R - 1.6f)) + y15, p.r, p.f, 0.45f, 0.18f, 7.18f, 0.12f, 1.f);
            break;
        }
        case ville::ARBRE_RUE: {   // DECOR.streettree : grille d'arbre 1,3 × 1,3 m et platane, graine mulberry32(a · 13 + côté)
            const vec3 base = p.o + p.r * (s * o.lat) + vec3{0, 0.15f, 0};
            box(out[P_METAL], base, p.r, p.f, 0.65f, 0.65f, -0.02f, 0.05f, 1.f);
            Mulberry32 Rt((uint32_t)(seg * 13 + (o.side > 0 ? 1 : 0)));
            Geo* leaf[2] = {&out[P_LEAF0], &out[P_LEAF1]};
            veg::platane(&out[P_BARK], &out[P_CORE], leaf, base, o.h, Rt, lod);
            break;
        }
        case ville::ZEBRA: {   // passage piéton : 8 bandes blanches sur 3 segments
            const carte::Pose p3 = carte::poseAt(t, a + 3);
            for (int k = 0; k < 8; k++) {
                const float x0 = -R + 0.6f + k * (2 * R - 1.2f) / 8, x1 = x0 + 0.95f;
                carte::quadSol(U, carte::lateral(p, x0, 0.015f), carte::lateral(p, x1, 0.015f), carte::lateral(p3, x1, 0.015f), carte::lateral(p3, x0, 0.015f), uvc(U_ZEBRA));
            }
            break;
        }
        case ville::ABRIBUS: {   // paroi du fond, toit, vitre, banc
            if (lod) break;
            const vec3 b = carte::lateral(p, s * (R + 3.8f), 0.15f), rs = p.r * s;
            carte::box(U, b + rs * 0.8f, p.r, p.f, 0.1f, 2.5f, 4, uvc(U_METAL));
            carte::box(U, b + up * 2.5f, p.r, p.f, 1.8f, 0.1f, 4.2f, uvc(U_METAL));
            carte::box(U, b + rs * 0.75f + up * 0.6f, p.r, p.f, 0.04f, 1.6f, 1.2f, uvc(U_VITRE));
            carte::box(U, b + rs * 0.4f, p.r, p.f, 0.4f, 0.5f, 2.4f, uvc(U_BANC));
            break;
        }
        case ville::GRILLE:   // grille du parc : poteau et lisse de 4 segments
            if (lod) break;
            carte::box(U, carte::lateral(p, s * o.lat, 0.15f), p.r, p.f, 0.06f, 1.1f, 0.06f, uvc(U_METAL));
            carte::box(U, carte::lateral(p, s * o.lat, 1.15f), p.r, p.f, 0.05f, 0.05f, 4 * (float)SEG_M, uvc(U_METAL));
            break;
        case ville::ARBRE: {   // DECOR.tree de la ville : pin (vegPine sans parasol) ou tilleul (vegBroadleaf)
            const bool allege = lod || o.lat > R + 28;   // fond du parc : version allégée même de près
            const vec3 b = carte::lateral(p, s * o.lat, ville::solY(z, o.lat) - 0.1f) + p.f * o.along;
            Mulberry32 Rt((uint32_t)((int)(o.h * 1000) + (int)seg * 7 + (o.side > 0 ? 3 : 0) + (int)(o.lat * 10)));
            if (o.k == 1) {
                const vec3 tint = TINT_PINE[(int)(Rt.next() * 2)];
                const veg::Ecorce bas{uvc(U_PIN_BAS).x, uvc(U_PIN_BAS).x}, haut{uvc(U_PIN).x, uvc(U_PIN_BAS).x};
                veg::pin({&U, nullptr, &out[P_CONIF]}, b, o.h, Rt, tkOf(tint, TINT_PINE[0]), false, bas, haut, allege);
            } else {
                const vec3 tint = TILLEUL[(int)(Rt.next() * 2)];
                veg::Broadleaf bl;
                bl.cw = 0.3f; bl.ch = 0.3f; bl.cy = 0.65f;
                bl.uA = uvc(U_CHENE2).x; bl.uB = uvc(U_CHENE).x; bl.tk = tkOf(tint, veg::TINT_PLANE[0]);
                veg::broadleaf({&U, nullptr, &out[P_LEAF0]}, b, o.h, Rt, bl, allege);
            }
            break;
        }
        case ville::BANC: {   // assise et dossier
            if (lod) break;
            const vec3 b = carte::lateral(p, s * o.lat, 0.15f);
            carte::box(U, b, p.r, p.f, 0.5f, 0.45f, 1.8f, uvc(U_BANC));
            carte::box(U, b + p.r * (s * 0.25f), p.r, p.f, 0.08f, 0.95f, 1.8f, uvc(U_BANC));
            break;
        }
        case ville::FONTAINE: {   // bassin, eau, colonne
            const vec3 b = carte::lateral(p, s * o.lat, ville::solY(z, o.lat));
            carte::cyl(U, b, 4, 0.6f, 12, uvc(U_PIERRE));
            carte::cyl(U, b + up * 0.05f, 3.7f, 0.6f, 12, uvc(U_EAU));
            carte::cyl(U, b, 0.5f, 2.2f, 12, uvc(U_PIERRE));
            break;
        }
        case ville::ENTREPOT: {   // corps, toit en losange, deux portes côté route ; pose au milieu (Math.round)
            const carte::Pose pm = carte::poseAt(t, a + (int64_t)std::floor(o.len / SEG_M / 2 + 0.5));
            const vec3 c = carte::lateral(pm, s * (o.lat + o.d / 2), -0.2f);
            carte::box(U, c, pm.r, pm.f, o.d, o.h, o.len, uvc(U_ENTREPOT0 + o.k));
            carte::toit(U, c + up * (o.h + 0.6f), pm.r, pm.f, o.d * 1.02f, 1.2f, o.len * 1.02f, uvc(U_TOLE));
            const vec3 porte = carte::lateral(pm, s * (o.lat - 0.02f), -0.2f);
            for (float k : {-0.25f, 0.25f}) carte::box(U, porte + pm.f * (o.len * k), pm.r, pm.f, 0.1f, o.h * 0.6f, o.len * 0.18f, uvc(U_PORTE));
            break;
        }
        case ville::CONTENEURS: {   // k colonnes de 1 à 3 conteneurs (12 m), hauteur et couleur tirées de la graine
            const vec3 b = carte::lateral(p, s * o.lat, ville::solY(z, o.lat));
            Mulberry32 Rc(o.graine);
            for (int i = 0; i < o.k; i++) for (int hh = 0; hh < 1 + (int)(Rc.next() * 3); hh++)
                carte::box(U, b + p.r * (i * 2.6f) + up * (hh * 2.6f), p.r, p.f, 2.45f, 2.55f, 12, uvc(U_CONT0 + (int)(Rc.next() * 6)));
            break;
        }
        case ville::GRILLAGE:   // grillage du port : poteau et lisse de 3 segments
            if (lod) break;
            carte::box(U, carte::lateral(p, s * o.lat, 0), p.r, p.f, 0.08f, 2.2f, 0.08f, uvc(U_METAL));
            carte::box(U, carte::lateral(p, s * o.lat, 2.1f), p.r, p.f, 0.04f, 0.06f, 3 * (float)SEG_M, uvc(U_METAL));
            break;
        case ville::GRUE: {   // 4 pieds, tête, flèche de 40 m au-dessus des quais, cabine
            const vec3 b = carte::lateral(p, s * o.lat, ville::solY(z, o.lat));
            for (float dx : {-3.f, 3.f}) for (float dz : {-3.f, 3.f}) carte::box(U, b + p.r * dx + p.f * dz, p.r, p.f, 0.7f, o.h, 0.7f, uvc(U_GRUE));
            carte::box(U, b + up * o.h, p.r, p.f, 7.5f, 2.2f, 7.5f, uvc(U_GRUE));
            carte::box(U, b - p.r * (s * 14) + up * (o.h + 1.5f), p.r, p.f, 40, 1.4f, 2, uvc(U_GRUE));
            carte::box(U, b + p.r * (s * 3) + up * (o.h + 2.2f), p.r, p.f, 4, 3, 4, uvc(U_CABINE));
            break;
        }
        }
    }

    // géométrie d'un morceau (segments [c*CH, (c+1)*CH)) — appelée en parallèle, lit seulement la piste et le plan
    void buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]) const {
        const ville::Plan& P = *plan_;
        const int64_t L = (int64_t)P.objets.size(), a0 = c * CH, a1 = a0 + CH;
        for (int64_t a = a0; a < a1; a++) {
            const int64_t seg = ((a % L) + L) % L;   // la piste se répète (tours)
            const char* z = zoneId(*t.map(), seg, L);
            const carte::Pose p = carte::poseAt(t, a);
            {   // WFS-94 (RG-06, ANO-49) : trottoir de RW + 4 à RW + 6 (GROUND.city : walk jusqu'à RW + 6), à 0,15 m, des deux
                // côtés, dans tous les quartiers ; uv en mètres comme Game::buildChunkCpu (u latéral, v = segment · SEG_M) :
                // les dalles continuent celles du trottoir de Game, sans jointure visible
                const carte::Pose p1 = carte::poseAt(t, a + 1);
                Geo& w = out[P_TROTTOIR];
                const float v0 = (float)(a * SEG_M), v1 = (float)((a + 1) * SEG_M), R = (float)RW;
                for (int s : {-1, 1}) {
                    const uint32_t k = (uint32_t)w.v.size();
                    // WFS-137 (RG-C1) : côté intérieur d'une courbe serrée, jamais au-delà de 90 % du rayon (comme Game)
                    auto som = [&](const carte::Pose& q, int64_t sa, float lat, float v) {
                        lat = std::min(lat, P.lim(sa, s)); const vec3 x = carte::lateral(q, s * lat, 0.15f); w.v.push_back({x.x, x.y, x.z, 0, 1, 0, s * lat, v}); };
                    // WFS-137 (RG-A, ANO-81) : devant un immeuble, jusque sous sa façade (RW + 6,5 m) : plus d'herbe au pied
                    const float bord = facade(a, s) ? R + FACADE_TROTTOIR : R + 6;
                    som(p, a, R + 4, v0); som(p, a, bord, v0); som(p1, a + 1, bord, v1); som(p1, a + 1, R + 4, v1);
                    const vec3 A{w.v[k].px, w.v[k].py, w.v[k].pz}, B{w.v[k + 1].px, w.v[k + 1].py, w.v[k + 1].pz}, C{w.v[k + 2].px, w.v[k + 2].py, w.v[k + 2].pz};
                    if (cross(B - A, C - A).y >= 0) w.i.insert(w.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
                    else w.i.insert(w.i.end(), {k, k + 2, k + 1, k, k + 3, k + 2});
                }
            }
            ville::Bande bd[2];
            if (const int nb = ville::bandes(z, bd)) {   // pelouse du parc, béton du port (des deux côtés, comme GROUND.city)
                const carte::Pose p1 = carte::poseAt(t, a + 1);
                const vec2 col = uvc(std::strcmp(z, "parc") ? U_BETON : U_PELOUSE);
                for (int s : {-1, 1}) for (int i = 0; i < nb; i++) {
                    const ville::Bande& b = bd[i];
                    const float l0 = P.lim(a, s), l1 = P.lim(a + 1, s);   // WFS-137 (RG-C1) : côté intérieur des courbes serrées
                    // WFS-137 (RG-A) : devant un immeuble, la pelouse ou le béton commencent derrière le trottoir prolongé
                    const float ba = i == 0 && facade(a, s) ? std::max(b.a, (float)RW + FACADE_TROTTOIR) : b.a;
                    if (ba >= std::min(l0, l1) || ba >= b.b) continue;
                    carte::quadSol(out[P_UNI], carte::lateral(p, s * ba, b.ya), carte::lateral(p1, s * ba, b.ya),
                                   carte::lateral(p1, s * std::min(b.b, l1), b.yb), carte::lateral(p, s * std::min(b.b, l0), b.yb), col);
                }
            }
            for (const ville::Objet& o : P.objets[(size_t)seg]) objet(t, o, a, seg, z, p, lod, out);
            lieux(t, a, seg, p, lod, out);   // WFS-137 : rond-point et carrefours du tracé 2
        }
    }

    // WFS-137 (RG-01) : îlot du rond-point : bordure, gazon, fontaine (bassin, colonne, vasque ; le jet est animé dans
    // render), trois platanes, panneaux bleus « sens giratoire » à l'entrée. (RG-02) carrefour à feux : ligne d'arrêt,
    // passages zébrés de part et d'autre de la rue transversale (asphalte, 9 m, jusqu'à 40 m), mâts des feux aux 4 coins
    // (les optiques, allumées selon trace::feuA, sont dessinées dans render), plaques de rue (bleues, sans nom).
    void lieux(Track& t, int64_t a, int64_t seg, const carte::Pose& p, bool lod, Geo out[P_COUNT]) const {
        const ville::Plan& P = *plan_;
        const int64_t L = (int64_t)P.objets.size();
        const float R = (float)RW;
        Geo& U = out[P_UNI];
        const vec3 up{0, 1, 0};
        auto disque = [&](vec3 c, float rad, int n, vec2 uv) {   // disque horizontal, face vers le haut
            for (int k = 0; k < n; k++) {
                const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
                carte::tri(U, c, c + vec3{std::cos(a1), 0, std::sin(a1)} * rad, c + vec3{std::cos(a0), 0, std::sin(a0)} * rad, uv);
            }
        };
        auto mod = [&](int64_t x) { return ((x % L) + L) % L; };
        for (const ville::RondPoint& rp : P.ronds) {
            if (seg == mod(rp.debut - (int64_t)(6 / SEG_M))) {   // panneaux « sens giratoire » des deux côtés, 6 m avant l'anneau
                for (float sd : {-1.f, 1.f}) {
                    const vec3 b = carte::lateral(p, sd * (R + 1.2f), 0.15f);
                    carte::cyl(out[P_METAL], b, 0.04f, 2.3f, 6, {0, 0});
                    const vec3 c = b + vec3{0, 2.3f, 0} - p.f * 0.06f;
                    for (int k = 0; k < 16; k++) {   // disque bleu de 70 cm, face aux conducteurs qui arrivent
                        const float a0 = 2 * PI * k / 16, a1 = 2 * PI * (k + 1) / 16;
                        carte::tri(U, c, c + (p.r * std::cos(a0) + up * std::sin(a0)) * 0.35f, c + (p.r * std::cos(a1) + up * std::sin(a1)) * 0.35f, uvc(U_BLEU));
                    }
                    for (int k = 0; k < 3; k++) {   // trois flèches blanches qui tournent (sens giratoire)
                        const float an = 2 * PI * k / 3 + 0.4f, ap = an + 0.7f;
                        const vec3 q = c - p.f * 0.01f, d0 = p.r * std::cos(an) + up * std::sin(an), d1 = p.r * std::cos(ap) + up * std::sin(ap);
                        carte::tri(U, q + d0 * 0.16f, q + d0 * 0.28f, q + d1 * 0.22f, uvc(U_ZEBRA));
                    }
                }
            }
            if (seg != mod(rp.centre)) continue;
            const double c = t.segAt(a).curve;
            const float kc = c > 0 ? 1.f : -1.f;
            const vec3 ctr = p.o + p.r * (rp.rayon * kc);
            const float ri = rp.rayon - R - 1.2f;   // 1,2 m de pavés entre la chaussée et la bordure
            const size_t irp = (size_t)(&rp - P.ronds.data());
            auto monde = [&](vec3 l) { return p.o + p.r * l.x + p.f * l.z + vec3{0, l.y, 0}; };
            auto dir = [&](vec3 l) { return p.r * l.x + p.f * l.z; };
            if (irp < abords_.size()) for (const Abord& ab : abords_[irp]) {
                const vec3 c = monde(ab.c), u = dir(ab.u), n = dir(ab.n);
                if (ab.k < 0) {   // amorce : asphalte (au ras de l'anneau) et ses deux bordures de trottoir
                    const vec3 a0 = c + u * ab.a, a1 = c + u * (ab.a + ab.b);
                    carte::quadSol(U, a0 - n * ab.h + vec3{0, 0.02f, 0}, a0 + n * ab.h + vec3{0, 0.02f, 0}, a1 + n * ab.h + vec3{0, 0.02f, 0}, a1 - n * ab.h + vec3{0, 0.02f, 0}, uvc(U_ASPHALTE));
                    for (float sd : {-1.f, 1.f})
                        carte::quadSol(out[P_TROTTOIR], a0 + n * (sd * ab.h) + vec3{0, 0.15f, 0}, a0 + n * (sd * (ab.h + 5)) + vec3{0, 0.15f, 0},
                                       a1 + n * (sd * (ab.h + 5)) + vec3{0, 0.15f, 0}, a1 + n * (sd * ab.h) + vec3{0, 0.15f, 0}, {0, 0});
                    continue;
                }
                Geo& fac = out[ab.k == 1 || ab.k == 2 ? P_FACADE_B : P_FACADE_A];   // immeuble : façade vers la branche (−n)
                box(fac, c, n, u, ab.b, ab.a, -3.f, ab.h + 3.f, 4.f);
                if (nuit_) fenetres(out, c, n * -1.f, u, ab.b, 2 * ab.a, ab.h, 4.f, ab.h - 3.f, (uint32_t)(rp.centre * 16 + ab.bras * 4 + (int)(&ab - abords_[irp].data())), 1.f);
            }
            if (ri < 2.5f) continue;
            const vec3 b = ctr + vec3{0, 0.15f, 0};
            carte::cyl(U, b, ri, 0.22f, 28, uvc(U_BORDURE));
            disque(b + vec3{0, 0.225f, 0}, ri - 0.25f, 28, uvc(U_PELOUSE));   // gazon (la bordure de 25 cm reste visible)
            const vec3 g = b + vec3{0, 0.22f, 0};
            carte::cyl(U, g, 2.4f, 0.55f, 20, uvc(U_PIERRE));            // bassin (4,8 m)
            disque(g + vec3{0, 0.57f, 0}, 2.15f, 20, uvc(U_EAU));       // eau du bassin
            carte::cyl(U, g, 0.3f, 1.75f, 10, uvc(U_PIERRE));            // colonne
            carte::cyl(U, g + vec3{0, 1.75f, 0}, 1.0f, 0.18f, 16, uvc(U_PIERRE));   // vasque
            disque(g + vec3{0, 1.95f, 0}, 0.85f, 16, uvc(U_EAU));
            Mulberry32 Rt((uint32_t)(rp.centre * 31 + 7));
            Geo* leaf[2] = {&out[P_LEAF0], &out[P_LEAF1]};
            for (int k = 0; k < 3; k++) {   // trois platanes autour du bassin, entre la fontaine et la bordure
                const float an = 2 * PI * k / 3 + 0.5f, d = std::min(ri - 1.f, 3.9f);
                veg::platane(&out[P_BARK], &out[P_CORE], leaf, g + vec3{std::cos(an) * d, 0, std::sin(an) * d}, 3.2f, Rt, lod);
            }
        }
        for (const ville::Carrefour& cf : P.carrefours) {
            const double dr = ville::RUE_TRANS_M / SEG_M;
            auto bandes = [&](double s0, double s1) {   // passage zébré de s0 à s1 (segments), 8 bandes sur la largeur
                const carte::Pose q0 = poseS(t, s0), q1 = poseS(t, s1);
                for (int k = 0; k < 8; k++) {
                    const float x0 = -R + 0.6f + k * (2 * R - 1.2f) / 8, x1 = x0 + 0.95f;
                    carte::quadSol(U, carte::lateral(q0, x0, 0.015f), carte::lateral(q0, x1, 0.015f), carte::lateral(q1, x1, 0.015f), carte::lateral(q1, x0, 0.015f), uvc(U_ZEBRA));
                }
            };
            auto mat = [&](const carte::Pose& q, float sd, float face) {   // mât du feu, caisson, plaque de rue (sur les mâts d'avant)
                const vec3 b = carte::lateral(q, sd * (R + 0.8f), 0.15f);
                carte::cyl(out[P_METAL], b, 0.07f, 3.6f, 8, {0, 0});
                carte::box(out[P_METAL], b + q.f * (face * 0.12f), q.r, q.f, 0.32f, 0.95f, 0.22f, {0, 0}, 2.65f);
                if (face < 0) carte::box(U, b + q.r * (sd * 0.5f), q.f, q.r, 0.04f, 0.25f, 0.9f, uvc(U_BLEU), 2.3f);   // plaque, le long de la rue
            };
            if (seg == mod(cf.ligne)) {
                const double s0 = (double)a;
                const carte::Pose q = poseS(t, s0);
                carte::quadSol(U, carte::lateral(q, -R, 0.016f), carte::lateral(q, R, 0.016f),
                               carte::lateral(poseS(t, s0 + 0.4 / SEG_M), R, 0.016f), carte::lateral(poseS(t, s0 + 0.4 / SEG_M), -R, 0.016f), uvc(U_ZEBRA));   // ligne d'arrêt
                bandes(s0 + 0.9 / SEG_M, s0 + 3.2 / SEG_M);
                for (float sd : {-1.f, 1.f}) mat(q, sd, -1.f);
            }
            if (seg == mod(cf.croise)) {
                const double s0 = (double)a;
                const carte::Pose q0 = poseS(t, s0 - dr), q1 = poseS(t, s0 + dr), qm = poseS(t, s0);
                for (float sd : {-1.f, 1.f}) {   // rue transversale (asphalte), ligne médiane en tirets
                    carte::quadSol(U, carte::lateral(q0, sd * R, 0.17f), carte::lateral(q1, sd * R, 0.17f), carte::lateral(q1, sd * (R + 40), 0.17f), carte::lateral(q0, sd * (R + 40), 0.17f), uvc(U_ASPHALTE));
                    for (float x = R + 6; x < R + 39; x += 4)
                        carte::quadSol(U, carte::lateral(qm, sd * x, 0.18f) - qm.f * 0.07f, carte::lateral(qm, sd * x, 0.18f) + qm.f * 0.07f,
                                       carte::lateral(qm, sd * (x + 2), 0.18f) + qm.f * 0.07f, carte::lateral(qm, sd * (x + 2), 0.18f) - qm.f * 0.07f, uvc(U_ZEBRA));
                }
                const double sf = s0 + dr + 0.6 / SEG_M;   // après la rue : passage zébré et mâts des 2 autres coins
                bandes(sf, sf + 2.3 / SEG_M);
                const carte::Pose qf = poseS(t, sf + 2.6 / SEG_M);
                for (float sd : {-1.f, 1.f}) mat(qf, sd, 1.f);
            }
        }
    }

    // pilote automatique (démonstrations des écrans, banc d'essai) : Game l'appelle une fois par image, avant les pas de
    // conduite ; on note l'image pour que le décor ne l'arrête pas (le jeu web n'a pas de pilote automatique) : ses
    // trajectoires, donc les captures course-* et trafic-*, restent celles d'avant les collisions de la ville
    void autopilot(double, DriveInput&, const GameView& g) override { piloteT_ = g.time; pilote_ = true; }

    // banc d'essai : départ avancé (une fois) ; puis collisions du décor (update.js) : hors de la chaussée, en course,
    // au-dessus de MAX_SPEED / 6, un sprite « collide » du segment du joueur à moins de hit → crash()
    void collide(double, Player& pl, const GameView& g) override {
        if (!active_ || !plan_) return;
        if (depart_) {
            const double d = (double)depart_ * SEG;
            pl.position = g.track->wrap(pl.position + d); pl.dist += d;
            depart_ = 0;
        }
        if (pilote_ && piloteT_ == g.time) return;   // image conduite par le pilote automatique
        if (std::fabs(pl.playerX) <= 1 || pl.speed <= MAX_SPEED / 6) return;
        Course* course = activeCourseMut();
        if (!course || !course->playing()) return;
        const int64_t L = (int64_t)plan_->chocs.size(), a = (((int64_t)std::floor(pl.position / SEG)) % L + L) % L;
        for (const ville::Choc& h : plan_->chocs[(size_t)a])
            if (std::fabs(pl.playerX - h.offset) < h.hit) {
                WFS_LOG("Ville : choc contre le décor (segment %lld, x %.2f)", (long long)a, pl.playerX);
                course->crash(pl, *g.track);
                break;
            }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!active_ || g.sceneDediee) return;   // scène dédiée (garage…) : pas de ville (WFS-44)
        frame_++;
        Track& t = *g.track;
        const int64_t aP = (int64_t)std::floor(g.player->dist / SEG), pc = aP / CH;
        auto lodOf = [&](int64_t c) { return (double)(c * CH - aP) * SEG_M > LOD_FAR_M; };
        for (auto it = chunks_.begin(); it != chunks_.end();) {
            // hors de vue, ou morceau allégé qui se rapproche : reconstruit en détail
            if (it->first < pc - BEHIND || it->first > pc + AHEAD || (it->second.lod && !lodOf(it->first))) {
                for (Mesh& m : it->second.m) if (m.vb.buf) graveyard_.push_back({frame_, m});
                it = chunks_.erase(it);
            } else ++it;
        }
        std::vector<int64_t> missing;
        for (int64_t c = pc - BEHIND; c <= pc + AHEAD; c++) if (!chunks_.count(c)) missing.push_back(c);
        if (!missing.empty()) {
            // poses calculées ici (thread principal) ; les threads ne font que lire la piste
            t.pose(missing.front() * CH - 2); t.pose((missing.back() + 1) * CH + 40);
            std::vector<std::array<Geo, P_COUNT>> cpu(missing.size());
            jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunk(t, missing[i], lodOf(missing[i]), cpu[i].data()); });
            for (size_t i = 0; i < missing.size(); i++) {
                Chunk& ch = chunks_[missing[i]];
                ch.lod = lodOf(missing[i]);
                for (int p = 0; p < P_COUNT; p++) if (!cpu[i][p].i.empty()) ch.m[p] = r.makeMesh(cpu[i][p].v, cpu[i][p].i, true);
            }
        }
        for (auto it = graveyard_.begin(); it != graveyard_.end();) {
            if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
        }
        const mat4 I = mat4::identity();
        int draws = 0;
        for (auto& [k, ch] : chunks_) for (int p = 0; p < P_COUNT; p++) if (ch.m[p].indexCount) { r.draw(ch.m[p], mats_[p], I); draws++; }
        draws += lieuxAnimes(r, g, aP);   // WFS-137
        lastDraws_ = draws;
        if (frame_ % 300 == 1) WFS_LOG("Ville : %d appels de dessin (%zu morceaux)", draws, chunks_.size());
    }

    // WFS-137 (RG-01) : abords du rond-point, dans le repère de la route au centre de l'anneau (x : latéral, z : avant ; le même
    // à chaque tour) : amorces des deux branches sans route (asphalte, 30 m) et immeubles le long des quatre branches, gardés
    // seulement s'ils restent à plus de RW + 6 m de toute la route (± 360 m) : rien ne retombe sur l'entrée ni la sortie
    void abordsRondPoint(Track& t) {
        abords_.assign(plan_->ronds.size(), {});
        const float R = (float)RW;
        for (size_t i = 0; i < plan_->ronds.size(); i++) {
            const ville::RondPoint& rp = plan_->ronds[i];
            const carte::Pose pc = carte::poseAt(t, rp.centre), pe = carte::poseAt(t, rp.debut), ps = carte::poseAt(t, rp.fin);
            const vec3 ctr = pc.o + pc.r * (rp.rayon * (t.segAt(rp.centre).curve > 0 ? 1.f : -1.f));
            std::vector<vec3> route;
            for (int64_t k = -300; k <= 300; k++) route.push_back(carte::poseAt(t, rp.centre + k).o);
            auto libre = [&](vec3 q, float marge) {
                for (const vec3& o : route) if ((o.x - q.x) * (o.x - q.x) + (o.z - q.z) * (o.z - q.z) < marge * marge) return false;
                return true;
            };
            auto local = [&](vec3 w) { const vec3 d = w - pc.o; return vec3{dot(d, pc.r), d.y, dot(d, pc.f)}; };
            auto localDir = [&](vec3 d) { return vec3{dot(d, pc.r), 0, dot(d, pc.f)}; };
            const vec3 bras[4] = {pe.f * -1.f, ps.f, pe.f, ps.f * -1.f};   // entrée, sortie (route) ; deux amorces sans route
            Mulberry32 D((uint32_t)(rp.centre * 7 + 3));
            for (int b = 0; b < 4; b++) {
                const vec3 u = bras[b], n{-u.z, 0, u.x};
                const float r0 = rp.rayon + R - 0.5f;   // bord extérieur de l'anneau
                if (b >= 2) {   // amorce : 30 m d'asphalte (largeur de la route), si elle ne touche aucune autre partie de la route
                    bool ok = true;
                    for (float d = r0 + 6; d <= r0 + 30 && ok; d += 3) ok = libre(ctr + u * d, 2 * R);
                    if (ok) abords_[i].push_back({local(ctr), localDir(u), localDir(n), r0, 30, R, -1, b});
                }
                for (float sd : {-1.f, 1.f}) for (int k = 0; k < 2; k++) {   // deux immeubles de chaque côté de chaque branche
                    const float len = 14 + (float)D.next() * 6, dep = 9 + (float)D.next() * 5, h = 12 + (float)D.next() * 18;
                    const float along = r0 + 4 + k * 22 + len / 2, lat = R + SET_BACK + dep / 2;
                    const vec3 c = ctr + u * along + n * (sd * lat);
                    bool ok = libre(c, R + 6 + std::hypot(len, dep) / 2);
                    if (ok) abords_[i].push_back({local(c), localDir(u), localDir(n * sd), len / 2, dep / 2, h, (int)(D.next() * 4), (int)b});
                }
            }
            WFS_LOG("Ville (WFS-137) : rond-point au segment %lld (rayon %.1f m) : %zu amorces et immeubles aux abords",
                    (long long)rp.centre, rp.rayon, abords_[i].size());
        }
    }

    // WFS-137 : occurrence du segment seg (sur un tour) la plus proche du segment absolu aP
    int64_t proche(int64_t seg, int64_t aP) const {
        const int64_t L = (int64_t)plan_->objets.size();
        int64_t x = aP - ((aP % L) + L) % L + seg;
        if (x - aP > L / 2) x -= L; else if (aP - x > L / 2) x += L;
        return x;
    }

    // WFS-137 (RG-02) : optiques des feux (rouge en haut), allumées selon trace::feuA, sur les 4 mâts du carrefour ; maillages
    // construits à l'occurrence la plus proche du joueur (le tour change la position dans le monde). (RG-01) jet de la fontaine :
    // gerbe translucide qui monte et retombe dans la vasque, hauteur animée
    int lieuxAnimes(Renderer& r, const GameView& g, int64_t aP) {
        Track& t = *g.track;
        const ville::Plan& P = *plan_;
        const float R = (float)RW;
        int draws = 0;
        optiques_.resize(P.carrefours.size());
        for (size_t i = 0; i < P.carrefours.size(); i++) {
            const ville::Carrefour& cf = P.carrefours[i];
            const int64_t abs = proche(cf.ligne, aP);
            if (std::abs(abs - aP) * SEG_M > 400) continue;
            Optiques& op = optiques_[i];
            if (op.abs != abs) {
                for (Mesh& m : op.m) if (m.vb.buf) { graveyard_.push_back({frame_, m}); m = Mesh{}; }
                op.abs = abs;
                const double sf = (double)(abs + cf.croise - cf.ligne) + ville::RUE_TRANS_M / SEG_M + 2.9 / SEG_M;
                t.pose(abs - 2); t.pose((int64_t)sf + 3);
                const carte::Pose qa = poseS(t, (double)abs), qf = poseS(t, sf);
                for (int k = 0; k < 3; k++) {   // optiques de 200 mm, face aux conducteurs (mâts d'avant : vers −f ; d'après : vers +f)
                    Geo gk;
                    for (const auto& [q, face] : {std::pair{qa, -1.f}, std::pair{qf, 1.f}})
                        for (float sd : {-1.f, 1.f}) {
                            const vec3 c = carte::lateral(q, sd * (R + 0.8f), 0.15f + 3.35f - 0.23f * k) + q.f * (face * 0.23f);
                            carte::cylAxe(gk, c, q.f * face, 0.09f, 0.03f, 10, {0, 0});
                        }
                    op.m[k] = r.makeMesh(gk.v, gk.i, false);
                }
            }
            const trace::Feu f = trace::feuA(cf.feu);
            const int allume = f == trace::Feu::Rouge ? 0 : f == trace::Feu::Orange ? 1 : f == trace::Feu::Vert ? 2 : -1;
            for (int k = 0; k < 3; k++) if (op.m[k].indexCount) { r.draw(op.m[k], k == allume ? lentille_[k] : lentilleEteinte_, mat4::identity()); draws++; }
        }
        if (jet_.indexCount) for (const ville::RondPoint& rp : P.ronds) {
            const int64_t abs = proche(rp.centre, aP);
            if (std::abs(abs - aP) * SEG_M > 300) continue;
            const carte::Pose p = carte::poseAt(t, abs);
            const vec3 ctr = p.o + p.r * (rp.rayon * (t.segAt(abs).curve > 0 ? 1.f : -1.f)) + vec3{0, 0.15f + 0.22f + 1.93f, 0};
            const float h = 1.f + 0.12f * (float)std::sin(g.time * 3.1) + 0.05f * (float)std::sin(g.time * 11.7);
            r.draw(jet_, eauJet_, translate(ctr) * scale({1, h, 1})); draws++;
        }
        return draws;
    }

    void shutdown(Renderer& r) override {
        if (!active_) return;
        WFS_LOG("Ville : %d appels de dessin à la dernière image", lastDraws_);
        r.vk().waitIdle();
        for (Optiques& op : optiques_) for (Mesh& m : op.m) if (m.vb.buf) r.destroy(m);
        optiques_.clear();
        if (jet_.vb.buf) { r.destroy(jet_); jet_ = Mesh{}; }
        for (auto& [k, ch] : chunks_) for (Mesh& m : ch.m) if (m.vb.buf) r.destroy(m);
        chunks_.clear();
        for (auto& g : graveyard_) r.destroy(g.second);
        graveyard_.clear();
        for (Image& im : facadeTex_) r.vk().destroy(im);
        for (Image* im : {&leafTex_, &barkTex_, &palTex_}) if (im->img) { r.vk().destroy(*im); *im = Image{}; }
        for (Material& m : mats_) r.destroy(m);
        for (Material* m : {&lentilleEteinte_, &lentille_[0], &lentille_[1], &lentille_[2], &eauJet_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        plan_ = nullptr;
        pilote_ = false;
        active_ = false;
    }

private:
    static constexpr float FACADE_TROTTOIR = SET_BACK + 0.3f;   // WFS-137 : trottoir prolongé 30 cm sous la façade (pas de jour)
    bool facade(int64_t a, int s) const {
        if (bati_.empty()) return false;
        const int64_t L = (int64_t)bati_.size();
        return bati_[(size_t)(((a % L) + L) % L)] & (s > 0 ? 2 : 1);
    }
    std::vector<uint8_t> bati_;   // WFS-137 (ANO-81)
    // WFS-137 (RG-01, RG-02) : optiques des feux (rouge, orange, vert) de chaque carrefour, jet de la fontaine (repère local)
    struct Optiques { int64_t abs = INT64_MIN; Mesh m[3]{}; };
    std::vector<Optiques> optiques_;
    // abords d'un rond-point (repère de la route au centre de l'anneau) : amorce (bras = −1 : demi-longueur a = départ, b = longueur,
    // h = demi-largeur) ou immeuble (a, b : demi-longueur et demi-profondeur, h : hauteur, k : teinte)
    struct Abord { vec3 c, u, n; float a, b, h; int k, bras; };
    std::vector<std::vector<Abord>> abords_;
    Mesh jet_{};
    Material lentilleEteinte_, lentille_[3], eauJet_;
    bool active_ = false, nuit_ = false;   // nuit_ : WFS-94
    const ville::Plan* plan_ = nullptr;
    int64_t depart_ = 0;   // banc d'essai : départ du joueur avancé de depart_ segments (WFS_DEPART)
    bool pilote_ = false;  // pilote automatique vu à l'image de temps piloteT_ (GameView::time)
    double piloteT_ = 0;
    Material mats_[P_COUNT];
    Image facadeTex_[2], leafTex_{}, barkTex_{}, palTex_{};
    struct Chunk { std::array<Mesh, P_COUNT> m{}; bool lod = false; };
    std::map<int64_t, Chunk> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
    int lastDraws_ = 0;
};

} // namespace

WFS_SYSTEM(Ville, 100)

} // namespace wfs
