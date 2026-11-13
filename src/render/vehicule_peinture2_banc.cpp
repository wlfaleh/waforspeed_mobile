// WaForSpeed — WFS-488 : banc de la couleur secondaire (WFS_BANC=peinture2). Carrosserie Rouge, toit et coques de rétroviseurs
// en Noir (couleurs du catalogue, PEINTURES), comme les captures demandées par le ticket :
//   - peinture2-<id>-34, -dessus : 208, GT86, Corvette (id mustang), de 3/4 avant gauche et de dessus (Ville, jour) ;
//   - peinture2-<id>-retro, -joint : gros plans du rétroviseur gauche et du joint avant du pavillon ;
//   - peinture2-planche : les voitures permises (17) de 3/4 avant, en mosaïque (RG-01 : toit + rétros sur chaque voiture) ;
//   - peinture2-course-jour, -nuit : 208 en course (caméra de poursuite), Ville.
// WFS_PEINTURE2_VOITURES="id id …" : seulement ces voitures (gros plans et planche ; WFS_VOITURES reste au banc vitrines-feux). Sans couleur 2, l'aspect d'avant est contrôlé par le
// banc vitrines-feux (mêmes images que main). Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "renderer.h"
#include "vehicule_peinture2.h"

namespace wfs {
namespace {

bool g_actif = false, g_repeindre = false;
vec3 g_peinture{};
const Model* g_modele = nullptr;

vec3 lin(unsigned h) { auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }

// actif seulement pendant ce banc : repeint la voiture (GameView::choisirVoiture, comme « Choisir » du garage) et note son modèle
class Peinture2Banc final : public System {
public:
    const char* name() const override { return "peinture2-banc"; }
    void update(double, const GameView& g) override {
        if (!g_actif) return;
        g_modele = g.modele;
        if (g_repeindre && g.car && g.choisirVoiture) { g.choisirVoiture(g.car->id, g_peinture); g_repeindre = false; }
    }
};

unsigned hexDe(const char* id) { const PeintureDef* p = peintureDef(id); return p ? p->hex : 0; }

} // namespace

WFS_SYSTEM(Peinture2Banc, 497)

void benchPeinture2(Game& g, Renderer& r, const std::string& out) {
    const unsigned ROUGE = hexDe("rouge"), NOIR = hexDe("noir");
    std::vector<std::string> ids;
    if (const char* l = std::getenv("WFS_PEINTURE2_VOITURES")) { std::istringstream is(l); std::string id; while (is >> id) ids.push_back(id); }
    else for (int i = 0; i < CAR_COUNT; i++) if (!voitureRetiree(CARS[i].id)) ids.push_back(CARS[i].id);
    double t = 3000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); } };
    auto capture = [&](const std::string& nom) { g.render(t + 0.001); g.render(t + 0.002); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s (WFS-488)", nom.c_str()); };
    // charge la voiture, carrosserie Rouge, toit et rétros Noir ; faux si impossible
    auto preparer = [&](const std::string& id, const char* carte) {
        if (!g.init(r, id.c_str(), carte)) { WFS_ERR("peinture2 : %s impossible à charger", id.c_str()); return false; }
        g.setAutopilot(false); g.camMode = 2;
        g_actif = true; g_repeindre = true; g_peinture = lin(ROUGE); g_modele = nullptr;
        images(3);                                          // choix appliqué entre deux images (modèle rechargé)
        g.setPeinture2(vehicule_peinture2::code(NOIR));     // après le rechargement (setCar relit la sauvegarde)
        images(2);
        return true;
    };
    // caméra de vitrine : avantM devant, droiteM à droite de la voiture, à hauteur m (même calcul que police_banc.cpp)
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    // gros plan : la vitrine regarde toujours le centre de la voiture (+0,55 m) ; l'œil est posé sur la droite qui part de ce
    // point et passe par P (repère du modèle), à « recul » m au-delà de P : P est au centre de l'image, vu de près
    auto grosPlan = [&](vec3 P, float recul) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 L = v.carPos + vec3{0, 0.55f, 0}, Pm = v.carM.transformPoint(P), dir = normalize(Pm - L);
        const vec3 e = Pm + dir * recul - v.carPos;
        g.orbit = std::atan2(e.x, e.z); g.orbitRecul = std::sqrt(e.x * e.x + e.z * e.z); g.orbitHauteur = e.y;
    };
    const float h0 = g.orbitHauteur, r0 = g.orbitRecul;
    for (const char* id : {"p208", "gt86", "mustang"}) {
        if (std::find(ids.begin(), ids.end(), id) == ids.end() || !preparer(id, "ville")) continue;
        viser(4.0, -3.2, 2.3); capture(std::string("peinture2-") + id + "-34");
        viser(1.2, -0.9, 5.2); capture(std::string("peinture2-") + id + "-dessus");
        if (g_modele && g_modele->retroMesure[0]) { grosPlan((g_modele->retroMin[0] + g_modele->retroMax[0]) * 0.5f, 0.75f); capture(std::string("peinture2-") + id + "-retro"); }
        else WFS_ERR("peinture2 : %s : rétroviseur gauche non mesuré, pas de gros plan", id);
        if (g_modele) {   // joint avant gauche du pavillon (haut du pare-brise) : 36 % de la largeur à gauche, 42 % de la longueur
            const vec3 a = g_modele->bmin, b = g_modele->bmax;
            grosPlan({a.x + (b.x - a.x) * 0.14f, b.y - 0.03f, a.z + (b.z - a.z) * 0.42f}, 0.9f); capture(std::string("peinture2-") + id + "-joint");
        }
        g.orbitHauteur = h0; g.orbitRecul = r0; g_actif = false;
        g.shutdown();
    }
    // planche : 3/4 avant gauche de chaque voiture, centre de l'image réduit (5 × 4 cases de 384 × 216)
    {
        constexpr int COL = 5, LIG = 4, CW = 384, CH = 216;
        std::vector<uint8_t> planche((size_t)COL * CW * LIG * CH * 3, 18);
        const std::string tmp = out + "/.peinture2-tmp.png";
        int place = 0; std::string ordre;
        for (const std::string& id : ids) {
            if (place >= COL * LIG || !preparer(id, "ville")) continue;
            viser(4.6, -3.6, 2.6); g.render(t); g.render(t + 0.001); r.capturePng(tmp);
            g.orbitHauteur = h0; g.orbitRecul = r0; g_actif = false;
            g.shutdown();
            int w = 0, h = 0, n = 0;
            stbi_uc* px = stbi_load(tmp.c_str(), &w, &h, &n, 3);
            if (!px) { WFS_ERR("peinture2 : capture de %s illisible", id.c_str()); continue; }
            const int x0 = (place % COL) * CW, y0 = (place / COL) * CH, stride = COL * CW * 3;   // centre (moitié de l'image)
            for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
                const int sx = w / 4 + x * (w / 2) / CW, sy = h / 4 + y * (h / 2) / CH;
                const stbi_uc* q = px + ((size_t)std::min(h - 1, sy) * w + std::min(w - 1, sx)) * 3;
                uint8_t* d = &planche[(size_t)(y0 + y) * stride + (size_t)(x0 + x) * 3];
                d[0] = q[0]; d[1] = q[1]; d[2] = q[2];
            }
            stbi_image_free(px);
            ordre += (ordre.empty() ? "" : ", ") + id; place++;
        }
        std::remove(tmp.c_str());
        stbi_write_png((out + "/peinture2-planche.png").c_str(), COL * CW, LIG * CH, 3, planche.data(), COL * CW * 3);
        WFS_LOG("capture peinture2-planche : %d voitures (Rouge, toit et rétroviseurs Noir), de gauche à droite et de haut en bas : %s (WFS-488)", place, ordre.c_str());
    }
    // course : 208, caméra de poursuite, de jour puis de nuit
    if (std::find(ids.begin(), ids.end(), "p208") != ids.end())
        for (const char* moment : {"jour", "nuit"}) {
            setenv("WFS_MOMENT", moment, 1);
            if (!preparer("p208", "ville")) continue;
            g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            images(4);
            g.setPeinture2(vehicule_peinture2::code(NOIR));   // l'écran Course recharge-t-il la voiture ? la couleur 2 reste posée
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            capture(std::string("peinture2-course-") + moment);
            g.setAutopilot(false); g_actif = false;
            g.shutdown();
        }
    unsetenv("WFS_MOMENT");
}

} // namespace wfs
