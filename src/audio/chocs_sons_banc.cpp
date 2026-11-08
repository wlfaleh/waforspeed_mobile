// WaForSpeed — son des chocs (WFS-298) : banc d'essai PC (RG-08), écouté et regardé par la QA. Même synthèse que le jeu
// (SonsChocs::choc puis mix, blocs de 400 échantillons), coup sourd actuel de la carrosserie au même instant (réglages de sfx.cpp) :
//   chocs-<famille>.wav       : 15, 30, 50 et 80 km/h (80 : accident), obstacle à droite (4 premiers chocs) puis à gauche ;
//   chocs-<famille>-avant.wav : la même scène avec le seul coup sourd actuel (au centre), comme dans main ;
//   chocs-familles.wav        : les 5 familles à 30 km/h, obstacle à droite ;
// .png : en haut, niveaux gauche (cyan) et droite (orange), 0 à −70 dB ; en bas, spectre au tiers d'octave (100 Hz - 16 kHz,
// −100 à −40 dB) du son de l'obstacle seul : un choc par couleur (blanc 15, jaune 30, rouge 50, vert 80 km/h ; familles : poteau,
// arbre, mobilier, béton, voiture en blanc, vert, jaune, gris, rouge).
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../game/track.h"
#include "chocs_sons.h"
#include "voitures_sons.h"

namespace wfs::sonchocs {
namespace {

constexpr int R = 48000, PAS = R / 120;
constexpr float MASTER = 0.5f, ECART = 1.7f;   // s entre deux chocs (le métal sonne 1,2 s)

void wavStereo(const std::string& path, const std::vector<float>& G, const std::vector<float>& D) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t x) { f.write((const char*)&x, 4); };
    auto u16 = [&](uint16_t x) { f.write((const char*)&x, 2); };
    const uint32_t bytes = (uint32_t)G.size() * 4;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(2); u32(R); u32(R * 4); u16(4); u16(16);
    f.write("data", 4); u32(bytes);
    for (size_t i = 0; i < G.size(); i++)
        for (float x : {G[i], D[i]}) { const int16_t s = (int16_t)std::lround(std::clamp(x, -1.f, 1.f) * 32767); f.write((const char*)&s, 2); }
}
float dB(double x) { return 20.f * (float)std::log10(std::max(x, 1e-9)); }

struct Choc { float t, kmh; int famille; bool droite; };
struct Scene { std::vector<float> G, D, oG, oD; };   // sortie complète ; son de l'obstacle seul

Scene rendre(const std::vector<Choc>& chocs, float duree, bool avecObstacle) {
    Scene s; const size_t N = (size_t)(duree * R);
    s.G.assign(N, 0.f); s.D.assign(N, 0.f); s.oG.assign(N, 0.f); s.oD.assign(N, 0.f);
    const std::vector<float> petit = coupSourd((float)R, false), acc = coupSourd((float)R, true);
    SonsChocs sc; sc.build((float)R);
    std::vector<float> bus(PAS);
    size_t k = 0;
    for (size_t a = 0; a + PAS <= N; a += PAS) {
        for (; k < chocs.size() && chocs[k].t * R <= (float)a; k++) {
            const Choc& c = chocs[k];
            const float v = c.kmh / 3.6f; const bool accident = v >= 14.f;
            // coup sourd actuel (sfx.cpp) : petit choc au gain 0,4 + impact, accident au gain 1 ; au centre, comme dans main
            const std::vector<float>& b = accident ? acc : petit;
            const float g = (accident ? 1.f : 0.4f + v / (float)(MAX_SPEED * M)) * MASTER;
            for (size_t i = 0; i < b.size() && a + i < N; i++) { s.G[a + i] += b[i] * g; s.D[a + i] += b[i] * g; }
            if (avecObstacle) { const voitures::Pan p = voitures::panExistant(c.droite ? 1.5707963f : -1.5707963f); sc.choc(c.famille, v, p.g, p.d, accident); }
        }
        std::fill(bus.begin(), bus.end(), 0.f);
        sc.mix(s.oG.data() + a, s.oD.data() + a, bus.data(), PAS, MASTER);
    }
    for (size_t i = 0; i < N; i++) { s.G[i] += s.oG[i]; s.D[i] += s.oD[i]; }
    return s;
}

void image(const std::string& path, const Scene& s, const std::vector<std::pair<float, float>>& fenetres, const std::vector<std::array<uint8_t, 3>>& couleurs) {
    const int W = 1000, H = 400, HN = 230;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, std::array<uint8_t, 3> k) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = k[0]; p[1] = k[1]; p[2] = k[2];
    };
    for (int d = 0; d <= 70; d += 10) for (int x = 0; x < W; x += 2) px(x, (int)(d / 70.f * (HN - 1)), {60, 60, 70});
    for (int x = 0; x < W; x++) px(x, HN + 2, {90, 90, 100});
    const size_t w = R / 40, N = s.G.size();
    for (size_t a = 0; a + w <= N; a += w) {
        double eg = 0, ed = 0;
        for (size_t i = a; i < a + w; i++) { eg += (double)s.G[i] * s.G[i]; ed += (double)s.D[i] * s.D[i]; }
        const int x = (int)(a * (W - 1) / N);
        for (const auto& [l, k] : {std::pair{dB(std::sqrt(eg / (double)w)), std::array<uint8_t, 3>{0, 229, 255}},
                                   std::pair{dB(std::sqrt(ed / (double)w)), std::array<uint8_t, 3>{255, 140, 0}}})
            if (l > -70) for (int dx = 0; dx < 3; dx++) for (int dy = -1; dy <= 1; dy++) px(x + dx, (int)(-l / 70.f * (HN - 1)) + dy, k);
    }
    for (int dec : {100, 1000, 10000}) { const int x = (int)(std::log10(dec / 100.f) / std::log10(160.f) * (W - 1)); for (int y = HN + 4; y < H; y += 3) px(x, y, {70, 70, 80}); }
    for (size_t p = 0; p < fenetres.size() && p < couleurs.size(); p++) {
        std::vector<float> o(s.oG.size()); for (size_t i = 0; i < o.size(); i++) o[i] = s.oG[i] + s.oD[i];
        const size_t a = (size_t)(fenetres[p].first * R), n = (size_t)((fenetres[p].second - fenetres[p].first) * R);
        int yAvant = -1;
        for (int x = 0; x < W; x += 10) {
            const float f = 100.f * std::pow(160.f, (float)x / (W - 1));
            const float l = niveauTiers(o, a, n, (float)R, f);
            const int y = H - 2 - (int)(std::clamp((l + 100) / 60.f, 0.f, 1.f) * (H - HN - 8));
            if (yAvant >= 0) for (int dx = -10; dx <= 0; dx++) { const int yy = yAvant + (y - yAvant) * (dx + 10) / 10; px(x + dx, yy, couleurs[p]); px(x + dx, yy + 1, couleurs[p]); }
            yAvant = y;
        }
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

// fréquence (Hz) de la bande d'un tiers d'octave la plus forte du son de l'obstacle (de 63 Hz à 12,5 kHz)
float dominante(const std::vector<float>& o, size_t a, size_t n) {
    float best = -1e9f, fb = 0;
    for (float f = 63; f < 13000; f *= 1.2599f) { const float l = niveauTiers(o, a, n, (float)R, f); if (l > best) { best = l; fb = f; } }
    return fb;
}

} // namespace

void benchChocs(const std::string& out) {
    const std::array<std::array<uint8_t, 3>, 4> cV = {{{240, 240, 240}, {255, 220, 0}, {255, 60, 60}, {0, 220, 120}}};
    WFS_LOG("son (WFS-298) : tableau des familles (30 km/h, obstacle à droite) : crête / coup sourd, T60 mesuré (retenu), fréquence dominante, droite − gauche");
    for (int f = 0; f < NB_FAMILLES; f++) {
        std::vector<Choc> chocs;
        const float vitesses[4] = {15, 30, 50, 80};
        for (int k = 0; k < 8; k++) chocs.push_back({0.3f + k * ECART, vitesses[k % 4], f, k < 4});
        const float duree = 0.3f + 8 * ECART;
        const std::string nom = std::string("chocs-") + (f == BETON ? "mur" : nomFamille(f));
        const Scene apres = rendre(chocs, duree, true), avant = rendre(chocs, duree, false);
        wavStereo(out + "/" + nom + ".wav", apres.G, apres.D);
        wavStereo(out + "/" + nom + "-avant.wav", avant.G, avant.D);
        std::vector<std::pair<float, float>> fen;
        for (int k = 0; k < 4; k++) fen.push_back({chocs[(size_t)k].t, chocs[(size_t)k].t + ECART - 0.05f});
        image(out + "/" + nom + ".png", apres, fen, {cV.begin(), cV.end()});
        image(out + "/" + nom + "-avant.png", avant, fen, {cV.begin(), cV.end()});
        // tableau : choc à 30 km/h à droite (2e choc)
        const size_t a = (size_t)(chocs[1].t * R), n = (size_t)((ECART - 0.05f) * R);
        float pO = 0, pC = 0; double eG = 0, eD = 0;
        for (size_t i = a; i < a + n; i++) { pO = std::max(pO, std::max(std::fabs(apres.oG[i]), std::fabs(apres.oD[i]))); pC = std::max(pC, std::fabs(avant.G[i])); eG += (double)apres.oG[i] * apres.oG[i]; eD += (double)apres.oD[i] * apres.oD[i]; }
        std::vector<float> o(apres.oG.begin() + (long)a, apres.oG.begin() + (long)(a + n));
        for (size_t i = 0; i < o.size(); i++) o[i] += apres.oD[a + i];
        const float fMode = matiere(f).modes[0].f;
        const float t60 = mesurerT60(o, (float)R, f == BETON ? 0.f : fMode);
        float pk80 = 0; const size_t a80 = (size_t)(chocs[3].t * R);
        for (size_t i = a80; i < a80 + n && i < apres.G.size(); i++) pk80 = std::max(pk80, std::max(std::fabs(apres.G[i]), std::fabs(apres.D[i])));
        WFS_LOG("son (WFS-298) : %s.wav : crête %.1f dB du coup sourd (règle : −6 à 0), T60 %.3f s (retenu %.3f, bande %s), dominante %.0f Hz, droite − gauche %.1f dB ; "
                "à 80 km/h (accident) crête totale %.3f (< 1)", nom.c_str(), dB(pO) - dB(pC), t60, t60Retenu(f), f == BETON ? "large" : (std::to_string((int)fMode) + " Hz").c_str(),
                dominante(o, 0, o.size()), 10 * std::log10(std::max(eD, 1e-24) / std::max(eG, 1e-24)), pk80);
    }
    {   // les 5 familles à 30 km/h
        std::vector<Choc> chocs; std::vector<std::pair<float, float>> fen;
        for (int f = 0; f < NB_FAMILLES; f++) { chocs.push_back({0.3f + f * ECART, 30, f, true}); fen.push_back({0.3f + f * ECART, 0.3f + (f + 1) * ECART - 0.05f}); }
        const Scene s = rendre(chocs, 0.3f + (float)NB_FAMILLES * ECART, true);
        wavStereo(out + "/chocs-familles.wav", s.G, s.D);
        image(out + "/chocs-familles.png", s, fen, {{240, 240, 240}, {0, 220, 120}, {255, 220, 0}, {150, 150, 150}, {255, 60, 60}});
        WFS_LOG("son (WFS-298) : chocs-familles.wav : poteau, arbre, mobilier, mur (béton), voiture à 30 km/h, à droite (1,7 s d'écart)");
    }
    {   // RG-07 : coût, toutes les voix actives (4 chocs de poteau, le son le plus long et le plus riche)
        SonsChocs sc; sc.build((float)R);
        std::vector<float> G(480), D(480), bus(480);
        for (int k = 0; k < VOIX; k++) sc.choc(POTEAU, 80 / 3.6f, 1, 1, false);
        sc.mix(G.data(), D.data(), bus.data(), 1, MASTER);
        const auto t0 = std::chrono::steady_clock::now();
        int b = 0;
        for (; b < 120 && sc.actives() == VOIX; b++) sc.mix(G.data(), D.data(), bus.data(), 480, MASTER);
        WFS_LOG("son (WFS-298) : coût des chocs : %.4f ms par bloc de 10 ms (%d voix actives, %d blocs ; ≤ 0,05 ms), aucun tampon de son",
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / std::max(1, b), VOIX, b);
    }
}

} // namespace wfs::sonchocs
