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
#include "../game/cars.h"          // WFS-175 : voitures en sens inverse
#include "../game/roues.h"
#include "../game/trace_cartes.h"   // WFS-175 : sections du tracé 2, bas-côtés (rayonInterieur)
#include "../game/traffic.h"        // WFS-175 : TRAFFIC_PAINTS
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "../render/eclairage.h"   // WFS-09
#include "../render/effets_particules.h"   // WFS-94 : effets::versHdr (fenêtres allumées)
#include "../../third_party/stb/stb_image.h"
#include "../core/platform.h"
#include "decor_carte.h"
#include "enseignes.h"   // WFS-175 : atlas de textes (portiques)
#include "vegetation.h"
#include "ville_quartiers.h"
#include "archi_carte.h"   // WFS-161 (accroche) : immeubles du kit d'architecture
#include "zones.h"
#include "chaussee.h"   // WFS-212 : rien sur la chaussée d'une autre partie du tracé
#include "giratoire.h"  // WFS-248 : giratoire reconstruit (anneau, îlot, raccord des branches)
#include "../render/route_revetements.h"   // WFS-205 (accroche E9)

namespace wfs {
namespace {

enum Part { P_FACADE_A, P_FACADE_B, P_METAL, P_BARK, P_CORE, P_LEAF0, P_LEAF1, P_LAMP, P_UNI, P_CONIF, P_TROTTOIR, P_FEN_CHAUD, P_FEN_FROID, P_FEN_RARE,
           P_BETON, P_VOUTE, P_ACIER, P_SODIUM, P_ROUTE, P_EAU, P_TEXTE, P_SOL,
           P_OR, P_VERRE, P_TXT_A, P_TXT_B, P_COUNT };   // WFS-176 : dorures, vitrages, néons du casino (A) et des autres enseignes (B)   // WFS-175 : autoroute, tunnel, pont (P_TEXTE : matériau de l'atlas)   // WFS-94 : trottoir (ANO-49), fenêtres allumées la nuit (ANO-37)

// couleurs unies du jeu web (C2, DECOR.*, BLD_C, GCOL2, BARK) : palette d'une ligne, une case par couleur
enum Couleur {
    U_METAL, U_BANC, U_VITRE, U_PIERRE, U_EAU, U_ENTREPOT0, U_TOLE = U_ENTREPOT0 + 4, U_PORTE, U_CONT0, U_GRUE = U_CONT0 + 6,
    U_CABINE, U_ZEBRA, U_DALLE, U_GARDE_CORPS, U_PELOUSE, U_BETON, U_CHENE2, U_CHENE, U_PIN, U_PIN_BAS,
    U_BLEU, U_ASPHALTE, U_BORDURE,   // WFS-137 : panneaux et plaques bleus, rue transversale, bordures de l'îlot
    U_PEINTURE, U_CLAIR, U_BETON2, U_ACIER_LOIN,   // WFS-175 : marquage, parements du tunnel, béton (piles, lointain), acier au loin
    // WFS-176 : pierre claire du casino, marches, tapis, enduit et sol des garages, pont élévateur, noir, pigeon (4 teintes),
    // shiba et husky (6 teintes chacun, matériaux du modèle), laisse, enrobé du parking, aluminium des menuiseries
    U_CREME, U_MARCHE, U_TAPIS, U_ENDUIT, U_SOL_GARAGE, U_PONT, U_NOIR, U_PIG0, U_SHI0 = U_PIG0 + 4, U_HUS0 = U_SHI0 + 6,
    U_LAISSE = U_HUS0 + 6, U_PARKING, U_ALU, U_NB
};
constexpr uint32_t PALETTE[U_NB] = {
    0x3a3d44, 0x6b4a2e, 0xbfe8ff, 0xc9c4b8, 0x4aa3c7,
    0x8a8f96, 0xa0723c, 0x5d6b78, 0x8e3b2e,                       // C2.wh
    0x55585e, 0x2c2f35,                                           // toit et portes des entrepôts
    0xb8352a, 0x1f5f9a, 0xd8a12a, 0x3a7d44, 0x7a3fa0, 0xd06a1d,   // C2.cont
    0xd9a21b, 0xe8e8e8, 0xe9e9e6, 0xcfcac0, 0x3a3d42,             // grue, cabine, passage piéton, dalle, garde-corps
    0x4e7a34, 0x6e6e6a,                                           // GCOL2.park, GCOL2.concrete
    0x5c4c3c, 0x4a3b2e, 0x8a5236, 0x4e4038,                       // BARK.oak2, oak, pine, pineLow (cases voisines)
    0x1d5fb4, 0x3b3e43, 0xbdb9b0,                                 // WFS-137 : bleu signalisation, asphalte, bordure
    // WFS-175 : peinture routière blanche (sRGB 0xdcdcd6 → albédo ≈ 0,72, charte : marquage blanc 0,65-0,80), parement clair du tunnel (≈ 0,57),
    // béton des piles (sRGB 0x908e89 → albédo ≈ 0,27, charte : béton 0,25-0,35)
    0xdcdcd6, 0xc6c4bd, 0x908e89, 0x9a9ea3,   // acier galvanisé vu de loin (≈ 0,33, diffus : au-delà de 200 m)
    // WFS-176 : pierre claire 0xc2b9a5 (albédo ≈ 0,48, charte : pierre claire 0,40-0,55), marches 0xb5ad9c (≈ 0,42), tapis
    // rouge, enduit gris clair (≈ 0,47), sol de garage en résine (≈ 0,21), pont élévateur rouge-orangé, noir ;
    // pigeon biset (Columba livia) : corps gris-bleu, ailes plus sombres, barres et queue gris foncé, bec ;
    // shiba (roux, noir, crème, yeux) et husky (gris foncé, noir, blanc, yeux) : couleurs des matériaux du modèle Quaternius ;
    // laisse rouge, enrobé du parking (≈ 0,06, charte : asphalte neuf 0,04-0,07), aluminium anodisé des menuiseries
    0xc2b9a5, 0xb5ad9c, 0x7a1c22, 0xb9b6ae, 0x7d8085, 0xc2461e, 0x1a1b1e,
    0x9aa0ac, 0x6e7480, 0x3a3d44, 0x2f2f33,
    0x99703d, 0x2e2e2e, 0xe8dcc4, 0x0e0e0e, 0xeeeeee, 0x2a1d10,
    0x47494a, 0x2e2e2e, 0xd9d9d4, 0x0e0e0e, 0xf0f0f0, 0x2a2a2a,
    0xb02a2a, 0x45474b, 0xc8ccd0};
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
        { Material m; m.flags = MAT_WALK; m.rough = 0.85f; mats_[P_TROTTOIR] = route::creerMateriau(r, route::Revetement::BetonTrottoir, m); }   // comme le trottoir de Game (WFS-205, accroche)
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
        archi::carte::vider();   // WFS-161 (accroche, ST-5) : immeubles posés dans le kit (6 styles) ; repli WFS_ARCHI=0 : anciennes boîtes
        for (int64_t a = 0, L = (int64_t)plan_->objets.size(); a < L; a++) for (const ville::Objet& o : plan_->objets[(size_t)a]) if (o.t == ville::IMMEUBLE) {
            const carte::Pose pm = poseS(*g.track, (double)a + o.len / SEG_M / 2); const float s = (float)o.side;
            const vec3 x = pm.f * -s, c = pm.o + pm.r * (s * ((float)RW + SET_BACK));   // façade face à la rue, x dans son sens
            archi::carte::poser(a, c - x * (o.len / 2), x, pm.r * -s, o.len, o.d, o.h, o.balcons || o.h < 20 ? archi::Style::Classique : archi::Style::Moderne, (uint32_t)(a * 2 + (o.side > 0)));
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
        initOuvrages(r, *g.track);   // WFS-175
        initLieux(r, *g.track);      // WFS-176
        size_t nl = 0;
        for (size_t a = 0; a < plan_->objets.size(); a++) for (const ville::Objet& o : plan_->objets[a])
            if (o.t == ville::LAMPE) { lampadaires().push_back({(int64_t)a, o.side * ((float)RW - 1.6f), 7.1f, StyleLampadaire::Ville}); nl++; }
        WFS_LOG("Ville : %zu lampadaires%s", nl, depart_ ? " (départ avancé, banc d'essai)" : "");
        if (lots::g_lotsDecor) {   // WFS-268 : tout le décor fixe d'un tour, construit ici (écran de chargement), envoyé au GPU une fois
            limiteTour_ = (int64_t)plan_->objets.size();
            lots_.preparer(r, *g.track, P_COUNT, 4, [this, &g](int64_t c, bool lod, std::vector<Vertex>* v, std::vector<uint32_t>* i) {
                Geo out[P_COUNT];
                buildChunk(*g.track, c, lod, out);
                for (int p = 0; p < P_COUNT; p++) { v[p] = std::move(out[p].v); i[p] = std::move(out[p].i); }
            }, "ville");
            limiteTour_ = INT64_MAX;
            for (const ville::Carrefour& cf : plan_->carrefours) lieuxAnimes(r, g, cf.ligne);   // optiques des feux, tour 0 (rien n'est dessiné hors image)
        }
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
            if (archi::carte::actif()) break;   // WFS-161 (accroche) : bâtiment du kit à la place de la boîte (archi_carte.cpp)
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
        const int64_t L = (int64_t)P.objets.size(), a0 = c * CH, a1 = std::min(a0 + CH, limiteTour_);   // WFS-268 : borné au tour à la préparation
        solChunk(t, c, out[P_SOL]);   // WFS-175 : bas-côtés de Game, abaissés sous l'eau de la rivière ; WFS-212 : toujours (coupés au rond-point)
        trottoirChunk(t, c, out[P_TROTTOIR]);           // WFS-212 : trottoir de Game (bordure, RW à RW + 4), dessiné par la Ville
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
                const uint8_t vo = P.voie[(size_t)seg];   // WFS-175 : pas de trottoir sous la voûte, sur le pont, ni côté chaussée opposée
                for (int s : {-1, 1}) {
                    if (vo == ville::TUNNEL || vo == ville::PONT || (vo == ville::AUTOROUTE && s < 0)) continue;
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
                const uint8_t vo = P.voie[(size_t)seg];
                for (int s : {-1, 1}) for (int i = 0; i < nb; i++) {
                    if (vo != ville::RUE && (s < 0 || vo != ville::AUTOROUTE)) continue;   // WFS-175 : chaussée opposée, tunnel, rivière
                    const ville::Bande& b = bd[i];
                    const float l0 = P.lim(a, s), l1 = P.lim(a + 1, s);   // WFS-137 (RG-C1) : côté intérieur des courbes serrées
                    // WFS-137 (RG-A) : devant un immeuble, la pelouse ou le béton commencent derrière le trottoir prolongé
                    const float ba = i == 0 && facade(a, s) ? std::max(b.a, (float)RW + FACADE_TROTTOIR) : b.a;
                    if (ba >= std::min(l0, l1) || ba >= b.b) continue;
                    carte::quadSol(out[P_UNI], carte::lateral(p, s * ba, b.ya), carte::lateral(p1, s * ba, b.ya),
                                   carte::lateral(p1, s * std::min(b.b, l1), b.yb), carte::lateral(p, s * std::min(b.b, l0), b.yb), col);
                }
            }
            for (const ville::Objet& o : P.objets[(size_t)seg]) {
                // WFS-212 (RG-01) : au rond-point, un objet du bord (lampadaire, banc, arbre…) qui retombe sur la chaussée d'une autre
                // partie du tracé n'est pas posé (les immeubles sont déjà écartés de toute la route : abordsRondPoint)
                if (o.t != ville::IMMEUBLE && surAutreChaussee(carte::lateral(p, (float)o.side * o.lat), seg, 1.0f)) continue;
                if (o.t != ville::IMMEUBLE && surGiratoire(carte::lateral(p, (float)o.side * o.lat), 1.0f)) continue;   // WFS-248
                objet(t, o, a, seg, z, p, lod, out);
            }
            lieux(t, a, seg, p, lod, out);   // WFS-137 : rond-point et carrefours du tracé 2
            for (const ville::Lieu& l : P.lieux) if (l.seg == seg) lieuVille(t, a, l, lod, out);   // WFS-176
            if (!P.voie.empty() && P.voie[(size_t)seg] != ville::RUE) {   // WFS-175 : autoroute, tunnel, pont
                if (!lod) ouvrages(t, a, seg, false, out);
                else {   // au-delà de 200 m : béton, acier, asphalte et voûte dans la palette (un seul matériau), sans lettres
                    Geo tmp[P_COUNT];
                    ouvrages(t, a, seg, true, tmp);
                    struct Fond { int part, couleur; float k; };
                    const Fond fonds[] = {{P_BETON, U_BETON2, 1.f}, {P_VOUTE, U_BETON2, 0.4f}, {P_ACIER, U_ACIER_LOIN, 1.f}, {P_ROUTE, U_ASPHALTE, 1.f}};
                    for (int q = 0; q < P_COUNT; q++) {
                        if (q == P_TEXTE || tmp[q].i.empty()) continue;
                        const Fond* f = nullptr;
                        for (const Fond& x : fonds) if (x.part == q) f = &x;
                        Geo& dst = f ? out[P_UNI] : out[q];
                        const uint32_t base = (uint32_t)dst.v.size();
                        for (Vertex w : tmp[q].v) {
                            if (f) { w.u = uvc(f->couleur).x; w.v = uvc(f->couleur).y; w.nx *= f->k; w.ny *= f->k; w.nz *= f->k; }
                            dst.v.push_back(w);
                        }
                        for (uint32_t i : tmp[q].i) dst.i.push_back(base + i);
                    }
                }
            }
        }
        horsChaussee(out[P_TROTTOIR]);   // WFS-212 (RG-01) : rien sur la chaussée d'une autre partie du tracé (rond-point)
        horsChaussee(out[P_SOL]);
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
            // WFS-248 : le cédez-le-passage de WFS-223 (en travers de toute la chaussée, de travers) est remplacé par celui de
            // l'ouvrage reconstruit (giratoire.cpp : voie entrante seulement, au bord de l'anneau)
            if (seg != mod(rp.centre)) continue;
            const double c = t.segAt(a).curve;
            const float kc = c > 0 ? 1.f : -1.f;
            const size_t irp = (size_t)(&rp - P.ronds.data());
            // WFS-248 : ouvrage reconstruit (anneau de 9 m, couronne, îlot de 8 m, raccord des branches) ; le centre vient de
            // l'ouvrage (cercle passant par l'axe de l'anneau) ; îlot plus grand : fontaine et platanes s'y posent
            const giratoire::Giratoire* gr = irp < gir_.size() && gir_[irp].ok ? &gir_[irp] : nullptr;
            const vec3 ctr = gr ? gr->c : p.o + p.r * (rp.rayon * kc);
            const float ri = gr ? giratoire::R_ILOT : rp.rayon - R - 1.5f;
            if (gr) {
                std::vector<giratoire::Tri> tris; giratoire::geometrie(*gr, t, tris);
                const vec2 uvs[giratoire::NB_SURFACES] = {uvc(U_ASPHALTE), uvc(U_ZEBRA), uvc(U_DALLE), uvc(U_BORDURE), uvc(U_PELOUSE)};
                for (const giratoire::Tri& tr : tris) {
                    if (tr.s != giratoire::ASPHALTE) { carte::tri(U, tr.a, tr.b, tr.c, uvs[tr.s]); continue; }
                    // asphalte : matériau de la chaussée de la route (même grain), uv en mètres sans aucun marquage
                    Geo& G = out[P_ROUTE];
                    const uint32_t k0 = (uint32_t)G.v.size();
                    for (const vec3& q : {tr.a, tr.b, tr.c}) { const vec2 u = giratoire::uvChaussee(*gr, q); G.v.push_back({q.x, q.y, q.z, 0, 1, 0, u.x, u.y}); }
                    G.i.insert(G.i.end(), {k0, k0 + 1, k0 + 2});
                }
            }
            auto monde = [&](vec3 l) { return p.o + p.r * l.x + p.f * l.z + vec3{0, l.y, 0}; };
            auto dir = [&](vec3 l) { return p.r * l.x + p.f * l.z; };
            if (irp < abords_.size()) for (const Abord& ab : abords_[irp]) {
                const vec3 c = monde(ab.c), u = dir(ab.u), n = dir(ab.n);
                if (ab.k < 0) {   // amorce : asphalte (au ras de l'anneau) et ses deux bordures de trottoir
                    const vec3 a0 = c + u * ab.a, a1 = c + u * (ab.a + ab.b);
                    if (gr) {   // WFS-248 : même asphalte que l'anneau et la route (matériau de la chaussée, sans marquage)
                        const vec3 Q[4] = {a0 - n * ab.h + vec3{0, giratoire::Y_ANNEAU, 0}, a0 + n * ab.h + vec3{0, giratoire::Y_ANNEAU, 0},
                                           a1 + n * ab.h + vec3{0, giratoire::Y_ANNEAU, 0}, a1 - n * ab.h + vec3{0, giratoire::Y_ANNEAU, 0}};
                        Geo& G = out[P_ROUTE];
                        const uint32_t k0 = (uint32_t)G.v.size();
                        for (const vec3& q : Q) { const vec2 u = giratoire::uvChaussee(*gr, q); G.v.push_back({q.x, q.y, q.z, 0, 1, 0, u.x, u.y}); }
                        if (cross(Q[1] - Q[0], Q[2] - Q[0]).y > 0) G.i.insert(G.i.end(), {k0, k0 + 1, k0 + 2, k0, k0 + 2, k0 + 3});
                        else G.i.insert(G.i.end(), {k0, k0 + 2, k0 + 1, k0, k0 + 3, k0 + 2});
                    } else
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
            if (!gr) {   // repli (ouvrage non reconnu) : îlot de WFS-137
                carte::cyl(U, b, ri, 0.22f, 28, uvc(U_BORDURE));
                disque(b + vec3{0, 0.225f, 0}, ri - 0.25f, 28, uvc(U_PELOUSE));   // gazon (la bordure de 25 cm reste visible)
            }
            const vec3 g = gr ? ctr + vec3{0, giratoire::H_BORDURE, 0} : b + vec3{0, 0.22f, 0};   // WFS-248 : sur le gazon de l'îlot
            carte::cyl(U, g, 2.4f, 0.55f, 20, uvc(U_PIERRE));            // bassin (4,8 m)
            disque(g + vec3{0, 0.57f, 0}, 2.15f, 20, uvc(U_EAU));       // eau du bassin
            carte::cyl(U, g, 0.3f, 1.75f, 10, uvc(U_PIERRE));            // colonne
            carte::cyl(U, g + vec3{0, 1.75f, 0}, 1.0f, 0.18f, 16, uvc(U_PIERRE));   // vasque
            disque(g + vec3{0, 1.95f, 0}, 0.85f, 16, uvc(U_EAU));
            Mulberry32 Rt((uint32_t)(rp.centre * 31 + 7));
            Geo* leaf[2] = {&out[P_LEAF0], &out[P_LEAF1]};
            for (int k = 0; k < 3; k++) {   // trois platanes autour du bassin, entre la fontaine et la bordure
                const float an = 2 * PI * k / 3 + 0.5f, d = gr ? ri - 2.5f : std::min(ri - 1.f, 3.9f);   // WFS-248 : îlot de 8 m
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

    // WFS-175 : sections du tracé 2 (tunnel, pont, autoroute), matériaux, portiques, candélabres, voitures en sens inverse
    void initOuvrages(Renderer& r, Track& t) {
        const int64_t L = (int64_t)plan_->objets.size();
        auto mod = [&](int64_t x) { return ((x % L) + L) % L; };
        bool vu = false;
        int64_t auDeb = -1, auFin = -1;
        for (const trace::Section& s : trace::sections(t)) {
            if (s.type == trace::Type::Tunnel && tun_.first < 0) tun_ = {mod(s.debut), mod(s.fin)};
            if (s.type == trace::Type::Pont && riv_.deb < 0) {
                riv_.deb = mod(s.debut); riv_.fin = riv_.deb + (s.fin - s.debut);
                const int64_t berge = (int64_t)std::lround(20 / SEG_M);   // 20 m de culée de chaque côté
                riv_.d0 = riv_.deb + berge; riv_.d1 = riv_.fin - berge; riv_.mil = (riv_.deb + riv_.fin) / 2;
                riv_.yw = (float)t.point((double)s.debut * SEG, 0).y - PONT_EAU;
                if (riv_.d1 >= L) riv_ = Riviere{};   // pont à cheval sur la fin du tour : pas de rivière (n'arrive pas au tracé 2)
            }
            if (s.autoroute && !vu) { if (auDeb < 0) auDeb = s.debut; auFin = s.fin; }   // premier tronçon continu d'autoroute (tunnel compris)
            else if (auDeb >= 0) vu = true;
        }
        if (auDeb >= 0) au_ = {mod(auDeb), mod(auDeb) + (auFin - auDeb)};
        bool ouvrage = false;
        for (uint8_t v : plan_->voie) ouvrage = ouvrage || v != ville::RUE;
        if (!ouvrage) return;
        {   // béton de coffrage (texture procédurale, 2,4 × 2,4 m) : albédo moyen ≈ 0,30 (charte : 0,25-0,35), joints de banches tous
            // les 1,2 m, trous d'entretoises, nuances de prise ; rugosité 0,9, jamais métallique
            constexpr int N = 256;
            std::vector<uint8_t> px((size_t)N * N * 4);
            Mulberry32 Rb(1750);
            float bruit[32][32];
            for (auto& l : bruit) for (float& b : l) b = (float)Rb.next();
            auto lisse = [&](float x, float y) {   // bruit de valeur, périodique sur 32 cases
                const int x0 = (int)std::floor(x), y0 = (int)std::floor(y); const float fx = x - x0, fy = y - y0;
                auto B = [&](int i, int j) { return bruit[((j % 32) + 32) % 32][((i % 32) + 32) % 32]; };
                const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
                return (B(x0, y0) * (1 - sx) + B(x0 + 1, y0) * sx) * (1 - sy) + (B(x0, y0 + 1) * (1 - sx) + B(x0 + 1, y0 + 1) * sx) * sy;
            };
            for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
                const float u = (float)x / N * 32, v = (float)y / N * 32;
                float a = 0.30f * (0.86f + 0.18f * lisse(u / 4, v / 4) + 0.08f * lisse(u * 2, v * 2) + 0.06f * (float)Rb.next() - 0.1f);
                const int jx = x % (N / 2), jy = y % (N / 2);
                if (jy < 2 || jx < 1) a *= 0.8f;                                                   // joints de banches (1,2 m)
                const int tx = (x % (N / 2)) - N / 4, ty = (y % (N / 2)) - N / 4;
                if (tx * tx + ty * ty < 9) a *= 0.55f;                                             // trou d'entretoise
                const uint8_t c = (uint8_t)std::clamp(std::pow(std::max(a, 0.f), 1 / 2.2f) * 255.f + 0.5f, 0.f, 255.f);
                uint8_t* q = &px[((size_t)y * N + x) * 4];
                q[0] = c; q[1] = (uint8_t)std::max(0, c - 2); q[2] = (uint8_t)std::max(0, c - 5); q[3] = 255;
            }
            betonTex_ = r.vk().uploadTexture(px.data(), N, N, true);
            // éclairé d'un seul côté (pas MAT_VTINT) : les lumières du tunnel n'éclairent pas la face extérieure des têtes
            Material m; m.rough = 0.9f; m.flags = MAT_DOUBLE;
            mats_[P_BETON] = r.makeMaterial(m, &betonTex_, nullptr, nullptr);
            // voûte : le moteur n'a ni occultation ambiante ni éclairage d'environnement (DA-realisme.md § 1) ; sous la dalle, la
            // lumière du ciel n'arrive pas : couleur × 0,4 (comme la voûte de la Corniche, × 0,3 sous la roche)
            m.color = {0.4f, 0.4f, 0.4f, 1};
            mats_[P_VOUTE] = r.makeMaterial(m, &betonTex_, nullptr, nullptr);
        }
        // acier galvanisé terni (glissières, portiques, arcs, mâts) : métal, albédo ≈ 0,56 (zinc terni), rugosité 0,6 (galvanisation
        // à chaud vieillie, mate : moins de reflets des lampadaires sur les mâts la nuit)
        { Material m; m.color = {0.56f, 0.57f, 0.58f, 1}; m.metal = 1; m.rough = 0.6f; m.flags = MAT_DOUBLE; mats_[P_ACIER] = r.makeMaterial(m, nullptr, nullptr, nullptr); }
        // luminaires au sodium haute pression ≈ 2000 K (DA-realisme.md : 1900-2200 K), allumés jour et nuit (tunnel)
        { Material m; m.color = {0.2f, 0.2f, 0.2f, 1}; m.rough = 0.5f; m.flags = MAT_DOUBLE; m.emissive = vec3{1.f, 0.42f, 0.09f} * (nuit_ ? 3.f : 5.f);
          mats_[P_SODIUM] = r.makeMaterial(m, nullptr, nullptr, nullptr); }
        { Material m; m.flags = MAT_ROAD; m.rough = 0.8f; mats_[P_ROUTE] = route::creerMateriau(r, route::Revetement::EnrobeUse, m); }   // asphalte de Game (WFS-205, accroche)
        // eau de rivière : sombre (albédo ≈ 0,04), lisse (rugosité 0,08), F0 diélectrique 0,02 : le ciel s'y reflète
        { Material m; m.color = {0.035f, 0.05f, 0.055f, 1}; m.rough = 0.08f; m.metal = 0; mats_[P_EAU] = r.makeMaterial(m, nullptr, nullptr, nullptr); }
        { Material m; m.flags = MAT_GROUND; m.rough = 0.95f; mats_[P_SOL] = r.makeMaterial(m, nullptr, nullptr, nullptr); }   // = sol de Game
        // portiques : 3 au plus sur l'autoroute, à 150 m du début puis tous les 420 m (pas dans les 60 m avant le tunnel)
        static const char* const DIR[3][2] = {{"Centre", "Port"}, {"Port", "Aéroport"}, {"Aéroport", "Centre"}};
        if (au_.first >= 0) {
            for (int64_t sgm = au_.first + (int64_t)(150 / SEG_M); sgm < au_.second && portiques_.size() < 3; sgm += 5) {
                bool ok = true;
                for (int64_t k = -2; k <= (int64_t)(60 / SEG_M) && ok; k++) ok = plan_->voie[(size_t)mod(sgm + k)] == ville::AUTOROUTE;
                if (!ok) continue;
                const int i = (int)portiques_.size();
                portiques_.push_back({mod(sgm), DIR[i][0], DIR[i][1]});
                sgm += (int64_t)(420 / SEG_M) - 5;
            }
            for (int i = 0; i < 3; i++) for (const char* x : DIR[i]) atlas_.ajouter(x);
            if (atlas_.construire(r)) texteMat_ = &atlas_.materiau(r, 0xf4f4f0, nuit_ ? 0.45f : 0.12f, 0.45f);
            // voitures en sens inverse : deux modèles qui ne sont pas ceux du trafic (variété), version allégée au loin
            for (int i = 0; i < 2; i++) if (!loadCarModel(r, i ? "cars/x5.wfm" : "cars/p208.wfm", autos_[i])) WFS_ERR("Ville : modèle du sens inverse introuvable");
        }
        // éclairage de nuit (nuit.cpp, les 4 plus proches) : candélabres de l'autoroute, du pont, points du tunnel tous les 14,4 m
        for (int64_t a = 0; a < L; a++) {
            const uint8_t v = plan_->voie[(size_t)a];
            if (v == ville::AUTOROUTE && a % LAMPE_AR == 0 && !presPortique(a)) lampadaires().push_back({a, -((float)RW + AR_DBA - 2.5f), 11.8f, StyleLampadaire::Ville});
            if (v == ville::PONT && a % LAMPE_PONT == 0) {
                const float sd = ((a / LAMPE_PONT) % 2 == 0) ? 1.f : -1.f;
                lampadaires().push_back({a, sd * ((float)RW + 4.25f - 1.3f), 7.4f, StyleLampadaire::Ville});
            }
        }
        if (tun_.first >= 0) for (int64_t n = tun_.first + 21, k = 0; n < tun_.first + mod(tun_.second - tun_.first) - 21; n += 12, k++)
            lampadaires().push_back({mod(n), (k % 2 ? 1.f : -1.f) * 1.8f, TUN_H - 0.4f, StyleLampadaire::Ville});
        WFS_LOG("Ville (WFS-175) : autoroute %lld-%lld, tunnel %lld-%lld, pont %lld-%lld (eau à %.1f m), %zu portique(s)",
                (long long)au_.first, (long long)au_.second, (long long)tun_.first, (long long)tun_.second, (long long)riv_.deb, (long long)riv_.fin,
                riv_.yw, portiques_.size());
    }

    // WFS-175 : voitures en sens inverse sur la chaussée opposée (3 voies ; 94 à 119 km/h, la voie de gauche la plus rapide), à
    // horaire fixe (même temps → même position), 2 au plus dans le champ (coût : ≈ 30 appels de dessin chacune) (−30 à 280 m) ; phares allumés la nuit (feux du modèle)
    int sensInverse(Renderer& r, const GameView& g, int64_t aP) {
        if (au_.first < 0 || !autos_[0].ok) return 0;
        Track& t = *g.track;
        const int64_t L = (int64_t)plan_->objets.size();
        const double lon = (double)(au_.second - au_.first) * SEG_M;
        int n = 0;
        for (int i = 0; i < 18 && n < 2; i++) {
            const int vi = i % 3;
            const double v = vi == 0 ? 33.0 : vi == 1 ? 30.0 : 26.0;
            const double m = lon - std::fmod(i * 83.0 + 29.0 * vi + v * g.time, lon);
            const int64_t sl = au_.first + (int64_t)(m / SEG_M);
            const double sA = (double)proche(sl % L, aP) + std::fmod(m, SEG_M) / SEG_M;
            const double dz = (sA - g.player->dist / SEG) * SEG_M;
            if (dz < -30 || dz > 280) continue;
            const float lat = -((float)RW + 8.05f + 3.5f * vi);
            t.pose((int64_t)sA - 1); t.pose((int64_t)sA + 2);
            const TrackPoint p = t.point(sA * SEG, lat);
            const Model& mo = autos_[i % 2];
            const mat4 w = translate({(float)p.x, (float)p.y + 0.15f, (float)p.z}) * rotateY((float)-(p.th + PI)) * rotateX((float)-p.slope);
            const vec3 paint = carte::linHex(TRAFFIC_PAINTS[(i * 5 + 2) % 9]);
            const RouesPose rp = roues::poseTrafic(0, false, v * g.time / M, mo.nRoues ? mo.roues[0].rayon : roues::RW_DEFAUT);
            FeuxVoiture f; f.nuit = nuit_;
            r.feuxProchaineVoiture(f);
            r.draw(mo, w, &paint, &rp);
            n++; piecesInv_ += (int)mo.parts.size();   // borne haute des appels (version allégée au loin : moins)
        }
        return n;
    }

    // ======================================================================================================================
    // WFS-175 (RG-03 de WFS-137) : autoroute urbaine, tunnel et pont du tracé 2. Dimensions réelles (source en commentaire) :
    //   - autoroute : chaussée opposée de 3 voies de 3,50 m, bande dérasée de gauche 1,00 m, bande d'arrêt d'urgence 2,50 m
    //     (ICTAVRU, voies rapides urbaines de type A) ; marquage de 15 cm (2u, u = 7,5 cm sur autoroute), lignes T1 de
    //     3 m de trait pour 10 m d'intervalle entre les voies (IISR, 7e partie, art. 113 et 115) ; séparateur en béton
    //     double face (DBA, profil « New Jersey » : 0,80 m de haut, 0,60 m à la base) ; glissière GS (lisse en W de 0,31 m,
    //     haut à 0,75 m, supports tous les 4 m, NF EN 1317 / SETRA) ; écran antibruit de 4 m ; candélabres de 12 m tous les
    //     36 m (interdistance ≈ 3 hauteurs) ; portiques de signalisation : 5,50 m de hauteur libre sous les panneaux (IISR,
    //     5e partie), panneaux bleus de 4,6 × 2,2 m, lettres blanches de 0,55 m ;
    //   - tunnel : deux tubes (un par sens), gabarit et voûte du tunnel de la Corniche (piédroits 3,2 m, clé 7,6 m, CETU),
    //     parements clairs jusqu'à 3,2 m ; luminaires au sodium haute pression (≈ 2000 K), zone de seuil renforcée sur les
    //     60 premiers mètres et 30 derniers (deux rangées, un tube de 1,5 m tous les 3,6 m), puis zone intérieure (une rangée,
    //     un tube tous les 6 m) : éclairage de seuil plus fort que l'intérieur (CIE 88:2004) ;
    //   - pont : deux arcs en acier (bow-string) de 100 m de portée et 16 m de flèche (rapport 1/6), suspentes tous les 6 m,
    //     tirants en caisson ; tablier de 1,4 m (corniche à 0,25 m au-dessus du trottoir), garde-corps de 1,00 m au-dessus du
    //     trottoir (NF P 01-012 : ≥ 1,00 m), pile centrale avec avant-becs, quais en béton, eau à 5,5 m sous les culées.
    // ======================================================================================================================
    static constexpr float TUN_PIED = 3.2f, TUN_H = 7.6f, TUN_DESSUS = 8.4f;   // piédroits, clé de voûte, dessus de la dalle (m)
    static constexpr float AR_TPC = 4.f, AR_TPC_FIN = 5.3f, AR_DBA = 4.95f;     // terre-plein central : de RW + 4 à RW + 5,3 ; DBA au milieu
    static constexpr float AR_OPP0 = 5.3f, AR_OPP1 = 19.3f;                      // chaussée opposée : BDG 1 + 3 × 3,5 + BAU 2,5 = 14 m
    static constexpr float AR_GS_D = 3.6f, AR_GS_G = 19.6f, AR_ECRAN = 20.5f;    // glissières droite et gauche, écran antibruit
    static constexpr float TUN_OPP_C = 12.3f, TUN_OPP_W = 7.3f;                  // tube opposé : centre −(RW + 12,3), demi-largeur
    static constexpr float PONT_CORN = 4.45f, PONT_ARC = 5.f, PONT_SOUS = -1.25f, PONT_EAU = 5.5f, PONT_FLECHE = 16.f;
    static constexpr int LAMPE_AR = 30, LAMPE_PONT = 25;                         // segments entre deux candélabres (36 m, 30 m)
    static constexpr float TUILE_BETON = 2.4f;                                   // m par tuile de la texture du béton

    // point de la route au segment (fractionnaire) s, latéral lat (m), hauteur y au-dessus de la chaussée
    static vec3 Wp(Track& t, double s, float lat, float y) { const TrackPoint p = t.point(s * SEG, lat); return {(float)p.x, (float)p.y + y, (float)p.z}; }
    // quad à normale calculée (côté avant = ordre a, b, c), longueur k de la normale (luminosité des matériaux MAT_VTINT)
    static void quadN(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 ua, vec2 ub, vec2 uc, vec2 ud, float k = 1) {
        vec3 n = cross(b - a, c - a); const float l = length(n);
        n = l > 1e-9f ? n * (k / l) : vec3{0, k, 0};
        const uint32_t i = (uint32_t)g.v.size();
        for (auto [p, u] : {std::pair{a, ua}, {b, ub}, {c, uc}, {d, ud}}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y});
        g.i.insert(g.i.end(), {i, i + 1, i + 2, i, i + 2, i + 3});
    }
    // profil transversal (lat, y) extrudé de s0 à s1 (segments) ; uv en mètres / tuile (u le long du profil, v le long de la route)
    static void extrude(Track& t, Geo& g, double s0, double s1, const vec2* pr, int n, float tuile, float k) {
        const float v0 = (float)std::fmod(s0 * SEG_M, 480.0) / tuile, v1 = v0 + (float)((s1 - s0) * SEG_M) / tuile;
        float u = 0;
        for (int i = 0; i + 1 < n; i++) {
            const float l = std::hypot(pr[i + 1].x - pr[i].x, pr[i + 1].y - pr[i].y);
            quadN(g, Wp(t, s0, pr[i].x, pr[i].y), Wp(t, s1, pr[i].x, pr[i].y), Wp(t, s1, pr[i + 1].x, pr[i + 1].y), Wp(t, s0, pr[i + 1].x, pr[i + 1].y),
                  {u / tuile, v0}, {u / tuile, v1}, {(u + l) / tuile, v1}, {(u + l) / tuile, v0}, k);
            u += l;
        }
    }
    // profil d'un tube : pied du piédroit (c + w, 0), piédroit, voûte elliptique jusqu'à la clé (TUN_H), autre piédroit
    static int profilTube(vec2 pts[24], float c, float w) {
        int n = 0;
        pts[n++] = {c + w, 0.f};
        for (int k = 0; k <= 16; k++) { const float f = PI * k / 16; pts[n++] = {c + w * std::cos(f), TUN_PIED + (TUN_H - TUN_PIED) * std::sin(f)}; }
        pts[n++] = {c - w, 0.f};
        return n;
    }
    // hauteur de la voûte de notre tube au latéral lat
    static float voute(float lat) { const float q = std::min(1.f, std::fabs(lat) / ((float)RW + 4.f)); return TUN_PIED + (TUN_H - TUN_PIED) * std::sqrt(1 - q * q); }

    // rivière sous le pont : segments du tour (deb, fin : section ; d0, d1 : quais ; mil : pile centrale), niveau de l'eau (absolu)
    struct Riviere { int64_t deb = -1, fin = -1, d0 = -1, d1 = -1, mil = -1; float yw = 0; };
    bool dansRiviere(int64_t seg) const { return riv_.deb >= 0 && seg > riv_.d0 && seg < riv_.d1; }   // sol abaissé sous l'eau
    // portique : segment, textes des deux panneaux (gauche, droite)
    struct Portique { int64_t seg; const char* g; const char* d; };
    bool presPortique(int64_t seg) const {   // candélabre à moins de 12 m d'un portique : retiré (pas de lanterne contre les panneaux)
        for (const Portique& q : portiques_) if (std::llabs(q.seg - seg) * SEG_M < 12) return true;
        return false;
    }

    // WFS-212 (RG-01) : trottoirs de Game (Game::buildChunkCpu, bande 1, même calcul au bit près : bordure de 15 cm, de RW à RW + 4,
    // replié à 90 % du rayon intérieur des courbes serrées), dessinés par la Ville (remplaceTrottoir) pour pouvoir les couper là où
    // une autre partie du tracé passe : au rond-point, l'anneau fait le tour de l'îlot et son trottoir extérieur coupait la route
    // d'entrée (le joueur roulait sur des dalles, ANO bloquante de la recette de WFS-137)
    void trottoirChunk(Track& t, int64_t c, Geo& W) const {
        const int64_t a0 = c * CH;
        const float R = (float)RW, WALK_W = 4.0f, CURB_H = 0.15f;
        Geo G;
        const int nCH = limiteTour_ == INT64_MAX ? CH : (int)std::min<int64_t>(CH, limiteTour_ - a0);   // WFS-268 : dernier morceau borné au tour (préparation)
        auto strip = [&](const std::vector<std::pair<float, float>>& prof) {
            const uint32_t cols = (uint32_t)prof.size(), base = (uint32_t)G.v.size();
            for (int i = 0; i <= nCH; i++) {
                const double s = (double)(a0 + i) * SEG;
                const trace::Rayon ri = trace::rayonInterieur(t, s);
                for (uint32_t k = 0; k < cols; k++) {
                    const float lat = prof[k].first, lim = (float)(0.9 * (lat > 0 ? ri.droite : ri.gauche));
                    TrackPoint p = t.point(s, ri.m < 60 && std::abs(lat) > lim ? (lat > 0 ? lim : -lim) : lat);
                    G.v.push_back({(float)p.x, (float)(p.y + prof[k].second), (float)p.z, 0, 1, 0, prof[k].first, (float)((a0 + i) * SEG_M)});
                }
            }
            for (int i = 0; i < nCH; i++) for (uint32_t k = 0; k + 1 < cols; k++) {
                const uint32_t q = base + i * cols + k;
                G.i.insert(G.i.end(), {q, q + cols, q + 1, q + 1, q + cols, q + cols + 1});
            }
        };
        strip({{-R - WALK_W, CURB_H}, {-R - 0.12f, CURB_H}, {-R, CURB_H - 0.02f}, {-R, 0.f}});
        strip({{R, 0.f}, {R, CURB_H - 0.02f}, {R + 0.12f, CURB_H}, {R + WALK_W, CURB_H}});
        for (auto& v : G.v) { v.nx = v.ny = v.nz = 0; }
        for (size_t k = 0; k + 2 < G.i.size(); k += 3) {
            Vertex &A = G.v[G.i[k]], &B = G.v[G.i[k + 1]], &C = G.v[G.i[k + 2]];
            vec3 e1{B.px - A.px, B.py - A.py, B.pz - A.pz}, e2{C.px - A.px, C.py - A.py, C.pz - A.pz};
            vec3 n = cross(e1, e2);
            if (n.y < 0 && std::fabs(n.y) > 0.3f * length(n)) n = -n;
            for (Vertex* x : {&A, &B, &C}) { x->nx += n.x; x->ny += n.y; x->nz += n.z; }
        }
        for (auto& v : G.v) { vec3 n = normalize({v.nx, v.ny, v.nz}); if (length(n) < 0.5f) n = {0, 1, 0}; v.nx = n.x; v.ny = n.y; v.nz = n.z; }
        const uint32_t base = (uint32_t)W.v.size();
        W.v.insert(W.v.end(), G.v.begin(), G.v.end());
        for (uint32_t i : G.i) W.i.push_back(base + i);
    }

    // WFS-212 (RG-01) : retire d'un maillage de trottoir les triangles posés sur la chaussée d'une AUTRE partie du tracé, autour des
    // ronds-points (axe de la route à moins de RW + 0,3 m du centre du triangle, la partie du tracé du triangle lui-même exclue :
    // ± 30 m autour du segment lu dans sa coordonnée v = segment × SEG_M)

    // WFS-212 : le point q (monde) est-il sur la chaussée d'une autre partie du tracé (rond-point) ? (src/world/chaussee.h)
    bool surAutreChaussee(const vec3& q, int64_t seg, float marge) const { return chaussee::surAutre(routesRp_, (int64_t)plan_->objets.size(), q, seg, marge); }
    bool procheRondPoint(int64_t a0, int64_t a1) const { for (int64_t a = a0; a < a1; a += 4) if (chaussee::proche(routesRp_, (int64_t)plan_->objets.size(), a)) return true; return false; }

    bool surGiratoire(const vec3& q, float marge) const { for (const giratoire::Giratoire& g : gir_) if (giratoire::dansOuvrage(g, q, marge)) return true; return false; }
    void horsChaussee(Geo& W) const {
        if (routesRp_.empty() || W.i.empty()) return;
        std::vector<uint32_t> garde; garde.reserve(W.i.size());
        for (size_t k = 0; k + 2 < W.i.size(); k += 3) {
            const Vertex &A = W.v[W.i[k]], &B = W.v[W.i[k + 1]], &C = W.v[W.i[k + 2]];
            const vec3 m{(A.px + B.px + C.px) / 3, 0, (A.pz + B.pz + C.pz) / 3};
            if (surGiratoire(m, 0.3f)) continue;   // WFS-248 : rien sur l'ouvrage reconstruit (trottoirs, bas-côtés)
            if (!surAutreChaussee(m, (int64_t)std::lround(A.v / SEG_M), 0.3f)) garde.insert(garde.end(), {W.i[k], W.i[k + 1], W.i[k + 2]});
        }
        W.i.swap(garde);
    }

    // bas-côtés de Game (Game::buildChunkCpu, bande 2, même calcul au bit près : mêmes sommets, mêmes normales), sauf sous le pont
    // où ils passent sous l'eau : dessinés par la Ville à la place de Game quand la rivière est en vue (remplaceSol)
    void solChunk(Track& t, int64_t c, Geo& G) const {
        const int64_t a0 = c * CH, L = (int64_t)plan_->objets.size();
        const float R = (float)RW, WALK_W = 4.0f, CURB_H = 0.15f, GROUND_W = 260.f;
        const int nCH = limiteTour_ == INT64_MAX ? CH : (int)std::min<int64_t>(CH, limiteTour_ - a0);   // WFS-268 : dernier morceau borné au tour (préparation)
        auto strip = [&](const std::vector<std::pair<float, float>>& prof) {
            auto& V = G.v; auto& I = G.i;
            const uint32_t cols = (uint32_t)prof.size(), base = (uint32_t)V.size();
            for (int i = 0; i <= nCH; i++) {
                const double s = (double)(a0 + i) * SEG;
                const trace::Rayon ri = trace::rayonInterieur(t, s);
                const bool eau = dansRiviere((((a0 + i) % L) + L) % L);
                for (uint32_t k = 0; k < cols; k++) {
                    const float lat = prof[k].first, lim = (float)(0.9 * (lat > 0 ? ri.droite : ri.gauche));
                    TrackPoint p = t.point(s, ri.m < 60 && std::abs(lat) > lim ? (lat > 0 ? lim : -lim) : lat);
                    const float y = eau ? riv_.yw - 1.5f : (float)(p.y + prof[k].second);   // WFS-175 : lit de la rivière
                    V.push_back({(float)p.x, y, (float)p.z, 0, 1, 0, prof[k].first, (float)((a0 + i) * SEG_M)});
                }
            }
            for (int i = 0; i < nCH; i++) for (uint32_t k = 0; k + 1 < cols; k++) {
                uint32_t q = base + i * cols + k;
                I.insert(I.end(), {q, q + cols, q + 1, q + 1, q + cols, q + cols + 1});
            }
        };
        // WFS-212 : près d'un rond-point, mêmes bandes (même surface, linéaire par morceaux) découpées en colonnes de 1 m jusqu'à
        // RW + WALK_W + 40 m, pour que horsChaussee retire exactement la partie posée sur une autre branche du tracé
        std::vector<std::pair<float, float>> d = {{R + WALK_W, CURB_H - 0.02f}};
        if (procheRondPoint(a0, a0 + CH))
            for (int k = 1; k < 40; k++) { const float x = (float)k; d.push_back({R + WALK_W + x, x <= 20 ? (CURB_H - 0.02f) * (1 - x / 20) : -0.8f * (x - 20) / 60}); }
        for (auto q : std::vector<std::pair<float, float>>{{R + WALK_W + 20.f, 0.f}, {R + WALK_W + 80.f, -0.8f}, {R + WALK_W + GROUND_W, -6.f}})
            if (q.first > d.back().first) d.push_back(q);
        std::vector<std::pair<float, float>> g;
        for (auto it = d.rbegin(); it != d.rend(); ++it) g.push_back({-it->first, it->second});
        strip(g);
        strip(d);
        auto& V = G.v; auto& I = G.i;
        for (auto& v : V) { v.nx = v.ny = v.nz = 0; }
        for (size_t k = 0; k + 2 < I.size(); k += 3) {
            Vertex &A = V[I[k]], &B = V[I[k + 1]], &C = V[I[k + 2]];
            vec3 e1{B.px - A.px, B.py - A.py, B.pz - A.pz}, e2{C.px - A.px, C.py - A.py, C.pz - A.pz};
            vec3 n = cross(e1, e2);
            if (n.y < 0 && std::fabs(n.y) > 0.3f * length(n)) n = -n;
            for (Vertex* x : {&A, &B, &C}) { x->nx += n.x; x->ny += n.y; x->nz += n.z; }
        }
        for (auto& v : V) { vec3 n = normalize({v.nx, v.ny, v.nz}); if (length(n) < 0.5f) n = {0, 1, 0}; v.nx = n.x; v.ny = n.y; v.nz = n.z; }
    }

    // ouvrages du segment a (seg : sur un tour) : appelé en parallèle, lit seulement la piste et le plan
    void ouvrages(Track& t, int64_t a, int64_t seg, bool lod, Geo out[P_COUNT]) const {
        const uint8_t vo = plan_->voie[(size_t)seg];
        if (vo == ville::RUE) return;
        const float R = (float)RW;
        const double s0 = (double)a, s1 = (double)(a + 1);
        const carte::Pose p = carte::poseAt(t, a);
        const vec3 up{0, 1, 0};
        Geo& U = out[P_UNI];
        const double dist = (double)a * SEG_M;   // abscisse (m) : phase des tirets et des supports
        auto chaque = [&](double pas) { return std::floor(dist / pas) != std::floor((dist + SEG_M) / pas); };   // un repère tous les « pas » m
        // glissière GS : lisse en W (0,44 à 0,75 m) posée à y0, face vers la chaussée (dir = −1 : face vers la gauche), supports
        auto glissiere = [&](float lat, float dir, float y0) {
            static const vec2 Wl[5] = {{0, 0.44f}, {0.05f, 0.52f}, {0, 0.60f}, {0.05f, 0.67f}, {0, 0.75f}};
            vec2 pr[5];
            for (int i = 0; i < 5; i++) pr[i] = {lat - dir * Wl[i].x, y0 + Wl[i].y};
            extrude(t, out[P_ACIER], s0, s1, pr, 5, 1.f, 1.f);
            if (!lod && chaque(4.0)) carte::box(out[P_ACIER], Wp(t, s0, lat + dir * 0.12f, y0), p.r, p.f, 0.05f, 0.62f, 0.1f, {0, 0});
        };
        if (vo == ville::AUTOROUTE || vo == ville::TUNNEL) {
            // chaussée opposée (asphalte de la route, sans le marquage du moteur : uv latéral hors des lignes) et son marquage
            const float c0 = -(R + AR_OPP0), c1 = -(R + AR_OPP1), cm = (c0 + c1) / 2, y = 0.15f;
            const vec3 A = Wp(t, s0, c0, y), B = Wp(t, s1, c0, y), C = Wp(t, s1, c1, y), D = Wp(t, s0, c1, y);
            const float va = (float)dist, vb = (float)(dist + SEG_M);
            quadN(out[P_ROUTE], A, B, C, D, {c0 - cm, va}, {c0 - cm, vb}, {c1 - cm, vb}, {c1 - cm, va});   // normale vers le haut
            auto ligne = [&](float l0, float l1, double t0, double t1) {   // bande peinte de l0 à l1 (m), de t0 à t1 (segments)
                carte::quadSol(U, Wp(t, t0, l0, y + 0.012f), Wp(t, t0, l1, y + 0.012f), Wp(t, t1, l1, y + 0.012f), Wp(t, t1, l0, y + 0.012f), uvc(U_PEINTURE));
            };
            ligne(-(R + 6.15f), -(R + 6.3f), s0, s1);       // rive gauche (côté terre-plein)
            ligne(-(R + 16.8f), -(R + 16.95f), s0, s1);     // rive droite (bande d'arrêt d'urgence)
            const double ph = std::fmod(dist, 13.0);          // T1 : 3 m peints, 10 m vides
            if (ph < 3.0) {
                const double fin = std::min(SEG_M, 3.0 - ph) / SEG_M;
                for (float l : {R + 9.8f, R + 13.3f}) ligne(-(l - 0.075f), -(l + 0.075f), s0, s0 + fin);
            } else if (ph + SEG_M > 13.0) {
                const double deb = (13.0 - ph) / SEG_M;
                for (float l : {R + 9.8f, R + 13.3f}) ligne(-(l - 0.075f), -(l + 0.075f), s0 + deb, s1);
            }
        }
        if (vo == ville::AUTOROUTE) {
            // terre-plein central en béton (dalle) et séparateur DBA « New Jersey » (0,80 m, base 0,60 m)
            { const vec2 dalle[2] = {{-(R + AR_TPC), 0.15f}, {-(R + AR_TPC_FIN), 0.15f}}; extrude(t, out[P_BETON], s0, s1, dalle, 2, TUILE_BETON, 1.f); }
            const float c = -(R + AR_DBA), b = 0.15f;
            const vec2 dba[8] = {{c + 0.30f, b}, {c + 0.30f, b + 0.075f}, {c + 0.20f, b + 0.25f}, {c + 0.09f, b + 0.80f},
                                 {c - 0.09f, b + 0.80f}, {c - 0.20f, b + 0.25f}, {c - 0.30f, b + 0.075f}, {c - 0.30f, b}};
            extrude(t, out[P_BETON], s0, s1, dba, 8, TUILE_BETON, 1.f);
            glissiere(R + AR_GS_D, 1.f, 0.15f);              // à droite, sur l'accotement, face à notre chaussée
            glissiere(-(R + AR_GS_G), -1.f, 0.15f);          // à gauche, au bord de la bande d'arrêt opposée
            // écran antibruit : voile de béton de 4 m, poteaux HEA tous les 4 m
            const vec2 ecran[4] = {{-(R + AR_ECRAN - 0.1f), -0.4f}, {-(R + AR_ECRAN - 0.1f), 4.f}, {-(R + AR_ECRAN + 0.1f), 4.f}, {-(R + AR_ECRAN + 0.1f), -0.4f}};
            extrude(t, out[P_BETON], s0, s1, ecran, 4, TUILE_BETON, 1.f);
            if (!lod && chaque(4.0)) carte::box(out[P_ACIER], Wp(t, s0, -(R + AR_ECRAN - 0.2f), -0.2f), p.r, p.f, 0.16f, 4.3f, 0.16f, {0, 0});
            // candélabre de 12 m sur le terre-plein, crosses de 2,6 m vers les deux chaussées, lanternes au sodium
            if (seg % LAMPE_AR == 0 && !presPortique(seg)) {
                const vec3 pied = Wp(t, s0, c, b + 0.8f);
                carte::cyl(out[P_ACIER], pied, 0.11f, 11.2f, 8, {0, 0});
                for (float sd : {1.f, -1.f}) {
                    carte::box(out[P_ACIER], pied + p.r * (sd * 1.3f), p.r, p.f, 2.6f, 0.08f, 0.08f, {0, 0}, 11.f);
                    carte::box(out[P_SODIUM], pied + p.r * (sd * 2.5f), p.r, p.f, 0.7f, 0.12f, 0.32f, {0, 0}, 10.88f);
                }
            }
            for (const Portique& pq : portiques_) if (pq.seg == seg) portique(t, a, pq, out);
        }
        if (vo == ville::TUNNEL) {
            const int64_t L = (int64_t)plan_->objets.size();
            auto mod = [&](int64_t x) { return ((x % L) + L) % L; };
            vec2 pts[24];
            for (int tube = 0; tube < 2; tube++) {
                const float cc = tube ? -(R + TUN_OPP_C) : 0.f, w = tube ? TUN_OPP_W : R + 4.f;
                const int n = profilTube(pts, cc, w);
                // parements clairs des piédroits (couleur de la palette, luminosité 0,5 : lumière du ciel masquée) ; voûte en béton
                for (int cote : {0, n - 2}) {
                    const size_t k0 = U.v.size();
                    extrude(t, U, s0, s1, pts + cote, 2, TUILE_BETON, 0.5f);
                    for (size_t k = k0; k < U.v.size(); k++) { U.v[k].u = uvc(U_CLAIR).x; U.v[k].v = uvc(U_CLAIR).y; }
                }
                extrude(t, out[P_VOUTE], s0, s1, pts + 1, n - 2, TUILE_BETON, 1.f);
            }
            // dalle de couverture enherbée et murs extérieurs de la tranchée couverte
            const float gx = -(R + AR_ECRAN - 0.1f), dx = R + 4.8f;
            carte::quadSol(U, Wp(t, s0, gx, TUN_DESSUS), Wp(t, s0, dx, TUN_DESSUS), Wp(t, s1, dx, TUN_DESSUS), Wp(t, s1, gx, TUN_DESSUS), uvc(U_PELOUSE));
            for (float lx : {gx, dx}) { const vec2 mur[2] = {{lx, -0.6f}, {lx, TUN_DESSUS}}; extrude(t, out[P_BETON], s0, s1, mur, 2, TUILE_BETON, 1.f); }
            // luminaires au sodium : seuil (60 m après l'entrée, 30 m avant la sortie) en deux rangées serrées, puis une rangée
            const int64_t dE = mod(seg - tun_.first), dS = mod(tun_.second - 1 - seg);
            const bool seuil = dE * SEG_M < 60 || dS * SEG_M < 30;
            const bool pose = seuil ? chaque(3.6) : chaque(6.0);
            const float rangs[2] = {seuil ? -1.8f : 0.f, 1.8f};
            if (pose) for (int k = 0; k < (seuil ? 2 : 1); k++)
                carte::box(out[P_SODIUM], Wp(t, s0 + 0.5, rangs[k], 0), p.r, p.f, 0.18f, 0.09f, 1.5f, {0, 0}, voute(rangs[k]) - 0.2f);
            if (chaque(6.0)) carte::box(out[P_SODIUM], Wp(t, s0 + 0.5, -(R + TUN_OPP_C), 0), p.r, p.f, 0.18f, 0.09f, 1.5f, {0, 0}, TUN_H - 0.2f);
            // têtes : voile entre les voûtes et le rectangle de la tranchée, à l'entrée (s0) et à la sortie (s1)
            for (int bout : {0, 1}) {
                if (bout == 0 ? seg != tun_.first : mod(seg + 1) != tun_.second) continue;
                const double s = bout ? s1 : s0;
                for (int tube = 0; tube < 2; tube++) {
                    const float cc = tube ? -(R + TUN_OPP_C) : 0.f, w = tube ? TUN_OPP_W : R + 4.f;
                    const int n = profilTube(pts, cc, w);
                    for (int i = 1; i + 2 < n; i++) {
                        const vec2 q0 = pts[i], q1 = pts[i + 1];
                        quadN(out[P_BETON], Wp(t, s, q0.x, q0.y), Wp(t, s, q1.x, q1.y), Wp(t, s, q1.x, TUN_DESSUS), Wp(t, s, q0.x, TUN_DESSUS),
                              {q0.x / TUILE_BETON, q0.y / TUILE_BETON}, {q1.x / TUILE_BETON, q1.y / TUILE_BETON},
                              {q1.x / TUILE_BETON, TUN_DESSUS / TUILE_BETON}, {q0.x / TUILE_BETON, TUN_DESSUS / TUILE_BETON}, 1.f);
                    }
                }
                for (const vec2 bd : {vec2{R + 4.f, dx}, vec2{-(R + 5.f), -(R + 4.f)}, vec2{gx, -(R + TUN_OPP_C + TUN_OPP_W)}})   // voiles pleins
                    quadN(out[P_BETON], Wp(t, s, bd.x, -0.6f), Wp(t, s, bd.y, -0.6f), Wp(t, s, bd.y, TUN_DESSUS), Wp(t, s, bd.x, TUN_DESSUS),
                          {bd.x / TUILE_BETON, 0}, {bd.y / TUILE_BETON, 0}, {bd.y / TUILE_BETON, TUN_DESSUS / TUILE_BETON}, {bd.x / TUILE_BETON, TUN_DESSUS / TUILE_BETON}, 1.f);
                // bandeau de tête (couronnement) : 0,6 m au-dessus de la dalle
                const vec2 cour[4] = {{gx - 0.3f, TUN_DESSUS}, {gx - 0.3f, TUN_DESSUS + 0.6f}, {dx + 0.3f, TUN_DESSUS + 0.6f}, {dx + 0.3f, TUN_DESSUS}};
                quadN(out[P_BETON], Wp(t, s, cour[0].x, cour[0].y), Wp(t, s, cour[3].x, cour[3].y), Wp(t, s, cour[2].x, cour[2].y), Wp(t, s, cour[1].x, cour[1].y),
                      {0, 0}, {(cour[3].x - cour[0].x) / TUILE_BETON, 0}, {(cour[3].x - cour[0].x) / TUILE_BETON, 0.25f}, {0, 0.25f}, 1.f);
            }
        }
        if (vo == ville::PONT) {
            // tablier : corniche (0,25 m au-dessus du trottoir), rive extérieure et sous-face, de chaque côté
            for (float sd : {1.f, -1.f}) {
                const vec2 corn[4] = {{sd * R + sd * 4.f, 0.15f}, {sd * (R + 4.f), 0.40f}, {sd * (R + PONT_CORN), 0.40f}, {sd * (R + PONT_CORN), PONT_SOUS}};
                extrude(t, out[P_BETON], s0, s1, corn, 4, TUILE_BETON, 1.f);
                // garde-corps : main courante à 1,15 m (1,00 m au-dessus du trottoir), lisse intermédiaire, montants tous les 2,4 m
                const float lg = sd * (R + 4.25f);
                carte::box(out[P_ACIER], Wp(t, s0 + 0.5, lg, 0), p.r, p.f, 0.06f, 0.06f, (float)SEG_M + 0.02f, {0, 0}, 1.10f);
                carte::box(out[P_ACIER], Wp(t, s0 + 0.5, lg, 0), p.r, p.f, 0.03f, 0.03f, (float)SEG_M + 0.02f, {0, 0}, 0.75f);
                if (!lod && seg % 2 == 0) carte::box(out[P_ACIER], Wp(t, s0, lg, 0), p.r, p.f, 0.05f, 0.75f, 0.05f, {0, 0}, 0.40f);
                // candélabres de 8 m sur la corniche, en quinconce tous les 30 m (lanterne LED blanc chaud)
                if (seg % LAMPE_PONT == 0 && ((seg / LAMPE_PONT) % 2 == 0) == (sd > 0)) {
                    const vec3 pied = Wp(t, s0, sd * (R + 4.25f), 0.40f);
                    carte::cyl(out[P_METAL], pied, 0.08f, 7.6f, 6, {0, 0});
                    carte::box(out[P_METAL], pied - p.r * (sd * 0.7f), p.r, p.f, 1.4f, 0.06f, 0.06f, {0, 0}, 7.5f);
                    carte::box(out[P_LAMP], pied - p.r * (sd * 1.3f), p.r, p.f, 0.6f, 0.1f, 0.3f, {0, 0}, 7.4f);
                }
            }
            { const vec2 sous[2] = {{R + PONT_CORN, PONT_SOUS}, {-(R + PONT_CORN), PONT_SOUS}}; extrude(t, out[P_BETON], s0, s1, sous, 2, TUILE_BETON, 1.f); }
            if (riv_.deb >= 0) {
                const bool eau = seg >= riv_.d0 + 1 && seg < riv_.d1 - 1;
                // eau de la rivière (niveau absolu), de la corniche jusqu'au bout des bas-côtés de Game
                if (eau) for (float sd : {1.f, -1.f}) {
                    auto Wa = [&](double s, float lat) { vec3 q = Wp(t, s, lat, 0); q.y = riv_.yw; return q; };
                    carte::quadSol(out[P_EAU], Wa(s0, sd * (R + PONT_CORN)), Wa(s0, sd * (R + 264.f)), Wa(s1, sd * (R + 264.f)), Wa(s1, sd * (R + PONT_CORN)), {0, 0});
                }
                // quais : murs de béton face à l'eau, du fond jusqu'au profil des bas-côtés (Game : 0,13 / 0 / −0,8 / −6 m)
                // (mur dans le plan du premier et du dernier segment noyés ; haut : bas-côtés du segment voisin, hors de l'eau, + 0,3 m)
                for (int bout : {0, 1}) {
                    if (seg != (bout ? riv_.d1 - 1 : riv_.d0 + 1)) continue;
                    const double sh = bout ? s1 : s0 - 1;   // segment des bas-côtés restés à leur hauteur
                    const vec2 prof[4] = {{R + 4.f, 0.13f}, {R + 24.f, 0.f}, {R + 84.f, -0.8f}, {R + 264.f, -6.f}};
                    for (float sd : {1.f, -1.f}) for (int i = 0; i < 3; i++) {
                        vec3 b0 = Wp(t, s0, sd * prof[i].x, 0), b1 = Wp(t, s0, sd * prof[i + 1].x, 0);
                        const vec3 h0{b0.x, Wp(t, sh, sd * prof[i].x, prof[i].y).y + 0.3f, b0.z}, h1{b1.x, Wp(t, sh, sd * prof[i + 1].x, prof[i + 1].y).y + 0.3f, b1.z};
                        b0.y = b1.y = riv_.yw - 0.6f;
                        const float lu = (prof[i + 1].x - prof[i].x) / TUILE_BETON;
                        quadN(out[P_BETON], b0, b1, h1, h0, {0, 0}, {lu, 0}, {lu, (h1.y - b1.y) / TUILE_BETON}, {0, (h0.y - b0.y) / TUILE_BETON}, 1.f);
                    }
                }
                // pile centrale : voile sous le tablier, avant-becs arrondis, de l'eau jusqu'à la sous-face
                if (seg == riv_.mil) {
                    const vec3 c = Wp(t, s0, 0, 0);
                    const float h = c.y + PONT_SOUS - (riv_.yw - 0.5f);
                    const vec3 base{c.x, riv_.yw - 0.5f, c.z};
                    carte::box(U, base, p.r, p.f, 2 * (R + 5.5f), h, 2.6f, uvc(U_BETON2));
                    for (float sd : {1.f, -1.f}) carte::cyl(U, base + p.r * (sd * (R + 5.5f)), 1.3f, h, 12, uvc(U_BETON2));
                }
                // arcs bow-string de part et d'autre (portées d0 → mil → d1), tirants en caisson, suspentes tous les 6 m
                const int64_t a0 = riv_.d0 + 1, a1 = riv_.d1 - 1;
                if (seg >= a0 && seg < a1) {
                    const bool premier = seg < riv_.mil;
                    const double sp0 = (double)(premier ? a0 : riv_.mil), sp1 = (double)(premier ? riv_.mil : a1);
                    auto yArc = [&](double sa) { const double u = std::clamp((sa - sp0) / (sp1 - sp0), 0.0, 1.0); return 0.6f + PONT_FLECHE * (float)(4 * u * (1 - u)); };
                    const double sa0 = (double)seg, sa1 = sa0 + 1;   // abscisses sur le tour (arc) ; s0, s1 : absolues (position)
                    for (float sd : {1.f, -1.f}) {
                        const float lc = sd * (R + PONT_ARC);
                        const vec2 tir[4] = {{lc - 0.45f, PONT_SOUS}, {lc - 0.45f, 0.6f}, {lc + 0.45f, 0.6f}, {lc + 0.45f, PONT_SOUS}};
                        extrude(t, out[P_ACIER], s0, s1, tir, 4, 1.f, 1.f);
                        const float y0 = yArc(sa0), y1 = yArc(sa1);
                        const float hw = 0.45f, hh = 0.55f;
                        const vec3 c00 = Wp(t, s0, lc - hw, y0 - hh), c01 = Wp(t, s0, lc + hw, y0 - hh), c02 = Wp(t, s0, lc + hw, y0 + hh), c03 = Wp(t, s0, lc - hw, y0 + hh);
                        const vec3 c10 = Wp(t, s1, lc - hw, y1 - hh), c11 = Wp(t, s1, lc + hw, y1 - hh), c12 = Wp(t, s1, lc + hw, y1 + hh), c13 = Wp(t, s1, lc - hw, y1 + hh);
                        const vec2 z{0, 0};
                        quadN(out[P_ACIER], c03, c13, c12, c02, z, z, z, z);   // dessus
                        quadN(out[P_ACIER], c00, c01, c11, c10, z, z, z, z);   // dessous
                        quadN(out[P_ACIER], c00, c10, c13, c03, z, z, z, z);   // côté intérieur
                        quadN(out[P_ACIER], c01, c02, c12, c11, z, z, z, z);   // côté extérieur
                        if (!lod && seg % 5 == 0 && y0 > 2.f) carte::cyl(out[P_ACIER], Wp(t, s0, lc, 0.6f), 0.045f, y0 - hh - 0.6f, 6, {0, 0});
                    }
                }
            }
        }
    }

    // portique de signalisation : poteaux (à droite derrière la glissière, à gauche sur le terre-plein), poutre treillis,
    // deux panneaux bleus de 4,6 × 2,2 m (bas à 5,6 m) au-dessus des voies, bordure et lettres blanches (atlas de textes)
    void portique(Track& t, int64_t a, const Portique& pq, Geo out[P_COUNT]) const {
        const float R = (float)RW;
        const carte::Pose p = carte::poseAt(t, a);
        const vec3 up{0, 1, 0};
        Geo& U = out[P_UNI];
        const float lg = -(R + AR_DBA), ld = R + 4.2f;
        for (float lx : {lg, ld}) carte::cyl(out[P_ACIER], Wp(t, (double)a, lx, lx < 0 ? 0.95f : 0.15f), 0.22f, lx < 0 ? 6.75f : 7.55f, 10, {0, 0});
        const vec3 mil = Wp(t, (double)a, (lg + ld) / 2, 0);
        carte::box(out[P_ACIER], mil, p.r, p.f, ld - lg, 0.9f, 0.9f, {0, 0}, 6.7f);
        int k = 0;
        for (const char* txt : {pq.g, pq.d}) {
            const float lc = k++ ? 3.75f : -3.75f;
            const vec3 c = Wp(t, (double)a, lc, 0) - p.f * 0.55f;
            carte::box(U, c, p.r, p.f, 4.6f, 2.2f, 0.12f, uvc(U_BLEU), 5.6f);
            const vec3 face = c - p.f * 0.065f + up * 6.7f;
            // listel blanc de 6 cm à 8 cm du bord
            const float bw = 4.6f - 0.16f, bh = 2.2f - 0.16f, e = 0.06f;
            for (int s = -1; s <= 1; s += 2) {
                const vec3 hy = up * (s * (bh / 2 - e / 2)), hx = p.r * (s * (bw / 2 - e / 2));
                carte::quad(U, face + hy - p.r * (bw / 2) - up * (e / 2), face + hy + p.r * (bw / 2) - up * (e / 2), face + hy + p.r * (bw / 2) + up * (e / 2), face + hy - p.r * (bw / 2) + up * (e / 2), uvc(U_PEINTURE));
                carte::quad(U, face + hx - up * (bh / 2) - p.r * (e / 2), face + hx - up * (bh / 2) + p.r * (e / 2), face + hx + up * (bh / 2) + p.r * (e / 2), face + hx + up * (bh / 2) - p.r * (e / 2), uvc(U_PEINTURE));
            }
            const vie::Texte& T = atlas_.texte(txt);
            if (T.ok) {
                const float h = 0.55f / 0.62f, w = h * T.aspect;   // lettres de 0,55 m (la ligne de l'atlas a ≈ 38 % de marge)
                const vec3 o = face - p.f * 0.01f, hx = p.r * (std::min(w, 4.2f) / 2), hy = up * (h * std::min(1.f, 4.2f / w) / 2);   // comme vie::plan
                quadN(out[P_TEXTE], o - hx - hy, o + hx - hy, o + hx + hy, o - hx + hy, {T.u0, T.v1}, {T.u1, T.v1}, {T.u1, T.v0}, {T.u0, T.v0});
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
        if (pl.aide != Aide::Web) return;   // WFS-256 (accroche) : voiture du joueur : chocs du décor en vraie physique, dans les deux modèles (src/game/chocs_cartes.cpp)
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

    // ==================================================================================================================
    // WFS-176 (RG-04, RG-05 de WFS-137) : casino, centre commercial, garages, pigeons et chiens
    // ==================================================================================================================
    // poses figées .wfa (assets/world/animaux : mouette.wfa de WFS-181, shiba.wfa et husky.wfa de convertir_chiens.py) ;
    // teintes : cases de la palette (couleurs[code de teinte du fichier])
    bool chargerAnimal(Renderer& r, const char* chemin, const int* couleurs, int nCouleurs, std::vector<Mesh>& poses) {
        std::vector<uint8_t> d;
        if (!readAsset(chemin, d) || d.size() < 12 || std::memcmp(d.data(), "WFA1", 4)) { WFS_ERR("Animal illisible : %s", chemin); return false; }
        uint32_t np = 0, nv = 0; std::memcpy(&np, &d[4], 4); std::memcpy(&nv, &d[8], 4);
        if (np == 0 || np > 32 || nv == 0 || nv > 200000 || nv % 3 || d.size() < 12 + (size_t)nv + (size_t)np * nv * 24) { WFS_ERR("Animal invalide : %s", chemin); return false; }
        std::vector<uint32_t> idx(nv);
        for (uint32_t i = 0; i < nv; i++) idx[i] = i;
        for (uint32_t p = 0; p < np; p++) {
            std::vector<Vertex> v(nv);
            const uint8_t* src = d.data() + 12 + nv + (size_t)p * nv * 24;
            for (uint32_t i = 0; i < nv; i++) {
                float f[6]; std::memcpy(f, src + (size_t)i * 24, 24);
                const vec2 uv = uvc(couleurs[std::min<int>(d[12 + i], nCouleurs - 1)]);
                v[i] = {f[0], f[1], f[2], f[3], f[4], f[5], uv.x, uv.y};
            }
            poses.push_back(r.makeMesh(v, idx, false));
        }
        return true;
    }

    int64_t modL(int64_t n) const { const int64_t L = (int64_t)plan_->objets.size(); return ((n % L) + L) % L; }
    void initLieux(Renderer& r, Track& t) {
        const ville::Plan& P = *plan_;
        basse_ = r.qualiteBasse;
        // dorures (laiton doré : métal, F0 ≈ (1,0 ; 0,78 ; 0,34) de l'or, rugosité 0,35), vitrages (verre clair : diélectrique
        // lisse, F0 0,04 ; teinte sombre vue de jour ; la nuit, intérieur éclairé 3000 K)
        Material mo; mo.color = {1.f, 0.78f, 0.34f, 1}; mo.metal = 1; mo.rough = 0.35f; mo.flags = MAT_DOUBLE;
        mats_[P_OR] = r.makeMaterial(mo, nullptr, nullptr, nullptr);
        Material mv; mv.color = {0.06f, 0.08f, 0.09f, 1}; mv.rough = 0.05f; mv.flags = MAT_DOUBLE;
        if (nuit_) mv.emissive = vec3{1.f, 0.72f, 0.45f} * 0.9f;
        mats_[P_VERRE] = r.makeMaterial(mv, nullptr, nullptr, nullptr);
        for (const char* x : {"CASINO", "GARAGE 88", "GALERIE DES ARCADES", "LUNA MODE", "ATELIER 7", "MAISON VERTE"}) atlasLieux_.ajouter(x);
        if (!P.lieux.empty() && atlasLieux_.construire(r)) {
            // néons bornés (pas de halo qui mange l'écran) : or-rose pour le casino, cyan pour les autres (design-street-racing)
            enseigneMat_[0] = &atlasLieux_.materiau(r, 0xffb347, nuit_ ? 2.2f : 0.5f, 0.45f);
            enseigneMat_[1] = &atlasLieux_.materiau(r, 0x00e5ff, nuit_ ? 1.8f : 0.35f, 0.45f);
        }
        for (const ville::Lieu& l : P.lieux) if (l.t == ville::CASINO && !luxe_[0].ok) {
            if (!loadCarModel(r, "cars/sf90.wfm", luxe_[0]) || !loadCarModel(r, "cars/gallardo.wfm", luxe_[1])) WFS_ERR("Ville : voitures du casino introuvables");
        }
        // lumière de nuit (nuit.cpp, les 4 plus proches) : parvis du casino, parking du centre commercial, intérieur des garages
        for (const ville::Lieu& l : P.lieux) {
            const float s = (float)l.side;
            if (l.t == ville::CASINO) for (float al : {10.f, 30.f}) lampadaires().push_back({modL(l.seg + (int64_t)(al / SEG_M)), s * ((float)RW + 9.f), 4.5f, StyleLampadaire::Ville});
            if (l.t == ville::CENTRE_COMMERCIAL) for (float al : {15.f, 40.f, 65.f}) lampadaires().push_back({modL(l.seg + (int64_t)(al / SEG_M)), s * ((float)RW + 17.f), 8.f, StyleLampadaire::Ville});
            if (l.t == ville::GARAGE) lampadaires().push_back({modL(l.seg + (int64_t)(7 / SEG_M)), s * ((float)RW + 12.f), 3.4f, StyleLampadaire::Ville});
        }
        // pigeons : 2 groupes à chaque carrefour (trottoir, 15 m avant et après le passage), 1 sur le parvis du casino ;
        // forme de la mouette de WFS-181 à la taille du pigeon biset (envergure 0,67 m, longueur 0,32 m : × 0,48)
        static const int PIG[4] = {U_PIG0, U_PIG0 + 1, U_PIG0 + 2, U_PIG0 + 3};
        vols_.clear();
        if (chargerAnimal(r, "world/animaux/mouette.wfa", PIG, 4, pigeon_) && pigeon_.size() >= 10) {
            for (const ville::Carrefour& c : P.carrefours) for (int k = 0; k < 2; k++)
                vols_.push_back({modL(c.croise + (k ? 1 : -1) * (int64_t)(15 / SEG_M)), (k ? 1.f : -1.f) * ((float)RW + 3.2f), 0.f});
            for (const ville::Lieu& l : P.lieux) if (l.t == ville::CASINO) vols_.push_back({modL(l.seg + (int64_t)(6 / SEG_M)), (float)l.side * ((float)RW + 8.f), 0.f});
        } else pigeon_.clear();
        static const int SHI[6] = {U_SHI0, U_SHI0 + 1, U_SHI0 + 2, U_SHI0 + 3, U_SHI0 + 4, U_SHI0 + 5};
        static const int HUS[6] = {U_HUS0, U_HUS0 + 1, U_HUS0 + 2, U_HUS0 + 3, U_HUS0 + 4, U_HUS0 + 5};
        if (!P.promeneurs.empty() && (!chargerAnimal(r, "world/animaux/shiba.wfa", SHI, 6, chien_[0]) || !chargerAnimal(r, "world/animaux/husky.wfa", HUS, 6, chien_[1]))) { chien_[0].clear(); chien_[1].clear(); }
        promS_.clear();
        for (const ville::Promeneur& m : P.promeneurs) promS_.push_back(m.s);
        { Geo g; carte::box(g, {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, 0.012f, 0.012f, 1.f, uvc(U_LAISSE)); laisse_ = r.makeMesh(g.v, g.i, false); }   // laisse : 1 m le long de +Z
        for (const ville::Lieu& l : P.lieux) WFS_LOG("Ville (WFS-176) : lieu %d au segment %lld, côté %d, %.0f m", (int)l.t, (long long)l.seg, l.side, l.len);
        WFS_LOG("Ville (WFS-176) : %zu lieu(x), %zu groupe(s) de pigeons, %zu chien(s)%s", P.lieux.size(), vols_.size(), promS_.size(),
                chien_[0].empty() ? " (modèles absents : chiens non dessinés)" : "");
        (void)t;
    }

    // promeneurs : même marche que pietons.cpp (update : s += dir · v / SEG_M · dt en course), pour que le chien suive son maître
    void update(double dt, const GameView& g) override {
        if (!active_ || g.ecran != Ecran::Course || promS_.empty()) return;
        const double len = (double)plan_->objets.size();
        for (size_t i = 0; i < promS_.size(); i++) {
            const ville::Promeneur& m = plan_->promeneurs[i];
            promS_[i] += m.dir * m.v / SEG_M * dt;
            if (promS_[i] >= len) promS_[i] -= len;
            if (promS_[i] < 0) promS_[i] += len;
        }
    }

    // géométrie fixe d'un lieu, dans le repère de la route au milieu de sa façade (lignes droites : RG-C1)
    void lieuVille(Track& t, int64_t a, const ville::Lieu& l, bool lod, Geo out[P_COUNT]) const {
        const float R = (float)RW, s = (float)l.side, demi = l.len / 2;
        const carte::Pose pm = poseS(t, (double)a + demi / SEG_M);
        const vec3 up{0, 1, 0}, X = pm.r * s, Z = pm.f;   // X : vers l'extérieur de la route
        auto P = [&](float lat, float al, float y) { return pm.o + X * lat + Z * al + up * y; };
        Geo& U = out[P_UNI];
        // boîte : centre de la base (lat, al, y0), demi-tailles hx (selon X), hz (selon Z), hauteur h, couleur
        auto bloc = [&](Geo& g, float lat, float al, float y0, float hx, float hz, float h, vec2 uv) {
            carte::box(g, P(lat, al, 0), X, Z, 2 * hx, h, 2 * hz, uv, y0);
        };
        // enseigne : texte de l'atlas des lieux, centré en (lat, al, y), sur la face tournée vers la route, hauteur des lettres h
        auto enseigne = [&](Geo& g, const char* txt, float lat, float al, float y, float h, float largeurMax) {
            const vie::Texte& T = atlasLieux_.texte(txt);
            if (!T.ok) return;
            const float hl = h / 0.62f, w = std::min(hl * T.aspect, largeurMax), k = w / (hl * T.aspect);
            const vec3 o = P(lat - 0.02f, al, y), hx = Z * (-s * w / 2), hy = up * (hl * k / 2), n = -X;
            quad(g, o - hx - hy, o + hx - hy, o + hx + hy, o - hx + hy, n, {T.u0, T.v1}, {T.u1, T.v1}, {T.u1, T.v0}, {T.u0, T.v0});
        };
        switch (l.t) {
        case ville::CASINO: {
            // casino : 40 × 24 m, 14 m (3 niveaux), en retrait à RW + 12 derrière un parvis de pierre ; façade de pierre claire à
            // pilastres (tous les 4 m) et bandeaux, vitrages en retrait de 0,35 m (fenêtres en profondeur), corniche et chapiteaux
            // dorés, marquise dorée de 12 m au-dessus de 3 marches (17 / 30 cm : 2h + g = 64 cm, loi de Blondel) et d'un tapis
            const float f0 = R + 12.f;
            quad(U, P(R + 6.5f, -demi, 0.16f), P(R + 6.5f, demi, 0.16f), P(f0, demi, 0.16f), P(f0, -demi, 0.16f), up, uvc(U_MARCHE), uvc(U_MARCHE), uvc(U_MARCHE), uvc(U_MARCHE));
            bloc(out[P_VERRE], f0 + 12.35f, 0, 0, 12.f, demi - 0.2f, 14.f, {0, 0});                     // vitrages (en retrait)
            bloc(U, f0 + 12.f, 0, 0, 12.f, demi, 1.2f, uvc(U_CREME));                                    // soubassement
            for (float y : {5.f, 9.6f}) bloc(U, f0 + 12.f, 0, y, 12.f, demi, 0.6f, uvc(U_CREME));       // bandeaux d'étage
            bloc(U, f0 + 12.f, 0, 13.f, 12.f, demi, 1.f, uvc(U_CREME));                                  // attique
            bloc(out[P_OR], f0 + 11.85f, 0, 14.f, 12.15f, demi + 0.15f, 0.45f, {0, 0});                  // corniche dorée
            for (float al = -demi; al <= demi + 0.01f; al += 4.f) {
                bloc(U, f0 + 12.f, al, 0, 12.f, 0.4f, 13.f, uvc(U_CREME));                               // pilastres
                if (!lod) bloc(out[P_OR], f0 - 0.08f, al, 12.4f, 0.12f, 0.5f, 0.6f, {0, 0});             // chapiteaux
            }
            for (int k = 0; k < 3; k++)                                                                   // marches
                bloc(U, f0 - 0.9f + k * 0.3f, 0, 0.16f, 0.9f - k * 0.3f, 6.f, 0.17f * (k + 1), uvc(U_MARCHE));
            quad(U, P(f0 - 1.8f, -2.f, 0.69f), P(f0 - 1.8f, 2.f, 0.69f), P(f0, 2.f, 0.69f), P(f0, -2.f, 0.69f), up, uvc(U_TAPIS), uvc(U_TAPIS), uvc(U_TAPIS), uvc(U_TAPIS));
            bloc(out[P_OR], f0 - 2.f, 0, 4.4f, 2.f, 6.f, 0.4f, {0, 0});                                   // marquise
            for (float al : {-5.6f, 5.6f}) bloc(out[P_OR], f0 - 3.8f, al, 0.16f, 0.1f, 0.1f, 4.25f, {0, 0});   // poteaux de la marquise
            enseigne(out[P_TXT_A], "CASINO", f0 - 0.4f, 0, 16.6f, 2.6f, 24.f);                            // néon sur l'attique
            enseigne(out[P_TXT_A], "CASINO", f0 - 4.05f, 0, 4.6f, 0.75f, 7.f);                            // et sur le bandeau de la marquise
            bloc(U, f0 + 0.3f, 0, 14.45f, 0.15f, 9.f, 3.3f, uvc(U_NOIR));                                 // fond de l'enseigne
            break;
        }
        case ville::CENTRE_COMMERCIAL: {
            // centre commercial : bâtiment de 80 × 26 m, 12 m, à RW + 31 ; parking d'une double rangée de places de 2,5 × 5 m
            // (norme NF P 91-100) et d'une allée de 6 m ; mur-rideau vitré (meneaux d'aluminium tous les 1,5 m), bandeau, 4 enseignes
            const float b0 = R + 31.f;
            quad(U, P(R + 6.5f, -demi, 0.16f), P(R + 6.5f, demi, 0.16f), P(b0, demi, 0.16f), P(b0, -demi, 0.16f), up, uvc(U_PARKING), uvc(U_PARKING), uvc(U_PARKING), uvc(U_PARKING));
            for (float r0 : {R + 9.f, R + 20.f}) {   // rangées de places : lignes blanches de 10 cm
                for (float al = -demi + 2.5f; al <= demi - 2.5f + 0.01f; al += 2.5f)
                    quad(U, P(r0, al - 0.05f, 0.17f), P(r0, al + 0.05f, 0.17f), P(r0 + 5.f, al + 0.05f, 0.17f), P(r0 + 5.f, al - 0.05f, 0.17f), up, uvc(U_PEINTURE), uvc(U_PEINTURE), uvc(U_PEINTURE), uvc(U_PEINTURE));
            }
            quad(U, P(R + 25.f, -demi, 0.18f), P(R + 25.f, demi, 0.18f), P(b0, demi, 0.18f), P(b0, -demi, 0.18f), up, uvc(U_MARCHE), uvc(U_MARCHE), uvc(U_MARCHE), uvc(U_MARCHE));   // parvis piéton
            bloc(out[P_VERRE], b0 + 13.f, 0, 0, 13.f, demi, 9.f, {0, 0});
            bloc(U, b0 + 13.f, 0, 9.f, 13.f, demi, 3.f, uvc(U_CLAIR));                                     // bandeau opaque
            if (!lod) for (float al = -demi; al <= demi + 0.01f; al += 1.5f) bloc(U, b0 - 0.05f, al, 0, 0.06f, 0.05f, 9.f, uvc(U_ALU));
            if (!lod) for (float y : {3.f, 6.f}) bloc(U, b0 - 0.05f, 0, y, 0.06f, demi, 0.08f, uvc(U_ALU));
            bloc(U, b0 - 2.5f, 0, 4.f, 2.5f, 10.f, 0.35f, uvc(U_ALU));                                     // auvent de l'entrée
            enseigne(out[P_TXT_B], "GALERIE DES ARCADES", b0 - 0.1f, 0, 10.5f, 1.6f, 40.f);
            const char* boutiques[3] = {"LUNA MODE", "ATELIER 7", "MAISON VERTE"};
            for (int k = 0; k < 3; k++) enseigne(out[P_TXT_B], boutiques[k], b0 - 0.1f, -26.f + k * 26.f + (k == 1 ? 14.f : 0.f), 7.5f, 0.7f, 10.f);
            break;
        }
        case ville::GARAGE: {
            // garage : 14 × 12,5 m, 6 m de haut, sur l'alignement des façades (RW + 6,5) ; rideau métallique levé (baie de
            // 4,5 × 3,6 m, rouleau en haut), sol en résine, pont élévateur à 2 colonnes (3 m), enseigne « GARAGE 88 »
            const float f0 = R + 6.5f, al0 = -2.f;   // baie centrée à 2 m avant le milieu
            bloc(U, f0 + 12.3f, 0, 0, 0.2f, demi, 6.f, uvc(U_ENDUIT));                                     // mur du fond
            for (float sd : {-1.f, 1.f}) bloc(U, f0 + 6.25f, sd * (demi - 0.2f), 0, 6.25f, 0.2f, 6.f, uvc(U_ENDUIT));   // pignons
            bloc(U, f0 + 6.25f, 0, 5.8f, 6.25f, demi, 0.2f, uvc(U_ENDUIT));                                // toiture
            bloc(U, f0 + 0.2f, (al0 - 2.25f - demi) / 2, 0, 0.2f, (demi + al0 - 2.25f) / 2, 6.f, uvc(U_ENDUIT));       // façade : trumeau gauche
            bloc(U, f0 + 0.2f, (al0 + 2.25f + demi) / 2, 0, 0.2f, (demi - al0 - 2.25f) / 2, 6.f, uvc(U_ENDUIT));       // trumeau droit
            bloc(U, f0 + 0.2f, al0, 3.6f, 0.2f, 2.25f, 2.4f, uvc(U_ENDUIT));                                // linteau
            quad(U, P(f0, -demi, 0.17f), P(f0, demi, 0.17f), P(f0 + 12.1f, demi, 0.17f), P(f0 + 12.1f, -demi, 0.17f), up, uvc(U_SOL_GARAGE), uvc(U_SOL_GARAGE), uvc(U_SOL_GARAGE), uvc(U_SOL_GARAGE));
            carte::cylAxe(out[P_ACIER], P(f0 + 0.6f, al0 - 2.3f, 3.95f), Z, 0.3f, 4.6f, 10, {0, 0});        // rouleau du rideau
            bloc(out[P_ACIER], f0 + 6.5f, al0 - 1.6f, 0.17f, 0.18f, 0.18f, 3.f, {0, 0});                   // colonnes du pont
            for (float sd : {-1.6f, 1.6f}) {
                bloc(U, f0 + 6.5f, al0 + sd, 0.17f, 0.18f, 0.18f, 3.f, uvc(U_PONT));
                if (!lod) bloc(U, f0 + 6.5f, al0 + sd * 0.55f, 1.1f, 0.9f, 0.06f, 0.08f, uvc(U_PONT));   // bras
            }
            bloc(U, f0 - 0.15f, al0, 3.7f, 0.12f, 2.6f, 1.1f, uvc(U_NOIR));                                 // fond de l'enseigne
            enseigne(out[P_TXT_B], "GARAGE 88", f0 - 0.3f, al0, 4.25f, 0.65f, 4.8f);
            bloc(out[P_VERRE], f0 + 0.25f, al0 + 4.6f, 0.17f, 0.05f, 0.9f, 2.2f, {0, 0});                    // porte vitrée du bureau
            break;
        }
        }
    }

    // voitures garées devant le casino (modèles du jeu, allégés au loin), pigeons qui s'envolent quand le joueur passe à moins
    // de 10 m, chiens en laisse qui suivent leur promeneur
    int vieDesLieux(Renderer& r, const GameView& g, int64_t aP) {
        Track& t = *g.track;
        const int64_t L = (int64_t)plan_->objets.size();
        int draws = 0;
        const double jx = g.player->playerX * RW, jz = g.player->dist / SEG;   // joueur : latéral (m), segment
        for (const ville::Lieu& l : plan_->lieux) {
            if (l.t != ville::CASINO || !luxe_[0].ok) continue;
            const int64_t a = proche(l.seg, aP);
            if ((double)(a - aP) * SEG_M < -40 || (double)(a - aP) * SEG_M > 260) continue;
            const vec3 couleurs[2] = {carte::linHex(0xb3000f), carte::linHex(0xe6b800)};   // rosso, giallo
            for (int k = 0; k < 2; k++) {
                const double sA = (double)a + (12.0 + 16.0 * k) / SEG_M;
                t.pose((int64_t)sA - 1); t.pose((int64_t)sA + 2);
                const TrackPoint p = t.point(sA * SEG, (float)l.side * ((float)RW + 8.4f));
                const mat4 w = translate({(float)p.x, (float)p.y + 0.16f, (float)p.z}) * rotateY((float)-(p.th + PI));
                const RouesPose rp = roues::poseTrafic(0, false, 0.0, luxe_[k].nRoues ? luxe_[k].roues[0].rayon : roues::RW_DEFAUT);
                FeuxVoiture f; f.nuit = false;
                r.feuxProchaineVoiture(f);
                r.draw(luxe_[k], w, &couleurs[k], &rp);
                draws += (int)luxe_[k].parts.size();
            }
        }
        // pigeons : 6 par groupe, posés (pose 9) en picorant ; envol (poses 0-7, 6 battements/s) en s'éloignant de la route,
        // montée à 7 m en 3 s ; retour au sol 25 s plus tard. En Basse : 3 par groupe (RG-C2)
        const int n = basse_ ? 3 : 6;
        for (Vol& v : vols_) {
            const int64_t a = proche(v.seg, aP);
            const double dz = (double)(a - aP) * SEG_M;
            if (dz < -30 || dz > 160) continue;
            const double dJ = std::hypot((double)(a - jz) * SEG_M, v.lat - jx);
            if (dJ < 10 && g.time - v.envol > 25) v.envol = g.time;
            const double te = g.time - v.envol;
            if (te > 4 && te < 25) continue;   // partis
            for (int i = 0; i < n; i++) {
                Mulberry32 R((uint32_t)(v.seg * 7 + i * 131));
                const float al = ((float)R.next() - 0.5f) * 3.f, dl = ((float)R.next() - 0.5f) * 1.6f, yaw = (float)R.next() * 6.283f, ph = (float)R.next() * 6.283f;
                const double sA = (double)a + al / SEG_M;
                t.pose((int64_t)sA - 1); t.pose((int64_t)sA + 2);
                const float sd = v.lat > 0 ? 1.f : -1.f;
                const bool vole = te >= 0 && te <= 4;
                const float tv = vole ? (float)te : 0.f, mont = vole ? std::min(7.f, tv * 2.4f) : 0.f;
                const TrackPoint p = t.point(sA * SEG, v.lat + dl + sd * tv * 3.f);
                const vec3 rr{(float)std::cos(p.th), 0, (float)std::sin(p.th)}, ff{(float)std::sin(p.th), 0, (float)-std::cos(p.th)};
                vec3 z = vole ? normalize(rr * sd + vec3{0, 0.5f, 0}) : ff * std::cos(yaw) + rr * std::sin(yaw);
                vec3 y{0, 1, 0};
                if (!vole) { const float pic = std::max(0.f, (float)std::sin(g.time * 2.2 + ph)) * 0.6f; z = normalize(z + y * (-pic)); }   // picore
                const vec3 x = normalize(cross(y, z)); y = cross(z, x);
                const float k = 0.48f;
                const vec3 o{(float)p.x, (float)p.y + 0.16f + mont, (float)p.z};
                const int idx = vole ? (int)(g.time * 6 * 8 + i) % 8 : 9;
                r.draw(pigeon_[(size_t)idx], mats_[P_UNI], repere(o, x * k, y * k, z * k)); draws++;
            }
        }
        // chiens : 1,3 m devant leur promeneur, côté façades ; pas de marche réglé sur la distance (cycle de 0,9 m)
        if (!chien_[0].empty()) for (size_t i = 0; i < promS_.size(); i++) {
            const ville::Promeneur& m = plan_->promeneurs[i];
            double rel = promS_[i] - std::fmod(std::fmod((double)aP, (double)L) + L, (double)L);
            if (rel > L / 2.0) rel -= L; if (rel < -L / 2.0) rel += L;
            if (rel * SEG_M < -30 || rel * SEG_M > 120) continue;
            const double sM = (double)aP + rel, sC = sM + m.dir * 1.3 / SEG_M;
            t.pose((int64_t)std::min(sM, sC) - 1); t.pose((int64_t)std::max(sM, sC) + 2);
            const float sd = m.lat > 0 ? 1.f : -1.f;
            const TrackPoint pc = t.point(sC * SEG, (float)m.lat + sd * 0.6f), pmt = t.point(sM * SEG, (float)m.lat);
            const vec3 ff{(float)std::sin(pc.th), 0, (float)-std::cos(pc.th)}, z = ff * (float)m.dir, y{0, 1, 0}, x = cross(y, z);
            const vec3 oc{(float)pc.x, (float)pc.y + 0.15f, (float)pc.z};
            const double parcours = std::fabs(promS_[i] - m.s) * SEG_M;
            const int idx = g.ecran == Ecran::Course ? (int)(std::fmod(parcours / 0.9, 1.0) * 8) % 8 : 8;
            const std::vector<Mesh>& C = chien_[m.race % 2];
            r.draw(C[(size_t)std::min<int>(idx, (int)C.size() - 1)], mats_[P_UNI], repere(oc, x, y, z)); draws++;
            // laisse : de la main du promeneur (≈ 0,8 m) au collier (≈ 0,45 m shiba, 0,6 m husky)
            const vec3 main{(float)pmt.x, (float)pmt.y + 0.95f, (float)pmt.z};
            const vec3 col = oc + z * (m.race % 2 ? 0.38f : 0.28f) + y * (m.race % 2 ? 0.55f : 0.4f);
            const vec3 d = col - main; const float lg = length(d);
            if (lg > 0.1f) {
                const vec3 zl = d * (1.f / lg), xl = normalize(cross(std::fabs(zl.y) > 0.9f ? vec3{1, 0, 0} : vec3{0, 1, 0}, zl)), yl = cross(zl, xl);
                r.draw(laisse_, mats_[P_UNI], repere(main, xl, yl, zl * lg)); draws++;
            }
        }
        return draws;
    }
    static mat4 repere(vec3 o, vec3 x, vec3 y, vec3 z) {
        mat4 m = mat4::identity();
        const vec3 c[4] = {x, y, z, o};
        for (int j = 0; j < 4; j++) { m.at(0, j) = c[j].x; m.at(1, j) = c[j].y; m.at(2, j) = c[j].z; }
        return m;
    }

    void render(Renderer& r, const GameView& g) override {
        if (!active_ || g.sceneDediee) return;   // scène dédiée (garage…) : pas de ville (WFS-44)
        frame_++;
        Track& t = *g.track;
        const int64_t aP = (int64_t)std::floor(g.player->dist / SEG), pc = aP / CH;
        auto lodOf = [&](int64_t c) { return (double)(c * CH - aP) * SEG_M > LOD_FAR_M; };
        int draws = 0, ouv = 0;
        // matériau d'une part (mêmes règles que l'ancienne gestion) ; nul : part non dessinée
        auto matDe = [&](int p) -> const Material* {
            if (p == P_SOL && !solActif_) return nullptr;     // WFS-175 : sol de Game, sauf quand la rivière est en vue
            if (p == P_TEXTE) return texteMat_;
            if (p == P_TXT_A || p == P_TXT_B) return enseigneMat_[p - P_TXT_A];   // WFS-176 : enseignes des lieux (atlas à part)
            return &mats_[p];
        };
        if (lots::g_lotsDecor && lots_.pret()) {   // WFS-268 : blocs préparés au chargement, aucune création en course
            lots_.dessiner(t, pc * CH, BEHIND * CH, (AHEAD + 1) * CH - 1, LOD_FAR_M, [&](int p, const Mesh& m, const mat4& M) {
                const Material* mat = matDe(p);
                if (!mat) return;
                // WFS-175 : asphalte de la chaussée opposée : demi-largeur de 100 m → aucune ligne du moteur (marquage dessiné à part)
                r.draw(m, *mat, M, p == P_ROUTE ? 100.f : 0.f); draws++;
                if (p >= P_BETON && p != P_SOL && p != P_TXT_A && p != P_TXT_B) ouv++;
            });
            fixes_ = draws;   // appels du décor fixe (avant le masquage du renderer)
        } else {   // ancienne gestion (repli) : morceaux construits en course
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
        for (auto& [k, ch] : chunks_) for (int p = 0; p < P_COUNT; p++) if (ch.m[p].indexCount) {
            if (p == P_SOL && !solActif_) continue;     // WFS-175 : sol de Game, sauf quand la rivière est en vue
            if (p == P_TEXTE && !texteMat_) continue;
            if (p == P_TXT_A || p == P_TXT_B) {   // WFS-176 : enseignes des lieux (atlas à part)
                if (enseigneMat_[p - P_TXT_A]) { r.draw(ch.m[p], *enseigneMat_[p - P_TXT_A], I); draws++; }
                continue;
            }
            // WFS-175 : asphalte de la chaussée opposée : demi-largeur de 100 m → aucune ligne du moteur (marquage dessiné à part)
            r.draw(ch.m[p], p == P_TEXTE ? *texteMat_ : mats_[p], I, p == P_ROUTE ? 100.f : 0.f); draws++;
            if (p >= P_BETON && p != P_SOL) ouv++;   // WFS-175 : coût des ouvrages (le sol remplace celui de Game, à coût égal)
        }
        }
        draws += lieuxAnimes(r, g, aP);   // WFS-137
        const int vie = vieDesLieux(r, g, aP);   // WFS-176 : voitures du casino (pièces), pigeons, chiens
        draws += vie; lastVie_ = vie;
        piecesInv_ = 0;
        const int inv = sensInverse(r, g, aP);   // WFS-175 : voitures (une par modèle ; leurs pièces sont comptées par le moteur)
        draws += inv;
        lastDraws_ = draws; lastOuv_ = ouv; lastInv_ = inv; lastPieces_ = piecesInv_;
        if (frame_ % 300 == 1) { unsigned long long cm, cmat; lots_.creationsEnCourse(r, cm, cmat); WFS_LOG("Ville : %d appels de dessin (%zu morceaux, %d blocs préparés : %d appels pour le décor fixe, avant masquage) ; créations en course (tout le jeu) : %llu maillages, %llu matériaux", draws, chunks_.size(), lots_.blocs(), fixes_, cm, cmat); }
    }

    // WFS-137 (RG-01) : abords du rond-point, dans le repère de la route au centre de l'anneau (x : latéral, z : avant ; le même
    // à chaque tour) : amorces des deux branches sans route (asphalte, 30 m) et immeubles le long des quatre branches, gardés
    // seulement s'ils restent à plus de RW + 6 m de toute la route (± 360 m) : rien ne retombe sur l'entrée ni la sortie
    void abordsRondPoint(Track& t) {
        abords_.assign(plan_->ronds.size(), {});
        routesRp_ = chaussee::axes(t, {trace::Type::RondPoint});   // WFS-212 : axe de la route autour des ronds-points
        gir_.assign(plan_->ronds.size(), {});
        for (size_t i = 0; i < plan_->ronds.size(); i++)   // WFS-248 : ouvrage reconstruit, à partir de la section du tracé
            for (const trace::Section& s : trace::sections(t)) if (s.type == trace::Type::RondPoint && s.debut == plan_->ronds[i].debut) gir_[i] = giratoire::calculer(t, s);
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
                const float r0 = rp.rayon + R - 0.5f;   // bord extérieur de l'anneau (immeubles)
                if (b >= 2) {   // amorce : 30 m d'asphalte (largeur de la route), si elle ne touche aucune autre partie de la route
                    bool ok = true;
                    for (float d = r0 + 6; d <= r0 + 30 && ok; d += 3) ok = libre(ctr + u * d, 2 * R);
                    // WFS-248 : l'amorce part du bord de l'anneau reconstruit (plus d'espace vide entre les deux) ; rive ouverte
                    const vec3 cg = gir_[i].ok ? gir_[i].c : ctr;
                    if (ok) { abords_[i].push_back({local(cg), localDir(u), localDir(n), giratoire::R_EXT, 30 + r0 - giratoire::R_EXT, R, -1, b});
                              gir_[i].autresBranches.push_back(std::atan2(u.z, u.x)); }
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
            const int64_t absJ = proche(cf.ligne, aP);
            if (std::abs(absJ - aP) * SEG_M > 400) continue;
            // WFS-268 : décor préparé : optiques construites une fois dans le tour 0 (init), dessinées avec la matrice du tour
            const bool tour0 = lots::g_lotsDecor && lots_.pret();
            const int64_t k = tour0 ? lots_.tourDe(absJ) : 0, abs = absJ - k * (int64_t)P.objets.size();
            const mat4 M = tour0 ? lots_.matrice(t, k) : mat4::identity();
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
            for (int k = 0; k < 3; k++) if (op.m[k].indexCount) { r.draw(op.m[k], k == allume ? lentille_[k] : lentilleEteinte_, M); draws++; }
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
        WFS_LOG("Ville : %d appels de dessin à la dernière image (WFS-175 : %d pour les ouvrages, %d voiture(s) en sens inverse, ≤ %d pièces ; WFS-176 : %d pour les voitures du casino, pigeons et chiens)", lastDraws_, lastOuv_, lastInv_, lastPieces_, lastVie_);
        r.vk().waitIdle();
        for (Optiques& op : optiques_) for (Mesh& m : op.m) if (m.vb.buf) r.destroy(m);
        optiques_.clear();
        if (jet_.vb.buf) { r.destroy(jet_); jet_ = Mesh{}; }
        for (auto& [k, ch] : chunks_) for (Mesh& m : ch.m) if (m.vb.buf) r.destroy(m);
        chunks_.clear();
        lots_.liberer(r);   // WFS-268
        for (auto& g : graveyard_) r.destroy(g.second);
        graveyard_.clear();
        for (Image& im : facadeTex_) r.vk().destroy(im);
        for (Image* im : {&leafTex_, &barkTex_, &palTex_}) if (im->img) { r.vk().destroy(*im); *im = Image{}; }
        for (Material& m : mats_) r.destroy(m);
        for (Material* m : {&lentilleEteinte_, &lentille_[0], &lentille_[1], &lentille_[2], &eauJet_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        // WFS-175 : matériaux des ouvrages (dans mats_, détruits ci-dessus ; P_TEXTE appartient à l'atlas), atlas, béton, voitures
        atlas_.detruire(r); texteMat_ = nullptr;
        atlasLieux_.detruire(r); enseigneMat_[0] = enseigneMat_[1] = nullptr;   // WFS-176
        for (Model& m : luxe_) if (m.ok) { r.destroy(m); m = Model{}; }
        for (std::vector<Mesh>* v : {&pigeon_, &chien_[0], &chien_[1]}) { for (Mesh& m : *v) if (m.vb.buf) r.destroy(m); v->clear(); }
        if (laisse_.vb.buf) { r.destroy(laisse_); laisse_ = Mesh{}; }
        if (betonTex_.img) { r.vk().destroy(betonTex_); betonTex_ = Image{}; }
        for (Model& m : autos_) if (m.ok) { r.destroy(m); m = Model{}; }
        portiques_.clear(); riv_ = Riviere{}; tun_ = {-1, -1}; au_ = {-1, -1}; solActif_ = false;
        plan_ = nullptr;
        pilote_ = false;
        active_ = false;
    }

    // WFS-212 (RG-01) : la Ville dessine aussi les trottoirs de Game (trottoirChunk), coupés là où une autre partie du tracé passe
    bool remplaceTrottoir(const GameView& g) const override { return active_ && plan_ && !g.sceneDediee && g.player; }

    // WFS-175 : la Ville dessine les bas-côtés de Game (au bit près), pour les abaisser sous l'eau de la rivière du pont
    // WFS-212 : toujours (bas-côtés coupés au rond-point, comme les trottoirs)
    bool remplaceSol(const GameView& g) const override {
        solActif_ = active_ && plan_ && !g.sceneDediee && g.player;
        return solActif_;
    }

private:
    // WFS-175
    Riviere riv_;
    std::pair<int64_t, int64_t> tun_{-1, -1}, au_{-1, -1};   // tunnel, autoroute (segments du tour ; fin éventuellement > L)
    std::vector<Portique> portiques_;
    vie::AtlasTextes atlas_;
    const Material* texteMat_ = nullptr;
    Image betonTex_{};
    Model autos_[2];
    mutable bool solActif_ = false;
    static constexpr float FACADE_TROTTOIR = SET_BACK + 0.3f;   // WFS-137 : trottoir prolongé 30 cm sous la façade (pas de jour)
    bool facade(int64_t a, int s) const {
        if (bati_.empty()) return false;
        const int64_t L = (int64_t)bati_.size();
        return bati_[(size_t)(((a % L) + L) % L)] & (s > 0 ? 2 : 1);
    }
    std::vector<uint8_t> bati_;   // WFS-137 (ANO-81)
    // WFS-176 : enseignes des lieux (atlas à part), voitures du casino, pigeons, chiens
    vie::AtlasTextes atlasLieux_;
    const Material* enseigneMat_[2] = {nullptr, nullptr};
    Model luxe_[2];
    std::vector<Mesh> pigeon_, chien_[2];
    struct Vol { int64_t seg; float lat, along; double envol = -1e9; };   // groupe de pigeons : envol à l'instant envol
    std::vector<Vol> vols_;
    std::vector<double> promS_;   // abscisse (segments) de chaque promeneur, intégrée comme dans pietons.cpp
    Mesh laisse_{};
    bool basse_ = false;
    // WFS-137 (RG-01, RG-02) : optiques des feux (rouge, orange, vert) de chaque carrefour, jet de la fontaine (repère local)
    struct Optiques { int64_t abs = INT64_MIN; Mesh m[3]{}; };
    std::vector<Optiques> optiques_;
    // abords d'un rond-point (repère de la route au centre de l'anneau) : amorce (bras = −1 : demi-longueur a = départ, b = longueur,
    // h = demi-largeur) ou immeuble (a, b : demi-longueur et demi-profondeur, h : hauteur, k : teinte)
    struct Abord { vec3 c, u, n; float a, b, h; int k, bras; };
    std::vector<chaussee::Axe> routesRp_;   // WFS-212 : axe de la route autour des ronds-points
    std::vector<giratoire::Giratoire> gir_;   // WFS-248 : un ouvrage par rond-point (même ordre que plan_->ronds)
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
    std::map<int64_t, Chunk> chunks_;   // ancienne gestion (repli, lots::g_lotsDecor = false)
    lots::DecorFixe lots_;   // WFS-268 : décor du tour 0, préparé au chargement
    int fixes_ = 0;   // WFS-268 : maillages du décor fixe envoyés à la dernière image
    int64_t limiteTour_ = INT64_MAX;   // WFS-268 : fin des morceaux (longueur du tour pendant la préparation)
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frame_ = 0;
    int lastDraws_ = 0, lastOuv_ = 0, lastInv_ = 0, lastPieces_ = 0, piecesInv_ = 0, lastVie_ = 0;   // lastVie_ : WFS-176
};

} // namespace

WFS_SYSTEM(Ville, 100)

} // namespace wfs
