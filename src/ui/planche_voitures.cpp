// WaForSpeed — planche de contrôle des 20 voitures (WFS-77) : voir planche_voitures.h.
#include "planche_voitures.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/roues.h"
#include "../render/renderer.h"
#include "text.h"

namespace wfs {
namespace {

bool g_actif = false, g_repeindre = false;
std::string g_nom;
constexpr unsigned TURQUOISE = 0x0f8f8c;   // peinture « Turquoise » du garage (PAINTS du jeu web)

vec3 lin(unsigned h) { auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }

// système actif seulement pendant la planche : repeint la voiture (GameView::choisirVoiture, comme « Choisir » du garage)
// et écrit son nom en bas de l'image
class PlancheVoitures : public System {
public:
    const char* name() const override { return "planche des voitures"; }
    void update(double, const GameView& g) override {
        if (g_actif && g_repeindre && g.car && g.choisirVoiture) { g.choisirVoiture(g.car->id, lin(TURQUOISE)); g_repeindre = false; }
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!g_actif || !ui::fontReady()) return;
        const float W = g.uiWidth, H = g.uiHeight, k = H / 720.f;
        r.uiRect(0, H - 120 * k, W, 120 * k, {0.04f, 0.045f, 0.06f, 0.82f});
        ui::text(r, W / 2, H - 100 * k, 80 * k, ui::majuscules(g_nom), {1, 1, 1, 1}, ui::CENTRE, 0.02f, false);
    }
};

// roues séparées et matériaux de peinture d'un .wfm (même lecture que tests/test_roues.cpp)
void controler(const CarDef& c, int& roues, int& peintures) {
    roues = 0; peintures = 0;
    std::vector<uint8_t> b;
    if (!readAsset(c.file, b) || b.size() < 44 || std::memcmp(b.data(), "WFM1", 4) != 0) return;
    size_t o = 4;
    auto u32 = [&]() { uint32_t x = 0; if (o + 4 <= b.size()) std::memcpy(&x, &b[o], 4); o += 4; return x; };
    u32(); const uint32_t nT = u32(), nM = u32(), nMe = u32(); o += 24;
    for (uint32_t t = 0; t < nT && o < b.size(); t++) o += u32();
    std::vector<uint32_t> flags;
    for (uint32_t m = 0; m < nM; m++) { o += 52; flags.push_back(u32()); if (flags.back() & MAT_PAINT) peintures++; }
    std::vector<std::vector<float>> v; std::vector<std::vector<uint32_t>> ix; std::vector<roues::Maillage> ms;
    for (uint32_t m = 0; m < nMe && o < b.size(); m++) {
        const uint32_t mat = u32(), nv = u32(), ni = u32();
        if (o + (size_t)nv * 32 + (size_t)ni * 4 > b.size()) return;
        v.emplace_back((size_t)nv * 8); std::memcpy(v.back().data(), &b[o], (size_t)nv * 32); o += (size_t)nv * 32;
        ix.emplace_back(ni); std::memcpy(ix.back().data(), &b[o], (size_t)ni * 4); o += (size_t)ni * 4;
        ms.push_back({v.back().data(), 8, nv, ix.back().data(), ni, mat < flags.size() && (flags[mat] & MAT_WHEEL) != 0});
    }
    // les pointeurs de ms restent valides : v et ix ne bougent plus (chaque vecteur garde son propre tampon)
    for (size_t k = 0; k < ms.size(); k++) { ms[k].sommets = v[k].data(); ms[k].idx = ix[k].data(); }
    roues = roues::decouper(ms).n;
}

} // namespace

void benchPlancheVoitures(Game& g, Renderer& r, const std::string& out) {
    constexpr int COL = 5, LIG = 4, CW = 256, CH = 144;
    std::vector<uint8_t> planche((size_t)COL * CW * LIG * CH * 3, 18);
    const std::string tmp = out + "/.planche-tmp.png";
    int place = 0;
    for (int i = 0; i < CAR_COUNT && place < COL * LIG; i++, place++) {
        const CarDef& c = CARS[i];
        if (!g.init(r, c.id, "ville")) { WFS_ERR("planche : %s impossible à charger", c.id); continue; }
        g.setAutopilot(false); g.camMode = 2; g.orbit = 0.7f;
        g_actif = true; g_repeindre = true; g_nom = std::string(marque(c)) + " " + modele(c);   // WFS-191
        for (int k = 0; k < 3; k++) { g.update(1.0 / 60); g.render(2000 + k / 60.0); }
        g.render(2001); g.render(2001.01);
        r.capturePng(tmp);
        g_actif = false;
        int roues = 0, peintures = 0;
        controler(c, roues, peintures);
        WFS_LOG("planche : %-9s roues %d/4, peinture %d matériau%s (Turquoise)%s", c.id, roues, peintures, peintures > 1 ? "x" : "",
                roues == 4 && peintures > 0 ? "" : " — À CORRIGER");
        int w = 0, h = 0, n = 0;
        stbi_uc* px = stbi_load(tmp.c_str(), &w, &h, &n, 3);
        if (!px) { WFS_ERR("planche : capture de %s illisible", c.id); g.shutdown(); continue; }
        // réduction par moyenne de blocs (image entière, proportions 16:9 gardées), case (place % COL, place / COL)
        const int x0 = (place % COL) * CW, y0 = (place / COL) * CH, stride = COL * CW * 3;
        for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
            const int ax = x * w / CW, bx = std::max(ax + 1, (x + 1) * w / CW), ay = y * h / CH, by = std::max(ay + 1, (y + 1) * h / CH);
            unsigned s[3] = {0, 0, 0}, nb = 0;
            for (int yy = ay; yy < by; yy++) for (int xx = ax; xx < bx; xx++) { const stbi_uc* p = px + ((size_t)yy * w + xx) * 3; s[0] += p[0]; s[1] += p[1]; s[2] += p[2]; nb++; }
            uint8_t* d = &planche[(size_t)(y0 + y) * stride + (size_t)(x0 + x) * 3];
            for (int k = 0; k < 3; k++) d[k] = (uint8_t)(s[k] / std::max(1u, nb));
        }
        stbi_image_free(px);
        g.shutdown();
    }
    std::remove(tmp.c_str());
    stbi_write_png((out + "/vitrines-planche.png").c_str(), COL * CW, LIG * CH, 3, planche.data(), COL * CW * 3);
    WFS_LOG("capture vitrines-planche : %d voitures en Turquoise (%d × %d)", place, COL, LIG);
}

WFS_SYSTEM(PlancheVoitures, 498)   // interface (400-499) : tout à la fin, au-dessus du reste ; inactif hors de la planche

} // namespace wfs
