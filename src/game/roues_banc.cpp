// WaForSpeed — scènes du banc d'essai des roues animées (WFS-60) : voir roues_banc.h.
#include "roues_banc.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../core/log.h"
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "cars.h"
#include "course5.h"
#include "fantome.h"
#include "game.h"
#include "traffic.h"
#include "traffic_system.h"
#include "../ui/ecrans/reglages.h"   // qualité Basse (scène police-basse)
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

// ---------- WFS-452 : banc « roues-suivies » (voir roues_banc.h) ----------
int benchRouesSuivies(Game& g, Renderer& r, const std::string& out) {
    const ModeCourse mode0 = modeCourse();
    int ecarts = 0;
    constexpr double PAS = 1.0 / 60;   // une mise à jour = 2 pas de 1/120 s ; une image dessinée sur deux : 30 img/s
    struct Scene { const char* nom; ModeCourse mode; bool nuit; int images; bool basse = false; };
    const Scene scenes[] = {{"c5", ModeCourse::Course5, false, 12}, {"c5-nuit", ModeCourse::Course5, true, 12},
                            {"police", ModeCourse::Poursuite, false, 14}, {"police-basse", ModeCourse::Poursuite, false, 14, true},
                            {"fantome", ModeCourse::Checkpoint, false, 12}, {"trafic", ModeCourse::Checkpoint, false, 24}};
    const int qualite0 = ecrans::reglages().qualite;
    const char* seule = std::getenv("WFS_ROUES_SCENE");   // banc : une seule scène (mise au point)
    for (const Scene& sc : scenes) {
        if (seule && std::string(seule) != sc.nom) continue;
        double msRendu[2] = {0, 0}; int nRendu[2] = {0, 0}; double appels[2] = {0, 0}, triangles[2] = {0, 0};
        for (int passe = 0; passe < 2; passe++) {
            roues::g_avant452 = passe == 0;
            setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", sc.nuit ? "nuit" : "jour", 1);
            modeCourse() = sc.mode;
            ecrans::reglages().qualite = sc.basse ? ecrans::QUALITE_BASSE : qualite0; ecrans::appliquerQualiteNiveau(r);
            if (sc.mode == ModeCourse::Course5) kmCourse5() = 10;
            if (!g.init(r, "golf", "ville")) { WFS_ERR("roues-suivies : chargement de la Ville impossible"); ecarts++; continue; }
            g.allerA(Ecran::Course); g.camMode = 2;
            double t = 1500;
            // caméra « vitrine » autour du joueur : point de vue à avantM devant et droiteM à droite de sa voiture, à hauteur m
            auto viser = [&](double avantM, double droiteM, float hauteur) {
                const Game::Vue v = g.calculerVue(t);
                const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
                const vec3 o = f * (float)avantM + d * (float)droiteM;
                g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
            };
            const float cible = 30;   // km/h (RG-06 : vers 30 km/h, pas d'effet stroboscopique sur les rayons)
            auto pas = [&](bool rendu) {
                g.setAutopilot(g.player().kmh() < cible);
                g.update(PAS); t += PAS;
                if (rendu) {
                    const auto a = std::chrono::steady_clock::now();
                    g.render(t);
                    nRendu[passe]++; msRendu[passe] += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count();
                }
            };
            for (int i = 0; i < 60 * 25 && g.player().kmh() < cible - 1; i++) pas(i % 8 == 0);   // départ, puis 30 km/h
            if (g.player().kmh() < cible - 3) { WFS_ERR("roues-suivies : %s : %.0f km/h au lieu de %.0f", sc.nom, g.player().kmh(), cible); ecarts++; }
            const double px = g.player().playerX;
            const int cote = px <= 0 ? 1 : -1;            // la voiture suivie est du côté où la route est la plus large
            int voiture = -1, fileDepart = -1;
            // écart latéral (m) entre le joueur et la voiture suivie : la caméra se place 4,5 m au-delà de cette voiture
            double ecartLat = 0.62 * RW;
            if (sc.mode == ModeCourse::Course5) { course5BancSuiveurs({{1.3, cote > 0 ? 2 : 0}, {-60, 1}, {-75, 1}, {-90, 1}}); ecartLat = std::abs((cote > 0 ? 0.62 : -0.62) - px) * RW; }   // essieu arrière au milieu de l'image
            if (sc.mode == ModeCourse::Poursuite) { poursuiteBancPolice(true, 1.6, px + cote * 0.62); ecartLat = 0.62 * RW; }
            if (sc.nom[0] == 'f') {   // fantôme : trace d'une voiture à côté du joueur, à sa vitesse
                fantome::Trace tr; tr.terrain = "ville"; tr.car = "m4"; tr.peinture = 0x3a6ea5;
                const double v = g.player().speed;
                for (int k = 0; k < 200; k++) tr.e.push_back({g.player().dist + 1.3 / M + v * k * fantome::PAS_S, px + cote * 0.62, 0, v});
                courseBancFantome(tr);
            }
            if (sc.nom[0] == 't') {   // trafic : le joueur 14 m derrière une voiture, dans sa file, à sa vitesse
                trafficBenchPlace(14, 0, 0, true);
                for (int i = 0; i < 4; i++) pas(false);
                voiture = trafficBenchCar();
            }
            for (int i = 0; i < 40; i++) pas(i % 2 == 1);   // mise en place (voitures tenues, filtres du braquage établis)
            for (int k = 0; k < sc.images; k++) {
                const double u = (double)k / sc.images;
                // police : glisse de côté (4,6 → 3,4 m, sans contact), place donnée à chaque mise à jour (pas d'à-coups)
                auto police = [&](double uu) { if (sc.mode == ModeCourse::Poursuite) poursuiteBancPolice(true, 1.6, px + cote * (0.62 - 0.17 * std::min(1.0, uu * 1.6))); };
                if (sc.nom[0] == 't' && k == 6) {   // changement de file forcé, après 6 images en ligne droite
                    Traffic* tf = activeTrafficMut();
                    if (tf && voiture >= 0 && voiture < (int)tf->cars.size()) {
                        TrafficCar& c = tf->cars[(size_t)voiture];
                        fileDepart = c.lane;
                        c.target = c.lane > 0 ? c.lane - 1 : c.lane + 1; c.pending = false; c.signal = c.target > c.lane ? 1 : -1;
                    } else { WFS_ERR("roues-suivies : trafic : aucune voiture placée"); ecarts++; }
                }
                // point de vue : roue arrière de côté (c5, fantôme) ; 3/4 avant (police) ; de face, au-dessus (trafic)
                if (sc.nom[0] == 't') viser(19.5, 1.2, 1.9);
                else if (sc.mode == ModeCourse::Poursuite) viser(5.5, cote * (ecartLat + 4.5), 1.4);
                else if (sc.nom[0] == 'f') viser(-0.4, cote * (ecartLat + 4.5), 2.3);   // fantôme : plus haut, le joueur passe derrière
                else viser(-0.4, cote * (ecartLat + 4.5), 0.75);
                police(u); pas(false); police(u + 0.5 / sc.images); roues::g_journal452 = true; pas(true); roues::g_journal452 = false;
                { uint32_t a = 0, tri = 0; r.fileScene(a, tri); appels[passe] += a; triangles[passe] += tri; }   // RG-05 : coût de l'image
                char nom[96]; std::snprintf(nom, sizeof nom, "roues-%s-%s-%02d", sc.nom, passe == 0 ? "avant" : "apres", k);
                r.capturePng(out + "/" + nom + ".png");
                const Traffic* tf = activeTraffic();
                const TrafficCar* c = tf && voiture >= 0 && voiture < (int)tf->cars.size() ? &tf->cars[(size_t)voiture] : nullptr;
                if (c) WFS_LOG("capture %s : %.0f km/h, voiture du trafic %.1f m devant, file %d → %d, latéral %.2f m", nom, g.player().kmh(),
                               (c->z - g.player().position) * M, c->lane, c->target, c->offset * RW);
                else WFS_LOG("capture %s : %.0f km/h", nom, g.player().kmh());
            }
            if (sc.nom[0] == 't' && fileDepart < 0) { WFS_ERR("roues-suivies : trafic : pas de changement de file"); ecarts++; }
            course5BancSuiveurs({}); poursuiteBancPolice(false, 0, 0);
            g.setAutopilot(false); g.camMode = 0;
            g.shutdown();
        }
        WFS_LOG("roues-suivies (WFS-452, RG-05) : %s : temps moyen du rendu d'une image (processeur, lavapipe) : avant %.2f ms (%d images), après %.2f ms (%d images)",
                sc.nom, msRendu[0] / std::max(1, nRendu[0]), nRendu[0], msRendu[1] / std::max(1, nRendu[1]), nRendu[1]);
        WFS_LOG("roues-suivies (WFS-452, RG-05) : %s : appels de dessin par image (moyenne des captures) : avant %.0f, après %.0f ; triangles : avant %.0f, après %.0f",
                sc.nom, appels[0] / sc.images, appels[1] / sc.images, triangles[0] / sc.images, triangles[1] / sc.images);
    }
    roues::g_avant452 = false;
    modeCourse() = mode0;
    ecrans::reglages().qualite = qualite0; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    WFS_LOG("roues-suivies (WFS-452) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs
