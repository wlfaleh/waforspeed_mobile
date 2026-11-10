// WaForSpeed — banc d'essai des feux arrière (WFS-109, RG-15) : voir feux_modele.h.
#include "feux_modele.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "../core/ecran.h"
#include "../game/camera_vue.h"
#include "correctifs_voitures.h"
#include "../core/log.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/track.h"
#include "renderer.h"

namespace wfs {

// RG-15 : la Golf du joueur (feu arrière découpé, RG-13) vue de la caméra arrière, en ville : (a) de jour au repos, feux
// éteints ; (b) au freinage, de jour comme de nuit, feux du modèle rouge vif ; (c) en marche arrière, feux de recul blancs
void benchFeuxFrein(Game& g, Renderer& r, const std::string& out) {
    double t = 1200;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto capture = [&](const std::string& nom) {
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : %.0f km/h, frein %d, recul %d (WFS-109)", nom.c_str(), g.player().kmh(), (int)g.player().braking,
                (int)g.player().reversing);
    };
    for (const char* moment : {"jour", "nuit"}) {
        const bool nuit = moment[0] == 'n';
        if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
        if (!g.init(r, "golf", "ville")) { WFS_ERR("feux : chargement impossible"); continue; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 240; i++) image(i % 8 == 0);
        if (!nuit) capture("feux-repos-jour");
        // pédale de frein tenue, pilote coupé : 0,2 s de freinage (transition de 80 ms finie)
        g.setAutopilot(false);
        const float W = (float)r.width(), H = (float)r.height();
        g.onTouch({7, W * 0.66f, H * 0.8f, true});
        for (int i = 0; i < 12; i++) image(true);
        capture(std::string("feux-frein-") + moment);
        if (!nuit) {   // marche arrière : frein tenu jusqu'à l'arrêt, puis recul
            for (int i = 0; i < 1800 && !g.player().reversing; i++) image(i % 30 == 0);
            for (int i = 0; i < 90; i++) image(i % 8 == 0);
            capture("feux-recul");
        }
        g.onTouch({7, W * 0.66f, H * 0.8f, false});
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
}


void benchVitrinesFeux(Game& g, Renderer& r, const std::string& out) {
    std::vector<std::string> ids;
    const char* choix = std::getenv("WFS_VOITURES");
    if (choix) { std::istringstream is(choix); std::string id; while (is >> id) ids.push_back(id); }
    else for (const CarDef& c : CARS) ids.push_back(c.id);
    if (std::getenv("WFS_VITRINE_NUIT")) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");   // WFS-193 (banc) : vitrines de nuit
    for (const std::string& id : ids) {
        if (!g.init(r, id.c_str(), "ville")) { WFS_ERR("vitrine feux : chargement de %s impossible", id.c_str()); continue; }
        if (choix) { g.camMode = 2; g.orbit = 0.7f; g.render(0); g.render(0); r.capturePng(out + "/vitrine-" + id + "-0.png"); }
        // WFS-192 (accroche, essai local) : logos vus de près : WFS_VITRINE_ANGLES="<angle de l'orbite en rad> …" (0 : arrière,
        // 3,14 : avant, 1,57 et 4,71 : côtés) : captures vitrine-<id>-a<n> ; rien sans la variable
        if (const char* an = std::getenv("WFS_VITRINE_ANGLES")) {
            std::istringstream ia(an); float a; int k = 0; g.camMode = 2;
            while (ia >> a) { g.orbit = a; g.render(0); g.render(0); r.capturePng(out + "/vitrine-" + id + "-a" + std::to_string(k++) + ".png"); }
            // WFS-193 (banc) : vue basse, au niveau du pare-chocs : WFS_VITRINE_BAS="<angle> <hauteur m> <recul m>" → vitrine-<id>-bas
            // WFS-358 : plusieurs vues séparées par « ; » → vitrine-<id>-bas, -bas1, -bas2…
            if (const char* bas = std::getenv("WFS_VITRINE_BAS")) {
                std::istringstream vues(bas); std::string vue; int k = 0;
                const float h0 = g.orbitHauteur, d0 = g.orbitRecul;   // valeurs d'avant, remises après les captures
                while (std::getline(vues, vue, ';')) {
                    std::istringstream ib(vue); float h = 0.6f, d = 4.f; if (!(ib >> g.orbit)) continue;
                    if (float v; ib >> v) { h = v; if (ib >> v) d = v; }   // hauteur et recul facultatifs (0,6 m et 4 m par défaut)
                    g.orbitHauteur = h; g.orbitRecul = d; g.render(0); g.render(0);
                    r.capturePng(out + "/vitrine-" + id + "-bas" + (k ? std::to_string(k) : "") + ".png"); k++;
                }
                g.orbitHauteur = h0; g.orbitRecul = d0;
            }
        }
        double t = 1300;
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 120; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 8 == 0) g.render(t); }
        g.setAutopilot(false);
        const float W = (float)r.width(), H = (float)r.height();
        g.onTouch({7, W * 0.66f, H * 0.8f, true});   // pédale de frein tenue : feux stop allumés (transition de 80 ms finie)
        for (int i = 0; i < 12; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); }
        g.camMode = 2; g.orbit = 0.45f;              // vitrine, de trois quarts arrière
        g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/vitrine-" + id + "-feux.png");
        WFS_LOG("capture vitrine-%s-feux : frein %d (WFS-170)", id.c_str(), (int)g.player().braking);
        g.onTouch({7, W * 0.66f, H * 0.8f, false});
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");   // WFS-193 : rien de laissé aux bancs suivants
}

// ---------- WFS-413 : banc des optiques avant (WFS_BANC=phares-optiques) ----------
// Captures demandées par les règles du ticket, de nuit en Ville : (4) planche des voitures de face, optiques allumées ;
// (5) gros plans à moins de 2 m d'une optique allumée ; (1) voiture du joueur de face sur la chaussée ; (3) trafic vu du
// conducteur ; témoin de jour (rien d'allumé). Les voitures de la planche sont celles de PHARES_AVANT (17, sans les 3 retirées).
void benchPharesOptiques(Game& g, Renderer& r, const std::string& out) {
    setenv("WFS_MOMENT", "nuit", 1);
    double t = 900;
    auto image = [&](bool rendu) { g.update(1.0 / 60); t += 1.0 / 60; if (rendu) g.render(t); };
    auto capture = [&](const std::string& nom) { g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s (WFS-413)", nom.c_str()); };
    // caméra de vitrine tournée vers l'avant de la voiture (+ décalage en radians), à la hauteur et au recul donnés
    auto face = [&](float decale, float hauteur, float recul) {
        const Game::Vue v = g.calculerVue(t); g.camMode = 2; g.orbit = std::atan2(v.fwd.x, v.fwd.z) + decale; g.orbitHauteur = hauteur; g.orbitRecul = recul;
    };
    const float h0 = g.orbitHauteur, r0 = g.orbitRecul;
    // (4) planche : centre de chaque capture (la moitié de l'image) réduit de moitié, grille 5 × 4
    std::vector<const CarDef*> voitures;
    for (int i = 0; i < CAR_COUNT; i++) {
        bool connue = false;
        for (const correctifs::PhareVoiture& p : correctifs::PHARES_AVANT) connue = connue || !std::strcmp(p.voiture, CARS[i].id);
        if (connue) voitures.push_back(&CARS[i]);
    }
    constexpr int COL = 5, LIG = 4;
    const int W = r.width(), H = r.height(), CW = W / 4, CH = H / 4;
    std::vector<uint8_t> planche((size_t)COL * CW * LIG * CH * 3, 12);
    const std::string tmp = out + "/.phares-tmp.png";
    std::string ordre;
    for (size_t k = 0; k < voitures.size() && (int)k < COL * LIG; k++) {
        const CarDef& c = *voitures[k];
        if (!g.init(r, c.id, "ville")) { WFS_ERR("phares-optiques : %s impossible à charger", c.id); continue; }
        g.setAutopilot(false);
        for (int i = 0; i < 4; i++) image(i == 3);
        face(0.f, 0.75f, 4.6f);
        g.render(t); capture(".phares-tmp");
        ordre += std::string(ordre.empty() ? "" : ", ") + c.id;
        int w = 0, h = 0, n = 0;
        stbi_uc* px = stbi_load(tmp.c_str(), &w, &h, &n, 3);
        g.shutdown();
        if (!px) { WFS_ERR("phares-optiques : capture de %s illisible", c.id); continue; }
        const int x0 = (int)(k % COL) * CW, y0 = (int)(k / COL) * CH, stride = COL * CW * 3, ox = w / 4, oy = h / 4;
        for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
            unsigned sm[3] = {0, 0, 0};
            for (int yy = 0; yy < 2; yy++) for (int xx = 0; xx < 2; xx++) {
                const stbi_uc* q = px + ((size_t)std::min(h - 1, oy + y * 2 + yy) * w + std::min(w - 1, ox + x * 2 + xx)) * 3;
                sm[0] += q[0]; sm[1] += q[1]; sm[2] += q[2];
            }
            uint8_t* d = &planche[(size_t)(y0 + y) * stride + (size_t)(x0 + x) * 3];
            for (int j = 0; j < 3; j++) d[j] = (uint8_t)(sm[j] / 4);
        }
        stbi_image_free(px);
    }
    std::remove(tmp.c_str());
    stbi_write_png((out + "/phares-planche.png").c_str(), COL * CW, LIG * CH, 3, planche.data(), COL * CW * 3);
    WFS_LOG("capture phares-planche : %zu voitures de face, de nuit, de gauche à droite et de haut en bas : %s (WFS-413)", voitures.size(), ordre.c_str());
    // (5) gros plans : optique avant gauche à moins de 2 m (Golf, M4, 911), de trois quarts (l'orbite part du centre de la voiture)
    for (const char* id : {"golf", "m4", "p911"}) {
        if (!g.init(r, id, "ville")) continue;
        g.setAutopilot(false);
        for (int i = 0; i < 4; i++) image(i == 3);
        face(-0.30f, 0.75f, 3.7f);
        capture(std::string("phares-gros-plan-") + id);
        g.shutdown();
    }
    g.orbitHauteur = h0; g.orbitRecul = r0;
    // (1) voiture du joueur de face sur la chaussée (Golf), puis (3) trafic de la Ville vu de face : le trafic roule dans le même
    // sens que le joueur, la caméra est donc 14 m devant lui et regarde vers l'arrière (le joueur et les voitures qui le suivent)
    if (g.init(r, "golf", "ville")) {
        choisirVue(VueCamera::Poursuite);
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 300; i++) image(i % 8 == 0);
        face(0.f, 1.1f, 6.5f);
        capture("phares-joueur-face");
        g.camMode = 0;
        for (int i = 0; i < 1200; i++) image(i % 8 == 0);
        face(0.f, 1.5f, 14.f);
        capture("phares-trafic-face");
        g.orbitHauteur = h0; g.orbitRecul = r0;
        g.shutdown();
    }
    // témoin de jour : la même vue de face, optiques éteintes
    setenv("WFS_MOMENT", "jour", 1);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(false);
        for (int i = 0; i < 4; i++) image(i == 3);
        face(0.f, 0.75f, 4.6f);
        capture("phares-jour-golf");
        g.shutdown();
    }
    g.orbitHauteur = h0; g.orbitRecul = r0;
    unsetenv("WFS_MOMENT");
}

} // namespace wfs
