// WaForSpeed — WFS-492 : banc des skins (WFS_BANC=skins). Skin Rallye sur les 6 voitures du 1er lot (208, 500, GT86, Golf,
// Corvette C7 = id mustang, Giulia), carrosserie Bleu (catalogue PEINTURES), voiture du joueur en vitrine (Ville) :
//   - skins-<id>-jour : planche 2 × 2 (3/4 avant gauche, profil gauche, 3/4 arrière droit, 3/4 avant droit vu de haut) ;
//   - skins-<id>-nuit : même planche de nuit (golf, gt86, giulia) ;
//   - skins-golf-jonction : gros plan de la jonction porte avant / aile avant gauche (motifs sans couture, RG-04) ;
//   - skins-<id>-course-jour, -nuit : en course, caméra de poursuite (golf, gt86, giulia) ;
//   - skins-golf-sans : la même vitrine sans skin (aspect d'avant).
// Journal : géométrie de chaque voiture (boîte, roues : outil tools/skins/generer.py), texture posée, mémoire des textures du GPU
// avant / après, nombre de textures d'habillage en mémoire (RG-03 : une seule). WFS_SKINS_VOITURES="id …" : seulement ces voitures.
// Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche).
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
#include "vehicule_skin.h"

namespace wfs {
namespace {

bool g_actif = false, g_repeindre = false;
vec3 g_peinture{};
const Model* g_modele = nullptr;   // modèle de la voiture du joueur (GameView::modele), lu pour le journal de la géométrie

vec3 lin(unsigned h) { auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }

// actif seulement pendant ce banc : repeint la voiture (GameView::choisirVoiture, comme « Choisir » du garage)
class SkinsBanc final : public System {
public:
    const char* name() const override { return "skins-banc"; }
    void update(double, const GameView& g) override {
        if (!g_actif) return;
        if (g_repeindre && g.car && g.choisirVoiture) { g.choisirVoiture(g.car->id, g_peinture); g_repeindre = false; }
        g_modele = g.modele;
    }
};

// copie la moitié centrale d'une capture (moyenne 2 × 2) dans la case (cx, cy) d'une planche de COL cases de CW × CH
void coller(const std::string& png, std::vector<uint8_t>& planche, int COL, int CW, int CH, int cx, int cy, float zoom) {
    int w = 0, h = 0, n = 0;
    stbi_uc* px = stbi_load(png.c_str(), &w, &h, &n, 3);
    if (!px) { WFS_ERR("skins : capture illisible (%s)", png.c_str()); return; }
    const int x0 = cx * CW, y0 = cy * CH, stride = COL * CW * 3;
    const float fw = w * zoom, fh = h * zoom;   // part de la capture gardée (centrée)
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
        const int sx = (int)((w - fw) * 0.5f + x * fw / CW), sy = (int)((h - fh) * 0.5f + y * fh / CH);
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

} // namespace

WFS_SYSTEM(SkinsBanc, 497)

void benchSkins(Game& g, Renderer& r, const std::string& out) {
    namespace vs = vehicule_skin;
    const PeintureDef* bleu = peintureDef("bleu");
    const unsigned BLEU = bleu ? bleu->hex : 0x1d3f8fu;
    std::vector<std::string> ids{"p208", "abarth", "gt86", "golf", "mustang", "giulia"};
    if (const char* l = std::getenv("WFS_SKINS_VOITURES")) { ids.clear(); std::istringstream is(l); std::string id; while (is >> id) ids.push_back(id); }
    auto avec = [&](const std::string& id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); };
    double t = 3000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); } };
    auto rendre = [&](const std::string& png) { g.render(t + 0.001); g.render(t + 0.002); r.capturePng(png); };
    auto capture = [&](const std::string& nom) { rendre(out + "/" + nom + ".png"); WFS_LOG("capture %s (WFS-492)", nom.c_str()); };
    const size_t texAvant = r.vk().octetsTextures;
    auto preparer = [&](const std::string& id, const char* skin) {
        setenv("WFS_SKIN", skin, 1);
        if (!g.init(r, id.c_str(), "ville")) { WFS_ERR("skins : %s impossible à charger", id.c_str()); return false; }
        g.setAutopilot(false); g.camMode = 2;
        g_actif = true; g_repeindre = true; g_peinture = lin(BLEU);
        images(5);   // choix appliqué entre deux images (modèle rechargé, skin reposée par Game::setCar)
        const vs::Etat e = vs::etat();
        const Model* m = g_modele;
        if (m) {
            WFS_LOG("skins : géométrie %s : boîte x %.3f..%.3f y %.3f..%.3f z %.3f..%.3f ; roues %d", id.c_str(), m->bmin.x, m->bmax.x, m->bmin.y,
                    m->bmax.y, m->bmin.z, m->bmax.z, m->nRoues);
            for (int k = 0; k < m->nRoues; k++)
                WFS_LOG("skins : géométrie %s : roue %d centre %.3f %.3f %.3f rayon %.3f", id.c_str(), k, m->roues[k].centre.x, m->roues[k].centre.y, m->roues[k].centre.z, m->roues[k].rayon);
        }
        WFS_LOG("skins : %s, skin « %s » : %d texture(s) d'habillage en mémoire, %.1f Ko, %d pièces habillées%s ; textures du GPU %.1f Mo (avant le banc %.1f Mo) (WFS-492, RG-03)",
                id.c_str(), *skin ? skin : "aucune", e.textures, e.octetsGpu / 1024.0, e.materiaux, e.decodeCpu ? ", EAC décodé sur le processeur" : "",
                r.vk().octetsTextures / 1048576.0, texAvant / 1048576.0);
        return true;
    };
    // caméra de vitrine : avantM devant, droiteM à droite de la voiture, à hauteur m (même calcul que vehicule_finitions_banc.cpp)
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    const float h0 = g.orbitHauteur, r0 = g.orbitRecul;
    auto finir = [&]() { g.orbitHauteur = h0; g.orbitRecul = r0; g_actif = false; g.shutdown(); unsetenv("WFS_SKIN"); };
    const std::string tmp = out + "/.skins-tmp.png";
    constexpr int COL = 2, LIG = 2, CW = 960, CH = 540;
    // 4 vues : 3/4 avant gauche, profil gauche, 3/4 arrière droit, 3/4 avant droit vu de haut
    const double VUES[4][3] = {{4.4, -3.4, 1.9}, {0.2, -6.2, 1.3}, {-4.4, 3.4, 2.0}, {3.0, 2.6, 4.4}};
    auto planche = [&](const std::string& nom) {
        std::vector<uint8_t> p((size_t)COL * CW * LIG * CH * 3, 18);
        for (int k = 0; k < 4; k++) { viser(VUES[k][0], VUES[k][1], (float)VUES[k][2]); rendre(tmp); coller(tmp, p, COL, CW, CH, k % COL, k / COL, 0.62f); }
        stbi_write_png((out + "/" + nom + ".png").c_str(), COL * CW, LIG * CH, 3, p.data(), COL * CW * 3);
        WFS_LOG("capture %s : 3/4 avant gauche, profil gauche, 3/4 arrière droit, dessus (WFS-492)", nom.c_str());
    };

    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        const bool nuit = std::string(moment) == "nuit";
        for (const std::string& id : ids) {
            if (nuit && id != "golf" && id != "gt86" && id != "giulia") continue;
            if (!preparer(id, "rallye")) continue;
            planche("skins-" + id + "-" + moment);
            if (id == "golf" && !nuit) { viser(1.3, -2.2, 1.0); capture("skins-golf-jonction"); }
            finir();
        }
    }
    // sans skin : même vitrine (aspect d'avant) ; le contrôle pixel près se fait sur les bancs existants (peinture2, finitions)
    setenv("WFS_MOMENT", "jour", 1);
    if (avec("golf") && preparer("golf", "")) { planche("skins-golf-sans"); finir(); }
    // en course, caméra de poursuite, jour et nuit
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const char* id : {"golf", "gt86", "giulia"}) {
            if (!avec(id) || !preparer(id, "rallye")) continue;
            g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            images(4);
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            const vs::Etat e = vs::etat();
            WFS_LOG("skins : en course (%s, %s) : %d texture(s) d'habillage en mémoire (RG-03)", id, moment, e.textures);
            capture(std::string("skins-") + id + "-course-" + moment);
            g.setAutopilot(false);
            finir();
        }
    }
    unsetenv("WFS_MOMENT");
    std::remove(tmp.c_str());
    const vs::Etat e = vs::etat();
    WFS_LOG("skins : fin du banc : %d texture(s) d'habillage en mémoire (0 attendu), textures du GPU %.1f Mo (avant le banc %.1f Mo)", e.textures,
            r.vk().octetsTextures / 1048576.0, texAvant / 1048576.0);
}

} // namespace wfs
