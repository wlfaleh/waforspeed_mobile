// WaForSpeed — garage : décor 3D de la scène dédiée (WFS-534) : voir garage_decor.h. Code repris tel quel de garage.cpp.
#include "garage_decor.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "../core/system.h"
#include "garage_commun.h"

namespace wfs::ecrans::garage {
namespace {
// WFS-99 (RG-04) : intensité du projecteur de contre-jour, 0,25 × celle des projecteurs du diffus (1,6)
constexpr float CONTRE_JOUR = 0.25f * 1.6f;
// couleur 0xRRGGBB → valeur HDR qui ressort à l'écran avec cette couleur (fond uni, comme bench_spots.cpp)
vec3 fondUni(unsigned h, float exposure) { return lin(h) * (1.f / (0.214f * exposure)); }

// disque horizontal (CircleGeometry couché) ou cylindre plat (dessus + tour) : repris de bench_spots.cpp (WFS-45)
Mesh disque(Renderer& r, float rayon, float y, float hauteur, int seg) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    v.push_back({0, y, 0, 0, 1, 0, 0, 0});
    for (int i = 0; i <= seg; i++) {
        const float a = (float)i / seg * 2 * PI;
        v.push_back({std::cos(a) * rayon, y, -std::sin(a) * rayon, 0, 1, 0, std::cos(a) * rayon, std::sin(a) * rayon});
    }
    for (int i = 0; i < seg; i++) idx.insert(idx.end(), {0u, (uint32_t)i + 1, (uint32_t)i + 2});
    if (hauteur > 0) {
        const uint32_t k = (uint32_t)v.size();
        for (int i = 0; i <= seg; i++) {
            const float a = (float)i / seg * 2 * PI, cx = std::cos(a), sz = -std::sin(a);
            v.push_back({cx * rayon, y, sz * rayon, cx, 0, sz, 0, 0});
            v.push_back({cx * rayon, y - hauteur, sz * rayon, cx, 0, sz, 0, 1});
        }
        for (uint32_t i = 0; i < (uint32_t)seg; i++) { const uint32_t a = k + i * 2; idx.insert(idx.end(), {a, a + 1, a + 3, a, a + 3, a + 2}); }
    }
    return r.makeMesh(v, idx);
}
// TorusGeometry(R, r, segTube, segAnneau) couché (rotation.x = π/2), à la hauteur y
Mesh tore(Renderer& r, float R, float rt, int st, int sa, float y) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    for (int i = 0; i <= sa; i++) for (int j = 0; j <= st; j++) {
        const float u = (float)i / sa * 2 * PI, w = (float)j / st * 2 * PI;
        const float cx = std::cos(u), cz = std::sin(u), nx = std::cos(w) * cx, ny = std::sin(w), nz = std::cos(w) * cz;
        v.push_back({(R + rt * std::cos(w)) * cx, y + rt * std::sin(w), (R + rt * std::cos(w)) * cz, nx, ny, nz, 0, 0});
    }
    for (int i = 0; i < sa; i++) for (int j = 0; j < st; j++) {
        const uint32_t a = i * (st + 1) + j, b = a + st + 1;
        idx.insert(idx.end(), {a, b, a + 1, b, b + 1, a + 1});
    }
    return r.makeMesh(v, idx);
}
// BoxGeometry(w, h, d) centrée
Mesh boite(Renderer& r, float w, float h, float d) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    const float X = w / 2, Y = h / 2, Z = d / 2;
    const float f[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (auto& n : f) {
        const vec3 N{n[0], n[1], n[2]}, U = std::abs(n[1]) > 0 ? vec3{1, 0, 0} : vec3{0, 1, 0}, V = cross(N, U);
        const uint32_t k = (uint32_t)v.size();
        for (int s = 0; s < 4; s++) {
            const float a = (s & 1) ? 1.f : -1.f, b = (s & 2) ? 1.f : -1.f;
            const vec3 p{N.x * X + (U.x * a + V.x * b) * X, N.y * Y + (U.y * a + V.y * b) * Y, N.z * Z + (U.z * a + V.z * b) * Z};
            v.push_back({p.x, p.y, p.z, N.x, N.y, N.z, 0, 0});
        }
        idx.insert(idx.end(), {k, k + 1, k + 3, k, k + 3, k + 2, k, k + 3, k + 1, k, k + 2, k + 3});   // deux faces : sens sans importance
    }
    return r.makeMesh(v, idx);
}
} // namespace

void DecorGarage::init(Renderer& r) {
    sol_ = disque(r, 30.f, 0.f, 0.f, 64); plateau_ = disque(r, 3.6f, 0.08f, 0.08f, 64);
    anneau_ = tore(r, 3.62f, 0.035f, 8, 96, 0.08f); barre_ = boite(r, 0.08f, 9.f, 0.08f);
    // WFS-99 (RG-03, ANO-23) : sol mat (#1A1C24, rugosité 0,6, métal 0,2) au lieu du miroir de garage.js
    // (#101219, 0,22, 0,7) : le diffus des 3 projecteurs y dessine ses taches orange, cyan et blanche
    Material ms; ms.color = {lin(0x1a1c24).x, lin(0x1a1c24).y, lin(0x1a1c24).z, 1}; ms.rough = 0.6f; ms.metal = 0.2f;
    Material mp; mp.color = {lin(0x161925).x, lin(0x161925).y, lin(0x161925).z, 1}; mp.rough = 0.4f; mp.metal = 0.6f;
    matSol_ = r.makeMaterial(ms, nullptr, nullptr, nullptr); matPlateau_ = r.makeMaterial(mp, nullptr, nullptr, nullptr);
    matSol_.flags |= MAT_DOUBLE; matPlateau_.flags |= MAT_DOUBLE;
    // MeshBasicMaterial (non éclairé) : couleur noire, tout en émission
    for (int i = 0; i < 3; i++) {
        Material me; me.color = {0, 0, 0, 1}; me.rough = 1; me.metal = 0;
        me.emissive = lin(i == 0 ? 0x00e5ff : i == 1 ? 0xff6a00 : 0x00b8d4) * 2.2f;
        matNeon_[i] = r.makeMaterial(me, nullptr, nullptr, nullptr); matNeon_[i].flags |= MAT_DOUBLE;
    }
}

void DecorGarage::shutdown(Renderer& r) {
    r.destroy(sol_); r.destroy(plateau_); r.destroy(anneau_); r.destroy(barre_);
    for (Material* m : {&matSol_, &matPlateau_, &matNeon_[0], &matNeon_[1], &matNeon_[2]}) r.destroy(*m);   // WFS-187 (accroche) : matAnneau_ = copie de matNeon_[0]
    matAnneau_.set = VK_NULL_HANDLE;
}

void DecorGarage::regler(FrameParams& fp, SceneDediee& s, const GameView& g, const CarDef& car) const {
    const float dist = 1.9f * car.d.L + 1.f;   // renderGarage en paysage large : 1,9 × L + 1
    const vec3 eye{std::sin(0.55f) * dist, 2.3f, std::cos(0.55f) * dist}, look{0, -0.35f, 0};
    fp.view = lookAt(eye, look, {0, 1, 0});
    fp.proj = perspectiveVk(36.f * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
    fp.camPos = eye; fp.exposure = 0.8f;   // GARAGE_GRADE.exposure
    const vec3 bg = fondUni(0x07070c, fp.exposure);   // scene.background et Fog(0x07070c, 14, 40)
    fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = bg;
    fp.fogDensity = 0; fp.fogNear = 14; fp.fogFar = 40;
    fp.sunIntensity = 0; fp.ambient = 1; fp.shadows = false; fp.night = false;
    fp.hemiSky = lin(0x5060a0); fp.hemiGround = lin(0x050508); fp.hemiIntensity = 0.6f;
    const FrameParams::Spot spots[3] = {
        {{-6, 8, 4}, {0, 0, 0}, lin(0xff8a3a), 1.6f, 30, 0.6f, 0.6f, 1.2f},
        {{6, 8, -3}, {0, 0, 0}, lin(0x40e0ff), 1.6f, 30, 0.6f, 0.6f, 1.2f},
        {{0, 10, 8}, {0, 0, 0}, lin(0xffffff), 1.4f, 30, 0.5f, 0.7f, 1.2f},
    };
    for (int i = 0; i < 3; i++) fp.spots[i] = spots[i];
    // WFS-99 (RG-04, ANO-23) : 4e projecteur cyan en contre-jour, derrière la voiture à l'opposé de la caméra :
    // liseré sur la carrosserie (une voiture noire se détache du fond noir), force 0,25 × le diffus des projecteurs
    fp.spots[3] = {{-std::sin(0.55f) * 7.f, 3.2f, -std::cos(0.55f) * 7.f}, {0, 0.6f, 0}, lin(0x00e5ff), CONTRE_JOUR, 30, 0.6f, 0.6f, 1.2f};
    fp.spotCount = 4;
    s.route = false; s.joueur = false; s.commandes = false; s.cible = look;
}

void DecorGarage::dessiner(Renderer& r, unsigned glow) {
    r.draw(sol_, matSol_, mat4::identity());
    r.draw(plateau_, matPlateau_, mat4::identity());
    // anneau néon de la couleur glow de la voiture (gRing.material.color.set(myCar.glow))
    // Renderer::draw garde un pointeur vers le matériau jusqu'à endFrame : matériau membre, pas local
    matAnneau_ = matNeon_[0]; matAnneau_.emissive = lin(glow) * 2.2f;
    r.draw(anneau_, matAnneau_, mat4::identity());
    for (int i = 0; i < 9; i++) {   // 9 barres lumineuses en arc derrière la voiture
        // WFS-99 (RG-02, ANO-22) : arc de garage.js (rayon 16, centre z = 4) tourné vers la droite et resserré
        // (−0,9 + i × 0,32 → −0,36 + i × 0,18) : toute barre visible tombe à x ≥ 0,53 de l'écran (derrière la
        // voiture ou à droite), jamais sur le titre, l'accroche ni la barre d'index ; 4 barres visibles en 16/9
        const float a = -0.36f + i * 0.18f;
        r.draw(barre_, matNeon_[i % 3 == 1 ? 1 : 2], translate({std::sin(a) * 16, 4.5f, -std::cos(a) * 16 + 4}));
    }
}

} // namespace wfs::ecrans::garage
