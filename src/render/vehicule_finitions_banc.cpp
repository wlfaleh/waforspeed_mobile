// WaForSpeed — WFS-490 : banc des finitions de la peinture (WFS_BANC=finitions). Carrosserie Rouge (catalogue PEINTURES), voiture
// du joueur en vitrine (Ville), les 8 finitions de vehicule_finitions.h, comme les captures demandées par le ticket :
//   - finitions-<id>-jour, -nuit : planche 4 × 2 (de gauche à droite et de haut en bas : brillant, métallisé, nacré, mat /
//     satiné, chromé, caméléon, candy), 3/4 avant gauche, GT86, Corvette (id mustang), 208 (RG-12 : 8 finitions × 3 voitures) ;
//   - finitions-gt86-<finition>-pres : gros plan de la portière de la GT86, les 8 finitions (paillettes, flop, satiné, mat), jour ;
//   - finitions-mustang-cameleon-1, -2, -3 : caméléon sur la Corvette sous 3 angles (la couleur change), jour ;
//   - finitions-p208-course-chrome, -mat : 208 en course de jour (caméra de poursuite), chromé puis mat ;
//   - finitions-p208-toit-<finition> : 208, toit et rétroviseurs en Noir (couleur 2, WFS-488), métallisé puis chromé : la
//     finition s'applique aussi à la couleur 2 (RG-11), 3/4 avant et de dessus, jour.
// WFS_FINITIONS_VOITURES="id id …" : seulement ces voitures pour les planches. Le brillant est l'aspect d'avant : contrôle par le
// banc peinture2 (mêmes images que main). Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
#include "vehicule_finitions.h"
#include "vehicule_peinture2.h"

namespace wfs {
namespace {

bool g_actif = false, g_repeindre = false;
vec3 g_peinture{};

vec3 lin(unsigned h) { auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }

// actif seulement pendant ce banc : repeint la voiture (GameView::choisirVoiture, comme « Choisir » du garage)
class FinitionsBanc final : public System {
public:
    const char* name() const override { return "finitions-banc"; }
    void update(double, const GameView& g) override {
        if (!g_actif) return;
        if (g_repeindre && g.car && g.choisirVoiture) { g.choisirVoiture(g.car->id, g_peinture); g_repeindre = false; }
    }
};

} // namespace

WFS_SYSTEM(FinitionsBanc, 498)

void benchFinitions(Game& g, Renderer& r, const std::string& out) {
    namespace vf = vehicule_finitions;
    const PeintureDef* rouge = peintureDef("rouge");
    const unsigned ROUGE = rouge ? rouge->hex : 0xC0141Cu;
    std::vector<std::string> ids{"gt86", "mustang", "p208"};
    if (const char* l = std::getenv("WFS_FINITIONS_VOITURES")) { ids.clear(); std::istringstream is(l); std::string id; while (is >> id) ids.push_back(id); }
    double t = 3000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); } };
    auto rendre = [&](const std::string& png) { g.render(t + 0.001); g.render(t + 0.002); r.capturePng(png); };
    auto capture = [&](const std::string& nom) { rendre(out + "/" + nom + ".png"); WFS_LOG("capture %s (WFS-490)", nom.c_str()); };
    auto preparer = [&](const std::string& id) {
        if (!g.init(r, id.c_str(), "ville")) { WFS_ERR("finitions : %s impossible à charger", id.c_str()); return false; }
        g.setAutopilot(false); g.camMode = 2;
        g_actif = true; g_repeindre = true; g_peinture = lin(ROUGE);
        images(5);   // choix appliqué entre deux images (modèle rechargé)
        return true;
    };
    // caméra de vitrine : avantM devant, droiteM à droite de la voiture, à hauteur m (même calcul que vehicule_peinture2_banc.cpp)
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    const float h0 = g.orbitHauteur, r0 = g.orbitRecul;
    auto finir = [&]() { g.orbitHauteur = h0; g.orbitRecul = r0; g_actif = false; r.finitionJoueur(0); g.shutdown(); };
    const std::string tmp = out + "/.finitions-tmp.png";

    // planches : 8 finitions × chaque voiture, de jour puis de nuit (4 × 2 cases de 480 × 270, centre de l'image)
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const std::string& id : ids) {
            if (!preparer(id)) continue;
            constexpr int COL = 4, LIG = 2, CW = 480, CH = 270;
            std::vector<uint8_t> planche((size_t)COL * CW * LIG * CH * 3, 18);
            viser(4.6, -3.6, 2.4);
            for (int f = 0; f < vf::NOMBRE; f++) {
                r.finitionJoueur(f);
                rendre(tmp);
                int w = 0, h = 0, n = 0;
                stbi_uc* px = stbi_load(tmp.c_str(), &w, &h, &n, 3);
                if (!px) { WFS_ERR("finitions : capture illisible (%s, %s)", id.c_str(), vf::IDS[f]); continue; }
                const int x0 = (f % COL) * CW, y0 = (f / COL) * CH, stride = COL * CW * 3;
                for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {   // moyenne 2 × 2 sur la moitié centrale (pas de crénelage)
                    const int sx = w / 4 + x * (w / 2) / CW, sy = h / 4 + y * (h / 2) / CH;
                    uint8_t* d = &planche[(size_t)(y0 + y) * stride + (size_t)(x0 + x) * 3];
                    for (int c = 0; c < 3; c++) {
                        int s = 0;
                        for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++)
                            s += px[((size_t)std::min(h - 1, sy + dy) * w + std::min(w - 1, sx + dx)) * 3 + c];
                        d[c] = (uint8_t)(s / 4);
                    }
                }
                stbi_image_free(px);
            }
            const std::string nom = "finitions-" + id + "-" + moment;
            stbi_write_png((out + "/" + nom + ".png").c_str(), COL * CW, LIG * CH, 3, planche.data(), COL * CW * 3);
            WFS_LOG("capture %s : planche Rouge, brillant, métallisé, nacré, mat / satiné, chromé, caméléon, candy (WFS-490)", nom.c_str());
            // gros plans de jour sur la GT86 : paillettes, flop et teinte du candy, vus de près (portière avant gauche)
            if (id == "gt86" && std::string(moment) == "jour")
                for (int f = 0; f < vf::NOMBRE; f++) {
                    r.finitionJoueur(f); viser(0.4, -1.9, 1.1);
                    capture(std::string("finitions-gt86-") + vf::IDS[f] + "-pres");
                }
            // caméléon sur la Corvette sous 3 angles (de face à 3/4, de profil, de 3/4 arrière), jour
            if (id == "mustang" && std::string(moment) == "jour") {
                r.finitionJoueur(vf::CAMELEON);
                viser(4.6, -2.0, 1.6); capture("finitions-mustang-cameleon-1");
                viser(0.3, -5.4, 1.4); capture("finitions-mustang-cameleon-2");
                viser(-4.2, -3.4, 2.8); capture("finitions-mustang-cameleon-3");
            }
            finir();
        }
    }
    std::remove(tmp.c_str());
    // course de jour : 208, caméra de poursuite, chromé puis mat
    setenv("WFS_MOMENT", "jour", 1);
    if (preparer("p208")) {
        g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
        images(4);
        for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
        r.finitionJoueur(vf::CHROME); capture("finitions-p208-course-chrome");
        for (int i = 0; i < 60; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
        r.finitionJoueur(vf::MAT); capture("finitions-p208-course-mat");
        g.setAutopilot(false);
        finir();
    }
    // couleur 2 (toit et rétroviseurs Noir) avec une finition : 208, 3/4 avant gauche et de dessus, jour
    const PeintureDef* noir = peintureDef("noir");
    if (noir && preparer("p208")) {
        g.setPeinture2(vehicule_peinture2::code(noir->hex));   // après le rechargement (setCar relit la sauvegarde)
        images(2);
        for (int f : {(int)vf::METALLISE, (int)vf::CHROME}) {
            r.finitionJoueur(f);
            viser(4.0, -3.2, 2.3); capture(std::string("finitions-p208-toit-") + vf::IDS[f]);
            viser(1.2, -0.9, 5.2); capture(std::string("finitions-p208-toit-") + vf::IDS[f] + "-dessus");
        }
        finir();
    }
    unsetenv("WFS_MOMENT");
}

} // namespace wfs
