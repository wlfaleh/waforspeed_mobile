// WaForSpeed — scènes du banc d'essai des roues animées (WFS-60) : voir roues_banc.h.
#include "roues_banc.h"
#include <cmath>
#include <vector>
#include "../core/log.h"
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "cars.h"
#include "roues.h"
#include "track.h"

namespace wfs {

void benchRoues(Renderer& r, const std::string& out) {
    const CarDef& car = carById("golf");
    Model m;
    if (!loadCarModel(r, car.file, m)) { WFS_ERR("roues : %s introuvable", car.file); return; }
    // sol gris uni de 40 m
    const std::vector<Vertex> v = {{-20, 0, -20, 0, 1, 0, 0, 0}, {20, 0, -20, 0, 1, 0, 1, 0}, {20, 0, 20, 0, 1, 0, 1, 1}, {-20, 0, 20, 0, 1, 0, 0, 1}};
    Mesh sol = r.makeMesh(v, {0, 2, 1, 0, 3, 2});
    Material ms; ms.color = {0.18f, 0.18f, 0.19f, 1}; ms.rough = 0.9f;
    Material matSol = r.makeMaterial(ms, nullptr, nullptr, nullptr); matSol.flags |= MAT_DOUBLE;
    auto lin = [](unsigned h) { auto c = [&](int s) { return std::pow(((h >> s) & 255) / 255.f, 2.2f); }; return vec3{c(16), c(8), c(0)}; };
    const vec3 peinture = lin(car.paint);
    const float rayon = m.nRoues ? m.roues[0].rayon : roues::RW_DEFAUT;
    auto image = [&](const vec3& eye, const vec3& look, const RouesPose& p, const char* nom) {
        FrameParams fp;
        fp.view = lookAt(eye, look, {0, 1, 0});
        fp.proj = perspectiveVk(40.f * PI / 180.f, (float)r.width() / (float)std::max(1u, r.height()), 0.1f);
        fp.camPos = eye; fp.hasFocus = true; fp.focus = {0, 0, 0}; fp.exposure = 0.85f;
        for (int k = 0; k < 2; k++) {
            if (!r.beginFrame(fp)) return;
            r.draw(sol, matSol, mat4::identity());
            r.draw(m, mat4::identity(), &peinture, &p);
            r.endFrame();
        }
        r.capturePng(out + "/" + nom + ".png");
    };
    // de côté, à hauteur de roue, vers l'avant gauche (jantes bien visibles)
    const vec3 cote{-4.2f, 0.75f, -0.9f}, cible{0, 0.35f, -0.4f};
    image(cote, cible, {}, "roues-arret-1");
    image(cote, cible, {}, "roues-arret-2");
    // 0,30 m parcourus : la jante a tourné de 0,30 / rayon (≈ 52° pour la Golf)
    const RouesPose roule = roues::poseJoueur(0, 0, 0.30 / M, rayon);
    image(cote, cible, roule, "roues-roule");
    // virage à droite à l'arrêt (steer = 1) : braquage 0,6 rad, roue intérieure (droite) × 1,12 ; vue de dessus, de l'avant
    const RouesPose virage = roues::poseJoueur(1, 0, 0, rayon);
    image({0.6f, 4.2f, -5.2f}, {0, 0.2f, -0.6f}, virage, "roues-virage");
    WFS_LOG("captures roues-* : %d roues, rayon %.3f m, roulage %.3f rad (%.0f°), braquage %.2f rad (int. %.3f, ext. %.3f)",
            m.nRoues, rayon, roule.roulage, roule.roulage * 180 / PI, virage.braquage, virage.braquage * 1.12f, virage.braquage * 0.9f);
    r.vk().waitIdle();
    r.destroy(sol); r.destroy(m); r.destroy(matSol);   // WFS-187 : jeu de descripteurs rendu
}

} // namespace wfs
