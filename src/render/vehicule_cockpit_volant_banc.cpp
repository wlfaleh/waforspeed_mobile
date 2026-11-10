// WaForSpeed — WFS-414 : banc « cockpit-volant » (voir vehicule_cockpit_volant_banc.h).
// Le gros plan est pris dans l'image entière (aucun autre rendu) : la jante est projetée avec la caméra de l'image (vue et
// projection relevées par un système inactif hors de ce banc), puis la zone est agrandie 2 fois (filtre bilinéaire) ; une copie
// éclaircie (…-gros-plan-eclairci) montre les formes dans l'ombre du toit (aide à la revue : ce n'est pas l'image du jeu).
// Le banc n'utilise que ce qui existe aussi sur main (captures « avant » : main + ce banc seul).
#include "vehicule_cockpit_volant_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../core/ecran.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/camera_vue.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/ressenti.h"
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "renderer.h"
#include "vehicule_cockpit.h"

namespace wfs {

namespace {
// caméra de la dernière image (banc seulement : relevée par le système ci-dessous quand `actif` est vrai)
struct CameraImage { bool actif = false, vue = false; mat4 view = mat4::identity(), proj = mat4::identity(); };
CameraImage& cameraImage() { static CameraImage c; return c; }

class VolantBancCamera : public System {
public:
    const char* name() const override { return "volant-banc-camera"; }
    void render(Renderer&, const GameView& g) override {
        CameraImage& c = cameraImage();
        if (!c.actif) return;   // hors du banc « cockpit-volant » : rien
        c.view = g.view; c.proj = g.proj; c.vue = true;
    }
};
WFS_SYSTEM(VolantBancCamera, 399)

// point du repère du volant (plan xy, +z vers le conducteur) → pixel de l'image (x, y depuis le coin haut gauche) ; faux derrière la caméra
bool projeter(const cockpit::InterieurDef& d, float angleVolant, vec3 p, int W, int H, float& px, float& py) {
    const CameraImage& c = cameraImage();
    const mat4 M = etatCockpit().repere * translate(d.volantPos) * rotateX(d.volantIncl) * rotateZ(angleVolant);
    const vec3 v = c.view.transformPoint(M.transformPoint(p));   // repère de la caméra
    const mat4& P = c.proj;
    const float qx = P.at(0, 0) * v.x + P.at(0, 1) * v.y + P.at(0, 2) * v.z + P.at(0, 3), qy = P.at(1, 0) * v.x + P.at(1, 1) * v.y + P.at(1, 2) * v.z + P.at(1, 3);
    const float qw = P.at(3, 0) * v.x + P.at(3, 1) * v.y + P.at(3, 2) * v.z + P.at(3, 3);
    if (qw <= 1e-4f) return false;
    px = (qx / qw * 0.5f + 0.5f) * (float)W;
    py = (qy / qw * 0.5f + 0.5f) * (float)H;   // projection Vulkan : y de l'écran vers le bas
    return true;
}

// gros plan : rectangle 4:3 autour de la jante (+ 15 %), agrandi 2 fois ; rend faux si l'image ne se lit pas
bool grosPlan(const cockpit::InterieurDef& d, float angleVolant, const std::string& png, const std::string& sortie) {
    int W, H, n;
    unsigned char* img = stbi_load(png.c_str(), &W, &H, &n, 3);
    if (!img) return false;
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    const float R = d.volantD * 0.5f;
    for (int k = 0; k < 48; k++) {
        const float a = 2 * PI * k / 48;
        float px, py;
        if (projeter(d, angleVolant, {std::cos(a) * R, std::sin(a) * R, 0}, W, H, px, py)) {
            x0 = std::min(x0, px); x1 = std::max(x1, px); y0 = std::min(y0, py); y1 = std::max(y1, py);
        }
    }
    if (x1 <= x0) { stbi_image_free(img); return false; }
    const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
    float h = std::max(y1 - y0, (x1 - x0) * 0.75f) * 1.15f, w = h * 4.f / 3.f;
    h = std::min(h, (float)H); w = std::min(w, (float)W);
    const float ox = std::clamp(cx - w * 0.5f, 0.f, (float)W - w), oy = std::clamp(cy - h * 0.5f, 0.f, (float)H - h);
    const int OW = (int)(w * 2), OH = (int)(h * 2);
    std::vector<unsigned char> out((size_t)OW * OH * 3);
    for (int y = 0; y < OH; y++) for (int x = 0; x < OW; x++) {
        const float sx = std::clamp(ox + (x + 0.5f) * 0.5f - 0.5f, 0.f, (float)W - 1.001f), sy = std::clamp(oy + (y + 0.5f) * 0.5f - 0.5f, 0.f, (float)H - 1.001f);
        const int ix = (int)sx, iy = (int)sy; const float fx = sx - ix, fy = sy - iy;
        for (int c = 0; c < 3; c++) {
            auto at = [&](int xx, int yy) { return (float)img[((size_t)yy * W + xx) * 3 + c]; };
            const float v = (at(ix, iy) * (1 - fx) + at(ix + 1, iy) * fx) * (1 - fy) + (at(ix, iy + 1) * (1 - fx) + at(ix + 1, iy + 1) * fx) * fy;
            out[((size_t)y * OW + x) * 3 + c] = (unsigned char)std::lround(std::clamp(v, 0.f, 255.f));
        }
    }
    stbi_image_free(img);
    // copie éclaircie pour la revue (l'habitacle est sombre sous le toit) : niveaux 0..56 étirés sur 0..255, comme « -level 0%,22% »
    std::vector<unsigned char> clair(out.size());
    for (size_t k = 0; k < out.size(); k++) clair[k] = (unsigned char)std::min(255, out[k] * 255 / 56);
    const std::string nomClair = sortie.substr(0, sortie.size() - 4) + "-eclairci.png";
    stbi_write_png(nomClair.c_str(), OW, OH, 3, clair.data(), OW * 3);
    WFS_LOG("gros plan %s (WFS-414) : jante de %.0f × %.0f px dans l'image %d × %d, zone %.0f × %.0f px à (%.0f, %.0f), agrandie 2 fois",
            sortie.c_str(), x1 - x0, y1 - y0, W, H, w, h, ox, oy);
    return stbi_write_png(sortie.c_str(), OW, OH, 3, out.data(), OW * 3) != 0;
}
} // namespace

void benchCockpitVolant(Game& g, Renderer& r, const std::string& out, std::vector<std::string> voitures) {
    if (voitures.empty()) voitures = {"mustang", "golf", "abarth", "tcross", "s3", "giulia", "m4", "p911"};
    const VueCamera vueAvant = vueCamera();
    const RegardCockpit regardAvant = regardCockpit();
    const bool reduireAvant = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;   // tête immobile : même cadrage d'une voiture à l'autre
    cameraImage().actif = true;
    double t = 1500;
    auto rouler = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    auto course = [&](const std::string& car, bool nuit) {
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        if (!g.init(r, car.c_str(), "ville")) { WFS_ERR("cockpit-volant : chargement impossible (%s)", car.c_str()); return false; }
        choisirVue(VueCamera::Cockpit); choisirRegard(RegardCockpit::Normal); g.camMode = 0; g.setAutopilot(true); g.allerA(Ecran::Course);
        rouler(60 * 3 + 40);   // décompte puis départ (ligne droite du début de la Ville)
        return true;
    };
    auto capture = [&](const cockpit::InterieurDef& d, const std::string& nom) {
        g.render(t); g.render(t + 1.0 / 60); t += 1.0 / 60;
        const std::string png = out + "/" + nom + ".png";
        r.capturePng(png);
        const float angle = etatCockpit().volant;
        if (!cameraImage().vue || !grosPlan(d, angle, png, out + "/" + nom + "-gros-plan.png"))
            WFS_ERR("cockpit-volant : gros plan impossible (%s)", nom.c_str());
        WFS_LOG("capture %s (WFS-414) : voiture %s, volant %+.2f rad, %.0f km/h", nom.c_str(), d.car, angle, g.player().kmh());
    };
    // suite de 4 images d'un braquage à gauche (doigt glissé dans la zone de braquage, comme le banc « cockpit-reference »)
    auto braquage = [&](const cockpit::InterieurDef& d, const std::string& base) {
        const float W = (float)r.width(), H = (float)r.height();
        g.setAutopilot(false);
        g.onTouch({32, W * 0.20f, H * 0.70f, true});
        for (int k = 1; k <= 4; k++) {
            g.onTouch({32, W * (0.20f - 0.02f * k), H * 0.70f, true});
            rouler(8); capture(d, base + "-braquage-" + std::to_string(k));
        }
        g.onTouch({32, W * 0.12f, H * 0.70f, false});
        g.setAutopilot(true);
    };
    for (const std::string& car : voitures) {
        const cockpit::InterieurDef* d = cockpit::interieurPour(car.c_str());
        if (!d) { WFS_ERR("cockpit-volant : %s n'a pas d'intérieur", car.c_str()); continue; }
        if (course(car, false)) {
            capture(*d, "volant-" + car + "-jour");
            if (car == "mustang" || car == "golf") braquage(*d, "volant-" + car);
        }
        if (car == "mustang" && course(car, true)) capture(*d, "volant-" + car + "-nuit");
    }
    cameraImage() = {};
    g.shutdown();
    unsetenv("WFS_MOMENT");
    reglagesRessenti().reduireMouvements = reduireAvant;
    choisirRegard(regardAvant);
    choisirVue(vueAvant);
}

} // namespace wfs
