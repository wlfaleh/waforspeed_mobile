// WaForSpeed — socle commun des cartes Campagne et Grand Prix (WFS-53) : voir decor_carte.h.
#include "decor_carte.h"
#include "../render/route_revetements.h"   // WFS-248 : asphalte du giratoire = chaussée de la route
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "../../third_party/stb/stb_image.h"
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/player.h"
#include "chaussee.h"   // WFS-212 : rien sur la chaussée d'une autre partie du tracé
#include "zones.h"

namespace wfs {
namespace carte {
namespace {

// codes du jeu web, dans l'ordre de l'énumération Col (decor_carte.h)
std::array<uint32_t, PAL_W> palette() {
    std::array<uint32_t, PAL_W> p{};
    auto set = [&](int c, std::initializer_list<uint32_t> v) { for (uint32_t h : v) p[(size_t)c++] = h; };
    set(C_VERGE, {0x6d8a3a, 0x4f6b2b, 0x2f3f22, 0x6c8a3e, 0x8a8884});
    set(C_FIELD0, {0x9bb04f, 0x8aa043, 0xcdb259, 0xbfa34c, 0x7e9a3c, 0xa9a35a});          // FIELDS
    set(C_VINE_A, {0x8a6e4a, 0x7a6240, 0xd81e24, 0xe8e8e8, 0x4f7f32, 0x4a7a2e, 0xc9b98f, 0x3a3b3f});
    set(C_OAK, {0x4a3b2e, 0x5c4c3c, 0x3a2e26, 0x8a5236, 0x4e4038});                        // BARK
    set(C_WOOD, {0x8a6a46, 0x2b4520, 0xf2efe6, 0xe8dcc4, 0x8e2a1e, 0xb5532e, 0x2f5f8f, 0x6b6b6b, 0x1d2a38, 0xd7b85a, 0xe9edf2});
    set(C_WALLS0, {0xf2e6c9, 0xe9c9a4, 0xf4dcd0, 0xd9e6ef, 0xf0e1a8, 0xe2d5c3});            // C2.walls
    set(C_CHURCH, {0xe2d7bf, 0x4d7a2a, 0x3a3d44, 0xf0f0f0, 0xc81e24, 0x2a2e3b, 0xe9e9e6, 0x9aa0a8, 0xf2f2f2, 0x0f0f12, 0xffd9a8, 0xe8f4ff});
    set(C_TIRE, {0x141416, 0xc81e24, 0xe8e8e8, 0x7a7f88, 0x5a5f68, 0xd9dde2});
    set(C_SEAT0, {0x1d3f8f, 0xc81e24, 0xe8e8e8, 0x2f8f4f});                                  // C.standSeat
    set(C_AD0, {0xe2262b, 0xffffff, 0x101014, 0xffd23a, 0xffd23a, 0x101014, 0x1d3f8f, 0xffffff});   // AD_TEXTS (fond, texte)
    set(C_MARSHAL, {0xe8e8e8, 0xff8a00, 0xe8e8e8, 0xa8b0b8, 0xe8e8ea, 0x22252b});
    set(C_GARAGE0, {0xc81e24, 0x1d3f8f, 0x2f8f4f, 0xff6a00, 0x101014, 0xffd23a});            // C2.garage
    set(C_FLAGPOLE, {0xd8d8d8});
    set(C_FLAG0, {0xe2262b, 0xffd23a, 0x1d8fb8, 0xffffff, 0x2f8f4f, 0xff6a00});              // C2.flagC
    set(C_PIERRE0, {0xb3a68f, 0xa79a83, 0xc0b39b});   // WFS-178 : pierre calcaire, albédo linéaire 0,39 à 0,49 (charte : 0,40-0,55)
    return p;
}

// teintes du jeu web (VEG_TINT de life.js) : la première de chaque famille est celle du matériau
const vec3 TINT_OAK[3] = {{0.95f, 1, 0.82f}, {0.85f, 0.95f, 0.8f}, {1.05f, 1.02f, 0.78f}};
const vec3 TINT_FIR[3] = {{0.92f, 1, 0.98f}, {0.85f, 0.95f, 0.95f}, {1, 1.02f, 0.95f}};
const vec3 TINT_PINE[2] = {{0.66f, 0.8f, 0.74f}, {0.72f, 0.84f, 0.74f}};
const vec3 TINT_CYPRESS = {0.62f, 0.78f, 0.66f};
// luminosité d'une teinte rapportée à celle du matériau de sa famille
float tkOf(vec3 t, vec3 base) { return (t.x + t.y + t.z) / (base.x + base.y + base.z); }

float rnd(Mulberry32& R) { return (float)R.next(); }

} // namespace

Pose poseAt(Track& t, int64_t a) {
    const TrackPoint p = t.point((double)a * SEG, 0);
    const float th = (float)p.th;
    return {{(float)p.x, (float)p.y, (float)p.z}, {std::cos(th), 0, std::sin(th)}, {std::sin(th), 0, -std::cos(th)}, th};
}

vec3 lin(vec3 c) { return {std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f)}; }
vec3 linHex(uint32_t h) { return lin(veg::hex(h)); }

// ---------------------------------------------------------------- primitives
void tri(Geo& g, vec3 a, vec3 b, vec3 c, vec2 uv) {
    const vec3 n = normalize(cross(b - a, c - a));
    const uint32_t k = (uint32_t)g.v.size();
    for (const vec3& p : {a, b, c}) g.v.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2});
}
void quad(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv) { tri(g, a, b, c, uv); tri(g, a, c, d, uv); }
void quadSol(Geo& g, vec3 a, vec3 b, vec3 c, vec3 d, vec2 uv) {
    if (cross(b - a, c - a).y < 0) std::swap(b, d);
    quad(g, a, b, c, d, uv);
}

void box(Geo& g, vec3 o, vec3 r, vec3 f, float w, float h, float l, vec2 uv, float y0, float taper) {
    auto C = [&](float sx, float sy, float sz) {
        const float k = sy ? taper : 1, x = sx * w / 2 * k, z = sz * l / 2 * k;
        return o + r * x + f * z + vec3{0, y0 + sy * h, 0};
    };
    // ordre des sommets de renderer.js : normales vers l'extérieur
    quad(g, C(-1, 1, -1), C(1, 1, -1), C(1, 1, 1), C(-1, 1, 1), uv);    // dessus
    quad(g, C(1, 0, -1), C(1, 0, 1), C(1, 1, 1), C(1, 1, -1), uv);      // côté r
    quad(g, C(-1, 0, 1), C(-1, 0, -1), C(-1, 1, -1), C(-1, 1, 1), uv);  // côté −r
    quad(g, C(1, 0, 1), C(-1, 0, 1), C(-1, 1, 1), C(1, 1, 1), uv);      // avant
    quad(g, C(-1, 0, -1), C(1, 0, -1), C(1, 1, -1), C(-1, 1, -1), uv);  // arrière
}

void cylAxe(Geo& g, vec3 b, vec3 ax, float rad, float len, int n, vec2 uv) {
    const vec3 e1 = normalize(cross(std::fabs(ax.y) < 0.9f ? vec3{0, 1, 0} : vec3{1, 0, 0}, ax)), e2 = cross(ax, e1);
    const vec3 top = b + ax * len;
    for (int k = 0; k < n; k++) {
        const float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
        const vec3 d0 = e1 * std::cos(a0) + e2 * std::sin(a0), d1 = e1 * std::cos(a1) + e2 * std::sin(a1);
        quad(g, b + d0 * rad, b + d1 * rad, top + d1 * rad, top + d0 * rad, uv);
        tri(g, top, top + d0 * rad, top + d1 * rad, uv);
        tri(g, b, b + d1 * rad, b + d0 * rad, uv);
    }
}
void cyl(Geo& g, vec3 b, float rad, float h, int n, vec2 uv) { cylAxe(g, b, {0, 1, 0}, rad, h, n, uv); }

void cone(Geo& g, vec3 b, float rad, float h, int n, vec2 uv, float yaw) {
    const vec3 top = b + vec3{0, h, 0};
    for (int k = 0; k < n; k++) {
        const float a0 = yaw + 2 * PI * k / n, a1 = yaw + 2 * PI * (k + 1) / n;
        const vec3 p0 = b + vec3{std::cos(a0), 0, std::sin(a0)} * rad, p1 = b + vec3{std::cos(a1), 0, std::sin(a1)} * rad;
        tri(g, p1, p0, top, uv);
    }
}

void toit(Geo& g, vec3 c, vec3 r, vec3 f, float L, float H, float W, vec2 uv) {
    const vec3 up{0, 1, 0};
    vec3 P[2][4];   // section en losange à chaque bout : haut, avant, bas, arrière
    for (int e = 0; e < 2; e++) {
        const vec3 o = c + r * ((e ? 0.5f : -0.5f) * L);
        P[e][0] = o + up * (H / 2); P[e][1] = o + f * (W / 2); P[e][2] = o - up * (H / 2); P[e][3] = o - f * (W / 2);
    }
    for (int k = 0; k < 4; k++) {
        const int k1 = (k + 1) % 4;
        quad(g, P[0][k], P[1][k], P[1][k1], P[0][k1], uv);
    }
    tri(g, P[0][0], P[0][1], P[0][2], uv); tri(g, P[0][0], P[0][2], P[0][3], uv);   // bout −r
    tri(g, P[1][0], P[1][2], P[1][1], uv); tri(g, P[1][0], P[1][3], P[1][2], uv);   // bout +r
}

// ---------------------------------------------------------------- système
void CarteDecor::init(Renderer& r, const GameView& g) {
    active_ = g.map && std::strcmp(g.map->id, carteId()) == 0;
    if (!active_) return;
    track_ = g.track;
    // WFS-212 (RG-02) : axe de la route autour des ronds-points, épingles et lacets, pour n'y rien poser sur une autre branche
    serres_ = chaussee::axes(*track_, {trace::Type::RondPoint, trace::Type::Epingle, trace::Type::Lacets});
    const char* m = std::getenv("WFS_MOMENT");   // banc d'essai et écran Terrain (WFS-41)
    night_ = m && std::strcmp(m, "nuit") == 0;
    const char* dep = std::getenv("WFS_DEPART");   // banc d'essai : zone choisie (segments)
    depart_ = dep ? std::atoll(dep) : 0;
    // palette des couleurs unies (valeurs d'affichage du jeu web : texture sRGB)
    const auto pal = palette();
    uint8_t px[PAL_W * 4] = {};
    for (int c = 0; c < PAL_W; c++) {
        const uint32_t h = pal[(size_t)c];
        px[c * 4] = (h >> 16) & 255; px[c * 4 + 1] = (h >> 8) & 255; px[c * 4 + 2] = h & 255; px[c * 4 + 3] = 255;
    }
    palTex_ = r.vk().uploadTexture(px, PAL_W, 1, true);
    // couleurs unies : éclairage du feuillage (MAT_VTINT, sans reflet), comme le matériau « side » du jeu web
    Material mu; mu.rough = 0.85f; mu.flags = MAT_DOUBLE | MAT_VTINT;
    mats_[P_UNI] = r.makeMaterial(mu, &palTex_, nullptr, nullptr);
    // lampes : part propre (neonK du jeu web : 1 la nuit, 0,5 de jour)
    // WFS-225 (RG-01) : lampes allumées la nuit sur toutes les cartes (la campagne les avait éteintes : envFor lamps = false du web)
    Material mn; mn.rough = 0.4f; mn.flags = MAT_DOUBLE;
    mn.emissive = linHex(0xfff0dc) * (night_ ? 2.f : 0.5f);
    mats_[P_NEON] = r.makeMaterial(mn, &palTex_, nullptr, nullptr);
    // feuillage : atlas du jeu web
    std::vector<uint8_t> png; int w = 0, h = 0, n = 0;
    stbi_uc* lp = readAsset("world/feuillage.png", png) ? stbi_load_from_memory(png.data(), (int)png.size(), &w, &h, &n, 4) : nullptr;
    if (lp) { leafTex_ = r.vk().uploadTexture(lp, (uint32_t)w, (uint32_t)h, true); stbi_image_free(lp); }
    else WFS_ERR("Atlas de feuillage introuvable (world/feuillage.png)");
    auto leafMat = [&](vec3 tint) {
        Material lm; const vec3 c = lin(tint); lm.color = {c.x, c.y, c.z, 1}; lm.rough = 0.8f; lm.cutoff = 0.5f;
        lm.flags = MAT_MASK | MAT_DOUBLE | MAT_VTINT;
        return r.makeMaterial(lm, leafTex_.img ? &leafTex_ : nullptr, nullptr, nullptr);
    };
    mats_[P_FEUILLU] = leafMat(TINT_OAK[0]);
    mats_[P_CONIF] = leafMat(TINT_FIR[0]);
    if (routePropre()) {
        // roadCircuit (renderer.js) : asphalte plus clair, deux traces de gomme, lignes blanches de bord, 256 × 512
        std::vector<uint8_t> rp(256 * 512 * 4);
        auto fill = [&](int x0, int y0, int ww, int hh, int cr, int cg, int cb, float a) {
            for (int y = std::max(0, y0); y < std::min(512, y0 + hh); y++) for (int x = std::max(0, x0); x < std::min(256, x0 + ww); x++) {
                uint8_t* q = &rp[((size_t)y * 256 + x) * 4];
                q[0] = (uint8_t)(q[0] + (cr - q[0]) * a + 0.5f); q[1] = (uint8_t)(q[1] + (cg - q[1]) * a + 0.5f); q[2] = (uint8_t)(q[2] + (cb - q[2]) * a + 0.5f); q[3] = 255;
            }
        };
        fill(0, 0, 256, 512, 0x2b, 0x2d, 0x33, 1);
        Mulberry32 R(6);
        for (int i = 0; i < 2600; i++) { const int v = 34 + (int)(R.next() * 20); const int x = (int)(R.next() * 256), y = (int)(R.next() * 512); fill(x, y, 2, 2, v, v + 1, v + 4, 1); }
        fill(70, 0, 30, 512, 16, 17, 20, 0.35f); fill(150, 0, 30, 512, 16, 17, 20, 0.35f);
        fill(4, 0, 6, 512, 236, 238, 242, 0.95f); fill(246, 0, 6, 512, 236, 238, 242, 0.95f);
        routeTex_ = r.vk().uploadTexture(rp.data(), 256, 512, true);
        Material mr; mr.rough = 0.8f;
        mats_[P_ROUTE] = r.makeMaterial(mr, &routeTex_, nullptr, nullptr);
    }
    // WFS-248 : giratoires reconstruits (avant le décor : les objets qui tomberaient sur l'ouvrage ne sont pas dessinés) ;
    // asphalte de l'anneau avec le matériau de la chaussée de la route (même grain, sans marquage)
    gir_.clear(); girSeg_.clear();
    for (const trace::Section& s : trace::sections(*track_)) if (s.type == trace::Type::RondPoint) {
        const giratoire::Giratoire gg = giratoire::calculer(*track_, s);
        if (!gg.ok) { WFS_ERR("Carte %s : giratoire du segment %lld non reconnu (WFS-248)", carteId(), (long long)s.debut); continue; }
        gir_.push_back(gg); girSeg_.push_back((gg.entree + gg.sortie) / 2);
        WFS_LOG("Carte %s : giratoire reconstruit au segment %lld (anneau %.1f-%.1f m, îlot %.1f m, axe à ±%.2f m du cercle) (WFS-248)", carteId(),
                (long long)s.debut, giratoire::R_COURONNE, giratoire::R_EXT, giratoire::R_ILOT, gg.ecartAxe);
    }
    if (!gir_.empty() && !routePropre()) { Material m; m.flags = MAT_ROAD; m.rough = 0.8f; mats_[P_ROUTE] = route::creerMateriau(r, route::chaussee(), m); }
    // generateDecor : plages [z0, z1) de même zone, tirages de la graine de la carte mulberry32(seed · 31 + 7)
    const int64_t L = len();
    items_.assign((size_t)L, {});
    std::vector<std::pair<int64_t, int64_t>> ranges;
    for (int64_t a = 0; a < L; a++) if (a == 0 || zoneIndex(map(), a, L) != zoneIndex(map(), a - 1, L)) ranges.push_back({a, L});
    for (size_t i = 0; i + 1 < ranges.size(); i++) ranges[i].second = ranges[i + 1].first;
    Mulberry32 D(map().seed * 31 + 7);
    for (auto [z0, z1] : ranges) place(zone(z0), z0, z1, D);
    // touffes d'herbe sur les bas-côtés herbeux : aucun tirage D(), le reste du décor ne change pas
    for (int64_t a = 0; a < L; a += 2) {
        const char* z = zone(a);
        if (std::strcmp(z, "village") && std::strcmp(z, "paddock") && std::strcmp(z, "gravier")) { Item v; v.t = T_VERGE; item(a, v); }
    }
    {   // WFS-256 (accroche, autorisée par le PO) : objets solides pour les chocs du joueur, mêmes filtres que le dessin (buildChunk)
        const float R = (float)RW;
        for (int64_t a = 0; a < L; a++) {
            const Pose p0 = poseAt(*track_, a);
            for (const Item& it : items_[(size_t)a]) {
                chocs::ObstacleRoute o[4]; const float lo = it.t == T_LAMP ? R + 0.8f : it.lat, m = it.t == T_TREE ? 1.5f : 0.5f;
                const int n = solide(it, o);
                if (n == 0) continue;
                const vec3 q = lateral(p0, it.side * lo) + p0.f * it.along;
                if ((!it.ilot && !gir_.empty() && surGiratoire(q, m)) || (lo > 0 && surAutreChaussee(q, a, m))) continue;
                for (int k = 0; k < n; k++) chocs::declarerObjet(*track_, a, o[k], false);
            }
        }
    }
    size_t nb = 0; for (auto& v : items_) nb += v.size();
    WFS_LOG("Carte %s : %zu objets de décor, %zu zones", carteId(), nb, ranges.size());
    WFS_LOG("Carte %s : %s%s", carteId(), night_ ? "nuit" : "jour", depart_ ? " (départ avancé, banc d'essai)" : "");
    lotsTente_ = false;   // WFS-268 : décor fixe préparé à la fin de l'init de la carte (preparerDecor), sinon à la première image
}

// WFS-256 (accroche) : objets communs : arbre (tronc de 25 à 30 cm de rayon ; cyprès 15 cm), lampadaire de village (mât de
// 18 × 18 cm à RW + 0,8 m), projecteur du circuit (mât de 25 cm à RW + 7 m : lampe(), flood)
int CarteDecor::solide(const Item& it, chocs::ObstacleRoute o[4]) const {
    const float R = (float)RW, s = (float)it.side;
    o[0] = {};
    if (it.t == T_TREE) { o[0].lat = s * it.lat; o[0].along = it.along; o[0].r = it.k == K_CYPRESS ? 0.15f : 0.3f; o[0].genre = chocs::Genre::Arbre; return 1; }
    if (it.t == T_LAMP) { o[0].lat = s * (it.k != 0 ? R + 7.f : R + 0.8f); o[0].r = it.k != 0 ? 0.25f : 0.09f; o[0].genre = chocs::Genre::Poteau; return 1; }
    return 0;
}

const char* CarteDecor::zone(int64_t a) const { return zoneId(map(), a, len()); }

float CarteDecor::solY(const char* z, int64_t a, int side, float lat) const {
    Band b[6] = {}; const int n = profil(z, a, side, b);
    for (int i = 0; i < n; i++) if (lat >= b[i].a && lat <= b[i].b) return b[i].ya + (b[i].yb - b[i].ya) * (lat - b[i].a) / (b[i].b - b[i].a);
    return n ? b[n - 1].yb : 0;
}

bool CarteDecor::dansMorceau(int64_t seg, int64_t a0, int64_t a1, int64_t& abs) const {
    const int64_t L = len(), lap = (int64_t)std::floor((double)a0 / L) * L;
    for (int64_t o : {lap, lap + L}) { const int64_t a = seg + o; if (a >= a0 && a < a1) { abs = a; return true; } }
    return false;
}

// envFor (maps.js) : brouillard plus loin de jour, nuit sans lampadaires (campagne) ou bleutée (circuit)
void CarteDecor::ambiance(FrameParams& fp, const GameView& g) {
    if (!active_ || (g.sceneDediee && !g_vueCarte.actif)) return;
    const Env e = env();
    // Game : brouillard exponentiel 0,0009 pour la ville (fogFar 620 m du jeu web) → même rapport, comme la plage
    if (!night_) {
        fp.fogColor = linHex(e.fogJour); fp.fogDensity = 0.0009f * 620.f / e.farJour;
    } else {
        const float k = 2.2f;   // éclaircit les couleurs de nuit du jeu web (son étalonnage n'est pas porté), comme la plage
        fp.skyTop = linHex(0x04040a) * k; fp.skyHorizon = linHex(e.horizonNuit) * k; fp.ground = linHex(0x0a0a10) * k;
        fp.fogColor = linHex(e.fogNuit) * k; fp.fogDensity = 0.0009f * 620.f / e.farNuit;
        fp.sunDir = normalize(vec3{-0.4f, 0.8f, 0.3f}); fp.sunColor = linHex(0x8a96ff); fp.sunIntensity = 0.9f;   // lune
        fp.ambient = 0.7f; fp.exposure = 1.1f; fp.night = true; fp.shadows = false;
    }
}

// banc d'essai : départ avancé (une fois, au premier pas de conduite)
void CarteDecor::collide(double, Player& p, const GameView& g) {
    if (!active_ || !depart_) return;
    const double d = (double)depart_ * SEG;
    p.position = g.track->wrap(p.position + d); p.dist += d;
    depart_ = 0;
}

// ---------------------------------------------------------------- arbres, herbe, lampes (life.js, decor.js)
void CarteDecor::arbre(Track& t, const Item& it, int64_t a, bool lod0, Geo out[P_COUNT]) const {
    // arbres loin de la route (forêt profonde) : version allégée même de près
    const bool lod = lod0 || it.lat > (float)RW + 28;
    const Pose p = poseAt(t, a);
    const char* z = zone(a);
    const vec3 b = lateral(p, it.side * it.lat, solY(z, a, it.side, it.lat) - 0.1f) + p.f * it.along;
    Mulberry32 R((uint32_t)((int)(it.h * 1000) + (int)a * 7 + (it.side > 0 ? 3 : 0) + (int)(it.lat * 10)));
    const bool foret = !std::strcmp(z, "foret");
    const char* zn = zoneName(map(), a, len());
    const veg::Ecorce fir{uv(C_FIR).x, uv(C_FIR).x}, pineLow{uv(C_PINELOW).x, uv(C_PINELOW).x}, pine{uv(C_PINE).x, uv(C_PINELOW).x};
    Geo* S = &out[P_UNI];
    if (it.k == K_CYPRESS) { veg::cypres({S, nullptr, &out[P_CONIF]}, b, it.h, R, tkOf(TINT_CYPRESS, TINT_FIR[0]), fir, lod); return; }
    if (it.k == K_PINE) {
        if (std::strstr(zn, "pins")) {   // « Bois de pins » : pin sylvestre
            const vec3 tint = TINT_PINE[(int)(R.next() * 2)];
            veg::pin({S, nullptr, &out[P_CONIF]}, b, it.h, R, tkOf(tint, TINT_FIR[0]), false, pineLow, pine, lod);
        } else {   // sapin
            const vec3 tint = TINT_FIR[(int)(R.next() * 3)];
            const float clear = foret ? 0.1f + rnd(R) * 0.15f : 0.04f, w = 0.19f + rnd(R) * 0.05f;
            veg::sapin({S, nullptr, &out[P_CONIF]}, b, it.h, R, tkOf(tint, TINT_FIR[0]), clear, w, fir, lod);
        }
    } else {   // chêne
        const vec3 tint = TINT_OAK[(int)(R.next() * 3)];
        veg::Broadleaf o;
        o.cw = foret ? 0.27f : 0.36f; o.ch = foret ? 0.3f : 0.27f; o.cy = foret ? 0.7f : 0.62f; o.trunkH = foret ? 0.5f : 0.4f;
        o.uA = uv(C_OAK).x; o.uB = uv(C_OAK2).x; o.tk = tkOf(tint, TINT_OAK[0]);
        veg::broadleaf({S, nullptr, &out[P_FEUILLU]}, b, it.h, R, o, lod);
    }
    if (foret && !lod && rnd(R) < 0.55f) {   // sous-bois : fougère ou petit buisson
        const float d = (rnd(R) - 0.5f) * 3;
        const vec3 tint = rnd(R) < 0.6f ? vec3{0.8f, 1, 0.75f} : vec3{0.9f, 0.95f, 0.75f};
        const bool fougere = rnd(R) < 0.6f;
        veg::sousBois(out[P_FEUILLU], b + p.f * d + vec3{0.8f, 0.05f, 0}, R, tkOf(tint, TINT_OAK[0]), fougere);
    }
}

bool CarteDecor::procheSerre(int64_t a) const { return chaussee::proche(serres_, len(), a); }

bool CarteDecor::surAutreChaussee(const vec3& q, int64_t a, float marge) const { return chaussee::surAutre(serres_, len(), q, a, marge); }

void CarteDecor::touffes(Track& t, int64_t a, Geo out[P_COUNT]) const {
    static const vec3 CHAMPS[3] = {{1.05f, 1.05f, 0.7f}, {0.95f, 1, 0.65f}, {1.15f, 1.05f, 0.62f}};
    static const vec3 FORET[2] = {{0.7f, 0.85f, 0.65f}, {0.8f, 0.9f, 0.6f}};
    static const vec3 AUTRE[2] = {{0.95f, 1.02f, 0.7f}, {0.88f, 0.98f, 0.66f}};
    const char* z = zone(a);
    const vec3* cols = !std::strcmp(z, "champs") ? CHAMPS : !std::strcmp(z, "foret") ? FORET : AUTRE;
    const int nc = cols == CHAMPS ? 3 : 2;
    Mulberry32 R((uint32_t)((int)a * 7919 + 17));
    const Pose p = poseAt(t, a);
    for (int side : {-1, 1}) {
        const int n = 7 + (int)(rnd(R) * 4);
        for (int k = 0; k < n; k++) {
            const bool far = k > n - 3;
            const float lat = far ? (float)RW + 3 + rnd(R) * 7 : (float)RW + 0.35f + rnd(R) * 2.9f, al = rnd(R) * 2 * (float)SEG_M;
            const vec3 c = lateral(p, side * lat, solY(z, a, side, lat) - 0.04f) + p.f * al;
            const float hgt = (far ? 0.55f : 0.32f) + rnd(R) * 0.4f, yaw = rnd(R) * 3.14f;
            const vec3 col = cols[(int)(rnd(R) * nc)];
            const float j = 0.92f + rnd(R) * 0.16f; rnd(R);   // vCol(c, 1, R)
            if (surAutreChaussee(c, a, 0.5f)) continue;   // WFS-212 (RG-02) : tirages faits, la touffe n'est pas posée
            veg::touffe(out[P_FEUILLU], c, hgt, yaw, j * tkOf(col, TINT_OAK[0]));
        }
    }
}

void CarteDecor::lampe(const Pose& p, int s, bool flood, Geo out[P_COUNT]) const {
    const float R = (float)RW;
    if (flood) {   // projecteur de circuit : grand mât et rampe de lumières
        cyl(out[P_UNI], lateral(p, s * (R + 7)), 0.25f, 18, 8, uv(C_STEEL));
        box(out[P_UNI], lateral(p, s * (R + 6.4f), 17.6f), p.r, p.f, 1.6f, 1.4f, 3.2f, uv(C_POLE));
        box(out[P_NEON], lateral(p, s * (R + 5.58f), 17.75f), p.r, p.f, 0.06f, 1.1f, 2.9f, uv(C_LAMP_C));
        return;
    }
    box(out[P_UNI], lateral(p, s * (R + 0.8f), 0.1f), p.r, p.f, 0.18f, 7.6f, 0.18f, uv(C_POLE));
    box(out[P_UNI], lateral(p, s * (R - 0.6f), 7.5f), p.r, p.f, 2.9f, 0.14f, 0.14f, uv(C_POLE));
    box(out[P_NEON], lateral(p, s * (R - 1.9f), 7.36f), p.r, p.f, 0.9f, 0.12f, 0.38f, uv(C_LAMP_W));
}

// WFS-283 (RG-02) : segment à 100 segments au plus du carrefour simple qui remplace un rond-point (section exRondPoint)
static bool presCarrefourSimple(const Track& t, int64_t a, int64_t L) {
    const int64_t m = ((a % L) + L) % L;
    for (const trace::Section& s : trace::sections(t)) if (s.exRondPoint && m >= s.debut - 100 && m < s.fin + 100) return true;
    return false;
}

// ---------------------------------------------------------------- morceaux (buildChunk de renderer.js)
void CarteDecor::buildChunk(Track& t, int64_t c, bool lod, Geo out[P_COUNT]) {
    const int64_t a0 = c * CH, a1 = std::min(a0 + CH, limiteTour_), L = len();   // WFS-268 : borné au tour à la préparation
    const char* z = zone(a0);   // tout le morceau est dans la même zone (zones alignées sur les morceaux)
    const float R = (float)RW;
    for (int64_t a = a0; a < a1; a++) {
        const Pose p0 = poseAt(t, a), p1 = poseAt(t, a + 1);
        if (routePropre()) {   // chaussée : texture répétée tous les 9 m (v = a · SEG_M / 9)
            const float v0 = (float)(a * SEG_M / 9), v1 = (float)((a + 1) * SEG_M / 9);
            Geo& g = out[P_ROUTE];
            const uint32_t k = (uint32_t)g.v.size();
            const vec3 P[4] = {lateral(p0, -R, 0.012f), lateral(p0, R, 0.012f), lateral(p1, R, 0.012f), lateral(p1, -R, 0.012f)};
            const float U[4] = {0, 1, 1, 0}, V[4] = {v0, v0, v1, v1};
            for (int i = 0; i < 4; i++) g.v.push_back({P[i].x, P[i].y, P[i].z, 0, 1, 0, U[i], V[i]});
            g.i.insert(g.i.end(), {k, k + 1, k + 3, k + 1, k + 2, k + 3});   // face vers le haut : sans MAT_ROAD, scene.frag retourne la normale des faces vues de dos (WFS-97, RG-08)
        }
        if (((a % L) + L) % L == 0)   // ligne de départ en damier
            for (int k = 0; k < 10; k++) {
                const float x0 = -R + k * R / 5;
                quadSol(out[P_UNI], lateral(p0, x0, 0.02f), lateral(p0, x0 + R / 5, 0.02f), lateral(p1, x0 + R / 5, 0.02f), lateral(p1, x0, 0.02f), uv(k % 2 ? C_CHECK_W : C_CHECK_B));
            }
        const bool serre = procheSerre(a);   // WFS-212 : rond-point, épingle
        // WFS-283 (RG-02) : autour du carrefour simple, chaque bande rejoint au segment suivant sa coupe du segment suivant
        // (trapèze) : le sol coupé à la mi-distance de l'autre branche ne fait plus de marches (on y voyait le sol de fond)
        const bool lisse = !serre && presCarrefourSimple(t, a, L);
        for (int s : {-1, 1}) {
            Band b[6] = {}; const int n = profil(z, a, s, b);
            Band b1[6] = {}; const int n1 = lisse ? profil(z, a + 1, s, b1) : 0;
            for (int i = 0; i < n; i++) {
                if (!serre) {
                    const Band& q = n1 == n && b1[i].col == b[i].col ? b1[i] : b[i];   // bord au segment suivant
                    quadSol(out[P_UNI], lateral(p0, s * b[i].a, b[i].ya), lateral(p1, s * q.a, q.ya), lateral(p1, s * q.b, q.yb), lateral(p0, s * b[i].b, b[i].yb), uv(b[i].col));
                    continue;
                }
                // WFS-212 (RG-02) : bande découpée en pas de 1 m (même surface) jusqu'à RW + 40 m ; un pas posé sur une autre branche
                // du tracé (rond-point, épingle) n'est pas dessiné : la chaussée reste nette
                for (float x0 = b[i].a; x0 < b[i].b; ) {
                    const float x1 = x0 < R + 40 ? std::min(b[i].b, x0 + 1.f) : b[i].b;
                    auto y = [&](float x) { return b[i].ya + (b[i].yb - b[i].ya) * (x - b[i].a) / (b[i].b - b[i].a); };
                    const vec3 m = (lateral(p0, s * x0) + lateral(p1, s * x1)) * 0.5f;
                    if (!surAutreChaussee(m, a, 0.f) && !surGiratoire(m, 0.2f))   // WFS-248 : rien sur le giratoire reconstruit
                        quadSol(out[P_UNI], lateral(p0, s * x0, y(x0)), lateral(p1, s * x0, y(x0)), lateral(p1, s * x1, y(x1)), lateral(p0, s * x1, y(x1)), uv(b[i].col));
                    x0 = x1;
                }
            }
        }
        const Pose& p = p0;
        for (size_t gi = 0; gi < gir_.size(); gi++) if (((a % L) + L) % L == ((girSeg_[gi] % L) + L) % L) {   // WFS-248 : l'ouvrage
            std::vector<giratoire::Tri> tris; giratoire::geometrie(gir_[gi], t, tris);
            const int col[giratoire::NB_SURFACES] = {C_WALK, C_ZEBRA, C_WALL2, C_WALK, C_GRASS};
            for (const giratoire::Tri& tr : tris) {
                if (tr.s != giratoire::ASPHALTE) { tri(out[P_UNI], tr.a, tr.b, tr.c, uv(col[tr.s])); continue; }
                Geo& G = out[P_ROUTE];
                const uint32_t k0 = (uint32_t)G.v.size();
                for (const vec3& q : {tr.a, tr.b, tr.c}) { const vec2 u = giratoire::uvChaussee(gir_[gi], q); G.v.push_back({q.x, q.y, q.z, 0, 1, 0, u.x, u.y}); }
                G.i.insert(G.i.end(), {k0, k0 + 1, k0 + 2});
            }
        }
        for (const Item& it : items_[(size_t)(((a % L) + L) % L)]) {
            if (lod && it.petit) continue;   // SMALL_DECOR : rien au loin
            if (!it.ilot && it.t != T_VERGE && !gir_.empty() && surGiratoire(lateral(p0, it.side * (it.t == T_LAMP ? R + 0.8f : it.lat)) + p0.f * it.along, it.t == T_TREE ? 1.5f : 0.5f)) continue;   // WFS-248
            // WFS-212 (RG-02) : rien sur une autre branche du rond-point ni dans l'épingle (arbres : 1,5 m de marge, couronne)
            const float latObj = it.t == T_LAMP ? R + 0.8f : it.lat;   // lampadaire : mât à RW + 0,8 m
            if (it.t != T_VERGE && latObj > 0 && surAutreChaussee(lateral(p0, it.side * latObj) + p0.f * it.along, a, it.t == T_TREE ? 1.5f : 0.5f)) continue;
            if (it.t == T_TREE) arbre(t, it, a, lod, out);
            else if (it.t == T_VERGE) { if (!lod) touffes(t, a, out); }
            else if (it.t == T_LAMP) lampe(p, it.side, it.k != 0, out);
            else dessine(t, it, a, p, lod, out);
        }
    }
}

VueBancCarte g_vueCarte;

// WFS-216 (RG-04) : vue libre du banc d'essai (même principe que la vue de dessus de la Corniche, plage.cpp)
bool CarteDecor::scene(FrameParams& fp, SceneDediee& sd, const GameView& g) {
    if (!active_ || !g_vueCarte.actif) return false;
    fp.view = lookAt(g_vueCarte.oeil, g_vueCarte.cible, {0, 1, 0});
    fp.proj = perspectiveVk(g_vueCarte.fov * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.5f);
    fp.camPos = g_vueCarte.oeil; fp.hasFocus = false;
    sd.route = true; sd.joueur = true; sd.cible = g_vueCarte.cible;
    return true;
}

// WFS-268 : tout le décor fixe d'un tour, construit une fois (écran de chargement), envoyé au GPU une fois. Appelé par la carte
// à la fin de son init (enseignes et panneaux prêts : buildChunk les lit) ; repli : première image (police absente…)
void CarteDecor::preparerDecor(Renderer& r) {
    if (!active_ || lotsTente_) return;
    lotsTente_ = true;
    if (!lots::g_lotsDecor || !LOTS_CARTES_SOCLE) return;
    // revue du lead dev (PR #248) : tour du Grand Prix impair (5159 segments) → les vibreurs du tour 0 n'alternent pas à la ligne
    // de départ ; son décor reste sur l'ancienne gestion jusqu'à WFS-273 (route, immeubles du kit et commerces : préparés)
    if (!std::strcmp(carteId(), "circuit")) return;
    limiteTour_ = len();
    // WFS-269 (accroche) : aucune occlusion précalculée en Campagne et au Grand Prix : le décor uni (P_UNI) la recevait, et c'est lui qui
    // donnait les dents de scie sous les surplombs à la Corniche (revues du lead dev) ; décor préparé par WFS-268 : image identique à main
    lots_.occlusion(0, 0, 0);
    lots_.preparer(r, *track_, P_COUNT, 4, [this](int64_t c, bool lod, std::vector<Vertex>* v, std::vector<uint32_t>* i) {
        Geo out[P_COUNT];
        buildChunk(*track_, c, lod, out);
        for (int p = 0; p < P_COUNT; p++) { v[p] = std::move(out[p].v); i[p] = std::move(out[p].i); }
    }, carteId());
    limiteTour_ = INT64_MAX;
}

void CarteDecor::render(Renderer& r, const GameView& g) {
    if (!active_ || (g.sceneDediee && !g_vueCarte.actif)) return;
    frame_++;
    preparerDecor(r);   // WFS-268 : repli (sans effet si déjà fait)
    Track& t = *g.track;
    const int64_t pc = (int64_t)std::floor(g.player->dist / SEG) / CH;
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const bool far = std::abs(it->first - pc) * CH * SEG_M > LOD_FAR_M;
        if (it->first < pc - BEHIND || it->first > pc + AHEAD || far != it->second.lod) {
            for (Mesh& m : it->second.m) if (m.vb.buf) graveyard_.push_back({frame_, m});
            it = chunks_.erase(it);
        } else ++it;
    }
    if (lots::g_lotsDecor && lots_.pret()) {   // WFS-268 : blocs préparés au chargement, aucune création en course
        int draws = lots_.dessiner(t, pc * CH, BEHIND * CH, (AHEAD + 1) * CH - 1, LOD_FAR_M,
                                   [&](int p, const Mesh& m, const mat4& M) { r.draw(m, mats_[p], M); });
        draws += animes(r, g, pc - BEHIND, pc + AHEAD);
        lastDraws_ = draws;
        if (frame_ % 300 == 1) { unsigned long long cm, cmat; lots_.creationsEnCourse(r, cm, cmat); WFS_LOG("Carte %s : %d appels de dessin (%d blocs préparés) ; créations en course (tout le jeu) : %llu maillages, %llu matériaux", carteId(), draws, lots_.blocs(), cm, cmat); }
        return;
    }
    std::vector<int64_t> missing; std::vector<char> lods;
    for (int64_t c = pc - BEHIND; c <= pc + AHEAD; c++) if (!chunks_.count(c)) { missing.push_back(c); lods.push_back(std::abs(c - pc) * CH * SEG_M > LOD_FAR_M); }
    if (!missing.empty()) {
        // poses calculées ici (thread principal) ; les threads ne font que lire la piste
        t.pose(missing.front() * CH - 2); t.pose((missing.back() + 1) * CH + 40);
        std::vector<std::array<Geo, P_COUNT>> cpu(missing.size());
        jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunk(t, missing[i], lods[i], cpu[i].data()); });
        for (size_t i = 0; i < missing.size(); i++) {
            Chunk& ch = chunks_[missing[i]]; ch.lod = lods[i];
            for (int p = 0; p < P_COUNT; p++) if (!cpu[i][p].i.empty()) ch.m[p] = r.makeMesh(cpu[i][p].v, cpu[i][p].i, true);
        }
    }
    for (auto it = graveyard_.begin(); it != graveyard_.end();) {
        if (frame_ > it->first + 3) { r.destroy(it->second); it = graveyard_.erase(it); } else ++it;
    }
    const mat4 I = mat4::identity();
    int draws = 0;
    for (auto& [k, ch] : chunks_) for (int p = 0; p < P_COUNT; p++) if (ch.m[p].indexCount) { r.draw(ch.m[p], mats_[p], I); draws++; }
    draws += animes(r, g, pc - BEHIND, pc + AHEAD);
    lastDraws_ = draws;
    if (frame_ % 300 == 1) WFS_LOG("Carte %s : %d appels de dessin (%zu morceaux)", carteId(), draws, chunks_.size());
}

void CarteDecor::shutdown(Renderer& r) {
    if (!active_) return;
    WFS_LOG("Carte %s : %d appels de dessin à la dernière image", carteId(), lastDraws_);
    r.vk().waitIdle();
    for (auto& [k, ch] : chunks_) for (Mesh& m : ch.m) if (m.vb.buf) r.destroy(m);
    chunks_.clear();
    lots_.liberer(r);   // WFS-268
    for (auto& gm : graveyard_) r.destroy(gm.second);
    graveyard_.clear();
    for (Material& m : mats_) r.destroy(m);   // WFS-187 (accroche) : jeux de descripteurs rendus
    shutdownCarte(r);
    for (Image* im : {&palTex_, &leafTex_, &routeTex_}) if (im->img) { r.vk().destroy(*im); *im = Image{}; }
    active_ = false;
}

} // namespace carte
} // namespace wfs
