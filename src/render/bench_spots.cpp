// WaForSpeed — scène du banc d'essai pour les projecteurs (WFS-45) : voir bench_spots.h.
// Valeurs reprises de js/world/garage.js (buildGarage, renderGarage) du jeu web, sans changement.
#include "bench_spots.h"
#include <cmath>
#include <vector>
#include "../core/log.h"
#include "../game/cars.h"
#include "model_loader.h"
#include "renderer.h"

namespace wfs {

// couleur 0xRRGGBB → linéaire (même conversion que la peinture des voitures)
static vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
// couleur 0xRRGGBB → valeur HDR qui ressort à l'écran avec cette couleur (fond uni, comme scene_essai.cpp)
static vec3 fondUni(unsigned h, float exposure) { return lin(h) * (1.f / (0.214f * exposure)); }

// disque horizontal (CircleGeometry couché) ou cylindre plat (dessus + tour), centré en (0, y, 0)
static Mesh disque(Renderer& r, float rayon, float y, float hauteur, int seg) {
    std::vector<Vertex> v; std::vector<uint32_t> idx;
    v.push_back({0, y, 0, 0, 1, 0, 0, 0});
    for (int i = 0; i <= seg; i++) {
        const float a = (float)i / seg * 2 * PI;
        v.push_back({std::cos(a) * rayon, y, -std::sin(a) * rayon, 0, 1, 0, std::cos(a) * rayon, std::sin(a) * rayon});
    }
    for (int i = 0; i < seg; i++) idx.insert(idx.end(), {0u, (uint32_t)i + 1, (uint32_t)i + 2});
    if (hauteur > 0) {   // tour du plateau
        const uint32_t k = (uint32_t)v.size();
        for (int i = 0; i <= seg; i++) {
            const float a = (float)i / seg * 2 * PI, cx = std::cos(a), sz = -std::sin(a);
            v.push_back({cx * rayon, y, sz * rayon, cx, 0, sz, 0, 0});
            v.push_back({cx * rayon, y - hauteur, sz * rayon, cx, 0, sz, 0, 1});
        }
        for (uint32_t i = 0; i < (uint32_t)seg; i++) {
            const uint32_t a = k + i * 2;
            idx.insert(idx.end(), {a, a + 1, a + 3, a, a + 3, a + 2});
        }
    }
    return r.makeMesh(v, idx);
}

void benchSpots(Renderer& r, const std::string& out) {
    const CarDef& car = carById("golf");
    Model voiture;
    if (!loadCarModel(r, car.file, voiture)) { WFS_ERR("spots-sol : voiture %s introuvable", car.file); return; }
    // sol : CircleGeometry(30) ; WFS-99 (RG-03) : couleur 0x1A1C24, rugosité 0,6, métal 0,2 comme le garage (au lieu de
    // 0x101219, 0,22, 0,7) pour que les taches des projecteurs se voient ; plateau : cylindre 3,6 × 0,08, 0x161925, 0,4, 0,6
    Mesh sol = disque(r, 30.f, 0.f, 0.f, 64), plateau = disque(r, 3.6f, 0.08f, 0.08f, 64);
    Material ms; ms.color = {lin(0x1a1c24).x, lin(0x1a1c24).y, lin(0x1a1c24).z, 1}; ms.rough = 0.6f; ms.metal = 0.2f;
    Material mp; mp.color = {lin(0x161925).x, lin(0x161925).y, lin(0x161925).z, 1}; mp.rough = 0.4f; mp.metal = 0.6f;
    Material matSol = r.makeMaterial(ms, nullptr, nullptr, nullptr), matPlateau = r.makeMaterial(mp, nullptr, nullptr, nullptr);
    matSol.flags |= MAT_DOUBLE; matPlateau.flags |= MAT_DOUBLE;
    const vec3 peinture = lin(car.paint);

    // caméra du garage en paysage large (renderGarage : FOV 36°, distance 1,9 × L + 1, regard vers (0, −0,35, 0))
    const float dist = 1.9f * car.d.L + 1.f;
    const vec3 eye{std::sin(0.55f) * dist, 2.3f, std::cos(0.55f) * dist}, look{0, -0.35f, 0};
    FrameParams fp;
    fp.view = lookAt(eye, look, {0, 1, 0});
    fp.proj = perspectiveVk(36.f * PI / 180.f, (float)r.width() / (float)std::max(1u, r.height()), 0.1f);
    fp.camPos = eye; fp.exposure = 0.8f;   // GARAGE_GRADE.exposure
    const vec3 bg = fondUni(0x07070c, fp.exposure);   // scene.background et Fog(0x07070c, 14, 40)
    fp.skyTop = fp.skyHorizon = fp.ground = fp.fogColor = bg;
    fp.fogDensity = 0; fp.fogNear = 14; fp.fogFar = 40;
    fp.sunIntensity = 0; fp.ambient = 1;   // pas de soleil dans le garage
    fp.shadows = false;
    fp.hemiSky = lin(0x5060a0); fp.hemiGround = lin(0x050508); fp.hemiIntensity = 0.6f;
    // SpotLight(couleur, intensité, portée, angle, pénombre, décroissance), cible par défaut : l'origine
    const FrameParams::Spot spots[3] = {
        {{-6, 8, 4}, {0, 0, 0}, lin(0xff8a3a), 1.6f, 30, 0.6f, 0.6f, 1.2f},
        {{6, 8, -3}, {0, 0, 0}, lin(0x40e0ff), 1.6f, 30, 0.6f, 0.6f, 1.2f},
        {{0, 10, 8}, {0, 0, 0}, lin(0xffffff), 1.4f, 30, 0.5f, 0.7f, 1.2f},
    };
    for (int i = 0; i < 3; i++) fp.spots[i] = spots[i];
    fp.spotCount = 3;

    const mat4 carM = translate({0, 0.08f, 0}) * rotateY(0.6f + PI);   // gCar.position.y = 0,08 ; WFS-99 (RG-05) : 3/4 avant
    auto image = [&](const FrameParams& p) {
        if (!r.beginFrame(p)) return;
        r.draw(sol, matSol, mat4::identity());
        r.draw(plateau, matPlateau, mat4::identity());
        r.draw(voiture, carM, &peinture);
        r.endFrame();
    };
    image(fp); image(fp);
    r.capturePng(out + "/spots-sol.png");
    WFS_LOG("capture spots-sol : %d projecteurs, hémisphère %.1f, brouillard linéaire %.0f-%.0f m", fp.spotCount, fp.hemiIntensity, fp.fogNear, fp.fogFar);

    // coût (RG-06) : la mesure GPU de la passe de scène n'est écrite qu'avec les ombres du soleil actives. 240 images
    // contiennent toujours une moyenne complète de 120 images du journal (« passe de scène … ms »).
    FrameParams mesure = fp;
    mesure.shadows = true; mesure.night = false; mesure.hasFocus = true; mesure.focus = {0, 0, 0};
    for (int avec = 0; avec < 2; avec++) {
        mesure.spotCount = avec ? 3 : 0;
        WFS_LOG("spots-sol : mesure du coût, %d projecteur(s) — début", mesure.spotCount);
        for (int i = 0; i < 240; i++) image(mesure);
        WFS_LOG("spots-sol : mesure du coût, %d projecteur(s) — fin", mesure.spotCount);
    }
    r.vk().waitIdle();
    r.destroy(sol); r.destroy(plateau); r.destroy(voiture); r.destroy(matSol); r.destroy(matPlateau);   // WFS-187
}

} // namespace wfs
